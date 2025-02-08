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

#pragma once

#include "RHI.h"
#include "Sampler.h"

class TextureManager;

class Texture {
    friend class TextureManager;

public:
    enum class Flag : uint32_t {
        None                    = 0,
        Permanence              = BIT(0),   ///< 영원히 사용됨 (해제되지 않는다)
        NoCompression           = BIT(1),   ///< 압축을 사용하지 않는다. (원본 포맷이 압축되어 있다면 소용없다)
        NoScaleDown             = BIT(2),   ///< 
        HighQuality             = NoCompression | NoScaleDown,
        NoMipmaps               = BIT(3),   ///< 밉맵을 사용하지 않는다.
        AllocateEmptyMipmaps    = BIT(4),
        NormalMap               = BIT(5),   ///< 노말맵
        NonePowerOfTwo          = BIT(6),   ///< 2 의 승수 사이즈가 아님
        UnorderedAccess         = BIT(7),   ///< UAV 로 접근 가능한 텍스쳐
        LoadedFromFile          = BIT(8)    ///< 파일로부터 읽어들인 텍스쳐인지 여부 (내부적으로 사용됨)
    };

    enum class SamplerParamsType : uint8_t {
        NearestRepeat,
        NearestClamp,
        NearestClampToBorder,
        LinearRepeat,
        LinearClamp,
        LinearClampToBorder,
        Count
    };

    struct SamplerParams {
        RHI::TextureFilter      filter = RHI::TextureFilter::LinearMipmapLinear;
        RHI::TextureAddressMode addressModeU = RHI::TextureAddressMode::Clamp;
        RHI::TextureAddressMode addressModeV = RHI::TextureAddressMode::Clamp;
        RHI::TextureAddressMode addressModeW = RHI::TextureAddressMode::Clamp;
        uint32_t                maxAnisotropy = 1;
        RHI::TextureBorderColor borderColor = RHI::TextureBorderColor::OpaqueBlack;
    };

    Texture() = default;
    ~Texture();

    const char *                GetName() const { return name.c_str(); }
    const char *                GetHashName() const { return hashName.c_str(); }
    RHI::TextureType            GetType() const { return type; }
    uint32_t                    GetWidth() const { return texture->GetWidth(); }
    uint32_t                    GetHeight() const { return texture->GetHeight(); }
    uint32_t                    GetDepth() const { return texture->GetDepth(); }
    uint32_t                    GetArraySize() const { return texture->GetArraySize(); }
    uint32_t                    GetMipLevelCount() const { return texture->GetMipLevelCount(); }
    BE1::Image::Format          GetFormat() const { return format; }
    RHI::Texture *              GetRHITexture() const { return texture; }
    RHI::Sampler *              GetRHISampler() const { return sampler ? sampler->GetRHISampler() : nullptr; }

    int                         MemRequired(bool includingMipmaps) const;

    void                        Update2D(int mipLevel, int xoffset, int yoffset, int width, int height, BE1::Image::Format format, const byte *data);
    void                        Update3D(int mipLevel, int xoffset, int yoffset, int zoffset, int width, int height, int depth, BE1::Image::Format format, const byte *data);
    void                        UpdateCubeFace(RHI::CubemapFace face, int mipLevel, int xoffset, int yoffset, int width, int height, BE1::Image::Format format, const byte *data);

    void                        GetTexels2D(int mipLevel, BE1::Image::Format format, void *pixels) const;
    void                        GetTexels3D(int mipLevel, BE1::Image::Format format, void *pixels) const;
    void                        GetTexelsCubeFace(RHI::CubemapFace face, int mipLevel, BE1::Image::Format format, void *pixels) const;

    void                        Purge();

    void                        Create(RHI::TextureType textureType, const BE1::Image *srcImage, Texture::Flag flags);
    void                        SetSamplerParameters(const SamplerParams &samplerParams);
    void                        SetSamplerParameters(SamplerParamsType samplerParamsType);

    void                        GenerationMipmaps();

    bool                        Load(const char *filename, Texture::Flag flags);

    static void                 GetCubeImageFromCubeTexture(const Texture *cubeTexture, int numMipLevels, BE1::Image &outCubeImage);

private:
    void                        CreateDefaultTexture(int size, Texture::Flag flags);
    void                        CreateColorTexture(int size, const BE1::Color4 &color, Texture::Flag flags);
    void                        CreateFlatNormalTexture(int size, Texture::Flag flags);

    BE1::Str                    hashName;
    BE1::Str                    name;
    mutable int                 refCount = 0;
    RHI::TextureType            type;
    RHI::Texture *              texture = nullptr;
    Sampler *                   sampler = nullptr;
    uint32_t                    srcWidth;       ///< Original width
    uint32_t                    srcHeight;      ///< Original height
    uint32_t                    srcDepth;       ///< Original depth
    BE1::Image::Format          format;
    Texture::Flag               flags;

    static SamplerParams        defaultSamplerParams[to_int(SamplerParamsType::Count)];
};

BE_INLINE Texture::~Texture() {
    Purge();
}

template<>
struct enable_bitmask_operators<Texture::Flag> {
    static const bool enable = true;
};

class TextureManager {
    friend class Texture;

public:
    void                        Init();
    void                        Shutdown();

    Texture *                   AllocTexture(const char *hashName);
    Texture *                   FindTexture(const char *hashName) const;
    Texture *                   GetTexture(const char *hashName, Texture::Flag flags);
    void                        ReleaseTexture(Texture *texture);
    void                        DestroyTexture(Texture *texture);

    Texture *                   defaultTexture;
    Texture *                   whiteTexture;
    Texture *                   flatNormalTexture;

private:
    void                        CreateEngineTextures();

    BE1::StrIHashMap<Texture *> textureHashMap;
};

extern TextureManager           textureManager;
