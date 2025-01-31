// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
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
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    int maxSrcMipLevels;
    int maxAllocationMipLevels;

    if (allocateEmptyMipmaps) {
        // 0 mip 레벨의 src 이미지만 사용하고, 나머지 서브 mip 레벨은 빈 공간으로 채운다.
        maxSrcMipLevels = 1;
        maxAllocationMipLevels = BE1::Image::MaxMipLevels(srcImage->GetWidth(), srcImage->GetHeight(), srcImage->GetDepth());
    } else {
        // src 이미지의 mip 레벨을 모두 사용한다.
        maxSrcMipLevels = srcImage->NumMipmaps();
        maxAllocationMipLevels = maxSrcMipLevels;
    }

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = textureDimension;
    textureDesc.Width = static_cast<UINT>(srcImage->GetWidth());
    textureDesc.Height = static_cast<UINT>(srcImage->GetHeight());
    textureDesc.DepthOrArraySize = static_cast<UINT>(textureDimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? srcImage->GetDepth() : srcImage->NumSlices());
    textureDesc.MipLevels = static_cast<UINT16>(maxAllocationMipLevels);
    textureDesc.Format = dxgiFormat;
    textureDesc.SampleDesc.Count = sampleCount;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Flags = resourceFlags;

    D3D12_CLEAR_VALUE optimizedClearValue = {};
    bool useClearValue = BE1::HasFlag(flags, RHI::ResourceFlag::RenderTarget) || BE1::HasFlag(flags, RHI::ResourceFlag::DepthStencil);
    if (useClearValue) {
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

    D3D12_RESOURCE_STATES d3d12InitialState = ToD3D12ResourceState(initialState);
    if (!srcImage->IsEmpty()) {
        d3d12InitialState = D3D12_RESOURCE_STATE_COMMON;
    }

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
        int numSlices = srcImage->NumSlices();
        int numFaces = srcImage->NumFaces();

        for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex) {
            for (int faceIndex = 0; faceIndex < numFaces; ++faceIndex) {
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
        }

        CD3DX12_RANGE writtenRange(0, size);
        uploadBuffer->Unmap(0, &writtenRange);

        // 업로드 버퍼에서 텍스쳐로 데이터 카피
        resourceCommandList->Reset();
        resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

        for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex) {
            for (int faceIndex = 0; faceIndex < numFaces; ++faceIndex) {
                for (int mipLevel = 0; mipLevel < maxSrcMipLevels; ++mipLevel) {
                    int subresourceIndex = maxSrcMipLevels * (numFaces * sliceIndex + faceIndex) + mipLevel;

                    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
                    srcLocation.PlacedFootprint = mipLevelFootprints[mipLevel];
                    srcLocation.pResource = uploadBuffer;
                    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

                    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
                    dstLocation.PlacedFootprint = mipLevelFootprints[mipLevel];
                    dstLocation.pResource = textureResource;
                    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    dstLocation.SubresourceIndex = subresourceIndex;

                    resourceCommandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
                }
            }
        }

        resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        resourceCommandList->CloseAndExecute(RHI::CommandQueueType::Graphics);

        if (uploadBuffer) {
            MarkForRelease(uploadBuffer);
        }
    }

    D3D12Texture *texture = new D3D12Texture;
    texture->textureType = textureType;
#ifdef USE_D3D12_MEMALLOC
    texture->textureAllocation = allocation;
#else
    texture->textureResource = textureResource;
#endif
    texture->textureDesc = textureResource->GetDesc();
    texture->clearValue = optimizedClearValue;
    texture->initialState = initialState;

    if (!BE1::HasFlag(flags, RHI::ResourceFlag::SkipDefaultViews)) {
        if (BE1::HasFlag(flags, RHI::ResourceFlag::ShaderResource)) {
            CreateSubresourceSRV(texture);
        }
        if (BE1::HasFlag(flags, RHI::ResourceFlag::RenderTarget)) {
            CreateSubresourceRTV(texture);
        }
        if (BE1::HasFlag(flags, RHI::ResourceFlag::DepthStencil)) {
            CreateSubresourceDSV(texture);
        }
        if (BE1::HasFlag(flags, RHI::ResourceFlag::UnorderedAccess)) {
            CreateSubresourceUAV(texture);
        }
    }

    return texture;
}

