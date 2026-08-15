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
#include <cfloat>

BE_NAMESPACE_BEGIN

template <typename T>
static BE_INLINE T ResizeSampleStore(float value) {
    const T minValue = std::numeric_limits<T>::lowest();
    const T maxValue = std::numeric_limits<T>::max();

    if (value <= static_cast<float>(minValue)) {
        return minValue;
    }
    if (value >= static_cast<float>(maxValue)) {
        return maxValue;
    }
    return static_cast<T>(value);
}

static BE_INLINE float SanitizeResizeSample(float value) {
    if (Math::IsFinite(value)) {
        return value;
    }
    if (Math::IsInf(value)) {
        return value < 0.0f ? -FLT_MAX : FLT_MAX;
    }
    return 0.0f;
}

template <>
BE_INLINE float ResizeSampleStore<float>(float value) {
    return SanitizeResizeSample(value);
}

template <>
BE_INLINE half ResizeSampleStore<half>(float value) {
    return half(ClampFloat(-(float)HALF_MAX, (float)HALF_MAX, SanitizeResizeSample(value)));
}

static int GetResizeAlphaComponentIndex(Image::Format format) {
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
    default:
        return Image::HasAlpha(format) ? Image::NumComponents(format) - 1 : -1;
    }
}

template <typename T>
void ResizeImageNearest(const T *src, int srcWidth, int srcHeight, T *dst, int dstWidth, int dstHeight, int numComponents) {
    int srcPitch = srcWidth * numComponents;

    float ratioX = (float)srcWidth / dstWidth;
    float ratioY = (float)srcHeight / dstHeight;

    for (int y = 0; y < dstHeight; y++) {
        int iY = y * ratioY;

        int offsetY = iY * srcPitch;

        for (int x = 0; x < dstWidth; x++) {
            int iX = x * ratioX;

            int offsetX = iX * numComponents;
        
            const T *srcPtrY = &src[offsetY];

            for (int i = 0; i < numComponents; i++) {
                *dst++ = srcPtrY[offsetX + i];
            }
        }
    }
}

template <typename T>
static void ResizeImageBilinear(const T *src, int srcWidth, int srcHeight, T *dst, int dstWidth, int dstHeight, int numComponents) {
    int srcPitch = srcWidth * numComponents;

    float ratioX = (float)srcWidth / dstWidth;
    float ratioY = (float)srcHeight / dstHeight;

    for (int y = 0; y < dstHeight; y++) {
        float fY0 = y * ratioY;
        float fracY = Math::Fract(fY0);

        int iY0 = fY0 - fracY;
        int iY1 = Min(iY0 + 1, srcHeight - 1);

        int offsetY0 = iY0 * srcPitch;
        int offsetY1 = iY1 * srcPitch;

        for (int x = 0; x < dstWidth; x++) {
            float fX0 = x * ratioX;
            float fracX = Math::Fract(fX0);

            int iX0 = fX0 - fracX;
            int iX1 = Min(iX0 + 1, srcWidth - 1);

            int offsetX0 = iX0 * numComponents;
            int offsetX1 = iX1 * numComponents;

            const T *srcPtrY[2];
            srcPtrY[0] = &src[offsetY0];
            srcPtrY[1] = &src[offsetY1];

            for (int i = 0; i < numComponents; i++) {
                int index0 = offsetX0 + i;
                int index1 = offsetX1 + i;

                float p00 = (float)srcPtrY[0][index0];
                float p01 = (float)srcPtrY[0][index1];

                float p10 = (float)srcPtrY[1][index0];
                float p11 = (float)srcPtrY[1][index1];

                float p0 = Math::Lerp<float>(p00, p01, fracX);
                float p1 = Math::Lerp<float>(p10, p11, fracX);
                float po = Math::Lerp<float>(p0, p1, fracY);

                *dst++ = ResizeSampleStore<T>(po);
            }
        }
    }
}

