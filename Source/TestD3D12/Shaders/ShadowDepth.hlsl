#include "SurfaceDrawCommon.hlsli"

cbuffer ShadowViewCB : register(b1) {
    ShadowViewConstants shadowViewConstants;
};

#define SHADOW_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

struct VSInput {
    float4 position : POSITION;
#ifdef PERFORATED
#if _BASECOLOR != 0
    float2 texCoord : TEXCOORD0;
#endif
    float4 color : COLOR;
#endif
    uint instanceID : SV_InstanceID;
#if GPU_SKINNING
    SkinningVertexInput skinning;
#endif
};

struct PSInput {
    float4 position : SV_POSITION;
#ifdef PERFORATED
#if _BASECOLOR != 0
    float2 texCoord : TEXCOORD0;
#endif
    float4 color : COLOR;
    nointerpolation uint shaderMaterialIndex : TEXCOORD1;
#endif
};

[RootSignature(SHADOW_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;

    ShaderSurface shaderSurface = GetShaderSurfaceByInstanceID(input.instanceID, shadowViewConstants.surfaceTableIndex);

    float4 localPosition = input.position;
#if GPU_SKINNING
    const float3x4 skinningMatrix = GetSkinningMatrix(input.skinning, shaderSurface.skinningJointOffset);
    localPosition = float4(mul(skinningMatrix, localPosition), 1.0f);
#endif
    float4 positionWS = float4(mul(shaderSurface.worldMatrix, localPosition), 1.0);
    result.position = mul(shadowViewConstants.viewProjMatrix, positionWS);

#ifdef PERFORATED
#if _BASECOLOR != 0
    result.texCoord = input.texCoord;
#endif
    result.color = input.color;
    result.shaderMaterialIndex = shaderSurface.shaderMaterialIndex;
#endif
    return result;
}

[RootSignature(SHADOW_ROOT_SIGNATURE)]
void PSMain(PSInput input) {
#ifdef PERFORATED
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);
    float baseColorAlpha = shaderMaterial.GetBaseColorFactor().a;
#if _BASECOLOR != 0
    SamplerState materialSampler = GetSampler(shaderMaterial.samplerIndex);

    const float2 texCoord = shaderMaterial.TransformTexCoord(input.texCoord);
    float4 textureColor = GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(materialSampler, texCoord);
    baseColorAlpha = textureColor.a * baseColorAlpha;
#endif
    clip(baseColorAlpha * input.color.a - shaderMaterial.alphaCutoff);
#endif
}
