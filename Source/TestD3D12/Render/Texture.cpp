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
#include "Texture.h"
#include "Sampler.h"

int Texture::MemRequired(bool includingMipmaps) const {
    uint32_t w = GetWidth();
    uint32_t h = GetHeight();
    uint32_t d = GetDepth();
    uint32_t numSlices = GetArraySize();
    uint32_t numMipmaps = includingMipmaps ? BE1::Image::MaxMipLevels(w, h, d) : 1;
    uint32_t size = BE1::Image::MemRequired(w, h, d, numMipmaps, numSlices, format);
    return size;
}

void Texture::Purge() {
    if (texture) {
        RHI::renderer->DestroyTexture(texture);
        texture = nullptr;
    }
}

void Texture::Create(RHI::TextureType textureType, std::shared_ptr<const BE1::Image> srcImage, Texture::Flag flags) {
    Purge();

    BE1::Image::Format srcFormat = srcImage->GetFormat();
    BE1::Image::Format forceFormat = BE1::Image::Format::Unknown;

    bool useNormalMap = BE1::HasFlag(flags, Texture::Flag::NormalMap);
    bool useCompression = !BE1::HasFlag(flags, Texture::Flag::NoCompression);
    bool useNPOT = BE1::HasFlag(flags, Texture::Flag::NonePowerOfTwo);
    bool useUAV = BE1::HasFlag(flags, Texture::Flag::UnorderedAccess);
    bool allocateEmptyMipmaps = BE1::HasFlag(flags, Texture::Flag::AllocateEmptyMipmaps);
    bool generateMipmaps = !allocateEmptyMipmaps && !BE1::HasFlag(flags, Texture::Flag::NoMipmaps);

    BE1::Image::Format dstFormat;
    if (forceFormat != BE1::Image::Format::Unknown) {
        dstFormat = forceFormat;
    } else {
        RHI::renderer->AdjustTextureFormat(useCompression, useNormalMap, useUAV, srcImage->GetFormat(), &dstFormat);
    }

    type = textureType;
    this->srcWidth = srcImage->GetWidth();
    this->srcHeight = srcImage->GetHeight();
    this->srcDepth = srcImage->GetDepth();

    uint32_t dstWidth, dstHeight, dstDepth;
    RHI::renderer->AdjustTextureSize(textureType, useNPOT, srcWidth, srcHeight, srcDepth, &dstWidth, &dstHeight, &dstDepth);
    assert(srcDepth == dstDepth);

    if (!srcImage->IsEmpty()) {
        if (srcWidth != dstWidth || srcHeight != dstHeight) {
            std::shared_ptr<BE1::Image> scaledImage = std::make_shared<BE1::Image>();
            srcImage->Resize(dstWidth, dstHeight, BE1::Image::ResampleFilter::Bicubic, *scaledImage);
            srcImage = scaledImage;
        }
    }

    RHI::TextureFlag resourceFlags = RHI::TextureFlag::ShaderResource;
    if (useUAV) {
        resourceFlags |= RHI::TextureFlag::UnorderedAccess;

        // UAV 가 필요하다면, sRGB 포맷은 UAV 로 만들 수 없기 때문에 Typeless 텍스쳐로 만든다.
        if (srcImage->GetGammaSpace() != BE1::Image::GammaSpace::Linear) {
            resourceFlags |= RHI::TextureFlag::Typeless;
        }
    }

    // Depth 포맷은 Typeless 텍스쳐로 생성한다.
    // SRV 에서 Depth 포맷을 사용할 수는 없고, 그와 호환되는 포맷을 사용해야 한다.
    if (srcImage->IsDepthFormat()) {
        resourceFlags |= RHI::TextureFlag::Typeless;
    }

    RHI::GPUResourceState initialState = BE1::HasFlag(flags, Texture::Flag::Compute) ? RHI::GPUResourceState::ShaderResourceCompute : RHI::GPUResourceState::ShaderResource;

    this->format = dstFormat;
    this->flags = flags;
    this->texture = PrepareTextureImage(textureType, resourceFlags, srcImage, dstFormat, generateMipmaps, allocateEmptyMipmaps, initialState);
}