static void ResizeImageBilinearWithGamma(const byte *src, int srcWidth, int srcHeight, byte *dst, int dstWidth, int dstHeight, int numComponents, int alphaComponentIndex, const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    int srcPitch = srcWidth * numComponents;

    float ratioX = (float)srcWidth / dstWidth;
    float ratioY = (float)srcHeight / dstHeight;

    for (int y = 0; y < dstHeight; y++) {
        float fY0 = y * ratioY;
        float fracY = Math::Fract(fY0);

        int iY0 = fY0 - fracY;
        int iY1 = Min(iY0 + 1, srcHeight - 1);

        int offsetY0 = iY0 * srcPitch;
        int offsetY1 = iY1 * srcPitch;

        for (int x = 0; x < dstWidth; x++) {
            float fX0 = x * ratioX;
            float fracX = Math::Fract(fX0);

            int iX0 = fX0 - fracX;
            int iX1 = Min(iX0 + 1, srcWidth - 1);

            int offsetX0 = iX0 * numComponents;
            int offsetX1 = iX1 * numComponents;

            const byte *srcPtrY[2];
            srcPtrY[0] = &src[offsetY0];
            srcPtrY[1] = &src[offsetY1];

            for (int i = 0; i < numComponents; i++) {
                const bool gammaCorrect = i != alphaComponentIndex;
                int index0 = offsetX0 + i;
                int index1 = offsetX1 + i;

                float p00;
                float p01;
                float p10;
                float p11;

                if (gammaCorrect) {
                    p00 = gammaToLinear[srcPtrY[0][index0]];
                    p01 = gammaToLinear[srcPtrY[0][index1]];
                    p10 = gammaToLinear[srcPtrY[1][index0]];
                    p11 = gammaToLinear[srcPtrY[1][index1]];
                } else {
                    p00 = srcPtrY[0][index0];
                    p01 = srcPtrY[0][index1];
                    p10 = srcPtrY[1][index0];
                    p11 = srcPtrY[1][index1];
                }

                float p0 = Math::Lerp<float>(p00, p01, fracX);
                float p1 = Math::Lerp<float>(p10, p11, fracX);
                float po = Math::Lerp<float>(p0, p1, fracY);

                if (gammaCorrect) {
                    *dst++ = Math::Ftob(255.0f * linearToGamma(po));
                } else {
                    *dst++ = ResizeSampleStore<byte>(po);
                }
            }
        }
    }
}

template <typename T>
static void ResizeImageBicubic(const T *src, int srcWidth, int srcHeight, T *dst, int dstWidth, int dstHeight, int numComponents) {
    int srcPitch = srcWidth * numComponents;

    float ratioX = (float)srcWidth / dstWidth;
    float ratioY = (float)srcHeight / dstHeight;

    for (int y = 0; y < dstHeight; y++) {
        float fY1 = y * ratioY;
        float fracY = Math::Fract(fY1);

        int iY1 = fY1 - fracY;
        int iY0 = Max(iY1 - 1, 0);
        int iY2 = Min(iY1 + 1, srcHeight - 1);
        int iY3 = Min(iY1 + 2, srcHeight - 1);

        int offsetY0 = iY0 * srcPitch;
        int offsetY1 = iY1 * srcPitch;
        int offsetY2 = iY2 * srcPitch;
        int offsetY3 = iY3 * srcPitch;

        for (int x = 0; x < dstWidth; x++) {
            float fX1 = x * ratioX;
            float fracX = Math::Fract(fX1);

            int iX1 = fX1 - fracX;
            int iX0 = Max(iX1 - 1, 0);
            int iX2 = Min(iX1 + 1, srcWidth - 1);
            int iX3 = Min(iX1 + 2, srcWidth - 1);

            int offsetX0 = iX0 * numComponents;
            int offsetX1 = iX1 * numComponents;
            int offsetX2 = iX2 * numComponents;
            int offsetX3 = iX3 * numComponents;

            const T *srcPtrY[4];
            srcPtrY[0] = &src[offsetY0];
            srcPtrY[1] = &src[offsetY1];
            srcPtrY[2] = &src[offsetY2];
            srcPtrY[3] = &src[offsetY3];

            for (int i = 0; i < numComponents; i++) {
                int index0 = offsetX0 + i;
                int index1 = offsetX1 + i;
                int index2 = offsetX2 + i;
                int index3 = offsetX3 + i;

                float p00 = (float)srcPtrY[0][index0];
                float p01 = (float)srcPtrY[0][index1];
                float p02 = (float)srcPtrY[0][index2];
                float p03 = (float)srcPtrY[0][index3];

                float p10 = (float)srcPtrY[1][index0];
                float p11 = (float)srcPtrY[1][index1];
                float p12 = (float)srcPtrY[1][index2];
                float p13 = (float)srcPtrY[1][index3];

                float p20 = (float)srcPtrY[2][index0];
                float p21 = (float)srcPtrY[2][index1];
                float p22 = (float)srcPtrY[2][index2];
                float p23 = (float)srcPtrY[2][index3];

                float p30 = (float)srcPtrY[3][index0];
                float p31 = (float)srcPtrY[3][index1];
                float p32 = (float)srcPtrY[3][index2];
                float p33 = (float)srcPtrY[3][index3];

                float p0 = Math::Cerp<float>(p00, p01, p02, p03, fracX);
                float p1 = Math::Cerp<float>(p10, p11, p12, p13, fracX);
                float p2 = Math::Cerp<float>(p20, p21, p22, p23, fracX);
                float p3 = Math::Cerp<float>(p30, p31, p32, p33, fracX);
                float po = Math::Cerp<float>(p0, p1, p2, p3, fracY);

                *dst++ = ResizeSampleStore<T>(po);
            }
        }
    }
}

