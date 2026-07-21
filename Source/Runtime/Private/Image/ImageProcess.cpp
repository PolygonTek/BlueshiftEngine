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

    byte *tmp = (byte *)Mem_Alloc16(Image::MemRequired(width, 1, 1, 1, 1, format));
    byte *src = pic;

    for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
        const int w = GetWidth(mipLevel);
        const int h = GetHeight(mipLevel);
        const size_t sliceSize = SizeInBytes(mipLevel);

        const int h2 = h / 2;
        const int pitch = static_cast<int>(Image::MemRequired(w, 1, 1, 1, 1, format));

        for (int y = 0; y < h2; y++) {
            simdProcessor->Memcpy(tmp, src + y*pitch, pitch);
            simdProcessor->Memcpy(src + y*pitch, src + (h - y - 1)*pitch, pitch);
            simdProcessor->Memcpy(src + (h - y - 1)*pitch, tmp, pitch);
        }
    }

    Mem_AlignedFree(tmp);
    return *this;
}

Image &Image::FlipX() {
    if (IsCompressed()) {
        assert(0);
        return *this;
    }

    const int bpp = BytesPerPixel();
    byte *tmp = (byte *)Mem_Alloc16(bpp);
    byte *src = pic;

    for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
        const int w = GetWidth(mipLevel);
        const int h = GetHeight(mipLevel);
        const size_t sliceSize = SizeInBytes(mipLevel);

        const int w2 = w / 2;
        const int pitch = bpp * w;

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w2; x++) {
                simdProcessor->Memcpy(tmp, src + y*pitch + x*bpp, bpp);
                simdProcessor->Memcpy(src + y*pitch + x*bpp, src + y*pitch + (w - x - 1)*bpp, bpp);
                simdProcessor->Memcpy(src + y*pitch + (w - x - 1)*bpp, tmp, bpp);
            }
        }
    }

    Mem_AlignedFree(tmp);
    return *this;
}

// FIXME
Image &Image::AdjustBrightness(float factor) {
    if (IsPacked() || IsCompressed() || IsFloatFormat()) {
        assert(0);
        return *this;
    }

    const int bpp = Image::BytesPerPixel(format);
    const uint64_t numPixels = NumPixels(0, numMipLevels);

    byte *ptr = pic;

    for (uint64_t i = 0; i < numPixels; i++) {
        float r = (float)ptr[0];
        float g = (float)ptr[1];
        float b = (float)ptr[2];

        r = r * factor / 255.0f;
        g = g * factor / 255.0f;
        b = b * factor / 255.0f;

        float scale = 1.0f;

        if (r > 1.0f) {
            float tmp = 1.0f / r;
            if (tmp < scale) {
                scale = tmp;
            }
        }
        if (g > 1.0f) {
            float tmp = 1.0f / g;
            if (tmp < scale) {
                scale = tmp;
            }
        }
        if (b > 1.0f) {
            float tmp = 1.0f / b;
            if (tmp < scale) {
                scale = tmp;
            }
        }

        scale *= 255.0f;

        r *= scale;
        g *= scale;
        b *= scale;

        ptr[0] = (byte)r;
        ptr[1] = (byte)g;
        ptr[2] = (byte)b;

        ptr += bpp;
    }

    return *this;
}

Image &Image::ApplyGammaRampTableRGB888(const uint16_t table[768]) {
    if (format != Format::R8G8B8) {
        assert(0);
        return *this;
    }

    const uint64_t numPixels = NumPixels(0, numMipLevels);

    for (uint64_t i = 0; i < numPixels; i++) {
        const uint16_t r = table[pic[0] + 0];
        const uint16_t g = table[pic[1] + 256];
        const uint16_t b = table[pic[2] + 512];
    
        pic[0] = (r * 255) / 65535;
        pic[1] = (g * 255) / 65535;
        pic[2] = (b * 255) / 65535;

        pic += 3;
    }

    return *this;
}

