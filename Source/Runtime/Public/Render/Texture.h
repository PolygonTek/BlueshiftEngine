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

class Texture {
    friend class TextureManager;
    friend class RenderTarget;
    friend class Shader;

public:
    enum Flag {
        Repeat              = BIT(0),
        MirroredRepeat      = BIT(1),
        Clamp               = BIT(2),       ///< set wrap mode to Clamp (can be set with Mirror)
        ClampToBorder       = BIT(3),       ///< set wrap mode to ClampToBorder (can be set with Mirror)
        ZeroClamp           = BIT(4),       ///< set wrap mode to ClampToBorder and set border color to black
        Shadow              = BIT(5),       ///< set shadow func (shader 에서 samperXDShadow 를 사용하려면 이 플래그를 켜야한다)
        Nearest             = BIT(6),       ///< use nearest filtering (should be set with NoMipmaps)
        NoMipmaps           = BIT(7),       ///< don't use mipmaps
        NoScaleDown         = BIT(8),       ///< texture_mip 값으로 사이즈를 줄이기를 하지 않는다
        NoCompression       = BIT(9),       ///< texture_useCompression 값으로 압축 텍스쳐 사용하지 않는다 (format이 원래 압축 이미지일 경우는 소용없다)
        NonPowerOfTwo       = BIT(10),      ///< non power of two texture
        NormalMap           = BIT(11),      ///< this is normal map !
        HighPriority        = BIT(12),
        LowPriority         = BIT(13),
        SRGB                = BIT(14),      ///< generally color image is encoded in sRGB
        Trilinear           = BIT(15),
        HighQuality         = NoScaleDown | NoCompression,
        CubeMap             = BIT(28),      ///< cube map
        CameraCubeMap       = BIT(29),
        LoadedFromFile      = BIT(30),      ///< mark it is loaded from the file
        Permanence          = BIT(31)
    };

    Texture();
    ~Texture();

    const char *            GetHashName() const { return hashName; }
    int                     GetType() const { return type; }
    int                     GetWidth() const { return width; }
    int                     GetHeight() const { return height; }
    int                     GetDepth() const { return depth; }
    int                     NumSlices() const { return numSlices; }
    Image::Format           GetFormat() const { return format; }
    int                     GetFlags() const { return flags; }

    int                     MemRequired(bool includingMipmaps) const;

    void                    Create(RHI::TextureType type, const Image &srcImage, int flags);
    void                    CreateEmpty(RHI::TextureType type, int width, int height, int depth, int numSlices, int numMipmaps, Image::Format format, int flags);
    void                    CreateFromBuffer(Image::Format format, RHI::Handle bufferHandle);

                            /// Create indirection cubemap
                            /// @param size size of indirection cubemap
                            /// @param vcmWidth actual height of virtual cubemap texture
                            /// @param vcmHeight actual width of virtual cubemap texture
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

    void                    Update2D(int xoffset, int yoffset, int width, int height, Image::Format format, const byte *data);
    void                    Update3D(int xoffset, int yoffset, int zoffset, int width, int height, int depth, Image::Format format, const byte *data);
    void                    UpdateCubemap(int face, int xoffset, int yoffset, int width, int height, Image::Format format, const byte *data);
    void                    UpdateRect(int xoffset, int yoffset, int width, int height, Image::Format format, const byte *data);

    void                    GetTexels2D(Image::Format format, void *pixels) const;
    void                    GetTexels3D(Image::Format format, void *pixels) const;
    void                    GetTexelsCubemap(int face, Image::Format format, void *pixels) const;
    void                    GetTexelsRect(Image::Format format, void *pixels) const;

    void                    Purge();

    bool                    Load(const char *filename, int flags);
    bool                    Reload();

    const Texture *         AddRefCount() const { refCount++; return this; }

    void                    Bind() const;

private:
    Str                     hashName;                   // texture filename including path
    Str                     name;
    mutable int             refCount;                   // reference count
    bool                    permanence;                 //
    int                     frameCount;
    int                     flags;                      // texture load flags

    RHI::Handle             textureHandle;              // texture handle
    RHI::TextureType        type;
    RHI::AddressMode        addressMode;

    Image::Format           format;                     // internal image format

    int                     srcWidth;                   // original width
    int                     srcHeight;                  // original height
    int                     srcDepth;                   // original depth
    int                     numSlices;

    int                     width;                      // scaled width
    int                     height;                     // scaled height
    int                     depth;                      // scaled depth

    bool                    hasMipmaps;
};

BE_INLINE Texture::Texture() {
    refCount                = 0;
    permanence              = false;
    flags                   = 0;
    textureHandle           = RHI::Handle::NullTexture;
    type                    = RHI::TextureType::Texture2D;
    addressMode             = RHI::AddressMode::Repeat;
    format                  = Image::Format::UnknownFormat;
    srcWidth                = 0;
    srcHeight               = 0;
    srcDepth                = 0;
    numSlices               = 0;
    width                   = 0;
    height                  = 0;
    depth                   = 0;
    hasMipmaps              = false;
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
    Texture *               GetTexture(const char *name, int creationFlags = 0);

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
    Texture *               linearTexture;
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
    static CVar             texture_sRGB;
    static CVar             texture_useCompression;
    static CVar             texture_useNormalCompression;
    static CVar             texture_mipLevel;

private:
    int                     LoadTextureInfo(const char *filename) const;

    static void             Cmd_ListTextures(const CmdArgs &args);
    static void             Cmd_ReloadTexture(const CmdArgs &args);
    static void             Cmd_ConvertNormalAR2RGB(const CmdArgs &args);

    friend void             RB_DrawDebugTextures();

    StrIHashMap<Texture *>  textureHashMap;

    RHI::TextureFilter      textureFilter;
    int                     textureAnisotropy;
};

extern TextureManager       textureManager;

BE_NAMESPACE_END
