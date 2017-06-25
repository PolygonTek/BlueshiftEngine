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
// XXXToRGBA8888 (unpacking function from custom format to rgba8888
//
//--------------------------------------------------------------------------------------------------

static void L8ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels;
    for (; src < srcEnd; src += 1, dst += 4) {
        dst[0] = src[0];
        dst[1] = src[0];
        dst[2] = src[0];
        dst[3] = 255;
    }
}
static void A8ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels;
    for (; src < srcEnd; src += 1, dst += 4) {
        dst[0] = 255;
        dst[1] = 255;
        dst[2] = 255;
        dst[3] = src[0];
    }
}
static void LA88ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 2;
    for (; src < srcEnd; src += 2, dst += 4) {
        dst[0] = src[0];
        dst[1] = src[0];
        dst[2] = src[0];
        dst[3] = src[1];
    }
}
static void LA1616ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    byte l, a;
    for (; src < srcEnd; src += 4, dst += 4) {
        l = (*(uint16_t *)(&src[0])) >> 1;
        a = (*(uint16_t *)(&src[1])) >> 1;
        dst[0] = l;
        dst[1] = l;
        dst[2] = l;
        dst[3] = a;
    }
}
static void R8ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels;
    for (; src < srcEnd; src += 1, dst += 4) {
        dst[0] = src[0];
        dst[1] = 0;
        dst[2] = 0;
        dst[3] = 255;
    }
}
static void RG88ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 2;
    for (; src < srcEnd; src += 2, dst += 4) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = 0;
        dst[3] = 255;
    }
}
static void RGB888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 3;
    for (; src < srcEnd; src += 3, dst += 4) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = 255;
    }
}
static void BGR888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 3;
    for (; src < srcEnd; src += 3, dst += 4) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        dst[3] = 255;
    }
}
static void RGBX8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = 255;
    }
}
static void BGRX8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        dst[3] = 255;
    }
}
static void RGBA8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    //simdProcessor->Memcpy(dst, src, 4 * numPixels);
    memcpy(dst, src, 4 * numPixels);
}
static void BGRA8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        dst[3] = src[3];
    }
}
static void ABGR8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = src[3];
        dst[1] = src[2];
        dst[2] = src[1];
        dst[3] = src[0];
    }
}
static void ARGB8888ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = src[1];
        dst[1] = src[2];
        dst[2] = src[3];
        dst[3] = src[0];
    }
}
static void RGBX4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dst[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dst[2] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dst[3] = 255;
    }
}
static void BGRX4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dst[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dst[2] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dst[3] = 255;
    }
}
static void RGBA4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dst[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dst[2] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dst[3] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
    }
}
static void BGRA4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dst[1] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dst[2] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
        dst[3] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
    }
}
static void ABGR4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
        dst[1] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dst[2] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dst[3] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
    }
}
static void ARGB4444ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr << 0) & 0xF0) | ((*srcPtr >> 4) & 0x0F);
        dst[1] = ((*srcPtr >> 4) & 0xF0) | ((*srcPtr >> 8) & 0x0F);
        dst[2] = ((*srcPtr >> 8) & 0xF0) | ((*srcPtr >> 12) & 0x0F);
        dst[3] = ((*srcPtr << 4) & 0xF0) | ((*srcPtr >> 0) & 0x0F);
    }
}
static void RGBX5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dst[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dst[2] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dst[3] = 255;
    }
}
static void BGRX5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dst[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dst[2] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dst[3] = 255;
    }
}
static void RGBA5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dst[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dst[2] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dst[3] = ((int16_t)*srcPtr >> 15) & 0xFF;
    }
}
static void BGRA5551ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr >> 7) & 0xF8) | ((*srcPtr >> 12) & 0x7);
        dst[1] = ((*srcPtr >> 2) & 0xF8) | ((*srcPtr >> 7) & 0x7);
        dst[2] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dst[3] = ((int16_t)*srcPtr >> 15) & 0xFF;
    }
}
static void ABGR1555ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 13) & 0x7);
        dst[1] = ((*srcPtr >> 3) & 0xF8) | ((*srcPtr >> 8) & 0x7);
        dst[2] = ((*srcPtr << 2) & 0xF8) | ((*srcPtr >> 3) & 0x7);
        dst[3] = (*srcPtr & 1) ? 0xFF : 0;
    }
}
static void ARGB1555ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr << 2) & 0xF8) | ((*srcPtr >> 3) & 0x7);
        dst[1] = ((*srcPtr >> 3) & 0xF8) | ((*srcPtr >> 8) & 0x7);
        dst[2] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 13) & 0x7);
        dst[3] = (*srcPtr & 1) ? 0xFF : 0;
    }
}
static void RGB565ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dst[1] = ((*srcPtr >> 3) & 0xFC) | ((*srcPtr >> 9) & 0x3);
        dst[2] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 14) & 0x7);
        dst[3] = 255;
    }
}
static void BGR565ToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const uint16_t *srcPtr = (const uint16_t *)src;
    const uint16_t *srcEnd = srcPtr + numPixels;
    for (; srcPtr < srcEnd; srcPtr++, dst += 4) {
        dst[0] = ((*srcPtr >> 8) & 0xF8) | ((*srcPtr >> 14) & 0x7);
        dst[1] = ((*srcPtr >> 3) & 0xFC) | ((*srcPtr >> 9) & 0x3);
        dst[2] = ((*srcPtr << 3) & 0xF8) | ((*srcPtr >> 2) & 0x7);
        dst[3] = 255;
    }
}
static void L16FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcEnd = (const float16_t *)src + numPixels;
    const float16_t *hsrc = (const float16_t *)src;
    for (; hsrc < srcEnd; hsrc += 1, dst += 4) {
        dst[0] = dst[1] = dst[2] = (byte)(F16toF32(hsrc[0]) * 255.0f);
        dst[3] = 255;
    }
}
static void L32FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcEnd = (const float *)src + numPixels;
    const float *fsrc = (const float *)src;
    for (; fsrc < srcEnd; fsrc += 1, dst += 4) {
        dst[0] = dst[1] = dst[2] = (byte)(fsrc[0] * 255.0f);
        dst[3] = 255;
    }
}
static void A16FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcEnd = (const float16_t *)src + numPixels;
    const float16_t *hsrc = (const float16_t *)src;
    for (; hsrc < srcEnd; hsrc += 1, dst += 4) {
        dst[0] = 255;
        dst[1] = 255;
        dst[2] = 255;
        dst[3] = (byte)(F16toF32(hsrc[0]) * 255.0f);
    }
}
static void A32FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcEnd = (const float *)src + numPixels;
    const float *fsrc = (const float *)src;
    for (; fsrc < srcEnd; fsrc += 1, dst += 4) {
        dst[0] = 255;
        dst[1] = 255;
        dst[2] = 255;
        dst[3] = (byte)(fsrc[0] * 255.0f);
    }
}
static void LA16F16FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcEnd = (const float16_t *)src + numPixels * 2;
    const float16_t *hsrc = (const float16_t *)src;
    for (; hsrc < srcEnd; hsrc += 2, dst += 4) {
        dst[0] = dst[1] = dst[2] = (byte)(F16toF32(hsrc[0]) * 255.0f);
        dst[3] = (byte)(F16toF32(hsrc[1]) * 255.0f);
    }
}
static void LA32F32FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcEnd = (const float *)src + numPixels * 2;
    const float *fsrc = (const float *)src;
    for (; fsrc < srcEnd; fsrc += 2, dst += 4) {
        dst[0] = dst[1] = dst[2] = (byte)(fsrc[0] * 255.0f);
        dst[3] = (byte)(fsrc[1] * 255.0f);
    }
}
static void R16FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcEnd = (const float16_t *)src + numPixels;
    const float16_t *hsrc = (const float16_t *)src;
    for (; hsrc < srcEnd; hsrc += 1, dst += 4) {
        dst[0] = (byte)(F16toF32(hsrc[0]) * 255.0f);
        dst[1] = 0;
        dst[2] = 0;
        dst[3] = 255;
    }
}
static void R32FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcEnd = (const float *)src + numPixels;
    const float *fsrc = (const float *)src;
    for (; fsrc < srcEnd; fsrc += 1, dst += 4) {
        dst[0] = (byte)(fsrc[0] * 255.0f);
        dst[1] = 0;
        dst[2] = 0;
        dst[3] = 255;
    }
}
static void RG16F16FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcEnd = (const float16_t *)src + numPixels * 2;
    const float16_t *hsrc = (const float16_t *)src;
    for (; hsrc < srcEnd; hsrc += 2, dst += 4) {
        dst[0] = (byte)(F16toF32(hsrc[0]) * 255.0f);
        dst[1] = (byte)(F16toF32(hsrc[1]) * 255.0f);
        dst[2] = 0;
        dst[3] = 255;
    }
}
static void RG32F32FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcEnd = (const float *)src + numPixels * 2;
    const float *fsrc = (const float *)src;
    for (; fsrc < srcEnd; fsrc += 2, dst += 4) {
        dst[0] = (byte)(fsrc[0] * 255.0f);
        dst[1] = (byte)(fsrc[1] * 255.0f);
        dst[2] = 0;
        dst[3] = 255;
    }
}
static void RGB16F16F16FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcEnd = (const float16_t *)src + numPixels * 3;
    const float16_t *hsrc = (const float16_t *)src;
    for (; hsrc < srcEnd; hsrc += 3, dst += 4) {
        dst[0] = (byte)(F16toF32(hsrc[0]) * 255.0f);
        dst[1] = (byte)(F16toF32(hsrc[1]) * 255.0f);
        dst[2] = (byte)(F16toF32(hsrc[2]) * 255.0f);
        dst[3] = 255;
    }
}
static void RGB32F32F32FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcEnd = (const float *)src + numPixels * 3;
    const float *fsrc = (const float *)src;
    for (; fsrc < srcEnd; fsrc += 3, dst += 4) {
        dst[0] = (byte)(fsrc[0] * 255.0f);
        dst[1] = (byte)(fsrc[1] * 255.0f);
        dst[2] = (byte)(fsrc[2] * 255.0f);
        dst[3] = 255;
    }
}
static void RGBA16F16F16F16FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float16_t *srcEnd = (const float16_t *)src + numPixels * 3;
    const float16_t *hsrc = (const float16_t *)src;
    for (; hsrc < srcEnd; hsrc += 3, dst += 4) {
        dst[0] = (byte)(F16toF32(hsrc[0]) * 255.0f);
        dst[1] = (byte)(F16toF32(hsrc[1]) * 255.0f);
        dst[2] = (byte)(F16toF32(hsrc[2]) * 255.0f);
        dst[3] = (byte)(F16toF32(hsrc[3]) * 255.0f);
    }
}
static void RGBA32F32F32F32FToRGBA8888(const byte *src, byte *dst, int numPixels) {
    const float *srcEnd = (const float *)src + numPixels * 3;
    const float *fsrc = (const float *)src;
    for (; fsrc < srcEnd; fsrc += 3, dst += 4) {
        dst[0] = (byte)(fsrc[0] * 255.0f);
        dst[1] = (byte)(fsrc[1] * 255.0f);
        dst[2] = (byte)(fsrc[2] * 255.0f);
        dst[3] = (byte)(fsrc[3] * 255.0f);
    }
}