Image Image::MakeDilation() const {
    if (IsPacked() || IsCompressed() || IsFloatFormat() || GetDepth() > 1) {
        assert(0);
        return *this;
    }

    Image image;
    image.Create2D(width, height, 1, format, gammaSpace, nullptr, Flag::None);

    const int bpp = BytesPerPixel();

    const int dx[] = { -1, 0, 1, 0 };
    const int dy[] = { 0, 1, 0, -1 };
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const int offset = (y * width + x) * bpp;
            float maxColor[4];
            for (int ch = 0; ch < bpp; ch++) {
                maxColor[ch] = pic[offset + ch];
            }

            for (int d = 0; d < 4; d++) {
                const int cx = x + dx[d];
                const int cy = y + dy[d];

                if (cx < 0 || cx >= width || cy < 0 || cy >= height) {
                    continue;
                }

                const int doffset = (cy * width + cx) * bpp;
                for (int ch = 0; ch < bpp; ch++) {
                    float c = pic[doffset + ch];
                    if (maxColor[ch] < c) {
                        maxColor[ch] = c;
                    }
                }
            }

            for (int ch = 0; ch < bpp; ch++) {
                image.pic[offset + ch] = (byte)Math::Round(maxColor[ch]);
            }
        }
    }

    return image;
}

Image Image::MakeErosion() const {
    if (IsPacked() || IsCompressed() || IsFloatFormat() || GetDepth() > 1) {
        assert(0);
        return *this;
    }

    Image image;
    image.Create2D(width, height, 1, format, gammaSpace, nullptr, Flag::None);

    const int bpp = BytesPerPixel();

    const int dx[] = { -1, 0, 1, 0 };
    const int dy[] = { 0, 1, 0, -1 };

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const int offset = (y * width + x) * bpp;
            float minColor[4];
            for (int ch = 0; ch < bpp; ch++) {
                minColor[ch] = pic[offset + ch];
            }

            for (int d = 0; d < 4; d++) {
                const int cx = x + dx[d];
                const int cy = y + dy[d];

                if (cx < 0 || cx >= width || cy < 0 || cy >= height) {
                    continue;
                }

                const int doffset = (cy * width + cx) * bpp;
                for (int ch = 0; ch < bpp; ch++) {
                    float c = pic[doffset + ch];
                    if (minColor[ch] > c) {
                        minColor[ch] = c;
                    }
                }
            }

            for (int ch = 0; ch < bpp; ch++) {
                image.pic[offset + ch] = (byte)Math::Round(minColor[ch]);
            }
        }
    }

    return image;
}

Image Image::MakeSDF(int32_t spread) const {
    Image image;
    image.Create2D(width, height, 1, Format::A8, GammaSpace::Linear, nullptr, Flag::None);

    for (int centerY = 0; centerY < height; centerY++) {
        for (int centerX = 0; centerX < width; centerX++) {
            const bool base = !!pic[centerY * width + centerX];

            const int startX = Max(0, centerX - spread);
            const int endX = Min(width - 1, centerX + spread);
            const int startY = Max(0, centerY - spread);
            const int endY = Min(height - 1, centerY + spread);

            int nearestSquaredDist = spread * spread;

            for (int y = startY; y <= endY; y++) {
                for (int x = startX; x <= endX; x++) {
                    if (base != !!pic[y * width + x]) {
                        int dx = x - centerX;
                        int dy = y - centerY;

                        int squaredDist = dx * dx + dy * dy;
                        if (squaredDist < nearestSquaredDist) {
                            nearestSquaredDist = squaredDist;
                        }
                    }
                }
            }

            const int value = base ? 255 : 255 * (1.0f - Math::Sqrt(nearestSquaredDist) / spread);

            image.pic[centerY * width + centerX] = value;
        }
    }

    return image;
}

Image &Image::SwapRedAlphaRGBA8888() {
    const uint64_t numPixels = NumPixels(0, numMipLevels);

    for (uint64_t i = 0; i < numPixels; i++) {
        pic[3] = pic[0];
        //pic[3] = 0;
        pic += 4;
    }

    return *this;
}

