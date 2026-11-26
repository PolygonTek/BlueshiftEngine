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

#ifndef __TONE_MAPPING_INTEROP_H__
#define __TONE_MAPPING_INTEROP_H__

#include "../ShaderTypes.h"

static const uint ToneMappingOperatorLinear = 0;
static const uint ToneMappingOperatorExponential = 1;
static const uint ToneMappingOperatorLogarithmic = 2;
static const uint ToneMappingOperatorDragoLogarithmic = 3;
static const uint ToneMappingOperatorReinhard = 4;
static const uint ToneMappingOperatorReinhardExtended = 5;
static const uint ToneMappingOperatorFilmicALU = 6;
static const uint ToneMappingOperatorFilmicACES = 7;
static const uint ToneMappingOperatorFilmicUnreal3 = 8;
static const uint ToneMappingOperatorFilmicUncharted2 = 9;
static const uint ToneMappingOperatorFilmicGT = 10;
static const uint ToneMappingOperatorGT7 = 11;
static const uint ToneMappingOperatorPBRNeutral = 12;
static const uint ToneMappingOperatorAgX = 13;
static const uint ToneMappingOperatorAgXPunch = 14;

struct GT7ToneMappingParams {
    float3 shoulder;               // Precomputed kA, kB, kC for the highlight curve.
    float linearEnd;
    float inputScale;              // Engine linear units to GT7's 100-nit units.
    float outputScale;             // GT7's 100-nit units to display-linear engine units.
    float peakIntensity;
    float inverseTargetIntensity;  // Reciprocal of the target's ICtCp intensity.
};

#ifdef __cplusplus
static_assert(sizeof(GT7ToneMappingParams) == 32, "GT7ToneMappingParams must match the HLSL constant-buffer layout");
#endif

#endif // __TONE_MAPPING_INTEROP_H__
