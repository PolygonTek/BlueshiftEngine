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
#include "Core/Str.h"
#include "Core/Heap.h"
#include "Math/Math.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

Image &Image::FlipY() {
    if (IsCompressed()) {
        assert(0);
        return *this;
    }

    byte *tmp = (byte *)Mem_Alloc16(Image::MemRequired(width, 1, 1, 1, format));
    byte *src = pic;

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = GetWidth(mipLevel);
        int h = GetHeight(mipLevel);
        int sliceSize = GetSize(mipLevel);

        int h2 = h / 2;
        int pitch = Image::MemRequired(w, 1, 1, 1, format);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            for (int y = 0; y < h2; y++) {
                memcpy(tmp, src + y*pitch, pitch);
                memcpy(src + y*pitch, src + (h - y - 1)*pitch, pitch);
                memcpy(src + (h - y - 1)*pitch, tmp, pitch);
            }

            src += sliceSize;
        }
    }

    Mem_AlignedFree(tmp);
    return *this;
}

Image &Image::FlipX() {
    if (IsCompressed())	{
        assert(0);
        return *this;
    }

    int bpp = BytesPerPixel();
    byte *tmp = (byte *)Mem_Alloc16(bpp);
    byte *src = pic;

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = GetWidth(mipLevel);
        int h = GetHeight(mipLevel);
        int sliceSize = GetSize(mipLevel);

        int w2 = w / 2;
        int pitch = bpp * w;

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w2; x++) {
                    memcpy(tmp, src + y*pitch + x*bpp, bpp);
                    memcpy(src + y*pitch + x*bpp, src + y*pitch + (w - x - 1)*bpp, bpp);
                    memcpy(src + y*pitch + (w - x - 1)*bpp, tmp, bpp);
                }
            }

            src += sliceSize;
        }
    }

    Mem_AlignedFree(tmp);
    return *this;
}

Image &Image::AdjustBrightness(float factor) {
    if (IsPacked() || IsCompressed() || IsFloatFormat()) {
        assert(0);
        return *this;
    }

    if (factor > 0) {
        int bpp = Image::BytesPerPixel(format);
        byte *src = pic;

        for (int i = 0; i < width * height; i++) {
            float r = (float)src[0];
            float g = (float)src[1];
            float b = (float)src[2];

            r = r * (1.0 + factor) / 255.f;
            g = g * (1.0 + factor) / 255.f;
            b = b * (1.0 + factor) / 255.f;

            float scale = 1.f;
            float tmp;
            if (r > 1.f && (tmp = (1.f / r)) < scale) {
                scale = tmp;
            }
            if (g > 1.f && (tmp = (1.f / g)) < scale) {
                scale = tmp;
            }
            if (b > 1.f && (tmp = (1.f / b)) < scale) {
                scale = tmp;
            }
            scale *= 255.f;

            r *= scale;
            g *= scale;
            b *= scale;

            src[0] = (byte)r;
            src[1] = (byte)g;
            src[2] = (byte)b;

            src += bpp;
        }
    }

    return *this;
}

Image &Image::GammaCorrectRGB888(uint16_t ramp[768]) {
    int numPixels = NumPixels(0, numMipmaps);

    for (int i = 0; i < numPixels; i++) {
        uint16_t r = ramp[*(pic)];
        uint16_t g = ramp[*(pic+1) + 256];
        uint16_t b = ramp[*(pic+2) + 512];
    
        *(pic) = (r * 255) / 65535;
        *(pic+1) = (g * 255) / 65535;
        *(pic+2) = (b * 255) / 65535;

        pic += 3;
    }

    return *this;
}

Image &Image::SwapRedAlphaRGBA8888() {
    int numPixels = NumPixels(0, numMipmaps);

    for (int i = 0; i < numPixels; i++)	{
        pic[3] = pic[0];
        //pic[3] = 0;
        pic += 4;
    }

    return *this;
}

Image Image::MakeNormalMapRGBA8888(float bumpiness) const {
    Image image;
    image.Create2D(width, height, 1, Image::RGBA_8_8_8_8, nullptr, LinearSpaceFlag);

    byte *dst_ptr = image.pic;

    float inv255 = 1.0f / 255.0f;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            byte r = this->pic[4 * (y * width + x) + 0];
            byte g = this->pic[4 * (y * width + x) + 1];
            byte b = this->pic[4 * (y * width + x) + 2];
            byte a = this->pic[4 * (y * width + x) + 3];

            float c = (r + g + b) * inv255;

            r = this->pic[4 * (y * width + ((x + 1) % width)) + 0];
            g = this->pic[4 * (y * width + ((x + 1) % width)) + 1];
            b = this->pic[4 * (y * width + ((x + 1) % width)) + 2];

            float cx = (r + g + b) * inv255;

            r = this->pic[4 * (((y + 1) % height) * width + x) + 0];
            g = this->pic[4 * (((y + 1) % height) * width + x) + 1];
            b = this->pic[4 * (((y + 1) % height) * width + x) + 2];

            float cy = (r + g + b) * inv255;

            float dcx = bumpiness * (c - cx) * 0.5f;
            float dcy = bumpiness * (c - cy) * 0.5f;
            float dcz = 1.0f;

            float invLen = Math::InvSqrt(dcx * dcx + dcy * dcy + dcz * dcz);
            r = (dcx * invLen + 1.0f) * 0.5f * 255;
            g = (dcy * invLen + 1.0f) * 0.5f * 255;
            b = (dcz * invLen + 1.0f) * 0.5f * 255;
            
            *dst_ptr++ = r;
            *dst_ptr++ = g;
            *dst_ptr++ = b;
            *dst_ptr++ = a;
        }
    }

    return image;
}

