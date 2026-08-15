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
    if (IsEmpty()) {
        return *this;
    }

    if (IsCompressed()) {
        assert(0);
        return *this;
    }

    byte *tmp = (byte *)Mem_Alloc16(Image::MemRequired(width, 1, 1, 1, 1, format));

    for (int sliceIndex = 0; sliceIndex < arraySize; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
            const int w = GetWidth(mipLevel);
            const int h = GetHeight(mipLevel);
            const int d = GetDepth(mipLevel);
            const int pitch = static_cast<int>(Image::MemRequired(w, 1, 1, 1, 1, format));
            const size_t planeSize = static_cast<size_t>(pitch) * h;
            const int h2 = h / 2;

            byte *mipPixels = GetPixels(mipLevel, sliceIndex);

            for (int z = 0; z < d; z++) {
                byte *planePixels = mipPixels + z * planeSize;

                for (int y = 0; y < h2; y++) {
                    simdProcessor->Memcpy(tmp, planePixels + y * pitch, pitch);
                    simdProcessor->Memcpy(planePixels + y * pitch, planePixels + (h - y - 1) * pitch, pitch);
                    simdProcessor->Memcpy(planePixels + (h - y - 1) * pitch, tmp, pitch);
                }
            }
        }
    }

    Mem_AlignedFree(tmp);
    return *this;
}

Image &Image::FlipX() {
    if (IsEmpty()) {
        return *this;
    }

    if (IsCompressed()) {
        assert(0);
        return *this;
    }

    const int bpp = BytesPerPixel();
    if (bpp <= 0) {
        assert(0);
        return *this;
    }

    byte *tmp = (byte *)Mem_Alloc16(bpp);

    for (int sliceIndex = 0; sliceIndex < arraySize; sliceIndex++) {
        for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
            const int w = GetWidth(mipLevel);
            const int h = GetHeight(mipLevel);
            const int d = GetDepth(mipLevel);
            const int pitch = bpp * w;
            const size_t planeSize = static_cast<size_t>(pitch) * h;
            const int w2 = w / 2;

            byte *mipPixels = GetPixels(mipLevel, sliceIndex);

            for (int z = 0; z < d; z++) {
                byte *planePixels = mipPixels + z * planeSize;

                for (int y = 0; y < h; y++) {
                    byte *rowPixels = planePixels + y * pitch;

                    for (int x = 0; x < w2; x++) {
                        simdProcessor->Memcpy(tmp, rowPixels + x * bpp, bpp);
                        simdProcessor->Memcpy(rowPixels + x * bpp, rowPixels + (w - x - 1) * bpp, bpp);
                        simdProcessor->Memcpy(rowPixels + (w - x - 1) * bpp, tmp, bpp);
                    }
                }
            }
        }
    }

    Mem_AlignedFree(tmp);
    return *this;
}

Image &Image::AdjustBrightness(float factor) {
    int rgbOffset;
    switch (format) {
    case Format::R8G8B8:
    case Format::B8G8R8:
    case Format::R8G8B8A8:
    case Format::B8G8R8A8:
    case Format::R8G8B8X8:
    case Format::B8G8R8X8:
        rgbOffset = 0;
        break;
    case Format::A8B8G8R8:
    case Format::A8R8G8B8:
        rgbOffset = 1;
        break;
    default:
        assert(0);
        return *this;
    }

    const int bpp = Image::BytesPerPixel(format);
    const uint64_t numPixels = NumPixels(0, numMipLevels);

    byte *ptr = pic;

    for (uint64_t i = 0; i < numPixels; i++) {
        byte *rgb = ptr + rgbOffset;
        float r = (float)rgb[0];
        float g = (float)rgb[1];
        float b = (float)rgb[2];

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

        rgb[0] = (byte)r;
        rgb[1] = (byte)g;
        rgb[2] = (byte)b;

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
    byte *ptr = pic;

    for (uint64_t i = 0; i < numPixels; i++) {
        const uint16_t r = table[ptr[0] + 0];
        const uint16_t g = table[ptr[1] + 256];
        const uint16_t b = table[ptr[2] + 512];
    
        ptr[0] = (r * 255) / 65535;
        ptr[1] = (g * 255) / 65535;
        ptr[2] = (b * 255) / 65535;

        ptr += 3;
    }

    return *this;
}

template <typename T, bool Dilation>
static void ApplyMorphology2D(const T *src, int width, int height, int numComponents, T *dst) {
    const int dx[] = { -1, 0, 1, 0 };
    const int dy[] = { 0, 1, 0, -1 };

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const size_t offset = (static_cast<size_t>(y) * width + x) * numComponents;
            size_t sampleOffsets[5] = { offset };
            int numSamples = 1;

            for (int d = 0; d < 4; d++) {
                const int cx = x + dx[d];
                const int cy = y + dy[d];

                if (cx < 0 || cx >= width || cy < 0 || cy >= height) {
                    continue;
                }

                sampleOffsets[numSamples++] = (static_cast<size_t>(cy) * width + cx) * numComponents;
            }

            for (int ch = 0; ch < numComponents; ch++) {
                T value = src[offset + ch];
                for (int sampleIndex = 1; sampleIndex < numSamples; sampleIndex++) {
                    const T sample = src[sampleOffsets[sampleIndex] + ch];
                    if constexpr (Dilation) {
                        if (value < sample) {
                            value = sample;
                        }
                    } else {
                        if (value > sample) {
                            value = sample;
                        }
                    }
                }
                dst[offset + ch] = value;
            }
        }
    }
}

