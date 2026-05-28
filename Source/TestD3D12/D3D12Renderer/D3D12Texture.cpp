// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "D3D12Renderer.h"
#include "D3D12Texture.h"
#include "D3D12CommandList.h"
#include "D3D12DescriptorPool.h"
#include "D3D12FrameThreadData.h"
#include "Shaders/ShaderInterop.h"

void D3D12Texture::Release() {
    for (const D3D12SRVEntry &srvEntry : srvEntries) {
        if (srvEntry.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(srvEntry.cpuDescriptorHandle);
        }
        if (srvEntry.bindlessIndex != D3D12BindlessDescriptorAllocator::NullIndex) {
            D3D12Renderer::GetRenderer()->FreeBindlessResourceDescriptor(srvEntry.bindlessIndex);
        }
    }

    for (const D3D12UAVEntry &uavEntry : uavEntries) {
        if (uavEntry.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(uavEntry.cpuDescriptorHandle);
        }
        if (uavEntry.bindlessIndex != D3D12BindlessDescriptorAllocator::NullIndex) {
            D3D12Renderer::GetRenderer()->FreeBindlessResourceDescriptor(uavEntry.bindlessIndex);
        }
    }

    for (const D3D12RTVEntry &rtvEntry : rtvEntries) {
        if (rtvEntry.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->rtvCpuDescriptorPool->Free(rtvEntry.cpuDescriptorHandle);
        }
    }

    for (const D3D12DSVEntry &dsvEntry : dsvEntries) {
        if (dsvEntry.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->dsvCpuDescriptorPool->Free(dsvEntry.cpuDescriptorHandle);
        }
    }

    srvEntries.Clear();
    uavEntries.Clear();
    rtvEntries.Clear();
    dsvEntries.Clear();

#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(textureAllocation);
#else
    SAFE_RELEASE(textureResource);
#endif
}

bool D3D12Texture::HasValidView(RHI::ViewType viewType, RHI::ViewID viewID) const {
    if (viewID.IsValid()) {
        if (viewType == RHI::ViewType::SRV) {
            return srvEntries.IsValidIndex(viewID.value) && srvEntries[viewID.value].cpuDescriptorHandle.ptr != 0;
        }
        if (viewType == RHI::ViewType::UAV) {
            return uavEntries.IsValidIndex(viewID.value) && uavEntries[viewID.value].cpuDescriptorHandle.ptr != 0;
        }
        if (viewType == RHI::ViewType::RTV) {
            return rtvEntries.IsValidIndex(viewID.value) && rtvEntries[viewID.value].cpuDescriptorHandle.ptr != 0;
        }
        if (viewType == RHI::ViewType::DSV) {
            return dsvEntries.IsValidIndex(viewID.value) && dsvEntries[viewID.value].cpuDescriptorHandle.ptr != 0;
        }
    }
    return false;
}

uint32_t D3D12Texture::GetBindlessIndex(RHI::ViewType viewType, RHI::ViewID viewID) const {
    if (viewID.IsValid()) {
        if (viewType == RHI::ViewType::SRV) {
            return srvEntries.IsValidIndex(viewID.value) ? srvEntries[viewID.value].bindlessIndex : D3D12BindlessDescriptorAllocator::NullIndex;
        }
        if (viewType == RHI::ViewType::UAV) {
            return uavEntries.IsValidIndex(viewID.value) ? uavEntries[viewID.value].bindlessIndex : D3D12BindlessDescriptorAllocator::NullIndex;
        }
    }
    return D3D12BindlessDescriptorAllocator::NullIndex;
}

RHI::Texture *D3D12Renderer::CreateTexture(RHI::TextureType textureType, RHI::TextureFlag flags, std::shared_ptr<const BE1::Image> srcImage, bool allocateEmptyMipmaps, const RHI::ClearValue &clearValue, uint32_t sampleCount, RHI::GPUResourceState initialState) {
    if (!srcImage) {
        BE_ERRLOG("D3D12Renderer::CreateTexture: srcImage is null\n");
        return nullptr;
    }

    BE1::Image::Format srcFormat = srcImage->GetFormat();
    bool isLinearSpace = srcImage->GetGammaSpace() == BE1::Image::GammaSpace::Linear;

    DXGI_FORMAT dxgiFormat;
    bool srcFormatSupported = D3D12Renderer::ImageFormatToDXGIFormat(srcFormat, !isLinearSpace, &dxgiFormat);
    if (!srcFormatSupported) {
        BE_WARNLOG("D3D12Renderer::CreateTexture: Unsupported image format %s\n", BE1::Image::FormatName(srcFormat));
        return nullptr;
    }

    D3D12_RESOURCE_DIMENSION textureDimension;
    switch (textureType) {
    case RHI::TextureType::Texture1D:
        textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        break;
    case RHI::TextureType::Texture2D:
    case RHI::TextureType::Texture2DArray:
    case RHI::TextureType::TextureCube:
    case RHI::TextureType::TextureCubeArray:
        textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        break;
    case RHI::TextureType::Texture3D:
        textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        break;
    default:
        BE_ERRLOG("D3D12Renderer::CreateTexture: unknown texture type\n");
        return nullptr;
    }

    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

    if (BE1::HasFlag(flags, RHI::TextureFlag::RenderTarget)) {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::DepthStencil)) {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        if (!BE1::HasFlag(flags, RHI::TextureFlag::ShaderResource)) {
            // D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE 는 오직 D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL 와 같이 사용 가능
            resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::UnorderedAccess)) {
        assert(!(resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)); // UAV 는 DSV 와 같은 리소스로 사용할 수 없다.
        assert(sampleCount == 1); // MSAA 텍스쳐는 UAV 로 사용할 수 없다.

        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    int maxDstMipLevels = 0;

    if (allocateEmptyMipmaps) {
        // srcImage 의 mipmap level 0 만 사용하고, 나머지 서브 mipmap levels 는 빈 공간으로 채운다.
        maxDstMipLevels = BE1::Image::MaxMipLevels(srcImage->GetWidth(), srcImage->GetHeight(), srcImage->GetDepth());
    } else {
        // srcImage 의 모든 mipmap levels 를 사용한다.
        maxDstMipLevels = srcImage->NumMipmaps();
    }

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = textureDimension;
    textureDesc.Width = static_cast<UINT>(srcImage->GetWidth());
    textureDesc.Height = static_cast<UINT>(srcImage->GetHeight());
    textureDesc.DepthOrArraySize = static_cast<UINT>(textureDimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? srcImage->GetDepth() : srcImage->GetArraySize());
    textureDesc.MipLevels = static_cast<UINT16>(maxDstMipLevels);
    textureDesc.Format = dxgiFormat;
    textureDesc.SampleDesc.Count = sampleCount;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Flags = resourceFlags;

    D3D12_CLEAR_VALUE optimizedClearValue = {};
    bool useClearValue = BE1::HasAnyFlag(flags, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::DepthStencil);
    if (useClearValue) {
        // Typed 포맷이어야 한다.
        optimizedClearValue.Format = textureDesc.Format;

        if (BE1::HasFlag(flags, RHI::TextureFlag::RenderTarget)) {
            optimizedClearValue.Color[0] = clearValue.color[0];
            optimizedClearValue.Color[1] = clearValue.color[1];
            optimizedClearValue.Color[2] = clearValue.color[2];
            optimizedClearValue.Color[3] = clearValue.color[3];
        }
        if (BE1::HasFlag(flags, RHI::TextureFlag::DepthStencil)) {
            optimizedClearValue.DepthStencil.Depth = clearValue.depthStencil.depth;
            optimizedClearValue.DepthStencil.Stencil = clearValue.depthStencil.stencil;
        }
    }

    if (BE1::HasFlag(flags, RHI::TextureFlag::Typeless)) {
        // optimizedClearValue.Format 을 설정한 후에 Typeless 포맷으로 변환한다.
        textureDesc.Format = ToTypelessFormat(textureDesc.Format);
        if (textureDesc.Format == DXGI_FORMAT_UNKNOWN) {
            BE_ERRLOG("D3D12Renderer::CreateTexture: Cannot convert format %s to typeless format\n", BE1::Image::FormatName(srcFormat));
            return nullptr;
        }
    }

    D3D12_RESOURCE_STATES d3d12InitialState = ToD3D12ResourceState(initialState);

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_TIME;
    allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12MA::Allocation *textureAllocation;
    if (FAILED(allocator->CreateResource(
        &allocationDesc,
        &textureDesc,
        d3d12InitialState,
        useClearValue ? &optimizedClearValue : nullptr,
        &textureAllocation,
        IID_NULL, nullptr))) {
        return false;
    }
#else
    ID3D12Resource *textureResource = nullptr;
    if (FAILED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        d3d12InitialState,
        useClearValue ? &optimizedClearValue : nullptr,
        IID_PPV_ARGS(&textureResource)))) {
        return false;
    }
#endif

    D3D12Texture *texture = new D3D12Texture;
    texture->type = textureType;
    texture->flags = flags;
    texture->srcFormat = srcFormat;
    texture->srcSRGB = !isLinearSpace;
    texture->textureDesc = textureDesc;
    texture->clearValue = optimizedClearValue;
    texture->defaultState = initialState;

#ifdef USE_D3D12_MEMALLOC
    texture->textureAllocation = textureAllocation;
#else
    texture->textureResource = textureResource;
