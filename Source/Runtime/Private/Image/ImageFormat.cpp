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
#include "Math/Math.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
//
// XXXToRGBA8888 (unpacking function from custom format to rgba8888)
//
//--------------------------------------------------------------------------------------------------

static void L8ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[0];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 255;
    }
}

static void A8ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = 255;
        dstPtr[1] = 255;
        dstPtr[2] = 255;
        dstPtr[3] = srcPtr[0];
    }
}

static void LA88ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 2;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[0];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = srcPtr[1];
    }
}

static void LA1616ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels * 2;
    byte *dstPtr = dst;
    byte l, a;

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        l = srcPtr[0] >> 8;
        a = srcPtr[1] >> 8;
        dstPtr[0] = l;
        dstPtr[1] = l;
        dstPtr[2] = l;
        dstPtr[3] = a;
    }
}

static void R8ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 255;
    }
}

static void RG88ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 2;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 255;
    }
}

static void RGB888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 3;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 255;
    }
}

static void BGR888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 3;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 255;
    }
}

static void RGBX8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 255;
    }
}

static void BGRX8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 255;
    }
}

static void RGBA8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    memcpy(dst, src, 4 * numPixels);
}

static void BGRA8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = srcPtr[3];
    }
}

static void ABGR8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[3];
        dstPtr[1] = srcPtr[2];
        dstPtr[2] = srcPtr[1];
        dstPtr[3] = srcPtr[0];
    }
}

static void ARGB8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[1];
        dstPtr[1] = srcPtr[2];
        dstPtr[2] = srcPtr[3];
        dstPtr[3] = srcPtr[0];
    }
}

static void RGBX4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dstPtr[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[2] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[3] = 255;
    }
}

static void BGRX4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[2] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dstPtr[3] = 255;
    }
}

static void RGBA4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dstPtr[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[2] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[3] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
    }
}

static void BGRA4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[2] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dstPtr[3] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
    }
}

static void ABGR4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
        dstPtr[1] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[2] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[3] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
    }
}

static void ARGB4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dstPtr[1] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dstPtr[2] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
        dstPtr[3] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
    }
}

static void RGBX5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dstPtr[2] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dstPtr[3] = 255;
    }
}

static void BGRX5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dstPtr[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dstPtr[2] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[3] = 255;
    }
}

static void RGBA5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dstPtr[2] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dstPtr[3] = ((int16_t)*srcPtr >> 15) & 0xFF;
    }
}

static void BGRA5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dstPtr[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dstPtr[2] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[3] = ((int16_t)*srcPtr >> 15) & 0xFF;
    }
}

static void ABGR1555ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 13) & 0x7);
        dstPtr[1] = ((*srcPtr >> 3) & 0xF8) | ((*srcPtr >> 8) & 0x7);
        dstPtr[2] = ((*srcPtr << 2) & 0xF8) | ((*srcPtr >> 3) & 0x7);
        dstPtr[3] = (*srcPtr & 1) ? 0xFF : 0;
    }
}

static void ARGB1555ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 2) & 0xF8) | ((*srcPtr >> 3) & 0x7);
        dstPtr[1] = ((*srcPtr >> 3) & 0xF8) | ((*srcPtr >> 8) & 0x7);
        dstPtr[2] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 13) & 0x7);
        dstPtr[3] = (*srcPtr & 1) ? 0xFF : 0;
    }
}

static void RGB565ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[1] = ((*srcPtr >> 3) & 0xFC) | ((*srcPtr >> 9) & 0x3);
        dstPtr[2] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 14) & 0x7);
        dstPtr[3] = 255;
    }
}

static void BGR565ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 14) & 0x7);
        dstPtr[1] = ((*srcPtr >> 3) & 0xFC) | ((*srcPtr >> 9) & 0x3);
        dstPtr[2] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dstPtr[3] = 255;
    }
}

static void RGBA32FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[3]);
    }
}

//--------------------------------------------------------------------------------------------------
//
// RGBA8888ToXXX (packing function from rgba8888 to custom format)
//
//--------------------------------------------------------------------------------------------------

static void RGBA8888ToL8(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = (byte)(0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]);
    }
}

static void RGBA8888ToA8(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = srcPtr[3];
    }
}

static void RGBA8888ToLA88(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = (byte)(0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]);
        dstPtr[1] = srcPtr[3];
    }
}

static void RGBA8888ToLA1616(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = (((uint16_t)(0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2])) << 8);
        dstPtr[1] = (((uint16_t)srcPtr[3]) << 8);
    }
}

static void RGBA8888ToR8(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = srcPtr[0];
    }
}

static void RGBA8888ToRG88(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
    }
}

static void RGBA8888ToRGB888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
    }
}

static void RGBA8888ToBGR888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
    }
}

static void RGBA8888ToRGBX8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 0;
    }
}

static void RGBA8888ToBGRX8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = 0;
    }
}

static void RGBA8888ToBGRA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[0];
        dstPtr[3] = srcPtr[3];
    }
}

static void RGBA8888ToABGR8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[3];
        dstPtr[1] = srcPtr[2];
        dstPtr[2] = srcPtr[1];
        dstPtr[3] = srcPtr[0];
    }
}

static void RGBA8888ToARGB8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[1];
        dstPtr[1] = srcPtr[2];
        dstPtr[2] = srcPtr[3];
        dstPtr[3] = srcPtr[0];
    }
}

static void RGBA8888ToRGBX4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 4) | ((srcPtr[1] >> 4) << 4) | ((srcPtr[2] >> 4) << 8);
    }
}

static void RGBA8888ToBGRX4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[2] >> 4) | ((srcPtr[1] >> 4) << 4) | ((srcPtr[0] >> 4) << 8);
    }
}

static void RGBA8888ToRGBA4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 4) | ((srcPtr[1] >> 4) << 4) | ((srcPtr[2] >> 4) << 8) | ((srcPtr[3] >> 4) << 12);
    }
}

static void RGBA8888ToBGRA4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[2] >> 4) | ((srcPtr[1] >> 4) << 4) | ((srcPtr[0] >> 4) << 8) | ((srcPtr[3] >> 4) << 12);
    }
}

static void RGBA8888ToABGR4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[3] >> 4) | ((srcPtr[2] >> 4) << 4) | ((srcPtr[1] >> 4) << 8) | ((srcPtr[0] >> 4) << 12);
    }
}

static void RGBA8888ToARGB4444(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[1] >> 4) | ((srcPtr[2] >> 4) << 4) | ((srcPtr[3] >> 4) << 8) | ((srcPtr[0] >> 4) << 12);
    }
}

static void RGBA8888ToRGBX5551(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 3) | ((srcPtr[1] >> 3) << 5) | ((srcPtr[2] >> 3) << 10);
    }
}

