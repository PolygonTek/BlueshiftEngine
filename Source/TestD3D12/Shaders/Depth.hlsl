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
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
    nointerpolation uint shaderMaterialIndex : TEXCOORD1;
};

[RootSignature(COMMON_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;

    ShaderSurface shaderSurface = GetShaderSurfaceByInstanceID(input.instanceID);

    float4 positionWS = float4(mul(shaderSurface.worldMatrix, input.position), 1.0);
    result.position = mul(cameraConstants.viewProjMatrix, positionWS);
    result.texCoord = input.texCoord;
    result.color = input.color;
    result.shaderMaterialIndex = shaderSurface.shaderMaterialIndex;

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
void PSMain(PSInput input) {
#ifdef PERFORATED
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);

    float4 textureColor = GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(linearWrapSampler, input.texCoord);
    const float baseColorAlpha = shaderMaterial.GetBaseColorFactor().a;
    clip(textureColor.a * baseColorAlpha * input.color.a - shaderMaterial.alphaCutoff);
#endif
}