#endif

    if (!BE1::HasFlag(texture->flags, RHI::TextureFlag::SkipDefaultViews) && !BE1::HasFlag(texture->flags, RHI::TextureFlag::Typeless)) {
        // 전체 텍스쳐 리소스에 대한 SRV 생성
        if (BE1::HasFlag(texture->flags, RHI::TextureFlag::ShaderResource)) {
            CreateSRV(texture, RHI::Format::Auto);
            texture->hasDefaultSRV = true;
        }
        // 전체 텍스쳐 리소스에 대한 UAV 생성
        if (BE1::HasFlag(texture->flags, RHI::TextureFlag::UnorderedAccess)) {
            CreateUAV(texture, RHI::Format::Auto);
            texture->hasDefaultUAV = true;
        }
        // 전체 텍스쳐 리소스에 대한 RTV 생성
        if (BE1::HasFlag(texture->flags, RHI::TextureFlag::RenderTarget)) {
            CreateRTV(texture, RHI::Format::Auto);
            texture->hasDefaultRTV = true;
        }
        // 전체 텍스쳐 리소스에 대한 DSV 생성
        if (BE1::HasFlag(texture->flags, RHI::TextureFlag::DepthStencil)) {
            CreateDSV(texture, RHI::Format::Auto);
            texture->hasDefaultDSV = true;
        }
    }

    // 업로드 요청을 등록한다.
    if (!srcImage->IsEmpty()) {
        texture->SetStatus(RHI::GPUResourceStatus::Pending);

        D3D12ResourceRequest request;
        request.type = D3D12ResourceRequest::Type::UploadTexture;
        request.object = texture;
        request.uploadTextureRequest.srcImage = srcImage;
        request.uploadTextureRequest.resourceDesc = textureDesc;
        request.uploadTextureRequest.dstFirstMip = 0;
        // allocateEmptyMipmaps == true 라면, srcImage 의 mipmap level 0 만 사용하고, 나머지 서브 mipmap levels 는 빈 공간으로 채운다.
        request.uploadTextureRequest.mipCount = allocateEmptyMipmaps ? 1 : srcImage->NumMipmaps();
        EnqueueResourceRequest(std::move(request));
    }

    return texture;
}

bool D3D12Renderer::UploadTextureMipRange(RHI::Texture *texture, std::shared_ptr<const BE1::Image> srcImage, int firstMip) {
    assert(texture);
    const D3D12Texture *_texture = static_cast<D3D12Texture *>(texture);

    if (!srcImage) {
        BE_ERRLOG("D3D12Renderer::UploadTextureMipRange: srcImage is null\n");
        return false;
    }

    if (srcImage->IsEmpty()) {
        BE_ERRLOG("D3D12Renderer::UploadTextureMipRange: srcImage is empty\n");
        return false;
    }

    if (firstMip < 0 || firstMip + srcImage->NumMipmaps() > _texture->textureDesc.MipLevels) {
        BE_ERRLOG("D3D12Renderer::UploadTextureMipRange: invalid mip range\n");
        return false;
    }

    UINT dstArraySize = _texture->textureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : _texture->textureDesc.DepthOrArraySize;
    if (srcImage->GetArraySize() != dstArraySize) {
        BE_ERRLOG("D3D12Renderer::UploadTextureMipRange: source array size mismatch\n");
        return false;
    }

    D3D12ResourceRequest request;
    request.type = D3D12ResourceRequest::Type::UploadTexture;
    request.object = texture;
    request.uploadTextureRequest.srcImage = srcImage;
    request.uploadTextureRequest.resourceDesc = _texture->textureDesc;
    request.uploadTextureRequest.dstFirstMip = firstMip;
    request.uploadTextureRequest.mipCount = srcImage->NumMipmaps();
    EnqueueResourceRequest(std::move(request));
    return true;
}

bool D3D12Renderer::ExecuteUploadTexture(D3D12ResourceCommandContext &commandContext, D3D12Texture *texture, const D3D12UploadTextureRequest &request) {
    if (!request.srcImage->IsEmpty()) {
#ifdef USE_D3D12_MEMALLOC
        ID3D12Resource *textureResource = texture->textureAllocation->GetResource();
#else
        ID3D12Resource *textureResource = texture->textureResource;
#endif

        UINT arraySize = request.resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : request.resourceDesc.DepthOrArraySize;
        UINT numSubresources = arraySize * request.resourceDesc.MipLevels;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT *subresourceFootprints = (D3D12_PLACED_SUBRESOURCE_FOOTPRINT *)_alloca(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) * numSubresources);
        UINT64 size;

        // 텍스쳐의 전체 사이즈와 서브 리소스 별 메모리 정보를 얻어온다.
        device->GetCopyableFootprints(&request.resourceDesc, 0, numSubresources, 0, subresourceFootprints, nullptr, nullptr, &size);

        // NOTE: size 는 텍스쳐 전체를 기준으로 계산된 값이다.
        // 만약 allocateEmptyMipmaps 가 true 라면, srcImage 의 mipmap level 0 만 사용하기 때문에 업로드 버퍼가 실제로 필요한 데이터보다 더 큰 사이즈가 될 수 있다.
        // 업로드 버퍼 할당에 약간의 낭비는 있으나 필요한 부분만 사용할 것이므로 문제가 되지는 않는다.
        D3D12_RESOURCE_DESC uploadBufferDesc;
        uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadBufferDesc.Alignment = 0;
        uploadBufferDesc.Width = size;
        uploadBufferDesc.Height = 1;
        uploadBufferDesc.DepthOrArraySize = 1;
        uploadBufferDesc.MipLevels = 1;
        uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadBufferDesc.SampleDesc.Count = 1;
        uploadBufferDesc.SampleDesc.Quality = 0;
        uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES heapProperties;
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 1;
        heapProperties.VisibleNodeMask = 1;

        // 업로드 버퍼 생성
        ID3D12Resource *uploadBuffer = nullptr;
        if (FAILED(device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&uploadBuffer)))) {
            return false;
        }

        // bpp means bytes per pixel or bytes per block for compressed formats
        int bpp = request.srcImage->IsCompressed() ? request.srcImage->BytesPerBlock() : request.srcImage->BytesPerPixel();
        int numSlices = request.srcImage->GetArraySize();

        // 이미지 데이터를 업로드 버퍼에 write
        UINT8 *mappedPtr = nullptr;
        uploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

        for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex) {
            for (int srcMipLevel = 0; srcMipLevel < request.mipCount; ++srcMipLevel) {
                int srcWidth = request.srcImage->GetWidth(srcMipLevel);
                int srcHeight = request.srcImage->GetHeight(srcMipLevel);
                int srcDepth = request.srcImage->GetDepth(srcMipLevel);
                int srcPitch = bpp * (request.srcImage->IsCompressed() ? ((srcWidth + 3) / 4) : srcWidth);
                int srcRows = request.srcImage->IsCompressed() ? ((srcHeight + 3) / 4) : srcHeight;
                const byte *srcPtr = request.srcImage->GetPixels(srcMipLevel, sliceIndex);

                int dstMipLevel = request.dstFirstMip + srcMipLevel;
                UINT subresourceIndex = D3D12CalcSubresource(dstMipLevel, sliceIndex, 0, request.resourceDesc.MipLevels, arraySize);
                const D3D12_PLACED_SUBRESOURCE_FOOTPRINT &footprint = subresourceFootprints[subresourceIndex];
                byte *dstPtr = mappedPtr + footprint.Offset;

                for (int z = 0; z < srcDepth; ++z) {
                    for (int r = 0; r < srcRows; ++r) {
                        BE1::simdProcessor->Memcpy(dstPtr, srcPtr, srcPitch);
                        srcPtr += srcPitch;
                        dstPtr += footprint.Footprint.RowPitch;
                    }
                }
            }
        }

        D3D12_RANGE writtenRange = { 0, size };
        uploadBuffer->Unmap(0, &writtenRange);

        // 업로드 버퍼에서 디바이스 텍스쳐 (VRAM) 로 GPU 에 복사 요청
        D3D12CommandList *commandList = commandContext.GetGraphicsCommandList();
        commandList->ResourceBarrier(textureResource, ToD3D12ResourceState(texture->defaultState), D3D12_RESOURCE_STATE_COPY_DEST);

        for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex) {
            for (int srcMipLevel = 0; srcMipLevel < request.mipCount; ++srcMipLevel) {
                int dstMipLevel = request.dstFirstMip + srcMipLevel;

                UINT subresourceIndex = D3D12CalcSubresource(dstMipLevel, sliceIndex, 0, request.resourceDesc.MipLevels, arraySize);

                D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
                srcLocation.pResource = uploadBuffer;
                srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                srcLocation.PlacedFootprint = subresourceFootprints[subresourceIndex];

                D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
                dstLocation.pResource = textureResource;
                dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dstLocation.SubresourceIndex = subresourceIndex;

                commandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
            }
        }

        commandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, ToD3D12ResourceState(texture->defaultState));

        if (uploadBuffer) {
            RequestRelease(uploadBuffer);
        }
    }

    return true;
}

void D3D12Renderer::DestroyTexture(RHI::Texture *texture, bool immediate) {
    if (immediate) {
        delete texture;
    } else {
        DeleteGPUObject(texture);
    }
}