static void RGBA8888ToBGRX5551(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[2] >> 3) | ((srcPtr[1] >> 3) << 5) | ((srcPtr[0] >> 3) << 10);
    }
}

static void RGBA8888ToRGBA5551(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 3) | ((srcPtr[1] >> 3) << 5) | ((srcPtr[2] >> 3) << 10) | ((srcPtr[3] >> 7) << 15);
    }
}

static void RGBA8888ToBGRA5551(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[2] >> 3) | ((srcPtr[1] >> 3) << 5) | ((srcPtr[0] >> 3) << 10) | ((srcPtr[3] >> 7) << 15);
    }
}

static void RGBA8888ToABGR1555(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[3] >> 3) | ((srcPtr[2] >> 3) << 5) | ((srcPtr[1] >> 3) << 10) | ((srcPtr[0] >> 7) << 15);
    }
}

static void RGBA8888ToARGB1555(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[1] >> 3) | ((srcPtr[2] >> 3) << 5) | ((srcPtr[3] >> 3) << 10) | ((srcPtr[0] >> 7) << 15);
    }
}

static void RGBA8888ToRGB565(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[0] >> 3) | ((srcPtr[1] >> 2) << 5) | ((srcPtr[2] >> 3) << 11);
    }
}

static void RGBA8888ToBGR565(const byte *src, byte *dst, int numPixels) {
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (srcPtr[2] >> 3) | ((srcPtr[1] >> 2) << 5) | ((srcPtr[0] >> 3) << 11);
    }
}

static void RGBA8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[2] * invNorm;
        dstPtr[3] = srcPtr[3] * invNorm;
    }
}

//--------------------------------------------------------------------------------------------------
//
// XXXToRGBA32F (unpacking function from custom format to rgba32f)
//
//--------------------------------------------------------------------------------------------------

static void L8ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void A8ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = 1.0f;
        dstPtr[3] = srcPtr[0] * invNorm;
    }
}

static void LA88ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = srcPtr[1] * invNorm;
    }
}

static void L16ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 65535.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);
    float l;

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        l = srcPtr[0] * invNorm;
        dstPtr[0] = l;
        dstPtr[1] = l;
        dstPtr[2] = l;
        dstPtr[3] = 1.0f;
    }
}

static void LA1616ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 65535.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);
    float l, a;

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        l = srcPtr[0] * invNorm;
        a = srcPtr[1] * invNorm;
        dstPtr[0] = l;
        dstPtr[1] = l;
        dstPtr[2] = l;
        dstPtr[3] = a;
    }
}

static void R8ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void R8SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float inv127 = 1.0f / 127.0f;
    const float inv128 = 1.0f / 128.0f;
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] < 0 ? Max(srcPtr[0] * inv128, -1.0f) : srcPtr[0] * inv127;
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void R8SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void R8UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint8_t *srcPtr = reinterpret_cast<const uint8_t *>(src);
    const uint8_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RG88ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RG88SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float inv127 = 1.0f / 127.0f;
    const float inv128 = 1.0f / 128.0f;
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] < 0 ? Max(srcPtr[0] * inv128, -1.0f) : srcPtr[0] * inv127;
        dstPtr[1] = srcPtr[1] < 0 ? Max(srcPtr[1] * inv128, -1.0f) : srcPtr[1] * inv127;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RG88SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RG88UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint8_t *srcPtr = reinterpret_cast<const uint8_t *>(src);
    const uint8_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RGB888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[2] * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void RGB888SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float inv127 = 1.0f / 127.0f;
    const float inv128 = 1.0f / 128.0f;
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] < 0 ? Max(srcPtr[0] * inv128, -1.0f) : srcPtr[0] * inv127;
        dstPtr[1] = srcPtr[1] < 0 ? Max(srcPtr[1] * inv128, -1.0f) : srcPtr[1] * inv127;
        dstPtr[2] = srcPtr[2] < 0 ? Max(srcPtr[2] * inv128, -1.0f) : srcPtr[2] * inv127;
        dstPtr[3] = 1.0f;
    }
}

static void RGB888SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void RGB888UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint8_t *srcPtr = reinterpret_cast<const uint8_t *>(src);
    const uint8_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void BGR888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[2] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void BGRX8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void BGRA8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[2] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[0] * invNorm;
        dstPtr[3] = srcPtr[3] * invNorm;
    }
}

static void ABGR8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[3] * invNorm;
        dstPtr[1] = srcPtr[2] * invNorm;
        dstPtr[2] = srcPtr[1] * invNorm;
        dstPtr[3] = srcPtr[0] * invNorm;
    }
}

static void ARGB8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[1] * invNorm;
        dstPtr[1] = srcPtr[2] * invNorm;
        dstPtr[2] = srcPtr[3] * invNorm;
        dstPtr[3] = srcPtr[0] * invNorm;
    }
}

static void RGBA1010102ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float inv1023 = 1.0f / 1023.0f;
    const float inv3 = 1.0f / 3.0f;
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = (*srcPtr & 0x3FF) * inv1023;
        dstPtr[1] = ((*srcPtr >> 10) & 0x3FF) * inv1023;
        dstPtr[2] = ((*srcPtr >> 20) & 0x3FF) * inv1023;
        dstPtr[3] = ((*srcPtr >> 30) & 0x3) * inv3;
    }
}

static void RGBA1010102UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = (*srcPtr & 0x3FF);
        dstPtr[1] = ((*srcPtr >> 10) & 0x3FF);
        dstPtr[2] = ((*srcPtr >> 20) & 0x3FF);
        dstPtr[3] = ((*srcPtr >> 30) & 0x3);
    }
}

static void RGBA8888SNormToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float inv127 = 1.0f / 127.0f;
    const float inv128 = 1.0f / 128.0f;
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] < 0 ? Max(srcPtr[0] * inv128, -1.0f) : srcPtr[0] * inv127;
        dstPtr[1] = srcPtr[1] < 0 ? Max(srcPtr[1] * inv128, -1.0f) : srcPtr[1] * inv127;
        dstPtr[2] = srcPtr[2] < 0 ? Max(srcPtr[2] * inv128, -1.0f) : srcPtr[2] * inv127;
        dstPtr[3] = srcPtr[3] < 0 ? Max(srcPtr[3] * inv128, -1.0f) : srcPtr[3] * inv127;
    }
}

static void RGBA8888SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int8_t *srcPtr = reinterpret_cast<const int8_t *>(src);
    const int8_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
}

static void RGBA8888UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint8_t *srcPtr = reinterpret_cast<const uint8_t *>(src);
    const uint8_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
}

