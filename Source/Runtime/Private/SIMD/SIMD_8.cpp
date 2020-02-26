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
#include "SIMD/SIMD.h"

#if defined(ENABLE_X86_AVX_INTRINSICS)

BE_NAMESPACE_BEGIN

const simd8f SIMD_8::F8_zero                 = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
const simd8f SIMD_8::F8_one                  = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
const simd8f SIMD_8::F8_half                 = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
const simd8f SIMD_8::F8_255                  = { 255.0f, 255.0f, 255.0f, 255.0f, 255.0f, 255.0f, 255.0f, 255.0f };
const simd8f SIMD_8::F8_min_char             = { -128.0f, -128.0f, -128.0f, -128.0f, -128.0f, -128.0f, -128.0f, -128.0f };
const simd8f SIMD_8::F8_max_char             = { 127.0f, 127.0f, 127.0f, 127.0f, 127.0f, 127.0f, 127.0f, 127.0f };
const simd8f SIMD_8::F8_min_short            = { -32768.0f, -32768.0f, -32768.0f, -32768.0f, -32768.0f, -32768.0f, -32768.0f, -32768.0f };
const simd8f SIMD_8::F8_max_short            = { 32767.0f, 32767.0f, 32767.0f, 32767.0f, 32767.0f, 32767.0f, 32767.0f, 32767.0f };
const simd8f SIMD_8::F8_tiny                 = { 1e-4f, 1e-4f, 1e-4f, 1e-4f, 1e-4f, 1e-4f, 1e-4f, 1e-4f };
const simd8f SIMD_8::F8_smallestNonDenorm    = { 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f, 1.1754944e-038f };
const simd8f SIMD_8::F8_sign_bit             = (simd8f &)simd8i(0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000);

void BE_FASTCALL SIMD_8::MulMat3x4RM(float *dst, const float *src0, const float *src1) {
    assert_32_byte_aligned(dst);
    assert_32_byte_aligned(src0);
    assert_32_byte_aligned(src1);

    simd8f ar01 = load_256ps(src0);
    simd4f ar2 = load_ps(src0 + 8);

    simd8f br00 = broadcast_256ps((simd4f *)(src1 + 0));
    simd8f br11 = broadcast_256ps((simd4f *)(src1 + 4));
    simd8f br22 = broadcast_256ps((simd4f *)(src1 + 8));
    simd8f br33 = broadcast_256ps((simd4f *)(src1 + 12));

    store_256ps(lincomb2x4x2x4(ar01, br00, br11, br22, br33), dst);
    store_ps(lincomb4x4(ar2, br00.l128, br11.l128, br22.l128, br33.l128), dst + 8);
}

void BE_FASTCALL SIMD_8::MulMat4x4RM(float *dst, const float *src0, const float *src1) {
    assert_32_byte_aligned(dst);
    assert_32_byte_aligned(src0);
    assert_32_byte_aligned(src1);

    simd8f ar01 = load_256ps(src0);
    simd8f ar23 = load_256ps(src0 + 8);

    simd8f br00 = broadcast_256ps((simd4f *)src1);
    simd8f br11 = broadcast_256ps((simd4f *)(src1 + 4));
    simd8f br22 = broadcast_256ps((simd4f *)(src1 + 8));
    simd8f br33 = broadcast_256ps((simd4f *)(src1 + 12));

    store_256ps(lincomb2x4x2x4(ar01, br00, br11, br22, br33), dst);
    store_256ps(lincomb2x4x2x4(ar23, br00, br11, br22, br33), dst + 8);
}

BE_NAMESPACE_END

#endif