RHI::ViewID D3D12Renderer::CreateView(RHI::Texture *texture, RHI::ViewType viewType, RHI::Format typelessCompatibleFormat, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel, uint32_t mipCount) {
    D3D12Texture *_texture = static_cast<D3D12Texture *>(texture);

    if (viewType == RHI::ViewType::SRV) {
        return CreateSRV(_texture, typelessCompatibleFormat, firstSlice, sliceCount, firstMipLevel, mipCount);
    }
    if (viewType == RHI::ViewType::UAV) {
        return CreateUAV(_texture, typelessCompatibleFormat, firstSlice, sliceCount, firstMipLevel);
    }
    if (viewType == RHI::ViewType::RTV) {
        return CreateRTV(_texture, typelessCompatibleFormat, firstSlice, sliceCount, firstMipLevel);
    }
    if (viewType == RHI::ViewType::DSV) {
        return CreateDSV(_texture, typelessCompatibleFormat, firstSlice, sliceCount, firstMipLevel);
    }
    return RHI::ViewID::InvalidID;
}

void D3D12Renderer::DestroyView(RHI::Texture *texture, RHI::ViewType viewType, RHI::ViewID viewID) {
    D3D12Texture *_texture = static_cast<D3D12Texture *>(texture);

    if (viewType == RHI::ViewType::SRV) {
        if (!_texture->srvEntries.IsValidIndex(viewID.value)) {
            BE_ERRLOG("D3D12Renderer::DestroyView: Invalid SRV view ID (%i)\n", viewID.value);
            return;
        }
        D3D12SRVEntry *srvEntryPtr = &_texture->srvEntries[viewID.value];
        if (srvEntryPtr->cpuDescriptorHandle.ptr != 0) {
            resCpuDescriptorPool->Free(srvEntryPtr->cpuDescriptorHandle);
        }
        if (srvEntryPtr->bindlessIndex != D3D12BindlessDescriptorAllocator::NullIndex) {
            FreeBindlessResourceDescriptor(srvEntryPtr->bindlessIndex);
        }
        *srvEntryPtr = {};
        return;
    }
    if (viewType == RHI::ViewType::UAV) {
        if (!_texture->uavEntries.IsValidIndex(viewID.value)) {
            BE_ERRLOG("D3D12Renderer::DestroyView: Invalid UAV view ID (%i)\n", viewID.value);
            return;
        }
        D3D12UAVEntry *uavEntryPtr = &_texture->uavEntries[viewID.value];
        if (uavEntryPtr->cpuDescriptorHandle.ptr != 0) {
            resCpuDescriptorPool->Free(uavEntryPtr->cpuDescriptorHandle);
        }
        if (uavEntryPtr->bindlessIndex != D3D12BindlessDescriptorAllocator::NullIndex) {
            FreeBindlessResourceDescriptor(uavEntryPtr->bindlessIndex);
        }
        *uavEntryPtr = {};
        return;
    }
    if (viewType == RHI::ViewType::RTV) {
        if (!_texture->rtvEntries.IsValidIndex(viewID.value)) {
            BE_ERRLOG("D3D12Renderer::DestroyView: Invalid RTV view ID (%i)\n", viewID.value);
            return;
        }
        D3D12RTVEntry *rtvEntryPtr = &_texture->rtvEntries[viewID.value];
        if (rtvEntryPtr->cpuDescriptorHandle.ptr != 0) {
            rtvCpuDescriptorPool->Free(rtvEntryPtr->cpuDescriptorHandle);
        }
        *rtvEntryPtr = {};
        return;
    }
    if (viewType == RHI::ViewType::DSV) {
        if (!_texture->dsvEntries.IsValidIndex(viewID.value)) {
            BE_ERRLOG("D3D12Renderer::DestroyView: Invalid DSV view ID (%i)\n", viewID.value);
            return;
        }
        D3D12DSVEntry *dsvEntryPtr = &_texture->dsvEntries[viewID.value];
        if (dsvEntryPtr->cpuDescriptorHandle.ptr != 0) {
            dsvCpuDescriptorPool->Free(dsvEntryPtr->cpuDescriptorHandle);
        }
        *dsvEntryPtr = {};
        return;
    }
}

RHI::ViewID D3D12Renderer::CreateSRV(D3D12Texture *texture, RHI::Format typelessCompatibleFormat, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel, uint32_t mipCount) {
    D3D12SRVEntry newSrvEntry;

    if (IsTypelessFormat(texture->textureDesc.Format)) {
        DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN;

        if (typelessCompatibleFormat == RHI::Format::Auto) {
            switch (texture->textureDesc.Format) {
            case DXGI_FORMAT_R32G32B32A32_TYPELESS:
                viewFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            case DXGI_FORMAT_R32G32B32_TYPELESS:
                viewFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                break;
            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
                viewFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
                break;
            case DXGI_FORMAT_R32G32_TYPELESS:
                viewFormat = DXGI_FORMAT_R32G32_FLOAT;
                break;
            case DXGI_FORMAT_R32G8X24_TYPELESS:
                viewFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                break;
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
                viewFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
                break;
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
                viewFormat = texture->srcSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
                break;
            case DXGI_FORMAT_R16G16_TYPELESS:
                viewFormat = DXGI_FORMAT_R16G16_FLOAT;
                break;
            case DXGI_FORMAT_R32_TYPELESS:
                viewFormat = DXGI_FORMAT_R32_FLOAT;
                break;
            case DXGI_FORMAT_R24G8_TYPELESS:
                viewFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                break;
            case DXGI_FORMAT_R8G8_TYPELESS:
                viewFormat = DXGI_FORMAT_R8G8_UNORM;
                break;
            case DXGI_FORMAT_R16_TYPELESS:
                viewFormat = DXGI_FORMAT_R16_UNORM;
                break;
            case DXGI_FORMAT_R8_TYPELESS:
                viewFormat = DXGI_FORMAT_R8_UNORM;
                break;
            case DXGI_FORMAT_BC1_TYPELESS:
                viewFormat = texture->srcSRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
                break;
            case DXGI_FORMAT_BC2_TYPELESS:
                viewFormat = texture->srcSRGB ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
                break;
            case DXGI_FORMAT_BC3_TYPELESS:
                viewFormat = texture->srcSRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
                break;
            case DXGI_FORMAT_BC4_TYPELESS:
                viewFormat = DXGI_FORMAT_BC4_UNORM;
                break;
            case DXGI_FORMAT_BC5_TYPELESS:
                viewFormat = DXGI_FORMAT_BC5_UNORM;
                break;
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
                viewFormat = texture->srcSRGB ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
                break;
            case DXGI_FORMAT_B8G8R8X8_TYPELESS:
                viewFormat = texture->srcSRGB ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB : DXGI_FORMAT_B8G8R8X8_UNORM;
                break;
            case DXGI_FORMAT_BC6H_TYPELESS:
                viewFormat = DXGI_FORMAT_BC6H_UF16;
                break;
            case DXGI_FORMAT_BC7_TYPELESS:
                viewFormat = texture->srcSRGB ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
                break;
            }
        } else {
            viewFormat = RHIFormatToDXGIFormat(typelessCompatibleFormat);
        }

        // typelessCompatibleFormat 이 texture 의 typeless format 과 호환 여부만 체크하는 최소한의 안전장치
        if (!IsTypelessCompatibleFormat(texture->textureDesc.Format, viewFormat)) {
            BE_ERRLOG("D3D12Renderer::CreateSRV: Incompatible typeless format (%i -> %i)\n", texture->textureDesc.Format, viewFormat);
            return RHI::ViewID::InvalidID;
        }
        newSrvEntry.viewDesc.Format = viewFormat;
        newSrvEntry.viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    } else {
        newSrvEntry.viewDesc.Format = texture->textureDesc.Format;
        newSrvEntry.viewDesc.Shader4ComponentMapping = GetComponentSwizzling(texture->srcFormat);
    }

    switch (texture->type) {
    case RHI::TextureType::Texture1D:
        newSrvEntry.viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        newSrvEntry.viewDesc.Texture1D.MostDetailedMip = firstMipLevel;
        newSrvEntry.viewDesc.Texture1D.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::Texture1DArray:
        newSrvEntry.viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        newSrvEntry.viewDesc.Texture1DArray.FirstArraySlice = firstSlice;
        newSrvEntry.viewDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        newSrvEntry.viewDesc.Texture1DArray.MostDetailedMip = firstMipLevel;
        newSrvEntry.viewDesc.Texture1DArray.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::Texture2D:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            newSrvEntry.viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        } else {
            newSrvEntry.viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            newSrvEntry.viewDesc.Texture2D.MostDetailedMip = firstMipLevel;
            newSrvEntry.viewDesc.Texture2D.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        }
        break;
    case RHI::TextureType::Texture2DArray:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            newSrvEntry.viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            newSrvEntry.viewDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
            newSrvEntry.viewDesc.Texture2DMSArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        } else {
            newSrvEntry.viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            newSrvEntry.viewDesc.Texture2DArray.FirstArraySlice = firstSlice;
            newSrvEntry.viewDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
            newSrvEntry.viewDesc.Texture2DArray.MostDetailedMip = firstMipLevel;
            newSrvEntry.viewDesc.Texture2DArray.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        }
        break;
    case RHI::TextureType::TextureCube:
        newSrvEntry.viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        newSrvEntry.viewDesc.TextureCube.MostDetailedMip = firstMipLevel;
        newSrvEntry.viewDesc.TextureCube.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::TextureCubeArray:
        assert(firstSlice % 6 == 0); // 큐브 맵 어레이는 6의 배수로 슬라이스가 할당되어야 한다.
        sliceCount = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        assert(sliceCount % 6 == 0); // 큐브 맵 어레이는 6의 배수로 슬라이스가 할당되어야 한다.
        newSrvEntry.viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        newSrvEntry.viewDesc.TextureCubeArray.First2DArrayFace = firstSlice;
        newSrvEntry.viewDesc.TextureCubeArray.NumCubes = sliceCount / 6;
        newSrvEntry.viewDesc.TextureCubeArray.MostDetailedMip = firstMipLevel;
        newSrvEntry.viewDesc.TextureCubeArray.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::Texture3D:
        newSrvEntry.viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        newSrvEntry.viewDesc.Texture3D.MostDetailedMip = firstMipLevel;
        newSrvEntry.viewDesc.Texture3D.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    }

    // 디스크립터 슬롯 할당
    if (!resCpuDescriptorPool->Alloc(&newSrvEntry.cpuDescriptorHandle, nullptr)) {
        return RHI::ViewID::InvalidID;
    }

    // 디스크립터 슬롯에 SRV 를 기록
    device->CreateShaderResourceView(texture->GetResource(), &newSrvEntry.viewDesc, newSrvEntry.cpuDescriptorHandle);

    // bindless index 할당
    newSrvEntry.bindlessIndex = AllocBindlessResourceDescriptor();
    if (newSrvEntry.bindlessIndex == D3D12BindlessDescriptorAllocator::NullIndex) {
        resCpuDescriptorPool->Free(newSrvEntry.cpuDescriptorHandle);
        return RHI::ViewID::InvalidID;
    }

    // shader-visible heap 의 bindless slot 으로 복사
    D3D12_CPU_DESCRIPTOR_HANDLE bindlessCpuDescriptorHandle = GetBindlessResourceCPUDescriptorHandle(newSrvEntry.bindlessIndex);
    device->CopyDescriptorsSimple(1, bindlessCpuDescriptorHandle, newSrvEntry.cpuDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // SRV 추가 후 viewID 를 리턴
    RHI::ViewID viewID;

    for (int i = 0; i < texture->srvEntries.Count(); ++i) {
        if (!texture->srvEntries[i].cpuDescriptorHandle.ptr) {
            texture->srvEntries[i] = newSrvEntry;
            viewID.value = i;
            return viewID;
        }
    }
    viewID.value = texture->srvEntries.Append(newSrvEntry);
    return viewID;
}

