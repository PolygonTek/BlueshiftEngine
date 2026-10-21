#include "DepthCommon.hlsli"

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

[RootSignature(DEPTH_ROOT_SIGNATURE)]
DepthPSInput VSMain(DepthVSInput input) {
    return BuildDepthVertex(input, cameraConstants);
}

#if defined(OUTPUT_NORMAL) || defined(OUTPUT_VELOCITY)
struct DepthPrepassPSOutput {
#ifdef OUTPUT_NORMAL
    float4 normalRoughness : SV_Target0;
#endif
#ifdef OUTPUT_VELOCITY
    #ifdef OUTPUT_NORMAL
        float2 velocity : SV_Target1;
    #else
        float2 velocity : SV_Target0;
    #endif
#endif
};
#endif

[RootSignature(DEPTH_ROOT_SIGNATURE)]
#if defined(OUTPUT_NORMAL) || defined(OUTPUT_VELOCITY)
DepthPrepassPSOutput PSMain(DepthPSInput input, bool isFrontFace : SV_IsFrontFace) {
#ifdef PERFORATED
    ApplyDepthAlphaCutoff(input);
#endif

    DepthPrepassPSOutput result;
#ifdef OUTPUT_NORMAL
    // input.viewNormal 은 smooth vertex normal 이다.
    // 양면 렌더링 중 뒷면이 렌더링될 때 smooth vertex normal 을 반전한다.
    const float3 viewNormal = input.viewNormal * (isFrontFace ? 1.0f : -1.0f);
    const float2 encodedNormal = dot(viewNormal, viewNormal) > FLT_MIN ? EncodeUnitVectorOctahedralUNorm(viewNormal) : float2(0.5f, 0.5f);
    // AO 전용 prepass 는 재질을 평가하지 않으므로 roughness 는 1 로 기록한다.
    result.normalRoughness = float4(encodedNormal, 1.0f, 0.0f);
#endif

#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currUnjitteredClipPos, input.prevUnjitteredClipPos);
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