Image &Image::AddNormalMapRGBA8888(const Image &normalMap) {
    byte *src1_ptr = this->pic;
    byte *src2_ptr = normalMap.pic;
    byte *dst_ptr = this->pic;

    float inv255 = 1.0f / 255.0f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float src1Red	= (*src1_ptr++ * inv255) * 2.0f - 1.0f;
            float src1Green	= (*src1_ptr++ * inv255) * 2.0f - 1.0f;
            float src1Blue	= (*src1_ptr++ * inv255) * 2.0f - 1.0f;
            byte src1Alpha	= *src1_ptr++;

            float src2Red	= (*src2_ptr++ * inv255) * 2.0f - 1.0f;
            float src2Green	= (*src2_ptr++ * inv255) * 2.0f - 1.0f;
            float src2Blue	= (*src2_ptr++ * inv255) * 2.0f - 1.0f;
            byte src2Alpha	= *src2_ptr++;

            float sumR = src1Red + src2Red;
            float sumG = src1Green + src2Green;
            float sumB = src1Blue + src2Blue;

            float invLen = 1.0f / Math::Sqrt(sumR * sumR + sumG * sumG + sumB * sumB);
            byte r = (byte)((sumR * invLen + 1.0f) * 127.5f);
            byte g = (byte)((sumG * invLen + 1.0f) * 127.5f);
            byte b = (byte)((sumB * invLen + 1.0f) * 127.5f);

            *dst_ptr++ = r;
            *dst_ptr++ = g;
            *dst_ptr++ = b;
            *dst_ptr++ = Min(src1Alpha + src2Alpha, 255);
        }
    }

    return *this;
}

template <typename T>
void BuildMipMap1D(T *dst, const T *src, const int width, const int components) {
    int xOff = (width  < 2) ? 0 : components;

    for (int x = 0; x < width; x += 2) {
        for (int i = 0; i < components; i++) {
            *dst++ = (src[0] + src[xOff]) / 2;
            src++;
        }
        src += xOff;
    }
}

template <typename T>
void BuildMipMap2D(T *dst, const T *src, const int width, const int height, const int components) {
    int xOff = (width  < 2) ? 0 : components;
    int yOff = (height < 2) ? 0 : components * width;

    for (int y = 0; y < height; y += 2) {
        for (int x = 0; x < width; x += 2) {
            for (int i = 0; i < components; i++) {
                *dst++ = (src[0] + src[xOff] + src[yOff] + src[yOff + xOff]) / 4;
                src++;
            }
            src += xOff;
        }
        src += yOff;
    }
}

template <typename T>
void BuildMipMap3D(T *dst, const T *src, const int width, const int height, const int depth, const int components) {
    int xOff = (width  < 2) ? 0 : components;
    int yOff = (height < 2) ? 0 : components * width;
    int zOff = (depth  < 2) ? 0 : components * width * height;

    for (int z = 0; z < depth; z += 2) {
        for (int y = 0; y < height; y += 2) {
            for (int x = 0; x < width; x += 2) {
                for (int i = 0; i < components; i++) {
                    *dst++ = (src[0] + src[xOff] + src[yOff] + src[yOff + xOff] + src[zOff] + src[zOff + xOff] + src[zOff + yOff] + src[zOff + yOff + xOff]) / 8;
                    src++;
                }
                src += xOff;
            }
            src += yOff;
        }
        src += zOff;
    }
}

template <typename T>
void BuildMipMap(T *dst, const T *src, const int width, const int height, const int depth, const int components) {
    if (depth > 1) {
        BuildMipMap3D(dst, src, width, height, depth, components);
    } else if (height > 1) {
        BuildMipMap2D(dst, src, width, height, components);
    } else {
        BuildMipMap1D(dst, src, width, components);
    }
}

Image &Image::GenerateMipmaps() {
    if (IsCompressed()) {
        BE_WARNLOG(L"Couldn't generate mipmaps for a compressed image.\n");
        return *this;
    }

    if (IsPacked()) {
        BE_WARNLOG(L"Couldn't generate mipmaps for a packed format image.\n");
        return *this;
    }

    int numComponents = NumComponents();

    for (int mipLevel = 0; mipLevel < numMipmaps - 1; mipLevel++) {
        int w = GetWidth(mipLevel);
        int h = GetHeight(mipLevel);
        int d = GetDepth(mipLevel);

        int srcSize = GetSliceSize(mipLevel, 1);
        int dstSize = GetSliceSize(mipLevel + 1, 1);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            byte *src = GetPixels(mipLevel, sliceIndex);
            byte *dst = GetPixels(mipLevel + 1, sliceIndex);

            if (IsFloatFormat()) {
                if (IsHalfFormat()) {
                    BuildMipMap((half *)dst, (half *)src, w, h, d, numComponents);
                } else {
                    BuildMipMap((float *)dst, (float *)src, w, h, d, numComponents);
                }
            } else {
                BuildMipMap(dst, src, w, h, d, numComponents);
            }
        }
    }

    return *this;
}

BE_NAMESPACE_END
