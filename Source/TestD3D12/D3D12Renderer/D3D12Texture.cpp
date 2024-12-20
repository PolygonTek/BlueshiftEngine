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
    if (descriptorHandle.ptr != 0) {
        renderer->srvDescriptorPool->Free(descriptorHandle);
        descriptorHandle.ptr = 0;
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

RHIRenderer::Texture *D3D12Renderer::CreateTexture(TextureType textureType, const Image *srcImage) {
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
    case TextureType::Texture2D:
    case TextureType::Texture2DArray:
    case TextureType::TextureCube:
    case TextureType::TextureCubeArray:
        textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        break;
    case TextureType::Texture3D:
        textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        break;
    case TextureType::TextureBuffer:
        textureDimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        break;
    }

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
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

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

                    resourceCommandList->graphicsCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
                }
            }
        }

        resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        resourceCommandList->CloseAndExecute(CommandQueueType::Graphics);

        if (uploadBuffer) {
            MarkForRelease(uploadBuffer);
        }
    }

    // 디스크립터에 SRV 정보 기록하기
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch (textureType) {
    case TextureType::Texture2D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
        break;
    case TextureType::Texture2DArray:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MipLevels = textureDesc.MipLevels;
        srvDesc.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
        break;
    case TextureType::Texture3D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MipLevels = textureDesc.MipLevels;
        break;
    case TextureType::TextureCube:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = textureDesc.MipLevels;
        break;
    case TextureType::TextureCubeArray:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDesc.TextureCubeArray.MipLevels = textureDesc.MipLevels;
        srvDesc.TextureCubeArray.NumCubes = textureDesc.DepthOrArraySize;
        break;
    case TextureType::TextureBuffer:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = textureDesc.Width;
        srvDesc.Buffer.StructureByteStride = sizeof(float);
        break;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = srvDescriptorPool->Alloc();
    device->CreateShaderResourceView(textureResource, &srvDesc, descriptorHandle);

    D3D12Texture *texture = new D3D12Texture;
    texture->textureType = textureType;
#ifdef USE_D3D12_MEMALLOC
    texture->textureAllocation = allocation;
#else
    texture->textureResource = textureResource;
#endif
    texture->textureDesc = textureResource->GetDesc();
    texture->descriptorHandle = descriptorHandle;

    return texture;
}

RHIRenderer::Texture *D3D12Renderer::CreateTexture(TextureType textureType, const Image *srcImage, Image::Format::Enum dstFormat, bool useMipmaps) {
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

    return CreateTexture(textureType, srcImage);
}

RHIRenderer::Texture *D3D12Renderer::CreateTextureFromFile(TextureType textureType, const char *filename, bool useCompression, bool useNormalMap) {
    Image *image = Image::NewImageFromFile(filename);
    if (!image) {
        return nullptr;
    }

    Image::Format::Enum dstFormat;
    D3D12Texture::AdjustTextureFormat(useCompression, useNormalMap, image->GetFormat(), &dstFormat);

    Texture *texture = CreateTexture(textureType, image, dstFormat, true);
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

    D3D12Buffer *readbackBuffer = static_cast<D3D12Buffer *>(CreateBuffer(BufferUsage::Readback, BufferFlag::None, mipLevelSize));
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
    resourceCommandList->graphicsCommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
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
    resourceCommandList->graphicsCommandList->CopyTextureRegion(&dstLocation, x, y, 0, &srcLocation, &box);

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
    resourceCommandList->graphicsCommandList->CopyTextureRegion(&dstLocation, x, y, z, &srcLocation, &box);

    resourceCommandList->ResourceBarrier(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    resourceCommandList->CloseAndExecute(CommandQueueType::Graphics);

    MarkForRelease(uploadBuffer);

    return true;
}

void D3D12Renderer::SetTexture(CommandList *commandList, int slot, const Texture *texture) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    int threadIndex = d3d12CommandList->GetThreadIndex();
    D3D12FrameData::DataPerThread &threadData = currentFrameData->threadData[threadIndex];

    assert(slot < COUNT_OF(threadData.psoDescriptorHandles));

    const D3D12Texture *d3d12Texture = static_cast<const D3D12Texture *>(texture);
    threadData.psoDescriptorHandles[slot] = d3d12Texture->descriptorHandle;
    threadData.srvResources[slot] = d3d12Texture;
}
