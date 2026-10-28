#ifndef __LIT_SURFACE_COMMON_HLSLI__
#define __LIT_SURFACE_COMMON_HLSLI__

#include "SurfaceDrawCommon.hlsli"

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

#include "PBR/ParallaxMapping.hlsli"

#if _PARALLAX != 0 || _NORMAL != 0 || (_CLEARCOAT != 0 && _CLEARCOAT_NORMAL != 0)
    #define USE_SURFACE_TANGENT_FRAME 1
#else
    #define USE_SURFACE_TANGENT_FRAME 0
#endif

#define COMMON_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

struct VSInput {
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
#ifdef PACKED_VERTEX_TANGENT_FRAME
    uint normal : NORMAL;
#else
    float4 normal : NORMAL;
    float4 tangent : TEXCOORD1;
#endif
    uint instanceID : SV_InstanceID;
#if GPU_SKINNING
    SkinningVertexInput skinning;
#endif
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    nointerpolation uint shaderMaterialIndex : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
    float3 worldNormal : TEXCOORD3;
#if USE_SURFACE_TANGENT_FRAME
    float4 worldTangent : TEXCOORD4;
#endif
#ifdef OUTPUT_VELOCITY
    float4 currUnjitteredClipPos : TEXCOORD5;
    float4 prevUnjitteredClipPos : TEXCOORD6;
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
    result.color = input.color;
    result.texCoord = input.texCoord;
    result.shaderMaterialIndex = shaderSurface.shaderMaterialIndex;
    result.worldPosition = positionWS.xyz;
    float3 vertexNormal;
#if USE_SURFACE_TANGENT_FRAME
    float4 vertexTangent;
    #ifdef PACKED_VERTEX_TANGENT_FRAME
        DecodeTangentFrame(input.normal, vertexNormal, vertexTangent);
    #else
        vertexNormal = DecodeVertexNormal(input.normal);
        vertexTangent = DecodeVertexTangent(input.tangent);
    #endif
#else
    #ifdef PACKED_VERTEX_TANGENT_FRAME
        vertexNormal = DecodeTangentFrameNormal(input.normal);
    #else
        vertexNormal = DecodeVertexNormal(input.normal);
    #endif
#endif

#if GPU_SKINNING
    vertexNormal = mul((float3x3)skinningMatrix, vertexNormal);
    #if USE_SURFACE_TANGENT_FRAME
        vertexTangent.xyz = mul((float3x3)skinningMatrix, vertexTangent.xyz);
    #endif
#endif

    result.worldNormal = TransformSurfaceNormal(vertexNormal, shaderSurface.worldMatrix, (shaderSurface.flags & ShaderSurfaceFlagNonUniformScale) != 0);
#if USE_SURFACE_TANGENT_FRAME
    result.worldTangent = TransformSurfaceTangent(vertexTangent, shaderSurface.worldMatrix);
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

// Lit and its surface prepass share vertex transforms, tangent frames and parallax UVs.
struct LitSurfaceInput {
    float3 worldNormal;
    float3 viewDir;
    float2 texCoord;
#if USE_SURFACE_TANGENT_FRAME
    float3x3 tangentToWorld;
#endif
};

LitSurfaceInput PrepareLitSurfaceInput(PSInput input, ShaderMaterial shaderMaterial, SamplerState materialSampler, float2 texCoord, float2 texCoordDx, float2 texCoordDy, bool hasTexCoordGradients) {
    const float3 worldNormal = normalize(input.worldNormal);

#if USE_SURFACE_TANGENT_FRAME
    // worldTangent 를 worldNormal 에 대해 Gram-Schmidt 직교화 + normalize
    float3 worldTangent = input.worldTangent.xyz - worldNormal * dot(worldNormal, input.worldTangent.xyz);
    const float worldTangentLengthSquared = dot(worldTangent, worldTangent);
    worldTangent = worldTangentLengthSquared > FLT_MIN ? worldTangent * rsqrt(worldTangentLengthSquared) : float3(1.0f, 0.0f, 0.0f);

    const float3 worldBitangent = cross(worldNormal, worldTangent) * input.worldTangent.w;
    const float3x3 tangentToWorld = float3x3(worldTangent, worldBitangent, worldNormal);
#endif

    const float3 viewDir = normalize(cameraConstants.cameraOrigin - input.worldPosition);
    float2 baseTc = texCoord;

#if _PARALLAX != 0
    const float3 viewTS = mul(tangentToWorld, viewDir);
    if (!hasTexCoordGradients) {
        texCoordDx = ddx_coarse(texCoord);
        texCoordDy = ddy_coarse(texCoord);
    }

    baseTc = ParallaxMapping(GetTexture2D(shaderMaterial.heightTextureIndex), materialSampler, baseTc, shaderMaterial.heightScale, viewTS, texCoordDx, texCoordDy);
#endif

    LitSurfaceInput result;
    result.worldNormal = worldNormal;
    result.viewDir = viewDir;
    result.texCoord = baseTc;
#if USE_SURFACE_TANGENT_FRAME
    result.tangentToWorld = tangentToWorld;
#endif
    return result;
}

float4 EvaluateLitBaseColor(PSInput input, ShaderMaterial shaderMaterial, SamplerState materialSampler, float2 baseTc) {
    float4 baseColor = shaderMaterial.GetBaseColorFactor() * input.color;
    #if _BASECOLOR != 0
        baseColor *= GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(materialSampler, baseTc);
    #endif

    #ifdef PERFORATED
        clip(baseColor.a - shaderMaterial.alphaCutoff);
    #endif
    #ifdef TRANSPARENT_SSR_PREPASS
        // 완전히 투명한 texel 은 가장 가까운 transparent SSR receiver 로 기록하지 않는다.
        clip(baseColor.a - (1.0f / 255.0f));
    #endif
    return baseColor;
}

#endif // __LIT_SURFACE_COMMON_HLSLI__
