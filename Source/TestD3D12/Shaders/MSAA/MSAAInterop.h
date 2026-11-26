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

#ifndef __MSAA_INTEROP_H__
#define __MSAA_INTEROP_H__

#include "../ShaderTypes.h"

static const uint MSAAResolveDepthBlockSizeX = 16;
static const uint MSAAResolveDepthBlockSizeY = 16;
static const uint MSAAResolveVelocityBlockSizeX = 16;
static const uint MSAAResolveVelocityBlockSizeY = 16;

struct MSAAResolveDepthParams {
    uint2 outputSize;
    uint reverseZ;
};

#ifdef __cplusplus
static_assert(sizeof(MSAAResolveDepthParams) == 12, "MSAAResolveDepthParams must match the three HLSL root constants");
#endif

struct MSAAResolveVelocityParams {
    uint2 outputSize;
    uint reverseZ;
};

#ifdef __cplusplus
static_assert(sizeof(MSAAResolveVelocityParams) == 12, "MSAAResolveVelocityParams must match the three HLSL root constants");
#endif

#endif // __MSAA_INTEROP_H__
