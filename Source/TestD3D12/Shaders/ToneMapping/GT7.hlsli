// Adapted from Polyphony Digital's GT7 Tone Mapping sample:
// https://blog.selfshadow.com/publications/s2025-shading-course/pdi/supplemental/gt7_tone_mapping.cpp
//
// MIT License
//
// Copyright (c) 2025 Polyphony Digital Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// HLSL adaptation with linear Rec.709 I/O and CPU-precomputed display parameters.
#ifndef __GT7_HLSLI__
#define __GT7_HLSLI__

#include "../ShaderInterop.h"
#include "../Colors.hlsli"

// The reference curve uses 100-nit units, with a 0.538 midpoint and 1.280 toe.
float3 GT7EvaluateCurve(float3 x, GT7ToneMappingParams params) {
    float3 toeWeight = 1.0 - smoothstep(0.0, 0.538, x);
    // The toe has zero weight beyond the midpoint. Bound its input so unused
    // pow results cannot overflow for very bright emissives.
    float3 toe = 0.538 * pow(min(x / 0.538, 1.0), 1.280);
    float3 toeLinear = lerp(x, toe, toeWeight);
    float3 shoulder = params.shoulder.x + params.shoulder.y * exp(x * params.shoulder.z);
    return select(x < params.linearEnd, toeLinear, shoulder);
}

// Linear Rec.2020 in GT7's 100-nit units -> PQ-encoded LMS.
float3 GT7RGBToLMSPQ(float3 rgb) {
    const float3x3 rgbToLms = {
        1688.0 / 4096.0, 2146.0 / 4096.0, 262.0 / 4096.0,
         683.0 / 4096.0, 2951.0 / 4096.0, 462.0 / 4096.0,
          99.0 / 4096.0,  309.0 / 4096.0, 3688.0 / 4096.0
    };
    // Colors.hlsli's PQ functions use 1.0 = 10,000 nits.
    return ApplyREC2084Curve(mul(rgbToLms, rgb) * 0.01);
}

float3 GT7RGBToICtCp(float3 rgb) {
    float3 lmsPQ = GT7RGBToLMSPQ(rgb);
    const float3x3 lmsToICtCp = {
        0.5, 0.5, 0.0,
        6610.0 / 4096.0, -13613.0 / 4096.0, 7003.0 / 4096.0,
        17933.0 / 4096.0, -17390.0 / 4096.0, -543.0 / 4096.0
    };
    return mul(lmsToICtCp, lmsPQ);
}

float3 GT7ICtCpToRGB(float3 ictcp) {
    const float3x3 ictcpToLms = {
        1.0,  0.00860904,  0.11103,
        1.0, -0.00860904, -0.11103,
        1.0,  0.560031,   -0.320627
    };
    const float3x3 lmsToRgb = {
         3.43661,   -2.50645,    0.0698454,
        -0.79133,    1.9836,    -0.192271,
        -0.0259499, -0.0989137,  1.12486
    };
    // Match the reference's inverse-PQ domain and nonnegative Rec.2020 result.
    float3 lms = RemoveREC2084Curve(saturate(mul(ictcpToLms, ictcp))) * 100.0;
    return max(mul(lmsToRgb, lms), 0.0);
}

// Exposed nonnegative linear Rec.709 in, display-linear Rec.709 out.
// SDR and HDR share the same operator; input/output scales preserve the engine's
// paper-white convention. Display encoding remains in the existing output pass.
float3 ToneMapGT7(float3 color, GT7ToneMappingParams params) {
    float3 rgb = REC709toREC2020(max(color, 0.0)) * params.inputScale;
    float3 originalICtCp = GT7RGBToICtCp(rgb);
    float3 skewedRgb = GT7EvaluateCurve(rgb, params);

    // Only intensity is needed from the tone-mapped color. The unused S/PQ
    // component and chroma matrix are eliminated by the shader compiler.
    float2 skewedLmsPQ = GT7RGBToLMSPQ(skewedRgb).xy;
    float skewedIntensity = 0.5 * (skewedLmsPQ.x + skewedLmsPQ.y);
    float chromaScale = 1.0 - smoothstep(0.98, 1.16, originalICtCp.x * params.inverseTargetIntensity);
    float3 scaledRgb = GT7ICtCpToRGB(float3(skewedIntensity, originalICtCp.yz * chromaScale));

    float3 mappedRgb = min(lerp(skewedRgb, scaledRgb, 0.6), params.peakIntensity);
    // The current grading/composition path is Rec.709. Bound out-of-gamut
    // channels here before grading; a wide-gamut composition path is separate.
    return clamp(REC2020toREC709(mappedRgb) * params.outputScale, 0.0, params.peakIntensity * params.outputScale);
}

#endif // __GT7_HLSLI__

