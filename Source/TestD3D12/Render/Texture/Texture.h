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

#include "RHI/RHI.h"
#include "Shaders/ShaderInterop.h"

class TextureManager;
class LoadingSystem;

/*
---------------------------------------------------------------------------------------------------

    Texture

---------------------------------------------------------------------------------------------------
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
        LoadedFromFile              = BIT(9)    ///< Indicates the texture was loaded from a file (used internally)
    };

    enum class State : uint8_t {
        Empty,
        Loading,
        Ready,
        Reloading,
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
        uint32_t                    streamingListIndex = TextureStreamingInvalidIndex;
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

    Texture() = default;
    Texture(const Texture &rhs) = delete;
    Texture &operator=(const Texture &rhs) = delete;
    ~Texture() { Purge(); }

    const char *                    GetName() const { return name.c_str(); }
    const char *                    GetHashName() const { return hashName.c_str(); }

    State                           GetState() const { return state.load(); }
    void                            SetState(State newState) { state.store(newState); }
    bool                            IsReady() const { State s = GetState(); return s == State::Ready || s == State::Reloading; }
    bool                            IsLoading() const { State s = GetState(); return s == State::Loading || s == State::Reloading; }

                                    // 로딩 요청 했던 시리얼 번호.
    uint32_t                        GetLoadSerial() const { return loadSerial.load(); }

                                    // 이미 진행 중인 로딩 completion 무효화
    uint32_t                        BumpLoadSerial() { return loadSerial.fetch_add(1) + 1; }

    uint64_t                        GetRetainedFrameSerial() const { return retainedFrameSerial; }
    void                            SetRetainedFrameSerial(uint64_t frameSerial) { retainedFrameSerial = frameSerial; }

    RHI::TextureType                GetType() const { assert(IsReady()); return internalData.rhiTextureType; }
    uint32_t                        GetWidth() const { assert(IsReady()); return internalData.rhiTexture->GetWidth(); }
    uint32_t                        GetHeight() const { assert(IsReady()); return internalData.rhiTexture->GetHeight(); }
    uint32_t                        GetDepth() const { assert(IsReady()); return internalData.rhiTexture->GetDepth(); }
    uint32_t                        GetArraySize() const { assert(IsReady()); return internalData.rhiTexture->GetArraySize(); }
    uint32_t                        GetMipLevelCount() const { assert(IsReady()); return internalData.rhiTexture->GetMipLevelCount(); }
    BE1::Image::Format              GetFormat() const { assert(IsReady()); return internalData.format; }
    Flag                            GetFlags() const { assert(IsReady()); return internalData.flags; }
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

    bool                            Create(RHI::TextureType textureType, SharedPtr<const BE1::Image> srcImage, Texture::Flag flags);

                                    // 리로드 완료 후 리소스를 사용할 수 있으면 true 를 반환한다. 새 데이터 로딩 성공 여부와는 무관하다.
    bool                            Reload();

                                    // 비동기 리로드 요청 등록에 성공하면 true 를 반환하며, 완료 전까지 기존 리소스를 계속 사용할 수 있다.
    bool                            ReloadAsync();

                                    // 최초 로딩 중이면 완료될 때까지 기다리고, 리로딩 중이면 기존 Texture 를 사용할 수 있으므로 Ready 로 간주한다.
    bool                            WaitReady();

                                    // 최초 로딩과 리로딩이 모두 완료될 때까지 기다린다.
    bool                            WaitLoadComplete();

    void                            RequestStreamingMaxDimension(uint32_t requestedMaxDimension);

    static Flag                     ApplyTextureHints(Flag baseFlags, BE1::Image::Flag textureHints);
    static bool                     GetCubeImageFromCubeTexture(const Texture *cubeTexture, int numMipLevels, BE1::Image &outCubeImage);

private:
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

    void                            Purge();

                                    /// Prepares a texture from an image, applying the renderer's normal texture processing such as size adjustment, format conversion, and mipmap generation.
    static bool                     Prepare(RHI::TextureType textureType, SharedPtr<const BE1::Image> srcImage, Texture::Flag flags, InternalData &outInternalData);
                                    /// Prepares a texture directly from an already-final image without resizing, format conversion, or mip generation.
    static bool                     PrepareDirect(RHI::TextureType textureType, SharedPtr<const BE1::Image> srcImage, Texture::Flag flags, InternalData &outInternalData);
    void                            ApplyPrepared(InternalData &newInternalData);

    bool                            Load(const char *filename, Texture::Flag flags);
    bool                            LoadAsync(const char *filename, Texture::Flag flags);

    static RHI::Texture *           CreateRHITexture(RHI::TextureType textureType, RHI::TextureFlag flags, SharedPtr<const BE1::Image> srcImage, BE1::Image::Format dstFormat, bool generateMipmapsOnCPU, RHI::TextureMipInitMode mipInitMode, RHI::GPUResourceState initialState);

    void                            CreateDefaultTexture(int size, Texture::Flag flags = Texture::Flag::None);
    void                            CreateColorTexture(int size, const BE1::Color4 &color, Texture::Flag flags = Texture::Flag::None);
    void                            CreateFlatNormalTexture(int size, Texture::Flag flags = Texture::Flag::None);

    BE1::Str                        hashName;
    BE1::Str                        name;
    std::atomic<State>              state = State::Empty;
    std::atomic<uint32_t>           loadSerial = 0;
    uint32_t                        index = 0;
    uint64_t                        retainedFrameSerial = 0;
    Streaming                       streaming;
    InternalData                    internalData;
};

template<>
struct enable_bitmask_operators<Texture::Flag> {
    static constexpr bool enable = true;
};
