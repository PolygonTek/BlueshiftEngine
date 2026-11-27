// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __SHADOW_DEPTH_INTEROP_H__
#define __SHADOW_DEPTH_INTEROP_H__

#include "../ShaderTypes.h"

// Shadow depth 기록에 필요한 광원 행렬. Surface table 은 프레임 상수를 참조하고, 샘플링용 데이터는 별도로 전달한다.
struct ShadowViewConstants {
    row_major float4x4 viewProjMatrix;
};

#ifdef __cplusplus
static_assert(sizeof(ShadowViewConstants) == 64, "ShadowViewConstants must match the HLSL constant-buffer layout");
#endif

#endif // __SHADOW_DEPTH_INTEROP_H__