//--------------------------------------------------------------------------------------------------
//
// RGBA8888ToXXX (packing function from rgba8888 to custom format)
//
//--------------------------------------------------------------------------------------------------

static void RGBA8888ToL8(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 1) {
        dst[0] = (byte)(0.299f * src[0] + 0.587f * src[1] + 0.114f * src[2]);	
    }
}
static void RGBA8888ToA8(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 1) {
        dst[0] = src[3];
    }
}
static void RGBA8888ToLA88(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 2) {
        dst[0] = (byte)(0.299f * src[0] + 0.587f * src[1] + 0.114f * src[2]);
        dst[1] = src[3];
    }
}
static void RGBA8888ToLA1616(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = (((uint16_t)(0.299f * src[0] + 0.587f * src[1] + 0.114f * src[2])) << 1);
        dst[1] = (((uint16_t)src[3]) << 1);
    }
}
static void RGBA8888ToR8(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 1) {
        dst[0] = (byte)(src[0]);
    }
}
static void RGBA8888ToRG88(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 2) {
        dst[0] = src[0];
        dst[1] = src[1];
    }
}
static void RGBA8888ToRGB888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 3) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
}
static void RGBA8888ToBGR888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 3) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
    }
}
static void RGBA8888ToRGBX8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = 0;
    }
}
static void RGBA8888ToBGRX8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        dst[3] = 0;
    }
}
static void RGBA8888ToBGRA8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = src[2];
        dst[1] = src[1];
        dst[2] = src[0];
        dst[3] = src[3];
    }
}
static void RGBA8888ToABGR8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = src[3];
        dst[1] = src[2];
        dst[2] = src[1];
        dst[3] = src[0];
    }
}
static void RGBA8888ToARGB8888(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dst += 4) {
        dst[0] = src[1];
        dst[1] = src[2];
        dst[2] = src[3];
        dst[3] = src[0];
    }
}
static void RGBA8888ToRGBX4444(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[0] >> 4) | ((src[1] >> 4) << 4) | ((src[2] >> 4) << 8);
    }
}
static void RGBA8888ToBGRX4444(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[2] >> 4) | ((src[1] >> 4) << 4) | ((src[0] >> 4) << 8);
    }
}
static void RGBA8888ToRGBA4444(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[0] >> 4) | ((src[1] >> 4) << 4) | ((src[2] >> 4) << 8) | ((src[3] >> 4) << 12);
    }
}
static void RGBA8888ToBGRA4444(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[2] >> 4) | ((src[1] >> 4) << 4) | ((src[0] >> 4) << 8) | ((src[3] >> 4) << 12);
    }
}
static void RGBA8888ToABGR4444(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[3] >> 4) | ((src[2] >> 4) << 4) | ((src[1] >> 4) << 8) | ((src[0] >> 4) << 12);
    }
}
static void RGBA8888ToARGB4444(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[1] >> 4) | ((src[2] >> 4) << 4) | ((src[3] >> 4) << 8) | ((src[0] >> 4) << 12);
    }
}
static void RGBA8888ToRGBX5551(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[0] >> 3) | ((src[1] >> 3) << 5) | ((src[2] >> 3) << 10);
    }
}
static void RGBA8888ToBGRX5551(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[2] >> 3) | ((src[1] >> 3) << 5) | ((src[0] >> 3) << 10);
    }
}
static void RGBA8888ToRGBA5551(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[0] >> 3) | ((src[1] >> 3) << 5) | ((src[2] >> 3) << 10) | ((src[3] >> 7) << 15);
    }
}
static void RGBA8888ToBGRA5551(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[2] >> 3) | ((src[1] >> 3) << 5) | ((src[0] >> 3) << 10) | ((src[3] >> 7) << 15);
    }
}
static void RGBA8888ToABGR1555(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[3] >> 3) | ((src[2] >> 3) << 5) | ((src[1] >> 3) << 10) | ((src[0] >> 7) << 15);
    }
}
static void RGBA8888ToARGB1555(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[1] >> 3) | ((src[2] >> 3) << 5) | ((src[3] >> 3) << 10) | ((src[0] >> 7) << 15);
    }
}
static void RGBA8888ToRGB565(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[0] >> 3) | ((src[1] >> 2) << 5) | ((src[2] >> 3) << 11);
    }
}
static void RGBA8888ToBGR565(const byte *src, byte *dst, int numPixels) {
    uint16_t *dstPtr = (uint16_t *)dst;
    const byte *srcEnd = src + numPixels * 4;
    for (; src < srcEnd; src += 4, dstPtr++) {
        *dstPtr = (src[2] >> 3) | ((src[1] >> 2) << 5) | ((src[0] >> 3) << 11);
    }
}
static void RGBA8888ToL16F(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    float16_t *hdst = (float16_t *)dst;
    for (; src < srcEnd; src += 4, hdst += 1) {
        hdst[0] = F32toF16(0.299f * (src[0] / 255.0f) + 0.587f * (src[1] / 255.0f) + 0.114f * (src[2] / 255.0f));
    }
}
static void RGBA8888ToL32F(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    float *fdst = (float *)dst;
    for (; src < srcEnd; src += 4, fdst += 1) {
        fdst[0] = 0.299f * (src[0] / 255.0f) + 0.587f * (src[1] / 255.0f) + 0.114f * (src[2] / 255.0f);
    }
}
static void RGBA8888ToA16F(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    float16_t *hdst = (float16_t *)dst;
    for (; src < srcEnd; src += 4, hdst += 1) {
        hdst[0] = F32toF16(src[3] / 255.0f);
    }
}
static void RGBA8888ToA32F(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    float *fdst = (float *)dst;
    for (; src < srcEnd; src += 4, fdst += 1) {
        fdst[0] = src[3] / 255.0f;
    }
}
static void RGBA8888ToLA16F16F(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    float16_t *hdst = (float16_t *)dst;
    for (; src < srcEnd; src += 4, hdst += 2) {
        hdst[0] = F32toF16(0.299f * (src[0] / 255.0f) + 0.587f * (src[1] / 255.0f) + 0.114f * (src[2] / 255.0f));
        hdst[1] = F32toF16(src[3] / 255.0f);
    }
}
static void RGBA8888ToLA32F32F(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    float *fdst = (float *)dst;
    for (; src < srcEnd; src += 4, fdst += 2) {
        fdst[0] = 0.299f * (src[0] / 255.0f) + 0.587f * (src[1] / 255.0f) + 0.114f * (src[2] / 255.0f);
        fdst[1] = src[3] / 255.0f;
    }
}
static void RGBA8888ToR16F(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    float16_t *hdst = (float16_t *)dst;
    for (; src < srcEnd; src += 4, hdst += 1) {
        hdst[0] = F32toF16(src[0] / 255.0f);
    }
}
static void RGBA8888ToR32F(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    float *fdst = (float *)dst;
    for (; src < srcEnd; src += 4, fdst += 1) {
        fdst[0] = src[0] / 255.0f;
    }
}
static void RGBA8888ToRG16F16F(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    float16_t *hdst = (float16_t *)dst;
    for (; src < srcEnd; src += 4, hdst += 2) {
        hdst[0] = F32toF16(src[0] / 255.0f);
        hdst[1] = F32toF16(src[1] / 255.0f);
    }
}
static void RGBA8888ToRG32F32F(const byte *src, byte *dst, int numPixels) {
    const byte *srcEnd = src + numPixels * 4;
    float *fdst = (float *)dst;
    for (; src < srcEnd; src += 4, fdst += 2) {
        fdst[0] = src[0] / 255.0f;
        fdst[1] = src[1] / 255.0f;
    }
}
static void RGBA8888ToRGB16F16F16F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcEnd = src + numPixels * 4;
    float16_t *hdst = (float16_t *)dst;
    float16_t hone = F32toF16(1.0f);
    for (; src < srcEnd; src += 4, hdst += 3) {
        hdst[0] = F32toF16(src[0] * invNorm);
        hdst[1] = F32toF16(src[1] * invNorm);
        hdst[2] = F32toF16(src[2] * invNorm);
        hdst[3] = hone;
    }
}
static void RGBA8888ToRGB32F32F32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcEnd = src + numPixels * 4;
    float *fdst = (float *)dst;
    for (; src < srcEnd; src += 4, fdst += 3) {
        fdst[0] = src[0] * invNorm;
        fdst[1] = src[1] * invNorm;
        fdst[2] = src[2] * invNorm;
        fdst[3] = 1.0f;
    }
}
static void RGBA8888ToRGBA16F16F16F16F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcEnd = src + numPixels * 4;
    float16_t *hdst = (float16_t *)dst;
    for (; src < srcEnd; src += 4, hdst += 3) {
        hdst[0] = F32toF16(src[0] * invNorm);
        hdst[1] = F32toF16(src[1] * invNorm);
        hdst[2] = F32toF16(src[2] * invNorm);
        hdst[3] = F32toF16(src[3] * invNorm);
    }
}
static void RGBA8888ToRGBA32F32F32F32F(const byte *src, byte *dst, int numPixels) {
    const float invNorm = 1.0f / 255.0f;
    const byte *srcEnd = src + numPixels * 4;
    float *fdst = (float *)dst;
    for (; src < srcEnd; src += 4, fdst += 3) {
        fdst[0] = src[0] * invNorm;
        fdst[1] = src[1] * invNorm;
        fdst[2] = src[2] * invNorm;
        fdst[3] = src[3] * invNorm;
    }
}

