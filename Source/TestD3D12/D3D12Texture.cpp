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
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include "D3D12Renderer.h"
#include "D3D12DescriptorPool.h"

void D3D12Texture::Release() {
    renderer.WaitAllFrameFences();

    if (descriptorHandle.ptr != 0) {
        renderer.srvDescriptorPool->Free(descriptorHandle);
        descriptorHandle.ptr = 0;
    }

#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(textureAllocation);
#else
    SAFE_RELEASE(textureResource);
#endif
}

void D3D12Texture::GetTextureImage2D(UINT level, Image::Format::Enum dstFormat, void *outPixels) {
    Image::Format::Enum textureImageFormat;
    bool isSRGB;
    if (!DXGIFormatToImageFormat(textureDesc.Format, &textureImageFormat, &isSRGB)) {
        BE_WARNLOG("D3D12Texture::GetTexture2D: Unsupported DXGI format %i\n", textureDesc.Format);
        return;
    }

    // 텍스쳐 리소스의 특정 밉레벨 (서브 리소스) 의 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprint;
    UINT64 mipLevelSize;
    renderer.device->GetCopyableFootprints(&textureDesc, level, 1, 0, &mipLevelFootprint, nullptr, nullptr, &mipLevelSize);

    D3D12Buffer *readbackBuffer = D3D12Buffer::CreateBuffer(D3D12Buffer::Usage::Readback, mipLevelSize);
    if (!readbackBuffer) {
        BE_WARNLOG("D3D12Texture::GetTextureImage2D: Failed to create readback buffer\n");
        return;
    }

#ifdef USE_D3D12_MEMALLOC
    ID3D12Resource *textureResource = textureAllocation->GetResource();
#endif

    // 텍스쳐에서 리드백 버퍼로 복사한다.
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = textureResource;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = readbackBuffer->GetResource();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLocation.PlacedFootprint = mipLevelFootprint;

    renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE));
    renderer.commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    renderer.commandList->Close();

    // CommandQueue 실행
    ID3D12CommandList *execCommandLists[] = { renderer.commandList };
    renderer.commandQueue->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);

    // GPU 에서 복사가 끝날 때까지 기다린다.
    renderer.Finish();

    // 복사된 리드백 버퍼를 메모리로 읽어오기 위해 Map 을 한다.
    void *mappedPtr = nullptr;
    D3D12_RANGE readRange = { 0, mipLevelSize };
    readbackBuffer->GetResource()->Map(0, &readRange, &mappedPtr);

    const byte *srcPtr = (byte *)mappedPtr;
    byte *dstPtr = nullptr;

    Image textureImage;
    if (textureImageFormat != dstFormat) {
        // 컨버팅이 필요하다면 리드백 버퍼에서 textureImage 에 카피한다.
        textureImage.InitFromMemory(textureDesc.Width, textureDesc.Height, 1, 1, 1, textureImageFormat, isSRGB ? Image::GammaSpace::sRGB : Image::GammaSpace::Linear, nullptr, 0);
        dstPtr = textureImage.GetPixels();
    } else {
        // 컨버팅할 필요가 없다면 리드백 버퍼에서 그대로 outPixels 로 카피한다.
        dstPtr = (byte *)outPixels;
    }

    int srcPitch = mipLevelFootprint.Footprint.RowPitch;
    int dstPitch = Image::MemRequired(textureDesc.Width, 1, 1, 1, textureImageFormat);

    for (UINT y = 0; y < textureDesc.Height; ++y) {
        simdProcessor->Memcpy(dstPtr, srcPtr, srcPitch);
        srcPtr += srcPitch;
        dstPtr += dstPitch;
    }

    D3D12_RANGE writtenRange = { 0, 0 };
    readbackBuffer->GetResource()->Unmap(0, &writtenRange);

    // 리드백 버퍼 삭제
    SAFE_DELETE(readbackBuffer);

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