RHI::ViewID D3D12Renderer::CreateUAV(D3D12Texture *texture, RHI::Format typelessCompatibleFormat, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel) {
    D3D12UAVEntry newUavEntry;

    if (IsTypelessFormat(texture->textureDesc.Format)) {
        DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN;

        if (typelessCompatibleFormat == RHI::Format::Auto) {
            switch (texture->textureDesc.Format) {
            case DXGI_FORMAT_R32G32B32A32_TYPELESS:
                viewFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
                viewFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
                break;
            case DXGI_FORMAT_R32G32_TYPELESS:
                viewFormat = DXGI_FORMAT_R32G32_FLOAT;
                break;
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
                viewFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
                break;
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
                viewFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;
            case DXGI_FORMAT_R16G16_TYPELESS:
                viewFormat = DXGI_FORMAT_R16G16_FLOAT;
                break;
            case DXGI_FORMAT_R32_TYPELESS:
                viewFormat = DXGI_FORMAT_R32_FLOAT;
                break;
            case DXGI_FORMAT_R8G8_TYPELESS:
                viewFormat = DXGI_FORMAT_R8G8_UNORM;
                break;
            case DXGI_FORMAT_R16_TYPELESS:
                viewFormat = DXGI_FORMAT_R16_UNORM;
                break;
            case DXGI_FORMAT_R8_TYPELESS:
                viewFormat = DXGI_FORMAT_R8_UNORM;
                break;
            }
        } else {
            viewFormat = RHIFormatToDXGIFormat(typelessCompatibleFormat);
        }

        // typelessCompatibleFormat 이 texture 의 typeless format 과 호환 여부만 체크하는 최소한의 안전장치
        if (!IsTypelessCompatibleFormat(texture->textureDesc.Format, viewFormat)) {
            BE_ERRLOG("D3D12Renderer::CreateUAV: Incompatible typeless format (%i -> %i)\n", texture->textureDesc.Format, viewFormat);
            return RHI::ViewID::InvalidID;
        }
        newUavEntry.viewDesc.Format = viewFormat;
    } else {
        newUavEntry.viewDesc.Format = texture->textureDesc.Format;
    }

    // UAV 호환 포맷인지 검사한다.
    if (!IsSupportedUAVFormat(newUavEntry.viewDesc.Format)) {
        BE_ERRLOG("D3D12Renderer::CreateUAV: Unsupported UAV format (%i)\n", newUavEntry.viewDesc.Format);
        return RHI::ViewID::InvalidID;
    }

    switch (texture->type) {
    case RHI::TextureType::Texture1D:
        newUavEntry.viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
        newUavEntry.viewDesc.Texture1D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture1DArray:
        newUavEntry.viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        newUavEntry.viewDesc.Texture1DArray.MipSlice = firstMipLevel;
        newUavEntry.viewDesc.Texture1DArray.FirstArraySlice = firstSlice;
        newUavEntry.viewDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture2D:
        newUavEntry.viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        newUavEntry.viewDesc.Texture2D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture2DArray:
    case RHI::TextureType::TextureCube:
    case RHI::TextureType::TextureCubeArray:
        newUavEntry.viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        newUavEntry.viewDesc.Texture2DArray.MipSlice = firstMipLevel;
        newUavEntry.viewDesc.Texture2DArray.FirstArraySlice = firstSlice;
        newUavEntry.viewDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture3D:
        newUavEntry.viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        newUavEntry.viewDesc.Texture3D.MipSlice = firstMipLevel;
        newUavEntry.viewDesc.Texture3D.FirstWSlice = 0;
        newUavEntry.viewDesc.Texture3D.WSize = -1;
        break;
    }

    // 디스크립터 슬롯 할당
    if (!resCpuDescriptorPool->Alloc(&newUavEntry.cpuDescriptorHandle, nullptr)) {
        return RHI::ViewID::InvalidID;
    }

    // 디스크립터 슬롯에 UAV 를 기록
    device->CreateUnorderedAccessView(texture->GetResource(), nullptr, &newUavEntry.viewDesc, newUavEntry.cpuDescriptorHandle);

    // bindless index 할당
    newUavEntry.bindlessIndex = AllocBindlessResourceDescriptor();
    if (newUavEntry.bindlessIndex == D3D12BindlessDescriptorAllocator::NullIndex) {
        resCpuDescriptorPool->Free(newUavEntry.cpuDescriptorHandle);
        return RHI::ViewID::InvalidID;
    }

    // shader-visible heap 의 bindless slot 으로 복사
    D3D12_CPU_DESCRIPTOR_HANDLE bindlessCpuDescriptorHandle = GetBindlessResourceCPUDescriptorHandle(newUavEntry.bindlessIndex);
    device->CopyDescriptorsSimple(1, bindlessCpuDescriptorHandle, newUavEntry.cpuDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // UAV 추가 후 viewID 를 리턴
    RHI::ViewID viewID;

    for (int i = 0; i < texture->uavEntries.Count(); ++i) {
        if (!texture->uavEntries[i].cpuDescriptorHandle.ptr) {
            texture->uavEntries[i] = newUavEntry;
            viewID.value = i;
            return viewID;
        }
    }
    viewID.value = texture->uavEntries.Append(newUavEntry);
    return viewID;
}

RHI::ViewID D3D12Renderer::CreateRTV(D3D12Texture *texture, RHI::Format typelessCompatibleFormat, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel) {
    D3D12RTVEntry newRtvEntry;

    if (IsTypelessFormat(texture->textureDesc.Format)) {
        DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN;

        if (typelessCompatibleFormat == RHI::Format::Auto) {
            switch (texture->textureDesc.Format) {
            case DXGI_FORMAT_R32G32B32A32_TYPELESS:
                viewFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
                viewFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
                break;
            case DXGI_FORMAT_R32G32_TYPELESS:
                viewFormat = DXGI_FORMAT_R32G32_FLOAT;
                break;
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
                viewFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
                break;
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
                viewFormat = texture->srcSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
                break;
            case DXGI_FORMAT_R16G16_TYPELESS:
                viewFormat = DXGI_FORMAT_R16G16_FLOAT;
                break;
            case DXGI_FORMAT_R32_TYPELESS:
                viewFormat = DXGI_FORMAT_R32_FLOAT;
                break;
            case DXGI_FORMAT_R8G8_TYPELESS:
                viewFormat = DXGI_FORMAT_R8G8_UNORM;
                break;
            case DXGI_FORMAT_R16_TYPELESS:
                viewFormat = DXGI_FORMAT_R16_UNORM;
                break;
            case DXGI_FORMAT_R8_TYPELESS:
                viewFormat = DXGI_FORMAT_R8_UNORM;
                break;
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
                viewFormat = texture->srcSRGB ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
                break;
            case DXGI_FORMAT_B8G8R8X8_TYPELESS:
                viewFormat = texture->srcSRGB ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB : DXGI_FORMAT_B8G8R8X8_UNORM;
                break;
            }
        } else {
            viewFormat = RHIFormatToDXGIFormat(typelessCompatibleFormat);
        }

        // typelessCompatibleFormat 이 texture 의 typeless format 과 호환 여부만 체크하는 최소한의 안전장치
        if (!IsTypelessCompatibleFormat(texture->textureDesc.Format, viewFormat)) {
            BE_ERRLOG("D3D12Renderer::CreateRTV: Incompatible typeless format (%i -> %i)\n", texture->textureDesc.Format, viewFormat);
            return RHI::ViewID::InvalidID;
        }
        newRtvEntry.viewDesc.Format = viewFormat;
    } else {
        newRtvEntry.viewDesc.Format = texture->textureDesc.Format;
    }

    switch (texture->type) {
    case RHI::TextureType::Texture1D:
        newRtvEntry.viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
        newRtvEntry.viewDesc.Texture1D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture1DArray:
        newRtvEntry.viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        newRtvEntry.viewDesc.Texture1DArray.MipSlice = firstMipLevel;
        newRtvEntry.viewDesc.Texture1DArray.FirstArraySlice = firstSlice;
        newRtvEntry.viewDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture2D:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            newRtvEntry.viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        } else {
            newRtvEntry.viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            newRtvEntry.viewDesc.Texture2D.MipSlice = firstMipLevel;
        }
        break;
    case RHI::TextureType::Texture2DArray:
    case RHI::TextureType::TextureCube:
    case RHI::TextureType::TextureCubeArray:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            newRtvEntry.viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            newRtvEntry.viewDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
            newRtvEntry.viewDesc.Texture2DMSArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        } else {
            newRtvEntry.viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            newRtvEntry.viewDesc.Texture2DArray.MipSlice = firstMipLevel;
            newRtvEntry.viewDesc.Texture2DArray.FirstArraySlice = firstSlice;
            newRtvEntry.viewDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        }
        break;
    case RHI::TextureType::Texture3D:
        // TODO: 정말 가능한지 실험해 볼 것
        newRtvEntry.viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        newRtvEntry.viewDesc.Texture3D.MipSlice = firstMipLevel;
        newRtvEntry.viewDesc.Texture3D.FirstWSlice = 0;
        newRtvEntry.viewDesc.Texture3D.WSize = -1;
        break;
    }

    // 디스크립터 슬롯 할당
    if (!rtvCpuDescriptorPool->Alloc(&newRtvEntry.cpuDescriptorHandle, nullptr)) {
        return RHI::ViewID::InvalidID;
    }

    // 디스크립터 슬롯에 RTV 를 기록
    device->CreateRenderTargetView(texture->GetResource(), &newRtvEntry.viewDesc, newRtvEntry.cpuDescriptorHandle);

    // RTV 추가 후 viewID 를 리턴
    RHI::ViewID viewID;

    for (int i = 0; i < texture->rtvEntries.Count(); ++i) {
        if (!texture->rtvEntries[i].cpuDescriptorHandle.ptr) {
            texture->rtvEntries[i] = newRtvEntry;
            viewID.value = i;
            return viewID;
        }
    }
    viewID.value = texture->rtvEntries.Append(newRtvEntry);
    return viewID;
}

