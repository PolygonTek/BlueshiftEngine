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
#include "D3D12FrameData.h"
#include "Shaders/ShaderInterop.h"

void D3D12Texture::Release() {
    if (srvDescriptor.cpuDescriptorHandle.ptr) {
        D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(srvDescriptor.cpuDescriptorHandle);
        srvDescriptor.cpuDescriptorHandle = {};
    }

    if (uavDescriptor.cpuDescriptorHandle.ptr) {
        D3D12Renderer::GetRenderer()->uavCpuDescriptorPool->Free(uavDescriptor.cpuDescriptorHandle);
        uavDescriptor.cpuDescriptorHandle = {};
    }

    if (rtvDescriptor.cpuDescriptorHandle.ptr) {
        D3D12Renderer::GetRenderer()->rtvCpuDescriptorPool->Free(rtvDescriptor.cpuDescriptorHandle);
        rtvDescriptor.cpuDescriptorHandle = {};
    }

    if (dsvDescriptor.cpuDescriptorHandle.ptr) {
        D3D12Renderer::GetRenderer()->dsvCpuDescriptorPool->Free(dsvDescriptor.cpuDescriptorHandle);
        dsvDescriptor.cpuDescriptorHandle = {};
    }

    for (const D3D12SRVDescriptor &srvDescriptor : subresourceSrvDescriptors) {
        if (srvDescriptor.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->resCpuDescriptorPool->Free(srvDescriptor.cpuDescriptorHandle);
        }
    }
    subresourceSrvDescriptors.Clear();

    for (const D3D12UAVDescriptor &uavDescriptor : subresourceUavDescriptors) {
        if (uavDescriptor.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->uavCpuDescriptorPool->Free(uavDescriptor.cpuDescriptorHandle);
        }
    }
    subresourceUavDescriptors.Clear();

    for (const D3D12RTVDescriptor &rtvDescriptor : subresourceRtvDescriptors) {
        if (rtvDescriptor.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->rtvCpuDescriptorPool->Free(rtvDescriptor.cpuDescriptorHandle);
        }
    }
    subresourceRtvDescriptors.Clear();

    for (const D3D12DSVDescriptor &dsvDescriptor : subresourceDsvDescriptors) {
        if (dsvDescriptor.cpuDescriptorHandle.ptr != 0) {
            D3D12Renderer::GetRenderer()->dsvCpuDescriptorPool->Free(dsvDescriptor.cpuDescriptorHandle);
        }
    }
    subresourceDsvDescriptors.Clear();

#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(textureAllocation);
#else
    SAFE_RELEASE(textureResource);
#endif
}

bool D3D12Texture::IsValidSubresource(RHI::SubresourceType type, int subresourceIndex) const {
    if (type == RHI::SubresourceType::SRV) {
        if (subresourceIndex == -1) {
            return !!srvDescriptor.cpuDescriptorHandle.ptr;
        }
        return subresourceSrvDescriptors.IsValidIndex(subresourceIndex);
    }
    if (type == RHI::SubresourceType::UAV) {
        if (subresourceIndex == -1) {
            return !!uavDescriptor.cpuDescriptorHandle.ptr;
        }
        return subresourceUavDescriptors.IsValidIndex(subresourceIndex);
    }
    if (type == RHI::SubresourceType::RTV) {
        if (subresourceIndex == -1) {
            return !!rtvDescriptor.cpuDescriptorHandle.ptr;
        }
        return subresourceRtvDescriptors.IsValidIndex(subresourceIndex);
    }
    if (type == RHI::SubresourceType::DSV) {
        if (subresourceIndex == -1) {
            return !!dsvDescriptor.cpuDescriptorHandle.ptr;
        }
        return subresourceDsvDescriptors.IsValidIndex(subresourceIndex);
    }
    return false;
}