RHI::Texture *D3D12Renderer::CreateTexture(RHI::TextureType textureType, RHI::ResourceFlag flags, const BE1::Image *srcImage, BE1::Image::Format dstFormat, bool generateMipmaps) {
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
            mipmapedImage.Create(w, h, d, srcImage->NumSlices(), maxMipLevels, srcImage->GetFormat(), srcImage->GetGammaSpace(), nullptr, srcImage->GetFlags());
            mipmapedImage.CopyFrom(*srcImage, 0, 1);
            mipmapedImage.GenerateMipmaps();
            srcImage = &mipmapedImage;
        }
    }

    BE1::Image dstImage;

    // dstFormat 으로 컨버팅
    if (srcFormat != dstFormat) {
        srcImage->ConvertFormat(dstFormat, dstImage);
        srcImage = &dstImage;
    }

    return CreateTexture(textureType, flags, srcImage, false);
}

void D3D12Renderer::DestroyTexture(RHI::Texture *texture, bool immediate) {
    if (immediate) {
        delete texture;
    } else {
        MarkForDelete(texture);
    }
}

int D3D12Renderer::CreateSubresource(RHI::Texture *texture, RHI::SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel, uint32_t mipCount) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);

    if (type == RHI::SubresourceType::SRV) {
        return CreateSubresourceSRV(d3d12Texture, firstSlice, sliceCount, firstMipLevel, mipCount);
    }
    if (type == RHI::SubresourceType::RTV) {
        return CreateSubresourceRTV(d3d12Texture, firstSlice, sliceCount, firstMipLevel);
    }
    if (type == RHI::SubresourceType::DSV) {
        return CreateSubresourceDSV(d3d12Texture, firstSlice, sliceCount, firstMipLevel);
    }
    if (type == RHI::SubresourceType::UAV) {
        return CreateSubresourceUAV(d3d12Texture, firstSlice, sliceCount, firstMipLevel);
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
}

int D3D12Renderer::CreateSubresourceSRV(D3D12Texture *texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel, uint32_t mipCount) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture->textureDesc.Format;

    switch (texture->textureType) {
    case RHI::TextureType::Texture1D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        srvDesc.Texture1D.MostDetailedMip = firstMipLevel;
        srvDesc.Texture1D.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::Texture1DArray:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        srvDesc.Texture1DArray.FirstArraySlice = firstSlice;
        srvDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        srvDesc.Texture1DArray.MostDetailedMip = firstMipLevel;
        srvDesc.Texture1DArray.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::Texture2D:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        } else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = firstMipLevel;
            srvDesc.Texture2D.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        }
        break;
    case RHI::TextureType::Texture2DArray:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            srvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
            srvDesc.Texture2DMSArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        } else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.FirstArraySlice = firstSlice;
            srvDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
            srvDesc.Texture2DArray.MostDetailedMip = firstMipLevel;
            srvDesc.Texture2DArray.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        }
        break;
    case RHI::TextureType::TextureCube:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = firstMipLevel;
        srvDesc.TextureCube.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::TextureCubeArray:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDesc.TextureCubeArray.First2DArrayFace = firstSlice;
        srvDesc.TextureCubeArray.NumCubes = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        srvDesc.TextureCubeArray.MostDetailedMip = firstMipLevel;
        srvDesc.TextureCubeArray.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    case RHI::TextureType::Texture3D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MostDetailedMip = firstMipLevel;
        srvDesc.Texture3D.MipLevels = BE1::Min(mipCount, texture->textureDesc.MipLevels - firstMipLevel);
        break;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuDescriptorHandle;
    if (!resCpuDescriptorPool->Alloc(&srvCpuDescriptorHandle, nullptr)) {
        return -1;
    }
    device->CreateShaderResourceView(texture->GetResource(), &srvDesc, srvCpuDescriptorHandle);

    D3D12SRVDescriptor srvDescriptor;
    srvDescriptor.srvDesc = srvDesc;
    srvDescriptor.cpuDescriptorHandle = srvCpuDescriptorHandle;

    if (!texture->srvDescriptor.cpuDescriptorHandle.ptr) {
        texture->srvDescriptor = srvDescriptor;
        return -1;
    }
    return texture->subresourceSrvDescriptors.Append(srvDescriptor);
}

