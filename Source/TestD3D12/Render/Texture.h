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

#pragma once

#include "RHI.h"

class TextureManager;

class Texture {
    friend class TextureManager;

public:
    enum class Flag : uint16_t {
        None                    = 0,
        Permanent               = BIT(0),   ///< Permanently used (never released)
        NoCompression           = BIT(1),   ///< Do not use compression (ineffective if the source format is already compressed)
        NoScaleDown             = BIT(2),   ///< Do not scale down
        HighQuality             = NoCompression | NoScaleDown, ///< High quality settings (disable compression and scaling down)
        NoMipmaps               = BIT(3),   ///< Do not use mipmaps
        AllocateEmptyMipmaps    = BIT(4),   ///< Allocate mipmap levels without initial data
        NormalMap               = BIT(5),   ///< Normal map
        NonePowerOfTwo          = BIT(6),   ///< Texture dimensions are not power-of-two
        Streaming               = BIT(7),   ///< Texture streaming loading (DDS file)
        UnorderedAccess         = BIT(8),   ///< Texture accessible via UAV
        Compute                 = BIT(9),   ///< Create as a compute resource in its initial update
        LoadedFromFile          = BIT(10)   ///< Indicates the texture was loaded from a file (used internally)
    };

    enum class State : uint8_t {
        Empty,
        Loading,
        DestroyPending,
        Ready,
        Failed
    };

    struct SamplerParams {
        RHI::TextureFilter      filter = RHI::TextureFilter::LinearMipmapLinear;
        RHI::TextureAddressMode addressModeU = RHI::TextureAddressMode::Clamp;
        RHI::TextureAddressMode addressModeV = RHI::TextureAddressMode::Clamp;
        RHI::TextureAddressMode addressModeW = RHI::TextureAddressMode::Clamp;
        uint32_t                maxAnisotropy = 1;
        RHI::TextureBorderColor borderColor = RHI::TextureBorderColor::OpaqueBlack;
    };

    struct StreamingState {
        int                     streamingListIndex = -1;
        int                     originalMipLevels = 0;
        int                     residentFirstMip = 0;
        int                     residentMipCount = 0;
        int                     requestedFirstMip = 0;
        int                     requestedMipCount = 0;
        bool                    pending = false;        // 추가 mip 로딩 요청이 진행 중인지 여부
    };

    struct Prepared {
        RHI::TextureType        textureType;
        RHI::Texture *          texture = nullptr;
        uint32_t                srcWidth = 0;
        uint32_t                srcHeight = 0;
        uint32_t                srcDepth = 0;
        BE1::Image::Format      format = BE1::Image::Format::Unknown;
        Texture::Flag           flags = Texture::Flag::None;
    };

    Texture() = default;
    ~Texture() { Purge(); }

    const char *                GetName() const { return name.c_str(); }
    const char *                GetHashName() const { return hashName.c_str(); }

    Texture *                   AddRefCount() { refCount++; return this; }
    const Texture *             AddRefCount() const { refCount++; return this; }
    int                         GetRefCount() const { return refCount; }

    RHI::TextureType            GetType() const { return textureType; }
    uint32_t                    GetIndex() const { return index; }
    uint32_t                    GetWidth() const { return texture->GetWidth(); }
    uint32_t                    GetHeight() const { return texture->GetHeight(); }
    uint32_t                    GetDepth() const { return texture->GetDepth(); }
    uint32_t                    GetArraySize() const { return texture->GetArraySize(); }
    uint32_t                    GetMipLevelCount() const { return texture->GetMipLevelCount(); }
    BE1::Image::Format          GetFormat() const { return format; }
    RHI::Texture *              GetRHITexture() const { return texture; }

    State                       GetState() const { return state.load(); }
    void                        SetState(State newState) { state.store(newState); }

    bool                        IsReadyForRendering() const;

    uint32_t                    GetLoadSerial() const { return loadSerial.load(); }
    uint32_t                    BumpLoadSerial() { return loadSerial.fetch_add(1) + 1; }

    int                         MemRequired(bool includingMipmaps) const;