RHI::Texture *D3D12Renderer::CreateTexture(RHI::TextureType textureType, RHI::ResourceFlag flags, const BE1::Image *srcImage, bool allocateEmptyMipmaps, const RHI::ClearValue &clearValue, uint32_t sampleCount, RHI::GPUResourceState initialState) {
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
    }

    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

    if (BE1::HasFlag(flags, RHI::ResourceFlag::RenderTarget)) {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (BE1::HasFlag(flags, RHI::ResourceFlag::DepthStencil)) {
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if (BE1::HasFlag(flags, RHI::ResourceFlag::UnorderedAccess)) {
        // DSV 와 UAV 는 같은 리소스로 사용할 수 없다.
        assert(!(resourceFlags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    int maxSrcMipLevels;
    int maxAllocMipLevels;

    if (allocateEmptyMipmaps) {
        // 0 mip 레벨의 src 이미지만 사용하고, 나머지 서브 mip 레벨은 빈 공간으로 채운다.
        maxSrcMipLevels = 1;
        maxAllocMipLevels = BE1::Image::MaxMipLevels(srcImage->GetWidth(), srcImage->GetHeight(), srcImage->GetDepth());
    } else {
        // src 이미지의 mip 레벨을 모두 사용한다.
        maxSrcMipLevels = srcImage->NumMipmaps();
        maxAllocMipLevels = maxSrcMipLevels;
    }

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = textureDimension;
    textureDesc.Width = static_cast<UINT>(srcImage->GetWidth());
    textureDesc.Height = static_cast<UINT>(srcImage->GetHeight());
    textureDesc.DepthOrArraySize = static_cast<UINT>(textureDimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? srcImage->GetDepth() : srcImage->GetArraySize());
    textureDesc.MipLevels = static_cast<UINT16>(maxAllocMipLevels);
    textureDesc.Format = dxgiFormat;
    textureDesc.SampleDesc.Count = sampleCount;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Flags = resourceFlags;

    D3D12_CLEAR_VALUE optimizedClearValue = {};
    bool useClearValue = BE1::HasFlag(flags, RHI::ResourceFlag::RenderTarget) || BE1::HasFlag(flags, RHI::ResourceFlag::DepthStencil);
    if (useClearValue) {
        // Typed 포맷이어야 한다.
        optimizedClearValue.Format = textureDesc.Format;
        if (BE1::HasFlag(flags, RHI::ResourceFlag::RenderTarget)) {
            optimizedClearValue.Color[0] = clearValue.color[0];
            optimizedClearValue.Color[1] = clearValue.color[1];
            optimizedClearValue.Color[2] = clearValue.color[2];
            optimizedClearValue.Color[3] = clearValue.color[3];
        }
        if (BE1::HasFlag(flags, RHI::ResourceFlag::DepthStencil)) {
            optimizedClearValue.DepthStencil.Depth = clearValue.depthStencil.depth;
            optimizedClearValue.DepthStencil.Stencil = clearValue.depthStencil.stencil;
        }
    }

    if (BE1::HasFlag(flags, RHI::ResourceFlag::Typeless)) {
        // optimizedClearValue.Format 을 설정한 후에 Typeless 포맷으로 변환한다.
        textureDesc.Format = ToTypelessFormat(textureDesc.Format);
    }

    D3D12_RESOURCE_STATES d3d12InitialState = ToD3D12ResourceState(initialState);

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_TIME;
    allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12MA::Allocation *allocation;
    if (FAILED(allocator->CreateResource(
        &allocationDesc,
        &textureDesc,
        d3d12InitialState,
        useClearValue ? &optimizedClearValue : nullptr,
        &allocation,
        IID_NULL, nullptr))) {
        return nullptr;
    }
    ID3D12Resource *textureResource = allocation->GetResource();
#else
    ID3D12Resource *textureResource = nullptr;
    if (FAILED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        d3d12InitialState,
        useClearValue ? &optimizedClearValue : nullptr,
        IID_PPV_ARGS(&textureResource)))) {
        return nullptr;
    }
#endif

    if (!srcImage->IsEmpty()) {
        // 텍스쳐 리소스의 서브 리소스 별 메모리 정보를 얻어온다.
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprints[16];
        UINT64 size;
        device->GetCopyableFootprints(&textureDesc, 0, textureDesc.MipLevels, 0, mipLevelFootprints, nullptr, nullptr, &size);

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
            textureResource->Release();
            return nullptr;
        }

        // 이미지 데이터를 업로드 버퍼에 write
        UINT8 *mappedPtr = nullptr;
        uploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mappedPtr));

        byte *dstPtr = mappedPtr;
        int bpp = srcImage->IsCompressed() ? srcImage->BytesPerBlock() : srcImage->BytesPerPixel();
        int numSlices = srcImage->GetArraySize();

        for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex) {
            for (int mipLevel = 0; mipLevel < maxSrcMipLevels; ++mipLevel) {
                int srcWidth = srcImage->GetWidth(mipLevel);
                int srcHeight = srcImage->GetHeight(mipLevel);
                int srcDepth = srcImage->GetDepth(mipLevel);
                int srcPitch = (srcImage->IsCompressed() ? (srcWidth >> 2) : srcWidth) * bpp;
                int srcRows = srcImage->IsCompressed() ? (srcHeight >> 2) : srcHeight;
                const byte *srcPtr = srcImage->GetPixels(mipLevel);

                for (int z = 0; z < srcDepth; ++z) {
                    for (int r = 0; r < srcRows; ++r) {
                        BE1::simdProcessor->Memcpy(dstPtr, srcPtr, srcPitch);
                        srcPtr += srcPitch;
                        dstPtr += mipLevelFootprints[mipLevel].Footprint.RowPitch;
                    }
                }
            }
        }

        CD3DX12_RANGE writtenRange(0, size);
        uploadBuffer->Unmap(0, &writtenRange);

        // 업로드 버퍼에서 텍스쳐로 데이터 카피
        D3D12CommandList *commandList = static_cast<D3D12CommandList *>(BeginCommandList(RHI::CommandQueueType::Graphics));
        commandList->ResourceBarrier(textureResource, d3d12InitialState, D3D12_RESOURCE_STATE_COPY_DEST);

        for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex) {
            for (int mipLevel = 0; mipLevel < maxSrcMipLevels; ++mipLevel) {
                int subresourceIndex = maxSrcMipLevels * sliceIndex + mipLevel;

                D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
                srcLocation.PlacedFootprint = mipLevelFootprints[mipLevel];
                srcLocation.pResource = uploadBuffer;
                srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

                D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
                dstLocation.PlacedFootprint = mipLevelFootprints[mipLevel];
                dstLocation.pResource = textureResource;
                dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                dstLocation.SubresourceIndex = subresourceIndex;

                commandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
            }
        }

        commandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, d3d12InitialState);
        commandList->CloseAndExecute();
        EndCommandList(commandList);

        if (uploadBuffer) {
            MarkForRelease(uploadBuffer);
        }
    }

    D3D12Texture *texture = new D3D12Texture;
    texture->textureType = textureType;
    texture->srcFormat = srcFormat;
    texture->srcSRGB = !isLinearSpace;
#ifdef USE_D3D12_MEMALLOC
    texture->textureAllocation = allocation;
#else
    texture->textureResource = textureResource;
#endif
    texture->textureDesc = textureResource->GetDesc();
    texture->clearValue = optimizedClearValue;
    texture->currentState = initialState;

    if (!BE1::HasFlag(flags, RHI::ResourceFlag::SkipDefaultViews)) {
        if (BE1::HasFlag(flags, RHI::ResourceFlag::ShaderResource)) {
            CreateSubresourceSRV(texture);
        }
        if (BE1::HasFlag(flags, RHI::ResourceFlag::UnorderedAccess)) {
            CreateSubresourceUAV(texture);
        }
        if (BE1::HasFlag(flags, RHI::ResourceFlag::RenderTarget)) {
            CreateSubresourceRTV(texture);
        }
        if (BE1::HasFlag(flags, RHI::ResourceFlag::DepthStencil)) {
            CreateSubresourceDSV(texture);
        }
    }

    return texture;
}

