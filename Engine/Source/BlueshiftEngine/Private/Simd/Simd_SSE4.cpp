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
#include "Math/Math.h"
#include "Core/JointPose.h"
#include "Simd/Simd.h"
#include "Simd/Simd_Generic.h"

#if defined(__X86__)

#include "Simd/Simd_SSE4.h"

BE_NAMESPACE_BEGIN

void BE_FASTCALL SIMD_SSE4::Add(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count > 16) {
        ssef c(constant);
        int c16 = count >> 4;
        while (c16 > 0) {
            //_mm_prefetch((const char *)(src_ptr + 16), _MM_HINT_NTA);

            ssef x0 = c + ssef(_mm_load_ps(src_ptr + 0));
            ssef x1 = c + ssef(_mm_load_ps(src_ptr + 4));
            ssef x2 = c + ssef(_mm_load_ps(src_ptr + 8));
            ssef x3 = c + ssef(_mm_load_ps(src_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

            src_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = constant + *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_SSE4::Add(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 16) {
        int c16 = count >> 4;
        while (c16 > 0) {
            //_mm_prefetch((const char *)(src0_ptr + 16), _MM_HINT_NTA);
            //_mm_prefetch((const char *)(src1_ptr + 16), _MM_HINT_NTA);

            ssef x0(_mm_load_ps(src0_ptr + 0));
            ssef x1(_mm_load_ps(src0_ptr + 4));
            ssef x2(_mm_load_ps(src0_ptr + 8));
            ssef x3(_mm_load_ps(src0_ptr + 12));

            x0 += ssef(_mm_load_ps(src1_ptr + 0));
            x1 += ssef(_mm_load_ps(src1_ptr + 4));
            x2 += ssef(_mm_load_ps(src1_ptr + 8));
            x3 += ssef(_mm_load_ps(src1_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

            src0_ptr += 16;
            src1_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = *src0_ptr++ + *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_SSE4::Sub(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count > 16) {
        ssef c(constant);
        int c16 = count >> 4;
        while (c16 > 0) {
            //_mm_prefetch((const char *)(src0_ptr + 16), _MM_HINT_NTA);

            ssef x0 = c - ssef(_mm_load_ps(src_ptr + 0));
            ssef x1 = c - ssef(_mm_load_ps(src_ptr + 4));
            ssef x2 = c - ssef(_mm_load_ps(src_ptr + 8));
            ssef x3 = c - ssef(_mm_load_ps(src_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

            src_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = constant + *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_SSE4::Sub(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 16) {
        int c16 = count >> 4;
        while (c16 > 0) {
            //_mm_prefetch((const char *)(src0_ptr + 16), _MM_HINT_NTA);
            //_mm_prefetch((const char *)(src1_ptr + 16), _MM_HINT_NTA);

            ssef x0(_mm_load_ps(src0_ptr + 0));
            ssef x1(_mm_load_ps(src0_ptr + 4));
            ssef x2(_mm_load_ps(src0_ptr + 8));
            ssef x3(_mm_load_ps(src0_ptr + 12));

            x0 -= ssef(_mm_load_ps(src1_ptr + 0));
            x1 -= ssef(_mm_load_ps(src1_ptr + 4));
            x2 -= ssef(_mm_load_ps(src1_ptr + 8));
            x3 -= ssef(_mm_load_ps(src1_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

            src0_ptr += 16;
            src1_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = *src0_ptr++ + *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_SSE4::Mul(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count > 16) {
        ssef c(constant);
        int c16 = count >> 4;
        while (c16 > 0) {
            //_mm_prefetch((const char *)(src0_ptr + 16), _MM_HINT_NTA);

            ssef x0 = c * ssef(_mm_load_ps(src_ptr + 0));
            ssef x1 = c * ssef(_mm_load_ps(src_ptr + 4));
            ssef x2 = c * ssef(_mm_load_ps(src_ptr + 8));
            ssef x3 = c * ssef(_mm_load_ps(src_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

            src_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = constant + *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_SSE4::Mul(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 16) {
        int c16 = count >> 4;
        while (c16 > 0) {
            //_mm_prefetch((const char *)(src0_ptr + 16), _MM_HINT_NTA);
            //_mm_prefetch((const char *)(src1_ptr + 16), _MM_HINT_NTA);

            ssef x0(_mm_load_ps(src0_ptr + 0));
            ssef x1(_mm_load_ps(src0_ptr + 4));
            ssef x2(_mm_load_ps(src0_ptr + 8));
            ssef x3(_mm_load_ps(src0_ptr + 12));

            x0 *= ssef(_mm_load_ps(src1_ptr + 0));
            x1 *= ssef(_mm_load_ps(src1_ptr + 4));
            x2 *= ssef(_mm_load_ps(src1_ptr + 8));
            x3 *= ssef(_mm_load_ps(src1_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

            src0_ptr += 16;
            src1_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = *src0_ptr++ + *src1_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_SSE4::Div(float *dst, const float constant, const float *src, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src_ptr = src;

    if (count > 16) {
        ssef c(constant);
        int c16 = count >> 4;
        while (c16 > 0) {
            //_mm_prefetch((const char *)(src0_ptr + 16), _MM_HINT_NTA);

            ssef x0 = c / ssef(_mm_load_ps(src_ptr + 0));
            ssef x1 = c / ssef(_mm_load_ps(src_ptr + 4));
            ssef x2 = c / ssef(_mm_load_ps(src_ptr + 8));
            ssef x3 = c / ssef(_mm_load_ps(src_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

            src_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = constant + *src_ptr++;
        count--;
    }
}

void BE_FASTCALL SIMD_SSE4::Div(float *dst, const float *src0, const float *src1, const int count0) {
    int count = count0;
    float *dst_ptr = dst;
    const float *src0_ptr = src0;
    const float *src1_ptr = src1;

    if (count > 16) {
        int c16 = count >> 4;
        while (c16 > 0) {
            //_mm_prefetch((const char *)(src0_ptr + 16), _MM_HINT_NTA);
            //_mm_prefetch((const char *)(src1_ptr + 16), _MM_HINT_NTA);

            ssef x0(_mm_load_ps(src0_ptr + 0));
            ssef x1(_mm_load_ps(src0_ptr + 4));
            ssef x2(_mm_load_ps(src0_ptr + 8));
            ssef x3(_mm_load_ps(src0_ptr + 12));

            x0 /= ssef(_mm_load_ps(src1_ptr + 0));
            x1 /= ssef(_mm_load_ps(src1_ptr + 4));
            x2 /= ssef(_mm_load_ps(src1_ptr + 8));
            x3 /= ssef(_mm_load_ps(src1_ptr + 12));
        
            _mm_store_ps(dst_ptr + 0, x0);
            _mm_store_ps(dst_ptr + 4, x1);
            _mm_store_ps(dst_ptr + 8, x2);
            _mm_store_ps(dst_ptr + 12, x3);

            src0_ptr += 16;
            src1_ptr += 16;
            dst_ptr += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dst_ptr++ = *src0_ptr++ + *src1_ptr++;
        count--;
    }
}

float BE_FASTCALL SIMD_SSE4::Sum(const float *src, const int count0) {
    int count = count0;
    const float *src_ptr = src;
    float ret = 0;
    
    if (count > 4) {
        int c4 = count >> 2;
        while (c4 > 0) {
            ret += reduce_add(ssef(_mm_load_ps(src_ptr)));
            src_ptr += 4;
            c4--;
        }

        count &= 3;
    }

    while (count > 0) {
        ret += *src_ptr++;
        count--;
    }

    return ret;
}

void BE_FASTCALL SIMD_SSE4::MatrixTranspose(float *dst, const float *src) {
    ssef a0(src);
    ssef a1(src + 4);
    ssef a2(src + 8);
    ssef a3(src + 12);
    ssef r0, r1, r2, r3;

    transpose(a0, a1, a2, a3, r0, r1, r2, r3);

    _mm_store_ps(dst, r0.m128);
    _mm_store_ps(dst + 4, r1.m128);
    _mm_store_ps(dst + 8, r2.m128);
    _mm_store_ps(dst + 12, r3.m128);
}

void BE_FASTCALL SIMD_SSE4::MatrixMultiply(float *dst, const float *src0, const float *src1) {
    __m128 b0 = _mm_load_ps(src1);
    __m128 b1 = _mm_load_ps(src1 + 4);
    __m128 b2 = _mm_load_ps(src1 + 8);
    __m128 b3 = _mm_load_ps(src1 + 12);

    // Row 0
    __m128 a0 = _mm_load_ps(src0);
    __m128 a1 = a0;
    __m128 a2 = a0;
    __m128 a3 = a0;

    a0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(0, 0, 0, 0));
    a1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(1, 1, 1, 1));
    a2 = _mm_shuffle_ps(a2, a2, _MM_SHUFFLE(2, 2, 2, 2));
    a3 = _mm_shuffle_ps(a3, a3, _MM_SHUFFLE(3, 3, 3, 3));

    a0 = _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a2, b2));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a3, b3));
    _mm_store_ps(dst, a0);

    // Row 1
    a0 = _mm_load_ps(src0 + 4);
    a1 = a0;
    a2 = a0;
    a3 = a0;

    a0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(0, 0, 0, 0));
    a1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(1, 1, 1, 1));   
    a2 = _mm_shuffle_ps(a2, a2, _MM_SHUFFLE(2, 2, 2, 2));
    a3 = _mm_shuffle_ps(a3, a3, _MM_SHUFFLE(3, 3, 3, 3));

    a0 = _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a2, b2));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a3, b3));
    _mm_store_ps(dst + 4, a0);

    // Row 2
    a0 = _mm_load_ps(src0 + 8);
    a1 = a0;
    a2 = a0;
    a3 = a0;

    a0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(0, 0, 0, 0));
    a1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(1, 1, 1, 1));  
    a2 = _mm_shuffle_ps(a2, a2, _MM_SHUFFLE(2, 2, 2, 2));
    a3 = _mm_shuffle_ps(a3, a3, _MM_SHUFFLE(3, 3, 3, 3));

    a0 = _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a2, b2));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a3, b3));
    _mm_store_ps(dst + 8, a0);

    // Row 3
    a0 = _mm_load_ps(src0 + 12);
    a1 = a0;
    a2 = a0;
    a3 = a0;

    a0 = _mm_shuffle_ps(a0, a0, _MM_SHUFFLE(0, 0, 0, 0));
    a1 = _mm_shuffle_ps(a1, a1, _MM_SHUFFLE(1, 1, 1, 1));
    a2 = _mm_shuffle_ps(a2, a2, _MM_SHUFFLE(2, 2, 2, 2));
    a3 = _mm_shuffle_ps(a3, a3, _MM_SHUFFLE(3, 3, 3, 3));

    a0 = _mm_add_ps(_mm_mul_ps(a0, b0), _mm_mul_ps(a1, b1));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a2, b2));
    a0 = _mm_add_ps(a0, _mm_mul_ps(a3, b3));
    _mm_store_ps(dst + 12, a0);
}

#if 0

static void SSE_Memcpy64B(void *dst, const void *src, const int count) {
    char *src_ptr = (char *)src;
    char *dst_ptr = (char *)dst;

    __m128i r0, r1, r2, r3;

    int c64 = count >> 6;
    while (c64 > 0) {
        //_mm_prefetch(src_ptr + 64, _MM_HINT_NTA);
        
        r0 = _mm_load_si128((__m128i *)src_ptr);
        r1 = _mm_load_si128((__m128i *)(src_ptr + 16));
        r2 = _mm_load_si128((__m128i *)(src_ptr + 32));
        r3 = _mm_load_si128((__m128i *)(src_ptr + 48));

        _mm_store_si128((__m128i *)dst_ptr, r0);
        _mm_store_si128((__m128i *)(dst_ptr + 16), r1);
        _mm_store_si128((__m128i *)(dst_ptr + 32), r2);
        _mm_store_si128((__m128i *)(dst_ptr + 48), r3);

        src_ptr += 64;
        dst_ptr += 64;

        c64--;
    }

    //_mm_mfence();
}

static void SSE_Memcpy2kB(void *dst, const void *src, const int count) {
    char *src_ptr = (char *)src;
    char *dst_ptr = (char *)dst;

    __m128i r0, r1, r2, r3, r4, r5, r6, r7;

    int c128;
    int c2k = count >> 11;
    while (c2k > 0) {
        c128 = 16;
        while (c128 > 0) {
            //_mm_prefetch(src_ptr + 128, _MM_HINT_NTA);
            //_mm_prefetch(src_ptr + 192, _MM_HINT_NTA);
            
            r0 = _mm_load_si128((__m128i *)(src_ptr + 0));
            r1 = _mm_load_si128((__m128i *)(src_ptr + 16));
            r2 = _mm_load_si128((__m128i *)(src_ptr + 32));
            r3 = _mm_load_si128((__m128i *)(src_ptr + 48));
            r4 = _mm_load_si128((__m128i *)(src_ptr + 64));
            r5 = _mm_load_si128((__m128i *)(src_ptr + 80));
            r6 = _mm_load_si128((__m128i *)(src_ptr + 96));
            r7 = _mm_load_si128((__m128i *)(src_ptr + 112));

            _mm_store_si128((__m128i *)(dst_ptr + 0), r0);
            _mm_store_si128((__m128i *)(dst_ptr + 16), r1);
            _mm_store_si128((__m128i *)(dst_ptr + 32), r2);
            _mm_store_si128((__m128i *)(dst_ptr + 48), r3);
            _mm_store_si128((__m128i *)(dst_ptr + 64), r4);
            _mm_store_si128((__m128i *)(dst_ptr + 80), r5);
            _mm_store_si128((__m128i *)(dst_ptr + 96), r6);
            _mm_store_si128((__m128i *)(dst_ptr + 112), r7);
                    
            src_ptr += 128;
            dst_ptr += 128;

            c128--;
        }
                
        c2k--;
    }

    //_mm_mfence();
}

static void SSE_MemcpyStream2kB(void *dst, const void *src, const int count) {
    char *tbuf = (char *)_alloca16(2048);
    char *src_ptr;
    char *src_next = (char *)src;
    char *dst_ptr = (char *)dst;
    char *dst_next;

    __m128i r0, r1, r2, r3, r4, r5, r6, r7;

    int c128;
    int c2k = count >> 11;
    while (c2k > 0) {
        // copy 2k into temporary buffer
        dst_next = dst_ptr;
        src_ptr = src_next;
        dst_ptr = tbuf;

        c128 = 16;
        while (c128 > 0) {
            _mm_prefetch(src_ptr + 128, _MM_HINT_NTA);
            _mm_prefetch(src_ptr + 192, _MM_HINT_NTA);
        
            r0 = _mm_load_si128((__m128i *)src_ptr);
            r1 = _mm_load_si128((__m128i *)(src_ptr + 16));
            r2 = _mm_load_si128((__m128i *)(src_ptr + 32));
            r3 = _mm_load_si128((__m128i *)(src_ptr + 48));
            r4 = _mm_load_si128((__m128i *)(src_ptr + 64));
            r5 = _mm_load_si128((__m128i *)(src_ptr + 80));
            r6 = _mm_load_si128((__m128i *)(src_ptr + 96));
            r7 = _mm_load_si128((__m128i *)(src_ptr + 112));

            _mm_store_si128((__m128i *)dst_ptr, r0);
            _mm_store_si128((__m128i *)(dst_ptr + 16), r1);
            _mm_store_si128((__m128i *)(dst_ptr + 32), r2);
            _mm_store_si128((__m128i *)(dst_ptr + 48), r3);
            _mm_store_si128((__m128i *)(dst_ptr + 64), r4);
            _mm_store_si128((__m128i *)(dst_ptr + 80), r5);
            _mm_store_si128((__m128i *)(dst_ptr + 96), r6);
            _mm_store_si128((__m128i *)(dst_ptr + 112), r7);

            src_ptr += 128;
            dst_ptr += 128;

            c128--;
        }

        // Now copy from L1 to system memory
        src_next = src_ptr;
        src_ptr = tbuf;
        dst_ptr = dst_next;
    
        c128 = 16;
        while (c128 > 0) {
            r0 = _mm_load_si128((__m128i *)src_ptr);
            r1 = _mm_load_si128((__m128i *)(src_ptr + 16));
            r2 = _mm_load_si128((__m128i *)(src_ptr + 32));
            r3 = _mm_load_si128((__m128i *)(src_ptr + 48));
            r4 = _mm_load_si128((__m128i *)(src_ptr + 64));
            r5 = _mm_load_si128((__m128i *)(src_ptr + 80));
            r6 = _mm_load_si128((__m128i *)(src_ptr + 96));
            r7 = _mm_load_si128((__m128i *)(src_ptr + 112));

            _mm_stream_si128((__m128i *)dst_ptr, r0);
            _mm_stream_si128((__m128i *)(dst_ptr + 16), r1);
            _mm_stream_si128((__m128i *)(dst_ptr + 32), r2);
            _mm_stream_si128((__m128i *)(dst_ptr + 48), r3);
            _mm_stream_si128((__m128i *)(dst_ptr + 64), r4);
            _mm_stream_si128((__m128i *)(dst_ptr + 80), r5);
            _mm_stream_si128((__m128i *)(dst_ptr + 96), r6);
            _mm_stream_si128((__m128i *)(dst_ptr + 112), r7);
        
            src_ptr += 128;
            dst_ptr += 128;

            c128--;
        }
                
        c2k--;
    }

    _mm_mfence();
}

// optimized memory copy routine that handles all alignment cases and block sizes efficiently
void BE_FASTCALL SIMD_SSE4::Memcpy(void *dest0, const void *src0, const int count0) {
    if (count0 > 4096 && !(((intptr_t)dest0 ^ (intptr_t)src0) & 15)) {
        byte *dest = (byte *)dest0;
        byte *src = (byte *)src0;

        // copy up to the first 16 byte aligned boundary
        int count = ((intptr_t)dest) & 15;
        if (count > 0) {
            memcpy(dest, src, count);
            dest += count;
            src += count;
        }

        count = count0 - count;

        // if there are multiple blocks of 2kB
        if (count & ~4095) {
            SSE_Memcpy2kB(dest, src, count);
            src += (count & ~2047);
            dest += (count & ~2047);
            count &= 2047;
        }

        // if there are blocks of 64 bytes
        if (count & ~63) {
            SSE_Memcpy64B(dest, src, count);
            src += (count & ~63);
            dest += (count & ~63);
            count &= 63;
        }

        // copy any remaining bytes
        memcpy(dest, src, count);
    } else {
        // use the regular one if we cannot copy 16 byte aligned
        memcpy(dest0, src0, count0);
    }
}

void BE_FASTCALL SIMD_SSE4::Memset(void *dest0, const int val, const int count0) {
    byte *dest = (byte *)dest0;
    int	count = count0;

    while (count > 0 && (((intptr_t)dest) & 15)) {
        *dest = val;
        dest++;
        count--;
    }

    if (!count) {
        return;
    }

    __m128i data = _mm_set1_epi8((char)val);

    if (count >= 128) {
        int c128 = count >> 7;
        while (c128 > 0) {
            _mm_store_si128((__m128i *)dest, data);
            _mm_store_si128((__m128i *)(dest + 16), data);
            _mm_store_si128((__m128i *)(dest + 32), data);
            _mm_store_si128((__m128i *)(dest + 48), data);
            _mm_store_si128((__m128i *)(dest + 64), data);
            _mm_store_si128((__m128i *)(dest + 80), data);
            _mm_store_si128((__m128i *)(dest + 96), data);
            _mm_store_si128((__m128i *)(dest + 112), data);

            dest += 128;
            c128--;
        }

        count &= 127;
    }

    if (count >= 16) {
        int c16 = count >> 4;
        while (c16 > 0) {
            _mm_store_si128((__m128i *)dest, data);

            dest += 16;
            c16--;
        }

        count &= 15;
    }

    while (count > 0) {
        *dest = val;
        dest++;
        count--;
    }
}

#endif

BE_NAMESPACE_END

#endif // #if defined(__X86__)
