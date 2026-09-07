#ifndef __SURFACE_DRAW_COMMON_HLSLI__
#define __SURFACE_DRAW_COMMON_HLSLI__

#include "Common.hlsli"
#include "CommonBindless.hlsli"
#include "CameraPassOutputCommon.hlsli"
#include "PackedTangentFrame.hlsli"
#include "Skinning.hlsli"

#define ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    "SRV(t0), " \
    ROOT_SIGNATURE_SKINNING_RESOURCES \
    "CBV(b0), " \
    "CBV(b1), " \
    "RootConstants(num32BitConstants=1, b2), "

DECLARE_COMMON_STATIC_SAMPLERS

StructuredBuffer<uint> instanceToShaderSurfaceTable : register(t0);

cbuffer FrameCB : register(b0) {
    FrameConstants frameConstants;
};

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

cbuffer SurfaceDrawCB : register(b2) {
    SurfaceDrawParams surfaceDrawParams;
};

ShaderSurface GetShaderSurface(uint shaderSurfaceIndex) {
    return GetShaderSurfaceTable(cameraConstants.surfaceTableIndex)[shaderSurfaceIndex];
}

ShaderSurface GetShaderSurfaceByInstanceID(uint instanceID) {
    return GetShaderSurface(instanceToShaderSurfaceTable[surfaceDrawParams.instanceBaseOffset + instanceID]);
}

ShaderMaterial GetShaderMaterial(uint shaderMaterialIndex) {
    return GetShaderMaterialTable(frameConstants.shaderMaterialTableIndex)[shaderMaterialIndex];
}

#ifdef PACKED_VERTEX_TANGENT_FRAME
float3 DecodeTangentFrameNormal(uint tangentFrame) {
    return UnpackTangentFrameNormal(tangentFrame);
}
void DecodeTangentFrame(uint tangentFrame, out float3 normal, out float4 tangent) {
    float3 tangentDirection;
    float bitangentSign;
    UnpackTangentFrame(tangentFrame, normal, tangentDirection, bitangentSign);
    tangent = float4(tangentDirection, bitangentSign);
}
#else
float3 DecodeVertexNormal(float4 normal) {
    return normal.xyz;
}
float4 DecodeVertexTangent(float4 tangent) {
    return float4(tangent.xyz, tangent.w < 0.0f ? -1.0f : 1.0f);
}
#endif

float4 CalcClipPosition(float4 localPosition, row_major float3x4 worldMatrix, row_major float4x4 viewProjMatrix) {
    float4 positionWS = float4(mul(worldMatrix, localPosition), 1.0);
    return mul(viewProjMatrix, positionWS);
}

float3 TransformSurfaceNormal(float3 localNormal, row_major float3x4 worldMatrix, bool nonUniformScale) {
    float3 worldNormal;
    if (!nonUniformScale) {
        worldNormal = mul((float3x3)worldMatrix, localNormal);
    } else {
        // inverse-transpose 와 같은 방향으로 변환하되, 정규화에 의해 제거되는 determinant 크기 계산은 생략한다.
        const float3 row0 = worldMatrix[0].xyz;
        const float3 row1 = worldMatrix[1].xyz;
        const float3 row2 = worldMatrix[2].xyz;

        const float3 cofactor0 = cross(row1, row2);
        const float3 cofactor1 = cross(row2, row0);
        const float3 cofactor2 = cross(row0, row1);
        const float determinant = dot(row0, cofactor0);

        worldNormal = float3(dot(cofactor0, localNormal), dot(cofactor1, localNormal), dot(cofactor2, localNormal));
        worldNormal *= determinant < 0.0 ? -1.0 : 1.0;
    }

    const float lengthSquared = dot(worldNormal, worldNormal);
    return lengthSquared > FLT_MIN ? worldNormal * rsqrt(lengthSquared) : float3(0.0, 0.0, 1.0);
}

float4 TransformSurfaceTangent(float4 localTangent, row_major float3x4 worldMatrix) {
    float3 worldTangent = mul((float3x3)worldMatrix, localTangent.xyz);
    const float lengthSquared = dot(worldTangent, worldTangent);
    worldTangent = lengthSquared > FLT_MIN ? worldTangent * rsqrt(lengthSquared) : float3(1.0f, 0.0f, 0.0f);

    const float3 row0 = worldMatrix[0].xyz;
    const float3 row1 = worldMatrix[1].xyz;
    const float3 row2 = worldMatrix[2].xyz;
    const float determinant = dot(row0, cross(row1, row2));
    const float handedness = determinant < 0.0f ? -localTangent.w : localTangent.w;

    return float4(worldTangent, handedness);
}

#endif // __SURFACE_DRAW_COMMON_HLSLI__