int D3D12Renderer::CreateSubresourceRTV(D3D12Texture *texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel) {
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = texture->textureDesc.Format;

    switch (texture->textureType) {
    case RHI::TextureType::Texture1D:
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
        rtvDesc.Texture1D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture1DArray:
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        rtvDesc.Texture1DArray.MipSlice = firstMipLevel;
        rtvDesc.Texture1DArray.FirstArraySlice = firstSlice;
        rtvDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture2D:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        } else {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = firstMipLevel;
        }
        break;
    case RHI::TextureType::Texture2DArray:
    case RHI::TextureType::TextureCube:
    case RHI::TextureType::TextureCubeArray:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            rtvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
            rtvDesc.Texture2DMSArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        } else {
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = firstMipLevel;
            rtvDesc.Texture2DArray.FirstArraySlice = firstSlice;
            rtvDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        }
        break;
    case RHI::TextureType::Texture3D:
        // TODO: 정말 가능한지 실험해 볼 것
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        rtvDesc.Texture3D.MipSlice = firstMipLevel;
        rtvDesc.Texture3D.FirstWSlice = 0;
        rtvDesc.Texture3D.WSize = -1;
        break;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuDescriptorHandle;
    if (!rtvCpuDescriptorPool->Alloc(&rtvCpuDescriptorHandle, nullptr)) {
        return -1;
    }
    device->CreateRenderTargetView(texture->GetResource(), &rtvDesc, rtvCpuDescriptorHandle);

    D3D12RTVDescriptor rtvDescriptor;
    rtvDescriptor.rtvDesc = rtvDesc;
    rtvDescriptor.cpuDescriptorHandle = rtvCpuDescriptorHandle;

    if (!texture->rtvDescriptor.cpuDescriptorHandle.ptr) {
        texture->rtvDescriptor = rtvDescriptor;
        return -1;
    }
    return texture->subresourceRtvDescriptors.Append(rtvDescriptor);
}

int D3D12Renderer::CreateSubresourceDSV(D3D12Texture *texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel) {
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = texture->textureDesc.Format;

    switch (texture->textureType) {
    case RHI::TextureType::Texture1D:
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
        dsvDesc.Texture1D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture1DArray:
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        dsvDesc.Texture1DArray.MipSlice = firstMipLevel;
        dsvDesc.Texture1DArray.FirstArraySlice = firstSlice;
        dsvDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture2D:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        } else {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = firstMipLevel;
        }
        break;
    case RHI::TextureType::Texture2DArray:
    case RHI::TextureType::TextureCube:
    case RHI::TextureType::TextureCubeArray:
        if (texture->textureDesc.SampleDesc.Count > 1) {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            dsvDesc.Texture2DMSArray.FirstArraySlice = firstSlice;
            dsvDesc.Texture2DMSArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        } else {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice = firstMipLevel;
            dsvDesc.Texture2DArray.FirstArraySlice = firstSlice;
            dsvDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        }
        break;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuDescriptorHandle;
    if (!dsvCpuDescriptorPool->Alloc(&dsvCpuDescriptorHandle, nullptr)) {
        return -1;
    }
    device->CreateDepthStencilView(texture->GetResource(), &dsvDesc, dsvCpuDescriptorHandle);

    D3D12DSVDescriptor dsvDescriptor;
    dsvDescriptor.dsvDesc = dsvDesc;
    dsvDescriptor.cpuDescriptorHandle = dsvCpuDescriptorHandle;

    if (!texture->dsvDescriptor.cpuDescriptorHandle.ptr) {
        texture->dsvDescriptor = dsvDescriptor;
        return -1;
    }
    return texture->subresourceDsvDescriptors.Append(dsvDescriptor);
}

