#define OCEAN_PREPASS 1
#include "OceanSurfaceCommon.hlsli"

[RootSignature(OCEAN_ROOT_SIGNATURE)]
PrepassPSOutput PSMain(PSInput input, bool isFrontFace : SV_IsFrontFace) {
#if OCEAN_USE_SURFACE_INPUT
    const OceanSurfaceInput surface = PrepareOceanSurfaceInput(input, isFrontFace);
#endif

    PrepassPSOutput result;
#if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    result.normalRoughness = float4(0.5f, 0.5f, 1.0f, 0.0f);
#endif

#ifdef OUTPUT_NORMAL
    result.normalRoughness.xy = EncodeUnitVectorOctahedralUNorm(mul((float3x3)cameraConstants.viewRotation, surface.normal));
#endif

#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currUnjitteredClipPos, input.prevUnjitteredClipPos);
#endif

#ifdef OUTPUT_ROUGHNESS
    result.normalRoughness.z = surface.roughness;
#endif
    return result;
}
