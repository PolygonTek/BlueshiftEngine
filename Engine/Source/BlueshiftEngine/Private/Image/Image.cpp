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
#include "Core/Heap.h"
#include "Simd/Simd.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

Image &Image::InitFromMemory(int width, int height, int depth, int numSlices, int numMipmaps, Image::Format format, byte *data, int flags) {
    Clear();

    this->width = width;
    this->height = height;
    this->depth = depth;
    this->numSlices = Max(numSlices, 1);
    this->numMipmaps = Max(numMipmaps, 1);
    this->format = format;
    this->flags = flags;
    this->alloced = false;
    this->pic = data;

    return *this;
}

Image &Image::Create(int width, int height, int depth, int numSlices, int numMipmaps, Image::Format format, const byte *data, int flags) {
    Clear();

    this->width = width;
    this->height = height;
    this->depth = depth;
    this->numSlices = Max(numSlices, 1);
    this->numMipmaps = Max(numMipmaps, 1);
    this->format = format;
    this->flags = flags;
    
    int size = GetSize(0, numMipmaps);
    this->pic = (byte *)Mem_Alloc16(size);
    this->alloced = true;
    
    if (data) {
        simdProcessor->Memcpy(this->pic, data, size);
    }

    return *this;
}

Image &Image::CreateCubeFrom6Faces(const Image *images) {
    Clear();

    this->width = images[0].width;
    this->height = this->width;
    this->depth = 1;
    this->numSlices = 6;
    this->numMipmaps = images[0].numMipmaps;
    this->format = images[0].format;
    this->flags = images[0].flags | Flag::CubeMapFlag;
    
    int sliceSize = GetSliceSize(0, numMipmaps);
    this->pic = (byte *)Mem_Alloc16(sliceSize * 6);
    this->alloced = true;
    
    byte *dst = this->pic;
    
    for (int i = 0; i < 6; i++) {
        assert(images[i].width == images[i].height);
        assert(images[i].width == images[0].width);
        assert(images[i].format == images[0].format);
        assert(images[i].numMipmaps == images[0].numMipmaps);

        simdProcessor->Memcpy(dst, images[i].pic, sliceSize);
        dst += sliceSize;
    }

    return *this;
}

Image &Image::CreateCubeFromEquirectangular(const Image &equirectangularImage, int faceSize) {
    Clear();

    this->width = faceSize;
    this->height = this->width;
    this->depth = 1;
    this->numSlices = 6;
    this->numMipmaps = 1;
    this->format = equirectangularImage.format;
    this->flags = equirectangularImage.flags | Flag::CubeMapFlag;

    int sliceSize = GetSliceSize(0, numMipmaps);
    this->pic = (byte *)Mem_Alloc16(sliceSize * 6);
    this->alloced = true;

    const ImageFormatInfo *info = GetImageFormatInfo(format);

    float invSize = 1.0f / (faceSize - 1);

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        byte *dst = GetPixels(0, faceIndex);

        for (int dstY = 0; dstY < faceSize; dstY++) {
            for (int dstX = 0; dstX < faceSize; dstX++) {
                float dstS = (dstX + 0.5f) * invSize;
                float dstT = (dstY + 0.5f) * invSize;

                Vec3 dir = FaceToCubeMapCoords((Image::CubeMapFace)faceIndex, dstS, dstT);

                float theta, phi;
                dir.ToSpherical(theta, phi);

                // Environment equirectangluar image has reversed phi
                phi = Math::TwoPi - phi;

                // Convert range [-1/4 pi, 7/4 pi] to [0.0, 1.0]
                float srcS = Math::Fract((phi + Math::OneFourthPi) * Math::InvTwoPi);
                // Convert range [0, pi] to [0.0, 1.0]
                float srcT = Math::Fract(theta * Math::InvPi);

                Color4 color = equirectangularImage.Sample2D(Vec2(srcS, srcT), Image::SampleWrapMode::RepeatMode, Image::SampleWrapMode::ClampMode);
                info->packRGBA32F((const byte *)&color, &dst[(dstY * width + dstX) * BytesPerPixel()], 1);
            }
        }
    }

    return *this;
}