template <bool Dilation>
static Image MakeMorphology2D(const Image &sourceImage) {
    if (sourceImage.IsPacked() || sourceImage.IsCompressed() || sourceImage.IsFloatFormat() ||
        sourceImage.IsDepthFormat() || sourceImage.IsDepthStencilFormat() || sourceImage.GetDepth() > 1) {
        assert(0);
        return sourceImage;
    }

    const int width = sourceImage.GetWidth();
    const int height = sourceImage.GetHeight();
    const int numComponents = sourceImage.NumComponents();
    const int bytesPerPixel = sourceImage.BytesPerPixel();
    if (numComponents <= 0 || bytesPerPixel % numComponents != 0) {
        assert(0);
        return sourceImage;
    }

    const int bytesPerComponent = bytesPerPixel / numComponents;
    if (bytesPerComponent != 1 && bytesPerComponent != 2 && bytesPerComponent != 4) {
        assert(0);
        return sourceImage;
    }

    Image image;
    image.Create2D(width, height, 1, sourceImage.GetFormat(), sourceImage.GetGammaSpace(), nullptr, Image::Flag::None);

    const ImageFormatInfo *formatInfo = GetImageFormatInfo(sourceImage.GetFormat());
    const bool isSigned = HasFlag(formatInfo->type, Image::FormatType::SNorm) || HasFlag(formatInfo->type, Image::FormatType::SInt);
    const byte *src = sourceImage.GetPixels();
    byte *dst = image.GetPixels();

    if (bytesPerComponent == 1) {
        if (isSigned) {
            ApplyMorphology2D<int8_t, Dilation>((const int8_t *)src, width, height, numComponents, (int8_t *)dst);
        } else {
            ApplyMorphology2D<uint8_t, Dilation>((const uint8_t *)src, width, height, numComponents, (uint8_t *)dst);
        }
    } else if (bytesPerComponent == 2) {
        if (isSigned) {
            ApplyMorphology2D<int16_t, Dilation>((const int16_t *)src, width, height, numComponents, (int16_t *)dst);
        } else {
            ApplyMorphology2D<uint16_t, Dilation>((const uint16_t *)src, width, height, numComponents, (uint16_t *)dst);
        }
    } else {
        if (isSigned) {
            ApplyMorphology2D<int32_t, Dilation>((const int32_t *)src, width, height, numComponents, (int32_t *)dst);
        } else {
            ApplyMorphology2D<uint32_t, Dilation>((const uint32_t *)src, width, height, numComponents, (uint32_t *)dst);
        }
    }

    return image;
}

Image Image::MakeDilation() const {
    return MakeMorphology2D<true>(*this);
}

Image Image::MakeErosion() const {
    return MakeMorphology2D<false>(*this);
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

            const int value = base ? 255 : 255 * (1.0f - Math::Sqrt(static_cast<float>(nearestSquaredDist)) / spread);

            image.pic[centerY * width + centerX] = value;
        }
    }

    return image;
}

Image &Image::SwapRedAlphaRGBA8888() {
    if (format != Format::R8G8B8A8) {
        assert(0);
        return *this;
    }

    const uint64_t numPixels = NumPixels(0, numMipLevels);
    byte *ptr = pic;

    for (uint64_t i = 0; i < numPixels; i++) {
        Swap(ptr[0], ptr[3]);
        ptr += 4;
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

BE_NAMESPACE_END
