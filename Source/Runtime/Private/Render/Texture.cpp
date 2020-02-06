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
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

static constexpr char   cubemap_postfix[6][3] = { "px", "nx", "py", "ny", "pz", "nz" };
static constexpr char   camera_cubemap_postfix[6][8] = { "rt", "lf", "up", "dn", "ft", "bk" };

bool Texture::IsDefaultTexture() const {
    return this == textureManager.defaultTexture;
}

int Texture::MemRequired(bool includingMipmaps) const {
    int numMipmaps = includingMipmaps ? Image::MaxMipMapLevels(width, height, depth) : 1;
    int size = Image::MemRequired(width, height, depth, numMipmaps, format);
    return size;
}

void Texture::Create(RHI::TextureType::Enum type, const Image &srcImage, int flags) {
    Purge();

    this->type = type;
    this->textureHandle = rhi.CreateTexture(type);
    this->flags = flags;

    /*if (flags & HighPriority) {
        rhi.PrioritizeTexture(texture, 1.0f);
    } else if (flags & LowPriority) {
        rhi.PrioritizeTexture(texture, 0.0f);
    }*/

    Upload(&srcImage);
}

void Texture::CreateEmpty(RHI::TextureType::Enum type, int width, int height, int depth, int numSlices, int numMipmaps, Image::Format::Enum format, int flags) {
    Purge();

    this->type = type;
    this->textureHandle = rhi.CreateTexture(type);
    this->flags = flags;

    Image image;
    Image::GammaSpace::Enum gammaSpace = (flags & Texture::Flag::SRGBColorSpace) ? Image::GammaSpace::sRGB : Image::GammaSpace::Linear;
    image.InitFromMemory(width, height, depth, type == RHI::TextureType::TextureCubeMap ? 6 : numSlices, numMipmaps, format, gammaSpace, nullptr, 0);
    Upload(&image);
}

void Texture::CreateFromBuffer(Image::Format::Enum format, RHI::Handle bufferHandle) {
    Purge();

    this->type = RHI::TextureType::TextureBuffer;
    this->textureHandle = rhi.CreateTexture(RHI::TextureType::TextureBuffer);
    this->flags = 0;
    this->format = format;

    rhi.BindBuffer(RHI::BufferType::Texel, bufferHandle);

    rhi.SelectTextureUnit(0);
    rhi.BindTexture(textureHandle);
    rhi.SetTextureImageBuffer(format, false, bufferHandle);

    rhi.BindBuffer(RHI::BufferType::Texel, RHI::NullBuffer);
}