Image &Image::CreateEquirectangularFromCube(const Image &cubeImage) {
    Clear();

    this->width = cubeImage.width * 2;
    this->height = cubeImage.width;
    this->depth = 1;
    this->numSlices = 1;
    this->numMipmaps = 1;
    this->format = cubeImage.format;
    this->flags = cubeImage.flags & ~CubeMapFlag;

    int size = GetSize(0, numMipmaps);
    this->pic = (byte *)Mem_Alloc16(size);
    this->alloced = true;

    const ImageFormatInfo *info = GetImageFormatInfo(format);

    byte *dst = GetPixels(0);

    for (int dstY = 0; dstY < height; dstY++) {
        for (int dstX = 0; dstX < width; dstX++) {
            float dstS = (float)dstX / (width - 1);
            float dstT = (float)dstY / (height - 1);

            // Convert range [0.0, 1.0] to [0, pi]
            float theta = dstT * Math::Pi;
            // Convert range [0.0, 1.0] to [-1/4 pi, 7/4 pi]
            float phi = dstS * Math::TwoPi - Math::OneFourthPi;

            Vec3 dir;
            dir.SetFromSpherical(1.0f, theta, phi);

            Color4 color = cubeImage.SampleCube(dir);
            info->packRGBA32F((const byte *)&color, &dst[(dstY * width + dstX) * BytesPerPixel()], 1);
        }
    }

    return *this;
}

Image &Image::CopyFrom(const Image &srcImage, int firstLevel, int numLevels) {
    if (srcImage.GetWidth() != width || srcImage.GetHeight() != height || srcImage.GetDepth() != depth || srcImage.NumSlices() != numSlices) {
        return *this;
    }

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        byte *src = srcImage.GetPixels(firstLevel, sliceIndex);
        byte *dst = GetPixels(firstLevel, sliceIndex);
        int size = srcImage.GetSliceSize(firstLevel, numLevels);

        simdProcessor->Memcpy(dst, src, size);
    }

    return *this;
}

Image &Image::operator=(const Image &rhs) {
    Clear();
    Create(rhs.width, rhs.height, rhs.depth, rhs.numSlices, rhs.numMipmaps, rhs.format, rhs.pic, rhs.flags);
    return (*this);
}

Image &Image::operator=(Image &&rhs) {
    Clear();

    width = rhs.width;
    height = rhs.height;
    depth = rhs.depth;
    numSlices = rhs.numSlices;
    numMipmaps = rhs.numMipmaps;
    format = rhs.format;
    flags = rhs.flags;
    alloced = rhs.alloced;
    pic = rhs.pic;

    rhs.alloced = false;
    rhs.pic = nullptr;
    
    return (*this);
}

void Image::Clear() {
    if (alloced && pic) {
        Mem_AlignedFree(pic);
        pic = nullptr;
        alloced = false;
    }
}

Color4 Image::Sample2D(const Vec2 &st, SampleWrapMode wrapModeS, SampleWrapMode wrapModeT, int level) const {
    if (IsCompressed()) {
        assert(0);
        return Color4::zero;
    }

    const ImageFormatInfo *info = GetImageFormatInfo(format);
    int numComponents = NumComponents();
    int bpp = BytesPerPixel();
    int pitch = width * bpp;

    float x = WrapCoord(st[0] * width, (float)(width - 1), wrapModeS);
    int iX0 = (int)x;
    int iX1 = WrapCoord(iX0 + 1, width - 1, wrapModeS);
    float fracX = x - (float)iX0;

    float y = WrapCoord(st[1] * height, (float)(height - 1), wrapModeT);
    int iY0 = (int)y;
    int iY1 = WrapCoord(iY0 + 1, height - 1, wrapModeT);
    float fracY = y - (float)iY0;

    const byte *src = GetPixels(level);
    const byte *srcY0 = &src[iY0 * pitch];
    const byte *srcY1 = &src[iY1 * pitch];

    Color4 color = Color4(0, 0, 0, 1);

    if (info->type & Float) {
        // [0] [1]
        // [2] [3]
        ALIGN16(float rgba32f[4][4]);

        info->unpackRGBA32F(&srcY0[iX0 * bpp], (byte *)rgba32f[0], 1);
        info->unpackRGBA32F(&srcY0[iX1 * bpp], (byte *)rgba32f[1], 1);
        info->unpackRGBA32F(&srcY1[iX0 * bpp], (byte *)rgba32f[2], 1);
        info->unpackRGBA32F(&srcY1[iX1 * bpp], (byte *)rgba32f[3], 1);

        for (int i = 0; i < numComponents; i++) {
            float a = Lerp(rgba32f[0][i], rgba32f[1][i], fracX);
            float b = Lerp(rgba32f[2][i], rgba32f[3][i], fracX);

            color[i] = Lerp(a, b, fracY);
        }
    } else {
        // [0] [1]
        // [2] [3]
        ALIGN16(byte rgba8888[4][4]);

        info->unpackRGBA8888(&srcY0[iX0 * bpp], rgba8888[0], 1);
        info->unpackRGBA8888(&srcY0[iX1 * bpp], rgba8888[1], 1);
        info->unpackRGBA8888(&srcY1[iX0 * bpp], rgba8888[2], 1);
        info->unpackRGBA8888(&srcY1[iX1 * bpp], rgba8888[3], 1);

        for (int i = 0; i < numComponents; i++) {
            byte a = Lerp(rgba8888[0][i], rgba8888[1][i], fracX);
            byte b = Lerp(rgba8888[2][i], rgba8888[3][i], fracX);

            color[i] = Lerp(a, b, fracY) / 255.0f;
        }
    }

    return color;
}

