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

#include "Precompiled.h"
#include "Core/LinearSamplingKernel.h"

BE_NAMESPACE_BEGIN

// Generate Gaussian weights based on kernel size.
void LinearSamplingKernelBase::ComputeGaussianWeights(int kernelSize, float *weights) {
    assert(kernelSize > 0 && (kernelSize & 1) == 1);

    const float sigma = kernelSize / 6.0f;
    const float coeff = 2.0f * sigma * sigma;
    float weightSum = 0.0f;

    for (int i = 0; i < kernelSize; ++i) {
        const float x = (float)i - kernelSize * 0.5f + 0.5f;
        weights[i] = Math::Exp(-x * x / coeff);
        weightSum += weights[i];
    }

    const float invWeightSum = 1.0f / weightSum;

    for (int i = 0; i < kernelSize; ++i) {
        weights[i] *= invWeightSum;
    }
}

// Generate weights and offsets for filter, taking advantage of linear texture filtering.
// Reference: GPU Gems 2, Chapter 20. Fast Third Order Texture Filtering
void LinearSamplingKernelBase::ComputeLinearSamplingWeightsAndOffsets(int kernelSize, const float *weights, float *linearWeights, Vec2 *hOffsets, Vec2 *vOffsets) {
    assert(kernelSize > 0 && (kernelSize & 1) == 1);

    const int linearKernelSize = (kernelSize + 1) >> 1;
    const float kernelRadius = (kernelSize - 1) * 0.5f;

    for (int i = 0; i < linearKernelSize; ++i) {
        const int sampleIndex = i * 2;
        const float a = weights[sampleIndex];
        const float b = sampleIndex + 1 < kernelSize ? weights[sampleIndex + 1] : 0.0f;
        const float weight = a + b;
        assert(weight > 0.0f);

        linearWeights[i] = weight;

        const float offset = b / weight;
        assert(offset >= 0.0f && offset <= 1.0f);

        const float sampleOffset = sampleIndex - kernelRadius + offset;
        hOffsets[i] = Vec2(sampleOffset, 0.0f);
        vOffsets[i] = Vec2(0.0f, sampleOffset);
    }
}

BE_NAMESPACE_END
