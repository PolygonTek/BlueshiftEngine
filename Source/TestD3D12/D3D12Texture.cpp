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
#include "D3D12Renderer.h"

void D3D12Texture::Release() {
    SAFE_RELEASE(textureResource);
}

D3D12Texture *D3D12Texture::CreateTexture2D(const char *filename, bool useCompression, bool useNormalMap) {
    BE1::Image *image = BE1::Image::NewImageFromFile(filename);
    if (!image) {
        return nullptr;
    }

    BE1::Image::Format::Enum dstFormat;
    D3D12Texture::AdjustTextureFormat(useCompression, useNormalMap, image->GetFormat(), &dstFormat);

    D3D12Texture* texture = D3D12Texture::CreateTexture2D(image, dstFormat, true);
    delete image;

    return texture;
}

D3D12Texture *D3D12Texture::CreateTexture2D(const BE1::Image *srcImage, BE1::Image::Format::Enum dstFormat, bool useMipmaps) {
    BE1::Image::Format::Enum srcFormat = srcImage->GetFormat();

    bool srcCompressed = BE1::Image::IsCompressed(srcFormat);
    bool dstCompressed = BE1::Image::IsCompressed(dstFormat);

    bool srcFormatSupported = IsSupportedImageFormat(srcFormat);
    bool dstFormatSupported = IsSupportedImageFormat(dstFormat);

    if (!dstFormatSupported) {
        BE_WARNLOG("Unsupported internal image format %s\n", BE1::Image::FormatName(dstFormat));
        return nullptr;
    }

    BE1::Image uncompressedImage;

    if (useMipmaps && srcImage->NumMipmaps() == 1) {
        if (srcImage->IsPacked() || srcImage->IsCompressed()) {
            // 밉맵을 생성해야 한다면, 지원되는 가장 비슷한 무압축 포맷으로 컨버팅한다.
            BE1::Image::Format::Enum supportedUncompressedFormat = ToUncompressedImageFormat(srcFormat);

            srcImage->ConvertFormat(supportedUncompressedFormat, uncompressedImage);
            srcImage = &uncompressedImage;

            srcFormat = supportedUncompressedFormat;
            srcFormatSupported = IsSupportedImageFormat(srcFormat);
            srcCompressed = false;
        }
    }

    BE1::Image mipmapedImage;

    // 밉맵을 직접 생성한다.
    if (useMipmaps && srcImage->NumMipmaps() == 1) {
        int w = srcImage->GetWidth();
        int h = srcImage->GetHeight();
        int d = srcImage->GetDepth();
        int maxGenLevels = BE1::Image::MaxMipMapLevels(w, h, d);

        mipmapedImage.Create(w, h, d, srcImage->NumSlices(), maxGenLevels, srcImage->GetFormat(), srcImage->GetGammaSpace(), nullptr, srcImage->GetFlags());
        mipmapedImage.CopyFrom(*srcImage, 0, 1);
        mipmapedImage.GenerateMipmaps();
        srcImage = &mipmapedImage;
    }

    BE1::Image dstImage;

    // dstFormat 으로 컨버팅
    if (srcFormat != dstFormat) {
        srcImage->ConvertFormat(dstFormat, dstImage);
        srcImage = &dstImage;
    }

    return CreateTexture2D(srcImage);
}

