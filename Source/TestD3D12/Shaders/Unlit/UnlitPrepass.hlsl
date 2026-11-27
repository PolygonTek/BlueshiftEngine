#define UNLIT_PREPASS 1
#include "UnlitSurfaceCommon.hlsli"

[RootSignature(COMMON_ROOT_SIGNATURE)]
PrepassPSOutput PSMain(PSInput input) {
#if defined(DITHER_FADE) && !defined(PERFORATED) && !defined(DITHERED)
    ApplyDitherFade(GetSurfaceFadeOpacity(input.surfaceFlags), (uint2)input.position.xy, GetSurfaceDitherPatternSeed(input.surfaceFlags));
#endif

#if defined(PERFORATED) || defined(DITHERED)
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);
#if _BASECOLOR != 0
    const float2 texCoord = shaderMaterial.TransformTexCoord(input.texCoord);
#else
    const float2 texCoord = 0.0f;
#endif
    EvaluateUnlitBaseColor(input, shaderMaterial, texCoord);
#endif

    PrepassPSOutput result;
#if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    result.normalRoughness = float4(0.5f, 0.5f, 1.0f, 0.0f);
#endif

#ifdef OUTPUT_NORMAL
    result.normalRoughness.xy = dot(input.viewNormal, input.viewNormal) > FLT_MIN ? EncodeUnitVectorOctahedralUNorm(input.viewNormal) : float2(0.5f, 0.5f);
#endif

#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currUnjitteredClipPos, input.prevUnjitteredClipPos);
#endif

#ifdef OUTPUT_ROUGHNESS
    // Unlit surface 는 표준 specular 모델에 참여하지 않는다.
    result.normalRoughness.z = 1.0f;
#endif
    return result;
}
