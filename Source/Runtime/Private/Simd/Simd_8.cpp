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

void BE_FASTCALL SIMD_8::Matrix4x4Multiply(float *dst, const float *src0, const float *src1) {
    assert_32_byte_aligned(dst);
    assert_32_byte_aligned(src0);
    assert_32_byte_aligned(src1);

    simd8f ar01 = load_256ps(src0);
    simd8f ar23 = load_256ps(src0 + 8);

    simd8f br00 = broadcast_256ps((simd4f *)src1);
    simd8f br11 = broadcast_256ps((simd4f *)(src1 + 4));
    simd8f br22 = broadcast_256ps((simd4f *)(src1 + 8));
    simd8f br33 = broadcast_256ps((simd4f *)(src1 + 12));

    store_256ps(lincomb4x8(ar01, br00, br11, br22, br33), dst);
    store_256ps(lincomb4x8(ar23, br00, br11, br22, br33), dst + 8);
}

BE_NAMESPACE_END

#endif