int D3D12Renderer::CreateSubresourceUAV(D3D12Texture *texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMipLevel) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format = texture->textureDesc.Format;

    switch (texture->textureType) {
    case RHI::TextureType::Texture1D:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
        uavDesc.Texture1D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture1DArray:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        uavDesc.Texture1DArray.MipSlice = firstMipLevel;
        uavDesc.Texture1DArray.FirstArraySlice = firstSlice;
        uavDesc.Texture1DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture2D:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = firstMipLevel;
        break;
    case RHI::TextureType::Texture2DArray:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Texture2DArray.MipSlice = firstMipLevel;
        uavDesc.Texture2DArray.FirstArraySlice = firstSlice;
        uavDesc.Texture2DArray.ArraySize = BE1::Min(sliceCount, texture->textureDesc.DepthOrArraySize - firstSlice);
        break;
    case RHI::TextureType::Texture3D:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.MipSlice = firstMipLevel;
        uavDesc.Texture3D.FirstWSlice = 0;
        uavDesc.Texture3D.WSize = -1;
        break;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE uavCpuDescriptorHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE destCpuDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE destGpuDescriptorHandle;

    if (!uavCpuDescriptorPool->Alloc(&uavCpuDescriptorHandle, nullptr)) {
        return -1;
    }
    if (!uavGpuDescriptorPool->Alloc(&destCpuDescriptorHandle, &destGpuDescriptorHandle)) {
        return -1;
    }
    device->CreateUnorderedAccessView(texture->GetResource(), nullptr, &uavDesc, uavCpuDescriptorHandle);

    // 만들어진 UAV 디스크립터를 shader visible 한 디스크립터에 복사 (CPU + GPU)
    device->CopyDescriptorsSimple(1, destCpuDescriptorHandle, uavCpuDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12UAVDescriptor uavDescriptor;
    uavDescriptor.uavDesc = uavDesc;
    uavDescriptor.cpuDescriptorHandle = uavCpuDescriptorHandle;

    if (!texture->uavDescriptor.cpuDescriptorHandle.ptr) {
        texture->uavDescriptor = uavDescriptor;
        return -1;
    }
    return texture->subresourceUavDescriptors.Append(uavDescriptor);
}

void D3D12Renderer::GetTextureImage2D(RHI::Texture *texture, int level, BE1::Image::Format dstFormat, void *outPixels) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);
    assert(d3d12Texture);

    BE1::Image::Format textureImageFormat;
    bool isSRGB;
    if (!D3D12Renderer::DXGIFormatToImageFormat(d3d12Texture->textureDesc.Format, &textureImageFormat, &isSRGB)) {
        BE_WARNLOG("D3D12Texture::GetTexture2D: Unsupported DXGI format %i\n", d3d12Texture->textureDesc.Format);
        return;
    }

    // 텍스쳐 리소스의 특정 밉레벨 (서브 리소스) 의 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprint;
    UINT64 mipLevelSize;
    device->GetCopyableFootprints(&d3d12Texture->textureDesc, level, 1, 0, &mipLevelFootprint, nullptr, nullptr, &mipLevelSize);

    // 리드백 버퍼를 생성한다.
    D3D12Buffer *readbackBuffer = static_cast<D3D12Buffer *>(CreateBuffer(RHI::BufferUsage::Readback, RHI::ResourceFlag::None, mipLevelSize, textureImageFormat, 0, nullptr));
    if (!readbackBuffer) {
        BE_WARNLOG("D3D12Texture::GetTextureImage2D: Failed to create readback buffer\n");
        return;
    }

    ID3D12Resource *textureResource = d3d12Texture->GetResource();

    // 텍스쳐에서 리드백 버퍼로 복사한다.
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = textureResource;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = readbackBuffer->GetResource();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLocation.PlacedFootprint = mipLevelFootprint;

    resourceCommandList->Reset();
    resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
    resourceCommandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    resourceCommandList->CloseAndExecute(RHI::CommandQueueType::Graphics);

    // GPU 에서 복사가 끝날 때까지 기다린다.
    Finish(RHI::CommandQueueType::Graphics);

    // 리드백 버퍼를 Map 하여 내용을 메모리로 읽어온다.
    void *mappedPtr = nullptr;
    D3D12_RANGE readRange = { 0, mipLevelSize };
    readbackBuffer->GetResource()->Map(0, &readRange, &mappedPtr);

    const byte *srcPtr = (byte *)mappedPtr;
    byte *dstPtr = nullptr;

    BE1::Image tempImage;
    if (textureImageFormat != dstFormat) {
        // 컨버팅이 필요하다면, 리드백 버퍼의 내용을 tempImage 에 카피할 준비를 한다.
        tempImage.Create2D(d3d12Texture->textureDesc.Width, d3d12Texture->textureDesc.Height, 1, textureImageFormat, isSRGB ? BE1::Image::GammaSpace::sRGB : BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
        dstPtr = tempImage.GetPixels();
    } else {
        // 컨버팅할 필요가 없다면, 리드백 버퍼의 내용을 그대로 outPixels 로 카피할 준비를 한다.
        dstPtr = (byte *)outPixels;
    }

    int srcPitch = mipLevelFootprint.Footprint.RowPitch;
    int dstPitch = BE1::Image::MemRequired(d3d12Texture->textureDesc.Width, 1, 1, 1, textureImageFormat);

    for (UINT y = 0; y < d3d12Texture->textureDesc.Height; ++y) {
        BE1::simdProcessor->Memcpy(dstPtr, srcPtr, srcPitch);
        srcPtr += srcPitch;
        dstPtr += dstPitch;
    }

    D3D12_RANGE writtenRange = { 0, 0 };
    readbackBuffer->GetResource()->Unmap(0, &writtenRange);

    // 리드백 버퍼 삭제
    DestroyBuffer(readbackBuffer, true);

    // 필요하다면 컨버팅한다.
    if (textureImageFormat != dstFormat) {
        BE1::Image dstImage;
        if (tempImage.ConvertFormat(dstFormat, dstImage)) {
            BE1::simdProcessor->Memcpy(outPixels, dstImage.GetPixels(), dstImage.SizeInBytes());
        }
    }
}