static void RGBX8888ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcPtr = src;
    const byte *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0] * invNorm;
        dstPtr[1] = srcPtr[1] * invNorm;
        dstPtr[2] = srcPtr[2] * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void RGBE9995ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);
    float m;

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        m = Math::Pow(2, ((*srcPtr >> 27) & 0x1F) - 24);
        dstPtr[0] = (*srcPtr & 0x1FF) * m;
        dstPtr[1] = ((*srcPtr >> 9) & 0x1FF) * m;
        dstPtr[2] = ((*srcPtr >> 18) & 0x1FF) * m;
        dstPtr[3] = 1.0f;
    }
}

static void RGBX4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 0) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void BGRX4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 0) & 0xF) * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void RGBA4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 0) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[3] = ((*srcPtr >> 12) & 0xF) * invNorm;
    }
}

static void BGRA4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 0) & 0xF) * invNorm;
        dstPtr[3] = ((*srcPtr >> 12) & 0xF) * invNorm;
    }
}

static void ABGR4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 12) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[3] = ((*srcPtr >> 0) & 0xF) * invNorm;
    }
}

static void ARGB4444ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 15.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 4) & 0xF) * invNorm;
        dstPtr[1] = ((*srcPtr >> 8) & 0xF) * invNorm;
        dstPtr[2] = ((*srcPtr >> 12) & 0xF) * invNorm;
        dstPtr[3] = ((*srcPtr >> 0) & 0xF) * invNorm;
    }
}

static void RGBX5551ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 0) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 5) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 10) & 0x1F) * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void BGRX5551ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 10) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 5) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 0) & 0x1F) * invNorm;
        dstPtr[3] = 1.0f;
    }
}

static void RGBA5551ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 0) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 5) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 10) & 0x1F) * invNorm;
        dstPtr[3] = ((*srcPtr >> 15));
    }
}

static void BGRA5551ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 10) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 5) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 0) & 0x1F) * invNorm;
        dstPtr[3] = ((*srcPtr >> 15));
    }
}

static void ABGR1555ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 11) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 6) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 1) & 0x1F) * invNorm;
        dstPtr[3] = ((*srcPtr >> 0) & 0x1);
    }
}

static void ARGB1555ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 31.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 1) & 0x1F) * invNorm;
        dstPtr[1] = ((*srcPtr >> 6) & 0x1F) * invNorm;
        dstPtr[2] = ((*srcPtr >> 11) & 0x1F) * invNorm;
        dstPtr[3] = ((*srcPtr >> 0) & 0x1);
    }
}

static void RGB565ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm5 = 1.0f / 31.0f;
    const float invNorm6 = 1.0f / 63.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 0) & 0x1F) * invNorm5;
        dstPtr[1] = ((*srcPtr >> 5) & 0x3F) * invNorm6;
        dstPtr[2] = ((*srcPtr >> 11) & 0x1F) * invNorm5;
        dstPtr[3] = 1.0f;
    }
}

static void BGR565ToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm5 = 1.0f / 31.0f;
    const float invNorm6 = 1.0f / 63.0f;
    const uint16_t *srcPtr = reinterpret_cast<const uint16_t *>(src);
    const uint16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr++, dstPtr += 4) {
        dstPtr[0] = ((*srcPtr >> 11) & 0x1F) * invNorm5;
        dstPtr[1] = ((*srcPtr >> 5) & 0x3F) * invNorm6;
        dstPtr[2] = ((*srcPtr >> 0) & 0x1F) * invNorm5;
        dstPtr[3] = 1.0f;
    }
}

static void L16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[3] = 1.0f;
    }
}

static void A16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = 1.0f;
        dstPtr[1] = 1.0f;
        dstPtr[2] = 1.0f;
        dstPtr[3] = F16Converter::ToF32(srcPtr[0]);
    }
}

static void LA16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[3] = F16Converter::ToF32(srcPtr[1]);
    }
}

static void R16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RG16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[1] = F16Converter::ToF32(srcPtr[1]);
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RGB16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[1] = F16Converter::ToF32(srcPtr[1]);
        dstPtr[2] = F16Converter::ToF32(srcPtr[2]);
        dstPtr[3] = 1.0f;
    }
}

static void RGBA16FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcPtr = reinterpret_cast<const float16_t *>(src);
    const float16_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = F16Converter::ToF32(srcPtr[0]);
        dstPtr[1] = F16Converter::ToF32(srcPtr[1]);
        dstPtr[2] = F16Converter::ToF32(srcPtr[2]);
        dstPtr[3] = F16Converter::ToF32(srcPtr[3]);
    }
}

static void L32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = srcPtr[0];
        dstPtr[3] = 1.0f;
    }
}

static void A32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = 1.0f;
        dstPtr[1] = 1.0f;
        dstPtr[2] = 1.0f;
        dstPtr[3] = srcPtr[0];
    }
}

static void LA32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = dstPtr[1] = dstPtr[2] = srcPtr[0];
        dstPtr[3] = srcPtr[1];
    }
}

static void R32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void R32SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int32_t *srcPtr = reinterpret_cast<const int32_t *>(src);
    const int32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void R32UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = 0;
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RG32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RG32SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int32_t *srcPtr = reinterpret_cast<const int32_t *>(src);
    const int32_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RG32UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels * 2;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 2, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = 0;
        dstPtr[3] = 1.0f;
    }
}

static void RGB32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void RGB32SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int32_t *srcPtr = reinterpret_cast<const int32_t *>(src);
    const int32_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void RGB32UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels * 3;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 3, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = 1.0f;
    }
}

static void RGBA32FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    memcpy(dst, src, sizeof(float) * 4 * numPixels);
}

static void RGBA32FToRGBA32SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int32_t *dstPtr = reinterpret_cast<int32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Clamp<float>(Math::Round(srcPtr[0]), INT32_MIN, INT32_MAX);
        dstPtr[1] = Clamp<float>(Math::Round(srcPtr[1]), INT32_MIN, INT32_MAX);
        dstPtr[2] = Clamp<float>(Math::Round(srcPtr[2]), INT32_MIN, INT32_MAX);
        dstPtr[3] = Clamp<float>(Math::Round(srcPtr[3]), INT32_MIN, INT32_MAX);
    }
}

static void RGBA32FToRGBA32UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Clamp<float>(Math::Round(srcPtr[0]), 0, UINT32_MAX);
        dstPtr[1] = Clamp<float>(Math::Round(srcPtr[1]), 0, UINT32_MAX);
        dstPtr[2] = Clamp<float>(Math::Round(srcPtr[2]), 0, UINT32_MAX);
        dstPtr[3] = Clamp<float>(Math::Round(srcPtr[3]), 0, UINT32_MAX);
    }
}

static void RGBA32SIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const int32_t *srcPtr = reinterpret_cast<const int32_t *>(src);
    const int32_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
}

static void RGBA32UIntToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
        dstPtr[3] = srcPtr[3];
    }
}