// Indirection cubemap : Converts cubic coords to VCM coords
void Texture::CreateIndirectionCubemap(int size, int vcmWidth, int vcmHeight, int flags) {
    Image cubeImage;
    cubeImage.CreateCube(size, 1, Image::Format::LA_16F_16F, Image::GammaSpace::Linear, nullptr, 0);

    float16_t *dstPtr = (float16_t *)cubeImage.GetPixels();

    int vcmFaceWidth = vcmWidth / 3;
    int vcmFaceHeight = vcmHeight / 2;

    // VCM layout
    // ----------------
    // | PX | PY | PZ |
    // |----+----+----| vcmHeight
    // | NX | NY | NZ |
    // ----------------
    //     vcmWidth

    for (int i = RHI::CubeMapFace::PositiveX; i <= RHI::CubeMapFace::NegativeZ; i++) {
        for (int y = 0; y < size; y++) {
            Vec2 vcmSt;
            vcmSt.y = ((float)y / (float)(size - 1)) * vcmFaceHeight;
            vcmSt.y = (vcmSt.y + vcmFaceHeight * (i & 1)) / vcmHeight;

            for (int x = 0; x < size; x++) {
                vcmSt.x = ((float)x / (float)(size - 1)) * vcmFaceWidth;
                vcmSt.x = (vcmSt.x + vcmFaceWidth * (i >> 1)) / vcmWidth;

                *dstPtr++ = F16Converter::FromF32(vcmSt.x);
                *dstPtr++ = F16Converter::FromF32(vcmSt.y);
            }
        }
    }

    Create(RHI::TextureType::TextureCubeMap, cubeImage, Flag::Clamp | Flag::NoMipmaps | Flag::HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    default map

-------------------------------------------------------------------------------
*/

void Texture::CreateDefaultTexture(int size, int flags) {
    Image image;
    image.Create2D(size, size, 1, Image::Format::RGB_8_8_8, Image::GammaSpace::sRGB, nullptr, 0);
    byte *dst = image.GetPixels();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (x == 0 || x == size - 1 || y == 0 || y == size - 1) {
                dst[3 * (y*size + x) + 0] = (byte)(0);
                dst[3 * (y*size + x) + 1] = (byte)(0);
                dst[3 * (y*size + x) + 2] = (byte)(0);
            } else {
                dst[3 * (y*size + x) + 0] = (byte)(255);
                dst[3 * (y*size + x) + 1] = (byte)(0);
                dst[3 * (y*size + x) + 2] = (byte)(255);
            }
        }
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    zero clamp map

-------------------------------------------------------------------------------
*/

void Texture::CreateZeroClampTexture(int size, int flags) {
    Image image;
    image.Create2D(size, size, 1, Image::Format::LA_8_8, Image::GammaSpace::Linear, nullptr, 0);
    byte *dst = image.GetPixels();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            byte c = (byte)((x == 0 || x == size - 1 || y == 0 || y == size - 1) ? 0 : 255);
            dst[(y*size + x) * 2 + 0] = c;
            dst[(y*size + x) * 2 + 1] = c;
        }
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::ZeroClamp | Texture::Flag::HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    flat normal map

-------------------------------------------------------------------------------
*/

void Texture::CreateFlatNormalTexture(int size, int flags) {
    Image image;
    image.Create2D(size, size, 1, Image::Format::RGB_8_8_8, Image::GammaSpace::Linear, nullptr, 0);
    byte *dst = image.GetPixels();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            dst[(y*size + x) * 3 + 0] = 127;
            dst[(y*size + x) * 3 + 1] = 127;
            dst[(y*size + x) * 3 + 2] = 255;
        }
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::NormalMap | Texture::Flag::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    default cube map

-------------------------------------------------------------------------------
*/

void Texture::CreateDefaultCubeMapTexture(int size, int flags) {
    Image image;
    image.CreateCube(size, 1, Image::Format::L_8, Image::GammaSpace::sRGB, nullptr, 0);
    byte *dst = image.GetPixels();

    int faceSize = image.GetSliceSize();

    for (int i = 0; i < 6; i++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                dst[y*size + x] = (byte)((x == 0 || x == size - 1 || y == 0 || y == size - 1) ? 255 : 0);
            }
        }

        dst += faceSize;
    }

    Create(RHI::TextureType::TextureCubeMap, image, Texture::Flag::Clamp | Texture::Flag::NoCompression | Texture::Flag::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    black cube map

-------------------------------------------------------------------------------
*/

void Texture::CreateBlackCubeMapTexture(int size, int flags) {
    Image image;
    image.CreateCube(size, 1, Image::Format::L_8, Image::GammaSpace::sRGB, nullptr, 0);
    byte *dst = image.GetPixels();

    int facesize = image.GetSliceSize();

    for (int i = 0; i < 6; i++) {
        memset(dst, 0, facesize);
        dst += facesize;
    }

    Create(RHI::TextureType::TextureCubeMap, image, Texture::Flag::Clamp | Texture::Flag::NoCompression | Texture::Flag::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    Normalization cube map

-------------------------------------------------------------------------------
*/

void Texture::CreateNormalizationCubeMapTexture(int size, int flags) {
    Image image;
    image.CreateCube(size, 1, Image::Format::RGB_8_8_8, Image::GammaSpace::Linear, nullptr, 0);
    byte *dst = image.GetPixels();

    int sliceSize = image.GetSliceSize();
    float invSize = 1.0f / (size - 1);
    Vec3 dir;

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float s = (x + 0.5f) * invSize;
                float t = (y + 0.5f) * invSize;

                dir = Image::FaceToCubeMapCoords((Image::CubeMapFace::Enum)faceIndex, s, t);
                dir.Normalize();

                // Convert cubemap coordinates from z-up to GL axis
                dir = Vec3(dir.y, dir.z, dir.x);

                dst[3 * (y * size + x) + 0] = Math::Ftob(dir.x * 127.5f + 128.0f);
                dst[3 * (y * size + x) + 1] = Math::Ftob(dir.y * 127.5f + 128.0f);
                dst[3 * (y * size + x) + 2] = Math::Ftob(dir.z * 127.5f + 128.0f);
            }
        }

        dst += sliceSize;
    }

    Create(RHI::TextureType::TextureCubeMap, image, Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::NoCompression | Texture::Flag::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    Cubic normal cube map

-------------------------------------------------------------------------------
*/

void Texture::CreateCubicNormalCubeMapTexture(int size, int flags) {
    int8_t faceNormal[6][3];

    faceNormal[0][0] = 127;
    faceNormal[0][1] = 0;
    faceNormal[0][2] = 0;

    faceNormal[1][0] = -128;
    faceNormal[1][1] = 0;
    faceNormal[1][2] = 0;

    faceNormal[2][0] = 0;
    faceNormal[2][1] = 127;
    faceNormal[2][2] = 0;

    faceNormal[3][0] = 0;
    faceNormal[3][1] = -128;
    faceNormal[3][2] = 0;

    faceNormal[4][0] = 0;
    faceNormal[4][1] = 0;
    faceNormal[4][2] = 127;

    faceNormal[5][0] = 0;
    faceNormal[5][1] = 0;
    faceNormal[5][2] = -128;

    Image image;
    image.CreateCube(size, 1, Image::Format::RGB_8_8_8_SNORM, Image::GammaSpace::Linear, nullptr, 0);
    int8_t *dst = (int8_t *)image.GetPixels();

    int facesize = image.GetSliceSize();

    for (int i = 0; i < 6; i++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                dst[3 * (y * size + x) + 0] = faceNormal[i][0];
                dst[3 * (y * size + x) + 1] = faceNormal[i][1];
                dst[3 * (y * size + x) + 2] = faceNormal[i][2];
            }
        }

        dst += facesize;
    }

    Create(RHI::TextureType::TextureCubeMap, image, Texture::Flag::Nearest | Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::NoCompression | Texture::Flag::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    Attenuation map

-------------------------------------------------------------------------------
*/

void Texture::CreateAttenuationTexture(int size, int flags) {
    Image image;
    image.Create2D(size, size, 1, Image::Format::LA_8_8, Image::GammaSpace::Linear, nullptr, 0);
    byte *dst = image.GetPixels();

    float centerx = size / 2.0;
    float centery = size / 2.0;
    float radius_squared = Square(size / 2.0);

    for (int s = 0; s < size; s++) {
        for (int t = 0; t < size; t++) {
            float dist_squared = Square(s - centerx) + Square(t - centery);
            if (dist_squared < radius_squared) {
                float falloff = 1.0f - dist_squared / radius_squared;
                byte value = Square(falloff) * 255.0;
                dst[(t * size + s) * 2] = value;
                dst[(t * size + s) * 2 + 1] = value;
            } else {
                dst[(t * size + s) * 2] = 0;
                dst[(t * size + s) * 2 + 1] = 0;
            }
        }
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::Clamp | Texture::Flag::NoMipmaps | Texture::Flag::NoCompression | Texture::Flag::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    Fog map

-------------------------------------------------------------------------------
*/

void Texture::CreateFogTexture(int flags) {
    Image image;
    image.Create2D(256, 1, 1, Image::Format::A_8, Image::GammaSpace::Linear, nullptr, 0);
    byte *dst = image.GetPixels();

    for (int s = 0; s < 256; s++) {
        int c = sqrt(s / 255.0) * 255;
        dst[s] = Min(c, 255);
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::NoMipmaps | Texture::Flag::Clamp | Texture::Flag::HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    Fog enter map

-------------------------------------------------------------------------------
*/

void Texture::CreateFogEnterTexture(int flags) {
    Image image;
    image.Create2D(256, 1, 1, Image::Format::A_8, Image::GammaSpace::Linear, nullptr, 0);
    byte *dst = image.GetPixels();

    for (int s = 0; s < 256; s++) {
        int c = sqrt(s / 255.0) * 255;
        dst[s] = Min(c, 255);
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::NoMipmaps | Texture::Flag::Clamp | Texture::Flag::HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    Random rotation matrix map

-------------------------------------------------------------------------------
*/

void Texture::CreateRandomRotMatTexture(int size, int flags) {
    Image image;
    image.Create2D(size, size, 1, Image::Format::RGBA_8_8_8_8, Image::GammaSpace::Linear, nullptr, 0);
    byte *dst = image.GetPixels();
    float s, c;

    Random random;
    random.SetSeed(23119);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            Math::SinCos(random.RandomFloat() * Math::TwoPi, s, c);

            dst[4 * (y * 64 + x) + 0] = ClampByte((+c + 1.0f) * 0.5f * 255 + 0.5f);
            dst[4 * (y * 64 + x) + 1] = ClampByte((-s + 1.0f) * 0.5f * 255 + 0.5f);
            dst[4 * (y * 64 + x) + 2] = ClampByte((+s + 1.0f) * 0.5f * 255 + 0.5f);
            dst[4 * (y * 64 + x) + 3] = ClampByte((+c + 1.0f) * 0.5f * 255 + 0.5f);
        }
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::Nearest | Texture::Flag::Repeat | Texture::Flag::NoMipmaps | Texture::Flag::NoCompression | Texture::Flag::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    Random direction map for SSAO

-------------------------------------------------------------------------------
*/

void Texture::CreateRandomDir4x4Texture(int flags) {
    Image image;
    image.Create2D(4, 4, 1, Image::Format::RGB_8_8_8, Image::GammaSpace::Linear, nullptr, 0);
    byte *dst = image.GetPixels();
    Vec3 dir;

    Random random;
    random.SetSeed(810202);

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            dir.x = random.RandomFloat();
            dir.y = random.RandomFloat();
            dir.z = random.RandomFloat();
            dir.Normalize();

            dir = (dir + Vec3(1.0f, 1.0f, 1.0f)) * 0.5f;

            dst[3 * (y * 4 + x) + 0] = ClampByte(dir.x * 255.0f + 0.5f);
            dst[3 * (y * 4 + x) + 1] = ClampByte(dir.y * 255.0f + 0.5f);
            dst[3 * (y * 4 + x) + 2] = ClampByte(dir.z * 255.0f + 0.5f);
        }
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::Nearest | Texture::Flag::Repeat | Texture::Flag::NoMipmaps | Texture::Flag::NoCompression | Texture::Flag::NoScaleDown | flags);
}

static RHI::AddressMode::Enum TextureFlagsToAddressMode(int flags) {
    if (flags & (Texture::Flag::ClampToBorder | Texture::Flag::ZeroClamp)) {
        return RHI::AddressMode::ClampToBorder;
    } else if (flags & Texture::Flag::Clamp) {
        return RHI::AddressMode::Clamp;
    } else if (flags & Texture::Flag::MirroredRepeat) {
        return RHI::AddressMode::MirroredRepeat;
    } else {
        return RHI::AddressMode::Repeat;
    }
}

void Texture::Upload(const Image *srcImage) {
    Image::Format::Enum srcFormat = srcImage->GetFormat();
    Image::Format::Enum forceFormat = Image::Format::Unknown;
    Image tmpImage;

    if (type == RHI::TextureType::TextureRectangle) {
        flags |= (Flag::NoMipmaps | Flag::Clamp | Flag::NoScaleDown);
    }

    if (flags & Flag::NonPowerOfTwo) {
        flags |= Flag::NoMipmaps;
    }

    if (srcImage->IsEmpty()) {
        flags |= (Flag::NoScaleDown | Flag::NoCompression);
        forceFormat = srcImage->GetFormat();
    }

    bool useNormalMap = (flags & Flag::NormalMap) ? true : false;
    bool useCompression = !(flags & Flag::NoCompression) ? TextureManager::texture_useCompression.GetBool() : false;
    bool useNPOT = (flags & Flag::NonPowerOfTwo) ? true : false;
    bool isSRGB = ((flags & Flag::SRGBColorSpace) && r_sRGB.GetBool()) ? true : false;

    Image::Format::Enum dstFormat;
    if (forceFormat != Image::Format::Unknown) {
        dstFormat = forceFormat;
    } else {
        rhi.AdjustTextureFormat(type, useCompression, useNormalMap, srcImage->GetFormat(), &dstFormat);
    }

    this->srcWidth = srcImage->GetWidth();
    this->srcHeight = srcImage->GetHeight();
    this->srcDepth = srcImage->GetDepth();
    this->numSlices = srcImage->NumSlices();

    int dstWidth, dstHeight, dstDepth;
    rhi.AdjustTextureSize(type, useNPOT, srcWidth, srcHeight, srcDepth, &dstWidth, &dstHeight, &dstDepth);

    // Apply scale down mip level
    int mipLevel = !(flags & Flag::NoScaleDown) ? TextureManager::texture_mipLevel.GetInteger() : 0;
    if (mipLevel > 0) {
        dstWidth = Max(dstWidth >> mipLevel, 1);
        dstHeight = Max(dstHeight >> mipLevel, 1);
        dstDepth = Max(dstDepth >> mipLevel, 1);
    }

    // Can't upload texels for depth texture
    if (!srcImage->IsEmpty() && Image::IsDepthFormat(srcImage->GetFormat())) {
        BE_FATALERROR("Texture::Upload: Couldn't upload texel data of depth texture");
    }

    Image scaledImage;

    if (!srcImage->IsEmpty()) {
        if (srcWidth != dstWidth || srcHeight != dstHeight) {
            srcImage->Resize(dstWidth, dstHeight, Image::ResampleFilter::Bicubic, scaledImage);
            srcImage = &scaledImage;
        }
    }

    this->format = dstFormat;
    this->width = dstWidth;
    this->height = dstHeight;
    this->depth = dstDepth;

    this->hasMipmaps = !(flags & Flag::NoMipmaps);
    this->permanence = !!(flags & Flag::Permanence);
    this->addressMode = TextureFlagsToAddressMode(flags);

    rhi.BindTexture(textureHandle);

    if (flags & Flag::ZeroClamp) {
        rhi.SetTextureBorderColor(Color4(0.0f, 0.0f, 0.0f, 0.0f));
    }

    if (flags & Flag::Shadow) {
        rhi.SetTextureBorderColor(Color4(1.0f, 1.0f, 1.0f, 1.0f));
        rhi.SetTextureShadowFunc(true);
    }

    rhi.SetTextureImage(type, srcImage, dstFormat, hasMipmaps, isSRGB);

    rhi.SetTextureAddressMode(addressMode);

    if (hasMipmaps) {
        rhi.SetTextureFilter((flags & Flag::Nearest) ? RHI::TextureFilter::NearestMipmapNearest : ((flags & Flag::Trilinear) ? RHI::TextureFilter::LinearMipmapLinear : textureManager.textureFilter));
    } else {
        rhi.SetTextureFilter((flags & Flag::Nearest) ? RHI::TextureFilter::Nearest : RHI::TextureFilter::Linear);
    }

    if (hasMipmaps && !(flags & Flag::Nearest)) {
        rhi.SetTextureAnisotropy(textureManager.textureAnisotropy);
    }
}

void Texture::Update2D(int mipLevel, int xoffset, int yoffset, int width, int height, Image::Format::Enum format, const byte *data) {
    rhi.SetTextureSubImage2D(mipLevel, xoffset, yoffset, width, height, format, data);
}

void Texture::Update3D(int mipLevel, int xoffset, int yoffset, int zoffset, int width, int height, int depth, Image::Format::Enum format, const byte *data) {
    rhi.SetTextureSubImage3D(0, xoffset, yoffset, zoffset, width, height, depth, format, data);
}

void Texture::UpdateCubemap(int face, int mipLevel, int xoffset, int yoffset, int width, int height, Image::Format::Enum format, const byte *data) {
    rhi.SetTextureSubImageCube((RHI::CubeMapFace::Enum)face, mipLevel, xoffset, yoffset, width, height, format, data);
}

void Texture::UpdateRect(int xoffset, int yoffset, int width, int height, Image::Format::Enum format, const byte *data) {
    rhi.SetTextureSubImageRect(xoffset, yoffset, width, height, format, data);
}

void Texture::GetTexels2D(int mipLevel, Image::Format::Enum format, void *pixels) const {
    rhi.GetTextureImage2D(mipLevel, format, pixels);
}

void Texture::GetTexels3D(int mipLevel, Image::Format::Enum format, void *pixels) const {
    rhi.GetTextureImage3D(mipLevel, format, pixels);
}

void Texture::GetTexelsCubemap(int face, int mipLevel, Image::Format::Enum format, void *pixels) const {
    rhi.GetTextureImageCube((RHI::CubeMapFace::Enum)face, mipLevel, format, pixels);
}

void Texture::GetTexelsRect(Image::Format::Enum format, void *pixels) const {
    rhi.GetTextureImageRect(format, pixels);
}

void Texture::CopyTo(int mipLevel, Texture *dstTexture) {
    rhi.CopyImageSubData(textureHandle, mipLevel, 0, 0, 0, dstTexture->textureHandle, mipLevel, 0, 0, 0, width, height, type == RHI::TextureType::TextureCubeMap ? numSlices : depth);
}

void Texture::Purge() {
    if (textureHandle != RHI::NullTexture) {
        rhi.DestroyTexture(textureHandle);

        // Invalidates render target which is linked with this texture.
        renderTarget = nullptr;
    }

    textureHandle = RHI::NullTexture;
}

bool Texture::Load(const char *filename, int flags) {
    flags |= Flag::LoadedFromFile;

    if (flags & (Flag::CubeMap | Flag::CameraCubeMap)) {
        Str name = filename;
        name.StripFileExtension();
        Image images[6];

        for (int i = 0; i < 6; i++) {
            Str filename2 = name + "_" + ((flags & Flag::CameraCubeMap) ? camera_cubemap_postfix[i] : cubemap_postfix[i]);
            BE_LOG("Loading texture '%s'...\n", filename2.c_str());

            images[i].Load(filename2.c_str());

            if (images[i].IsEmpty()) {
                BE_WARNLOG("Couldn't load texture \"%s\"\n", filename2.c_str());
                return false;
            }
        }

        Image cubeImage;
        cubeImage.CreateCubeFrom6Faces(images);
        Create(RHI::TextureType::TextureCubeMap, cubeImage, flags);
    } else {
        BE_LOG("Loading texture '%s'...\n", filename);

        Image image;
        image.Load(filename);

        if (image.IsEmpty()) {
            BE_WARNLOG("Couldn't load texture \"%s\"\n", filename);
            return false;
        }

        RHI::TextureType::Enum textureType;

        if (image.GetDepth() > 1) {
            textureType = RHI::TextureType::Texture3D;
        } else if (image.IsCubeMap()) {
            textureType = RHI::TextureType::TextureCubeMap;
        } else {
            textureType = RHI::TextureType::Texture2D;
        }

        Create(textureType, image, flags);
    }

    return true;
}

bool Texture::Reload() {
    if (!(flags & Flag::LoadedFromFile)) {
        return false;
    }

    Str _hashName = this->hashName;

    const Str textureInfoPath = _hashName + ".texture";
    int _flags = textureManager.LoadTextureInfo(textureInfoPath);

    bool ret = Load(_hashName, _flags);

    return ret;
}

void Texture::Bind() const {
    rhi.BindTexture(textureHandle);
}

void Texture::GenerateMipmap() const {
    rhi.GenerateMipmap();
}

void Texture::GetCubeImageFromCubeTexture(const Texture *cubeTexture, int numMipLevels, Image &cubeImage) {
    Image faceImages[6];

    Image::GammaSpace::Enum gammaSpace = Image::IsFloatFormat(cubeTexture->GetFormat()) ? Image::GammaSpace::Linear : Image::GammaSpace::sRGB;

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        faceImages[faceIndex].Create2D(cubeTexture->GetWidth(), cubeTexture->GetWidth(), numMipLevels, cubeTexture->GetFormat(), gammaSpace, nullptr, 0);

        cubeTexture->Bind();

        for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
            cubeTexture->GetTexelsCubemap(faceIndex, mipLevel, cubeTexture->GetFormat(), faceImages[faceIndex].GetPixels(mipLevel));
        }
    }

    cubeImage.CreateCubeFrom6Faces(faceImages);
}

//--------------------------------------------------------------------------------------------------

void TextureGenerator_CubicDiffuseIrradianceMap::Generate(Texture *texture) const {
    if (texture->GetType() != RHI::TextureType::TextureCubeMap) {
        BE_ERRLOG("TextureGenerator_CubicDiffuseIrradianceMap::Generate: src texture must be cube map\n");
        return;
    }
}

void TextureGenerator_CubicSpecularIrradianceMap::Generate(Texture *texture) const {
    if (texture->GetType() != RHI::TextureType::TextureCubeMap) {
        BE_ERRLOG("TextureGenerator_CubicSpecularIrradianceMap::Generate: src texture must be cube map\n");
        return;
    }
}

BE_NAMESPACE_END
