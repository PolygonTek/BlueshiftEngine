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

void D3D12Texture::Release() {
    if (srvDescriptorHandle.ptr != 0) {
        renderer->srvDescriptorPool->Free(srvDescriptorHandle);
        srvDescriptorHandle.ptr = 0;
    }
    if (rtvDescriptorHandle.ptr != 0) {
        renderer->srvDescriptorPool->Free(rtvDescriptorHandle);
        rtvDescriptorHandle.ptr = 0;
    }
    if (dsvDescriptorHandle.ptr != 0) {
        renderer->srvDescriptorPool->Free(dsvDescriptorHandle);
        dsvDescriptorHandle.ptr = 0;
    }
    if (uavDescriptorHandle.ptr != 0) {
        renderer->srvDescriptorPool->Free(uavDescriptorHandle);
        uavDescriptorHandle.ptr = 0;
    }
#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(textureAllocation);
#else
    SAFE_RELEASE(textureResource);
#endif
}

ID3D12Resource *D3D12Texture::GetResource() const {
#ifdef USE_D3D12_MEMALLOC
    return textureAllocation->GetResource();
#else
    return textureResource;
#endif
}

void D3D12Texture::AdjustTextureFormat(bool useCompression, bool useNormalMap, Image::Format::Enum inFormat, Image::Format::Enum *outFormat) {
    if (Image::IsDepthFormat(inFormat) || Image::IsDepthStencilFormat(inFormat)) {
        *outFormat = inFormat;
        return;
    }

    if (Image::IsCompressed(inFormat)) {
        if (D3D12Renderer::IsSupportedImageFormat(inFormat)) {
            *outFormat = inFormat;
            return;
        }

        inFormat = D3D12Renderer::ToUncompressedImageFormat(inFormat);
    }

    *outFormat = useCompression ? D3D12Renderer::ToCompressedImageFormat(inFormat, useNormalMap) : D3D12Renderer::ToUncompressedImageFormat(inFormat);
}