static void ResizeImageBicubicWithGamma(const byte *src, int srcWidth, int srcHeight, byte *dst, int dstWidth, int dstHeight, int numComponents, int alphaComponentIndex,
    const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    int srcPitch = srcWidth * numComponents;

    float ratioX = (float)srcWidth / dstWidth;
    float ratioY = (float)srcHeight / dstHeight;

    for (int y = 0; y < dstHeight; y++) {
        float fY1 = y * ratioY;
        float fracY = Math::Fract(fY1);

        int iY1 = fY1 - fracY;
        int iY0 = Max(iY1 - 1, 0);
        int iY2 = Min(iY1 + 1, srcHeight - 1);
        int iY3 = Min(iY1 + 2, srcHeight - 1);

        int offsetY0 = iY0 * srcPitch;
        int offsetY1 = iY1 * srcPitch;
        int offsetY2 = iY2 * srcPitch;
        int offsetY3 = iY3 * srcPitch;

        for (int x = 0; x < dstWidth; x++) {
            float fX1 = x * ratioX;
            float fracX = Math::Fract(fX1);

            int iX1 = fX1 - fracX;
            int iX0 = Max(iX1 - 1, 0);
            int iX2 = Min(iX1 + 1, srcWidth - 1);
            int iX3 = Min(iX1 + 2, srcWidth - 1);

            int offsetX0 = iX0 * numComponents;
            int offsetX1 = iX1 * numComponents;
            int offsetX2 = iX2 * numComponents;
            int offsetX3 = iX3 * numComponents;

            const byte *srcPtrY[4];
            srcPtrY[0] = &src[offsetY0];
            srcPtrY[1] = &src[offsetY1];
            srcPtrY[2] = &src[offsetY2];
            srcPtrY[3] = &src[offsetY3];

            for (int i = 0; i < numComponents; i++) {
                const bool gammaCorrect = i != alphaComponentIndex;
                int index0 = offsetX0 + i;
                int index1 = offsetX1 + i;
                int index2 = offsetX2 + i;
                int index3 = offsetX3 + i;

                float p00;
                float p01;
                float p02;
                float p03;
                float p10;
                float p11;
                float p12;
                float p13;
                float p20;
                float p21;
                float p22;
                float p23;
                float p30;
                float p31;
                float p32;
                float p33;

                if (gammaCorrect) {
                    p00 = gammaToLinear[srcPtrY[0][index0]];
                    p01 = gammaToLinear[srcPtrY[0][index1]];
                    p02 = gammaToLinear[srcPtrY[0][index2]];
                    p03 = gammaToLinear[srcPtrY[0][index3]];
                    p10 = gammaToLinear[srcPtrY[1][index0]];
                    p11 = gammaToLinear[srcPtrY[1][index1]];
                    p12 = gammaToLinear[srcPtrY[1][index2]];
                    p13 = gammaToLinear[srcPtrY[1][index3]];
                    p20 = gammaToLinear[srcPtrY[2][index0]];
                    p21 = gammaToLinear[srcPtrY[2][index1]];
                    p22 = gammaToLinear[srcPtrY[2][index2]];
                    p23 = gammaToLinear[srcPtrY[2][index3]];
                    p30 = gammaToLinear[srcPtrY[3][index0]];
                    p31 = gammaToLinear[srcPtrY[3][index1]];
                    p32 = gammaToLinear[srcPtrY[3][index2]];
                    p33 = gammaToLinear[srcPtrY[3][index3]];
                } else {
                    p00 = srcPtrY[0][index0];
                    p01 = srcPtrY[0][index1];
                    p02 = srcPtrY[0][index2];
                    p03 = srcPtrY[0][index3];
                    p10 = srcPtrY[1][index0];
                    p11 = srcPtrY[1][index1];
                    p12 = srcPtrY[1][index2];
                    p13 = srcPtrY[1][index3];
                    p20 = srcPtrY[2][index0];
                    p21 = srcPtrY[2][index1];
                    p22 = srcPtrY[2][index2];
                    p23 = srcPtrY[2][index3];
                    p30 = srcPtrY[3][index0];
                    p31 = srcPtrY[3][index1];
                    p32 = srcPtrY[3][index2];
                    p33 = srcPtrY[3][index3];
                }

                float p0 = Math::Cerp<float>(p00, p01, p02, p03, fracX);
                float p1 = Math::Cerp<float>(p10, p11, p12, p13, fracX);
                float p2 = Math::Cerp<float>(p20, p21, p22, p23, fracX);
                float p3 = Math::Cerp<float>(p30, p31, p32, p33, fracX);
                float po = Math::Cerp<float>(p0, p1, p2, p3, fracY);

                if (gammaCorrect) {
                    *dst++ = Math::Ftob(255.0f * linearToGamma(po));
                } else {
                    *dst++ = ResizeSampleStore<byte>(po);
                }
            }
        }
    }
}

