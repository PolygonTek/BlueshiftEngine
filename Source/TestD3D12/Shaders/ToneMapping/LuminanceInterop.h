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

#ifndef __LUMINANCE_INTEROP_H__
#define __LUMINANCE_INTEROP_H__

#include "../ShaderTypes.h"

#ifdef __cplusplus
#include <cstddef>
#endif

static const uint LuminanceBlockSize = 32;
static const uint LuminanceHistogramBinCount = LuminanceBlockSize * LuminanceBlockSize;
static const uint LuminanceBufferOffsetExposure = 0;
static const uint LuminanceBufferOffsetLuminance = LuminanceBufferOffsetExposure + 4;
static const uint LuminanceBufferOffsetHistogram = LuminanceBufferOffsetLuminance + 4;

struct LuminancePass1Params {
    uint2 sampleSize;
    float2 sampleSizeRcp;
    float luminanceLogMin;
    float luminanceLogRangeRcp;
};

struct LuminancePass2Params {
    float luminanceLogMin;
    float luminanceLogRange;
    float luminancePixelCount;
    float luminanceAdaptationRate;
    float middleGray;
    float exposureCompensation;
    float minExposure;
    float maxExposure;
    float frameTime;
};

#ifdef __cplusplus
static_assert(sizeof(LuminancePass2Params) == 36, "LuminancePass2Params must match the nine HLSL root constants");
static_assert(offsetof(LuminancePass2Params, frameTime) == 32, "Frame time must occupy the ninth root constant");
#endif

#endif // __LUMINANCE_INTEROP_H__