Image Image::MakeNormalMapRGBA8888(float bumpiness) const {
    Image image;
    image.Create2D(width, height, 1, Image::Format::R8G8B8A8, GammaSpace::Linear, nullptr, Flag::None);

    byte *dstPtr = image.pic;

    constexpr float inv255 = 1.0f / 255.0f;
    
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
            
            *dstPtr++ = r;
            *dstPtr++ = g;
            *dstPtr++ = b;
            *dstPtr++ = a;
        }
    }

    return image;
}

Image &Image::AddNormalMapRGBA8888(const Image &normalMap) {
    byte *src1_ptr = this->pic;
    byte *src2_ptr = normalMap.pic;
    byte *dstPtr = this->pic;

    constexpr float inv255 = 1.0f / 255.0f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float src1Red   = (*src1_ptr++ * inv255) * 2.0f - 1.0f;
            float src1Green = (*src1_ptr++ * inv255) * 2.0f - 1.0f;
            float src1Blue  = (*src1_ptr++ * inv255) * 2.0f - 1.0f;
            byte src1Alpha  = *src1_ptr++;

            float src2Red   = (*src2_ptr++ * inv255) * 2.0f - 1.0f;
            float src2Green = (*src2_ptr++ * inv255) * 2.0f - 1.0f;
            float src2Blue  = (*src2_ptr++ * inv255) * 2.0f - 1.0f;
            byte src2Alpha  = *src2_ptr++;

            float sumR = src1Red + src2Red;
            float sumG = src1Green + src2Green;
            float sumB = src1Blue + src2Blue;

            float invLen = 1.0f / Math::Sqrt(sumR * sumR + sumG * sumG + sumB * sumB);
            byte r = (byte)((sumR * invLen + 1.0f) * 127.5f);
            byte g = (byte)((sumG * invLen + 1.0f) * 127.5f);
            byte b = (byte)((sumB * invLen + 1.0f) * 127.5f);

            *dstPtr++ = r;
            *dstPtr++ = g;
            *dstPtr++ = b;
            *dstPtr++ = Min(src1Alpha + src2Alpha, 255);
        }
    }

    return *this;
}

template <typename T>
static BE_INLINE T MipMapSample(T value) {
    return value;
}

#ifndef IMAGE_GENERATE_MIPMAPS_CLAMP_INF_NAN
#define IMAGE_GENERATE_MIPMAPS_CLAMP_INF_NAN 1
#endif

// IMAGE_GENERATE_MIPMAPS_CLAMP_INF_NAN 이 켜져있으면, float/half 이미지의 밉맵 생성 시 INF/NAN 을 clamp 한다.
#if IMAGE_GENERATE_MIPMAPS_CLAMP_INF_NAN
static BE_INLINE float MipMapSample(float value) {
    const uint32_t bits = *(const uint32_t *)&value;
    const uint32_t absBits = bits & 0x7fffffff;

    if (absBits < 0x7f800000) {
        return value;
    }

    // INF should not propagate through the mip chain.
    if (absBits == 0x7f800000) {
        return (bits & 0x80000000) ? -(float)HALF_MAX : (float)HALF_MAX;
    }

    return 0.0f; // NaN
}

static BE_INLINE half MipMapSample(half value) {
    if (value.IsFinite()) {
        return value;
    }

    if (value.IsInfinity()) {
        return half(value.IsNegative() ? -(float)HALF_MAX : (float)HALF_MAX);
    }

    return half(0.0f); // NaN
}
#endif

template <typename T>
static void BuildMipMap1D(T *dst, const T *src, int width, int numComponents, int alphaComponentIndex, bool preserveCoverage) {
    const int xOff = (width < 2) ? 0 : numComponents;
    float a0 = 0;
    float a1 = 0;
    float alphaSum = 0;

    for (int x = 0; x < width; x += 2) {
        if (alphaComponentIndex >= 0) {
            a0 = MipMapSample(src[alphaComponentIndex]);
            a1 = MipMapSample(src[alphaComponentIndex + xOff]);
            alphaSum = a0 + a1;
        }

        for (int i = 0; i < numComponents; i++) {
            if (preserveCoverage && alphaSum > 0) {
                if (i != alphaComponentIndex) {
                    float p0 = MipMapSample(src[0]) * a0;
                    float p1 = MipMapSample(src[xOff]) * a1;
                    *dst++ = (p0 + p1) / alphaSum;
                } else {
                    *dst++ = Max(a0, a1);
                }
            } else {
                if (i != alphaComponentIndex) {
                    T p0 = MipMapSample(src[0]);
                    T p1 = MipMapSample(src[xOff]);
                    *dst++ = (p0 + p1) / 2;
                } else {
                    *dst++ = (a0 + a1) / 2;
                }
            }
            src++;
        }
        src += xOff;
    }
}

