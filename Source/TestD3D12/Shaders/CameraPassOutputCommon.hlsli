#ifndef __CAMERA_PASS_OUTPUT_COMMON_HLSLI__
#define __CAMERA_PASS_OUTPUT_COMMON_HLSLI__

#include "Octahedral.hlsli"

float2 CalcVelocity(float4 currentClipPosition, float4 prevClipPosition) {
    const float2 currentNDC = currentClipPosition.xy / currentClipPosition.w;
    const float2 prevNDC = prevClipPosition.xy / prevClipPosition.w;
    const float2 currentUV = float2(currentNDC.x * 0.5f + 0.5f, 0.5f - currentNDC.y * 0.5f);
    const float2 prevUV = float2(prevNDC.x * 0.5f + 0.5f, 0.5f - prevNDC.y * 0.5f);
    return currentUV - prevUV;
}

struct CameraPassPSOutput {
    float4 color : SV_Target0;
#ifdef OUTPUT_NORMAL
    float2 normal : SV_Target1; // Octahedral view-space normal, shared with the AO prepass.
#endif
#ifdef OUTPUT_VELOCITY
    #ifdef OUTPUT_NORMAL
    float2 velocity : SV_Target2;
    #else
    float2 velocity : SV_Target1;
    #endif
#endif
};

#endif // __CAMERA_PASS_OUTPUT_COMMON_HLSLI__