Color4 Image::SampleCube(const Vec3 &str, int level) const {
    if (IsCompressed()) {
        assert(0);
        return Color4::zero;
    }
    
    const ImageFormatInfo *info = GetImageFormatInfo(format);
    int numComponents = NumComponents();
    int bpp = BytesPerPixel();
    int pitch = width * bpp;

    float x, y;
    CubeMapFace cubeMapFace = CubeMapToFaceCoords(str, x, y);
    x *= width;
    y *= height;

    int iX0 = (int)x;
    int iX1 = Min(iX0 + 1, width - 1);
    float fracX = x - (float)iX0;

    int iY0 = (int)y;
    int iY1 = Min(iY0 + 1, height - 1);
    float fracY = y - (float)iY0;

    const byte *src = GetPixels(level, cubeMapFace);
    const byte *srcY0 = &src[iY0 * pitch];
    const byte *srcY1 = &src[iY1 * pitch];

    Color4 color = Color4(0, 0, 0, 1);

    if (info->type & Float) {
        // [0] [1]
        // [2] [3]
        ALIGN16(float rgba32f[4][4]);

        info->unpackRGBA32F(&srcY0[iX0 * bpp], (byte *)rgba32f[0], 1);
        info->unpackRGBA32F(&srcY0[iX1 * bpp], (byte *)rgba32f[1], 1);
        info->unpackRGBA32F(&srcY1[iX0 * bpp], (byte *)rgba32f[2], 1);
        info->unpackRGBA32F(&srcY1[iX1 * bpp], (byte *)rgba32f[3], 1);

        for (int i = 0; i < numComponents; i++) {
            float a = Lerp(rgba32f[0][i], rgba32f[1][i], fracX);
            float b = Lerp(rgba32f[2][i], rgba32f[3][i], fracX);

            color[i] = Lerp(a, b, fracY);
        }
    } else {
        // [0] [1]
        // [2] [3]
        ALIGN16(byte rgba8888[4][4]);

        info->unpackRGBA8888(&srcY0[iX0 * bpp], rgba8888[0], 1);
        info->unpackRGBA8888(&srcY0[iX1 * bpp], rgba8888[1], 1);
        info->unpackRGBA8888(&srcY1[iX0 * bpp], rgba8888[2], 1);
        info->unpackRGBA8888(&srcY1[iX1 * bpp], rgba8888[3], 1);

        for (int i = 0; i < numComponents; i++) {
            byte a = Lerp(rgba8888[0][i], rgba8888[1][i], fracX);
            byte b = Lerp(rgba8888[2][i], rgba8888[3][i], fracX);

            color[i] = Lerp(a, b, fracY) / 255.0f;
        }
    }

    return color;
}

Vec3 Image::FaceToCubeMapCoords(CubeMapFace cubeMapFace, float s, float t) {
    float sc = s * 2.0f - 1.0f;
    float tc = t * 2.0f - 1.0f;

    Vec3 glCubeMapCoords;
    switch (cubeMapFace) {
    case PositiveX: glCubeMapCoords = Vec3(+1.0f, -tc, -sc); break; // +Y direction in z-up axis
    case NegativeX: glCubeMapCoords = Vec3(-1.0f, -tc, +sc); break; // -Y direction in z-up axis
    case PositiveY: glCubeMapCoords = Vec3(+sc, +1.0f, +tc); break; // +Z direction in z-up axis
    case NegativeY: glCubeMapCoords = Vec3(+sc, -1.0f, -tc); break; // -Z direction in z-up axis
    case PositiveZ: glCubeMapCoords = Vec3(+sc, -tc, +1.0f); break; // +X direction in z-up axis
    case NegativeZ: glCubeMapCoords = Vec3(-sc, -tc, -1.0f); break; // -X direction in z-up axis
    }
    // Convert cubemap coordinates from GL axis to z-up axis
    return Vec3(glCubeMapCoords.z, glCubeMapCoords.x, glCubeMapCoords.y);
}

