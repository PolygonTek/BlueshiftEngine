#include "UnlitSurfaceCommon.hlsli"
#include "Fog/FogSurface.hlsli"
#if _BASECOLOR != 0
#include "TextureStreamingFeedback.hlsli"
#endif

[RootSignature(COMMON_ROOT_SIGNATURE)]
MainPSOutput PSMain(PSInput input) {
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);
#if _BASECOLOR != 0
    const float2 texCoord = shaderMaterial.TransformTexCoord(input.texCoord);

    if (frameConstants.feedbackBufferIndex != 0 && shaderMaterial.streamingMaterialIndex != TextureStreamingInvalidIndex) {
        WriteMipmapFeedback(GetRWBufferUInt(frameConstants.feedbackBufferIndex), shaderMaterial.streamingMaterialIndex, ddx_coarse(texCoord), ddy_coarse(texCoord));
    }
#else
    const float2 texCoord = 0.0f;
#endif

    const float4 baseColor = EvaluateUnlitBaseColor(input, shaderMaterial, texCoord);

    MainPSOutput result;
    result.color = baseColor;

    // transparent surface 에는 표면 깊이까지의 froxel 및 analytic fog 를 result.color 에 직접 합성한다.
    const uint vfogBlendMode = shaderMaterial.flags & ShaderMaterialVFogBlendMask;
    [branch]
    if ((cameraConstants.flags & (ShaderCameraFlagUseHeightFog | ShaderCameraFlagUseVFog)) != 0u && vfogBlendMode != 0u) {
        result.color = FogApplyToTransparentSurface(result.color, input.position.xyz, vfogBlendMode);
    }
    return result;
}
