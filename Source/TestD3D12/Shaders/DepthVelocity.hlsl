#include "SurfaceDrawCommon.hlsli"

#define COMMON_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

struct VSInput {
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
    uint instanceID : SV_InstanceID;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 currentPosition : TEXCOORD0;
    float4 prevPosition : TEXCOORD1;
    float2 texCoord : TEXCOORD2;
    float4 color : COLOR;
    nointerpolation uint shaderMaterialIndex : TEXCOORD3;
};

[RootSignature(COMMON_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;

    ShaderSurface shaderSurface = GetShaderSurfaceByInstanceID(input.instanceID);

    float4 currentPosition = CalcClipPosition(shaderSurface.worldMatrix, input.position, cameraConstants.viewProjMatrix);
    float4 prevPosition = CalcClipPosition(shaderSurface.prevWorldMatrix, input.position, cameraConstants.prevViewProjMatrix);

    result.position = currentPosition;
    result.currentPosition = currentPosition;
    result.prevPosition = prevPosition;
    result.texCoord = input.texCoord;
    result.color = input.color;
    result.shaderMaterialIndex = shaderSurface.shaderMaterialIndex;

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
float2 PSMain(PSInput input) : SV_Target0 {
#ifdef PERFORATED
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);

    float4 textureColor = GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(linearWrapSampler, input.texCoord);
    clip(textureColor.a * shaderMaterial.baseColorFactor.a * input.color.a - shaderMaterial.alphaCutoff);
#endif

    return CalcVelocity(input.currentPosition, input.prevPosition);
}
