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

                float srcComp[2][2];
                srcComp[0][0] = (float)srcPtrY[0][index0];
                srcComp[0][1] = (float)srcPtrY[0][index1];

                srcComp[1][0] = (float)srcPtrY[1][index0];
                srcComp[1][1] = (float)srcPtrY[1][index1];

                // NOTE: Should we need to lerp in linear color space ?
                float p0 = Math::Lerp<float>(srcComp[0][0], srcComp[0][1], fracX);
                float p1 = Math::Lerp<float>(srcComp[1][0], srcComp[1][1], fracX);

                float po = Math::Lerp<float>(p0, p1, fracY);

                *dst++ = ClampFloat(std::numeric_limits<T>::min(), std::numeric_limits<T>::max(), po);
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

                float srcComp[4][4];
                srcComp[0][0] = (float)srcPtrY[0][index0];
                srcComp[0][1] = (float)srcPtrY[0][index1];
                srcComp[0][2] = (float)srcPtrY[0][index2];
                srcComp[0][3] = (float)srcPtrY[0][index3];

                srcComp[1][0] = (float)srcPtrY[1][index0];
                srcComp[1][1] = (float)srcPtrY[1][index1];
                srcComp[1][2] = (float)srcPtrY[1][index2];
                srcComp[1][3] = (float)srcPtrY[1][index3];

                srcComp[2][0] = (float)srcPtrY[2][index0];
                srcComp[2][1] = (float)srcPtrY[2][index1];
                srcComp[2][2] = (float)srcPtrY[2][index2];
                srcComp[2][3] = (float)srcPtrY[2][index3];

                srcComp[3][0] = (float)srcPtrY[3][index0];
                srcComp[3][1] = (float)srcPtrY[3][index1];
                srcComp[3][2] = (float)srcPtrY[3][index2];
                srcComp[3][3] = (float)srcPtrY[3][index3];

                // NOTE: Should we need to lerp in linear color space ?
                float p0 = Math::Cerp<float>(srcComp[0][0], srcComp[0][1], srcComp[0][2], srcComp[0][3], fracX);
                float p1 = Math::Cerp<float>(srcComp[1][0], srcComp[1][1], srcComp[1][2], srcComp[1][3], fracX);
                float p2 = Math::Cerp<float>(srcComp[2][0], srcComp[2][1], srcComp[2][2], srcComp[2][3], fracX);
                float p3 = Math::Cerp<float>(srcComp[3][0], srcComp[3][1], srcComp[3][2], srcComp[3][3], fracX);

                float po = Math::Cerp<float>(p0, p1, p2, p3, fracY);

                *dst++ = ClampFloat(std::numeric_limits<T>::min(), std::numeric_limits<T>::max(), po);
            }
        }
    }
}

template <typename T>
static void ResizeImage(const T *src, int srcWidth, int srcHeight, T *dst, int dstWidth, int dstHeight, int numComponents, Image::ResampleFilter::Enum filter) {
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

bool Image::Resize(int dstWidth, int dstHeight, Image::ResampleFilter::Enum filter, Image &dstImage) const {
    assert(width && height);
    assert(dstWidth && dstHeight);
    
    if (IsPacked() || IsCompressed()) {
        BE_WARNLOG("Couldn't resize from source image format %s\n", FormatName());
        return false;
    }

    if (depth != 1) {
        BE_WARNLOG("Couldn't resize from %ix%ix%i size source image\n", width, height, depth);
        return false;
    }

    if (width == dstWidth && height == dstHeight) {
        return true;
    }

    dstImage.Create2D(dstWidth, dstHeight, 1, format, gammaSpace, nullptr, flags);

    int numComponents = NumComponents();

    if (IsFloatFormat()) {
        if (IsHalfFormat()) {
            ResizeImage((half *)this->pic, this->width, this->height, (half *)dstImage.pic, dstWidth, dstHeight, numComponents, filter);
        } else {
            ResizeImage((float *)this->pic, this->width, this->height, (float *)dstImage.pic, dstWidth, dstHeight, numComponents, filter);
        }
    } else {
        ResizeImage((byte *)this->pic, this->width, this->height, (byte *)dstImage.pic, dstWidth, dstHeight, numComponents, filter);
    }

    return true;
}

bool Image::ResizeSelf(int dstWidth, int dstHeight, Image::ResampleFilter::Enum filter) {
    assert(width && height);
    assert(dstWidth && dstHeight);

    if (IsPacked() || IsCompressed()) {
        BE_WARNLOG("Couldn't resize from source image format %s\n", FormatName());
        return false;
    }

    if (depth != 1) {
        BE_WARNLOG("Couldn't resize from %ix%ix%i size source image\n", width, height, depth);
        return false;
    }

    if (width == dstWidth && height == dstHeight) {
        return true;
    }
        
    byte *dst = (byte *)Mem_Alloc16(dstWidth * dstHeight * Image::BytesPerPixel(format));

    int numComponents = NumComponents();

    if (IsFloatFormat()) {
        ResizeImage((float *)this->pic, this->width, this->height, (float *)dst, dstWidth, dstHeight, numComponents, filter);
    } else if (IsHalfFormat()) {
        ResizeImage((half *)this->pic, this->width, this->height, (half *)dst, dstWidth, dstHeight, numComponents, filter);
    } else {
        ResizeImage(this->pic, this->width, this->height, dst, dstWidth, dstHeight, numComponents, filter);
    }

    if (this->alloced) {
        Mem_AlignedFree(this->pic);
    }

    this->pic = dst;
    this->width = dstWidth;
    this->height = dstHeight;

    return true;
}

BE_NAMESPACE_END