RHI::Texture *D3D12Renderer::CreateTexture(RHI::TextureType textureType, RHI::ResourceFlag flags, const BE1::Image *srcImage, BE1::Image::Format dstFormat, bool generateMipmaps, bool allocateEmptyMipmaps, RHI::GPUResourceState initialState) {
    BE1::Image::Format srcFormat = srcImage->GetFormat();

    bool srcCompressed = BE1::Image::IsCompressed(srcFormat);
    bool srcFormatSupported = IsSupportedImageFormat(srcFormat);
    bool dstFormatSupported = IsSupportedImageFormat(dstFormat);

    if (!dstFormatSupported) {
        BE_WARNLOG("D3D12Renderer::CreateTexture: Unsupported internal image format %s\n", BE1::Image::FormatName(dstFormat));
        return nullptr;
    }

    BE1::Image uncompressedImage;
    BE1::Image mipmapedImage;
    BE1::Image dstImage;

    if (!srcImage->IsEmpty()) {
        // 필요하다면 밉맵을 생성한다.
        if (generateMipmaps) {
            int w = srcImage->GetWidth();
            int h = srcImage->GetHeight();
            int d = srcImage->GetDepth();
            int maxMipLevels = BE1::Image::MaxMipLevels(w, h, d);

            // srcImage 가 이미 밉맵을 포함하고 있으면 무시된다.
            if (srcImage->NumMipmaps() < maxMipLevels) {
                if (srcImage->IsPacked() || srcImage->IsCompressed()) {
                    // 밉맵을 생성해야 한다면, 지원되는 가장 비슷한 무압축 포맷으로 컨버팅한다.
                    BE1::Image::Format supportedUncompressedFormat = ToUncompressedImageFormat(srcFormat);

                    srcImage->ConvertFormat(supportedUncompressedFormat, uncompressedImage);
                    srcImage = &uncompressedImage;

                    srcFormat = supportedUncompressedFormat;
                    srcFormatSupported = IsSupportedImageFormat(srcFormat);
                    srcCompressed = false;
                }

                // CPU 에서 밉맵을 직접 생성한다.
                mipmapedImage.Create(w, h, d, srcImage->GetArraySize(), maxMipLevels, srcImage->GetFormat(), srcImage->GetGammaSpace(), nullptr, srcImage->GetFlags());
                mipmapedImage.CopyFrom(*srcImage, 0, 1);
                mipmapedImage.GenerateMipmaps();
                srcImage = &mipmapedImage;
            }
        }

        // dstFormat 으로 컨버팅
        if (srcFormat != dstFormat) {
            srcImage->ConvertFormat(dstFormat, dstImage);
            srcImage = &dstImage;
        }
    }

    return CreateTexture(textureType, flags, srcImage, allocateEmptyMipmaps, {}, 1, initialState);
}

void D3D12Renderer::DestroyTexture(RHI::Texture *texture, bool immediate) {
    if (immediate) {
        delete texture;
    } else {
        MarkForDelete(texture);
    }
}

int D3D12Renderer::CreateSubresource(RHI::Texture *texture, RHI::SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel, uint32_t mipCount, const BE1::Image::Format *typelessCompatibleFormat, bool isSRGB) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);

    if (type == RHI::SubresourceType::SRV) {
        return CreateSubresourceSRV(d3d12Texture, firstSlice, sliceCount, firstMipLevel, mipCount, typelessCompatibleFormat, isSRGB);
    }
    if (type == RHI::SubresourceType::UAV) {
        return CreateSubresourceUAV(d3d12Texture, firstSlice, sliceCount, firstMipLevel, typelessCompatibleFormat, isSRGB);
    }
    if (type == RHI::SubresourceType::RTV) {
        return CreateSubresourceRTV(d3d12Texture, firstSlice, sliceCount, firstMipLevel, typelessCompatibleFormat, isSRGB);
    }
    if (type == RHI::SubresourceType::DSV) {
        return CreateSubresourceDSV(d3d12Texture, firstSlice, sliceCount, firstMipLevel, typelessCompatibleFormat, isSRGB);
    }
    return -1;
}

void D3D12Renderer::DestroySubresource(RHI::Texture *texture, RHI::SubresourceType type, int subresourceIndex) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);

    if (type == RHI::SubresourceType::SRV) {
        if (subresourceIndex < 0) {
            if (d3d12Texture->srvDescriptor.cpuDescriptorHandle.ptr) {
                resCpuDescriptorPool->Free(d3d12Texture->srvDescriptor.cpuDescriptorHandle);
                d3d12Texture->srvDescriptor = {};
            }
        } else {
            if (!d3d12Texture->subresourceSrvDescriptors.IsValidIndex(subresourceIndex)) {
                BE_ERRLOG("D3D12Renderer::DestroySubresource: Invalid SRV subresource index (%i)\n", subresourceIndex);
                return;
            }
            resCpuDescriptorPool->Free(d3d12Texture->subresourceSrvDescriptors[subresourceIndex].cpuDescriptorHandle);
            d3d12Texture->subresourceSrvDescriptors.RemoveIndexFast(subresourceIndex);
        }
        return;
    }
    if (type == RHI::SubresourceType::UAV) {
        if (subresourceIndex < 0) {
            if (d3d12Texture->uavDescriptor.cpuDescriptorHandle.ptr) {
                uavCpuDescriptorPool->Free(d3d12Texture->uavDescriptor.cpuDescriptorHandle);
                d3d12Texture->uavDescriptor = {};
            }
        } else {
            if (!d3d12Texture->subresourceUavDescriptors.IsValidIndex(subresourceIndex)) {
                BE_ERRLOG("D3D12Renderer::DestroySubresource: Invalid UAV subresource index (%i)\n", subresourceIndex);
                return;
            }
            uavCpuDescriptorPool->Free(d3d12Texture->subresourceUavDescriptors[subresourceIndex].cpuDescriptorHandle);
            d3d12Texture->subresourceUavDescriptors.RemoveIndexFast(subresourceIndex);
        }
        return;
    }
    if (type == RHI::SubresourceType::RTV) {
        if (subresourceIndex < 0) {
            if (d3d12Texture->rtvDescriptor.cpuDescriptorHandle.ptr) {
                rtvCpuDescriptorPool->Free(d3d12Texture->rtvDescriptor.cpuDescriptorHandle);
                d3d12Texture->rtvDescriptor = {};
            }
        } else {
            if (!d3d12Texture->subresourceRtvDescriptors.IsValidIndex(subresourceIndex)) {
                BE_ERRLOG("D3D12Renderer::DestroySubresource: Invalid RTV subresource index (%i)\n", subresourceIndex);
                return;
            }
            rtvCpuDescriptorPool->Free(d3d12Texture->subresourceRtvDescriptors[subresourceIndex].cpuDescriptorHandle);
            d3d12Texture->subresourceRtvDescriptors.RemoveIndexFast(subresourceIndex);
        }
        return;
    }
    if (type == RHI::SubresourceType::DSV) {
        if (subresourceIndex < 0) {
            if (d3d12Texture->dsvDescriptor.cpuDescriptorHandle.ptr) {
                dsvCpuDescriptorPool->Free(d3d12Texture->dsvDescriptor.cpuDescriptorHandle);
                d3d12Texture->dsvDescriptor = {};
            }
        } else {
            if (!d3d12Texture->subresourceDsvDescriptors.IsValidIndex(subresourceIndex)) {
                BE_ERRLOG("D3D12Renderer::DestroySubresource: Invalid DSV subresource index (%i)\n", subresourceIndex);
                return;
            }
            dsvCpuDescriptorPool->Free(d3d12Texture->subresourceDsvDescriptors[subresourceIndex].cpuDescriptorHandle);
            d3d12Texture->subresourceDsvDescriptors.RemoveIndexFast(subresourceIndex);
        }
        return;
    }
}