RHI::ViewID D3D12Renderer::CreateDSV(D3D12Texture *texture, RHI::Format typelessCompatibleFormat, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel) {
    D3D12DSVEntry newDsvEntry;

    if (IsTypelessFormat(texture->textureDesc.Format)) {
        DXGI_FORMAT viewFormat = DXGI_FORMAT_UNKNOWN;

        if (typelessCompatibleFormat == RHI::Format::Auto) {
            switch (texture->textureDesc.Format) {
            case DXGI_FORMAT_R32G8X24_TYPELESS:
                viewFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
                break;
            case DXGI_FORMAT_R32_TYPELESS:
                viewFormat = DXGI_FORMAT_D32_FLOAT;
                break;
            case DXGI_FORMAT_R24G8_TYPELESS:
                viewFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
                break;
            case DXGI_FORMAT_R16_TYPELESS:
                viewFormat = DXGI_FORMAT_D16_UNORM;
                break;
            }
        } else {
            viewFormat = RHIFormatToDXGIFormat(typelessCompatibleFormat);
        }

        // typelessCompatibleFormat 이 texture 의 typeless format 과 호환 여부만 체크하는 최소한의 안전장치
        if (!IsTypelessCompatibleFormat(texture->textureDesc.Format, viewFormat)) {
            BE_ERRLOG("D3D12Renderer::CreateDSV: Incompatible typeless format (%i -> %i)\n", texture->textureDesc.Format, viewFormat);
            return RHI::ViewID::InvalidID;
        }
        newDsvEntry.viewDesc.Format = viewFormat;
    } else {
        newDsvEntry.viewDesc.Format = texture->textureDesc.Format;
    }

    switch (texture->type) {
    case RHI::TextureType::Texture1D:
        newDsvEntry.viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
        newDsvEntry.viewDesc.Texture1D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture1DArray:
        newDsvEntry.viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        newDsvEntry.viewDesc.Texture1DArray.MipSlice = firstMipLevel;
        newDsvEntry.viewDesc.Texture1DArray.FirstArraySlice = firstSlice;
        newDsvEntry.viewDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture2D:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            newDsvEntry.viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        } else {
            newDsvEntry.viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            newDsvEntry.viewDesc.Texture2D.MipSlice = firstMipLevel;
        }
        break;
    case RHI::TextureType::Texture2DArray:
    case RHI::TextureType::TextureCube:
    case RHI::TextureType::TextureCubeArray:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            newDsvEntry.viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            newDsvEntry.viewDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
            newDsvEntry.viewDesc.Texture2DMSArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        } else {
            newDsvEntry.viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            newDsvEntry.viewDesc.Texture2DArray.MipSlice = firstMipLevel;
            newDsvEntry.viewDesc.Texture2DArray.FirstArraySlice = firstSlice;
            newDsvEntry.viewDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        }
        break;
    }

    // 디스크립터 슬롯 할당
    if (!dsvCpuDescriptorPool->Alloc(&newDsvEntry.cpuDescriptorHandle, nullptr)) {
        return RHI::ViewID::InvalidID;
    }

    // 디스크립터 슬롯에 DSV 를 기록
    device->CreateDepthStencilView(texture->GetResource(), &newDsvEntry.viewDesc, newDsvEntry.cpuDescriptorHandle);

    // DSV 추가 후 viewID 를 리턴
    RHI::ViewID viewID;

    for (int i = 0; i < texture->dsvEntries.Count(); ++i) {
        if (!texture->dsvEntries[i].cpuDescriptorHandle.ptr) {
            texture->dsvEntries[i] = newDsvEntry;
            viewID.value = i;
            return viewID;
        }
    }
    viewID.value = texture->dsvEntries.Append(newDsvEntry);
    return viewID;
}

bool D3D12Renderer::UpdateTextureViewMipRange(RHI::Texture *texture, RHI::ViewID viewID, uint32_t mostDetailedMip, uint32_t mipLevels) {
    D3D12Texture *_texture = static_cast<D3D12Texture *>(texture);
    assert(_texture);

    if (!_texture->srvEntries.IsValidIndex(viewID.value)) {
        return false;
    }

    if (mostDetailedMip >= _texture->textureDesc.MipLevels || mipLevels == 0) {
        return false;
    }

    D3D12SRVEntry &entry = _texture->srvEntries[viewID.value];
    if (entry.cpuDescriptorHandle.ptr == 0) {
        return false;
    }

    D3D12ResourceRequest request;
    request.type = D3D12ResourceRequest::Type::UpdateTextureViewMipRange;
    request.object = texture;
    request.updateTextureViewMipRangeRequest.viewID = viewID;
    request.updateTextureViewMipRangeRequest.firstMip = mostDetailedMip;
    request.updateTextureViewMipRangeRequest.mipCount = mipLevels;
    EnqueueResourceRequest(std::move(request));
    return true;
}

