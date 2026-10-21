#include "LitSurfaceCommon.hlsli"
#include "PBR/ShadingParams.hlsli"

[RootSignature(COMMON_ROOT_SIGNATURE)]
SurfacePrepassPSOutput PSMain(PSInput input) {
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);
    SamplerState materialSampler = GetSampler(shaderMaterial.samplerIndex);
    const float2 texCoord = shaderMaterial.TransformTexCoord(input.texCoord);
    const LitSurfaceInput surfaceInput = PrepareLitSurfaceInput(input, shaderMaterial, materialSampler, texCoord, 0.0f, 0.0f, false);

#if defined(PERFORATED) || defined(TRANSPARENT_SSR_PREPASS)
    // Opaque receivers do not need a base-color sample; preserve material alpha coverage otherwise.
    EvaluateLitBaseColor(input, shaderMaterial, materialSampler, surfaceInput.texCoord);
#endif

    const bool useSpecularAA = (cameraConstants.flags & ShaderCameraFlagUseSpecularAA) != 0u;
#if _NORMAL != 0
    const SurfacePrepassParams surface = PrepareSurfacePrepassParams(shaderMaterial, surfaceInput.texCoord, surfaceInput.worldNormal, surfaceInput.tangentToWorld, useSpecularAA, materialSampler);
#else
    const SurfacePrepassParams surface = PrepareSurfacePrepassParams(shaderMaterial, surfaceInput.texCoord, surfaceInput.worldNormal, useSpecularAA, materialSampler);
#endif

    SurfacePrepassPSOutput result;
#if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    result.normalRoughness = float4(0.5f, 0.5f, 1.0f, 0.0f);
#endif

#ifdef OUTPUT_NORMAL
    result.normalRoughness.xy = EncodeUnitVectorOctahedralUNorm(mul((float3x3)cameraConstants.viewRotation, surface.normal));
#endif

#ifdef OUTPUT_ROUGHNESS
    result.normalRoughness.z = surface.roughness;
#endif

#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currUnjitteredClipPos, input.prevUnjitteredClipPos);
#endif

    return result;
}