int D3D12Renderer::CreateSubresourceSRV(D3D12Texture *texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel, uint32_t mipCount, const BE1::Image::Format *typelessCompatibleFormat, bool isSRGB) {
    D3D12SRVDescriptor srvDescriptor;
    if (!resCpuDescriptorPool->Alloc(&srvDescriptor.cpuDescriptorHandle, nullptr)) {
        return -1;
    }

    srvDescriptor.srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    BE1::Image::Format subresourceFormat;

    if (typelessCompatibleFormat) {
        assert(IsTypelessFormat(texture->textureDesc.Format));
        subresourceFormat = *typelessCompatibleFormat;

        ImageFormatToDXGIFormat(subresourceFormat, isSRGB, &srvDescriptor.srvDesc.Format);
    } else {
        subresourceFormat = texture->srcFormat;

        switch (texture->srcFormat) {
        case BE1::Image::Format::D16:
            srvDescriptor.srvDesc.Format = DXGI_FORMAT_R16_UNORM;
            break;
        case BE1::Image::Format::D24S8:
            srvDescriptor.srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            break;
        case BE1::Image::Format::D32_FLOAT:
            srvDescriptor.srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
            break;
        case BE1::Image::Format::D32_FLOAT_S8X24:
            srvDescriptor.srvDesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
            break;
        default:
            ImageFormatToDXGIFormat(texture->srcFormat, texture->srcSRGB, &srvDescriptor.srvDesc.Format);
            break;
        }
    }

    // 텍스쳐의 Image::Format 에 따라 swizzling 이 필요할 수도 있다.
    srvDescriptor.srvDesc.Shader4ComponentMapping = GetComponentSwizzling(subresourceFormat);

    switch (texture->textureType) {
    case RHI::TextureType::Texture1D:
        srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        srvDescriptor.srvDesc.Texture1D.MostDetailedMip = firstMipLevel;
        srvDescriptor.srvDesc.Texture1D.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::Texture1DArray:
        srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        srvDescriptor.srvDesc.Texture1DArray.FirstArraySlice = firstSlice;
        srvDescriptor.srvDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        srvDescriptor.srvDesc.Texture1DArray.MostDetailedMip = firstMipLevel;
        srvDescriptor.srvDesc.Texture1DArray.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::Texture2D:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        } else {
            srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDescriptor.srvDesc.Texture2D.MostDetailedMip = firstMipLevel;
            srvDescriptor.srvDesc.Texture2D.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        }
        break;
    case RHI::TextureType::Texture2DArray:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            srvDescriptor.srvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
            srvDescriptor.srvDesc.Texture2DMSArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        } else {
            srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDescriptor.srvDesc.Texture2DArray.FirstArraySlice = firstSlice;
            srvDescriptor.srvDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
            srvDescriptor.srvDesc.Texture2DArray.MostDetailedMip = firstMipLevel;
            srvDescriptor.srvDesc.Texture2DArray.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        }
        break;
    case RHI::TextureType::TextureCube:
        srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDescriptor.srvDesc.TextureCube.MostDetailedMip = firstMipLevel;
        srvDescriptor.srvDesc.TextureCube.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::TextureCubeArray:
        srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDescriptor.srvDesc.TextureCubeArray.First2DArrayFace = firstSlice;
        srvDescriptor.srvDesc.TextureCubeArray.NumCubes = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        srvDescriptor.srvDesc.TextureCubeArray.MostDetailedMip = firstMipLevel;
        srvDescriptor.srvDesc.TextureCubeArray.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::Texture3D:
        srvDescriptor.srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDescriptor.srvDesc.Texture3D.MostDetailedMip = firstMipLevel;
        srvDescriptor.srvDesc.Texture3D.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    }

    device->CreateShaderResourceView(texture->GetResource(), &srvDescriptor.srvDesc, srvDescriptor.cpuDescriptorHandle);

    // 전체 리소스에 대한 SRV
    if (!texture->srvDescriptor.cpuDescriptorHandle.ptr) {
        texture->srvDescriptor = srvDescriptor;
        return -1;
    }
    return texture->subresourceSrvDescriptors.Append(srvDescriptor);
}