template <typename T>
static void BuildMipMap2D(T *dst, const T *src, int width, int height, int numComponents, int alphaComponentIndex, bool preserveCoverage) {
    const int xOff = (width  < 2) ? 0 : numComponents;
    const int yOff = (height < 2) ? 0 : numComponents * width;
    float a0 = 0;
    float a1 = 0;
    float a2 = 0;
    float a3 = 0;
    float alphaSum = 0;

    for (int y = 0; y < height; y += 2) {
        for (int x = 0; x < width; x += 2) {
            if (alphaComponentIndex >= 0) {
                a0 = MipMapSample(src[alphaComponentIndex]);
                a1 = MipMapSample(src[alphaComponentIndex + xOff]);
                a2 = MipMapSample(src[alphaComponentIndex + yOff]);
                a3 = MipMapSample(src[alphaComponentIndex + yOff + xOff]);
                alphaSum = a0 + a1 + a2 + a3;
            }

            for (int i = 0; i < numComponents; i++) {
                if (preserveCoverage && alphaSum > 0) {
                    if (i != alphaComponentIndex) {
                        float p0 = MipMapSample(src[0]) * a0;
                        float p1 = MipMapSample(src[xOff]) * a1;
                        float p2 = MipMapSample(src[yOff]) * a2;
                        float p3 = MipMapSample(src[yOff + xOff]) * a3;
                        *dst++ = (p0 + p1 + p2 + p3) / alphaSum;
                    } else {
                        *dst++ = Max(a0, Max(a1, Max(a2, a3)));
                    }
                } else {
                    if (i != alphaComponentIndex) {
                        T p0 = MipMapSample(src[0]);
                        T p1 = MipMapSample(src[xOff]);
                        T p2 = MipMapSample(src[yOff]);
                        T p3 = MipMapSample(src[yOff + xOff]);
                        *dst++ = (p0 + p1 + p2 + p3) / 4;
                    } else {
                        *dst++ = (a0 + a1 + a2 + a3) / 4;
                    }
                }
                src++;
            }
            src += xOff;
        }
        src += yOff;
    }
}

