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

#ifndef __TAA_INTEROP_H__
#define __TAA_INTEROP_H__

#include "../ShaderTypes.h"

static const uint TAAResolveBlockSizeX = 16;
static const uint TAAResolveBlockSizeY = 16;

struct TAAResolveParams {
    uint2 outputSize;
    float2 outputTexelSize;
    float historyWeight;
    uint historyValid;
    uint reverseZ;
    float depthFallbackThreshold;
    float maxSubpixelCurrentWeight;
    float luminanceWeight;
    float colorBoxSigma;
};

#ifdef __cplusplus
static_assert(sizeof(TAAResolveParams) == 44, "TAAResolveParams must match the HLSL root constants");
#endif

#endif // __TAA_INTEROP_H__
