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

// CPU setup for the HLSL operator's display-dependent curve coefficients.
#pragma once

#include <cassert>
#include <cmath>
#include "Shaders/ShaderInterop.h"

inline void InitializeGT7ToneMappingParams(float referenceWhiteNits, float targetNits, GT7ToneMappingParams &params) {
    // The published curve parameters assume a target of at least 250 nits.
    assert(referenceWhiteNits > 0.0f && targetNits >= 250.0f && targetNits <= 10000.0f);

    constexpr float referenceLuminance = 100.0f;
    constexpr float alpha = 0.25f;
    constexpr float linearSection = 0.444f;
    constexpr float k = (linearSection - 1.0f) / (alpha - 1.0f);
    const float peak = targetNits / referenceLuminance;

    params.shoulder = float3(peak * (linearSection + k), -peak * k * std::exp(linearSection / k), -1.0f / (k * peak));
    params.linearEnd = linearSection * peak;
    params.inputScale = referenceWhiteNits / referenceLuminance;
    params.outputScale = referenceLuminance / referenceWhiteNits;
    params.peakIntensity = peak;

    // For neutral RGB, L = M = targetNits and ICtCp intensity equals PQ(target).
    // Compute this once per dispatch, rather than running PQ in every pixel.
    constexpr float m1 = 2610.0f / 16384.0f;
    constexpr float m2 = 2523.0f / 32.0f;
    constexpr float c1 = 3424.0f / 4096.0f;
    constexpr float c2 = 2413.0f / 128.0f;
    constexpr float c3 = 2392.0f / 128.0f;
    const float lp = std::pow(targetNits / 10000.0f, m1);
    params.inverseTargetIntensity = 1.0f / std::pow((c1 + c2 * lp) / (1.0f + c3 * lp), m2);
}