RHI::Texture *Texture::PrepareTextureImage(RHI::TextureType textureType, RHI::TextureFlag flags, std::shared_ptr<const BE1::Image> srcImage, BE1::Image::Format dstFormat, bool generateMipmaps, bool allocateEmptyMipmaps, RHI::GPUResourceState initialState) {
    BE1::Image::Format srcFormat = srcImage->GetFormat();
    bool srcCompressed = BE1::Image::IsCompressed(srcFormat);
    bool srcFormatSupported = RHI::renderer->IsSupportedImageFormat(srcFormat);

    if (!srcImage->IsEmpty()) {
        bool dstFormatSupported = RHI::renderer->IsSupportedImageFormat(dstFormat);

        if (!dstFormatSupported) {
            BE_WARNLOG("Texture::PrepareTextureImage: Unsupported internal image format %s\n", BE1::Image::FormatName(dstFormat));
            return nullptr;
        }

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
                    BE1::Image::Format supportedUncompressedFormat = RHI::renderer->ToUncompressedImageFormat(srcFormat);

                    std::shared_ptr<BE1::Image> uncompressedImage = std::make_shared<BE1::Image>();
                    srcImage->ConvertFormat(supportedUncompressedFormat, *uncompressedImage);
                    srcImage = uncompressedImage;

                    srcFormat = supportedUncompressedFormat;
                    srcFormatSupported = RHI::renderer->IsSupportedImageFormat(srcFormat);
                    srcCompressed = false;
                }

                // CPU 에서 밉맵을 직접 생성한다.
                std::shared_ptr<BE1::Image> mipmappedImage = std::make_shared<BE1::Image>();
                mipmappedImage->Create(w, h, d, srcImage->GetArraySize(), maxMipLevels, srcImage->GetFormat(), srcImage->GetGammaSpace(), nullptr, srcImage->GetFlags());
                mipmappedImage->CopyFrom(*srcImage, 0, 1);
                mipmappedImage->GenerateMipmaps();
                srcImage = mipmappedImage;
            }
        }

        // dstFormat 으로 컨버팅
        if (srcFormat != dstFormat) {
            std::shared_ptr<BE1::Image> dstImage = std::make_shared<BE1::Image>();
            srcImage->ConvertFormat(dstFormat, *dstImage);
            srcImage = dstImage;
        }
    }

    return RHI::renderer->CreateTexture(textureType, flags, srcImage, allocateEmptyMipmaps, {}, 1, initialState);
}

void Texture::GenerateMipmaps() {
    RHI::renderer->GenerateMipmaps(texture);
}

void Texture::CreateDefaultTexture(int size, Texture::Flag flags) {
    std::shared_ptr<BE1::Image> image = std::make_shared<BE1::Image>();
    image->Create2D(size, size, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);
    byte *dstPtr = image->GetPixels();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (x == 0 || x == size - 1 || y == 0 || y == size - 1) {
                dstPtr[0] = (byte)(0);
                dstPtr[1] = (byte)(0);
                dstPtr[2] = (byte)(0);
                dstPtr[3] = (byte)(255);
            } else {
                dstPtr[0] = (byte)(255);
                dstPtr[1] = (byte)(0);
                dstPtr[2] = (byte)(255);
                dstPtr[3] = (byte)(255);
            }
            dstPtr += 4;
        }
    }

    flags |= Texture::Flag::HighQuality | Texture::Flag::NoCompression | Texture::Flag::UnorderedAccess | Texture::Flag::NoMipmaps | Texture::Flag::AllocateEmptyMipmaps;
    Create(RHI::TextureType::Texture2D, image, flags);

    // 컴퓨트 쉐이더를 통해 밉맵 생성
    // 컴퓨트 쉐이더로 밉맵을 생성하기 위해서는 대상 텍스쳐를 만들 때 하위 mip level 들이 존재해야 하고, UAV 도 만들어야 한다.
    // UAV 로 만들 sRGB 텍스쳐의 경우에는 텍스쳐 생성 함수에서 자동으로 Typeless 텍스쳐로 만들어 진다.
    // sRGB 텍스쳐를 생성할 때 Typeless 로 만들어야 SRV 는 sRGB 포맷으로, UAV 는 Linear 포맷으로 생성된다.
    GenerateMipmaps();
}