D3D12Texture* D3D12Texture::CreateTexture2D(const BE1::Image* srcImage) {
    BE1::Image::Format::Enum srcFormat = srcImage->GetFormat();
    bool isLinearSpace = srcImage->GetGammaSpace() == BE1::Image::GammaSpace::Linear;

    DXGI_FORMAT dxgiFormat;
    bool srcFormatSupported = ImageFormatToDXGIFormat(srcFormat, !isLinearSpace, &dxgiFormat);
    if (!srcFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", BE1::Image::FormatName(srcFormat));
        return nullptr;
    }

    int maxMipLevels = srcImage->NumMipmaps();

    // GPU 에 텍스쳐 리소스 생성
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = static_cast<UINT16>(maxMipLevels);
    textureDesc.Format = dxgiFormat;
    textureDesc.Width = static_cast<UINT>(srcImage->GetWidth());
    textureDesc.Height = static_cast<UINT>(srcImage->GetHeight());
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ID3D12Resource *textureResource = nullptr;
    if (FAILED(renderer.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&textureResource)))) {
        return nullptr;
    }

    // 텍스쳐 리소스에 write 할 수 있는 (Footprint = 차지하는 공간) 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipFootprints[16];
    UINT64 size;

    renderer.device->GetCopyableFootprints(&textureDesc, 0, textureDesc.MipLevels, 0, mipFootprints, nullptr, nullptr, &size);

    // 업로드 버퍼 생성
    ID3D12Resource *pUploadBuffer = nullptr;
    if (FAILED(renderer.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(size),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&pUploadBuffer)))) {
        textureResource->Release();
        return nullptr;
    }

    // 이미지 데이터를 업로드 버퍼에 write
    UINT8 *mappedPtr = nullptr;
    CD3DX12_RANGE writeRange(0, 0);
    pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedPtr));

    byte *dstPtr = mappedPtr;
    int bpp = srcImage->IsCompressed() ? srcImage->BytesPerBlock() : srcImage->BytesPerPixel();

    for (int mipLevel = 0; mipLevel < maxMipLevels; ++mipLevel) {
        int srcWidth = srcImage->GetWidth(mipLevel);
        int srcHeight = srcImage->GetHeight(mipLevel);
        int srcPitch = (srcImage->IsCompressed() ? (srcWidth >> 2) : srcWidth) * bpp;
        int srcRows = srcImage->IsCompressed() ? (srcHeight >> 2) : srcHeight;
        const byte *srcPtr = srcImage->GetPixels(mipLevel);

        while (srcRows--) {
            memcpy(dstPtr, srcPtr, srcPitch);
            srcPtr += srcPitch;
            dstPtr += mipFootprints[mipLevel].Footprint.RowPitch;
        }
    }

    pUploadBuffer->Unmap(0, nullptr);

    // 업로드 버퍼에서 텍스쳐로 데이터 카피
    renderer.commandAllocator->Reset();
    renderer.commandList->Reset(renderer.commandAllocator, nullptr);

    for (int mipLevel = 0; mipLevel < maxMipLevels; ++mipLevel) {
        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.PlacedFootprint = mipFootprints[mipLevel];
        srcLocation.pResource = pUploadBuffer;
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.PlacedFootprint = mipFootprints[mipLevel];
        dstLocation.pResource = textureResource;
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = mipLevel;

        renderer.commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }

    renderer.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
    renderer.commandList->Close();

    // 커맨드 큐 실행
    ID3D12CommandList *ppCommandLists[] = { renderer.commandList };
    renderer.commandQueue->ExecuteCommandLists(COUNT_OF(ppCommandLists), ppCommandLists);

    if (pUploadBuffer) {
        // 업로드 버퍼 사용이 끝날 때 까지 기다린 후 Release 한다.
        renderer.Finish();

        pUploadBuffer->Release();
    }

    D3D12Texture *texture = new D3D12Texture;
    texture->textureResource = textureResource;
    texture->textureDesc = textureResource->GetDesc();

    return texture;
}