template <typename T>
static void BuildMipMap3D(T *dst, const T *src, int width, int height, int depth, int numComponents, int alphaComponentIndex, bool preserveCoverage) {
    const int xOff = (width  < 2) ? 0 : numComponents;
    const int yOff = (height < 2) ? 0 : numComponents * width;
    const int zOff = (depth  < 2) ? 0 : numComponents * width * height;
    float a0 = 0;
    float a1 = 0;
    float a2 = 0;
    float a3 = 0;
    float a4 = 0;
    float a5 = 0;
    float a6 = 0;
    float a7 = 0;
    float alphaSum = 0;

    for (int z = 0; z < depth; z += 2) {
        for (int y = 0; y < height; y += 2) {
            for (int x = 0; x < width; x += 2) {
                if (alphaComponentIndex >= 0) {
                    a0 = MipMapSample(src[alphaComponentIndex]);
                    a1 = MipMapSample(src[alphaComponentIndex + xOff]);
                    a2 = MipMapSample(src[alphaComponentIndex + yOff]);
                    a3 = MipMapSample(src[alphaComponentIndex + yOff + xOff]);
                    a4 = MipMapSample(src[alphaComponentIndex + zOff]);
                    a5 = MipMapSample(src[alphaComponentIndex + zOff + xOff]);
                    a6 = MipMapSample(src[alphaComponentIndex + zOff + yOff]);
                    a7 = MipMapSample(src[alphaComponentIndex + zOff + yOff + xOff]);
                    alphaSum = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
                }

                for (int i = 0; i < numComponents; i++) {
                    if (preserveCoverage && alphaSum > 0) {
                        if (i != alphaComponentIndex) {
                            float p0 = MipMapSample(src[0]) * a0;
                            float p1 = MipMapSample(src[xOff]) * a1;
                            float p2 = MipMapSample(src[yOff]) * a2;
                            float p3 = MipMapSample(src[yOff + xOff]) * a3;
                            float p4 = MipMapSample(src[zOff]) * a4;
                            float p5 = MipMapSample(src[zOff + xOff]) * a5;
                            float p6 = MipMapSample(src[zOff + yOff]) * a6;
                            float p7 = MipMapSample(src[zOff + yOff + xOff]) * a7;
                            *dst++ = (p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) / alphaSum;
                        } else {
                            *dst++ = Max(a0, Max(a1, Max(a2, Max(a3, Max(a4, Max(a5, Max(a6, a7)))))));
                        }
                    } else {
                        if (i != alphaComponentIndex) {
                            T p0 = MipMapSample(src[0]);
                            T p1 = MipMapSample(src[xOff]);
                            T p2 = MipMapSample(src[yOff]);
                            T p3 = MipMapSample(src[yOff + xOff]);
                            T p4 = MipMapSample(src[zOff]);
                            T p5 = MipMapSample(src[zOff + xOff]);
                            T p6 = MipMapSample(src[zOff + yOff]);
                            T p7 = MipMapSample(src[zOff + yOff + xOff]);
                            *dst++ = (p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) / 8;
                        } else {
                            *dst++ = (a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7) * 0.125f;
                        }
                    }
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
static void BuildMipMap(T *dst, const T *src, int width, int height, int depth, int numComponents, int alphaComponentIndex, bool preserveCoverage) {
    if (depth > 1) {
        BuildMipMap3D(dst, src, width, height, depth, numComponents, alphaComponentIndex, preserveCoverage);
    } else if (height > 1) {
        BuildMipMap2D(dst, src, width, height, numComponents, alphaComponentIndex, preserveCoverage);
    } else {
        BuildMipMap1D(dst, src, width, numComponents, alphaComponentIndex, preserveCoverage);
    }
}

static void BuildMipMap1DWithGamma(byte *dst, const byte *src, int width, int numComponents, int alphaComponentIndex, bool preserveCoverage, const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    const int xOff = (width < 2) ? 0 : numComponents;
    float a0 = 0;
    float a1 = 0;
    float alphaSum = 0;

    for (int x = 0; x < width; x += 2) {
        if (alphaComponentIndex >= 0) {
            a0 = src[alphaComponentIndex];
            a1 = src[alphaComponentIndex + xOff];
            alphaSum = a0 + a1;
        }

        for (int i = 0; i < numComponents; i++) {
            if (preserveCoverage && alphaSum > 0) {
                if (i != alphaComponentIndex) {
                    float p0 = gammaToLinear[src[0]] * a0;
                    float p1 = gammaToLinear[src[xOff]] * a1;
                    *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1) / alphaSum));
                } else {
                    *dst++ = Max(a0, a1);
                }
            } else {
                if (i != alphaComponentIndex) {
                    float p0 = gammaToLinear[src[0]];
                    float p1 = gammaToLinear[src[xOff]];
                    *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1) * 0.5f));
                } else {
                    *dst++ = (a0 + a1) * 0.5f;
                }
            }
            src++;
        }
        src += xOff;
    }
}