RHIRenderer::Texture *D3D12Renderer::CreateTexture(TextureType textureType, int flags, const Image *srcImage) {
    Image::Format::Enum srcFormat = srcImage->GetFormat();
    bool isLinearSpace = srcImage->GetGammaSpace() == Image::GammaSpace::Linear;

    DXGI_FORMAT dxgiFormat;
    bool srcFormatSupported = D3D12Renderer::ImageFormatToDXGIFormat(srcFormat, !isLinearSpace, &dxgiFormat);
    if (!srcFormatSupported) {
        BE_WARNLOG("D3D12Renderer::CreateTexture: Unsupported image format %s\n", Image::FormatName(srcFormat));
        return nullptr;
    }

    D3D12_RESOURCE_DIMENSION textureDimension;
    switch (textureType) {
    case TextureType::Texture1D:
        textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        break;
    case TextureType::Texture2D:
    case TextureType::Texture2DArray:
    case TextureType::TextureCube:
    case TextureType::TextureCubeArray:
        textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        break;
    case TextureType::Texture3D:
        textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        break;
    }

    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

    int maxMipLevels = srcImage->NumMipmaps();

    // GPU 에 텍스쳐 리소스 생성
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.Dimension = textureDimension;
    textureDesc.Format = dxgiFormat;
    textureDesc.MipLevels = static_cast<UINT16>(maxMipLevels);
    textureDesc.Width = static_cast<UINT>(srcImage->GetWidth());
    textureDesc.Height = static_cast<UINT>(srcImage->GetHeight());
    textureDesc.DepthOrArraySize = static_cast<UINT>(textureDimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? srcImage->GetDepth() : srcImage->NumSlices());
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Flags = resourceFlags;

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.Flags |= D3D12MA::ALLOCATION_FLAG_STRATEGY_MIN_TIME;
    allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12MA::Allocation *allocation;
    if (FAILED(allocator->CreateResource(
        &allocationDesc,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
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
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&textureResource)))) {
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
                for (int mipLevel = 0; mipLevel < maxMipLevels; ++mipLevel) {
                    int srcWidth = srcImage->GetWidth(mipLevel);
                    int srcHeight = srcImage->GetHeight(mipLevel);
                    int srcDepth = srcImage->GetDepth(mipLevel);
                    int srcPitch = (srcImage->IsCompressed() ? (srcWidth >> 2) : srcWidth) * bpp;
                    int srcRows = srcImage->IsCompressed() ? (srcHeight >> 2) : srcHeight;
                    const byte *srcPtr = srcImage->GetPixels(mipLevel);

                    for (int z = 0; z < srcDepth; ++z) {
                        for (int r = 0; r < srcRows; ++r) {
                            simdProcessor->Memcpy(dstPtr, srcPtr, srcPitch);
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

        for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex) {
            for (int faceIndex = 0; faceIndex < numFaces; ++faceIndex) {
                for (int mipLevel = 0; mipLevel < maxMipLevels; ++mipLevel) {
                    int subresourceIndex = maxMipLevels * (numFaces * sliceIndex + faceIndex) + mipLevel;

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
        resourceCommandList->CloseAndExecute(CommandQueueType::Graphics);

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

    if (!(flags & ResourceFlag::SkipDefaultViews)) {
        if (flags & ResourceFlag::ShaderResource) {
            CreateSubresource(texture, SubresourceType::SRV);
        }
        if (flags & ResourceFlag::RenderTarget) {
            CreateSubresource(texture, SubresourceType::RTV);
        }
        if (flags & ResourceFlag::DepthStencil) {
            CreateSubresource(texture, SubresourceType::DSV);
        }
        if (flags & ResourceFlag::UnorderedAccess) {
            CreateSubresource(texture, SubresourceType::UAV);
        }
    }

    return texture;
}

void D3D12Renderer::CreateSubresource(Texture *texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);

    if (type == SubresourceType::SRV) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = d3d12Texture->textureDesc.Format;

        switch (d3d12Texture->textureType) {
        case TextureType::Texture1D:
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1D.MostDetailedMip = firstMip;
            srvDesc.Texture1D.MipLevels = Min(mipCount, d3d12Texture->textureDesc.MipLevels - firstMip);
            break;
        case TextureType::Texture1DArray:
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            srvDesc.Texture1DArray.FirstArraySlice = firstSlice;
            srvDesc.Texture1DArray.ArraySize = Min(sliceCount, d3d12Texture->textureDesc.DepthOrArraySize - firstSlice);
            srvDesc.Texture1DArray.MostDetailedMip = firstMip;
            srvDesc.Texture1DArray.MipLevels = Min(mipCount, d3d12Texture->textureDesc.MipLevels - firstMip);
            break;
        case TextureType::Texture2D:
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = firstMip;
            srvDesc.Texture2D.MipLevels = Min(mipCount, d3d12Texture->textureDesc.MipLevels - firstMip);
            break;
        case TextureType::Texture2DArray:
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.FirstArraySlice = firstSlice;
            srvDesc.Texture2DArray.ArraySize = Min(sliceCount, d3d12Texture->textureDesc.DepthOrArraySize - firstSlice);
            srvDesc.Texture2DArray.MostDetailedMip = firstMip;
            srvDesc.Texture2DArray.MipLevels = Min(mipCount, d3d12Texture->textureDesc.MipLevels - firstMip);
            break;
        case TextureType::Texture3D:
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MostDetailedMip = firstMip;
            srvDesc.Texture3D.MipLevels = Min(mipCount, d3d12Texture->textureDesc.MipLevels - firstMip);
            break;
        case TextureType::TextureCube:
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MostDetailedMip = firstMip;
            srvDesc.TextureCube.MipLevels = Min(mipCount, d3d12Texture->textureDesc.MipLevels - firstMip);
            break;
        case TextureType::TextureCubeArray:
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            srvDesc.TextureCubeArray.First2DArrayFace = firstSlice;
            srvDesc.TextureCubeArray.NumCubes = Min(sliceCount, d3d12Texture->textureDesc.DepthOrArraySize - firstSlice);
            srvDesc.TextureCubeArray.MostDetailedMip = firstMip;
            srvDesc.TextureCubeArray.MipLevels = Min(mipCount, d3d12Texture->textureDesc.MipLevels - firstMip);
            break;
        }

        d3d12Texture->srvDescriptorHandle = srvDescriptorPool->Alloc();
        device->CreateShaderResourceView(d3d12Texture->GetResource(), &srvDesc, d3d12Texture->srvDescriptorHandle);
    } else if (type == SubresourceType::RTV) {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = d3d12Texture->textureDesc.Format;

        switch (d3d12Texture->textureType) {
        case TextureType::Texture1D:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
            rtvDesc.Texture1D.MipSlice = firstMip;
            break;
        case TextureType::Texture1DArray:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
            rtvDesc.Texture1DArray.MipSlice = firstMip;
            rtvDesc.Texture1DArray.FirstArraySlice = firstSlice;
            rtvDesc.Texture1DArray.ArraySize = Min(sliceCount, d3d12Texture->textureDesc.DepthOrArraySize - firstSlice);
            break;
        case TextureType::Texture2D:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = firstMip;
            break;
        case TextureType::Texture2DArray:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = firstMip;
            rtvDesc.Texture2DArray.FirstArraySlice = firstSlice;
            rtvDesc.Texture2DArray.ArraySize = Min(sliceCount, d3d12Texture->textureDesc.DepthOrArraySize - firstSlice);
            break;
        case TextureType::Texture3D:
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice = firstMip;
            rtvDesc.Texture3D.FirstWSlice = 0;
            rtvDesc.Texture3D.WSize = -1;
            break;
        }

        d3d12Texture->rtvDescriptorHandle = rtvDescriptorPool->Alloc();
        device->CreateRenderTargetView(d3d12Texture->GetResource(), &rtvDesc, d3d12Texture->rtvDescriptorHandle);
    } else if (type == SubresourceType::DSV) {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = d3d12Texture->textureDesc.Format;

        switch (d3d12Texture->textureType) {
        case TextureType::Texture1D:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
            dsvDesc.Texture1D.MipSlice = firstMip;
            break;
        case TextureType::Texture1DArray:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            dsvDesc.Texture1DArray.MipSlice = firstMip;
            dsvDesc.Texture1DArray.FirstArraySlice = firstSlice;
            dsvDesc.Texture1DArray.ArraySize = Min(sliceCount, d3d12Texture->textureDesc.DepthOrArraySize - firstSlice);
            break;
        case TextureType::Texture2D:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = firstMip;
            break;
        case TextureType::Texture2DArray:
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice = firstMip;
            dsvDesc.Texture2DArray.FirstArraySlice = firstSlice;
            dsvDesc.Texture2DArray.ArraySize = Min(sliceCount, d3d12Texture->textureDesc.DepthOrArraySize - firstSlice);
            break;
        }

        d3d12Texture->dsvDescriptorHandle = dsvDescriptorPool->Alloc();
        device->CreateDepthStencilView(d3d12Texture->GetResource(), &dsvDesc, d3d12Texture->dsvDescriptorHandle);
    } else if (type == SubresourceType::UAV) {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Format = d3d12Texture->textureDesc.Format;

        switch (d3d12Texture->textureType) {
        case TextureType::Texture1D:
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            uavDesc.Texture1D.MipSlice = firstMip;
            break;
        case TextureType::Texture1DArray:
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            uavDesc.Texture1DArray.MipSlice = firstMip;
            uavDesc.Texture1DArray.FirstArraySlice = firstSlice;
            uavDesc.Texture1DArray.ArraySize = Min(sliceCount, d3d12Texture->textureDesc.DepthOrArraySize - firstSlice);
            break;
        case TextureType::Texture2D:
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = firstMip;
            break;
        case TextureType::Texture2DArray:
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uavDesc.Texture2DArray.MipSlice = firstMip;
            uavDesc.Texture2DArray.FirstArraySlice = firstSlice;
            uavDesc.Texture2DArray.ArraySize = Min(sliceCount, d3d12Texture->textureDesc.DepthOrArraySize - firstSlice);
            break;
        case TextureType::Texture3D:
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture3D.MipSlice = firstMip;
            uavDesc.Texture3D.FirstWSlice = 0;
            uavDesc.Texture3D.WSize = -1;
            break;
        }

        d3d12Texture->uavDescriptorHandle = uavDescriptorPool->Alloc();
        device->CreateUnorderedAccessView(d3d12Texture->GetResource(), nullptr, &uavDesc, d3d12Texture->uavDescriptorHandle);
    }
}

RHIRenderer::Texture *D3D12Renderer::CreateTexture(TextureType textureType, int flags, const Image *srcImage, Image::Format::Enum dstFormat, bool useMipmaps) {
    Image::Format::Enum srcFormat = srcImage->GetFormat();

    bool srcCompressed = Image::IsCompressed(srcFormat);
    bool dstCompressed = Image::IsCompressed(dstFormat);

    bool srcFormatSupported = D3D12Renderer::IsSupportedImageFormat(srcFormat);
    bool dstFormatSupported = D3D12Renderer::IsSupportedImageFormat(dstFormat);

    if (!dstFormatSupported) {
        BE_WARNLOG("D3D12Renderer::CreateTexture: Unsupported internal image format %s\n", Image::FormatName(dstFormat));
        return nullptr;
    }

    Image uncompressedImage;

    if (useMipmaps && srcImage->NumMipmaps() == 1) {
        if (srcImage->IsPacked() || srcImage->IsCompressed()) {
            // 밉맵을 생성해야 한다면, 지원되는 가장 비슷한 무압축 포맷으로 컨버팅한다.
            Image::Format::Enum supportedUncompressedFormat = D3D12Renderer::ToUncompressedImageFormat(srcFormat);

            srcImage->ConvertFormat(supportedUncompressedFormat, uncompressedImage);
            srcImage = &uncompressedImage;

            srcFormat = supportedUncompressedFormat;
            srcFormatSupported = D3D12Renderer::IsSupportedImageFormat(srcFormat);
            srcCompressed = false;
        }
    }

    Image mipmapedImage;

    // 밉맵을 직접 생성한다.
    if (useMipmaps && srcImage->NumMipmaps() == 1) {
        int w = srcImage->GetWidth();
        int h = srcImage->GetHeight();
        int d = srcImage->GetDepth();
        int maxGenLevels = Image::MaxMipMapLevels(w, h, d);

        mipmapedImage.Create(w, h, d, srcImage->NumSlices(), maxGenLevels, srcImage->GetFormat(), srcImage->GetGammaSpace(), nullptr, srcImage->GetFlags());
        mipmapedImage.CopyFrom(*srcImage, 0, 1);
        mipmapedImage.GenerateMipmaps();
        srcImage = &mipmapedImage;
    }

    Image dstImage;

    // dstFormat 으로 컨버팅
    if (srcFormat != dstFormat) {
        srcImage->ConvertFormat(dstFormat, dstImage);
        srcImage = &dstImage;
    }

    return CreateTexture(textureType, flags, srcImage);
}

RHIRenderer::Texture *D3D12Renderer::CreateTextureFromFile(TextureType textureType, int flags, const char *filename, bool useCompression, bool useNormalMap) {
    Image *image = Image::NewImageFromFile(filename);
    if (!image) {
        return nullptr;
    }

    Image::Format::Enum dstFormat;
    D3D12Texture::AdjustTextureFormat(useCompression, useNormalMap, image->GetFormat(), &dstFormat);

    Texture *texture = CreateTexture(textureType, flags, image, dstFormat, true);
    delete image;

    return texture;
}

void D3D12Renderer::DestroyTexture(Texture *texture, bool immediate) {
    if (immediate) {
        delete texture;
    } else {
        MarkForDelete(texture);
    }
}

void D3D12Renderer::GetTextureImage2D(Texture *texture, int level, Image::Format::Enum dstFormat, void *outPixels) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);
    assert(d3d12Texture);

    Image::Format::Enum textureImageFormat;
    bool isSRGB;
    if (!D3D12Renderer::DXGIFormatToImageFormat(d3d12Texture->textureDesc.Format, &textureImageFormat, &isSRGB)) {
        BE_WARNLOG("D3D12Texture::GetTexture2D: Unsupported DXGI format %i\n", d3d12Texture->textureDesc.Format);
        return;
    }

    // 텍스쳐 리소스의 특정 밉레벨 (서브 리소스) 의 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprint;
    UINT64 mipLevelSize;
    device->GetCopyableFootprints(&d3d12Texture->textureDesc, level, 1, 0, &mipLevelFootprint, nullptr, nullptr, &mipLevelSize);

    D3D12Buffer *readbackBuffer = static_cast<D3D12Buffer *>(CreateBuffer(BufferUsage::Readback, ResourceFlag::None, mipLevelSize, textureImageFormat, 0, nullptr));
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
    resourceCommandList->CloseAndExecute(CommandQueueType::Graphics);

    // GPU 에서 복사가 끝날 때까지 기다린다.
    Finish();

    // 복사된 리드백 버퍼를 메모리로 읽어오기 위해 Map 을 한다.
    void *mappedPtr = nullptr;
    D3D12_RANGE readRange = { 0, mipLevelSize };
    readbackBuffer->GetResource()->Map(0, &readRange, &mappedPtr);

    const byte *srcPtr = (byte *)mappedPtr;
    byte *dstPtr = nullptr;

    Image textureImage;
    if (textureImageFormat != dstFormat) {
        // 컨버팅이 필요하다면 리드백 버퍼에서 textureImage 에 카피한다.
        textureImage.InitFromMemory(d3d12Texture->textureDesc.Width, d3d12Texture->textureDesc.Height, 1, 1, 1, textureImageFormat, isSRGB ? Image::GammaSpace::sRGB : Image::GammaSpace::Linear, nullptr, 0);
        dstPtr = textureImage.GetPixels();
    } else {
        // 컨버팅할 필요가 없다면 리드백 버퍼에서 그대로 outPixels 로 카피한다.
        dstPtr = (byte *)outPixels;
    }

    int srcPitch = mipLevelFootprint.Footprint.RowPitch;
    int dstPitch = Image::MemRequired(d3d12Texture->textureDesc.Width, 1, 1, 1, textureImageFormat);

    for (UINT y = 0; y < d3d12Texture->textureDesc.Height; ++y) {
        simdProcessor->Memcpy(dstPtr, srcPtr, srcPitch);
        srcPtr += srcPitch;
        dstPtr += dstPitch;
    }

    D3D12_RANGE writtenRange = { 0, 0 };
    readbackBuffer->GetResource()->Unmap(0, &writtenRange);

    // 리드백 버퍼 삭제
    DestroyBuffer(readbackBuffer, true);

    // 컨버팅이 필요없다면 바로 리턴한다.
    if (textureImageFormat == dstFormat) {
        return;
    }

    // 필요하다면 컨버팅한다.
    Image dstImage;
    if (textureImage.ConvertFormat(dstFormat, dstImage)) {
        simdProcessor->Memcpy(outPixels, dstImage.GetPixels(), dstImage.SizeInBytes());
        return;
    }
}

bool D3D12Renderer::SetTextureSubImage2D(Texture *texture, int level, int x, int y, int width, int height, Image::Format::Enum srcFormat, const void *pixels) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);
    assert(d3d12Texture);

    // 텍스쳐 리소스의 특정 mipLevel 에 대한 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprint;
    device->GetCopyableFootprints(&d3d12Texture->textureDesc, level, 1, 0, &mipLevelFootprint, nullptr, nullptr, nullptr);

    int srcPitch = Image::MemRequired(width, 1, 1, 1, srcFormat);
    int dstPitch = mipLevelFootprint.Footprint.RowPitch;
    int uploadBufferSize = Image::MemRequired(dstPitch, height, 1, 1, srcFormat);

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
        simdProcessor->MemcpyStream(dstPtr, srcPtr, srcPitch);
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

    D3D12_BOX box = { 0, 0, 0, (UINT)width, (UINT)height, 1 };
    resourceCommandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, x, y, 0, &srcLocation, &box);

    resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    resourceCommandList->CloseAndExecute(CommandQueueType::Graphics);

    MarkForRelease(uploadBuffer);

    return true;
}