static void RGB11F11F10FToRGBA32F(const byte *src, byte *dst, int numPixels) {
    const uint32_t *srcPtr = reinterpret_cast<const uint32_t *>(src);
    const uint32_t *srcEnd = srcPtr + numPixels;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 1, dstPtr += 4) {
        dstPtr[0] = F11Converter::ToF32(*srcPtr & 0x7FF);
        dstPtr[1] = F11Converter::ToF32((*srcPtr >> 11) & 0x7FF);
        dstPtr[2] = F10Converter::ToF32((*srcPtr >> 22) & 0x3FF);
        dstPtr[3] = 1.0f;
    }
}

//--------------------------------------------------------------------------------------------------
//
// RGBA32FToXXX (packing function from rgba32f to custom format)
//
//--------------------------------------------------------------------------------------------------

static void RGBA32FToL8(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Math::Ftob(255.0f * (0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]));
    }
}

static void RGBA32FToA8(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[3]);
    }
}

static void RGBA32FToLA88(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftob(255.0f * (0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]));
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[3]);
    }
}

static void RGBA32FToL16(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Clamp<int>((0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]) * 65535.0f, 0, 65535);
    }
}

static void RGBA32FToLA1616(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Clamp<int>((0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]) * 65535.0f, 0, 65535);
        dstPtr[1] = Clamp<int>(srcPtr[3] * 65535.0f, 0, 65535);
    }
}

static void RGBA32FToR8(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
    }
}

static void RGBA32FToRG88(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
    }
}

static void RGBA32FToRGB888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[2]);
    }
}

static void RGBA32FToBGR888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[0]);
    }
}

static void RGBA32FToRGBX8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[3] = 255;
    }
}

static void RGBA32FToBGRX8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[3] = 255;
    }
}

static void RGBA32FToBGRA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[3]);
    }
}

static void RGBA32FToABGR8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[3]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[2]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[0]);
    }
}

static void RGBA32FToARGB8888(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    byte *dstPtr = dst;

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(255.0f * srcPtr[3]);
        dstPtr[1] = Math::Ftob(255.0f * srcPtr[0]);
        dstPtr[2] = Math::Ftob(255.0f * srcPtr[1]);
        dstPtr[3] = Math::Ftob(255.0f * srcPtr[2]);
    }
}

static void RGBA32FToRGBA1010102(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = Math::Ftoi(1023.0f * Clamp01(srcPtr[0])) | (Math::Ftoi(1023.0f * Clamp01(srcPtr[1])) << 10) | (Math::Ftoi(1023.0f * Clamp01(srcPtr[2])) << 20) | (Math::Ftoi(3.0f * Clamp01(srcPtr[3])) << 30);
    }
}

static void RGBA32FToRGBA1010102UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = Math::Ftoi(Clamp<float>(srcPtr[0], 0, 1023)) | (Math::Ftoi(Clamp<float>(srcPtr[1], 0, 1023)) << 10) | (Math::Ftoi(Clamp<float>(srcPtr[2], 0, 1023)) << 20) | (Math::Ftoi(Clamp<float>(srcPtr[3], 0, 3)) << 30);
    }
}

static void RGBA32FToR8SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
    }
}

static void RGBA32FToR8SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Math::Ftoi8(Clamp(Math::Round(srcPtr[0]), -128.0f, 127.0f));
    }
}

static void RGBA32FToR8UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint8_t *dstPtr = reinterpret_cast<uint8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Math::Ftob(Clamp(Math::Round(srcPtr[0]), 0.0f, 255.0f));
    }
}

static void RGBA32FToRG88SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
    }
}

static void RGBA32FToRG88SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[0]), INT8_MIN, INT8_MAX));
        dstPtr[1] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[1]), INT8_MIN, INT8_MAX));
    }
}

static void RGBA32FToRG88UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint8_t *dstPtr = reinterpret_cast<uint8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[0]), 0.0f, UINT8_MAX));
        dstPtr[1] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[1]), 0.0f, UINT8_MAX));
    }
}

static void RGBA32FToRGB888SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
        dstPtr[2] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[2], -1.0f, 1.0f)));
    }
}

static void RGBA32FToRGB888SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[0]), INT8_MIN, INT8_MAX));
        dstPtr[1] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[1]), INT8_MIN, INT8_MAX));
        dstPtr[2] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[2]), INT8_MIN, INT8_MAX));
    }
}

static void RGBA32FToRGB888UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint8_t *dstPtr = reinterpret_cast<uint8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[0]), 0.0f, UINT8_MAX));
        dstPtr[1] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[1]), 0.0f, UINT8_MAX));
        dstPtr[2] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[2]), 0.0f, UINT8_MAX));
    }
}

static void RGBA32FToRGBA8888SNorm(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[0], -1.0f, 1.0f)));
        dstPtr[1] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[1], -1.0f, 1.0f)));
        dstPtr[2] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[2], -1.0f, 1.0f)));
        dstPtr[3] = Math::Ftoi8(Math::Floor((255.0f / 2.0f) * Clamp(srcPtr[3], -1.0f, 1.0f)));
    }
}

static void RGBA32FToRGBA8888SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int8_t *dstPtr = reinterpret_cast<int8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[0]), INT8_MIN, INT8_MAX));
        dstPtr[1] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[1]), INT8_MIN, INT8_MAX));
        dstPtr[2] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[2]), INT8_MIN, INT8_MAX));
        dstPtr[3] = Math::Ftoi8(Clamp<float>(Math::Round(srcPtr[3]), INT8_MIN, INT8_MAX));
    }
}

static void RGBA32FToRGBA8888UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint8_t *dstPtr = reinterpret_cast<uint8_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[0]), 0.0f, UINT8_MAX));
        dstPtr[1] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[1]), 0.0f, UINT8_MAX));
        dstPtr[2] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[2]), 0.0f, UINT8_MAX));
        dstPtr[3] = Math::Ftob(Clamp<float>(Math::Round(srcPtr[3]), 0.0f, UINT8_MAX));
    }
}

static void RGBA32FToRGBE9995(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = RGBE9995::FromColor3(srcPtr[0], srcPtr[1], srcPtr[1]);
    }
}

static void RGBA32FToRGBX4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 0) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 8);
    }
}

static void RGBA32FToBGRX4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 8) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 0);
    }
}

static void RGBA32FToRGBA4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 0) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 8) | (Math::Ftoi(15.0f * Clamp01(srcPtr[3])) << 12);
    }
}

static void RGBA32FToBGRA4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 8) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 0) | (Math::Ftoi(15.0f * Clamp01(srcPtr[3])) << 12);
    }
}

static void RGBA32FToABGR4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 12) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 8) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[3])) << 0);
    }
}

static void RGBA32FToARGB4444(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(15.0f * Clamp01(srcPtr[0])) << 4) | (Math::Ftoi(15.0f * Clamp01(srcPtr[1])) << 8) | (Math::Ftoi(15.0f * Clamp01(srcPtr[2])) << 12) | (Math::Ftoi(15.0f * Clamp01(srcPtr[3])) << 0);
    }
}