template <typename T>
static void ResizeImage(const T *src, int srcWidth, int srcHeight, T *dst, int dstWidth, int dstHeight, int numComponents, Image::ResampleFilter filter) {
    switch (filter) {
    case Image::ResampleFilter::Nearest:
        ResizeImageNearest(src, srcWidth, srcHeight, dst, dstWidth, dstHeight, numComponents);
        break;
    case Image::ResampleFilter::Bilinear:
        ResizeImageBilinear(src, srcWidth, srcHeight, dst, dstWidth, dstHeight, numComponents);
        break;
    case Image::ResampleFilter::Bicubic:
        ResizeImageBicubic(src, srcWidth, srcHeight, dst, dstWidth, dstHeight, numComponents);
        break;
    }
}

static void ResizeImageWithGamma(const byte *src, int srcWidth, int srcHeight, byte *dst, int dstWidth, int dstHeight, int numComponents, int alphaComponentIndex, Image::ResampleFilter filter,
    const float (&gammaToLinear)[256], float (*linearToGamma)(float)) {
    switch (filter) {
    case Image::ResampleFilter::Nearest:
        ResizeImageNearest<byte>(src, srcWidth, srcHeight, dst, dstWidth, dstHeight, numComponents);
        break;
    case Image::ResampleFilter::Bilinear:
        ResizeImageBilinearWithGamma(src, srcWidth, srcHeight, dst, dstWidth, dstHeight, numComponents, alphaComponentIndex, gammaToLinear, linearToGamma);
        break;
    case Image::ResampleFilter::Bicubic:
        ResizeImageBicubicWithGamma(src, srcWidth, srcHeight, dst, dstWidth, dstHeight, numComponents, alphaComponentIndex, gammaToLinear, linearToGamma);
        break;
    }
}