bool D3D12Texture::ImageFormatToDXGIFormat(BE1::Image::Format::Enum imageFormat, bool isSRGB, DXGI_FORMAT *dxgiFormat) {
    switch (imageFormat) {
    case BE1::Image::Format::L_8:
    case BE1::Image::Format::R_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8_UNORM;
        return true;
    case BE1::Image::Format::A_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_A8_UNORM;
        return true;
    case BE1::Image::Format::RG_8_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8G8_UNORM;
        return true;
    case BE1::Image::Format::RGBA_8_8_8_8:
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        return true;
    case BE1::Image::Format::BGRA_8_8_8_8:
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
        return true;
    case BE1::Image::Format::BGRX_8_8_8_8:
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB : DXGI_FORMAT_B8G8R8X8_UNORM;
        return true;
    case BE1::Image::Format::R_8_SNORM:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8_SNORM;
        return true;
    case BE1::Image::Format::RG_8_8_SNORM:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8G8_SNORM;
        return true;
    case BE1::Image::Format::RGBA_8_8_8_8_SNORM:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
        return true;
    case BE1::Image::Format::BGR_5_6_5:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_B5G6R5_UNORM;
        return true;
    case BE1::Image::Format::BGRA_4_4_4_4:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_B4G4R4A4_UNORM;
        return true;
    case BE1::Image::Format::ABGR_4_4_4_4:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_A4B4G4R4_UNORM;
        return true;
    case BE1::Image::Format::BGRA_5_5_5_1:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_B5G5R5A1_UNORM;
        return true;
    case BE1::Image::Format::RGBA_10_10_10_2:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
        return true;
    case BE1::Image::Format::L_16F:
    case BE1::Image::Format::R_16F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R16_FLOAT;
        return true;
    case BE1::Image::Format::RG_16F_16F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R16G16_FLOAT;
        return true;
    case BE1::Image::Format::RGBA_16F_16F_16F_16F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        return true;
    case BE1::Image::Format::R_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32_FLOAT;
        return true;
    case BE1::Image::Format::RG_32F_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
        return true;
    case BE1::Image::Format::RGB_32F_32F_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        return true;
    case BE1::Image::Format::RGBA_32F_32F_32F_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        return true;
    case BE1::Image::Format::RGBE_9_9_9_5:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        return true;
    case BE1::Image::Format::RGB_11F_11F_10F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R11G11B10_FLOAT;
        return true;
    case BE1::Image::Format::DXT1: // BC1
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
        return true;
    case BE1::Image::Format::DXT3: // BC2
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
        return true;
    case BE1::Image::Format::DXT5: // BC3
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
        return true;
    case BE1::Image::Format::DXN1: // BC4
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_BC4_UNORM;
        return true;
    case BE1::Image::Format::DXN2: // BC5
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_BC5_UNORM;
        return true;
    case BE1::Image::Format::Depth_16:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D16_UNORM;
        return true;
    case BE1::Image::Format::Depth_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D32_FLOAT;
        return true;
    case BE1::Image::Format::DepthStencil_24_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        return true;
    }
    return false;
}