int D3D12Renderer::CreateSubresourceUAV(D3D12Texture *texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel, const BE1::Image::Format *typelessCompatibleFormat, bool isSRGB) {
    D3D12UAVDescriptor uavDescriptor;
    if (!uavCpuDescriptorPool->Alloc(&uavDescriptor.cpuDescriptorHandle, nullptr)) {
        return -1;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE destCpuDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE destGpuDescriptorHandle;

    if (!uavGpuDescriptorPool->Alloc(&destCpuDescriptorHandle, &destGpuDescriptorHandle)) {
        return -1;
    }

    uavDescriptor.uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

    if (typelessCompatibleFormat) {
        assert(IsTypelessFormat(texture->textureDesc.Format));
        ImageFormatToDXGIFormat(*typelessCompatibleFormat, isSRGB, &uavDescriptor.uavDesc.Format);
    } else {
        // UAV 는 sRGB 포맷으로 생성할 수 없다.
        ImageFormatToDXGIFormat(texture->srcFormat, false, &uavDescriptor.uavDesc.Format);
    }

    // UAV 호환 포맷인지 검사 필요
    if (!IsSupportedUAVFormat(uavDescriptor.uavDesc.Format)) {
        BE_ERRLOG("Unsupported UAV format (0x%x)\n", uavDescriptor.uavDesc.Format);
        return -1;
    }

    switch (texture->textureType) {
    case RHI::TextureType::Texture1D:
        uavDescriptor.uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
        uavDescriptor.uavDesc.Texture1D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture1DArray:
        uavDescriptor.uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        uavDescriptor.uavDesc.Texture1DArray.MipSlice = firstMipLevel;
        uavDescriptor.uavDesc.Texture1DArray.FirstArraySlice = firstSlice;
        uavDescriptor.uavDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture2D:
        uavDescriptor.uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDescriptor.uavDesc.Texture2D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture2DArray:
        uavDescriptor.uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDescriptor.uavDesc.Texture2DArray.MipSlice = firstMipLevel;
        uavDescriptor.uavDesc.Texture2DArray.FirstArraySlice = firstSlice;
        uavDescriptor.uavDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture3D:
        uavDescriptor.uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDescriptor.uavDesc.Texture3D.MipSlice = firstMipLevel;
        uavDescriptor.uavDesc.Texture3D.FirstWSlice = 0;
        uavDescriptor.uavDesc.Texture3D.WSize = -1;
        break;
    }

    device->CreateUnorderedAccessView(texture->GetResource(), nullptr, &uavDescriptor.uavDesc, uavDescriptor.cpuDescriptorHandle);

    // 만들어진 UAV 디스크립터를 shader visible 한 디스크립터에 복사 (CPU + GPU)
    device->CopyDescriptorsSimple(1, destCpuDescriptorHandle, uavDescriptor.cpuDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 전체 리소스에 대한 UAV
    if (!texture->uavDescriptor.cpuDescriptorHandle.ptr) {
        texture->uavDescriptor = uavDescriptor;
        return -1;
    }
    return texture->subresourceUavDescriptors.Append(uavDescriptor);
}

int D3D12Renderer::CreateSubresourceRTV(D3D12Texture *texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel, const BE1::Image::Format *typelessCompatibleFormat, bool isSRGB) {
    D3D12RTVDescriptor rtvDescriptor;
    if (!rtvCpuDescriptorPool->Alloc(&rtvDescriptor.cpuDescriptorHandle, nullptr)) {
        return -1;
    }

    if (typelessCompatibleFormat) {
        assert(IsTypelessFormat(texture->textureDesc.Format));
        ImageFormatToDXGIFormat(*typelessCompatibleFormat, isSRGB, &rtvDescriptor.rtvDesc.Format);
    } else {
        ImageFormatToDXGIFormat(texture->srcFormat, texture->srcSRGB, &rtvDescriptor.rtvDesc.Format);
    }

    switch (texture->textureType) {
    case RHI::TextureType::Texture1D:
        rtvDescriptor.rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
        rtvDescriptor.rtvDesc.Texture1D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture1DArray:
        rtvDescriptor.rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        rtvDescriptor.rtvDesc.Texture1DArray.MipSlice = firstMipLevel;
        rtvDescriptor.rtvDesc.Texture1DArray.FirstArraySlice = firstSlice;
        rtvDescriptor.rtvDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture2D:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            rtvDescriptor.rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        } else {
            rtvDescriptor.rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDescriptor.rtvDesc.Texture2D.MipSlice = firstMipLevel;
        }
        break;
    case RHI::TextureType::Texture2DArray:
    case RHI::TextureType::TextureCube:
    case RHI::TextureType::TextureCubeArray:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            rtvDescriptor.rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            rtvDescriptor.rtvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
            rtvDescriptor.rtvDesc.Texture2DMSArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        } else {
            rtvDescriptor.rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDescriptor.rtvDesc.Texture2DArray.MipSlice = firstMipLevel;
            rtvDescriptor.rtvDesc.Texture2DArray.FirstArraySlice = firstSlice;
            rtvDescriptor.rtvDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        }
        break;
    case RHI::TextureType::Texture3D:
        // TODO: 정말 가능한지 실험해 볼 것
        rtvDescriptor.rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        rtvDescriptor.rtvDesc.Texture3D.MipSlice = firstMipLevel;
        rtvDescriptor.rtvDesc.Texture3D.FirstWSlice = 0;
        rtvDescriptor.rtvDesc.Texture3D.WSize = -1;
        break;
    }

    device->CreateRenderTargetView(texture->GetResource(), &rtvDescriptor.rtvDesc, rtvDescriptor.cpuDescriptorHandle);

    // 전체 리소스에 대한 RTV
    if (!texture->rtvDescriptor.cpuDescriptorHandle.ptr) {
        texture->rtvDescriptor = rtvDescriptor;
        return -1;
    }
    return texture->subresourceRtvDescriptors.Append(rtvDescriptor);
}

