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

/*
-------------------------------------------------------------------------------

    Texture

-------------------------------------------------------------------------------
*/

#include "Core/Str.h"
#include "Containers/HashMap.h"
#include "Core/CVars.h"
#include "Image/Image.h"
#include "RHI/RHI.h"

BE_NAMESPACE_BEGIN

class RenderTarget;

class Texture {
    friend class TextureManager;
    friend class RenderTarget;
    friend class Shader;

public:
    struct Flag {
        enum Enum {
            Repeat              = BIT(0),
            MirroredRepeat      = BIT(1),
            Clamp               = BIT(2),       ///< Set wrap mode to Clamp (can be set with Mirror).
            ClampToBorder       = BIT(3),       ///< Set wrap mode to ClampToBorder (can be set with Mirror).
            ZeroClamp           = BIT(4),       ///< Set wrap mode to ClampToBorder and set border color to black.
            Shadow              = BIT(5),       ///< Set shadow func (You must turn this flag on to use samperXDShadow on the shader).
            Nearest             = BIT(6),       ///< Use nearest filtering (should be set with NoMipmaps).
            NoMipmaps           = BIT(7),       ///< Don't use mipmaps.
            NoScaleDown         = BIT(8),       ///< Don't reduce size by texture_mip cvar value.
            NoCompression       = BIT(9),       ///< Don't use compression by texture_useCompression cvar value. (Useless if original format is compressed).
            NonPowerOfTwo       = BIT(10),      ///< This is a non-power-of-two texture.
            NormalMap           = BIT(11),      ///< This is a normal map.
            HighPriority        = BIT(12),
            LowPriority         = BIT(13),
            SRGBColorSpace      = BIT(14),      ///< Generally color images are encoded in sRGB space. So we need to tell the GPU to upload it to memory in a linear fashion.
            Trilinear           = BIT(15),
            HighQuality         = NoScaleDown | NoCompression,
            CubeMap             = BIT(28),      ///< This is a cube map.
            CameraCubeMap       = BIT(29),
            LoadedFromFile      = BIT(30),      ///< Mark it as loaded from a file.
            Permanence          = BIT(31)
        };
    };

    Texture();
    ~Texture();

    const char *            GetName() const { return name; }
    const char *            GetHashName() const { return hashName; }

    RHI::TextureType::Enum  GetType() const { return type; }
    int                     GetWidth() const { return width; }
    int                     GetHeight() const { return height; }
    int                     GetDepth() const { return depth; }
    int                     NumSlices() const { return numSlices; }
    Image::Format::Enum     GetFormat() const { return format; }
    int                     GetFlags() const { return flags; }

    int                     MemRequired(bool includingMipmaps) const;

    bool                    IsDefaultTexture() const;

    RenderTarget *          GetRenderTarget() const { return renderTarget; }

    void                    Create(RHI::TextureType::Enum type, const Image &srcImage, int flags);
    void                    CreateEmpty(RHI::TextureType::Enum type, int width, int height, int depth, int numSlices, int numMipmaps, Image::Format::Enum format, int flags);
    void                    CreateFromBuffer(Image::Format::Enum format, RHI::Handle bufferHandle);

                            /// Create indirection cubemap
                            /// @param size         size of indirection cubemap
                            /// @param vcmWidth     actual height of virtual cubemap texture
                            /// @param vcmHeight    actual width of virtual cubemap texture
    void                    CreateIndirectionCubemap(int size, int vcmWidth, int vcmHeight, int flags = 0);
    void                    CreateDefaultTexture(int size, int flags = 0);
    void                    CreateZeroClampTexture(int size, int flags = 0);
    void                    CreateFlatNormalTexture(int size, int flags = 0);
    void                    CreateDefaultCubeMapTexture(int size, int flags = 0);
    void                    CreateBlackCubeMapTexture(int size, int flags = 0);
    void                    CreateNormalizationCubeMapTexture(int size, int flags = 0);
    void                    CreateCubicNormalCubeMapTexture(int size, int flags = 0);
    void                    CreateAttenuationTexture(int size, int flags = 0);
    void                    CreateFogTexture(int flags = 0);
    void                    CreateFogEnterTexture(int flags = 0);
    void                    CreateRandomRotMatTexture(int size, int flags = 0);
    void                    CreateRandomDir4x4Texture(int flags = 0);

    void                    Upload(const Image *srcImage);

    void                    Update2D(int mipLevel, int xoffset, int yoffset, int width, int height, Image::Format::Enum format, const byte *data);
    void                    Update3D(int mipLevel, int xoffset, int yoffset, int zoffset, int width, int height, int depth, Image::Format::Enum format, const byte *data);
    void                    UpdateCubemap(int face, int mipLevel, int xoffset, int yoffset, int width, int height, Image::Format::Enum format, const byte *data);
    void                    UpdateRect(int xoffset, int yoffset, int width, int height, Image::Format::Enum format, const byte *data);

