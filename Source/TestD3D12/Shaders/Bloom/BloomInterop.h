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

#ifndef __BLOOM_INTEROP_H__
#define __BLOOM_INTEROP_H__

#include "../ShaderTypes.h"

static const uint BloomBlockSizeX = 16;
static const uint BloomBlockSizeY = 16;

static const uint ConvolutionBloomApertureEnergyReduceBlockSize = 256;

struct BloomPrefilterParams {
    uint2 outputSize;
    float2 inputTexelSize;
    float2 outputTexelSize;
    float threshold;
    float softThreshold;
    float prefilterQuality;
    float radius;
};

struct BloomDownsampleParams {
    uint2 outputSize;
    float2 inputTexelSize;
    float2 outputTexelSize;
    float radius;
};

struct BloomUpsampleParams {
    uint2 outputSize;
    float2 inputTexelSize;
    float2 outputTexelSize;
    float weight;
    float radius;
};

struct ConvolutionBloomKernelPrepareParams {
    uint2 outputSize;
    float2 outputTexelSize;
    float rotationCos;
    float rotationSin;
    float2 mappedKernelScale;
    float kernelMipLevel;
};

struct ConvolutionBloomScreenPrepareParams {
    uint2 outputSize;
    float2 contentOffset;
    float2 contentSize;
    float screenMipLevel;
};

struct ConvolutionBloomResolveParams {
    uint2 inputSize;
    uint2 outputSize;
    float2 outputTexelSize;
    float2 contentOffset;
    float2 contentSize;
};

struct ConvolutionBloomAperturePrepareParams {
    uint2 outputSize;
    float2 outputTexelSize;
    float apertureScale;
    float apertureMipLevel;
};

struct ConvolutionBloomReduceApertureEnergyParams {
    uint inputElementCount;
    uint inputIsPartialEnergy;
};

struct ConvolutionBloomResolveDiffractionPSFParams {
    uint2 inputSize;
    uint2 outputSize;
};

struct ConvolutionBloomApplyDispersionParams {
    uint2 outputSize;
    float2 outputTexelSize;
    float dispersionAmount;
    float minWavelengthNm;
    float maxWavelengthNm;
    float referenceWavelengthNm;
    float sampleCount;
    float intensityScale;
};

#ifdef __cplusplus
static_assert(sizeof(BloomPrefilterParams) == 40, "BloomPrefilterParams must match the HLSL root constants");
static_assert(sizeof(BloomDownsampleParams) == 28, "BloomDownsampleParams must match the HLSL root constants");
static_assert(sizeof(BloomUpsampleParams) == 32, "BloomUpsampleParams must match the HLSL root constants");
static_assert(sizeof(ConvolutionBloomKernelPrepareParams) == 36, "ConvolutionBloomKernelPrepareParams must match the HLSL root constants");
static_assert(sizeof(ConvolutionBloomScreenPrepareParams) == 28, "ConvolutionBloomScreenPrepareParams must match the HLSL root constants");
static_assert(sizeof(ConvolutionBloomResolveParams) == 40, "ConvolutionBloomResolveParams must match the HLSL root constants");
static_assert(sizeof(ConvolutionBloomAperturePrepareParams) == 24, "ConvolutionBloomAperturePrepareParams must match the HLSL root constants");
static_assert(sizeof(ConvolutionBloomReduceApertureEnergyParams) == 8, "ConvolutionBloomReduceApertureEnergyParams must match the HLSL root constants");
static_assert(sizeof(ConvolutionBloomResolveDiffractionPSFParams) == 16, "ConvolutionBloomResolveDiffractionPSFParams must match the HLSL root constants");
static_assert(sizeof(ConvolutionBloomApplyDispersionParams) == 40, "ConvolutionBloomApplyDispersionParams must match the HLSL root constants");
#endif

#endif // __BLOOM_INTEROP_H__
