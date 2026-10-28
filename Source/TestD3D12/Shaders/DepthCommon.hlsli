#ifndef __DEPTH_COMMON_HLSLI__
#define __DEPTH_COMMON_HLSLI__

#include "SurfaceDrawCommon.hlsli"

#define DEPTH_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

struct DepthVSInput {
    float4 position : POSITION;
#ifdef PERFORATED
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
#endif
#ifdef OUTPUT_NORMAL
    #ifdef PACKED_VERTEX_TANGENT_FRAME
        uint normal : NORMAL;
    #else
        float4 normal : NORMAL;
    #endif
#endif
    uint instanceID : SV_InstanceID;
#if GPU_SKINNING
    SkinningVertexInput skinning;
#endif
};

struct DepthPSInput {
    float4 position : SV_POSITION;
#ifdef OUTPUT_NORMAL
    float3 viewNormal : TEXCOORD4;
#endif
#ifdef OUTPUT_VELOCITY
    float4 currUnjitteredClipPos : TEXCOORD2;
    float4 prevUnjitteredClipPos : TEXCOORD3;
#endif
#ifdef PERFORATED
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
    nointerpolation uint shaderMaterialIndex : TEXCOORD1;
#endif
};

template<typename ViewConstants>
DepthPSInput BuildDepthVertex(DepthVSInput input, ViewConstants viewConstants) {
    DepthPSInput result;

    ShaderSurface shaderSurface = GetShaderSurfaceByInstanceID(input.instanceID, viewConstants.surfaceTableIndex);

    float4 localPosition = input.position;
#if GPU_SKINNING
    const float3x4 skinningMatrix = GetSkinningMatrix(input.skinning, shaderSurface.skinningJointOffset);
    localPosition = float4(mul(skinningMatrix, localPosition), 1.0f);
#endif
    float4 positionWS = float4(mul(shaderSurface.worldMatrix, localPosition), 1.0);
    result.position = mul(viewConstants.viewProjMatrix, positionWS);

#ifdef OUTPUT_NORMAL
    #ifdef PACKED_VERTEX_TANGENT_FRAME
        float3 normal = DecodeTangentFrameNormal(input.normal);
    #else
        float3 normal = DecodeVertexNormal(input.normal);
    #endif

    #if GPU_SKINNING
        normal = mul((float3x3)skinningMatrix, normal);
    #endif
    const float3 worldNormal = TransformSurfaceNormal(normal, shaderSurface.worldMatrix, (shaderSurface.flags & ShaderSurfaceFlagNonUniformScale) != 0);
    result.viewNormal = mul((float3x3)viewConstants.viewRotation, worldNormal);
#endif

#ifdef OUTPUT_VELOCITY
    result.currUnjitteredClipPos = mul(viewConstants.unjitteredViewProjMatrix, positionWS);
    float4 prevLocalPosition = localPosition;

    #if GPU_SKINNING
        prevLocalPosition = GetPreviousSkinnedPosition(input.skinning, input.position, localPosition, shaderSurface.skinningJointOffset, shaderSurface.prevSkinningJointOffset);
    #endif
    result.prevUnjitteredClipPos = CalcClipPosition(prevLocalPosition, shaderSurface.prevWorldMatrix, viewConstants.prevUnjitteredViewProjMatrix);
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

    const float2 texCoord = shaderMaterial.TransformTexCoord(input.texCoord);
    float4 textureColor = GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(materialSampler, texCoord);
    const float baseColorAlpha = shaderMaterial.GetBaseColorFactor().a;
    clip(textureColor.a * baseColorAlpha * input.color.a - shaderMaterial.alphaCutoff);
}
#endif

#endif // __DEPTH_COMMON_HLSLI__
