#ifndef __SURFACE_DRAW_COMMON_HLSLI__
#define __SURFACE_DRAW_COMMON_HLSLI__

#include "Common.hlsli"
#include "CommonBindless.hlsli"

#define ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    "SRV(t0), " \
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

float3 DecodeVertexNormal(float3 normal) {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    return normal * 2.0f - 1.0f;
#else
    return normal;
#endif
}

float4 DecodeVertexTangent(float4 tangent) {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    return float4(tangent.xyz * 2.0f - 1.0f, tangent.w < 0.5f ? -1.0f : 1.0f);
#else
    return float4(tangent.xyz, tangent.w < 0.0f ? -1.0f : 1.0f);
#endif
}

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

float2 EncodeOctahedralNormal(float3 normal) {
    normal /= abs(normal.x) + abs(normal.y) + abs(normal.z);

    if (normal.z < 0.0) {
        const float2 signNotZero = float2(normal.x >= 0.0 ? 1.0 : -1.0, normal.y >= 0.0 ? 1.0 : -1.0);
        normal.xy = (1.0 - abs(normal.yx)) * signNotZero;
    }
    return normal.xy;
}

float2 CalcVelocity(float4 currentClipPosition, float4 prevClipPosition) {
    float2 currentNDC = currentClipPosition.xy / currentClipPosition.w;
    float2 prevNDC = prevClipPosition.xy / prevClipPosition.w;

    float2 currentUV = float2(currentNDC.x * 0.5 + 0.5, 0.5 - currentNDC.y * 0.5);
    float2 prevUV = float2(prevNDC.x * 0.5 + 0.5, 0.5 - prevNDC.y * 0.5);
    return currentUV - prevUV;
}

struct SurfacePSOutput {
    float4 color : SV_Target0;
#ifdef OUTPUT_NORMAL
    float2 normal : SV_Target1;
#endif
#ifdef OUTPUT_VELOCITY
    #ifdef OUTPUT_NORMAL
    float2 velocity : SV_Target2;
    #else
    float2 velocity : SV_Target1;
    #endif
#endif
};

#endif // __SURFACE_DRAW_COMMON_HLSLI__