    void                    GetTexels2D(int mipLevel, Image::Format::Enum format, void *pixels) const;
    void                    GetTexels3D(int mipLevel, Image::Format::Enum format, void *pixels) const;
    void                    GetTexelsCubemap(int face, int mipLevel, Image::Format::Enum format, void *pixels) const;
    void                    GetTexelsRect(Image::Format::Enum format, void *pixels) const;

    void                    CopyTo(int mipLevel, Texture *dstTexture);

    void                    Purge();

    bool                    Load(const char *filename, int flags);
    bool                    Reload();

    const Texture *         AddRefCount() const { refCount++; return this; }
    int                     GetRefCount() const { return refCount; }

    void                    Bind() const;

    void                    GenerateMipmap() const;

    static void             GetCubeImageFromCubeTexture(const Texture *cubeTexture, int numMipLevels, Image &cubeImage);

private:
    Str                     hashName;                   // texture filename including path
    Str                     name;
    mutable int             refCount = 0;               // reference count
    bool                    permanence = false;         //
    int                     frameCount;
    int                     flags = 0;                  // texture load flags

    RHI::Handle             textureHandle = RHI::NullTexture; // texture handle
    RHI::TextureType::Enum  type = RHI::TextureType::Texture2D;
    RHI::AddressMode::Enum  addressMode = RHI::AddressMode::Repeat;

    Image::Format::Enum     format = Image::Format::Unknown;    // internal image format

    int                     srcWidth = 0;               // original width
    int                     srcHeight = 0;              // original height
    int                     srcDepth = 0;               // original depth
    int                     numSlices = 0;

    int                     width = 0;                  // scaled width
    int                     height = 0;                 // scaled height
    int                     depth = 0;                  // scaled depth

    bool                    hasMipmaps = false;

    mutable RenderTarget *  renderTarget = nullptr;
};

BE_INLINE Texture::Texture() {
}

BE_INLINE Texture::~Texture() {
    Purge();
}

class TextureGeneratorBase {
public:
    virtual ~TextureGeneratorBase() {}
    virtual void            Generate(Texture *texture) const = 0;
};

class TextureGenerator_CubicDiffuseIrradianceMap : public TextureGeneratorBase {
public:
    virtual void            Generate(Texture *texture) const;
};

class TextureGenerator_CubicSpecularIrradianceMap : public TextureGeneratorBase {
public:
    virtual void            Generate(Texture *texture) const;
};

class TextureManager {
    friend class Texture;

public:
    void                    Init();
    void                    Shutdown();

    Texture *               AllocTexture(const char *name);
    Texture *               FindTexture(const char *name) const;
    Texture *               GetTexture(const char *name);
    Texture *               GetTextureWithoutTextureInfo(const char *name, int creationFlags);

    Texture *               TextureFromGenerator(const char *name, const TextureGeneratorBase &generator);

    void                    ReleaseTexture(Texture *texture, bool immediateDestroy = false);
    void                    DestroyTexture(Texture *texture);

                            // Destroy textures that zero reference counted
    void                    DestroyUnusedTextures();

    void                    PrecacheTexture(const char *filename);

    void                    RenameTexture(Texture *texture, const Str &newName);

    void                    SetFilter(const char *filterName);
    void                    SetAnisotropy(float anisotropy);
    void                    SetLodBias(float lodBias) const;

                            // pre-defined textures
    Texture *               defaultTexture;
    Texture *               zeroClampTexture;
    Texture *               defaultCubeMapTexture;
    Texture *               blackCubeMapTexture;
    Texture *               whiteTexture;
    Texture *               blackTexture;
    Texture *               greyTexture;
    Texture *               exponentTexture;
    Texture *               flatNormalTexture;
    Texture *               normalCubeMapTexture;
    Texture *               fogTexture;
    Texture *               fogEnterTexture;
    Texture *               cubicNormalCubeMapTexture;
    Texture *               randomRotMatTexture;
    Texture *               randomDir4x4Texture;

    static CVar             texture_filter;
    static CVar             texture_anisotropy;
    static CVar             texture_lodBias;
    static CVar             texture_useCompression;
    static CVar             texture_useNormalCompression;
    static CVar             texture_mipLevel;

private:
    void                    CreateEngineTextures();
    int                     LoadTextureInfo(const char *filename) const;

    static void             Cmd_ListTextures(const CmdArgs &args);
    static void             Cmd_ReloadTexture(const CmdArgs &args);
    static void             Cmd_DumpTexture(const CmdArgs &args);
    static void             Cmd_ConvertNormalAR2RGB(const CmdArgs &args);

    friend void             RB_DrawDebugTextures();

    StrIHashMap<Texture *>  textureHashMap;

    RHI::TextureFilter::Enum textureFilter;
    int                     textureAnisotropy;
};

extern TextureManager       textureManager;

BE_NAMESPACE_END
