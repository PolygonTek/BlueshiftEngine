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
#include "Shaders/ShaderInterop.h"

class TextureManager;
class LoadingSystem;

/*
-------------------------------------------------------------------------------

    Texture

-------------------------------------------------------------------------------
*/

class Texture : public std::enable_shared_from_this<Texture> {
    friend class TextureManager;
    friend class LoadingSystem;

public:
    enum class Flag : uint16_t {
        None                        = 0,
        NoCompression               = BIT(0),   ///< Do not use compression (ineffective if the source format is already compressed)
        NoScaleDown                 = BIT(1),   ///< Do not scale down
        HighQuality                 = NoCompression | NoScaleDown, ///< High quality settings (disable compression and scaling down)
        NoMipmaps                   = BIT(2),   ///< Do not use mipmaps
        AllocateEmptyMipmaps        = BIT(3),   ///< Allocate mipmap levels without initial data
        NormalMap                   = BIT(4),   ///< Normal map
        NonPowerOfTwo               = BIT(5),   ///< Texture dimensions are not power-of-two
        Streaming                   = BIT(6),   ///< Texture streaming loading (only DDS file, with async loading)
        UnorderedAccess             = BIT(7),   ///< Texture accessible via UAV
        Compute                     = BIT(8),   ///< Create as a compute resource in its initial update
        LoadedFromFile              = BIT(9)   ///< Indicates the texture was loaded from a file (used internally)
    };

    enum class State : uint8_t {
        Empty,
        Loading,
        Ready,
        Failed
    };

    struct SamplerParams {
        RHI::TextureFilter          filter = RHI::TextureFilter::LinearMipmapLinear;
        RHI::TextureAddressMode     addressModeU = RHI::TextureAddressMode::Clamp;
        RHI::TextureAddressMode     addressModeV = RHI::TextureAddressMode::Clamp;
        RHI::TextureAddressMode     addressModeW = RHI::TextureAddressMode::Clamp;
        uint32_t                    maxAnisotropy = 1;
        RHI::TextureBorderColor     borderColor = RHI::TextureBorderColor::OpaqueBlack;
    };

    struct Streaming {
        uint32_t                    streamingListIndex = TEXTURE_STREAMING_INVALID_INDEX;
        uint32_t                    originalWidth = 0;
        uint32_t                    originalHeight = 0;
        uint32_t                    originalDepth = 0;
        int                         originalMipLevels = 0;
        int                         residentFirstMip = -1;
        int                         requestedFirstMip = -1;
        uint32_t                    requestedMaxDimension = 0;
        uint32_t                    streamInFrameCount = 0;
        uint32_t                    streamOutFrameCount = 0;
        RHI::Texture *              pendingReplacementTexture = nullptr;
    };

    struct InternalData {
        InternalData() = default;
        InternalData(const InternalData &rhs) = delete;
        InternalData &operator=(const InternalData &rhs) = delete;
        InternalData(InternalData &&rhs) noexcept { Swap(rhs); }
        InternalData &operator=(InternalData &&rhs) noexcept;
        ~InternalData() { Purge(); }

        void                        Purge();
        void                        Swap(InternalData &rhs);

        RHI::TextureType            rhiTextureType = RHI::TextureType::Unknown;
        RHI::Texture *              rhiTexture = nullptr;
        BE1::Image::Format          format = BE1::Image::Format::Unknown;
        Texture::Flag               flags = Texture::Flag::None;
    };

    Texture() = default;
    Texture(const Texture &rhs) = delete;
    Texture &operator=(const Texture &rhs) = delete;
    ~Texture() { Purge(); }

    const char *                    GetName() const { return name.c_str(); }
    const char *                    GetHashName() const { return hashName.c_str(); }

    State                           GetState() const { return state.load(); }
    void                            SetState(State newState) { state.store(newState); }

    uint32_t                        GetLoadSerial() const { return loadSerial.load(); }
    uint32_t                        BumpLoadSerial() { return loadSerial.fetch_add(1) + 1; }

    RHI::TextureType                GetType() const { return internalData.rhiTextureType; }
    uint32_t                        GetWidth() const { return internalData.rhiTexture->GetWidth(); }
    uint32_t                        GetHeight() const { return internalData.rhiTexture->GetHeight(); }
    uint32_t                        GetDepth() const { return internalData.rhiTexture->GetDepth(); }
    uint32_t                        GetArraySize() const { return internalData.rhiTexture->GetArraySize(); }
    uint32_t                        GetMipLevelCount() const { return internalData.rhiTexture->GetMipLevelCount(); }
    BE1::Image::Format              GetFormat() const { return internalData.format; }
    Flag                            GetFlags() const { return internalData.flags; }
    RHI::Texture *                  GetRHITexture() const { return internalData.rhiTexture; }
    uint32_t                        GetIndex() const { return index; }
    const Streaming *               GetStreaming() const { return &streaming; }
    Streaming *                     GetStreaming() { return &streaming; }

    bool                            IsReadyForRendering() const;

    uint64_t                        MemRequired(bool includingMipmaps) const;
    uint64_t                        MemRequiredStreaming(int firstMip) const;