bool D3D12Renderer::ExecuteUpdateTextureViewMipRange(D3D12ResourceCommandContext &commandContext, D3D12Texture *texture, const D3D12UpdateTextureViewMipRangeRequest &request) {
    D3D12SRVEntry &entry = texture->srvEntries[request.viewID.value];
    if (entry.cpuDescriptorHandle.ptr == 0) {
        return false;
    }

    uint32_t clampedMipLevels = BE1::Min(request.mipCount, texture->textureDesc.MipLevels - request.firstMip);

    switch (entry.viewDesc.ViewDimension) {
    case D3D12_SRV_DIMENSION_TEXTURE1D:
        entry.viewDesc.Texture1D.MostDetailedMip = request.firstMip;
        entry.viewDesc.Texture1D.MipLevels = clampedMipLevels;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
        entry.viewDesc.Texture1DArray.MostDetailedMip = request.firstMip;
        entry.viewDesc.Texture1DArray.MipLevels = clampedMipLevels;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2D:
        entry.viewDesc.Texture2D.MostDetailedMip = request.firstMip;
        entry.viewDesc.Texture2D.MipLevels = clampedMipLevels;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
        entry.viewDesc.Texture2DArray.MostDetailedMip = request.firstMip;
        entry.viewDesc.Texture2DArray.MipLevels = clampedMipLevels;
        break;
    case D3D12_SRV_DIMENSION_TEXTURECUBE:
        entry.viewDesc.TextureCube.MostDetailedMip = request.firstMip;
        entry.viewDesc.TextureCube.MipLevels = clampedMipLevels;
        break;
    case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
        entry.viewDesc.TextureCubeArray.MostDetailedMip = request.firstMip;
        entry.viewDesc.TextureCubeArray.MipLevels = clampedMipLevels;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE3D:
        entry.viewDesc.Texture3D.MostDetailedMip = request.firstMip;
        entry.viewDesc.Texture3D.MipLevels = clampedMipLevels;
        break;
    default:
        return false;
    }

    device->CreateShaderResourceView(texture->GetResource(), &entry.viewDesc, entry.cpuDescriptorHandle);

    if (entry.bindlessIndex != D3D12BindlessDescriptorAllocator::NullIndex) {
        D3D12_CPU_DESCRIPTOR_HANDLE bindlessCpuDescriptorHandle = GetBindlessResourceCPUDescriptorHandle(entry.bindlessIndex);
        device->CopyDescriptorsSimple(1, bindlessCpuDescriptorHandle, entry.cpuDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    return true;
}

bool D3D12Renderer::GetTextureImage(RHI::Texture *texture, int mipLevel, int sliceIndex, BE1::Image::Format dstFormat, void *outPixels) {
    D3D12Texture *_texture = static_cast<D3D12Texture *>(texture);
    assert(_texture);

    if (_texture->GetStatus() != RHI::GPUResourceStatus::Ready) {
        BE_WARNLOG("D3D12Renderer::GetTextureImage: texture is not ready\n");
        return false;
    }

    // 서브 리소스 인덱스를 계산한다.
    int subresourceIndex = D3D12CalcSubresource(mipLevel, sliceIndex, 0, _texture->textureDesc.MipLevels, _texture->textureDesc.DepthOrArraySize);

    // 서브 리소스의 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprint;
    UINT64 subresourceSize;
    device->GetCopyableFootprints(&_texture->textureDesc, subresourceIndex, 1, 0, &subresourceFootprint, nullptr, nullptr, &subresourceSize);

    // 리드백 버퍼를 생성한다.
    D3D12Buffer *readbackBuffer = static_cast<D3D12Buffer *>(CreateBuffer(RHI::BufferUsage::Readback, RHI::BufferFlag::None, subresourceSize, nullptr));
    if (!readbackBuffer) {
        BE_WARNLOG("D3D12Texture::GetTextureImage: Failed to create readback buffer\n");
        return false;
    }

    ID3D12Resource *textureResource = _texture->GetResource();
    D3D12_RESOURCE_STATES currentState = ToD3D12ResourceState(_texture->defaultState);

    // 텍스쳐에서 리드백 버퍼로 복사한다.
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = textureResource;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = subresourceIndex;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = readbackBuffer->GetResource();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLocation.PlacedFootprint = subresourceFootprint;

    D3D12CommandList *commandList = static_cast<D3D12CommandList *>(BeginUtilityCommandList(RHI::CommandQueueType::Graphics));
    if (!commandList) {
        BE_ERRLOG("D3D12Texture::GetTextureImage: Failed to get command list\n");
        DestroyBuffer(readbackBuffer, true);
        return false;
    }

    commandList->ResourceBarrier(textureResource, currentState, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    commandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_SOURCE, currentState);
    commandList->CloseAndExecute();

    // GPU 측에서 복사를 완료할 때까지 기다린다.
    Finish(RHI::CommandQueueType::Graphics);

    EndUtilityCommandList(commandList);

    // 리드백 버퍼의 내용을 메모리로 읽어온다.
    const byte *srcPtr = (byte *)readbackBuffer->mappedPtr;
    byte *dstPtr = nullptr;

    BE1::Image tempImage;
    if (_texture->srcFormat != dstFormat) {
        // 컨버팅이 필요하다면, 리드백 버퍼의 내용을 tempImage 에 카피할 준비를 한다.
        tempImage.Create2D(subresourceFootprint.Footprint.Width, subresourceFootprint.Footprint.Height, subresourceFootprint.Footprint.Depth,
            _texture->srcFormat, _texture->srcSRGB ? BE1::Image::GammaSpace::sRGB : BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
        dstPtr = tempImage.GetPixels();
    } else {
        // 컨버팅할 필요가 없다면, 리드백 버퍼의 내용을 그대로 outPixels 로 카피할 준비를 한다.
        dstPtr = (byte *)outPixels;
    }

    int srcPitch = subresourceFootprint.Footprint.RowPitch;
    int dstPitch = BE1::Image::MemRequired(subresourceFootprint.Footprint.Width, 1, 1, 1, 1, _texture->srcFormat);

    for (UINT z = 0; z < subresourceFootprint.Footprint.Depth; ++z) {
        for (UINT y = 0; y < subresourceFootprint.Footprint.Height; ++y) {
            BE1::simdProcessor->Memcpy(dstPtr, srcPtr, dstPitch);
            srcPtr += srcPitch;
            dstPtr += dstPitch;
        }
    }

    // 리드백 버퍼 삭제
    DestroyBuffer(readbackBuffer, true);

    // 필요하다면 컨버팅한다.
    if (_texture->srcFormat != dstFormat) {
        BE1::Image dstImage;
        if (tempImage.ConvertFormat(dstFormat, dstImage)) {
            BE1::simdProcessor->Memcpy(outPixels, dstImage.GetPixels(), dstImage.SizeInBytes());
        }
    }
    return true;
}

bool D3D12Renderer::GetTextureImage2D(RHI::Texture *texture, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) {
    return GetTextureImage(texture, mipLevel, 0, imageFormat, outPixels);
}

bool D3D12Renderer::GetTextureImage3D(RHI::Texture *texture, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) {
    return GetTextureImage(texture, mipLevel, 0, imageFormat, outPixels);
}

bool D3D12Renderer::GetTextureImageCubeFace(RHI::Texture *texture, RHI::CubemapFace face, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) {
    return GetTextureImage(texture, mipLevel, to_int(face), imageFormat, outPixels);
}

bool D3D12Renderer::SetTextureSubImage(RHI::Texture *texture, int mipLevel, int sliceIndex, int x, int y, int z, int width, int height, int depth, BE1::Image::Format imageFormat, const void *pixels) {
    if (width <= 0 || height <= 0 || depth <= 0) {
        BE_WARNLOG("D3D12Renderer::SetTextureSubImage: invalid sub-image size (%d x %d x %d)\n", width, height, depth);
        return false;
    }
    if (!pixels) {
        BE_WARNLOG("D3D12Renderer::SetTextureSubImage: pixels is null\n");
        return false;
    }

    D3D12Texture *_texture = static_cast<D3D12Texture *>(texture);
    assert(_texture);

    // 서브 리소스 인덱스를 계산한다.
    int subresourceIndex = D3D12CalcSubresource(mipLevel, sliceIndex, 0, _texture->textureDesc.MipLevels, _texture->textureDesc.DepthOrArraySize);

    // 서브 리소스의 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprint;
    device->GetCopyableFootprints(&_texture->textureDesc, subresourceIndex, 1, 0, &subresourceFootprint, nullptr, nullptr, nullptr);

    int srcPitch = BE1::Image::MemRequired(width, 1, 1, 1, 1, imageFormat);
    int dstPitch = subresourceFootprint.Footprint.RowPitch;
    uint64_t uploadBufferSize = BE1::Image::MemRequired(dstPitch, height, depth, 1, 1, imageFormat);

    D3D12_RESOURCE_DESC uploadBufferDesc;
    uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadBufferDesc.Alignment = 0;
    uploadBufferDesc.Width = uploadBufferSize;
    uploadBufferDesc.Height = 1;
    uploadBufferDesc.DepthOrArraySize = 1;
    uploadBufferDesc.MipLevels = 1;
    uploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    uploadBufferDesc.SampleDesc.Count = 1;
    uploadBufferDesc.SampleDesc.Quality = 0;
    uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProperties;
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    // 업로드 버퍼 생성 (pitch 를 타겟 텍스쳐와 동일하게 잡는다)
    ID3D12Resource *uploadBuffer = nullptr;
    if (FAILED(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&uploadBuffer)))) {
        return false;
    }

    // 이미지 데이터를 업로드 버퍼에 write
    UINT8 *mappedPtr = nullptr;
    uploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

    byte *dstPtr = mappedPtr;
    const byte *srcPtr = (byte *)pixels;

    for (UINT d = 0; d < depth; ++d) {
        for (UINT h = 0; h < height; ++h) {
            BE1::simdProcessor->Memcpy(dstPtr, srcPtr, srcPitch);
            srcPtr += srcPitch;
            dstPtr += dstPitch;
        }
    }

    D3D12_RANGE writtenRange = { 0, uploadBufferSize };
    uploadBuffer->Unmap(0, &writtenRange);

    D3D12ResourceRequest request;
    request.type = D3D12ResourceRequest::Type::UploadTextureSubImage;
    request.object = texture;
    request.uploadTextureSubImageRequest.uploadBuffer = uploadBuffer;
    request.uploadTextureSubImageRequest.subresourceIndex = subresourceIndex;
    request.uploadTextureSubImageRequest.x = x;
    request.uploadTextureSubImageRequest.y = y;
    request.uploadTextureSubImageRequest.z = z;
    request.uploadTextureSubImageRequest.width = width;
    request.uploadTextureSubImageRequest.height = height;
    request.uploadTextureSubImageRequest.depth = depth;
    EnqueueResourceRequest(std::move(request));

    return true;
}

bool D3D12Renderer::ExecuteUploadTextureSubImage(D3D12ResourceCommandContext &commandContext, D3D12Texture *texture, const D3D12UploadTextureSubImageRequest &request) {
    ID3D12Resource *textureResource = texture->GetResource();
    D3D12_RESOURCE_STATES currentState = ToD3D12ResourceState(texture->defaultState);

    // 업로드 버퍼에서 디바이스 텍스쳐 (VRAM) 로 GPU 에 복사 요청
    D3D12CommandList *commandList = commandContext.GetGraphicsCommandList();
    commandList->ResourceBarrier(textureResource, currentState, D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprint;
    device->GetCopyableFootprints(&texture->textureDesc, request.subresourceIndex, 1, 0, &subresourceFootprint, nullptr, nullptr, nullptr);

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = request.uploadBuffer;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = subresourceFootprint;
    srcLocation.PlacedFootprint.Footprint.Width = request.width;
    srcLocation.PlacedFootprint.Footprint.Height = request.height;
    srcLocation.PlacedFootprint.Footprint.Depth = request.depth;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = textureResource;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = request.subresourceIndex;

    D3D12_BOX srcBox = { 0, 0, 0, (UINT)request.width, (UINT)request.height, (UINT)request.depth };
    commandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, request.x, request.y, request.z, &srcLocation, &srcBox);

    commandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, currentState);

    RequestRelease(request.uploadBuffer);
    return true;
}

