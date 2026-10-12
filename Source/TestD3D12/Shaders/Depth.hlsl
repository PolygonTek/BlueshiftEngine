#include "DepthCommon.hlsli"

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

[RootSignature(DEPTH_ROOT_SIGNATURE)]
DepthPSInput VSMain(DepthVSInput input) {
    return BuildDepthVertex(input, cameraConstants);
}

#if defined(OUTPUT_NORMAL) || defined(OUTPUT_VELOCITY)
struct DepthPSOutput {
#ifdef OUTPUT_VELOCITY
    float2 velocity : SV_Target0;
#endif
#ifdef OUTPUT_NORMAL
    #ifdef OUTPUT_VELOCITY
        float2 normal : SV_Target1;
    #else
        float2 normal : SV_Target0;
    #endif
#endif
};
#endif

[RootSignature(DEPTH_ROOT_SIGNATURE)]
#if defined(OUTPUT_NORMAL) || defined(OUTPUT_VELOCITY)
DepthPSOutput PSMain(DepthPSInput input, bool isFrontFace : SV_IsFrontFace) {
#ifdef PERFORATED
    ApplyDepthAlphaCutoff(input);
#endif

    DepthPSOutput result;
#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currentVelocityPosition, input.prevVelocityPosition);
#endif

#ifdef OUTPUT_NORMAL
    // 삼각형의 앞뒷면을 기준으로 smooth normal 방향을 맞춰, 비스듬한 시점에서의 급격한 반전을 방지한다.
    const float3 viewNormal = input.viewNormal * (isFrontFace ? 1.0f : -1.0f);

    // Octahedral 인코딩은 방향만 저장하므로, 정규화는 값을 읽어 디코딩할 때만 수행한다.
    result.normal = dot(viewNormal, viewNormal) > FLT_MIN ? EncodeUnitVectorOctahedral(viewNormal) : float2(0.0f, 0.0f);
#endif
    return result;
}
#else
void PSMain(DepthPSInput input) {
#ifdef PERFORATED
    ApplyDepthAlphaCutoff(input);
#endif
}
#endif