int D3D12Renderer::CreateSubresourceDSV(D3D12Texture *texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel, const BE1::Image::Format *typelessCompatibleFormat, bool isSRGB) {
    D3D12DSVDescriptor dsvDescriptor;
    if (!dsvCpuDescriptorPool->Alloc(&dsvDescriptor.cpuDescriptorHandle, nullptr)) {
        return -1;
    }

    if (typelessCompatibleFormat) {
        assert(IsTypelessFormat(texture->textureDesc.Format));
        ImageFormatToDXGIFormat(*typelessCompatibleFormat, isSRGB, &dsvDescriptor.dsvDesc.Format);
    } else {
        ImageFormatToDXGIFormat(texture->srcFormat, texture->srcSRGB, &dsvDescriptor.dsvDesc.Format);
    }

    switch (texture->textureType) {
    case RHI::TextureType::Texture1D:
        dsvDescriptor.dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
        dsvDescriptor.dsvDesc.Texture1D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture1DArray:
        dsvDescriptor.dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        dsvDescriptor.dsvDesc.Texture1DArray.MipSlice = firstMipLevel;
        dsvDescriptor.dsvDesc.Texture1DArray.FirstArraySlice = firstSlice;
        dsvDescriptor.dsvDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture2D:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            dsvDescriptor.dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        } else {
            dsvDescriptor.dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDescriptor.dsvDesc.Texture2D.MipSlice = firstMipLevel;
        }
        break;
    case RHI::TextureType::Texture2DArray:
    case RHI::TextureType::TextureCube:
    case RHI::TextureType::TextureCubeArray:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            dsvDescriptor.dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            dsvDescriptor.dsvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
            dsvDescriptor.dsvDesc.Texture2DMSArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        } else {
            dsvDescriptor.dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDescriptor.dsvDesc.Texture2DArray.MipSlice = firstMipLevel;
            dsvDescriptor.dsvDesc.Texture2DArray.FirstArraySlice = firstSlice;
            dsvDescriptor.dsvDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        }
        break;
    }


    device->CreateDepthStencilView(texture->GetResource(), &dsvDescriptor.dsvDesc, dsvDescriptor.cpuDescriptorHandle);

    // 전체 리소스에 대한 DSV
    if (!texture->dsvDescriptor.cpuDescriptorHandle.ptr) {
        texture->dsvDescriptor = dsvDescriptor;
        return -1;
    }
    return texture->subresourceDsvDescriptors.Append(dsvDescriptor);
}

void D3D12Renderer::GetTextureImage(RHI::Texture *texture, int mipLevel, int sliceIndex, BE1::Image::Format dstFormat, void *outPixels) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);
    assert(d3d12Texture);

    // 서브 리소스 인덱스를 계산한다.
    int subresourceIndex = D3D12CalcSubresource(mipLevel, sliceIndex, 0, d3d12Texture->textureDesc.MipLevels, d3d12Texture->textureDesc.DepthOrArraySize);

    // 서브 리소스의 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprint;
    UINT64 subresourceSize;
    device->GetCopyableFootprints(&d3d12Texture->textureDesc, subresourceIndex, 1, 0, &subresourceFootprint, nullptr, nullptr, &subresourceSize);

    // 리드백 버퍼를 생성한다.
    D3D12Buffer *readbackBuffer = static_cast<D3D12Buffer *>(CreateBuffer(RHI::BufferUsage::Readback, RHI::ResourceFlag::None, subresourceSize, d3d12Texture->srcFormat, 0, nullptr));
    if (!readbackBuffer) {
        BE_WARNLOG("D3D12Texture::GetTextureImage2D: Failed to create readback buffer\n");
        return;
    }

    ID3D12Resource *textureResource = d3d12Texture->GetResource();
    D3D12_RESOURCE_STATES currentState = ToD3D12ResourceState(d3d12Texture->currentState);

    // 텍스쳐에서 리드백 버퍼로 복사한다.
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = textureResource;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = subresourceIndex;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = readbackBuffer->GetResource();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLocation.PlacedFootprint = subresourceFootprint;

    D3D12CommandList *commandList = static_cast<D3D12CommandList *>(BeginCommandList(RHI::CommandQueueType::Graphics));
    commandList->ResourceBarrier(textureResource, currentState, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    commandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_SOURCE, currentState);
    commandList->CloseAndExecute();
    EndCommandList(commandList);

    // GPU 에서 복사가 끝날 때까지 기다린다.
    Finish(RHI::CommandQueueType::Graphics);

    // 리드백 버퍼를 Map 하여 내용을 메모리로 읽어온다.
    void *mappedPtr = nullptr;
    D3D12_RANGE readRange = { 0, subresourceSize };
    readbackBuffer->GetResource()->Map(0, &readRange, &mappedPtr);

    const byte *srcPtr = (byte *)mappedPtr;
    byte *dstPtr = nullptr;

    BE1::Image tempImage;
    if (d3d12Texture->srcFormat != dstFormat) {
        // 컨버팅이 필요하다면, 리드백 버퍼의 내용을 tempImage 에 카피할 준비를 한다.
        tempImage.Create2D(subresourceFootprint.Footprint.Width, subresourceFootprint.Footprint.Height, subresourceFootprint.Footprint.Depth,
            d3d12Texture->srcFormat, d3d12Texture->srcSRGB ? BE1::Image::GammaSpace::sRGB : BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
        dstPtr = tempImage.GetPixels();
    } else {
        // 컨버팅할 필요가 없다면, 리드백 버퍼의 내용을 그대로 outPixels 로 카피할 준비를 한다.
        dstPtr = (byte *)outPixels;
    }

    int srcPitch = subresourceFootprint.Footprint.RowPitch;
    int dstPitch = BE1::Image::MemRequired(subresourceFootprint.Footprint.Width, 1, 1, 1, 1, d3d12Texture->srcFormat);

    for (UINT z = 0; z < subresourceFootprint.Footprint.Depth; ++z) {
        for (UINT y = 0; y < subresourceFootprint.Footprint.Height; ++y) {
            BE1::simdProcessor->Memcpy(dstPtr, srcPtr, dstPitch);
            srcPtr += srcPitch;
            dstPtr += dstPitch;
        }
    }

    D3D12_RANGE writtenRange = { 0, 0 };
    readbackBuffer->GetResource()->Unmap(0, &writtenRange);

    // 리드백 버퍼 삭제
    DestroyBuffer(readbackBuffer, true);

    // 필요하다면 컨버팅한다.
    if (d3d12Texture->srcFormat != dstFormat) {
        BE1::Image dstImage;
        if (tempImage.ConvertFormat(dstFormat, dstImage)) {
            BE1::simdProcessor->Memcpy(outPixels, dstImage.GetPixels(), dstImage.SizeInBytes());
        }
    }
}

void D3D12Renderer::GetTextureImage2D(RHI::Texture *texture, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) {
    GetTextureImage(texture, mipLevel, 0, imageFormat, outPixels);
}

void D3D12Renderer::GetTextureImage3D(RHI::Texture *texture, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) {
    GetTextureImage(texture, mipLevel, 0, imageFormat, outPixels);
}

