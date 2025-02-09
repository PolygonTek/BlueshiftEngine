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
#include "Sampler.h"

Texture::SamplerParams  Texture::defaultSamplerParams[to_int(Texture::SamplerParamsType::Count)] = {
    { RHI::TextureFilter::NearestMipmapNearest, RHI::TextureAddressMode::Repeat, RHI::TextureAddressMode::Repeat, RHI::TextureAddressMode::Repeat, 1, RHI::TextureBorderColor::OpaqueBlack },
    { RHI::TextureFilter::NearestMipmapNearest, RHI::TextureAddressMode::Clamp, RHI::TextureAddressMode::Clamp, RHI::TextureAddressMode::Clamp, 1, RHI::TextureBorderColor::OpaqueBlack },
    { RHI::TextureFilter::NearestMipmapNearest, RHI::TextureAddressMode::ClampToBorder, RHI::TextureAddressMode::ClampToBorder, RHI::TextureAddressMode::ClampToBorder, 1, RHI::TextureBorderColor::OpaqueBlack },
    { RHI::TextureFilter::LinearMipmapLinear, RHI::TextureAddressMode::Repeat, RHI::TextureAddressMode::Repeat, RHI::TextureAddressMode::Repeat, 1, RHI::TextureBorderColor::OpaqueBlack },
    { RHI::TextureFilter::LinearMipmapLinear, RHI::TextureAddressMode::Clamp, RHI::TextureAddressMode::Clamp, RHI::TextureAddressMode::Clamp, 1, RHI::TextureBorderColor::OpaqueBlack },
    { RHI::TextureFilter::LinearMipmapLinear, RHI::TextureAddressMode::ClampToBorder, RHI::TextureAddressMode::ClampToBorder, RHI::TextureAddressMode::ClampToBorder, 1, RHI::TextureBorderColor::OpaqueBlack },
};

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
    if (sampler) {
        samplerManager.ReleaseSampler(sampler);
        sampler = nullptr;
    }
}

void Texture::Create(RHI::TextureType textureType, const BE1::Image *srcImage, Texture::Flag flags) {
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

    BE1::Image scaledImage;

    if (!srcImage->IsEmpty()) {
        if (srcWidth != dstWidth || srcHeight != dstHeight) {
            srcImage->Resize(dstWidth, dstHeight, BE1::Image::ResampleFilter::Bicubic, scaledImage);
            srcImage = &scaledImage;
        }
    }

    RHI::ResourceFlag resourceFlags = RHI::ResourceFlag::ShaderResource;
    if (useUAV) {
        resourceFlags |= RHI::ResourceFlag::UnorderedAccess;

        // UAV 가 필요하다면, sRGB 포맷은 UAV 로 만들 수 없기 때문에 Typeless 텍스쳐로 만든다.
        if (srcImage->GetGammaSpace() != BE1::Image::GammaSpace::Linear) {
            resourceFlags |= RHI::ResourceFlag::Typeless;
        }
    }

    // Depth 포맷은 Typeless 텍스쳐로 생성한다.
    // SRV 에서 Depth 포맷을 사용할 수는 없고, 그와 호환되는 포맷을 사용해야 한다.
    if (srcImage->IsDepthFormat()) {
        resourceFlags |= RHI::ResourceFlag::Typeless;
    }

    RHI::GPUResourceState initialState = BE1::HasFlag(flags, Texture::Flag::Compute) ? RHI::GPUResourceState::ShaderResourceCompute : RHI::GPUResourceState::ShaderResource;

    this->format = dstFormat;
    this->flags = flags;
    this->texture = RHI::renderer->CreateTexture(textureType, resourceFlags, srcImage, dstFormat, generateMipmaps, allocateEmptyMipmaps, initialState);
}

void Texture::SetSamplerParameters(const SamplerParams &samplerParams) {
    if (sampler) {
        samplerManager.ReleaseSampler(sampler);
    }
    sampler = samplerManager.GetSampler(samplerParams.filter, samplerParams.addressModeU, samplerParams.addressModeV, samplerParams.addressModeW, samplerParams.maxAnisotropy, samplerParams.borderColor);
}

void Texture::SetSamplerParameters(SamplerParamsType samplerParamsType) {
    int samplerParamsTypeIndex = to_int(samplerParamsType);
    assert(samplerParamsTypeIndex >= 0 && samplerParamsTypeIndex < to_int(SamplerParamsType::Count));
    SetSamplerParameters(Texture::defaultSamplerParams[samplerParamsTypeIndex]);
}