static void RGBA32FToRGBX5551(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 0) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 10);
    }
}

static void RGBA32FToBGRX5551(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 10) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 0);
    }
}

static void RGBA32FToRGBA5551(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 0) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 10) | (srcPtr[3] >= 0.5f ? (1<<15) : 0);
    }
}

static void RGBA32FToBGRA5551(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 10) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 0) | (srcPtr[3] >= 0.5f ? (1 << 15) : 0);
    }
}

static void RGBA32FToABGR1555(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 11) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 6) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 1) | (srcPtr[3] >= 0.5f ? 1 : 0);
    }
}

static void RGBA32FToARGB1555(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 1) | (Math::Ftoi(31.0f * Clamp01(srcPtr[1])) << 6) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 11) | (srcPtr[3] >= 0.5f ? 1 : 0);
    }
}

static void RGBA32FToRGB565(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 0) | (Math::Ftoi(63.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 11);
    }
}

static void RGBA32FToBGR565(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint16_t *dstPtr = reinterpret_cast<uint16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        *dstPtr = (Math::Ftoi(31.0f * Clamp01(srcPtr[0])) << 11) | (Math::Ftoi(63.0f * Clamp01(srcPtr[1])) << 5) | (Math::Ftoi(31.0f * Clamp01(srcPtr[2])) << 0);
    }
}

static void RGBA32FToL16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = F16Converter::FromF32(0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]);
    }
}

static void RGBA32FToA16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = F16Converter::FromF32(srcPtr[3]);
    }
}

static void RGBA32FToLA16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = F16Converter::FromF32(0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2]);
        dstPtr[1] = F16Converter::FromF32(srcPtr[3]);
    }
}

static void RGBA32FToR16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = F16Converter::FromF32(srcPtr[0]);
    }
}

static void RGBA32FToRG16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = F16Converter::FromF32(srcPtr[0]);
        dstPtr[1] = F16Converter::FromF32(srcPtr[1]);
    }
}

static void RGBA32FToRGB16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = F16Converter::FromF32(srcPtr[0]);
        dstPtr[1] = F16Converter::FromF32(srcPtr[1]);
        dstPtr[2] = F16Converter::FromF32(srcPtr[2]);
    }
}

static void RGBA32FToRGBA16F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float16_t *dstPtr = reinterpret_cast<float16_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 4) {
        dstPtr[0] = F16Converter::FromF32(srcPtr[0]);
        dstPtr[1] = F16Converter::FromF32(srcPtr[1]);
        dstPtr[2] = F16Converter::FromF32(srcPtr[2]);
        dstPtr[3] = F16Converter::FromF32(srcPtr[3]);
    }
}

static void RGBA32FToL32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = 0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2];
    }
}

static void RGBA32FToA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = srcPtr[3];
    }
}

static void RGBA32FToLA32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = 0.299f * srcPtr[0] + 0.587f * srcPtr[1] + 0.114f * srcPtr[2];
        dstPtr[1] = srcPtr[3];
    }
}

static void RGBA32FToR32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = srcPtr[0];
    }
}

static void RGBA32FToR32SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int32_t *dstPtr = reinterpret_cast<int32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Clamp<float>(Math::Round(srcPtr[0]), INT32_MIN, INT32_MAX);
    }
}

static void RGBA32FToR32UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 1) {
        dstPtr[0] = Clamp<float>(Math::Round(srcPtr[0]), 0, UINT32_MAX);
    }
}

static void RGBA32FToRG32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
    }
}

static void RGBA32FToRG32SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int32_t *dstPtr = reinterpret_cast<int32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Clamp<float>(Math::Round(srcPtr[0]), INT32_MIN, INT32_MAX);
        dstPtr[1] = Clamp<float>(Math::Round(srcPtr[1]), INT32_MIN, INT32_MAX);
    }
}

static void RGBA32FToRG32UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 2) {
        dstPtr[0] = Clamp<float>(Math::Round(srcPtr[0]), 0, UINT32_MAX);
        dstPtr[1] = Clamp<float>(Math::Round(srcPtr[1]), 0, UINT32_MAX);
    }
}

static void RGBA32FToRGB32F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    float *dstPtr = reinterpret_cast<float *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = srcPtr[0];
        dstPtr[1] = srcPtr[1];
        dstPtr[2] = srcPtr[2];
    }
}

static void RGBA32FToRGB32SInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    int32_t *dstPtr = reinterpret_cast<int32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Clamp<float>(Math::Round(srcPtr[0]), INT32_MIN, INT32_MAX);
        dstPtr[1] = Clamp<float>(Math::Round(srcPtr[1]), INT32_MIN, INT32_MAX);
        dstPtr[2] = Clamp<float>(Math::Round(srcPtr[2]), INT32_MIN, INT32_MAX);
    }
}

static void RGBA32FToRGB32UInt(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr += 3) {
        dstPtr[0] = Clamp<float>(Math::Round(srcPtr[0]), 0, UINT32_MAX);
        dstPtr[1] = Clamp<float>(Math::Round(srcPtr[1]), 0, UINT32_MAX);
        dstPtr[2] = Clamp<float>(Math::Round(srcPtr[2]), 0, UINT32_MAX);
    }
}

static void RGBA32FToRGB11F11F10F(const byte *src, byte *dst, int numPixels) {
    const float *srcPtr = reinterpret_cast<const float *>(src);
    const float *srcEnd = srcPtr + numPixels * 4;
    uint32_t *dstPtr = reinterpret_cast<uint32_t *>(dst);

    for (; srcPtr < srcEnd; srcPtr += 4, dstPtr++) {
        uint32_t r = F11Converter::FromF32(srcPtr[0]);
        uint32_t g = F11Converter::FromF32(srcPtr[1]);
        uint32_t b = F10Converter::FromF32(srcPtr[2]);
        *dstPtr = r | (g << 11) | (b << 22);
    }
}

