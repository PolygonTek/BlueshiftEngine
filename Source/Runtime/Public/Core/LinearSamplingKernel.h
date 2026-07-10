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

#include "Math/Math.h"

BE_NAMESPACE_BEGIN

class BE_API LinearSamplingKernelBase {
protected:
    static void             ComputeGaussianWeights(int kernelSize, float *weights);
    static void             ComputeLinearSamplingWeightsAndOffsets(int kernelSize, const float *weights, float *linearWeights, Vec2 *hOffsets, Vec2 *vOffsets);
};

template <int KernelSize>
class LinearSamplingKernel : private LinearSamplingKernelBase {
public:
    static_assert(KernelSize > 0 && (KernelSize & 1) == 1, "KernelSize must be odd");

    static constexpr int    SampleCount = (KernelSize + 1) >> 1;

                            // Gaussian input weight 를 만들고, bilinear sampling 으로 합칠 sample weight/offset 을 계산한다.
    void                    BuildGaussian();

    const float *           GetSampleWeights() const { return sampleWeights; }
    const Vec2 *            GetHorizontalOffsets() const { return hOffsets; }
    const Vec2 *            GetVerticalOffsets() const { return vOffsets; }

private:
    float                   inputWeights[KernelSize];       // discrete weights
    float                   sampleWeights[SampleCount];     // linear sampling weights
    Vec2                    hOffsets[SampleCount];          // 1D horizontal linear sampling offsets
    Vec2                    vOffsets[SampleCount];          // 1D vertical linear sampling offsets
};

template <int KernelSize>
BE_INLINE void LinearSamplingKernel<KernelSize>::BuildGaussian() {
    ComputeGaussianWeights(KernelSize, inputWeights);
    ComputeLinearSamplingWeightsAndOffsets(KernelSize, inputWeights, sampleWeights, hOffsets, vOffsets);
}

BE_NAMESPACE_END
