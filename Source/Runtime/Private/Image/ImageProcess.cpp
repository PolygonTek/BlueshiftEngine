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

// gammaToLinearTable[i] = (i/255)^2.2
float gammaToLinearTable[256] = {
    0.000000f, 0.000005f, 0.000023f, 0.000057f, 0.000107f, 0.000175f, 0.000262f, 0.000367f, 0.000493f, 0.000638f, 0.000805f, 0.000992f, 0.001202f, 0.001433f, 0.001687f, 0.001963f,
    0.002263f, 0.002586f, 0.002932f, 0.003303f, 0.003697f, 0.004116f, 0.004560f, 0.005028f, 0.005522f, 0.006041f, 0.006585f, 0.007155f, 0.007751f, 0.008373f, 0.009021f, 0.009696f,
    0.010398f, 0.011126f, 0.011881f, 0.012664f, 0.013473f, 0.014311f, 0.015175f, 0.016068f, 0.016988f, 0.017936f, 0.018913f, 0.019918f, 0.020951f, 0.022013f, 0.023104f, 0.024223f,
    0.025371f, 0.026549f, 0.027755f, 0.028991f, 0.030257f, 0.031551f, 0.032876f, 0.034230f, 0.035614f, 0.037029f, 0.038473f, 0.039947f, 0.041452f, 0.042987f, 0.044553f, 0.046149f,
    0.047776f, 0.049433f, 0.051122f, 0.052842f, 0.054592f, 0.056374f, 0.058187f, 0.060032f, 0.061907f, 0.063815f, 0.065754f, 0.067725f, 0.069727f, 0.071761f, 0.073828f, 0.075926f,
    0.078057f, 0.080219f, 0.082414f, 0.084642f, 0.086901f, 0.089194f, 0.091518f, 0.093876f, 0.096266f, 0.098689f, 0.101145f, 0.103634f, 0.106156f, 0.108711f, 0.111299f, 0.113921f,
    0.116576f, 0.119264f, 0.121986f, 0.124741f, 0.127530f, 0.130352f, 0.133209f, 0.136099f, 0.139022f, 0.141980f, 0.144972f, 0.147998f, 0.151058f, 0.154152f, 0.157281f, 0.160444f,
    0.163641f, 0.166872f, 0.170138f, 0.173439f, 0.176774f, 0.180144f, 0.183549f, 0.186989f, 0.190463f, 0.193972f, 0.197516f, 0.201096f, 0.204710f, 0.208360f, 0.212044f, 0.215764f,
    0.219520f, 0.223310f, 0.227137f, 0.230998f, 0.234895f, 0.238828f, 0.242796f, 0.246800f, 0.250840f, 0.254916f, 0.259027f, 0.263175f, 0.267358f, 0.271577f, 0.275833f, 0.280124f,
    0.284452f, 0.288816f, 0.293216f, 0.297653f, 0.302126f, 0.306635f, 0.311181f, 0.315763f, 0.320382f, 0.325037f, 0.329729f, 0.334458f, 0.339223f, 0.344026f, 0.348865f, 0.353741f,
    0.358654f, 0.363604f, 0.368591f, 0.373615f, 0.378676f, 0.383775f, 0.388910f, 0.394083f, 0.399293f, 0.404541f, 0.409826f, 0.415148f, 0.420508f, 0.425905f, 0.431340f, 0.436813f,
    0.442323f, 0.447871f, 0.453456f, 0.459080f, 0.464741f, 0.470440f, 0.476177f, 0.481952f, 0.487765f, 0.493616f, 0.499505f, 0.505432f, 0.511398f, 0.517401f, 0.523443f, 0.529523f,
    0.535642f, 0.541798f, 0.547994f, 0.554227f, 0.560499f, 0.566810f, 0.573159f, 0.579547f, 0.585973f, 0.592438f, 0.598942f, 0.605484f, 0.612066f, 0.618686f, 0.625345f, 0.632043f,
    0.638779f, 0.645555f, 0.652370f, 0.659224f, 0.666117f, 0.673049f, 0.680020f, 0.687031f, 0.694081f, 0.701169f, 0.708298f, 0.715465f, 0.722672f, 0.729919f, 0.737205f, 0.744530f,
    0.751895f, 0.759300f, 0.766744f, 0.774227f, 0.781751f, 0.789314f, 0.796917f, 0.804559f, 0.812241f, 0.819964f, 0.827726f, 0.835528f, 0.843370f, 0.851252f, 0.859174f, 0.867136f,
    0.875138f, 0.883180f, 0.891262f, 0.899384f, 0.907547f, 0.915750f, 0.923993f, 0.932277f, 0.940601f, 0.948965f, 0.957370f, 0.965815f, 0.974300f, 0.982826f, 0.991393f, 1.000000f
};

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
    if (IsCompressed()) {
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

    int bpp = Image::BytesPerPixel(format);
    int numPixels = NumPixels(0, numMipmaps);

    byte *ptr = pic;

    for (int i = 0; i < numPixels; i++) {
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
    if (IsPacked() || IsCompressed() || IsFloatFormat()) {
        assert(0);
        return *this;
    }

    int numPixels = NumPixels(0, numMipmaps);

    for (int i = 0; i < numPixels; i++) {
        uint16_t r = table[pic[0] + 0];
        uint16_t g = table[pic[1] + 256];
        uint16_t b = table[pic[2] + 512];
    
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
    image.Create2D(width, height, 1, format, gammaSpace, nullptr, 0);

    int bpp = BytesPerPixel();

    const int dx[] = { -1, 0, 1, 0 };
    const int dy[] = { 0, 1, 0, -1 };
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int offset = (y * width + x) * bpp;
            float maxColor[4];
            for (int ch = 0; ch < bpp; ch++) {
                maxColor[ch] = pic[offset + ch];
            }

            for (int d = 0; d < 4; d++) {
                int cx = x + dx[d];
                int cy = y + dy[d];

                if (cx < 0 || cx >= width || cy < 0 || cy >= height) {
                    continue;
                }

                int doffset = (cy * width + cx) * bpp;
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
    image.Create2D(width, height, 1, format, gammaSpace, nullptr, 0);

    int bpp = BytesPerPixel();

    const int dx[] = { -1, 0, 1, 0 };
    const int dy[] = { 0, 1, 0, -1 };

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int offset = (y * width + x) * bpp;
            float minColor[4];
            for (int ch = 0; ch < bpp; ch++) {
                minColor[ch] = pic[offset + ch];
            }

            for (int d = 0; d < 4; d++) {
                int cx = x + dx[d];
                int cy = y + dy[d];

                if (cx < 0 || cx >= width || cy < 0 || cy >= height) {
                    continue;
                }

                int doffset = (cy * width + cx) * bpp;
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

Image Image::MakeSDF(int spread) const {
    Image image;
    image.Create2D(width, height, 1, Format::A_8, GammaSpace::Linear, nullptr, 0);

    for (int centerY = 0; centerY < height; centerY++) {
        for (int centerX = 0; centerX < width; centerX++) {
            bool base = !!pic[centerY * width + centerX];

            int startX = Max(0, centerX - spread);
            int endX = Min(width - 1, centerX + spread);
            int startY = Max(0, centerY - spread);
            int endY = Min(height - 1, centerY + spread);

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

            int value = base ? 255 : 255 * (1.0f - Math::Sqrt(nearestSquaredDist) / spread);

            image.pic[centerY * width + centerX] = value;
        }
    }

    return image;
}

Image &Image::SwapRedAlphaRGBA8888() {
    int numPixels = NumPixels(0, numMipmaps);

    for (int i = 0; i < numPixels; i++) {
        pic[3] = pic[0];
        //pic[3] = 0;
        pic += 4;
    }

    return *this;
}

Image Image::MakeNormalMapRGBA8888(float bumpiness) const {
    Image image;
    image.Create2D(width, height, 1, Image::Format::RGBA_8_8_8_8, GammaSpace::Linear, nullptr, 0);

    byte *dstPtr = image.pic;

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

    float inv255 = 1.0f / 255.0f;

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

static void BuildMipMap1DWithGamma(byte *dst, const byte *src, const int width, const int components) {
    int xOff = (width < 2) ? 0 : components;

    for (int x = 0; x < width; x += 2) {
        for (int i = 0; i < components; i++) {
            float p0 = gammaToLinearTable[src[0]];
            float p1 = gammaToLinearTable[src[xOff]];

            *dst++ = Math::Ftob(255.0f * Math::Pow(0.5f * (p0 + p1), 1.0f / 2.2f));
            src++;
        }
        src += xOff;
    }
}

static void BuildMipMap2DWithGamma(byte *dst, const byte *src, const int width, const int height, const int components) {
    int xOff = (width < 2) ? 0 : components;
    int yOff = (height < 2) ? 0 : components * width;

    for (int y = 0; y < height; y += 2) {
        for (int x = 0; x < width; x += 2) {
            for (int i = 0; i < components; i++) {
                float p0 = gammaToLinearTable[src[0]];
                float p1 = gammaToLinearTable[src[xOff]];
                float p2 = gammaToLinearTable[src[yOff]];
                float p3 = gammaToLinearTable[src[yOff + xOff]];

                *dst++ = Math::Ftob(255.0f * Math::Pow(0.25f * (p0 + p1 + p2 + p3), 1.0f / 2.2f));
                src++;
            }
            src += xOff;
        }
        src += yOff;
    }
}

static void BuildMipMap3DWithGamma(byte *dst, const byte *src, const int width, const int height, const int depth, const int components) {
    int xOff = (width < 2) ? 0 : components;
    int yOff = (height < 2) ? 0 : components * width;
    int zOff = (depth < 2) ? 0 : components * width * height;

    for (int z = 0; z < depth; z += 2) {
        for (int y = 0; y < height; y += 2) {
            for (int x = 0; x < width; x += 2) {
                for (int i = 0; i < components; i++) {
                    float p0 = gammaToLinearTable[src[0]];
                    float p1 = gammaToLinearTable[src[xOff]];
                    float p2 = gammaToLinearTable[src[yOff]];
                    float p3 = gammaToLinearTable[src[yOff + xOff]];
                    float p4 = gammaToLinearTable[src[zOff]];
                    float p5 = gammaToLinearTable[src[zOff + xOff]];
                    float p6 = gammaToLinearTable[src[zOff + yOff]];
                    float p7 = gammaToLinearTable[src[zOff + yOff + xOff]];

                    *dst++ = Math::Ftob(255.0f * Math::Pow(0.125f * (p0 + p1 + p2 + p3 + p4 + p5 + p6 + p7), 1.0f / 2.2f));
                    src++;
                }
                src += xOff;
            }
            src += yOff;
        }
        src += zOff;
    }
}

static void BuildMipMapWithGamma(byte *dst, const byte *src, const int width, const int height, const int depth, const int components) {
    if (depth > 1) {
        BuildMipMap3DWithGamma(dst, src, width, height, depth, components);
    } else if (height > 1) {
        BuildMipMap2DWithGamma(dst, src, width, height, components);
    } else {
        BuildMipMap1DWithGamma(dst, src, width, components);
    }
}

Image &Image::GenerateMipmaps() {
    if (IsCompressed()) {
        BE_WARNLOG("Couldn't generate mipmaps for a compressed image.\n");
        return *this;
    }

    if (IsPacked()) {
        BE_WARNLOG("Couldn't generate mipmaps for a packed format image.\n");
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
                    BuildMipMap<half>((half *)dst, (half *)src, w, h, d, numComponents);
                } else {
                    BuildMipMap<float>((float *)dst, (float *)src, w, h, d, numComponents);
                }
            } else {
                if (gammaSpace != GammaSpace::Linear) {
                    BuildMipMapWithGamma(dst, src, w, h, d, numComponents);
                } else {
                    BuildMipMap(dst, src, w, h, d, numComponents);
                }
            }
        }
    }

    return *this;
}

BE_NAMESPACE_END
