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

#pragma once

#include "Platform/Intrinsics.h"

BE_FORCE_INLINE neonb set1_b32(bool a) {
    return vdupq_n_u32(a ? 0xFFFFFFFF : 0);
}

BE_FORCE_INLINE neonb set_b32(bool a, bool b, bool c, bool d) {
    ALIGN_AS16 uint32_t data[4] = { a ? 0xFFFFFFFF : 0, b ? 0xFFFFFFFF : 0, c ? 0xFFFFFFFF : 0, d ? 0xFFFFFFFF : 0 };
    return vld1q_u32(data);
}

BE_FORCE_INLINE neonb ps_to_b32(const ssef &a) {
    return vcvtq_u32_f32(a);
}

BE_FORCE_INLINE neonb operator!(const neonb &a) { return vmvnq_u32(a); }

BE_FORCE_INLINE neonb operator&(const neonb &a, const neonb &b) { return vandq_s32(a, b); }
BE_FORCE_INLINE neonb operator|(const neonb &a, const neonb &b) { return vorrq_s32(a, b); }
BE_FORCE_INLINE neonb operator^(const neonb &a, const neonb &b) { return veorq_s32(a, b); }

BE_FORCE_INLINE neonb operator&=(neonb &a, const neonb &b) { return a = a & b; }
BE_FORCE_INLINE neonb operator|=(neonb &a, const neonb &b) { return a = a | b; }
BE_FORCE_INLINE neonb operator^=(neonb &a, const neonb &b) { return a = a ^ b; }

BE_FORCE_INLINE neonb operator!=(const neonb &a, const neonb &b) { return vmvnq_u32(vceqq_u32(a, b)); }
BE_FORCE_INLINE neonb operator==(const neonb &a, const neonb &b) { return vceqq_u32(a, b); }

// Selects 4x32 bits using mask.
BE_FORCE_INLINE neonb select_b32(const neonb &a, const neonb &b, const neonb &mask) {
    return vbslq_u32(mask, a, b);
}

// Unpacks to (a0, b0, a1, b1).
BE_FORCE_INLINE neonb unpacklo_b32(const neonb &a, const neonb &b) { return unpacklo_ps(a, b); }

// Unpacks to (a2, b2, a3, b3).
BE_FORCE_INLINE neonb unpackhi_b32(const neonb &a, const neonb &b) { return unpackhi_ps(a, b); }

BE_FORCE_INLINE size_t movemask_b32(const neonb &a) { return vmovemaskq_f32(a); }

BE_FORCE_INLINE bool reduce_and_b32(const neonb &a) { return movemask_b32(a) == 0xf; }
BE_FORCE_INLINE bool reduce_or_b32(const neonb &a) { return movemask_b32(a) != 0x0; }
BE_FORCE_INLINE bool all_b32(const neonb &a) { return movemask_b32(a) == 0xf; }
BE_FORCE_INLINE bool any_b32(const neonb &a) { return movemask_b32(a) != 0x0; }
BE_FORCE_INLINE bool none_b32(const neonb &a) { return movemask_b32(a) == 0x0; }