static void BuildMipMap2DWithGamma(byte *dst, const byte *src, int width, int height, int numComponents, int alphaComponentIndex, bool preserveCoverage, const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    const int xOff = (width < 2) ? 0 : numComponents;
    const int yOff = (height < 2) ? 0 : numComponents * width;
    float a0 = 0;
    float a1 = 0;
    float a2 = 0;
    float a3 = 0;
    float alphaSum = 0;

    for (int y = 0; y < height; y += 2) {
        for (int x = 0; x < width; x += 2) {
            if (alphaComponentIndex >= 0) {
                a0 = src[alphaComponentIndex];
                a1 = src[alphaComponentIndex + xOff];
                a2 = src[alphaComponentIndex + yOff];
                a3 = src[alphaComponentIndex + yOff + xOff];
                alphaSum = a0 + a1 + a2 + a3;
            }

            for (int i = 0; i < numComponents; i++) {
                if (preserveCoverage && alphaSum > 0) {
                    if (i != alphaComponentIndex) {
                        float p0 = gammaToLinear[src[0]] * a0;
                        float p1 = gammaToLinear[src[xOff]] * a1;
                        float p2 = gammaToLinear[src[yOff]] * a2;
                        float p3 = gammaToLinear[src[yOff + xOff]] * a3;
                        *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1 + p2 + p3) / alphaSum));
                    } else {
                        *dst++ = Max(a0, Max(a1, Max(a2, a3)));
                    }
                } else {
                    if (i != alphaComponentIndex) {
                        float p0 = gammaToLinear[src[0]];
                        float p1 = gammaToLinear[src[xOff]];
                        float p2 = gammaToLinear[src[yOff]];
                        float p3 = gammaToLinear[src[yOff + xOff]];
                        *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1 + p2 + p3) * 0.25f));
                    } else {
                        *dst++ = (a0 + a1 + a2 + a3) * 0.25f;
                    }
                }
                src++;
            }
            src += xOff;
        }
        src += yOff;
    }
}

static void BuildMipMap3DWithGamma(byte *dst, const byte *src, int width, int height, int depth, int numComponents, int alphaComponentIndex, bool preserveCoverage, const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    const int xOff = (width < 2) ? 0 : numComponents;
    const int yOff = (height < 2) ? 0 : numComponents * width;
    const int zOff = (depth < 2) ? 0 : numComponents * width * height;
    float a0 = 0;
    float a1 = 0;
    float a2 = 0;
    float a3 = 0;
    float a4 = 0;
    float a5 = 0;
    float a6 = 0;
    float a7 = 0;
    float alphaSum = 0;

    for (int z = 0; z < depth; z += 2) {
        for (int y = 0; y < height; y += 2) {
            for (int x = 0; x < width; x += 2) {
                if (alphaComponentIndex >= 0) {
                    a0 = src[alphaComponentIndex];
                    a1 = src[alphaComponentIndex + xOff];
                    a2 = src[alphaComponentIndex + yOff];
                    a3 = src[alphaComponentIndex + yOff + xOff];
                    a4 = src[alphaComponentIndex + zOff];
                    a5 = src[alphaComponentIndex + zOff + xOff];
                    a6 = src[alphaComponentIndex + zOff + yOff];
                    a7 = src[alphaComponentIndex + zOff + yOff + xOff];
                    alphaSum = a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
                }

                for (int i = 0; i < numComponents; i++) {
                    if (preserveCoverage && alphaSum > 0) {
                        if (i != alphaComponentIndex) {
                            float p0 = gammaToLinear[src[0]] * a0;
                            float p1 = gammaToLinear[src[xOff]] * a1;
                            float p2 = gammaToLinear[src[yOff]] * a2;
                            float p3 = gammaToLinear[src[yOff + xOff]] * a3;
                            float p4 = gammaToLinear[src[zOff]] * a4;
                            float p5 = gammaToLinear[src[zOff + xOff]] * a5;
                            float p6 = gammaToLinear[src[zOff + yOff]] * a6;
                            float p7 = gammaToLinear[src[zOff + yOff + xOff]] * a7;
                            *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) / alphaSum));
                        } else {
                            *dst++ = Max(a0, Max(a1, Max(a2, Max(a3, Max(a4, Max(a5, Max(a6, a7)))))));
                        }
                    } else {
                        if (i != alphaComponentIndex) {
                            float p0 = gammaToLinear[src[0]];
                            float p1 = gammaToLinear[src[xOff]];
                            float p2 = gammaToLinear[src[yOff]];
                            float p3 = gammaToLinear[src[yOff + xOff]];
                            float p4 = gammaToLinear[src[zOff]];
                            float p5 = gammaToLinear[src[zOff + xOff]];
                            float p6 = gammaToLinear[src[zOff + yOff]];
                            float p7 = gammaToLinear[src[zOff + yOff + xOff]];
                            *dst++ = Math::Ftob(255.0f * linearToGamma((p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7) * 0.125f));
                        } else {
                            *dst++ = (a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7) * 0.125f;
                        }
                    }
                    src++;
                }
                src += xOff;
            }
            src += yOff;
        }
        src += zOff;
    }
}

