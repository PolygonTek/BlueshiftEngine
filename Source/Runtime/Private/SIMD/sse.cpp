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

#ifdef HAVE_X86_SSE_INTRIN

#include "SIMD/SSE/sse.h"

extern const __m128 _mm_lookupmask_ps[16] = {
    _mm_castsi128_ps(_mm_set_epi32( 0,  0,  0,  0)),
    _mm_castsi128_ps(_mm_set_epi32( 0,  0,  0, -1)),
    _mm_castsi128_ps(_mm_set_epi32( 0,  0, -1,  0)),
    _mm_castsi128_ps(_mm_set_epi32( 0,  0, -1, -1)),
    _mm_castsi128_ps(_mm_set_epi32( 0, -1,  0,  0)),
    _mm_castsi128_ps(_mm_set_epi32( 0, -1,  0, -1)),
    _mm_castsi128_ps(_mm_set_epi32( 0, -1, -1,  0)),
    _mm_castsi128_ps(_mm_set_epi32( 0, -1, -1, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1,  0,  0,  0)),
    _mm_castsi128_ps(_mm_set_epi32(-1,  0,  0, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1,  0, -1,  0)),
    _mm_castsi128_ps(_mm_set_epi32(-1,  0, -1, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1,  0,  0)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1,  0, -1)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1, -1,  0)),
    _mm_castsi128_ps(_mm_set_epi32(-1, -1, -1, -1))
};

extern const __m128d _mm_lookupmask_pd[4] = {
    _mm_castsi128_pd(_mm_set_epi32( 0,  0,  0,  0)),
    _mm_castsi128_pd(_mm_set_epi32( 0,  0, -1, -1)),
    _mm_castsi128_pd(_mm_set_epi32(-1, -1,  0,  0)),
    _mm_castsi128_pd(_mm_set_epi32(-1, -1, -1, -1))
};

#endif