bool D3D12Renderer::SetTextureSubImage2D(RHI::Texture *texture, int level, int x, int y, int width, int height, BE1::Image::Format srcFormat, const void *pixels) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);
    assert(d3d12Texture);

    // 텍스쳐 리소스의 특정 mipLevel 에 대한 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprint;
    device->GetCopyableFootprints(&d3d12Texture->textureDesc, level, 1, 0, &mipLevelFootprint, nullptr, nullptr, nullptr);

    int srcPitch = BE1::Image::MemRequired(width, 1, 1, 1, srcFormat);
    int dstPitch = mipLevelFootprint.Footprint.RowPitch;
    int uploadBufferSize = BE1::Image::MemRequired(dstPitch, height, 1, 1, srcFormat);

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

    for (UINT y = 0; y < height; ++y) {
        BE1::simdProcessor->MemcpyStream(dstPtr, srcPtr, srcPitch);
        srcPtr += srcPitch;
        dstPtr += dstPitch;
    }

    CD3DX12_RANGE writtenRange(0, uploadBufferSize);
    uploadBuffer->Unmap(0, &writtenRange);

    ID3D12Resource *textureResource = d3d12Texture->GetResource();

    // 업로드 버퍼에서 텍스쳐로 데이터 카피
    resourceCommandList->Reset();
    resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.PlacedFootprint = mipLevelFootprint;
    srcLocation.PlacedFootprint.Footprint.Width = width;
    srcLocation.PlacedFootprint.Footprint.Height = height;
    srcLocation.PlacedFootprint.Footprint.Depth = 1;
    srcLocation.pResource = uploadBuffer;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.PlacedFootprint = mipLevelFootprint;
    dstLocation.pResource = textureResource;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = level;

    D3D12_BOX srcBox = { 0, 0, 0, (UINT)width, (UINT)height, 1 };
    resourceCommandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, x, y, 0, &srcLocation, &srcBox);

    resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    resourceCommandList->CloseAndExecute(RHI::CommandQueueType::Graphics);

    MarkForRelease(uploadBuffer);

    return true;
}