static void BuildMipMapWithGamma(byte *dst, const byte *src, int width, int height, int depth, int numComponents, int alphaComponentIndex, bool preserveCoverage, const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    if (depth > 1) {
        BuildMipMap3DWithGamma(dst, src, width, height, depth, numComponents, alphaComponentIndex, preserveCoverage, gammaToLinear, linearToGamma);
    } else if (height > 1) {
        BuildMipMap2DWithGamma(dst, src, width, height, numComponents, alphaComponentIndex, preserveCoverage, gammaToLinear, linearToGamma);
    } else {
        BuildMipMap1DWithGamma(dst, src, width, numComponents, alphaComponentIndex, preserveCoverage, gammaToLinear, linearToGamma);
    }
}

static int GetAlphaComponentIndex(Image::Format format) {
    switch (format) {
    case Image::Format::A8:
    case Image::Format::A16_FLOAT:
    case Image::Format::A32_FLOAT:
    case Image::Format::A8B8G8R8:
    case Image::Format::A8R8G8B8:
        return 0;
    case Image::Format::L8A8:
    case Image::Format::L16A16_FLOAT:
    case Image::Format::L32A32_FLOAT:
        return 1;
    case Image::Format::R8G8B8A8:
    case Image::Format::B8G8R8A8:
    case Image::Format::R8G8B8A8_SNORM:
    case Image::Format::R16G16B16A16_FLOAT:
    case Image::Format::R32G32B32A32_FLOAT:
        return 3;
    }
    return -1;
}

Image &Image::GenerateMipmaps(bool preserveAlphaCoverage) {
    if (IsCompressed()) {
        BE_WARNLOG("Image::GenerateMipmaps: Couldn't generate mipmaps for a compressed image.\n");
        return *this;
    }

    if (IsPacked()) {
        BE_WARNLOG("Image::GenerateMipmaps: Couldn't generate mipmaps for a packed format image.\n");
        return *this;
    }

    preserveAlphaCoverage = preserveAlphaCoverage && HasAlpha();

    const int alphaComponentIndex = GetAlphaComponentIndex(format);
    const int numComponents = NumComponents();
    const int numSlices = GetArraySize();

    for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
        for (int mipLevel = 1; mipLevel < numMipLevels; mipLevel++) {
            const int srcMipLevel = mipLevel - 1;
            const int w = GetWidth(srcMipLevel);
            const int h = GetHeight(srcMipLevel);
            const int d = GetDepth(srcMipLevel);

            const byte *src = GetPixels(srcMipLevel, sliceIndex);
            byte *dst = GetPixels(mipLevel, sliceIndex);

            if (IsFloatFormat()) {
                if (IsHalfFormat()) {
                    BuildMipMap<half>((half *)dst, (half *)src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                } else {
                    BuildMipMap<float>((float *)dst, (float *)src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                }
            } else {
                if (gammaSpace == GammaSpace::sRGB) {
                    BuildMipMapWithGamma(dst, src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage, Image::sRGBToLinearTable, Image::LinearToGammaApprox);
                } else if (gammaSpace == GammaSpace::Pow22) {
                    BuildMipMapWithGamma(dst, src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage, Image::pow22ToLinearTable, Image::LinearToGammaFast);
                } else {
                    BuildMipMap<byte>(dst, src, w, h, d, numComponents, alphaComponentIndex, preserveAlphaCoverage);
                }
            }
        }
    }
    return *this;
}

BE_NAMESPACE_END