BE1::Image::Format::Enum D3D12Texture::ToUncompressedImageFormat(BE1::Image::Format::Enum inFormat) {
    BE1::Image::Format::Enum outFormat;

    switch (inFormat) {
    case BE1::Image::Format::RGB_5_6_5:
    case BE1::Image::Format::RGB_8_8_8:
    case BE1::Image::Format::BGR_5_6_5:
    case BE1::Image::Format::BGR_8_8_8:
    case BE1::Image::Format::RGBX_4_4_4_4:
    case BE1::Image::Format::RGBX_5_5_5_1:
    case BE1::Image::Format::RGBX_8_8_8_8:
    case BE1::Image::Format::BGRX_4_4_4_4:
    case BE1::Image::Format::BGRX_5_5_5_1:
        outFormat = BE1::Image::Format::BGRX_8_8_8_8;
        break;
    case BE1::Image::Format::LA_8_8:
    case BE1::Image::Format::RGBA_4_4_4_4:
    case BE1::Image::Format::RGBA_5_5_5_1:
    case BE1::Image::Format::BGRA_4_4_4_4:
    case BE1::Image::Format::BGRA_5_5_5_1:
    case BE1::Image::Format::ABGR_4_4_4_4:
    case BE1::Image::Format::ABGR_1_5_5_5:
    case BE1::Image::Format::ABGR_8_8_8_8:
    case BE1::Image::Format::ARGB_4_4_4_4:
    case BE1::Image::Format::ARGB_1_5_5_5:
    case BE1::Image::Format::ARGB_8_8_8_8:
        outFormat = BE1::Image::Format::BGRA_8_8_8_8;
        break;
    case BE1::Image::Format::RGB_8_8_8_SNORM:
        outFormat = BE1::Image::Format::RGBA_8_8_8_8_SNORM;
        break;
    case BE1::Image::Format::RGB_16F_16F_16F:
        outFormat = BE1::Image::Format::RGBA_16F_16F_16F_16F;
        break;
    case BE1::Image::Format::RGB_32F_32F_32F:
        outFormat = BE1::Image::Format::RGBA_32F_32F_32F_32F;
        break;
    case BE1::Image::Format::DXN1:
    case BE1::Image::Format::DXN2:
    case BE1::Image::Format::RGB_PVRTC_2BPPV1:
    case BE1::Image::Format::RGB_PVRTC_4BPPV1:
    case BE1::Image::Format::RGB_8_ETC1:
    case BE1::Image::Format::RGB_8_ETC2:
    case BE1::Image::Format::RGB_ATC:
        outFormat = BE1::Image::Format::BGRX_8_8_8_8;
        break;
    case BE1::Image::Format::DXT1:
    case BE1::Image::Format::DXT3:
    case BE1::Image::Format::DXT5:
    case BE1::Image::Format::RGBA_PVRTC_2BPPV1:
    case BE1::Image::Format::RGBA_PVRTC_4BPPV1:
    case BE1::Image::Format::RGBA_PVRTC_2BPPV2:
    case BE1::Image::Format::RGBA_PVRTC_4BPPV2:
    case BE1::Image::Format::RGBA_8_1_ETC2:
    case BE1::Image::Format::RGBA_8_8_ETC2:
    case BE1::Image::Format::RGBA_EA_ATC:
    case BE1::Image::Format::RGBA_IA_ATC:
        outFormat = BE1::Image::Format::RGBA_8_8_8_8;
        break;
    case BE1::Image::Format::R_11_EAC:
    case BE1::Image::Format::SignedR_11_EAC:
        outFormat = BE1::Image::Format::R_16F;
        break;
    case BE1::Image::Format::RG_11_11_EAC:
    case BE1::Image::Format::SignedRG_11_11_EAC:
        outFormat = BE1::Image::Format::RG_16F_16F;
        break;
    default:
        assert(0);
        outFormat = inFormat;
        break;
    }
    return outFormat;
}

BE1::Image::Format::Enum D3D12Texture::ToCompressedImageFormat(BE1::Image::Format::Enum inFormat, bool useNormalMap) {
    if (BE1::Image::IsCompressed(inFormat)) {
        assert(0);
        return inFormat;
    }

    int redBits, greenBits, blueBits, alphaBits;
    BE1::Image::GetBits(inFormat, &redBits, &greenBits, &blueBits, &alphaBits);

    BE1::Image::Format::Enum outFormat = inFormat;

    if (redBits > 0 && greenBits > 0 && blueBits > 0) {
        if (BE1::Image::IsFloatFormat(inFormat) || BE1::Image::IsHalfFormat(inFormat)) {
            if (alphaBits == 0) {
                outFormat = BE1::Image::Format::RGBE_9_9_9_5;
            }
        } else if (useNormalMap) {
            outFormat = BE1::Image::Format::DXN2;
        } else {
            if (alphaBits <= 1) {
                outFormat = BE1::Image::Format::DXT1;
            } else if (alphaBits <= 4) {
                outFormat = BE1::Image::Format::DXT3;
            } else {
                outFormat = BE1::Image::Format::DXT5;
            }
        }
    }

    return outFormat;
}

void D3D12Texture::AdjustTextureFormat(bool useCompression, bool useNormalMap, BE1::Image::Format::Enum inFormat, BE1::Image::Format::Enum *outFormat) {
    if (BE1::Image::IsDepthFormat(inFormat) || BE1::Image::IsDepthStencilFormat(inFormat)) {
        *outFormat = inFormat;
        return;
    }

    if (BE1::Image::IsCompressed(inFormat)) {
        if (IsSupportedImageFormat(inFormat)) {
            *outFormat = inFormat;
            return;
        }

        inFormat = ToUncompressedImageFormat(inFormat);
    }

    *outFormat = useCompression ? ToCompressedImageFormat(inFormat, useNormalMap) : ToUncompressedImageFormat(inFormat);
}