bool Image::Resize(int32_t dstWidth, int32_t dstHeight, Image::ResampleFilter filter, Image &dstImage) const {
    assert(width > 0 && height > 0);
    assert(dstWidth > 0 && dstHeight > 0);
    
    if (IsPacked() || IsCompressed() || IsDepthFormat() || IsDepthStencilFormat()) {
        BE_WARNLOG("Couldn't resize from source image format %s\n", FormatName());
        return false;
    }

    if (depth != 1) {
        BE_WARNLOG("Couldn't resize from %ix%ix%i size source image\n", width, height, depth);
        return false;
    }

    if (width == dstWidth && height == dstHeight) {
        if (&dstImage != this) {
            dstImage = *this;
        }
        return true;
    }

    const ImageFormatInfo *formatInfo = GetImageFormatInfo(format);
    const int numComponents = NumComponents();
    const int bytesPerPixel = BytesPerPixel();
    if (numComponents <= 0 || bytesPerPixel % numComponents != 0) {
        BE_WARNLOG("Couldn't resize from source image format %s\n", FormatName());
        return false;
    }

    const int bytesPerComponent = bytesPerPixel / numComponents;
    const bool isSigned = HasFlag(formatInfo->type, FormatType::SNorm) || HasFlag(formatInfo->type, FormatType::SInt);
    const bool useGammaLut = bytesPerComponent == 1 && HasFlag(formatInfo->type, FormatType::UNorm) && (gammaSpace == GammaSpace::sRGB || gammaSpace == GammaSpace::Pow22);
    const int alphaComponentIndex = GetResizeAlphaComponentIndex(format);

    Image tempImage;
    Image *outputImage = &dstImage;
    if (&dstImage == this) {
        outputImage = &tempImage;
    }
    outputImage->Create(dstWidth, dstHeight, 1, arraySize, 1, format, gammaSpace, nullptr, flags);

    for (int sliceIndex = 0; sliceIndex < arraySize; sliceIndex++) {
        const byte *src = GetPixels(0, sliceIndex);
        byte *dst = outputImage->GetPixels(0, sliceIndex);

        if (IsFloatFormat()) {
            if (IsHalfFormat()) {
                ResizeImage<half>((const half *)src, width, height, (half *)dst, dstWidth, dstHeight, numComponents, filter);
            } else {
                ResizeImage<float>((const float *)src, width, height, (float *)dst, dstWidth, dstHeight, numComponents, filter);
            }
        } else if (useGammaLut) { // bytesPerComponent == 1 && UNorm && (gammaSpace == sRGB || gammaSpace == Pow22)
            if (gammaSpace == GammaSpace::sRGB) {
                ResizeImageWithGamma(src, width, height, dst, dstWidth, dstHeight, numComponents, alphaComponentIndex, filter, Image::sRGBToLinearTable, Image::LinearToGammaApprox);
            } else {
                ResizeImageWithGamma(src, width, height, dst, dstWidth, dstHeight, numComponents, alphaComponentIndex, filter, Image::pow22ToLinearTable, Image::LinearToGammaFast);
            }
        } else if (bytesPerComponent == 1) {
            if (isSigned) {
                ResizeImage<int8_t>((const int8_t *)src, width, height, (int8_t *)dst, dstWidth, dstHeight, numComponents, filter);
            } else {
                ResizeImage<uint8_t>((const uint8_t *)src, width, height, (uint8_t *)dst, dstWidth, dstHeight, numComponents, filter);
            }
        } else if (bytesPerComponent == 2) {
            if (isSigned) {
                ResizeImage<int16_t>((const int16_t *)src, width, height, (int16_t *)dst, dstWidth, dstHeight, numComponents, filter);
            } else {
                ResizeImage<uint16_t>((const uint16_t *)src, width, height, (uint16_t *)dst, dstWidth, dstHeight, numComponents, filter);
            }
        } else if (bytesPerComponent == 4) {
            if (isSigned) {
                ResizeImage<int32_t>((const int32_t *)src, width, height, (int32_t *)dst, dstWidth, dstHeight, numComponents, filter);
            } else {
                ResizeImage<uint32_t>((const uint32_t *)src, width, height, (uint32_t *)dst, dstWidth, dstHeight, numComponents, filter);
            }
        } else {
            BE_WARNLOG("Couldn't resize from source image format %s\n", FormatName());
            return false;
        }
    }

    if (outputImage == &tempImage) {
        dstImage = std::move(tempImage);
    }
    return true;
}

bool Image::ResizeSelf(int32_t dstWidth, int32_t dstHeight, Image::ResampleFilter filter) {
    Image dstImage;
    bool ret = Resize(dstWidth, dstHeight, filter, dstImage);
    if (ret) {
        *this = std::move(dstImage);
    }
    return ret;
}

BE_NAMESPACE_END