bool D3D12Renderer::SetTextureSubImage2D(RHI::Texture *texture, int mipLevel, int x, int y, int width, int height, BE1::Image::Format imageFormat, const void *pixels) {
    return SetTextureSubImage(texture, mipLevel, 0, x, y, 0, width, height, 1, imageFormat, pixels);
}

bool D3D12Renderer::SetTextureSubImage3D(RHI::Texture *texture, int mipLevel, int x, int y, int z, int width, int height, int depth, BE1::Image::Format imageFormat, const void *pixels) {
    return SetTextureSubImage(texture, mipLevel, 0, x, y, z, width, height, depth, imageFormat, pixels);
}

bool D3D12Renderer::SetTextureSubImageCubeFace(RHI::Texture *texture, RHI::CubemapFace face, int mipLevel, int x, int y, int width, int height, BE1::Image::Format imageFormat, const void *pixels) {
    return SetTextureSubImage(texture, mipLevel, to_int(face), x, y, 0, width, height, 1, imageFormat, pixels);
}

void D3D12Renderer::GenerateMipmaps(RHI::Texture *texture) {
    D3D12ResourceRequest request;
    request.type = D3D12ResourceRequest::Type::GenerateMipmaps;
    request.object = texture;
    EnqueueResourceRequest(std::move(request));
}

void D3D12Renderer::GenerateMipmaps(RHI::CommandList *commandList, const RHI::Texture *texture, const RHI::ViewID *srvMipViewIDs, const RHI::ViewID *uavMipViewIDs, bool preserveCoverage) {
    const D3D12Texture *_texture = static_cast<const D3D12Texture *>(texture);
    uint32_t numMipmaps = _texture->textureDesc.MipLevels;
    if (numMipmaps <= 1) {
        BE_WARNLOG("D3D12Renderer::GenerateMipmaps: texture has no mipmap allocations\n");
        return;
    }

    // mip 레벨 개수만큼 SRV 와 UAV 가 생성되어 있지 않다면, 진행할 수 없다.
    // TODO: 엔트리 개수로만 체크하는 건 좀 허술하긴 한데, 일단은 간단히 이렇게 체크한다. 나중에 필요하면 좀 더 정교하게 체크하도록 하자.
    if (_texture->srvEntries.Count() < numMipmaps ||
        _texture->uavEntries.Count() < numMipmaps) {
        BE_WARNLOG("D3D12Renderer::GenerateMipmaps: Not enough SRV or UAV descriptors\n");
        return;
    }

    // packed 포맷이나 압축 포맷은 지원하지 않는다.
    if (BE1::Image::IsPacked(_texture->srcFormat) || BE1::Image::IsCompressed(_texture->srcFormat)) {
        return;
    }

    MipGenParams mipGenParams;
    mipGenParams.flags = 0;

    if (_texture->srcSRGB) {
        mipGenParams.flags |= MIPGEN_OPTION_BIT_SRGB;
    }

    if (preserveCoverage) {
        mipGenParams.flags |= MIPGEN_OPTION_BIT_PRESERVE_COVERAGE;
    }

    if (_texture->textureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        // TODO: Texture2DArray, CubemapArray 도 지원해야 함.
        if (_texture->type == RHI::TextureType::TextureCube && _texture->textureDesc.DepthOrArraySize == 6) {
            // Cubemap
            SetPSO(commandList, BE1::Image::IsFloatFormat(_texture->srcFormat) ? genMipmapsCubeFloat4PSO : genMipmapsCubeUNorm4PSO);

            for (uint32_t mipLevel = 0; mipLevel < numMipmaps - 1; ++mipLevel) {
                uint32_t destMipLevel = mipLevel + 1;
                uint32_t destW = BE1::Max(1, (int)(_texture->textureDesc.Width >> destMipLevel));
                uint32_t destH = BE1::Max(1, (int)(_texture->textureDesc.Height >> destMipLevel));

                mipGenParams.dstSize.x = destW;
                mipGenParams.dstSize.y = destH;
                mipGenParams.dstSize.z = 0;
                mipGenParams.dstSizeRcp.x = 1.0f / destW;
                mipGenParams.dstSizeRcp.y = 1.0f / destH;
                mipGenParams.dstSizeRcp.z = 0;

                uint32_t threadGroupCountX = (destW + GENMIP_2D_BLOCK_SIZE_X - 1) / GENMIP_2D_BLOCK_SIZE_X;
                uint32_t threadGroupCountY = (destH + GENMIP_2D_BLOCK_SIZE_Y - 1) / GENMIP_2D_BLOCK_SIZE_Y;
                uint32_t threadGroupCountZ = 6;

                // Set mipmap generation constants
                SetConstants(commandList, 0, &mipGenParams, sizeof(mipGenParams));
                // Set output texture (UAV)
                SetTexture(commandList, 0, true, texture, uavMipViewIDs[destMipLevel]);
                // Set input texture (SRV)
                SetTexture(commandList, 0, false, texture, srvMipViewIDs[mipLevel]);

                RHI::GPUBarrier startBarriers[] = {
                    RHI::Renderer::MakeImageBarrier(texture, _texture->defaultState, RHI::GPUResourceState::UnorderedAccess, 0, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, _texture->defaultState, RHI::GPUResourceState::UnorderedAccess, 1, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, _texture->defaultState, RHI::GPUResourceState::UnorderedAccess, 2, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, _texture->defaultState, RHI::GPUResourceState::UnorderedAccess, 3, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, _texture->defaultState, RHI::GPUResourceState::UnorderedAccess, 4, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, _texture->defaultState, RHI::GPUResourceState::UnorderedAccess, 5, destMipLevel)
                };
                Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

                Dispatch(commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);

                RHI::GPUBarrier endBarriers[] = {
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, _texture->defaultState, 0, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, _texture->defaultState, 1, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, _texture->defaultState, 2, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, _texture->defaultState, 3, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, _texture->defaultState, 4, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, _texture->defaultState, 5, destMipLevel)
                };
                Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
            }
        } else {
            // 2D
            SetPSO(commandList, BE1::Image::IsFloatFormat(_texture->srcFormat) ? genMipmaps2DFloat4PSO : genMipmaps2DUNorm4PSO);

            for (uint32_t mipLevel = 0; mipLevel < numMipmaps - 1; ++mipLevel) {
                uint32_t destMipLevel = mipLevel + 1;
                uint32_t destW = BE1::Max(1, (int)_texture->textureDesc.Width >> destMipLevel);
                uint32_t destH = BE1::Max(1, (int)_texture->textureDesc.Height >> destMipLevel);

                mipGenParams.dstSize.x = destW;
                mipGenParams.dstSize.y = destH;
                mipGenParams.dstSize.z = 0;
                mipGenParams.dstSizeRcp.x = 1.0f / destW;
                mipGenParams.dstSizeRcp.y = 1.0f / destH;
                mipGenParams.dstSizeRcp.z = 0;

                uint32_t threadGroupCountX = (destW + GENMIP_2D_BLOCK_SIZE_X - 1) / GENMIP_2D_BLOCK_SIZE_X;
                uint32_t threadGroupCountY = (destH + GENMIP_2D_BLOCK_SIZE_Y - 1) / GENMIP_2D_BLOCK_SIZE_Y;

                // Set mipmap generation constants
                SetConstants(commandList, 0, &mipGenParams, sizeof(mipGenParams));
                // Set output texture (UAV)
                SetTexture(commandList, 0, true, texture, uavMipViewIDs[destMipLevel]);
                // Set input texture (SRV)
                SetTexture(commandList, 0, false, texture, srvMipViewIDs[mipLevel]);

                RHI::GPUBarrier startBarriers[] = {
                    RHI::Renderer::MakeImageBarrier(texture, _texture->defaultState, RHI::GPUResourceState::UnorderedAccess, 0, destMipLevel)
                };
                Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

                Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

                RHI::GPUBarrier endBarriers[] = {
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, _texture->defaultState, 0, destMipLevel)
                };
                Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
            }
        }
    } else if (_texture->textureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        // 3D
        SetPSO(commandList, BE1::Image::IsFloatFormat(_texture->srcFormat) ? genMipmaps3DFloat4PSO : genMipmaps3DUNorm4PSO);

        for (uint32_t mipLevel = 0; mipLevel < numMipmaps - 1; ++mipLevel) {
            uint32_t destMipLevel = mipLevel + 1;
            uint32_t destW = BE1::Max(1, (int)_texture->textureDesc.Width >> destMipLevel);
            uint32_t destH = BE1::Max(1, (int)_texture->textureDesc.Height >> destMipLevel);
            uint32_t destD = BE1::Max(1, (int)_texture->textureDesc.DepthOrArraySize >> destMipLevel);

            mipGenParams.dstSize.x = destW;
            mipGenParams.dstSize.y = destH;
            mipGenParams.dstSize.z = destD;
            mipGenParams.dstSizeRcp.x = 1.0f / destW;
            mipGenParams.dstSizeRcp.y = 1.0f / destH;
            mipGenParams.dstSizeRcp.z = 1.0f / destD;

            uint32_t threadGroupCountX = (destW + GENMIP_3D_BLOCK_SIZE_X - 1) / GENMIP_3D_BLOCK_SIZE_X;
            uint32_t threadGroupCountY = (destH + GENMIP_3D_BLOCK_SIZE_Y - 1) / GENMIP_3D_BLOCK_SIZE_Y;
            uint32_t threadGroupCountZ = (destD + GENMIP_3D_BLOCK_SIZE_Z - 1) / GENMIP_3D_BLOCK_SIZE_Z;

            // Set mipmap generation constants
            SetConstants(commandList, 0, &mipGenParams, sizeof(mipGenParams));
            // Set output texture (UAV)
            SetTexture(commandList, 0, true, texture, uavMipViewIDs[destMipLevel]);
            // Set input texture (SRV)
            SetTexture(commandList, 0, false, texture, srvMipViewIDs[mipLevel]);

            RHI::GPUBarrier startBarriers[] = {
                RHI::Renderer::MakeImageBarrier(texture, _texture->defaultState, RHI::GPUResourceState::UnorderedAccess, 0, destMipLevel)
            };
            Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

            Dispatch(commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);

            RHI::GPUBarrier endBarriers[] = {
                RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, _texture->defaultState, 0, destMipLevel)
            };
            Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
        }
    }
}