// 컬러 채널 표기(RGBALX)는 바이트 주소체계와 선형이다 (D3D식 표기와 반대)
static const ImageFormatInfo imageFormatInfo[] = {
    // name                     size, numComponents, redBits, greenBits, blueBits, alphaBits, compressed, depthFormat, depthStencilFormat, floatFormat
    { "NULL",                   0,  0,  0,  0,  0,  0,  0, nullptr, nullptr },
    { "L_8",                    1,  1,  0,  0,  0,  0,  0, L8ToRGBA8888,       RGBA8888ToL8 }, 
    { "A_8",                    1,  1,  0,  0,  0,  8,  0, A8ToRGBA8888,       RGBA8888ToA8 }, 
    { "LA_8_8",                 2,  2,  0,  0,  0,  8,  0, LA88ToRGBA8888,     RGBA8888ToLA88 }, 
    { "LA_16_16",               4,  2,  0,  0,  0,  16, 0, LA1616ToRGBA8888,   RGBA8888ToLA1616 },
    { "R_8",                    1,  1,  8,  0,  0,  0,  0, R8ToRGBA8888,       RGBA8888ToR8 },
    { "R_SNORM_8",              1,  1,  8,  0,  0,  0,  0, nullptr/*R8ToRGBA8888*/, nullptr/*RGBA8888ToR8*/ },
    { "RG_8_8",                 2,  2,  8,  8,  0,  0,  0, RG88ToRGBA8888,     RGBA8888ToRG88 },
    { "RG_SNORM_8_8",           2,  2,  8,  8,  0,  0,  0, nullptr/*RG88ToRGBA8888*/, nullptr/*RGBA8888ToRG88*/ },
    { "RGB_8_8_8",              3,  3,  8,  8,  8,  0,  0, RGB888ToRGBA8888,   RGBA8888ToRGB888 }, 
    { "RGB_SNORM_8_8_8",        3,  3,  8,  8,  8,  0,  0, nullptr/*RGB888ToRGBA8888*/, nullptr/*RGBA8888ToRGB888*/ },
    { "BGR_8_8_8",              3,  3,  8,  8,  8,  0,  0, BGR888ToRGBA8888,   RGBA8888ToBGR888 },
    { "RGBX_8_8_8_8",           4,  4,  8,  8,  8,  0,  0, RGBX8888ToRGBA8888, RGBA8888ToRGBX8888 }, 
    { "BGRX_8_8_8_8",           4,  4,  8,  8,  8,  0,  0, BGRX8888ToRGBA8888, RGBA8888ToBGRX8888 },
    { "RGBA_8_8_8_8",           4,  4,  8,  8,  8,  8,  0, RGBA8888ToRGBA8888, RGBA8888ToRGBA8888 },
    { "RGBA_SNORM_8_8_8_8",     3,  4,  8,  8,  8,  0,  0, nullptr/*RGB888ToRGBA8888*/, nullptr /*RGBA8888ToRGB888*/ },
    { "BGRA_8_8_8_8",           4,  4 , 8,  8,  8,  8,  0, BGRA8888ToRGBA8888, RGBA8888ToBGRA8888 },
    { "ABGR_8_8_8_8",           4,  4,  8,  8,  8,  8,  0, ABGR8888ToRGBA8888, RGBA8888ToABGR8888 }, 
    { "ARGB_8_8_8_8",           4,  4,  8,  8,  8,  8,  0, ARGB8888ToRGBA8888, RGBA8888ToARGB8888 }, 
    // packed format
    { "RGBX_4_4_4_4",           2,  4,  4,  4,  4,  0,  Image::Packed, RGBX4444ToRGBA8888, RGBA8888ToRGBX4444 }, 
    { "BGRX_4_4_4_4",           2,  4,  4,  4,  4,  0,  Image::Packed, BGRX4444ToRGBA8888, RGBA8888ToBGRX4444 }, 
    { "RGBA_4_4_4_4",           2,  4,  4,  4,  4,  4,  Image::Packed, RGBA4444ToRGBA8888, RGBA8888ToRGBA4444 },
    { "BGRA_4_4_4_4",           2,  4,  4,  4,  4,  4,  Image::Packed, BGRA4444ToRGBA8888, RGBA8888ToBGRA4444 }, 
    { "ABGR_4_4_4_4",           2,  4,  4,  4,  4,  4,  Image::Packed, ABGR4444ToRGBA8888, RGBA8888ToABGR4444 }, 
    { "ARGB_4_4_4_4",           2,  4,  4,  4,  4,  4,  Image::Packed, ARGB4444ToRGBA8888, RGBA8888ToARGB4444 }, 
    { "RGBX_5_5_5_1",           2,  4,  5,  5,  5,  0,  Image::Packed, RGBX5551ToRGBA8888, RGBA8888ToRGBX5551 }, 
    { "BGRX_5_5_5_1",           2,  4,  5,  5,  5,  0,  Image::Packed, BGRX5551ToRGBA8888, RGBA8888ToBGRX5551 }, 
    { "RGBA_5_5_5_1",           2,  4,  5,  5,  5,  1,  Image::Packed, RGBA5551ToRGBA8888, RGBA8888ToRGBA5551 }, 
    { "BGRA_5_5_5_1",           2,  4,  5,  5,  5,  1,  Image::Packed, BGRA5551ToRGBA8888, RGBA8888ToBGRA5551 }, 
    { "ABGR_1_5_5_5",           2,  4,  5,  5,  5,  1,  Image::Packed, ABGR1555ToRGBA8888, RGBA8888ToABGR1555 }, 
    { "ARGB_1_5_5_5",           2,  4,  5,  5,  5,  1,  Image::Packed, ARGB1555ToRGBA8888, RGBA8888ToARGB1555 }, 
    { "RGB_5_6_5",              2,  3,  5,  6,  5,  0,  Image::Packed, RGB565ToRGBA8888,   RGBA8888ToRGB565 }, 
    { "BGR_5_6_5",              2,  3,  5,  6,  5,  0,  Image::Packed, BGR565ToRGBA8888,   RGBA8888ToBGR565 }, 
    { "RGBE_9_9_9_5",           4,  4,  9,  9,  9,  0,  Image::Packed, nullptr, nullptr },
    // float format
    { "L_16F",                  2,  1,  0,  0,  0,  0,  Image::Half,   L16FToRGBA8888,     RGBA8888ToL16F }, 
    { "L_32F",                  4,  1,  0,  0,  0,  0,  Image::Float,  L32FToRGBA8888,     RGBA8888ToL32F }, 
    { "A_16F",                  2,  1,  0,  0,  0,  16, Image::Half,   A16FToRGBA8888,     RGBA8888ToA16F }, 
    { "A_32F",                  4,  1,  0,  0,  0,  32, Image::Float,  A32FToRGBA8888,     RGBA8888ToA32F }, 
    { "LA_16F_16F",             4,  2,  0,  0,  0,  16, Image::Half,   LA16F16FToRGBA8888, RGBA8888ToLA16F16F },
    { "LA_32F_32F",             8,  2,  0,  0,  0,  32, Image::Float,  LA32F32FToRGBA8888, RGBA8888ToLA32F32F }, 
    { "R_16F",                  2,  1,  16, 0,  0,  0,  Image::Half,   R16FToRGBA8888,     RGBA8888ToR16F },
    { "R_32F",                  4,  1,  32, 0,  0,  0,  Image::Float,  R32FToRGBA8888,     RGBA8888ToR32F },
    { "RG_16F_16F",             4,  2,  16, 16, 0,  0,  Image::Half,   RG16F16FToRGBA8888, RGBA8888ToRG16F16F },
    { "RG_32F_32F",             8,  2,  32, 32, 0,  0,  Image::Float,  RG32F32FToRGBA8888, RGBA8888ToRG32F32F },
    { "RGB_11F_11F_10F",        4,  3,  11, 11, 10, 0,  Image::Float | Image::Packed, nullptr/*RGB32F32F32FToRGBA8888*/, nullptr/*RGBA8888ToRGB32F32F32F*/ }, 
    { "RGB_16F_16F_16F",        6,  3,  16, 16, 16, 0,  Image::Half,   RGB16F16F16FToRGBA8888, RGBA8888ToRGB16F16F16F }, 
    { "RGB_32F_32F_32F",        12, 3,  32, 32, 32, 0,  Image::Float,  RGB32F32F32FToRGBA8888, RGBA8888ToRGB32F32F32F }, 
    { "RGBA_16F_16F_16F_16F",   8,  4,  16, 16, 16, 16, Image::Half,   RGBA16F16F16F16FToRGBA8888, RGBA8888ToRGBA16F16F16F16F }, 
    { "RGBA_32F_32F_32F_32F",   16, 4,  32, 32, 32, 32, Image::Float,  RGBA32F32F32F32FToRGBA8888, RGBA8888ToRGBA32F32F32F32F }, 
    // DXT (BTC)
    { "DXT1",                   8,  4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr }, 
    { "DXT3",                   16, 4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr }, 
    { "DXT5",                   16, 4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr }, 
    { "DXT5_RXGB",              16, 4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "DXN1",                   8,  2,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "DXN2",                   16, 2,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    // PVRTC
    { "RGB_PVRTC_2BPPV1",       8,  3,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RGB_PVRTC_4BPPV1",       8,  3,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RGBA_PVRTC_2BPPV1",      8,  4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RGBA_PVRTC_4BPPV1",      8,  4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RGBA_PVRTC_2BPPV2",      8,  4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RGBA_PVRTC_4BPPV2",      8,  4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    // ETC
    { "RGB_8_ETC1",             8,  3,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RGB_8_ETC2",             8,  3,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RGBA_8_8_ETC2",          16, 4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RGBA_8_1_ETC2",          16, 4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "R_11_EAC",               8,  1,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RG_11_11_EAC",           16, 2,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "SignedR_11_EAC",         8,  1,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "SignedRG_11_11_EAC",     16, 2,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    // ATC
    { "RGB_ATC",                8,  3,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RGBA_EA_ATC",            16, 4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    { "RGBA_IA_ATC",            16, 4,  0,  0,  0,  0,  Image::Compressed, nullptr, nullptr },
    // depth
    { "Depth_16",               2,  1,  0,  0,  0,  0,  Image::Depth, nullptr, nullptr },
    { "Depth_24",               3,  1,  0,  0,  0,  0,  Image::Depth, nullptr, nullptr }, 
    { "Depth_32F",              4,  1,  0,  0,  0,  0,  Image::Depth, nullptr, nullptr }, 
    { "DepthStencil_24_8",      4,  2,  0,  0,  0,  0,  Image::DepthStencil, nullptr, nullptr },
    { "DepthStencil_32F_8",     5,  2,  0,  0,  0,  0,  Image::DepthStencil, nullptr, nullptr },
};

const ImageFormatInfo *GetImageFormatInfo(Image::Format imageFormat) {
    assert(imageFormat < Image::NumImageFormats);
    return &imageFormatInfo[imageFormat];
}

bool CompressedFormatMinDimensions(Image::Format imageFormat, int &minWidth, int &minHeight) {
    switch (imageFormat) {
    case Image::RGBA_DXT1:
    case Image::RGBA_DXT3:
    case Image::RGBA_DXT5:
    case Image::XGBR_DXT5:
    case Image::DXN1:
    case Image::DXN2:
        minWidth = 4;
        minHeight = 4;
        return true;
    case Image::RGB_PVRTC_2BPPV1:
    case Image::RGBA_PVRTC_2BPPV1:
        minWidth = 16;
        minHeight = 8;
        return true;
    case Image::RGB_PVRTC_4BPPV1:
    case Image::RGBA_PVRTC_4BPPV1:
        minWidth = 8;
        minHeight = 8;
        return true;
    case Image::RGBA_PVRTC_2BPPV2:
        minWidth = 8;
        minHeight = 4;
        return true;
    case Image::RGBA_PVRTC_4BPPV2:
        minWidth = 4;
        minHeight = 4;
        return true;
    case Image::RGB_8_ETC1:
    case Image::RGB_8_ETC2:
    case Image::RGBA_8_8_ETC2:
    case Image::RGBA_8_1_ETC2:
    case Image::R_11_EAC:
    case Image::SignedR_11_EAC:
    case Image::RG_11_11_EAC:
    case Image::SignedRG_11_11_EAC:
        minWidth = 4;
        minHeight = 4;
        return true;
    case Image::RGB_ATC:
    case Image::RGBA_EA_ATC: // Explicit alpha
    case Image::RGBA_IA_ATC: // Interpolated alpha
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
    case Image::RGBA_DXT1:
    case Image::RGBA_DXT3:
    case Image::RGBA_DXT5:
    case Image::XGBR_DXT5:
    case Image::DXN1:
    case Image::DXN2:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::RGB_PVRTC_2BPPV1:
    case Image::RGBA_PVRTC_2BPPV1:
        blockWidth = 8;
        blockHeight = 4;
        return true;
    case Image::RGB_PVRTC_4BPPV1:
    case Image::RGBA_PVRTC_4BPPV1:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::RGBA_PVRTC_2BPPV2:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::RGBA_PVRTC_4BPPV2:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::RGB_8_ETC1:
    case Image::RGB_8_ETC2:
    case Image::RGBA_8_8_ETC2:
    case Image::RGBA_8_1_ETC2:
    case Image::R_11_EAC:
    case Image::SignedR_11_EAC:
    case Image::RG_11_11_EAC:
    case Image::SignedRG_11_11_EAC:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::RGB_ATC:
    case Image::RGBA_EA_ATC: // Explicit alpha
    case Image::RGBA_IA_ATC: // Interpolated alpha
        blockWidth = 4;
        blockHeight = 4;
        return false;
    default:
        return false;
    }
    
    return false;
}

BE_NAMESPACE_END
