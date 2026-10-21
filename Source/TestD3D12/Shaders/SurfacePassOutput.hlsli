#ifndef __SURFACE_PASS_OUTPUT_HLSLI__
#define __SURFACE_PASS_OUTPUT_HLSLI__

#include "Octahedral.hlsli"

float2 CalcVelocity(float4 currClipPos, float4 prevClipPos) {
    const float2 currNDC = currClipPos.xy / currClipPos.w;
    const float2 prevNDC = prevClipPos.xy / prevClipPos.w;
    const float2 currUV = float2(currNDC.x * 0.5f + 0.5f, 0.5f - currNDC.y * 0.5f);
    const float2 prevUV = float2(prevNDC.x * 0.5f + 0.5f, 0.5f - prevNDC.y * 0.5f);
    return currUV - prevUV;
}

struct SurfacePassPSOutput {
    float4 color : SV_Target0;
#if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    float4 normalRoughness : SV_Target1; // RG: UNORM octahedral view-space normal, B: perceptual roughness, A: unused.
#endif
#ifdef OUTPUT_VELOCITY
    #if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    float2 velocity : SV_Target2;
    float taaReactiveMask : SV_Target3;
    #else
    float2 velocity : SV_Target1;
    float taaReactiveMask : SV_Target2;
    #endif
#endif
};

struct SurfacePrepassPSOutput {
#if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    float4 normalRoughness : SV_Target0; // RG: UNORM octahedral view-space normal, B: perceptual roughness, A: unused.
#endif
#ifdef OUTPUT_VELOCITY
    #if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    float2 velocity : SV_Target1; // Unjittered velocity for reprojection.
    #else
    float2 velocity : SV_Target0;
    #endif
#endif
};

#endif // __SURFACE_PASS_OUTPUT_HLSLI__
