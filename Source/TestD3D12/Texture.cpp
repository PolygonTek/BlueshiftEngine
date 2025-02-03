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
#include "Texture.h"

int Texture::MemRequired(bool includingMipmaps) const {
    int numMipmaps = includingMipmaps ? BE1::Image::MaxMipLevels(width, height, depth) : 1;
    int size = BE1::Image::MemRequired(width, height, depth, numMipmaps, format) * numSlices;
    return size;
}

void Texture::Purge() {
    if (texture) {
        RHI::renderer->DestroyTexture(texture);
        texture = nullptr;
    }
}

void Texture::Create(RHI::TextureType textureType, const BE1::Image *srcImage, Texture::Flag flags) {
    Purge();

    BE1::Image::Format srcFormat = srcImage->GetFormat();
    BE1::Image::Format forceFormat = BE1::Image::Format::Unknown;

    bool useNormalMap = BE1::HasFlag(flags, Texture::Flag::NormalMap);
    bool useCompression = !BE1::HasFlag(flags, Texture::Flag::NoCompression);
    bool useMipmaps = !BE1::HasFlag(flags, Texture::Flag::NoMipmaps);
    bool useNPOT = BE1::HasFlag(flags, Texture::Flag::NonePowerOfTwo);

    BE1::Image::Format dstFormat;
    if (forceFormat != BE1::Image::Format::Unknown) {
        dstFormat = forceFormat;
    } else {
        RHI::renderer->AdjustTextureFormat(useCompression, useNormalMap, srcImage->GetFormat(), &dstFormat);
    }

    type = textureType;
    this->srcWidth = srcImage->GetWidth();
    this->srcHeight = srcImage->GetHeight();
    this->srcDepth = srcImage->GetDepth();
    this->numSlices = srcImage->NumSlices();

    uint32_t dstWidth, dstHeight, dstDepth;
    RHI::renderer->AdjustTextureSize(textureType, useNPOT, srcWidth, srcHeight, srcDepth, &dstWidth, &dstHeight, &dstDepth);
    assert(srcDepth == dstDepth);

    BE1::Image scaledImage;

    if (!srcImage->IsEmpty()) {
        if (srcWidth != dstWidth || srcHeight != dstHeight) {
            srcImage->Resize(dstWidth, dstHeight, BE1::Image::ResampleFilter::Bicubic, scaledImage);
            srcImage = &scaledImage;
        }
    }

    this->format = dstFormat;
    this->width = dstWidth;
    this->height = dstHeight;
    this->depth = dstDepth;
    this->flags = flags;
    this->texture = RHI::renderer->CreateTexture(textureType, RHI::ResourceFlag::ShaderResource, srcImage, dstFormat, useMipmaps);
}

void Texture::CreateDefaultTexture(int size, Texture::Flag flags) {
    BE1::Image image;
    image.Create2D(size, size, 1, BE1::Image::Format::RGBA_8_8_8_8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);
    byte *dstPtr = image.GetPixels();

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

    Create(RHI::TextureType::Texture2D, &image, Texture::Flag::HighQuality | flags);
}

void Texture::CreateColorTexture(int size, const BE1::Color4 &color, Flag flags) {
    BE1::Image image;
    image.Create2D(size, size, 1, BE1::Image::Format::RGBA_8_8_8_8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);
    uint32_t *dstPtr = (uint32_t *)image.GetPixels();
    uint32_t c = color.ToUInt32();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            *(uint32_t *)dstPtr++ = c;
        }
    }

    Create(RHI::TextureType::Texture2D, &image, Texture::Flag::HighQuality | flags);
}

void Texture::CreateFlatNormalTexture(int size, Texture::Flag flags) {
    BE1::Image image;
    image.Create2D(size, size, 1, BE1::Image::Format::RGB_8_8_8, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    byte *dstPtr = image.GetPixels();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            dstPtr[0] = 127;
            dstPtr[1] = 127;
            dstPtr[2] = 255;
            dstPtr += 3;
        }
    }

    Create(RHI::TextureType::Texture2D, &image, Texture::Flag::NormalMap | Texture::Flag::NoScaleDown | flags);
}

bool Texture::Load(const char *filename, Texture::Flag flags) {
    flags |= Texture::Flag::LoadedFromFile;

    BE_LOG("Loading texture '%s'...\n", filename);

    BE1::Image image;
    image.Load(filename);

    if (image.IsEmpty()) {
        BE_WARNLOG("Couldn't load texture \"%s\"\n", filename);
        return false;
    }

    RHI::TextureType textureType;

    if (image.GetDepth() > 1) {
        textureType = RHI::TextureType::Texture3D;
    } else if (image.IsCubeMap()) {
        textureType = RHI::TextureType::TextureCube;
    } else {
        textureType = RHI::TextureType::Texture2D;
    }

    Create(textureType, &image, flags);
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
