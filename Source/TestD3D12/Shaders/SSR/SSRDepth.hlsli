#ifndef SSR_DEPTH_HLSLI
#define SSR_DEPTH_HLSLI

float SelectSSRDepthSlope(float backwardDelta, float forwardDelta, bool backwardValid, bool forwardValid) {
    if (!backwardValid) {
        return forwardValid ? forwardDelta : 0.0f;
    }

    if (!forwardValid) {
        return backwardDelta;
    }

    if (backwardDelta * forwardDelta <= 0.0f) {
        return 0.0f;
    }

    // silhouette 의 큰 depth jump 대신 현재 surface 쪽의 작은 변화를 사용한다.
    return abs(forwardDelta) < abs(backwardDelta) ? forwardDelta : backwardDelta;
}

float2 ComputeSSRDepthGradient(Texture2D<float> depthTexture, uint2 size, uint2 pixelCoord, float depth, float clearDepth) {
    const int2 coord = int2(pixelCoord);
    const int2 maxCoord = int2(size) - 1;
    const float depthL = depthTexture.Load(int3(max(coord - int2(1, 0), int2(0, 0)), 0));
    const float depthR = depthTexture.Load(int3(min(coord + int2(1, 0), maxCoord), 0));
    const float depthU = depthTexture.Load(int3(max(coord - int2(0, 1), int2(0, 0)), 0));
    const float depthD = depthTexture.Load(int3(min(coord + int2(0, 1), maxCoord), 0));

    // 화면 밖으로 clamp 된 자기 자신과 배경은 기울기 추정에서 제외한다.
    const bool validL = coord.x > 0 && depthL != clearDepth;
    const bool validR = coord.x < maxCoord.x && depthR != clearDepth;
    const bool validU = coord.y > 0 && depthU != clearDepth;
    const bool validD = coord.y < maxCoord.y && depthD != clearDepth;

    return float2(
        SelectSSRDepthSlope(depth - depthL, depthR - depth, validL, validR),
        SelectSSRDepthSlope(depth - depthU, depthD - depth, validU, validD));
}

#endif
