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

// Fixed point linear interpolation
// f(x) = ax + b
// p0: f(0)
// p1: f(1)
// t: interpolater in range [0, 255]
static int fix_lerp(int p0, int p1, int t) {
    // p0 : f(0) = b
    // p1 : f(1) = a + b
    //
    // a = p1 - p0
    // b = p0
    return ((p0 << 8) + (p1 - p0) * t) >> 8;
}

// Fixed point cubic interpolation
// f(x) = ax^3 + bx^2 + cx + d
// p0: f(-1)
// p1: f(0)
// p2: f(1)
// p3: f(2)
// t: interpolater in range [0, 127]
static int fix_cerp(int p0, int p1, int p2, int p3, int t) {
#if 0
    // Hermite cubic spline with 4 points
    // f'(x) = 3ax^2 + 2bx + c
    //
    // p1 : f(0) = d
    // p2 : f(1) = a + b + c + d 
    // p2 - p0 : f'(0) = c
    // p3 - p1 : f'(1) = 3a + 2b + c
    //
    // |  0  0  0  1 | | a |   | p1      |
    // |  1  1  1  1 | | b | = | p2      |
    // |  0  0  1  0 | | c |   | p2 - p0 |
    // |  3  2  1  0 | | d |   | p3 - p1 |
    //
    // |  2 -2  1  1 | | p1      |   | a | 
    // | -3  3 -2 -1 | | p2      | = | b |
    // |  0  0  1  0 | | p2 - p0 |   | c |
    // |  1  0  0  0 | | p3 - p1 |   | d |
    //
    // a = 2*p1 - 2*p2 + (p2 - p0) + (p3 - p1) = p3 - p2 + p1 - p0
    // b = -3*p1 + 3*p2 - 2*(p2 - p0) - (p3 - p1) = -p3 + p2 - 2*p1 + 2*p0
    // c = p2 - p0
    // d = p1
    int p01 = p0 - p1;
    int a = (p3 - p2) - p01;
    int b = p01 - a;
    int c = p2 - p0;
    int d = p1;
    return (t * (t * (t * a + (b << 7)) + (c << 14)) + (d << 21)) >> 21;
#else
    // Catmull-Rom cubic spline with 4 points
    // f'(x) = 3ax^2 + 2bx + c
    //
    // p1 : f(0) = d
    // p2 : f(1) = a + b + c + d
    // (p2 - p0)/2 : f'(0) = c
    // (p3 - p1)/2 : f'(1) = 3a + 2b + c
    //
    // |  0  0  0  1 | | a |   | p1      |
    // |  1  1  1  1 | | b | = | p2      |
    // |  0  0  2  0 | | c |   | p2 - p0 |
    // |  6  4  2  0 | | d |   | p3 - p1 |
    //
    //    |  4 -4  1  1 | | p1      |   | a | 
    // 1/2| -6  6 -2 -1 | | p2      | = | b |
    //    |  0  0  1  0 | | p2 - p0 |   | c |
    //    |  2  0  0  0 | | p3 - p1 |   | d |
    //
    // a = 4*p1 - 4*p2 + (p2 - p0) + (p3 - p1) = (p3 - 3*p2 + 3*p1 - p0) / 2
    // b = -6*p1 + 6*p2 - 2*(p2 - p0) - (p3 - p1) = (-p3 + 4*p2 - 5p1 + 2*p0) / 2
    // c = (p2 - p0) / 2
    // d = p1
    int a = p3 - 3 * p2 + 3 * p1 - p0;
    int b = p2 - 2 * p1 + p0 - a;
    int c = p2 - p0;
    int d = 2 * p1;
    return (t * (t * (t * a + (b << 7)) + (c << 14)) + (d << 21)) >> 22;
#endif
}

static void ResampleNearest(const byte *src, int srcWidth, int srcHeight, byte *dst, int dstWidth, int dstHeight, int bpp) {
    int srcPitch = srcWidth * bpp;

    for (int y = 0; y < dstHeight; y++) {
        int sampleY = srcHeight * y / dstHeight;

        for (int x = 0; x < dstWidth; x++) {
            int sampleX = srcWidth * x / dstWidth;
        
            for (int i = 0; i < bpp; i++) {
                *dst++ = src[sampleY * srcPitch + sampleX * bpp + i];
            }
        }
    }
}

static void ResampleBilinear(const byte *src, int srcWidth, int srcHeight, byte *dst, int dstWidth, int dstHeight, int bpp) {
    int srcPitch = srcWidth * bpp;

    for (int y = 0; y < dstHeight; y++) {
        int iY0 = ((y * srcHeight) << 8) / dstHeight;
        int fracY = iY0 & 255;
        iY0 >>= 8;

        int iY1 = Min(iY0 + 1, srcHeight - 1) * srcPitch;
        iY0 *= srcPitch;
                
        for (int x = 0; x < dstWidth; x++) {
            int iX0 = ((x * srcWidth) << 8) / dstWidth;
            int fracX = iX0 & 255;
            iX0 >>= 8;

            int iX1 = Min(iX0 + 1, srcWidth - 1);

            int offset0 = iX0 * bpp;
            int offset1 = iX1 * bpp;
            
            const byte *srcPtr[2]; 
            srcPtr[0] = &src[iY0];
            srcPtr[1] = &src[iY1];

            for (int i = 0; i < bpp; i++) {
                int p0 = fix_lerp(srcPtr[0][offset0 + i], srcPtr[0][offset1 + i], fracX);
                int p1 = fix_lerp(srcPtr[1][offset0 + i], srcPtr[1][offset1 + i], fracX);

                *dst++ = fix_lerp(p0, p1, fracY);
            }
        }
    }
}

