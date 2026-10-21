#ifndef __SSR_HIZ_COMMON_HLSLI__
#define __SSR_HIZ_COMMON_HLSLI__

static const float2 EmptyDepthExtrema = float2(0.0f, 1.0f);

float2 MergeDepthExtrema(float2 a, float2 b) {
    return float2(max(a.x, b.x), min(a.y, b.y));
}

float2 EncodeDepthExtrema(float depth, float clearDepth) {
    // Depth clear 값 (clearDepth) 은 비어있는 영역으로 취급한다.
    return depth == clearDepth ? EmptyDepthExtrema : float2(depth, depth);
}

#endif // __SSR_HIZ_COMMON_HLSLI__
