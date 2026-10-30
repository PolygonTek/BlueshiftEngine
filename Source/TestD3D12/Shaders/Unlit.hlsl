#include "UnlitSurfaceCommon.hlsli"
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
    return result;
}