Image::CubeMapFace Image::CubeMapToFaceCoords(const Vec3 &cubeMapCoords, float &s, float &t) {
    // Convert cubemap coordinates from z-up axis to GL axis
    Vec3 glCubeMapCoords = Vec3(cubeMapCoords.y, cubeMapCoords.z, cubeMapCoords.x);

    int faceIndex = glCubeMapCoords.Abs().MaxComponentIndex();
    float majorAxis = glCubeMapCoords[faceIndex];
    faceIndex = (faceIndex << 1) + IEEE_FLT_SIGNBITSET(majorAxis);
    float sc, tc;

    switch (faceIndex) {
    case PositiveX: 
        sc = -glCubeMapCoords.z;
        tc = -glCubeMapCoords.y;
        break;
    case NegativeX:
        sc = +glCubeMapCoords.z;
        tc = -glCubeMapCoords.y;
        break;
    case PositiveY:
        sc = +glCubeMapCoords.x;
        tc = +glCubeMapCoords.z;
        break;
    case NegativeY:
        sc = +glCubeMapCoords.x;
        tc = -glCubeMapCoords.z;
        break;
    case PositiveZ:
        sc = +glCubeMapCoords.x;
        tc = -glCubeMapCoords.y;
        break;
    case NegativeZ:
        sc = -glCubeMapCoords.x;
        tc = -glCubeMapCoords.y;
        break;
    }

    float ama = Math::Fabs(majorAxis);
    s = (sc / ama + 1.0f) * 0.5f;
    t = (tc / ama + 1.0f) * 0.5f;

    return (CubeMapFace)faceIndex;
}

static float AreaElement(float x, float y) {
    return Math::ATan(x * y, Math::Sqrt(x * x + y * y + 1.0f));
}

float Image::CubeMapTexelSolidAngle(float x, float y, int size) {
    float invSize = 1.0f / size;

    // Scale up to [-1, 1] range (inclusive), offset by 0.5 to point to texel center.
    // CHECK: is half pixel correct ?
    float s = (2.0f * ((float)x + 0.5f) * invSize) - 1.0f;
    float t = (2.0f * ((float)y + 0.5f) * invSize) - 1.0f;

    // s and t are the -1..1 texture coordinate on the current face.
    // Get projected area for this texel
    float x0 = s - invSize;
    float y0 = t - invSize;
    float x1 = s + invSize;
    float y1 = t + invSize;
    return AreaElement(x0, y0) - AreaElement(x0, y1) - AreaElement(x1, y0) + AreaElement(x1, y1);
}

int Image::NumPixels(int firstLevel, int numLevels) const {
    int w = GetWidth(firstLevel);
    int h = GetHeight(firstLevel);
    int d = GetDepth(firstLevel);
    
    int size = 0;
    while (numLevels){
        size += w * h * d;
        w >>= 1;
        h >>= 1;
        d >>= 1;

        if (w == 0 && h == 0 && d == 0) break;
        if (w == 0) w = 1;
        if (h == 0) h = 1;
        if (d == 0) d = 1;

        numLevels--;
    }

    return size * numSlices;
}

int Image::GetSize(int firstLevel, int numLevels) const {
    int size = MemRequired(GetWidth(firstLevel), GetHeight(firstLevel), GetDepth(firstLevel), numLevels, format);
    return size * numSlices;
}

int Image::GetSliceSize(int firstLevel, int numLevels) const {
    int size = MemRequired(GetWidth(firstLevel), GetHeight(firstLevel), GetDepth(firstLevel), numLevels, format);
    return size;
}

//--------------------------------------------------------------------------------------------------
//
// static helper functions for image information
//
//--------------------------------------------------------------------------------------------------

const char *Image::FormatName(Image::Format imageFormat) {
    return GetImageFormatInfo(imageFormat)->name;
}

int Image::BytesPerPixel(Image::Format imageFormat) {
    return !IsCompressed(imageFormat) ? GetImageFormatInfo(imageFormat)->size : 0;
}

