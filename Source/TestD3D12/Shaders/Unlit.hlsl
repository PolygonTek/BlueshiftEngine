#include "Common.hlsli"

#define COMMON_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    "SRV(t0), " \
    "CBV(b0), " \
    "CBV(b1), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

StructuredBuffer<uint> instanceToShaderSurfaceTable : register(t0);

struct VSInput {
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
    uint instanceID : SV_InstanceID;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    nointerpolation uint shaderMaterialIndex : TEXCOORD1;
};

cbuffer FrameCB : register(b0) {
    FrameConstants frameConstants;
};

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

[RootSignature(COMMON_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;

    const uint shaderSurfaceIndex = instanceToShaderSurfaceTable[input.instanceID];
    ShaderSurface shaderSurface = GetShaderSurface(cameraConstants.surfaceTableIndex)[shaderSurfaceIndex];

    float4 positionWS = float4(mul(shaderSurface.worldMatrix, input.position), 1.0);
    result.position = mul(cameraConstants.viewProjMatrix, positionWS);
    result.color = input.color;
    result.texCoord = input.texCoord;
    result.shaderMaterialIndex = shaderSurface.shaderMaterialIndex;

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
float4 PSMain(PSInput input) : SV_TARGET {
    ShaderMaterial shaderMaterial = GetShaderMaterial(frameConstants.shaderMaterialTableIndex)[input.shaderMaterialIndex];

    if (frameConstants.feedbackBufferIndex != 0 && shaderMaterial.streamingMaterialIndex != TextureStreamingInvalidIndex) {
        WriteMipmapFeedback(GetRWBufferUInt(frameConstants.feedbackBufferIndex), shaderMaterial.streamingMaterialIndex, ddx_coarse(input.texCoord.xy), ddy_coarse(input.texCoord.xy));
    }

    float4 textureColor = GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(anisoClampSampler, input.texCoord);
    return textureColor * shaderMaterial.baseColorFactor * input.color;
}
