// Rec.709 adaptation originally derived from three.js (MIT License):
// https://github.com/mrdoob/three.js/blob/dev/src/renderers/shaders/ShaderChunk/tonemapping_pars_fragment.glsl.js
// Copyright (c) 2010-2026 three.js authors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Contrast curve and Look adapted from Filament's ToneMapper.cpp:
// https://github.com/google/filament/blob/main/filament/src/ToneMapper.cpp
// Copyright (C) 2021 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// Modified for HLSL with per-operator Look selection and linear Rec.709 I/O.

#ifndef __AGX_HLSLI__
#define __AGX_HLSLI__

#include "../Colors.hlsli"

static const uint AgXLookNone = 0;
static const uint AgXLookPunchy = 1;

// Filament's default AgX contrast fit, evaluated in Horner form.
float3 AgXDefaultContrastApprox(float3 x) {
    return ((((((-17.86 * x + 78.01) * x - 126.7) * x + 92.06) * x - 28.72) * x + 4.361) * x - 0.1718) * x + 0.002857;
}

float3 AgXApplyLook(float3 color, uint look) {
    if (look == AgXLookNone) {
        return color;
    }

    // Filament deliberately computes luma before the slope/offset/power operation.
    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
    const float3 offset = float3(0.0, 0.0, 0.0);
    const float3 slope = float3(1.0, 1.0, 1.0);
    float3 power = float3(1.0, 1.0, 1.0);
    float saturation = 1.0;

    if (look == AgXLookPunchy) {
        power = float3(1.35, 1.35, 1.35);
        saturation = 1.4;
    }

    color = pow(color * slope + offset, power);
    return luminance + saturation * (color - luminance);
}

// Exposed nonnegative linear Rec.709 in, display-linear Rec.709 out. SDR AgX.
// Each tone mapping operator supplies a constant Look; base AgX uses None.
float3 ToneMapAgX(float3 color, uint look) {
    // Precombined AgX inset * REC709toREC2020 matrix from Colors.hlsli.
    const float3x3 inputMatrix = {
        0.544813028191, 0.373796126965, 0.081390749723,
        0.140415205159, 0.754143019743, 0.105441013856,
        0.088812109488, 0.178884955820, 0.732302857892
    };
    const float3x3 outsetMatrix = {
         1.127100581814437, -0.110606643096603, -0.016493938717835,
        -0.141329763498438,  1.157823702216272, -0.016493938717835,
        -0.141329763498438, -0.110606643096603,  1.251936406595041
    };
    // 16.5 stops: -10 to +6.5 EV relative to 18% middle gray.
    const float minEv = -12.47393;
    const float maxEv = 4.026069;

    color = mul(inputMatrix, color);
    color = saturate((log2(max(color, 1e-10)) - minEv) / (maxEv - minEv));
    color = AgXDefaultContrastApprox(color);
    color = AgXApplyLook(color, look);
    color = mul(outsetMatrix, color);

    // Undo AgX's display encoding; the existing output path handles sRGB encoding.
    color = pow(max(color, 0.0), 2.2);
    return saturate(REC2020toREC709(color));
}

#endif // __AGX_HLSLI__
