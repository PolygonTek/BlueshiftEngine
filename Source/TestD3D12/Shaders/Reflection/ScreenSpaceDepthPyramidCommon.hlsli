#ifndef __SCREEN_SPACE_DEPTH_PYRAMID_COMMON_HLSLI__
#define __SCREEN_SPACE_DEPTH_PYRAMID_COMMON_HLSLI__

// 유효 depth [0, 1] 의 max/min reduction 에 영향을 주지 않는 중립값이다.
// min > max 인 뒤집힌 깊이 범위이므로 clear 표식으로 사용한다.
static const float2 EmptyDepthExtrema = float2(0.0f, 1.0f);

float2 MergeDepthExtrema(float2 a, float2 b) {
    return float2(max(a.x, b.x), min(a.y, b.y));
}

float2 EncodeDepthExtrema(float depth, float farPlaneDeviceDepth) {
    // Depth clear 값은 geometry 가 없는 sample 이므로 pyramid 범위에서 제외한다.
    return depth == farPlaneDeviceDepth ? EmptyDepthExtrema : float2(depth, depth);
}

#endif // __SCREEN_SPACE_DEPTH_PYRAMID_COMMON_HLSLI__