void Texture::GenerationMipmaps() {
    // 전체 텍스쳐 리소스에 대한 SRV 생성
    if (!texture->IsValidSubresource(RHI::SubresourceType::SRV, -1)) {
        RHI::renderer->CreateSubresource(texture, RHI::SubresourceType::SRV);
    }
    // 전체 텍스쳐 리소스에 대한 UAV 생성
    if (!texture->IsValidSubresource(RHI::SubresourceType::UAV, -1)) {
        RHI::renderer->CreateSubresource(texture, RHI::SubresourceType::UAV);
    }

    // 각 Slice 와 Mip Level 별로 SRV/UAV 를 생성
    // NOTE: 이미 Mipmap 용이 아닌 Subresource 를 이전에 생성한 경우 문제가 될 수 있음
    for (int sliceIndex = 0; sliceIndex < GetArraySize(); ++sliceIndex) {
        for (int mipLevel = 0; mipLevel < GetMipLevelCount(); ++mipLevel) {
            const int subresourceIndex = GetMipLevelCount() * sliceIndex + mipLevel;

            if (!texture->IsValidSubresource(RHI::SubresourceType::SRV, subresourceIndex)) {
                RHI::renderer->CreateSubresource(texture, RHI::SubresourceType::SRV, sliceIndex, 1, mipLevel, 1);
            }
            if (!texture->IsValidSubresource(RHI::SubresourceType::UAV, subresourceIndex)) {
                RHI::renderer->CreateSubresource(texture, RHI::SubresourceType::UAV, sliceIndex, 1, mipLevel, 1);
            }
        }
    }

    // 컴퓨트 쉐이더를 이용해서 밉맵 생성
    RHI::CommandList *commandList = RHI::renderer->BeginCommandList(RHI::CommandQueueType::Graphics);
    RHI::renderer->GenerateMipmaps(commandList, GetRHITexture(), true);
    commandList->CloseAndExecute();
    RHI::renderer->EndCommandList(commandList);

    // TODO:
    // 컴퓨트 커맨드 리스트/큐를 이용하려면, 텍스쳐를 만들 때 non-pixel shader resource 로 상태 전이가 이루어져야 한다.
    // 이후에 렌더링에 사용할 것이므로, 다이렉트 커맨드 리스트/큐를 통해 다시 pixel shader resource 로 상태 전이를 해야 한다.
}

void Texture::CreateDefaultTexture(int size, Texture::Flag flags) {
    BE1::Image image;
    image.Create2D(size, size, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);
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

    SamplerParams samplerParams;
    samplerParams.filter = RHI::TextureFilter::LinearMipmapLinear;
    samplerParams.addressModeU = RHI::TextureAddressMode::Clamp;
    samplerParams.addressModeV = RHI::TextureAddressMode::Clamp;
    SetSamplerParameters(samplerParams);
}

void Texture::CreateColorTexture(int size, const BE1::Color4 &color, Flag flags) {
    BE1::Image image;
    image.Create2D(size, size, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);
    uint32_t *dstPtr = (uint32_t *)image.GetPixels();
    uint32_t c = color.ToUInt32();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            *(uint32_t *)dstPtr++ = c;
        }
    }

    Create(RHI::TextureType::Texture2D, &image, Texture::Flag::HighQuality | flags);

    SamplerParams samplerParams;
    samplerParams.filter = RHI::TextureFilter::LinearMipmapLinear;
    samplerParams.addressModeU = RHI::TextureAddressMode::Clamp;
    samplerParams.addressModeV = RHI::TextureAddressMode::Clamp;
    SetSamplerParameters(samplerParams);
}

void Texture::CreateFlatNormalTexture(int size, Texture::Flag flags) {
    BE1::Image image;
    image.Create2D(size, size, 1, BE1::Image::Format::R8G8B8, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
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

    SamplerParams samplerParams;
    samplerParams.filter = RHI::TextureFilter::LinearMipmapLinear;
    samplerParams.addressModeU = RHI::TextureAddressMode::Clamp;
    samplerParams.addressModeV = RHI::TextureAddressMode::Clamp;
    SetSamplerParameters(samplerParams);
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