static void ResampleBicubic(const byte *src, int srcWidth, int srcHeight, byte *dst, int dstWidth, int dstHeight, int bpp) {
    int srcPitch = srcWidth * bpp;

    for (int y = 0; y < dstHeight; y++) {
        int iY1 = ((y * srcHeight) << 7) / dstHeight;
        int fracY = iY1 & 127;
        iY1 >>= 7;

        int iY0 = Max(iY1 - 1, 0) * srcPitch;
        int iY2 = Min(iY1 + 1, srcHeight - 1) * srcPitch;
        int iY3 = Min(iY1 + 2, srcHeight - 1) * srcPitch;
        iY1 *= srcPitch;

        for (int x = 0; x < dstWidth; x++) {
            int iX1 = ((x * srcWidth) << 7) / dstWidth;
            int fracX = iX1 & 127;
            iX1 >>= 7;

            int iX0 = Max(iX1 - 1, 0);
            int iX2 = Min(iX1 + 1, srcWidth - 1);
            int iX3 = Min(iX1 + 2, srcWidth - 1);

            int offset0 = iX0 * bpp;
            int offset1 = iX1 * bpp;
            int offset2 = iX2 * bpp;
            int offset3 = iX3 * bpp;

            const byte *srcPtr[4];
            srcPtr[0] = &src[iY0];
            srcPtr[1] = &src[iY1];
            srcPtr[2] = &src[iY2];
            srcPtr[3] = &src[iY3];

            for (int i = 0; i < bpp; i++) {
                int p0 = fix_cerp(srcPtr[0][offset0 + i], srcPtr[0][offset1 + i], srcPtr[0][offset2 + i], srcPtr[0][offset3 + i], fracX);
                int p1 = fix_cerp(srcPtr[1][offset0 + i], srcPtr[1][offset1 + i], srcPtr[1][offset2 + i], srcPtr[1][offset3 + i], fracX);
                int p2 = fix_cerp(srcPtr[2][offset0 + i], srcPtr[2][offset1 + i], srcPtr[2][offset2 + i], srcPtr[2][offset3 + i], fracX);
                int p3 = fix_cerp(srcPtr[3][offset0 + i], srcPtr[3][offset1 + i], srcPtr[3][offset2 + i], srcPtr[3][offset3 + i], fracX);

                int res = fix_cerp(p0, p1, p2, p3, fracY);
                *dst++ = Clamp((const int &)res, 0, 255);
            }
        }
    }
}

bool Image::Resize(int dstWidth, int dstHeight, Image::ResampleFilter filter, Image &dstImage) const {
    assert(width && height);
    assert(dstWidth && dstHeight);
    
    if (IsCompressed() || IsPacked() || IsFloatFormat() || depth != 1) {
        BE_WARNLOG(L"Cannot be resized format %hs\n", FormatName());
        return false;
    }

    if (width == dstWidth && height == dstHeight) {
        return true;
    }

    dstImage.Create2D(dstWidth, dstHeight, 1, format, nullptr, flags);

    int bpp = Image::BytesPerPixel(format);

    switch (filter) {
    case Nearest:
        ResampleNearest(this->pic, this->width, this->height, dstImage.pic, dstWidth, dstHeight, bpp);
        break;
    case Bilinear:
        ResampleBilinear(this->pic, this->width, this->height, dstImage.pic, dstWidth, dstHeight, bpp);
        break;
    case Bicubic:
        ResampleBicubic(this->pic, this->width, this->height, dstImage.pic, dstWidth, dstHeight, bpp);
        break;
    }
    return true;
}

bool Image::ResizeSelf(int dstWidth, int dstHeight, Image::ResampleFilter filter) {
    assert(width && height);
    assert(dstWidth && dstHeight);
    
    if (IsPacked() || IsCompressed() || IsPacked() || IsFloatFormat() || depth != 1) {
        BE_WARNLOG(L"Cannot be resized format %hs\n", FormatName());
        return false;
    }

    if (width == dstWidth && height == dstHeight) {
        return true;
    }
    
    int bpp = Image::BytesPerPixel(format);
    
    byte *dst = (byte *)Mem_Alloc16(dstWidth * dstHeight * bpp);

    switch (filter) {
    case Nearest:
        ResampleNearest(this->pic, this->width, this->height, dst, dstWidth, dstHeight, bpp);
        break;
    case Bilinear:
        ResampleBilinear(this->pic, this->width, this->height, dst, dstWidth, dstHeight, bpp);
        break;
    case Bicubic:
        ResampleBicubic(this->pic, this->width, this->height, dst, dstWidth, dstHeight, bpp);
        break;
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
