#ifndef __UNLIT_SURFACE_COMMON_HLSLI__
#define __UNLIT_SURFACE_COMMON_HLSLI__

#include "SurfaceDrawCommon.hlsli"

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

#define COMMON_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

struct VSInput {
    float4 position : POSITION;
#if !defined(UNLIT_PREPASS) || defined(PERFORATED)
#if _BASECOLOR != 0
    float2 texCoord : TEXCOORD0;
#endif
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

struct PSInput {
    float4 position : SV_POSITION;
#if !defined(UNLIT_PREPASS) || defined(PERFORATED)
    float4 color : COLOR;
#if _BASECOLOR != 0
    float2 texCoord : TEXCOORD0;
#endif
    nointerpolation uint shaderMaterialIndex : TEXCOORD1;
#endif
#ifdef OUTPUT_NORMAL
    float3 viewNormal : TEXCOORD4;
#endif
#ifdef OUTPUT_VELOCITY
    float4 currUnjitteredClipPos : TEXCOORD2;
    float4 prevUnjitteredClipPos : TEXCOORD3;
#endif
};

[RootSignature(COMMON_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;

    ShaderSurface shaderSurface = GetShaderSurfaceByInstanceID(input.instanceID, cameraConstants.surfaceTableIndex);

    float4 localPosition = input.position;
#if GPU_SKINNING
    const float3x4 skinningMatrix = GetSkinningMatrix(input.skinning, shaderSurface.skinningJointOffset);
    localPosition = float4(mul(skinningMatrix, localPosition), 1.0f);
#endif
    float4 positionWS = float4(mul(shaderSurface.worldMatrix, localPosition), 1.0);
    result.position = mul(cameraConstants.viewProjMatrix, positionWS);
#if !defined(UNLIT_PREPASS) || defined(PERFORATED)
    result.color = input.color;
#if _BASECOLOR != 0
    result.texCoord = input.texCoord;
#endif
    result.shaderMaterialIndex = shaderSurface.shaderMaterialIndex;
#endif

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
    result.viewNormal = mul((float3x3)cameraConstants.viewRotation, worldNormal);
#endif

#ifdef OUTPUT_VELOCITY
    result.currUnjitteredClipPos = mul(cameraConstants.unjitteredViewProjMatrix, positionWS);
    float4 prevLocalPosition = localPosition;
    #if GPU_SKINNING
        prevLocalPosition = GetPreviousSkinnedPosition(input.skinning, input.position, localPosition, shaderSurface.skinningJointOffset, shaderSurface.prevSkinningJointOffset);
    #endif
    result.prevUnjitteredClipPos = CalcClipPosition(prevLocalPosition, shaderSurface.prevWorldMatrix, cameraConstants.prevUnjitteredViewProjMatrix);
#endif

    return result;
}

#if !defined(UNLIT_PREPASS) || defined(PERFORATED)
float4 EvaluateUnlitBaseColor(PSInput input, ShaderMaterial shaderMaterial, float2 texCoord) {
    float4 baseColor = shaderMaterial.GetBaseColorFactor();
#if _BASECOLOR != 0
    SamplerState materialSampler = GetSampler(shaderMaterial.samplerIndex);
    const float4 textureColor = GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(materialSampler, texCoord);
    baseColor = textureColor * baseColor;
#endif
    baseColor *= input.color;
#ifdef PERFORATED
    // Depth 전용 경로에서 수행하던 alpha coverage 를 재질 Prepass 와 Main 에서 공유한다.
    clip(baseColor.a - shaderMaterial.alphaCutoff);
#endif
    return baseColor;
}
#endif

#endif // __UNLIT_SURFACE_COMMON_HLSLI__
