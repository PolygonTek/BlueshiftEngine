#ifndef __DEPTH_COMMON_HLSLI__
#define __DEPTH_COMMON_HLSLI__

#include "SurfaceDrawCommon.hlsli"

#define DEPTH_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

struct DepthVSInput {
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
    uint instanceID : SV_InstanceID;
#if GPU_SKINNING
    SkinningVertexInput skinning;
#endif
};

struct DepthPSInput {
    float4 position : SV_POSITION;
#ifdef OUTPUT_VELOCITY
    float4 currentVelocityPosition : TEXCOORD2;
    float4 prevVelocityPosition : TEXCOORD3;
#endif
#ifdef PERFORATED
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
    nointerpolation uint shaderMaterialIndex : TEXCOORD1;
#endif
};

#ifdef OUTPUT_VELOCITY
DepthPSInput BuildDepthVertex(DepthVSInput input, uint surfaceTableIndex, row_major float4x4 viewProjMatrix, row_major float4x4 unjitteredViewProjMatrix, row_major float4x4 prevUnjitteredViewProjMatrix) {
#else
DepthPSInput BuildDepthVertex(DepthVSInput input, uint surfaceTableIndex, row_major float4x4 viewProjMatrix) {
#endif
    DepthPSInput result;

    ShaderSurface shaderSurface = GetShaderSurfaceByInstanceID(input.instanceID, surfaceTableIndex);

    float4 localPosition = input.position;
#if GPU_SKINNING
    const float3x4 skinningMatrix = GetSkinningMatrix(input.skinning, shaderSurface.skinningJointOffset);
    localPosition = float4(mul(skinningMatrix, localPosition), 1.0f);
#endif
    float4 positionWS = float4(mul(shaderSurface.worldMatrix, localPosition), 1.0);
    result.position = mul(viewProjMatrix, positionWS);

#ifdef OUTPUT_VELOCITY
    result.currentVelocityPosition = mul(unjitteredViewProjMatrix, positionWS);
    float4 prevLocalPosition = localPosition;
    #if GPU_SKINNING
        // History resets alias the current pose and need no additional joint reads.
        prevLocalPosition = GetPreviousSkinnedPosition(input.skinning, input.position, localPosition, shaderSurface.skinningJointOffset, shaderSurface.prevSkinningJointOffset);
    #endif
    result.prevVelocityPosition = CalcClipPosition(prevLocalPosition, shaderSurface.prevWorldMatrix, prevUnjitteredViewProjMatrix);
#endif

#ifdef PERFORATED
    result.texCoord = input.texCoord;
    result.color = input.color;
    result.shaderMaterialIndex = shaderSurface.shaderMaterialIndex;
#endif
    return result;
}

#ifdef PERFORATED
void ApplyDepthAlphaCutoff(DepthPSInput input) {
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);
    SamplerState materialSampler = GetSampler(shaderMaterial.samplerIndex);

    float4 textureColor = GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(materialSampler, input.texCoord);
    const float baseColorAlpha = shaderMaterial.GetBaseColorFactor().a;
    clip(textureColor.a * baseColorAlpha * input.color.a - shaderMaterial.alphaCutoff);
}
#endif

#endif // __DEPTH_COMMON_HLSLI__