static const ImageFormatInfo imageFormatInfo[] = {
    // +------------------------------------------------------------------------------------------------------------ name
    // |                        +----------------------------------------------------------------------------------- size (bytes per pixel or bytes per block)
    // |                        |   +------------------------------------------------------------------------------- number of components
    // |                        |   |   +--------------------------------------------------------------------------- r bits
    // |                        |   |   |   +----------------------------------------------------------------------- g bits
    // |                        |   |   |   |   +------------------------------------------------------------------- b bits
    // |                        |   |   |   |   |   +--------------------------------------------------------------- a bits
    // |                        |   |   |   |   |   |   +----------------------------------------------------------- type
    // |                        |   |   |   |   |   |   |                        +---------------------------------- unpackRGBA8888
    // |                        |   |   |   |   |   |   |                        |        +------------------------- packRGBA8888
    // |                        |   |   |   |   |   |   |                        |        |        +---------------- unpackRGBA32F
    // |                        |   |   |   |   |   |   |                        |        |        |        +------- packRGBA32F
    // |                        |   |   |   |   |   |   |                        |        |        |        |
    { "Unknown",                0,  0,  0,  0,  0,  0,  Image::FormatType::None, nullptr, nullptr, nullptr, nullptr },
    { "R32_FLOAT",              4,  1,  32, 0,  0,  0,  Image::FormatType::Float, nullptr, nullptr, R32FToRGBA32F, RGBA32FToR32F },
    { "R32_SINT",               4,  1,  32, 0,  0,  0,  Image::FormatType::SInt, nullptr, nullptr, R32SIntToRGBA32F, RGBA32FToR32SInt },
    { "R32_UINT",               4,  1,  32, 0,  0,  0,  Image::FormatType::UInt, nullptr, nullptr, R32UIntToRGBA32F, RGBA32FToR32UInt },
    { "R32G32_FLOAT",           8,  2,  32, 32, 0,  0,  Image::FormatType::Float, nullptr, nullptr, RG32FToRGBA32F, RGBA32FToRG32F },
    { "R32G32_SINT",            8,  2,  32, 32, 0,  0,  Image::FormatType::SInt, nullptr, nullptr, RG32SIntToRGBA32F, RGBA32FToRG32SInt },
    { "R32G32_UINT",            8,  2,  32, 32, 0,  0,  Image::FormatType::UInt, nullptr, nullptr, RG32UIntToRGBA32F, RGBA32FToRG32UInt },
    { "R32G32B32_FLOAT",        12, 3,  32, 32, 32, 0,  Image::FormatType::Float, nullptr, nullptr, RGB32FToRGBA32F, RGBA32FToRGB32F },
    { "R32G32B32_SINT",         12, 3,  32, 32, 32, 0,  Image::FormatType::SInt, nullptr, nullptr, RGB32SIntToRGBA32F, RGBA32FToRGB32SInt },
    { "R32G32B32_UINT",         12, 3,  32, 32, 32, 0,  Image::FormatType::UInt, nullptr, nullptr, RGB32UIntToRGBA32F, RGBA32FToRGB32UInt },
    { "R32G32B32A32_FLOAT",     16, 4,  32, 32, 32, 32, Image::FormatType::Float, nullptr, nullptr, RGBA32FToRGBA32F, RGBA32FToRGBA32F },
    { "R32G32B32A32_SINT",      16, 4,  32, 32, 32, 32, Image::FormatType::SInt, nullptr, nullptr, RGBA32SIntToRGBA32F, RGBA32FToRGBA32SInt },
    { "R32G32B32A32_UINT",      16, 4,  32, 32, 32, 32, Image::FormatType::UInt, nullptr, nullptr, RGBA32UIntToRGBA32F, RGBA32FToRGBA32UInt },
    { "R16_FLOAT",              2,  1,  16, 0,  0,  0,  Image::FormatType::Float, nullptr, nullptr, R16FToRGBA32F, RGBA32FToR16F },
    { "R16G16_FLOAT",           4,  2,  16, 16, 0,  0,  Image::FormatType::Float, nullptr, nullptr, RG16FToRGBA32F, RGBA32FToRG16F },
    { "R16G16B16_FLOAT",        6,  3,  16, 16, 16, 0,  Image::FormatType::Float, nullptr, nullptr, RGB16FToRGBA32F, RGBA32FToRGB16F },
    { "R16G16B16A16_FLOAT",     8,  4,  16, 16, 16, 16, Image::FormatType::Float, nullptr, nullptr, RGBA16FToRGBA32F, RGBA32FToRGBA16F },
    { "R9G9B9E5_FLOAT",         4,  3,  9,  9,  9,  0,  Image::FormatType::Float | Image::FormatType::Packed, nullptr, nullptr, RGBE9995ToRGBA32F, RGBA32FToRGBE9995 },
    { "R11G11B10_FLOAT",        4,  3,  11, 11, 10, 0,  Image::FormatType::Float | Image::FormatType::Packed, nullptr, nullptr, RGB11F11F10FToRGBA32F, RGBA32FToRGB11F11F10F },
    { "R8G8B8X8",               4,  4,  8,  8,  8,  0,  Image::FormatType::None, RGBX8888ToRGBA8888, RGBA8888ToRGBX8888, RGBX8888ToRGBA32F, RGBA32FToRGBX8888 },
    { "B8G8R8X8",               4,  4,  8,  8,  8,  0,  Image::FormatType::None, BGRX8888ToRGBA8888, RGBA8888ToBGRX8888, BGRX8888ToRGBA32F, RGBA32FToBGRX8888 },
    { "R8G8B8A8",               4,  4,  8,  8,  8,  8,  Image::FormatType::None, RGBA8888ToRGBA8888, RGBA8888ToRGBA8888, RGBA8888ToRGBA32F, RGBA32FToRGBA8888 },
    { "R8G8B8A8_SNORM",         3,  4,  8,  8,  8,  8,  Image::FormatType::SNorm, nullptr, nullptr, RGBA8888SNormToRGBA32F, RGBA32FToRGBA8888SNorm },
    { "R8G8B8A8_SINT",          3,  4,  8,  8,  8,  8,  Image::FormatType::SInt, nullptr, nullptr, RGBA8888SIntToRGBA32F, RGBA32FToRGBA8888SInt },
    { "R8G8B8A8_UINT",          3,  4,  8,  8,  8,  8,  Image::FormatType::UInt, nullptr, nullptr, RGBA8888UIntToRGBA32F, RGBA32FToRGBA8888UInt },
    { "B8G8R8A8",               4,  4,  8,  8,  8,  8,  Image::FormatType::None, BGRA8888ToRGBA8888, RGBA8888ToBGRA8888, BGRA8888ToRGBA32F, RGBA32FToBGRA8888 },
    { "A8B8G8R8",               4,  4,  8,  8,  8,  8,  Image::FormatType::None, ABGR8888ToRGBA8888, RGBA8888ToABGR8888, ABGR8888ToRGBA32F, RGBA32FToABGR8888 },
    { "A8R8G8B8",               4,  4,  8,  8,  8,  8,  Image::FormatType::None, ARGB8888ToRGBA8888, RGBA8888ToARGB8888, ARGB8888ToRGBA32F, RGBA32FToARGB8888 },
    { "R8G8B8"   ,              3,  3,  8,  8,  8,  0,  Image::FormatType::None, RGB888ToRGBA8888, RGBA8888ToRGB888, RGB888ToRGBA32F, RGBA32FToRGB888 },
    { "R8G8B8_SNORM",           3,  3,  8,  8,  8,  0,  Image::FormatType::SNorm, nullptr, nullptr, RGB888SNormToRGBA32F, RGBA32FToRGB888SNorm },
    { "R8G8B8_SINT",            3,  3,  8,  8,  8,  0,  Image::FormatType::SInt, nullptr, nullptr, RGB888SIntToRGBA32F, RGBA32FToRGB888SInt },
    { "R8G8B8_UINT",            3,  3,  8,  8,  8,  0,  Image::FormatType::UInt, nullptr, nullptr, RGB888UIntToRGBA32F, RGBA32FToRGB888UInt },
    { "B8G8R8",                 3,  3,  8,  8,  8,  0,  Image::FormatType::None, BGR888ToRGBA8888, RGBA8888ToBGR888, BGR888ToRGBA32F, RGBA32FToBGR888 },
    { "R8G8",                   2,  2,  8,  8,  0,  0,  Image::FormatType::None, RG88ToRGBA8888, RGBA8888ToRG88, RG88ToRGBA32F, RGBA32FToRG88 },
    { "R8G8_SNORM",             2,  2,  8,  8,  0,  0,  Image::FormatType::SNorm, nullptr, nullptr, RG88SNormToRGBA32F, RGBA32FToRG88SNorm },
    { "R8G8_SINT",              2,  2,  8,  8,  0,  0,  Image::FormatType::SInt, nullptr, nullptr, RG88SIntToRGBA32F, RGBA32FToRG88SInt },
    { "R8G8_UINT",              2,  2,  8,  8,  0,  0,  Image::FormatType::UInt, nullptr, nullptr, RG88UIntToRGBA32F, RGBA32FToRG88UInt },
    { "R8",                     1,  1,  8,  0,  0,  0,  Image::FormatType::None, R8ToRGBA8888, RGBA8888ToR8, R8ToRGBA32F, RGBA32FToR8 },
    { "R8_SNORM",               1,  1,  8,  0,  0,  0,  Image::FormatType::SNorm, nullptr, nullptr, R8SNormToRGBA32F, RGBA32FToR8SNorm },
    { "R8_SINT",                1,  1,  8,  0,  0,  0,  Image::FormatType::SInt, nullptr, nullptr, R8SIntToRGBA32F, RGBA32FToR8SInt },
    { "R8_UINT",                1,  1,  8,  0,  0,  0,  Image::FormatType::UInt, nullptr, nullptr, R8UIntToRGBA32F, RGBA32FToR8UInt },

    { "R10G10B10A2",            4,  4, 10, 10, 10,  2,  Image::FormatType::Packed, nullptr, nullptr, RGBA1010102ToRGBA32F, RGBA32FToRGBA1010102 },
    { "R10G10B10A2_UINT",       4,  4, 10, 10, 10,  2,  Image::FormatType::Packed | Image::FormatType::UInt, nullptr, nullptr, RGBA1010102UIntToRGBA32F, RGBA32FToRGBA1010102UInt },
    { "R4G4B4X4",               2,  4,  4,  4,  4,  0,  Image::FormatType::Packed, RGBX4444ToRGBA8888, RGBA8888ToRGBX4444, RGBX4444ToRGBA32F, RGBA32FToRGBX4444 },
    { "B4G4R4X4",               2,  4,  4,  4,  4,  0,  Image::FormatType::Packed, BGRX4444ToRGBA8888, RGBA8888ToBGRX4444, BGRX4444ToRGBA32F, RGBA32FToBGRX4444 },
    { "R4G4B4A4",               2,  4,  4,  4,  4,  4,  Image::FormatType::Packed, RGBA4444ToRGBA8888, RGBA8888ToRGBA4444, RGBA4444ToRGBA32F, RGBA32FToRGBA4444 },
    { "B4G4R4A4",               2,  4,  4,  4,  4,  4,  Image::FormatType::Packed, BGRA4444ToRGBA8888, RGBA8888ToBGRA4444, BGRA4444ToRGBA32F, RGBA32FToBGRA4444 },
    { "A4B4G4R4",               2,  4,  4,  4,  4,  4,  Image::FormatType::Packed, ABGR4444ToRGBA8888, RGBA8888ToABGR4444, ABGR4444ToRGBA32F, RGBA32FToABGR4444 },
    { "A4R4G4B4",               2,  4,  4,  4,  4,  4,  Image::FormatType::Packed, ARGB4444ToRGBA8888, RGBA8888ToARGB4444, ARGB4444ToRGBA32F, RGBA32FToARGB4444 },
    { "R5G5B5X1",               2,  4,  5,  5,  5,  0,  Image::FormatType::Packed, RGBX5551ToRGBA8888, RGBA8888ToRGBX5551, RGBX5551ToRGBA32F, RGBA32FToRGBX5551 },
    { "B5G5R5X1",               2,  4,  5,  5,  5,  0,  Image::FormatType::Packed, BGRX5551ToRGBA8888, RGBA8888ToBGRX5551, BGRX5551ToRGBA32F, RGBA32FToBGRX5551 },
    { "R5G5B5A1",               2,  4,  5,  5,  5,  1,  Image::FormatType::Packed, RGBA5551ToRGBA8888, RGBA8888ToRGBA5551, RGBA5551ToRGBA32F, RGBA32FToRGBA5551 },
    { "B5G5R5A1",               2,  4,  5,  5,  5,  1,  Image::FormatType::Packed, BGRA5551ToRGBA8888, RGBA8888ToBGRA5551, BGRA5551ToRGBA32F, RGBA32FToBGRA5551 },
    { "A1B5G5R5",               2,  4,  5,  5,  5,  1,  Image::FormatType::Packed, ABGR1555ToRGBA8888, RGBA8888ToABGR1555, ABGR1555ToRGBA32F, RGBA32FToABGR1555 },
    { "A1R5G5B5",               2,  4,  5,  5,  5,  1,  Image::FormatType::Packed, ARGB1555ToRGBA8888, RGBA8888ToARGB1555, ARGB1555ToRGBA32F, RGBA32FToARGB1555 },
    { "R5G6B5",                 2,  3,  5,  6,  5,  0,  Image::FormatType::Packed, RGB565ToRGBA8888, RGBA8888ToRGB565, RGB565ToRGBA32F, RGBA32FToRGB565 },
    { "B5G6R5",                 2,  3,  5,  6,  5,  0,  Image::FormatType::Packed, BGR565ToRGBA8888, RGBA8888ToBGR565, BGR565ToRGBA32F, RGBA32FToBGR565 },

    // depth/stencil ------------------------------------------------------------------------------
    { "D16",                    2,  1,  0,  0,  0,  0,  Image::FormatType::Depth, nullptr, nullptr, L16ToRGBA32F, RGBA32FToL16 },
    { "D24",                    3,  1,  0,  0,  0,  0,  Image::FormatType::Depth, nullptr, nullptr, nullptr, nullptr },
    { "D32_FLOAT",              4,  1,  0,  0,  0,  0,  Image::FormatType::Depth | Image::FormatType::Float, nullptr, nullptr, L32FToRGBA32F, RGBA32FToL32F },
    { "D24S8",                  4,  2,  0,  0,  0,  0,  Image::FormatType::DepthStencil, nullptr, nullptr, nullptr, nullptr },
    { "D32_FLOAT_S8X24",        8,  3,  0,  0,  0,  0,  Image::FormatType::DepthStencil | Image::FormatType::Float, nullptr, nullptr, nullptr, nullptr },

    // Luminance/Alpha
    { "L32_FLOAT",              4,  1,  32, 0,  0,  0,  Image::FormatType::Float, nullptr, nullptr, L32FToRGBA32F, RGBA32FToL32F },
    { "A32_FLOAT",              4,  1,  0,  0,  0,  32, Image::FormatType::Float, nullptr, nullptr, A32FToRGBA32F, RGBA32FToA32F },
    { "L32A32_FLOAT",           8,  2,  32, 0,  0,  32, Image::FormatType::Float, nullptr, nullptr, LA32FToRGBA32F, RGBA32FToLA32F },
    { "L16_FLOAT",              2,  1,  16, 0,  0,  0,  Image::FormatType::Float, nullptr, nullptr, L16FToRGBA32F, RGBA32FToL16F },
    { "A16_FLOAT",              2,  1,  0,  0,  0,  16, Image::FormatType::Float, nullptr, nullptr, A16FToRGBA32F, RGBA32FToA16F },
    { "L16A16_FLOAT",           4,  2,  16, 0,  0,  16, Image::FormatType::Float, nullptr, nullptr, LA16FToRGBA32F, RGBA32FToLA16F },
    { "L8",                     1,  1,  8,  0,  0,  0,  Image::FormatType::None, L8ToRGBA8888, RGBA8888ToL8, L8ToRGBA32F, RGBA32FToL8 },
    { "A8" ,                    1,  1,  0,  0,  0,  8,  Image::FormatType::None, A8ToRGBA8888, RGBA8888ToA8, A8ToRGBA32F, RGBA32FToA8 },
    { "L8A8"  ,                 2,  2,  8,  0,  0,  8,  Image::FormatType::None, LA88ToRGBA8888, RGBA8888ToLA88, LA88ToRGBA32F, RGBA32FToLA88 },

    // DXT (BTC) ----------------------------------------------------------------------------------
    { "DXT1",                   8,  4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "DXT3",                   16, 4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "DXT5",                   16, 4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "DXT5_XGBR",              16, 4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "DXN1",                   8,  2,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "DXN2",                   16, 2,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC6H_UF16",              16, 3,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC6H_SF16",              16, 3,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC7",                    16, 4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },

    // PVRTC --------------------------------------------------------------------------------------
    { "PVRTC12",                8,  3,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "PVRTC14",                8,  3,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "PVRTC12A",               8,  4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "PVRTC14A",               8,  4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "PVRTC22A",               8,  4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "PVRTC24A",               8,  4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },

    // ETC ----------------------------------------------------------------------------------------
    { "ETC1",                   8,  3,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "ETC2",                   8,  3,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "ETC2A1",                 8,  4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "ETC2A",                  16, 4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "EACR11",                 8,  1,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "EACRG11",                16, 2,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "EACR11_SNORM",           8,  1,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "EACRG11_SNORM",          16, 2,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },

    // ATC ----------------------------------------------------------------------------------------
    { "ATC",                    8,  3,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "ATCE",                   16, 4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "ATCI",                   16, 4,  0,  0,  0,  0,  Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
};

const ImageFormatInfo *GetImageFormatInfo(Image::Format imageFormat) {
    assert(imageFormat < Image::Format::Count);
    return &imageFormatInfo[to_int(imageFormat)];
}

bool CompressedFormatMinDimensions(Image::Format imageFormat, int &minWidth, int &minHeight) {
    switch (imageFormat) {
    case Image::Format::DXT1:
    case Image::Format::DXT3:
    case Image::Format::DXT5:
    case Image::Format::DXT5XGBR:
    case Image::Format::DXN1:
    case Image::Format::DXN2:
        minWidth = 4;
        minHeight = 4;
        return true;
    case Image::Format::PVRTC12:
    case Image::Format::PVRTC12A:
        minWidth = 16;
        minHeight = 8;
        return true;
    case Image::Format::PVRTC14:
    case Image::Format::PVRTC14A:
        minWidth = 8;
        minHeight = 8;
        return true;
    case Image::Format::PVRTC22A:
        minWidth = 8;
        minHeight = 4;
        return true;
    case Image::Format::PVRTC24A:
        minWidth = 4;
        minHeight = 4;
        return true;
    case Image::Format::ETC1:
    case Image::Format::ETC2:
    case Image::Format::ETC2A1:
    case Image::Format::ETC2A:
    case Image::Format::EACR11:
    case Image::Format::EACR11_SNORM:
    case Image::Format::EACRG11:
    case Image::Format::EACRG11_SNORM:
        minWidth = 4;
        minHeight = 4;
        return true;
    case Image::Format::ATC:
    case Image::Format::ATCE: // Explicit alpha
    case Image::Format::ATCI: // Interpolated alpha
        minWidth = 4;
        minHeight = 4;
        return false;
    default:
        return false;
    }
    
    return false;
}

bool CompressedFormatBlockDimensions(Image::Format imageFormat, int &blockWidth, int &blockHeight) {
    switch (imageFormat) {
    case Image::Format::DXT1:
    case Image::Format::DXT3:
    case Image::Format::DXT5:
    case Image::Format::DXT5XGBR:
    case Image::Format::DXN1:
    case Image::Format::DXN2:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::Format::PVRTC12:
    case Image::Format::PVRTC12A:
        blockWidth = 8;
        blockHeight = 4;
        return true;
    case Image::Format::PVRTC14:
    case Image::Format::PVRTC14A:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::Format::PVRTC22A:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::Format::PVRTC24A:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::Format::ETC1:
    case Image::Format::ETC2:
    case Image::Format::ETC2A1:
    case Image::Format::ETC2A:
    case Image::Format::EACR11:
    case Image::Format::EACR11_SNORM:
    case Image::Format::EACRG11:
    case Image::Format::EACRG11_SNORM:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::Format::ATC:
    case Image::Format::ATCE: // Explicit alpha
    case Image::Format::ATCI: // Interpolated alpha
        blockWidth = 4;
        blockHeight = 4;
        return false;
    default:
        return false;
    }
    
    return false;
}

BE_NAMESPACE_END