    bool                            Update2D(int mipLevel, int xoffset, int yoffset, int width, int height, BE1::Image::Format format, const byte *data);
    bool                            Update3D(int mipLevel, int xoffset, int yoffset, int zoffset, int width, int height, int depth, BE1::Image::Format format, const byte *data);
    bool                            UpdateCubeFace(RHI::CubemapFace face, int mipLevel, int xoffset, int yoffset, int width, int height, BE1::Image::Format format, const byte *data);

    bool                            GetTexels2D(int mipLevel, BE1::Image::Format format, void *pixels) const;
    bool                            GetTexels3D(int mipLevel, BE1::Image::Format format, void *pixels) const;
    bool                            GetTexelsCubeFace(RHI::CubemapFace face, int mipLevel, BE1::Image::Format format, void *pixels) const;

    void                            Reset();

    bool                            Create(RHI::TextureType textureType, std::shared_ptr<const BE1::Image> srcImage, Texture::Flag flags);

                                    /// Prepares a texture from an image, applying the renderer's normal texture processing such as size adjustment, format conversion, and mipmap generation.
    static bool                     Prepare(RHI::TextureType textureType, std::shared_ptr<const BE1::Image> srcImage, Texture::Flag flags, InternalData &outInternalData);
                                    /// Prepares a texture directly from an already-final image without resizing, format conversion, or mip generation.
    static bool                     PrepareDirect(RHI::TextureType textureType, std::shared_ptr<const BE1::Image> srcImage, Texture::Flag flags, InternalData &outInternalData);
    void                            ApplyPrepared(InternalData &newInternalData);

                                    /// Generate mipmaps using compute shader.
    void                            GenerateMipmaps();

    bool                            Load(const char *filename, Texture::Flag flags);
    bool                            LoadAsync(const char *filename, Texture::Flag flags);

    bool                            Reload();
    bool                            ReloadAsync();

    void                            RequestStreamingMaxDimension(uint32_t requestedMaxDimension);

    static bool                     GetCubeImageFromCubeTexture(const Texture *cubeTexture, int numMipLevels, BE1::Image &outCubeImage);

private:
    void                            Purge();

    static RHI::Texture *           CreateRHITexture(RHI::TextureType textureType, RHI::TextureFlag flags, std::shared_ptr<const BE1::Image> srcImage, BE1::Image::Format dstFormat, bool generateMipmaps, bool allocateEmptyMipmaps, RHI::GPUResourceState initialState);

    void                            CreateDefaultTexture(int size, Texture::Flag flags = Texture::Flag::None);
    void                            CreateColorTexture(int size, const BE1::Color4 &color, Texture::Flag flags = Texture::Flag::None);
    void                            CreateFlatNormalTexture(int size, Texture::Flag flags = Texture::Flag::None);

    BE1::Str                        hashName;
    BE1::Str                        name;
    std::atomic<State>              state = State::Empty;
    std::atomic<uint32_t>           loadSerial = 0;
    uint32_t                        index = 0;
    Streaming                       streaming;
    InternalData                    internalData;
};

template<>
struct enable_bitmask_operators<Texture::Flag> {
    static const bool enable = true;
};

class TextureManager {
    friend class Texture;
    friend class LoadingSystem;

public:
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    std::shared_ptr<Texture>        AllocTexture(const char *hashName);
    std::shared_ptr<Texture>        FindTexture(const char *hashName) const;
    std::shared_ptr<Texture>        GetTexture(const char *hashName, Texture::Flag flags = Texture::Flag::None);

    bool                            IsRegisteredStreamingTexture(const Texture *texture) const;
    void                            RegisterStreamingTexture(const std::shared_ptr<Texture> &texture, const BE1::Image::MipRange &mipRange);
    void                            UnregisterStreamingTexture(Texture *texture);
    uint32_t                        GetStreamingTextureCount() const { return streamingTextures.Count(); }
    uint64_t                        GetStreamingTextureMemoryUsageBytes() const { return streamingTextureMemoryUsageBytes; }
    uint64_t                        GetStreamingTextureMemoryBudgetBytes() const { return streamingTextureMemoryBudgetBytes; }
    void                            SetStreamingTextureMemoryBudgetBytes(uint64_t budget) { streamingTextureMemoryBudgetBytes = budget; }

    void                            ApplyStreamingFeedback(const uint32_t *feedbackData, uint32_t feedbackCapacity);

    std::shared_ptr<Texture>        defaultTexture;
    std::shared_ptr<Texture>        whiteTexture;
    std::shared_ptr<Texture>        flatNormalTexture;

    static constexpr uint32_t       StreamInDelay = 0;
    static constexpr uint32_t       MaxStreamingRequests = 256;
    static constexpr uint32_t       MaxStreamingLevelReduction = 6;
    static constexpr uint32_t       MinResidentStreamingTextureMaxDimension = 32;

private:
    void                            CleanExpiredTextures();
    void                            CreateEngineTextures();
    void                            DestroyEngineTextures();
    void                            UpdateStreamingTextures();
    void                            ComputeStreamingTextureMemoryUsage();

    BE1::StrIHashMap<std::weak_ptr<Texture>> textureHashMap;
    int                             expiredTextureHashMapCursor = 0;
    uint32_t                        nextTextureIndex = 0;
    BE1::Array<std::weak_ptr<Texture>> streamingTextures;
    uint64_t                        streamingTextureMemoryUsageBytes = 0;
    uint64_t                        streamingTextureMemoryBudgetBytes = 256 * (1024 * 1024);
};

extern TextureManager               textureManager;