bool D3D12Renderer::ExecuteGenerateMipmaps(D3D12ResourceCommandContext &commandContext, D3D12Texture *texture) {
    if (texture->GetStatus() != RHI::GPUResourceStatus::Ready) {
        BE_WARNLOG("D3D12Renderer::ExecuteGenerateMipmaps: texture is not ready\n");
        return false;
    }

    if (texture->GetMipLevelCount() <= 1) {
        BE_WARNLOG("D3D12Renderer::ExecuteGenerateMipmaps: texture has no mipmap allocations\n");
        return false;
    }

    // 전체 텍스쳐 리소스에 대한 SRV 생성
    if (!texture->HasValidView(RHI::ViewType::SRV, RHI::ViewID::PrimaryID)) {
        CreateView(texture, RHI::ViewType::SRV);
    }
    // 전체 텍스쳐 리소스에 대한 UAV 생성
    if (!texture->HasValidView(RHI::ViewType::UAV, RHI::ViewID::PrimaryID)) {
        CreateView(texture, RHI::ViewType::UAV);
    }

    RHI::ViewID *srvMipSubresourceIDs = (RHI::ViewID *)_alloca(sizeof(RHI::ViewID) * texture->GetMipLevelCount() * texture->GetArraySize());
    RHI::ViewID *uavMipSubresourceIDs = (RHI::ViewID *)_alloca(sizeof(RHI::ViewID) * texture->GetMipLevelCount() * texture->GetArraySize());

    // Mip Level 별로 임시 SRV/UAV 를 생성
    for (int mipLevel = 0; mipLevel < texture->GetMipLevelCount(); ++mipLevel) {
        srvMipSubresourceIDs[mipLevel] = CreateView(texture, RHI::ViewType::SRV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
        uavMipSubresourceIDs[mipLevel] = CreateView(texture, RHI::ViewType::UAV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
    }

    // 컴퓨트 쉐이더를 이용해서 밉맵 생성
    GenerateMipmaps(commandContext.GetGraphicsCommandList(), texture, srvMipSubresourceIDs, uavMipSubresourceIDs, true);

    // 임시 SRV/UAV 삭제
    for (int mipLevel = 0; mipLevel < texture->GetMipLevelCount(); ++mipLevel) {
        if (srvMipSubresourceIDs[mipLevel].IsValid()) {
            DestroyView(texture, RHI::ViewType::SRV, srvMipSubresourceIDs[mipLevel]);
        }
        if (uavMipSubresourceIDs[mipLevel].IsValid()) {
            DestroyView(texture, RHI::ViewType::UAV, uavMipSubresourceIDs[mipLevel]);
        }
    }

    // TODO:
    // 컴퓨트 커맨드 리스트/큐를 이용하려면, 텍스쳐를 만들 때 non-pixel shader resource 로 상태 전이가 이루어져야 한다.
    // 이후에 렌더링에 사용할 것이므로, 다이렉트 커맨드 리스트/큐를 통해 다시 pixel shader resource 로 상태 전이를 해야 한다.
    return true;
}

void D3D12Renderer::SetTexture(RHI::CommandList *commandList, int shaderRegister, bool shaderWritable, const RHI::Texture *texture, RHI::ViewID viewID) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    D3D12RootBindingData *rootBindingData = _commandList->GetRootBindingData();

    const D3D12Texture *_texture = static_cast<const D3D12Texture *>(texture);
    assert(_texture);

    // 셰이더 레지스터에 대한 루트 파라미터 인덱스를 얻는다.
    const D3D12PipelineState::RegisterBinder &binder = _commandList->currentPSO->binder;
    uint8_t rootParameterIndex = 0xFF;
    uint8_t indexInTable = 0xFF;

    if (shaderWritable) {
        // UAV
        assert(shaderRegister < COUNT_OF(binder.rootParameterIndexLUT.uav));
        rootParameterIndex = binder.rootParameterIndexLUT.uav[shaderRegister];
        if (rootParameterIndex >= 64) {
            BE_ERRLOG("D3D12Renderer::SetTexture: Invalid root parameter index\n");
            return;
        }

        // 루트 파라미터가 디스크립터 테이블일 경우 테이블 내의 인덱스 LUT 에는 유효한 값이 들어있다.
        indexInTable = binder.descriptorTableIndexLUT.uav[shaderRegister];
        if (indexInTable != 0xFF) {
            if (!_texture->HasValidView(RHI::ViewType::UAV, viewID)) {
                BE_ERRLOG("D3D12Renderer::SetTexture: Invalid UAV view ID (%i)\n", viewID.value);
                return;
            }

            // 사용할 디스크립터 테이블 변수에 디스크립터 핸들을 채워 넣는다.
            const D3D12UAVEntry &uavEntry = _texture->uavEntries[viewID.value];
            rootBindingData->descriptorTableHandles[rootParameterIndex][indexInTable] = uavEntry.cpuDescriptorHandle;
            if (rootBindingData->descriptorTableHandles[rootParameterIndex][indexInTable].ptr == 0) {
                BE_ERRLOG("D3D12Renderer::SetTexture: Texture has no valid UAV descriptor handle\n");
                return;
            }
        }
        rootBindingData->uavResources[shaderRegister] = _texture;
    } else {
        // SRV
        assert(shaderRegister < COUNT_OF(binder.rootParameterIndexLUT.srv));
        rootParameterIndex = binder.rootParameterIndexLUT.srv[shaderRegister];
        if (rootParameterIndex >= 64) {
            BE_ERRLOG("D3D12Renderer::SetTexture: Invalid root parameter index\n");
            return;
        }

        // 루트 파라미터가 디스크립터 테이블일 경우 테이블 내의 인덱스 LUT 에는 유효한 값이 들어있다.
        indexInTable = binder.descriptorTableIndexLUT.srv[shaderRegister];
        if (indexInTable != 0xFF) {
            if (!_texture->HasValidView(RHI::ViewType::SRV, viewID)) {
                BE_ERRLOG("D3D12Renderer::SetTexture: Invalid SRV view ID (%i)\n", viewID.value);
                return;
            }

            // 사용할 디스크립터 테이블 변수에 디스크립터 핸들을 채워 넣는다.
            const D3D12SRVEntry &srvEntry = _texture->srvEntries[viewID.value];
            rootBindingData->descriptorTableHandles[rootParameterIndex][indexInTable] = srvEntry.cpuDescriptorHandle;
            if (rootBindingData->descriptorTableHandles[rootParameterIndex][indexInTable].ptr == 0) {
                BE_ERRLOG("D3D12Renderer::SetTexture: Texture has no valid SRV descriptor handle\n");
                return;
            }
        }
        rootBindingData->srvResources[shaderRegister] = _texture;
    }

    // 변경할 루트 파라미터 인덱스에 대한 dirty mask 를 설정한다.
    if (_commandList->currentPSO->isGraphics) {
        _commandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        _commandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
