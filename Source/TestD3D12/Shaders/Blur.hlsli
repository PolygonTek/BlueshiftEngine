#ifndef __BLUR_HLSLI__
#define __BLUR_HLSLI__

#include "Common.hlsli"

// 중심을 제외한 Gaussian blur 의 이웃 tap 개수.
static const uint gaussianNeighborCount = 8;

// sigma = 2픽셀인 Gaussian 공간 가중치. 중심 가중치는 1이다.
static const float gaussianWeights[gaussianNeighborCount] = {
    0.135335283f, 0.324652467f, 0.606530660f, 0.882496903f,
    0.882496903f, 0.606530660f, 0.324652467f, 0.135335283f
};
static const int gaussianOffsets[gaussianNeighborCount] = { -4, -3, -2, -1, 1, 2, 3, 4 };

// 한 축 방향으로 스칼라 값을 처리하는 9-tap Gaussian blur.
float GaussianBlur(Texture2D<float> inputTexture, uint2 pixelCoord, int2 axis, uint2 inputSize) {
    float valueSum = inputTexture.Load(int3(pixelCoord, 0));
    float weightSum = 1.0f;

    [unroll]
    for (uint i = 0; i < gaussianNeighborCount; ++i) {
        const int2 sampleCoord = int2(pixelCoord) + gaussianOffsets[i] * axis;

        if (all(sampleCoord >= 0) && all(sampleCoord < int2(inputSize))) {
            const float weight = gaussianWeights[i];
            valueSum += weight * inputTexture.Load(int3(sampleCoord, 0));
            weightSum += weight;
        }
    }
    return valueSum / weightSum;
}

// 호출부의 constant buffer 레이아웃과 독립적인 셰이더 함수 인자.
struct BlurBilateralParams {
    uint2 inputSize;
    float depthSigmaRcp;
    float farPlaneDeviceDepth;
    float2 ndcDepthScaleBias;
    float2 projectionZ;
    bool orthogonal;
};

// 한 축 방향으로 스칼라 값을 처리하는 9-tap Gaussian bilateral blur.
// Bilateral 의 입력값과 depth 는 같은 texel 격자를 사용한다.
float BilateralBlur(Texture2D<float> inputTexture, Texture2D<float> inputDepthTexture, uint2 pixelCoord, int2 axis, BlurBilateralParams params) {
    const float centerDepth = inputDepthTexture.Load(int3(pixelCoord, 0));
    if (centerDepth == params.farPlaneDeviceDepth) {
        return inputTexture.Load(int3(pixelCoord, 0));
    }

    const float centerLinearDepth = ToLinearDepth(centerDepth, params.ndcDepthScaleBias, params.projectionZ, params.orthogonal);
    bool valid[gaussianNeighborCount];
    float sampleLinearDepths[gaussianNeighborCount];

    [unroll]
    for (uint i = 0; i < gaussianNeighborCount; ++i) {
        const int2 sampleCoord = int2(pixelCoord) + gaussianOffsets[i] * axis;

        const bool inBounds = all(sampleCoord >= 0) && all(sampleCoord < int2(params.inputSize));
        const float sampleDepth = inBounds ? inputDepthTexture.Load(int3(sampleCoord, 0)) : params.farPlaneDeviceDepth;
        valid[i] = inBounds && sampleDepth != params.farPlaneDeviceDepth;

        // infinite far plane 의 clear 값을 포함한 배경 depth 는 선형화하지 않는다.
        sampleLinearDepths[i] = valid[i] ? ToLinearDepth(sampleDepth, params.ndcDepthScaleBias, params.projectionZ, params.orthogonal) : 0.0f;
    }

    // depth 가 한 방향으로 증가하거나 감소하는 표면에서는 국소 depth 기울기를 제거한다.
    // 양쪽 기울기의 부호가 다르면 고립된 얇은 표면일 수 있으므로 기울기를 외삽하지 않는다.
    float depthSlope = 0.0f;
    if (valid[3] && valid[4]) {
        const float leftSlope = centerLinearDepth - sampleLinearDepths[3];
        const float rightSlope = sampleLinearDepths[4] - centerLinearDepth;

        if ((leftSlope > 0.0f && rightSlope > 0.0f) || (leftSlope < 0.0f && rightSlope < 0.0f)) {
            depthSlope = abs(leftSlope) < abs(rightSlope) ? leftSlope : rightSlope;
        }
    }

    float depthWeights[gaussianNeighborCount];
    [unroll]
    for (uint i = 0; i < gaussianNeighborCount; ++i) {
        const float depthError = (sampleLinearDepths[i] - centerLinearDepth - float(gaussianOffsets[i]) * depthSlope) * params.depthSigmaRcp;
        depthWeights[i] = valid[i] ? exp2(-0.721347520f * depthError * depthError) : 0.0f;
    }

    // 먼 tap 이 제외된 표면을 건너뛰지 않도록 가까운 tap 부터 바깥쪽으로 depth 가중치를 누적해 중간의 모든 tap 을 반영한다.
    depthWeights[2] *= depthWeights[3];
    depthWeights[1] *= depthWeights[2];
    depthWeights[0] *= depthWeights[1];
    depthWeights[5] *= depthWeights[4];
    depthWeights[6] *= depthWeights[5];
    depthWeights[7] *= depthWeights[6];

    float valueSum = inputTexture.Load(int3(pixelCoord, 0));
    float weightSum = 1.0f;

    [unroll]
    for (uint i = 0; i < gaussianNeighborCount; ++i) {
        if (valid[i] && depthWeights[i] > 0.0f) {
            const int2 sampleCoord = int2(pixelCoord) + gaussianOffsets[i] * axis;

            const float weight = gaussianWeights[i] * depthWeights[i];
            valueSum += weight * inputTexture.Load(int3(sampleCoord, 0));
            weightSum += weight;
        }
    }
    return valueSum / weightSum;
}

#endif // __BLUR_HLSLI__