bool D3D12Renderer::SetTextureSubImage3D(Texture *texture, int level, int x, int y, int z, int width, int height, int depth, Image::Format::Enum srcFormat, const void *pixels) {
    D3D12Texture *d3d12Texture = static_cast<D3D12Texture *>(texture);
    assert(d3d12Texture);

    // 텍스쳐 리소스의 특정 mipLevel 에 대한 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprint;
    device->GetCopyableFootprints(&d3d12Texture->textureDesc, level, 1, 0, &mipLevelFootprint, nullptr, nullptr, nullptr);

    int srcPitch = Image::MemRequired(width, 1, 1, 1, srcFormat);
    int dstPitch = mipLevelFootprint.Footprint.RowPitch;
    int uploadBufferSize = Image::MemRequired(dstPitch, height, depth, 1, srcFormat);

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
            simdProcessor->MemcpyStream(dstPtr, srcPtr, srcPitch);
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

    D3D12_BOX box = { 0, 0, 0, (UINT)width, (UINT)height, (UINT)depth };
    resourceCommandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, x, y, z, &srcLocation, &box);

    resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    resourceCommandList->CloseAndExecute(CommandQueueType::Graphics);

    MarkForRelease(uploadBuffer);

    return true;
}

void D3D12Renderer::SetTexture(CommandList *commandList, int slot, bool shaderWritable, const Texture *texture) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    int threadIndex = d3d12CommandList->GetThreadIndex();

    const D3D12Texture *d3d12Texture = static_cast<const D3D12Texture *>(texture);
    D3D12FrameData::DataPerThread &threadData = currentFrameData->threadData[threadIndex];

    // 슬롯 (레지스터) 에 대한 루트 파라미터 인덱스를 얻고, 디스크립터 테이블일 경우 테이블 인덱스도 얻어온다.
    const D3D12PipelineState::Binder &binder = d3d12CommandList->currentPSO->binder;
    int rootParameterIndex = -1;

    if (shaderWritable) {
        rootParameterIndex = binder.rootParameterBinder.uav[slot];
        int descriptorIndex = binder.descriptorTableBinder.uav[slot];

        threadData.psoDescriptorHandles[rootParameterIndex][descriptorIndex] = d3d12Texture->uavDescriptorHandle;
        if (threadData.psoDescriptorHandles[rootParameterIndex][descriptorIndex].ptr == 0) {
            BE_ERRLOG("Texture has no valid UAV descriptor handle\n");
            return;
        }
        threadData.uavResources[slot] = d3d12Texture;
    } else {
        rootParameterIndex = binder.rootParameterBinder.srv[slot];
        int descriptorIndex = binder.descriptorTableBinder.srv[slot];

        threadData.psoDescriptorHandles[rootParameterIndex][descriptorIndex] = d3d12Texture->srvDescriptorHandle;
        if (threadData.psoDescriptorHandles[rootParameterIndex][descriptorIndex].ptr == 0) {
            BE_ERRLOG("Texture has no valid SRV descriptor handle\n");
            return;
        }
        threadData.srvResources[slot] = d3d12Texture;
    }

    if (d3d12CommandList->GetCommandListType() == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        d3d12CommandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        d3d12CommandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}
