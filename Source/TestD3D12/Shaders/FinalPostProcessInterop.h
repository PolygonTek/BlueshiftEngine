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

#ifndef __FINAL_POST_PROCESS_INTEROP_H__
#define __FINAL_POST_PROCESS_INTEROP_H__

#include "ShaderTypes.h"
#include "ToneMapping/ToneMappingInterop.h"

static const uint FinalPostProcessBlockSizeX = 16;
static const uint FinalPostProcessBlockSizeY = 16;

static const uint FinalPostProcessFlagToneMapping = 1u << 0;
static const uint FinalPostProcessFlagDithering = 1u << 1;
static const uint FinalPostProcessFlagColorGrading = 1u << 2;
static const uint FinalPostProcessFlagHDROutput = 1u << 3;

struct FinalPostProcessParams {
    uint2 outputSize;
    float2 outputTexelSize;
    uint flags;
    uint toneMappingOperator;
    float bloomIntensity;
    float chromaticAberration;
    float vignetteIntensity;
    float lensDistortion;
    float lensDistortionFitScale;
    float lensDistortionRadiusSqScale;
    float3 whiteBalance;
    float bloomDirtIntensity;
    float2 bloomDirtUvScale;
    float paperWhiteNits;           // Absolute luminance represented by linear 1.0.
    float maxDisplayLinearValue;    // Peak luminance in paper-white-relative linear units.
    GT7ToneMappingParams gt7;
};

#ifdef __cplusplus
static_assert(sizeof(FinalPostProcessParams) == 112, "FinalPostProcessParams must match the HLSL constant-buffer layout");
#endif

#endif // __FINAL_POST_PROCESS_INTEROP_H__
