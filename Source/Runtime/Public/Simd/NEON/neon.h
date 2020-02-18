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

#ifdef __ARM_NEON__
#include "arm_neon.h"
#endif

struct neon128 {
    union {
        float32x4_t     f32;
        uint64x2_t      ui64;
        uint32x4_t      ui32;
        uint16x8_t      ui16;
        uint8x16_t      ui8;
        int64x2_t       i64;
        int32x4_t       i32;
        int16x8_t       i16;
        int8x16_t       i8;
    };
};