void D3D12Renderer::GetTextureImageCubeFace(RHI::Texture *texture, RHI::CubemapFace face, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) {
    GetTextureImage(texture, mipLevel, to_int(face), imageFormat, outPixels);
}

bool D3D12Renderer::SetTextureSubImage(RHI::Texture *texture, int mipLevel, int sliceIndex, int x, int y, int z, int width, int height, int depth, BE1::Image::Format imageFormat, const void *pixels) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);
    assert(d3d12Texture);

    // 서브 리소스 인덱스를 계산한다.
    int subresourceIndex = D3D12CalcSubresource(mipLevel, sliceIndex, 0, d3d12Texture->textureDesc.MipLevels, d3d12Texture->textureDesc.DepthOrArraySize);

    // 서브 리소스의 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT subresourceFootprint;
    device->GetCopyableFootprints(&d3d12Texture->textureDesc, subresourceIndex, 1, 0, &subresourceFootprint, nullptr, nullptr, nullptr);

    int srcPitch = BE1::Image::MemRequired(width, 1, 1, 1, 1, imageFormat);
    int dstPitch = subresourceFootprint.Footprint.RowPitch;
    int uploadBufferSize = BE1::Image::MemRequired(dstPitch, height, depth, 1, 1, imageFormat);

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
            BE1::simdProcessor->MemcpyStream(dstPtr, srcPtr, srcPitch);
            srcPtr += srcPitch;
            dstPtr += dstPitch;
        }
    }

    CD3DX12_RANGE writtenRange(0, uploadBufferSize);
    uploadBuffer->Unmap(0, &writtenRange);

    ID3D12Resource *textureResource = d3d12Texture->GetResource();
    D3D12_RESOURCE_STATES currentState = ToD3D12ResourceState(d3d12Texture->currentState);

    // 업로드 버퍼에서 텍스쳐로 데이터 카피
    D3D12CommandList *commandList = static_cast<D3D12CommandList *>(BeginCommandList(RHI::CommandQueueType::Graphics));
    commandList->ResourceBarrier(textureResource, currentState, D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.PlacedFootprint = subresourceFootprint;
    srcLocation.PlacedFootprint.Footprint.Width = width;
    srcLocation.PlacedFootprint.Footprint.Height = height;
    srcLocation.PlacedFootprint.Footprint.Depth = depth;
    srcLocation.pResource = uploadBuffer;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.PlacedFootprint = subresourceFootprint;
    dstLocation.pResource = textureResource;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = subresourceIndex;

    D3D12_BOX srcBox = { 0, 0, 0, (UINT)width, (UINT)height, (UINT)depth };
    commandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, x, y, z, &srcLocation, &srcBox);

    commandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, currentState);
    commandList->CloseAndExecute();
    EndCommandList(commandList);

    MarkForRelease(uploadBuffer);

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