int Image::BytesPerBlock(Image::Format imageFormat) {
    return IsCompressed(imageFormat) ? GetImageFormatInfo(imageFormat)->size : 0;
}

int Image::NumComponents(Image::Format imageFormat) {
    return GetImageFormatInfo(imageFormat)->numComponents;
}

void Image::GetBits(Image::Format imageFormat, int *redBits, int *greenBits, int *blueBits, int *alphaBits) {
    const ImageFormatInfo *info = GetImageFormatInfo(imageFormat);
    if (redBits)    *redBits    = info->redBits;
    if (greenBits)  *greenBits  = info->greenBits;
    if (blueBits)   *blueBits   = info->blueBits;
    if (alphaBits)  *alphaBits  = info->alphaBits;
}

bool Image::HasAlpha(Image::Format imageFormat) {
    const ImageFormatInfo *info = GetImageFormatInfo(imageFormat);
    if (info->type & Compressed) {
        switch (imageFormat) {
        case RGBA_DXT3:
        case RGBA_DXT5:
        case RGBA_PVRTC_2BPPV1:
        case RGBA_PVRTC_4BPPV1:
        case RGBA_PVRTC_2BPPV2:
        case RGBA_PVRTC_4BPPV2:
        case RGBA_8_8_ETC2:
        case RGBA_8_1_ETC2:
        case RGBA_EA_ATC:
        case RGBA_IA_ATC:
            return true;
        default:
            return false;
        }
        return false;
    }
    return info->alphaBits > 0 ? true : false;
}

bool Image::HasOneBitAlpha(Image::Format imageFormat) {
    const ImageFormatInfo *info = GetImageFormatInfo(imageFormat);
    if (info->type & Compressed) {
        switch (imageFormat) {
        case RGBA_DXT1: // TODO: check 1-bit-alpha is used
        case RGBA_8_1_ETC2:
            return true;
        default:
            return false;
        }
    }
    return info->alphaBits == 1 ? true : false;
}

bool Image::IsPacked(Image::Format imageFormat) {
    return !!(GetImageFormatInfo(imageFormat)->type & Packed);
}

bool Image::IsCompressed(Image::Format imageFormat) {
    return !!(GetImageFormatInfo(imageFormat)->type & Compressed);
}

bool Image::IsFloatFormat(Image::Format imageFormat) {
    return !!(GetImageFormatInfo(imageFormat)->type & Float);
}

bool Image::IsHalfFormat(Image::Format imageFormat) {
    return !!(GetImageFormatInfo(imageFormat)->type & Half);
}

bool Image::IsDepthFormat(Image::Format imageFormat) {
    return !!(GetImageFormatInfo(imageFormat)->type & Depth);
}

bool Image::IsDepthStencilFormat(Image::Format imageFormat) {
    return (GetImageFormatInfo(imageFormat)->type & DepthStencil) == DepthStencil;
}

int Image::MemRequired(int width, int height, int depth, int numMipmaps, Image::Format imageFormat) {
    int w = width;
    int h = height;
    int d = depth;

    int size = 0;
    while (numMipmaps) {
        if (IsCompressed(imageFormat)) {
            int blockWidth, blockHeight;
            int minWidth, minHeight;
            CompressedFormatBlockDimensions(imageFormat, blockWidth, blockHeight);
            CompressedFormatMinDimensions(imageFormat, minWidth, minHeight);
            int w2 = w + ((unsigned int)(-w) % minWidth);
            int h2 = h + ((unsigned int)(-h) % minHeight);
            size += (w2 * h2 * d) / (blockWidth * blockHeight);
        } else {
            size += w * h * d;
        }
        w >>= 1;
        h >>= 1;
        d >>= 1;

        if (w == 0 && h == 0 && d == 0) break;
        if (w == 0) w = 1;
        if (h == 0) h = 1;
        if (d == 0) d = 1;

        numMipmaps--;
    }

    if (IsCompressed(imageFormat)) {
        size *= BytesPerBlock(imageFormat);
    } else {
        size *= BytesPerPixel(imageFormat);
    }

    return size;
}

int Image::MaxMipMapLevels(int width, int height, int depth) {
    int m = Max3(width, height, depth);

    int numMipLevels = 0;
    while (m > 0) {
        m >>= 1;
        numMipLevels++;
    }

    return numMipLevels;
}

BE_NAMESPACE_END