void Texture::CreateColorTexture(int size, const BE1::Color4 &color, Flag flags) {
    std::shared_ptr<BE1::Image> image = std::make_shared<BE1::Image>();
    image->Create2D(size, size, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);
    uint32_t *dstPtr = (uint32_t *)image->GetPixels();
    uint32_t c = color.ToUInt32();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            *(uint32_t *)dstPtr++ = c;
        }
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::HighQuality | flags);
}

void Texture::CreateFlatNormalTexture(int size, Texture::Flag flags) {
    std::shared_ptr<BE1::Image> image = std::make_shared<BE1::Image>();
    image->Create2D(size, size, 1, BE1::Image::Format::R8G8B8, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    byte *dstPtr = image->GetPixels();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            dstPtr[0] = 127;
            dstPtr[1] = 127;
            dstPtr[2] = 255;
            dstPtr += 3;
        }
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::NormalMap | Texture::Flag::NoScaleDown | flags);
}

bool Texture::Load(const char *filename, Texture::Flag flags) {
    flags |= Texture::Flag::LoadedFromFile;

    BE_LOG("Loading texture '%s'...\n", filename);

    std::shared_ptr<BE1::Image> image = std::make_shared<BE1::Image>();
    image->Load(filename);

    if (image->IsEmpty()) {
        BE_WARNLOG("Couldn't load texture \"%s\"\n", filename);
        return false;
    }

    RHI::TextureType textureType;

    if (image->GetDepth() > 1) {
        textureType = RHI::TextureType::Texture3D;
    } else if (image->IsCubeMap()) {
        textureType = RHI::TextureType::TextureCube;
    } else {
        textureType = RHI::TextureType::Texture2D;
    }

    Create(textureType, image, flags);
    return true;
}

void Texture::Update2D(int mipLevel, int xoffset, int yoffset, int width, int height, BE1::Image::Format format, const byte *data) {
    RHI::renderer->SetTextureSubImage2D(texture, mipLevel, xoffset, yoffset, width, height, format, data);
}

void Texture::Update3D(int mipLevel, int xoffset, int yoffset, int zoffset, int width, int height, int depth, BE1::Image::Format format, const byte *data) {
    RHI::renderer->SetTextureSubImage3D(texture, mipLevel, xoffset, yoffset, zoffset, width, height, depth, format, data);
}

void Texture::UpdateCubeFace(RHI::CubemapFace face, int mipLevel, int xoffset, int yoffset, int width, int height, BE1::Image::Format format, const byte *data) {
    RHI::renderer->SetTextureSubImageCubeFace(texture, face, mipLevel, xoffset, yoffset, width, height, format, data);
}

void Texture::GetTexels2D(int mipLevel, BE1::Image::Format format, void *pixels) const {
    RHI::renderer->GetTextureImage2D(texture, mipLevel, format, pixels);
}

void Texture::GetTexels3D(int mipLevel, BE1::Image::Format format, void *pixels) const {
    RHI::renderer->GetTextureImage3D(texture, mipLevel, format, pixels);
}

void Texture::GetTexelsCubeFace(RHI::CubemapFace face, int mipLevel, BE1::Image::Format format, void *pixels) const {
    RHI::renderer->GetTextureImageCubeFace(texture, face, mipLevel, format, pixels);
}

void Texture::GetCubeImageFromCubeTexture(const Texture *cubeTexture, int numMipLevels, BE1::Image &outCubeImage) {
    BE1::Image::GammaSpace gammaSpace = BE1::Image::NeedFloatConversion(cubeTexture->format) ? BE1::Image::GammaSpace::Linear : BE1::Image::GammaSpace::sRGB;
    BE1::Image faceImages[6];

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        faceImages[faceIndex].Create2D(cubeTexture->GetWidth(), cubeTexture->GetWidth(), numMipLevels, cubeTexture->GetFormat(), gammaSpace, nullptr, BE1::Image::Flag::None);

        for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
            cubeTexture->GetTexelsCubeFace((RHI::CubemapFace)faceIndex, mipLevel, cubeTexture->GetFormat(), faceImages[faceIndex].GetPixels(mipLevel));
        }
    }

    outCubeImage.CreateCubeFrom6Faces(faceImages);
}