void D3D12Renderer::GenerateMipmaps(RHI::CommandList *commandList, const RHI::Texture *texture, bool preserveCoverage) {
    const D3D12Texture *d3d12Texture = static_cast<const D3D12Texture *>(texture);
    uint32_t numMipmaps = d3d12Texture->textureDesc.MipLevels;
    if (numMipmaps <= 1) {
        BE_WARNLOG("D3D12Renderer::GenerateMipmaps: texture has no mipmaps allocations\n");
        return;
    }

    // mip 레벨 개수만큼 SRV 와 UAV 가 생성되어 있지 않다면, 진행할 수 없다.
    if (d3d12Texture->subresourceSrvDescriptors.Count() < numMipmaps ||
        d3d12Texture->subresourceUavDescriptors.Count() < numMipmaps) {
        BE_WARNLOG("D3D12Renderer::GenerateMipmaps: Not enough SRV or UAV descriptors\n");
        return;
    }

    // packed 포맷이나 압축 포맷은 지원하지 않는다.
    if (BE1::Image::IsPacked(d3d12Texture->srcFormat) || BE1::Image::IsCompressed(d3d12Texture->srcFormat)) {
        return;
    }

    MipGenParams mipGenParams;
    mipGenParams.flags = 0;

    if (d3d12Texture->srcSRGB) {
        mipGenParams.flags |= MIPGEN_OPTION_BIT_SRGB;
    }

    if (preserveCoverage) {
        mipGenParams.flags |= MIPGEN_OPTION_BIT_PRESERVE_COVERAGE;
    }

    if (d3d12Texture->textureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        if (d3d12Texture->textureDesc.DepthOrArraySize == 6) {
            // Cubemap
            SetPSO(commandList, BE1::Image::IsFloatFormat(d3d12Texture->srcFormat) ? genMipmapsCubeFloat4PSO : genMipmapsCubeUNorm4PSO);

            for (uint32_t mipLevel = 0; mipLevel < numMipmaps - 1; ++mipLevel) {
                uint32_t destMipLevel = mipLevel + 1;
                uint32_t destW = d3d12Texture->textureDesc.Width >> destMipLevel;
                uint32_t destH = d3d12Texture->textureDesc.Height >> destMipLevel;

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
                SetConstants(commandList, &mipGenParams, sizeof(mipGenParams));
                // Set output texture (UAV)
                SetTexture(commandList, 0, true, texture, destMipLevel);
                // Set input texture (SRV)
                SetTexture(commandList, 0, false, texture, mipLevel);

                RHI::GPUBarrier startBarriers[] = {
                    RHI::Renderer::MakeImageBarrier(texture, d3d12Texture->currentState, RHI::GPUResourceState::UnorderedAccess, 0, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, d3d12Texture->currentState, RHI::GPUResourceState::UnorderedAccess, 1, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, d3d12Texture->currentState, RHI::GPUResourceState::UnorderedAccess, 2, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, d3d12Texture->currentState, RHI::GPUResourceState::UnorderedAccess, 3, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, d3d12Texture->currentState, RHI::GPUResourceState::UnorderedAccess, 4, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, d3d12Texture->currentState, RHI::GPUResourceState::UnorderedAccess, 5, destMipLevel)
                };
                Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

                Dispatch(commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);

                RHI::GPUBarrier endBarriers[] = {
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, d3d12Texture->currentState, 0, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, d3d12Texture->currentState, 1, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, d3d12Texture->currentState, 2, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, d3d12Texture->currentState, 3, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, d3d12Texture->currentState, 4, destMipLevel),
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, d3d12Texture->currentState, 5, destMipLevel)
                };
                Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
            }
        } else {
            // 2D
            SetPSO(commandList, BE1::Image::IsFloatFormat(d3d12Texture->srcFormat) ? genMipmaps2DFloat4PSO : genMipmaps2DUNorm4PSO);

            for (uint32_t mipLevel = 0; mipLevel < numMipmaps - 1; ++mipLevel) {
                uint32_t destMipLevel = mipLevel + 1;
                uint32_t destW = d3d12Texture->textureDesc.Width >> destMipLevel;
                uint32_t destH = d3d12Texture->textureDesc.Height >> destMipLevel;

                mipGenParams.dstSize.x = destW;
                mipGenParams.dstSize.y = destH;
                mipGenParams.dstSize.z = 0;
                mipGenParams.dstSizeRcp.x = 1.0f / destW;
                mipGenParams.dstSizeRcp.y = 1.0f / destH;
                mipGenParams.dstSizeRcp.z = 0;

                uint32_t threadGroupCountX = (destW + GENMIP_2D_BLOCK_SIZE_X - 1) / GENMIP_2D_BLOCK_SIZE_X;
                uint32_t threadGroupCountY = (destH + GENMIP_2D_BLOCK_SIZE_Y - 1) / GENMIP_2D_BLOCK_SIZE_Y;

                // Set mipmap generation constants
                SetConstants(commandList, &mipGenParams, sizeof(mipGenParams));
                // Set output texture (UAV)
                SetTexture(commandList, 0, true, texture, destMipLevel);
                // Set input texture (SRV)
                SetTexture(commandList, 0, false, texture, mipLevel);

                RHI::GPUBarrier startBarriers[] = {
                    RHI::Renderer::MakeImageBarrier(texture, d3d12Texture->currentState, RHI::GPUResourceState::UnorderedAccess, 0, destMipLevel)
                };
                Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

                Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

                RHI::GPUBarrier endBarriers[] = {
                    RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, d3d12Texture->currentState, 0, destMipLevel)
                };
                Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
            }
        }
    } else if (d3d12Texture->textureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
        // 3D
        SetPSO(commandList, BE1::Image::IsFloatFormat(d3d12Texture->srcFormat) ? genMipmaps3DFloat4PSO : genMipmaps3DUNorm4PSO);

        for (uint32_t mipLevel = 0; mipLevel < numMipmaps - 1; ++mipLevel) {
            uint32_t destMipLevel = mipLevel + 1;
            uint32_t destW = d3d12Texture->textureDesc.Width >> destMipLevel;
            uint32_t destH = d3d12Texture->textureDesc.Height >> destMipLevel;
            uint32_t destD = d3d12Texture->textureDesc.DepthOrArraySize >> destMipLevel;

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
            SetConstants(commandList, &mipGenParams, sizeof(mipGenParams));
            // Set output texture (UAV)
            SetTexture(commandList, 0, true, texture, destMipLevel);
            // Set input texture (SRV)
            SetTexture(commandList, 0, false, texture, mipLevel);

            RHI::GPUBarrier startBarriers[] = {
                RHI::Renderer::MakeImageBarrier(texture, d3d12Texture->currentState, RHI::GPUResourceState::UnorderedAccess, 0, destMipLevel)
            };
            Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

            Dispatch(commandList, threadGroupCountX, threadGroupCountY, threadGroupCountZ);

            RHI::GPUBarrier endBarriers[] = {
                RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, d3d12Texture->currentState, 0, destMipLevel)
            };
            Barrier(commandList, endBarriers, COUNT_OF(endBarriers));
        }
    }
}

void D3D12Renderer::SetTexture(RHI::CommandList *commandList, int slot, bool shaderWritable, const RHI::Texture *texture, int subresourceIndex) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    D3D12FrameThreadData *threadData = static_cast<D3D12FrameThreadData *>(d3d12CommandList->GetFrameThreadData());

    const D3D12Texture *d3d12Texture = static_cast<const D3D12Texture *>(texture);
    assert(d3d12Texture);

    // 슬롯 (레지스터) 에 대한 루트 파라미터 인덱스를 얻고, 디스크립터 테이블일 경우 테이블 인덱스도 얻어온다.
    const D3D12PipelineState::Binder &binder = d3d12CommandList->currentPSO->binder;
    uint8_t rootParameterIndex = 0xFF;
    uint8_t descriptorIndex = 0xFF;

    if (shaderWritable) {
        // UAV
        rootParameterIndex = binder.rootParameterBinder.uav[slot];
        descriptorIndex = binder.descriptorTableBinder.uav[slot];
        if (descriptorIndex != 0xFF) {
            const D3D12UAVDescriptor &uavDescriptor = subresourceIndex < 0 ? d3d12Texture->uavDescriptor : d3d12Texture->subresourceUavDescriptors[subresourceIndex];
            threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex] = uavDescriptor.cpuDescriptorHandle;
            if (threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex].ptr == 0) {
                BE_ERRLOG("D3D12Renderer::SetTexture: Texture has no valid UAV descriptor handle\n");
                return;
            }
        }
        threadData->uavResources[slot] = d3d12Texture;
    } else {
        // SRV
        rootParameterIndex = binder.rootParameterBinder.srv[slot];
        descriptorIndex = binder.descriptorTableBinder.srv[slot];
        if (descriptorIndex != 0xFF) {
            const D3D12SRVDescriptor &srvDescriptor = subresourceIndex < 0 ? d3d12Texture->srvDescriptor : d3d12Texture->subresourceSrvDescriptors[subresourceIndex];
            threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex] = srvDescriptor.cpuDescriptorHandle;
            if (threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorIndex].ptr == 0) {
                BE_ERRLOG("D3D12Renderer::SetTexture: Texture has no valid SRV descriptor handle\n");
                return;
            }
        }
        threadData->srvResources[slot] = d3d12Texture;
    }

    if (rootParameterIndex == 0xFF) {
        BE_ERRLOG("D3D12Renderer::SetTexture: Invalid root parameter index\n");
        return;
    }

    if (d3d12CommandList->currentPSO->graphics) {
        d3d12CommandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        d3d12CommandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
