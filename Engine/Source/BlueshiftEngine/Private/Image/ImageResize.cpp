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

static void ResampleNearest(const byte *src, int srcWidth, int srcHeight, byte *dst, int dstWidth, int dstHeight, int bytesPerPixel) {
    for (int y = 0; y < dstHeight; y++) {
        int sampleY = srcHeight * y / dstHeight;

        for (int x = 0; x < dstWidth; x++) {
            int sampleX = srcWidth * x / dstWidth;
        
            for (int i = 0; i < bytesPerPixel; i++) {
                *dst++ = src[(sampleY * srcWidth + sampleX) * bytesPerPixel + i];
            }
        }
    }
}

// fixed point bilinear interpolation
// p0: p(0)
// p1: p(1)
// t: 0 ~ 255
static int fix_lerp(int p0, int p1, int t) {
    return ((p0 << 8) + (p1 - p0) * t) >> 8;
}

static void ResampleBilinear(const byte *src, int srcWidth, int srcHeight, byte *dst, int dstWidth, int dstHeight, int bpp) {
    int a, b;

    for (int y = 0; y < dstHeight; y++) {
        int intY = ((y * (srcHeight)) << 8) / (dstHeight);
        int fracY = intY & 255;
        intY >>= 8;
                
        for (int x = 0; x < dstWidth; x++) {
            int intX = ((x * (srcWidth)) << 8) / (dstWidth);
            int fracX = intX & 255;
            intX >>= 8;
            
            byte *srcPtr = (byte *)(src + (intY * srcWidth + intX) * bpp);

            for (int i = 0; i < bpp; i++) {
                if (intX < srcWidth - 1) {
                    a = fix_lerp(srcPtr[0], srcPtr[bpp], fracX);
                    b = intY < srcHeight - 1 ? fix_lerp(srcPtr[srcWidth * bpp], srcPtr[(srcWidth + 1) * bpp], fracX) : a;
                } else {
                    a = srcPtr[0];
                    b = intY < srcHeight - 1 ? srcPtr[srcWidth * bpp] : a;
                }

                *dst++ = fix_lerp(a, b, fracY);
                    
                srcPtr++;
            }
        }
    }
}

// fixed point cubic interpolation
// p0: p(-1)
// p1: p(0)
// p2: p(1)
// p3: p(2)
// t: 0 ~ 127
static int fix_cerp(int p0, int p1, int p2, int p3, int t) {
    // Coefficients of third degree polynomial 
    // f(t) = at^3 + bt^2 + ct + d
#if 0
    int a = (p3 - p2) - (p0 - p1);
    int b = (p0 - p1) - a;
    int c = p2 - p0;
    int d = p1;
    return (t * (t * (t * a + (b << 7)) + (c << 14)) + (d << 21)) >> 21;
#else
    int a = p3 - 3 * p2 + 3 * p1 - p0;
    int b = p2 - 2 * p1 + p0 - a;
    int c = p2 - p0;
    int d = 2 * p1;
    return (t * (t * (t * a + (b << 7)) + (c << 14)) + (d << 21)) >> 22;
#endif
}

static void ResampleBicubic(const byte *src, int srcWidth, int srcHeight, byte *dst, int dstWidth, int dstHeight, int bpp) {
    int srcPitch = srcWidth * bpp;

    for (int y = 0; y < dstHeight; y++) {
        int intY1 = ((y * (srcHeight)) << 7) / (dstHeight);
        int fracY = intY1 & 127;
        intY1 >>= 7;

        int intY0 = Max(intY1 - 1, 0);
        int intY2 = Min(intY1 + 1, srcHeight - 1);
        int intY3 = Min(intY1 + 2, srcHeight - 1);

        for (int x = 0; x < dstWidth; x++) {
            int intX1 = ((x * (srcWidth)) << 7) / (dstWidth);
            int fracX = intX1 & 127;
            intX1 >>= 7;

            int intX0 = Max(intX1 - 1, 0);
            int intX2 = Min(intX1 + 1, srcWidth - 1);
            int intX3 = Min(intX1 + 2, srcWidth - 1);

            int offset0 = intX0 * bpp;
            int offset1 = intX1 * bpp;
            int offset2 = intX2 * bpp;
            int offset3 = intX3 * bpp;

            const byte *srcPtr[4];
            srcPtr[0] = &src[intY0 * srcPitch];
            srcPtr[1] = &src[intY1 * srcPitch];
            srcPtr[2] = &src[intY2 * srcPitch];
            srcPtr[3] = &src[intY3 * srcPitch];

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
    
    if (IsCompressed() || IsFloatFormat() || depth != 1) {
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
    
    if (IsPacked() || IsCompressed() || IsFloatFormat() || depth != 1) {
        BE_LOG(L"Cannot be resized format %hs\n", FormatName());
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

    if (this->allocated) {
        Mem_AlignedFree(this->pic);
    }

    this->pic = dst;
    this->width = dstWidth;
    this->height = dstHeight;

    return true;
}

BE_NAMESPACE_END
