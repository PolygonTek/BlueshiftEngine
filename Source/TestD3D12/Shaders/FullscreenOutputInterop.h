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

#ifndef __FULLSCREEN_OUTPUT_INTEROP_H__
#define __FULLSCREEN_OUTPUT_INTEROP_H__

#include "ShaderTypes.h"

struct FullscreenImageParams {
    float2 texCoordScale;
    float2 texCoordOffset;
};

struct HDR10OutputParams {
    float2 texCoordScale;
    float2 texCoordOffset;
    float paperWhiteNits;
    float maxDisplayLinearValue;
};

struct DisplayP3EDROutputParams {
    float2 texCoordScale;
    float2 texCoordOffset;
    float maxDisplayLinearValue;
};

#ifdef __cplusplus
static_assert(sizeof(HDR10OutputParams) == 24, "HDR10OutputParams must match the HLSL root constants");
static_assert(sizeof(DisplayP3EDROutputParams) == 20, "DisplayP3EDROutputParams must match the HLSL root constants");
#endif

#endif // __FULLSCREEN_OUTPUT_INTEROP_H__
