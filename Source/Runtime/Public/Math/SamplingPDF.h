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

#pragma once

#include "Math/MathCommon.h"

BE_NAMESPACE_BEGIN

class SamplingPDF {
public:
                                /// Arc-length PDF on the uniformly sampled unit circle: 1 / (2 * pi).
    static constexpr float      UniformCircle() { return Math::InvTwoPi; }

                                /// Area PDF within the uniformly sampled unit disk: 1 / pi.
                                /// Applies to both Vec2::FromUniformSampleDisk() and Vec2::FromConcentricSampleDisk().
    static constexpr float      UniformDisk() { return 0.5f * Math::InvTwoPi; }

                                /// Solid-angle PDF for uniform unit-sphere sampling: 1 / (4 * pi).
    static constexpr float      UniformSphere() { return 0.5f * Math::InvTwoPi; }

                                /// Solid-angle PDF within the uniformly sampled hemisphere: 1 / (2 * pi).
    static constexpr float      UniformHemisphere() { return Math::InvTwoPi;}

                                /// Solid-angle PDF for cosine-weighted hemisphere sampling.
                                /// cosTheta is the cosine to the sampling axis in [0, 1]; no clamping is performed.
    static constexpr float      CosineHemisphere(float cosTheta) { return cosTheta *Math::InvPi; }

                                /// Solid-angle PDF for power-cosine-weighted hemisphere sampling.
                                /// Requires cosTheta in [0, 1] and finite power >= 0; no clamping is performed.
    static float                PowerCosineHemisphere(float cosTheta, float power);
};

BE_INLINE float SamplingPDF::PowerCosineHemisphere(float cosTheta, float power) {
    return Math::Pow(cosTheta, power) * (power + 1.0f) * Math::InvTwoPi;
}

BE_NAMESPACE_END