bool D3D12Renderer::SetTextureSubImage3D(RHI::Texture *texture, int level, int x, int y, int z, int width, int height, int depth, BE1::Image::Format srcFormat, const void *pixels) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);
    assert(d3d12Texture);

    // 텍스쳐 리소스의 특정 mipLevel 에 대한 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprint;
    device->GetCopyableFootprints(&d3d12Texture->textureDesc, level, 1, 0, &mipLevelFootprint, nullptr, nullptr, nullptr);

    int srcPitch = BE1::Image::MemRequired(width, 1, 1, 1, srcFormat);
    int dstPitch = mipLevelFootprint.Footprint.RowPitch;
    int uploadBufferSize = BE1::Image::MemRequired(dstPitch, height, depth, 1, srcFormat);

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

    // 업로드 버퍼에서 텍스쳐로 데이터 카피
    resourceCommandList->Reset();
    resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.PlacedFootprint = mipLevelFootprint;
    srcLocation.PlacedFootprint.Footprint.Width = width;
    srcLocation.PlacedFootprint.Footprint.Height = height;
    srcLocation.PlacedFootprint.Footprint.Depth = depth;
    srcLocation.pResource = uploadBuffer;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.PlacedFootprint = mipLevelFootprint;
    dstLocation.pResource = textureResource;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = level;

    D3D12_BOX srcBox = { 0, 0, 0, (UINT)width, (UINT)height, (UINT)depth };
    resourceCommandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, x, y, z, &srcLocation, &srcBox);

    resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    resourceCommandList->CloseAndExecute(RHI::CommandQueueType::Graphics);

    MarkForRelease(uploadBuffer);

    return true;
}

void D3D12Renderer::GenerateMipmaps(RHI::CommandList *commandList, const RHI::Texture *texture) {
    const D3D12Texture *d3d12Texture = static_cast<const D3D12Texture *>(texture);
    uint32_t numMipmaps = d3d12Texture->textureDesc.MipLevels;
    if (numMipmaps <= 1) {
        return;
    }

    BE1::Image::Format imageFormat;
    bool isSRGB = false;
    DXGIFormatToImageFormat(d3d12Texture->textureDesc.Format, &imageFormat, &isSRGB);

    // packed 포맷이나 압축 포맷은 지원하지 않는다.
    if (BE1::Image::IsPacked(imageFormat) || BE1::Image::IsCompressed(imageFormat)) {
        return;
    }

    MipGenParams mipGenParams;
    mipGenParams.flags = 0;

    if (isSRGB) {
        mipGenParams.flags |= MIPGEN_OPTION_BIT_SRGB;
    }

    if (d3d12Texture->textureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
        SetPSO(commandList, BE1::Image::IsFloatFormat(imageFormat) ? genMipmaps2DFloat4PSO : genMipmaps2DUNorm4PSO);

        for (uint32_t mipLevel = 0; mipLevel < numMipmaps - 1; ++mipLevel) {
            uint32_t destMipLevel = mipLevel + 1;
            uint32_t destW = d3d12Texture->textureDesc.Width >> destMipLevel;
            uint32_t destH = d3d12Texture->textureDesc.Height >> destMipLevel;

            mipGenParams.dstSize.x = destW;
            mipGenParams.dstSize.y = destH;
            mipGenParams.dstSizeRcp.x = 1.0f / destW;
            mipGenParams.dstSizeRcp.y = 1.0f / destH;

            // Set mipmap generation constants
            SetConstants(commandList, &mipGenParams, sizeof(mipGenParams));
            // Set output texture
            SetTexture(commandList, 0, true, texture, destMipLevel);
            // Set input texture
            SetTexture(commandList, 0, false, texture, mipLevel);

            RHI::GPUBarrier startBarriers[] = {
                RHI::Renderer::MakeImageBarrier(texture, d3d12Texture->initialState, RHI::GPUResourceState::UnorderedAccess, 0, destMipLevel)
            };
            Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

            Dispatch(commandList,
                (destW + GENMIP_2D_BLOCK_SIZE - 1) / GENMIP_2D_BLOCK_SIZE,
                (destH + GENMIP_2D_BLOCK_SIZE - 1) / GENMIP_2D_BLOCK_SIZE, 1);

            RHI::GPUBarrier endBarriers[] = {
                RHI::Renderer::MakeImageBarrier(texture, RHI::GPUResourceState::UnorderedAccess, d3d12Texture->initialState, 0, destMipLevel)
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

    if (d3d12CommandList->GetCommandListType() == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        d3d12CommandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        d3d12CommandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
