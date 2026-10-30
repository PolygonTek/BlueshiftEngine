#define LIT_PREPASS 1
#include "LitSurfaceCommon.hlsli"
#include "PBR/ShadingParams.hlsli"

[RootSignature(COMMON_ROOT_SIGNATURE)]
PrepassPSOutput PSMain(PSInput input) {
#if defined(DITHER_FADE) && !defined(PERFORATED) && !defined(DITHERED)
    ApplyDitherFade(GetSurfaceFadeOpacity(input.surfaceFlags), (uint2)input.position.xy, GetSurfaceDitherPatternSeed(input.surfaceFlags));
#endif

#if LIT_USE_MATERIAL
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);
    SamplerState materialSampler = GetSampler(shaderMaterial.samplerIndex);
#if LIT_USE_TEXCOORD
    const float2 texCoord = shaderMaterial.TransformTexCoord(input.texCoord);
#else
    const float2 texCoord = 0.0f;
#endif
    const LitSurfaceInput surfaceInput = PrepareLitSurfaceInput(input, shaderMaterial, materialSampler, texCoord, 0.0f, 0.0f, false);

#if defined(PERFORATED) || defined(DITHERED) || defined(TRANSPARENT_SSR_PREPASS)
    // Opaque receivers do not need a base-color sample; preserve material alpha coverage otherwise.
    EvaluateLitBaseColor(input, shaderMaterial, materialSampler, surfaceInput.texCoord);
#endif

#endif // LIT_USE_MATERIAL

#if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
#if LIT_USE_MATERIAL
    const bool useSpecularAA = (cameraConstants.flags & ShaderCameraFlagUseSpecularAA) != 0u;
#if _NORMAL != 0
    const PrepassParams surface = PreparePrepassParams(shaderMaterial, surfaceInput.texCoord, surfaceInput.worldNormal, surfaceInput.tangentToWorld, useSpecularAA, materialSampler);
#else
    const PrepassParams surface = PreparePrepassParams(shaderMaterial, surfaceInput.texCoord, surfaceInput.worldNormal, useSpecularAA, materialSampler);
#endif
#else
    PrepassParams surface;
    surface.normal = normalize(input.worldNormal);
    surface.roughness = 1.0f;
#endif // LIT_USE_MATERIAL
#endif // OUTPUT_NORMAL || OUTPUT_ROUGHNESS

    PrepassPSOutput result;
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