bool D3D12Texture::SetTextureSubImage2D(UINT level, UINT x, UINT y, UINT width, UINT height, Image::Format::Enum srcFormat, const void *pixels) {
    // 텍스쳐 리소스의 특정 mipLevel 에 대한 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprint;
    renderer.device->GetCopyableFootprints(&textureDesc, level, 1, 0, &mipLevelFootprint, nullptr, nullptr, nullptr);

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
    if (FAILED(renderer.device->CreateCommittedResource(
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

#ifdef USE_D3D12_MEMALLOC
    ID3D12Resource *textureResource = textureAllocation->GetResource();
#endif

    // 업로드 버퍼에서 텍스쳐로 데이터 카피
    renderer.commandAllocator->Reset();
    renderer.commandList->Reset(renderer.commandAllocator, nullptr);
    renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

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

    D3D12_BOX box = { 0, 0, 0, width, height, 1 };
    renderer.commandList->CopyTextureRegion(&dstLocation, x, y, 0, &srcLocation, &box);

    renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
    renderer.commandList->Close();

    // CommandQueue 실행
    ID3D12CommandList *execCommandLists[] = { renderer.commandList };
    renderer.commandQueue->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);

    renderer.MarkForRelease(uploadBuffer);

    return true;
}

bool D3D12Texture::SetTextureSubImage3D(UINT level, UINT x, UINT y, UINT z, UINT width, UINT height, UINT depth, Image::Format::Enum srcFormat, const void *pixels) {
    // 텍스쳐 리소스의 특정 mipLevel 에 대한 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprint;
    renderer.device->GetCopyableFootprints(&textureDesc, level, 1, 0, &mipLevelFootprint, nullptr, nullptr, nullptr);

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

    // 업로드 버퍼 생성 (pitch 를 타겟 텍스쳐와 동일하게 잡는다)
    ID3D12Resource *uploadBuffer = nullptr;
    if (FAILED(renderer.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
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

#ifdef USE_D3D12_MEMALLOC
    ID3D12Resource *textureResource = textureAllocation->GetResource();
#endif

    // 업로드 버퍼에서 텍스쳐로 데이터 카피
    renderer.commandAllocator->Reset();
    renderer.commandList->Reset(renderer.commandAllocator, nullptr);
    renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

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

    D3D12_BOX box = { 0, 0, 0, width, height, depth };
    renderer.commandList->CopyTextureRegion(&dstLocation, x, y, z, &srcLocation, &box);

    renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
    renderer.commandList->Close();

    // CommandQueue 실행
    ID3D12CommandList *execCommandLists[] = { renderer.commandList };
    renderer.commandQueue->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);

    renderer.MarkForRelease(uploadBuffer);

    return true;
}

D3D12Texture *D3D12Texture::CreateTexture(D3D12Texture::Type::Enum textureType, const char *filename, bool useCompression, bool useNormalMap) {
    Image *image = Image::NewImageFromFile(filename);
    if (!image) {
        return nullptr;
    }

    Image::Format::Enum dstFormat;
    D3D12Texture::AdjustTextureFormat(useCompression, useNormalMap, image->GetFormat(), &dstFormat);

    D3D12Texture* texture = D3D12Texture::CreateTexture(textureType, image, dstFormat, true);
    delete image;

    return texture;
}

D3D12Texture *D3D12Texture::CreateTexture(D3D12Texture::Type::Enum textureType, const Image *srcImage, Image::Format::Enum dstFormat, bool useMipmaps) {
    Image::Format::Enum srcFormat = srcImage->GetFormat();

    bool srcCompressed = Image::IsCompressed(srcFormat);
    bool dstCompressed = Image::IsCompressed(dstFormat);

    bool srcFormatSupported = IsSupportedImageFormat(srcFormat);
    bool dstFormatSupported = IsSupportedImageFormat(dstFormat);

    if (!dstFormatSupported) {
        BE_WARNLOG("D3D12Texture::CreateTexture: Unsupported internal image format %s\n", Image::FormatName(dstFormat));
        return nullptr;
    }

    Image uncompressedImage;

    if (useMipmaps && srcImage->NumMipmaps() == 1) {
        if (srcImage->IsPacked() || srcImage->IsCompressed()) {
            // 밉맵을 생성해야 한다면, 지원되는 가장 비슷한 무압축 포맷으로 컨버팅한다.
            Image::Format::Enum supportedUncompressedFormat = ToUncompressedImageFormat(srcFormat);

            srcImage->ConvertFormat(supportedUncompressedFormat, uncompressedImage);
            srcImage = &uncompressedImage;

            srcFormat = supportedUncompressedFormat;
            srcFormatSupported = IsSupportedImageFormat(srcFormat);
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

D3D12Texture* D3D12Texture::CreateTexture(D3D12Texture::Type::Enum textureType, const Image* srcImage) {
    Image::Format::Enum srcFormat = srcImage->GetFormat();
    bool isLinearSpace = srcImage->GetGammaSpace() == Image::GammaSpace::Linear;

    DXGI_FORMAT dxgiFormat;
    bool srcFormatSupported = ImageFormatToDXGIFormat(srcFormat, !isLinearSpace, &dxgiFormat);
    if (!srcFormatSupported) {
        BE_WARNLOG("D3D12Texture::CreateTexture: Unsupported image format %s\n", Image::FormatName(srcFormat));
        return nullptr;
    }

    D3D12_RESOURCE_DIMENSION textureDimension;
    switch (textureType) {
    case D3D12Texture::Type::Texture2D:
    case D3D12Texture::Type::Texture2DArray:
    case D3D12Texture::Type::TextureCube:
    case D3D12Texture::Type::TextureCubeArray:
        textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        break;
    case D3D12Texture::Type::Texture3D:
        textureDimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        break;
    case D3D12Texture::Type::TextureBuffer:
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
    if (FAILED(renderer.allocator->CreateResource(
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
    if (FAILED(renderer.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&textureResource)))) {
        return nullptr;
    }
#endif

    // 텍스쳐 리소스의 서브 리소스 별 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprints[16];
    UINT64 size;
    renderer.device->GetCopyableFootprints(&textureDesc, 0, textureDesc.MipLevels, 0, mipLevelFootprints, nullptr, nullptr, &size);

    // 업로드 버퍼 생성
    ID3D12Resource *uploadBuffer = nullptr;
    if (FAILED(renderer.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(size),
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
    renderer.commandAllocator->Reset();
    renderer.commandList->Reset(renderer.commandAllocator, nullptr);

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

                renderer.commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
            }
        }
    }

    renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
    renderer.commandList->Close();

    // CommandQueue 실행
    ID3D12CommandList *execCommandLists[] = { renderer.commandList };
    renderer.commandQueue->ExecuteCommandLists(COUNT_OF(execCommandLists), execCommandLists);

    if (uploadBuffer) {
        renderer.MarkForRelease(uploadBuffer);
    }

    // 디스크립터에 SRV 정보 기록하기
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch (textureType) {
    case D3D12Texture::Type::Texture2D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
        break;
    case D3D12Texture::Type::Texture2DArray:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MipLevels = textureDesc.MipLevels;
        srvDesc.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
        break;
    case D3D12Texture::Type::Texture3D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MipLevels = textureDesc.MipLevels;
        break;
    case D3D12Texture::Type::TextureCube:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = textureDesc.MipLevels;
        break;
    case D3D12Texture::Type::TextureCubeArray:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDesc.TextureCubeArray.MipLevels = textureDesc.MipLevels;
        srvDesc.TextureCubeArray.NumCubes = textureDesc.DepthOrArraySize;
        break;
    case D3D12Texture::Type::TextureBuffer:
        // FIXME
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        break;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = renderer.srvDescriptorPool->Alloc();
    renderer.device->CreateShaderResourceView(textureResource, &srvDesc, descriptorHandle);

    D3D12Texture *texture = new D3D12Texture;
#ifdef USE_D3D12_MEMALLOC
    texture->textureAllocation = allocation;
#else
    texture->textureResource = textureResource;
#endif
    texture->textureDesc = textureResource->GetDesc();
    texture->descriptorHandle = descriptorHandle;

    return texture;
}

bool D3D12Texture::ImageFormatToDXGIFormat(Image::Format::Enum imageFormat, bool isSRGB, DXGI_FORMAT *dxgiFormat) {
    switch (imageFormat) {
    case Image::Format::L_8:
    case Image::Format::R_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8_UNORM;
        return true;
    case Image::Format::A_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_A8_UNORM;
        return true;
    case Image::Format::RG_8_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8G8_UNORM;
        return true;
    case Image::Format::RGBA_8_8_8_8:
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        return true;
    case Image::Format::BGRA_8_8_8_8:
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
        return true;
    case Image::Format::BGRX_8_8_8_8:
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB : DXGI_FORMAT_B8G8R8X8_UNORM;
        return true;
    case Image::Format::R_8_SNORM:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8_SNORM;
        return true;
    case Image::Format::RG_8_8_SNORM:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8G8_SNORM;
        return true;
    case Image::Format::RGBA_8_8_8_8_SNORM:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
        return true;
    case Image::Format::BGR_5_6_5:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_B5G6R5_UNORM;
        return true;
    case Image::Format::BGRA_4_4_4_4:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_B4G4R4A4_UNORM;
        return true;
    case Image::Format::ABGR_4_4_4_4:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_A4B4G4R4_UNORM;
        return true;
    case Image::Format::BGRA_5_5_5_1:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_B5G5R5A1_UNORM;
        return true;
    case Image::Format::RGBA_10_10_10_2:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
        return true;
    case Image::Format::L_16F:
    case Image::Format::R_16F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R16_FLOAT;
        return true;
    case Image::Format::RG_16F_16F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R16G16_FLOAT;
        return true;
    case Image::Format::RGBA_16F_16F_16F_16F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        return true;
    case Image::Format::R_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32_FLOAT;
        return true;
    case Image::Format::RG_32F_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
        return true;
    case Image::Format::RGB_32F_32F_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        return true;
    case Image::Format::RGBA_32F_32F_32F_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        return true;
    case Image::Format::RGBE_9_9_9_5:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        return true;
    case Image::Format::RGB_11F_11F_10F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R11G11B10_FLOAT;
        return true;
    case Image::Format::DXT1: // BC1
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
        return true;
    case Image::Format::DXT3: // BC2
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
        return true;
    case Image::Format::DXT5: // BC3
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
        return true;
    case Image::Format::DXN1: // BC4
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_BC4_UNORM;
        return true;
    case Image::Format::DXN2: // BC5
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_BC5_UNORM;
        return true;
    case Image::Format::Depth_16:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D16_UNORM;
        return true;
    case Image::Format::Depth_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D32_FLOAT;
        return true;
    case Image::Format::DepthStencil_24_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        return true;
    }
    return false;
}

bool D3D12Texture::DXGIFormatToImageFormat(DXGI_FORMAT dxgiFormat, Image::Format::Enum *imageFormat, bool *isSRGB) {
    if (isSRGB) {
        *isSRGB = false;
        switch (dxgiFormat) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            *isSRGB = true;
            break;
        }
    }

    switch (dxgiFormat) {
    case DXGI_FORMAT_R8_UNORM:
        if (imageFormat) *imageFormat = Image::Format::R_8;
        return true;
    case DXGI_FORMAT_A8_UNORM:
        if (imageFormat) *imageFormat = Image::Format::A_8;
        return true;
    case DXGI_FORMAT_R8G8_UNORM:
        if (imageFormat) *imageFormat = Image::Format::RG_8_8;
        return true;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        if (imageFormat) *imageFormat = Image::Format::RGBA_8_8_8_8;
        return true;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        if (imageFormat) *imageFormat = Image::Format::BGRA_8_8_8_8;
        return true;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        if (imageFormat) *imageFormat = Image::Format::BGRX_8_8_8_8;
        return true;
    case DXGI_FORMAT_R8_SNORM:
        if (imageFormat) *imageFormat = Image::Format::R_8_SNORM;
        return true;
    case DXGI_FORMAT_R8G8_SNORM:
        if (imageFormat) *imageFormat = Image::Format::RG_8_8_SNORM;
        return true;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        if (imageFormat) *imageFormat = Image::Format::RGBA_8_8_8_8_SNORM;
        return true;
    case DXGI_FORMAT_B5G6R5_UNORM:
        if (imageFormat) *imageFormat = Image::Format::BGR_5_6_5;
        return true;
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        if (imageFormat) *imageFormat = Image::Format::BGRA_4_4_4_4;
        return true;
    case DXGI_FORMAT_A4B4G4R4_UNORM:
        if (imageFormat) *imageFormat = Image::Format::ABGR_4_4_4_4;
        return true;
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        if (imageFormat) *imageFormat = Image::Format::BGRA_5_5_5_1;
        return true;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        if (imageFormat) *imageFormat = Image::Format::RGBA_10_10_10_2;
        return true;
    case DXGI_FORMAT_R16_FLOAT:
        if (imageFormat) *imageFormat = Image::Format::R_16F;
        return true;
    case DXGI_FORMAT_R16G16_FLOAT:
        if (imageFormat) *imageFormat = Image::Format::RG_16F_16F;
        return true;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        if (imageFormat) *imageFormat = Image::Format::RGBA_16F_16F_16F_16F;
        return true;
    case DXGI_FORMAT_R32_FLOAT:
        if (imageFormat) *imageFormat = Image::Format::R_32F;
        return true;
    case DXGI_FORMAT_R32G32_FLOAT:
        if (imageFormat) *imageFormat = Image::Format::RG_32F_32F;
        return true;
    case DXGI_FORMAT_R32G32B32_FLOAT:
        if (imageFormat) *imageFormat = Image::Format::RGB_32F_32F_32F;
        return true;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        if (imageFormat) *imageFormat = Image::Format::RGBA_32F_32F_32F_32F;
        return true;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        if (imageFormat) *imageFormat = Image::Format::RGBE_9_9_9_5;
        return true;
    case DXGI_FORMAT_R11G11B10_FLOAT:
        if (imageFormat) *imageFormat = Image::Format::RGB_11F_11F_10F;
        return true;
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        if (imageFormat) *imageFormat = Image::Format::DXT1;
        return true;
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        if (imageFormat) *imageFormat = Image::Format::DXT3;
        return true;
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        if (imageFormat) *imageFormat = Image::Format::DXT5;
        return true;
    case DXGI_FORMAT_BC4_UNORM:
        if (imageFormat) *imageFormat = Image::Format::DXN1;
        return true;
    case DXGI_FORMAT_BC5_UNORM:
        if (imageFormat) *imageFormat = Image::Format::DXN2;
        return true;
    case DXGI_FORMAT_D16_UNORM:
        if (imageFormat) *imageFormat = Image::Format::Depth_16;
        return true;
    case DXGI_FORMAT_D32_FLOAT:
        if (imageFormat) *imageFormat = Image::Format::Depth_32F;
        return true;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        if (imageFormat) *imageFormat = Image::Format::DepthStencil_24_8;
        return true;
    }
    return false;
}

Image::Format::Enum D3D12Texture::ToUncompressedImageFormat(Image::Format::Enum inFormat) {
    Image::Format::Enum outFormat;

    switch (inFormat) {
    case Image::Format::RGB_5_6_5:
    case Image::Format::RGB_8_8_8:
    case Image::Format::BGR_5_6_5:
    case Image::Format::BGR_8_8_8:
    case Image::Format::RGBX_4_4_4_4:
    case Image::Format::RGBX_5_5_5_1:
    case Image::Format::RGBX_8_8_8_8:
    case Image::Format::BGRX_4_4_4_4:
    case Image::Format::BGRX_5_5_5_1:
        outFormat = Image::Format::BGRX_8_8_8_8;
        break;
    case Image::Format::LA_8_8:
    case Image::Format::RGBA_4_4_4_4:
    case Image::Format::RGBA_5_5_5_1:
    case Image::Format::BGRA_4_4_4_4:
    case Image::Format::BGRA_5_5_5_1:
    case Image::Format::ABGR_4_4_4_4:
    case Image::Format::ABGR_1_5_5_5:
    case Image::Format::ABGR_8_8_8_8:
    case Image::Format::ARGB_4_4_4_4:
    case Image::Format::ARGB_1_5_5_5:
    case Image::Format::ARGB_8_8_8_8:
        outFormat = Image::Format::BGRA_8_8_8_8;
        break;
    case Image::Format::RGB_8_8_8_SNORM:
        outFormat = Image::Format::RGBA_8_8_8_8_SNORM;
        break;
    case Image::Format::RGB_16F_16F_16F:
        outFormat = Image::Format::RGBA_16F_16F_16F_16F;
        break;
    case Image::Format::RGB_32F_32F_32F:
        outFormat = Image::Format::RGBA_32F_32F_32F_32F;
        break;
    case Image::Format::DXN1:
    case Image::Format::DXN2:
    case Image::Format::RGB_PVRTC_2BPPV1:
    case Image::Format::RGB_PVRTC_4BPPV1:
    case Image::Format::RGB_8_ETC1:
    case Image::Format::RGB_8_ETC2:
    case Image::Format::RGB_ATC:
        outFormat = Image::Format::BGRX_8_8_8_8;
        break;
    case Image::Format::DXT1:
    case Image::Format::DXT3:
    case Image::Format::DXT5:
    case Image::Format::RGBA_PVRTC_2BPPV1:
    case Image::Format::RGBA_PVRTC_4BPPV1:
    case Image::Format::RGBA_PVRTC_2BPPV2:
    case Image::Format::RGBA_PVRTC_4BPPV2:
    case Image::Format::RGBA_8_1_ETC2:
    case Image::Format::RGBA_8_8_ETC2:
    case Image::Format::RGBA_EA_ATC:
    case Image::Format::RGBA_IA_ATC:
        outFormat = Image::Format::RGBA_8_8_8_8;
        break;
    case Image::Format::R_11_EAC:
    case Image::Format::SignedR_11_EAC:
        outFormat = Image::Format::R_16F;
        break;
    case Image::Format::RG_11_11_EAC:
    case Image::Format::SignedRG_11_11_EAC:
        outFormat = Image::Format::RG_16F_16F;
        break;
    default:
        assert(0);
        outFormat = inFormat;
        break;
    }
    return outFormat;
}

Image::Format::Enum D3D12Texture::ToCompressedImageFormat(Image::Format::Enum inFormat, bool useNormalMap) {
    if (Image::IsCompressed(inFormat)) {
        assert(0);
        return inFormat;
    }

    int redBits, greenBits, blueBits, alphaBits;
    Image::GetBits(inFormat, &redBits, &greenBits, &blueBits, &alphaBits);

    Image::Format::Enum outFormat = inFormat;

    if (redBits > 0 && greenBits > 0 && blueBits > 0) {
        if (Image::IsFloatFormat(inFormat) || Image::IsHalfFormat(inFormat)) {
            if (alphaBits == 0) {
                outFormat = Image::Format::RGBE_9_9_9_5;
            }
        } else if (useNormalMap) {
            outFormat = Image::Format::DXN2;
        } else {
            if (alphaBits <= 1) {
                outFormat = Image::Format::DXT1;
            } else if (alphaBits <= 4) {
                outFormat = Image::Format::DXT3;
            } else {
                outFormat = Image::Format::DXT5;
            }
        }
    }

    return outFormat;
}

void D3D12Texture::AdjustTextureFormat(bool useCompression, bool useNormalMap, Image::Format::Enum inFormat, Image::Format::Enum *outFormat) {
    if (Image::IsDepthFormat(inFormat) || Image::IsDepthStencilFormat(inFormat)) {
        *outFormat = inFormat;
        return;
    }

    if (Image::IsCompressed(inFormat)) {
        if (IsSupportedImageFormat(inFormat)) {
            *outFormat = inFormat;
            return;
        }

        inFormat = ToUncompressedImageFormat(inFormat);
    }

    *outFormat = useCompression ? ToCompressedImageFormat(inFormat, useNormalMap) : ToUncompressedImageFormat(inFormat);
}