    bool                        Update2D(int mipLevel, int xoffset, int yoffset, int width, int height, BE1::Image::Format format, const byte *data);
    bool                        Update3D(int mipLevel, int xoffset, int yoffset, int zoffset, int width, int height, int depth, BE1::Image::Format format, const byte *data);
    bool                        UpdateCubeFace(RHI::CubemapFace face, int mipLevel, int xoffset, int yoffset, int width, int height, BE1::Image::Format format, const byte *data);

    bool                        GetTexels2D(int mipLevel, BE1::Image::Format format, void *pixels) const;
    bool                        GetTexels3D(int mipLevel, BE1::Image::Format format, void *pixels) const;
    bool                        GetTexelsCubeFace(RHI::CubemapFace face, int mipLevel, BE1::Image::Format format, void *pixels) const;

    void                        Purge();

    void                        Create(RHI::TextureType textureType, std::shared_ptr<const BE1::Image> srcImage, Texture::Flag flags);

    void                        ApplyPrepared(const Prepared &prepared);

    static bool                 PrepareNew(RHI::TextureType textureType, std::shared_ptr<const BE1::Image> srcImage, Texture::Flag flags, Prepared &outPrepared);
    static bool                 PrepareMipRangeNew(RHI::TextureType textureType, std::shared_ptr<const BE1::Image> srcImage, BE1::Image::MipRange &mipRange, Texture::Flag flags, Prepared &outPrepared);

                                /// Generate mipmaps using compute shader.
    void                        GenerateMipmaps();

    bool                        Load(const char *filename, Texture::Flag flags);
    bool                        LoadAsync(const char *filename, Texture::Flag flags);

    static bool                 GetCubeImageFromCubeTexture(const Texture *cubeTexture, int numMipLevels, BE1::Image &outCubeImage);

private:
    static RHI::Texture *       PrepareRHITexture(RHI::TextureType textureType, RHI::TextureFlag flags, std::shared_ptr<const BE1::Image> srcImage, BE1::Image::Format dstFormat, bool generateMipmaps, bool allocateEmptyMipmaps, RHI::GPUResourceState initialState);

    void                        CreateDefaultTexture(int size, Texture::Flag flags);
    void                        CreateColorTexture(int size, const BE1::Color4 &color, Texture::Flag flags);
    void                        CreateFlatNormalTexture(int size, Texture::Flag flags);

    BE1::Str                    hashName;
    BE1::Str                    name;
    uint32_t                    index = 0;
    mutable int32_t             refCount = 0;
    RHI::TextureType            textureType;
    RHI::Texture *              texture = nullptr;
    uint32_t                    srcWidth;           ///< Original width
    uint32_t                    srcHeight;          ///< Original height
    uint32_t                    srcDepth;           ///< Original depth
    BE1::Image::Format          format;
    Texture::Flag               flags;
    StreamingState              streaming;
    std::atomic<State>          state = State::Empty;
    std::atomic<uint32_t>       loadSerial = 0;
};

template<>
struct enable_bitmask_operators<Texture::Flag> {
    static const bool enable = true;
};

class TextureManager {
    friend class Texture;
    friend class LoadingSystem;

public:
    void                        Init();
    void                        Shutdown();

    Texture *                   AllocTexture(const char *hashName);
    Texture *                   FindTexture(const char *hashName) const;
    Texture *                   GetTexture(const char *hashName, Texture::Flag flags);
    void                        ReleaseTexture(Texture *texture);

    void                        RegisterStreamingTexture(Texture *texture, const BE1::Image::MipRange &mipRange);
    void                        UnregisterStreamingTexture(Texture *texture);
    void                        UpdateStreamingTextures(int frameMsec);
    bool                        CompleteStreamingTextureRequest(Texture *texture, const BE1::Image::MipRange &loadedMipRange, uint32_t *outResidentFirstMip, uint32_t *outResidentMipCount);

    Texture *                   defaultTexture = nullptr;
    Texture *                   whiteTexture = nullptr;
    Texture *                   flatNormalTexture = nullptr;

private:
    void                        CreateEngineTextures();
    void                        DestroyTexture(Texture *texture);

    BE1::StrIHashMap<Texture *> textureHashMap;
    BE1::Array<Texture *>       streamingTextures;
};

extern TextureManager           textureManager;
