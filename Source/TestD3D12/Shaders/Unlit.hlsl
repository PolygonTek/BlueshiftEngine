#include "Common.hlsli"

#define COMMON_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    "CBV(b0), " \
    "CBV(b1), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

struct VSInput {
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
#if USE_INSTANCING == 1
    uint instanceID : SV_InstanceID;
#endif
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

cbuffer FrameCB : register(b0) {
    FrameConstants frameConstants;
};

#if USE_INSTANCING == 1
cbuffer ObjectCB : register(b1) {
    row_major float4x4 viewProjMatrix;
    row_major float3x4 worldMatrix[MaxDrawInstanceCount];
    uint shaderMaterialIndex;
};
#else
cbuffer ObjectCB : register(b1) {
    row_major float4x4 modelViewProjMatrix;
    uint shaderMaterialIndex;
};
#endif

[RootSignature(COMMON_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;
#if USE_INSTANCING == 1
    float4 positionWS = float4(mul(worldMatrix[input.instanceID], input.position), 1.0);
    result.position = mul(viewProjMatrix, positionWS);
#else
    result.position = mul(modelViewProjMatrix, input.position);
#endif
    result.color = input.color;
    result.texCoord = input.texCoord;

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
float4 PSMain(PSInput input) : SV_TARGET {
    ShaderMaterial shaderMaterial = GetShaderMaterial(frameConstants.shaderMaterialTableIndex)[shaderMaterialIndex];

    if (frameConstants.feedbackBufferIndex != 0 && shaderMaterial.streamingMaterialIndex != TextureStreamingInvalidIndex) {
        WriteMipmapFeedback(GetRWBufferUInt(frameConstants.feedbackBufferIndex), shaderMaterial.streamingMaterialIndex, ddx_coarse(input.texCoord.xy), ddy_coarse(input.texCoord.xy));
    }

    float4 textureColor = GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(anisoClampSampler, input.texCoord);
    return textureColor * shaderMaterial.baseColorFactor * input.color;
}
