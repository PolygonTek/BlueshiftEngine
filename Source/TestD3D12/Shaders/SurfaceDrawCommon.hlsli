#ifndef __SURFACEDRAWCOMMON_HLSLI__
#define __SURFACEDRAWCOMMON_HLSLI__

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

float4 CalcClipPosition(float4 localPosition, row_major float3x4 worldMatrix, row_major float4x4 viewProjMatrix) {
    float4 positionWS = float4(mul(worldMatrix, localPosition), 1.0);
    return mul(viewProjMatrix, positionWS);
}

float2 CalcVelocity(float4 currentClipPosition, float4 prevClipPosition) {
    float2 currentNDC = currentClipPosition.xy / currentClipPosition.w;
    float2 prevNDC = prevClipPosition.xy / prevClipPosition.w;

    float2 currentUV = float2(currentNDC.x * 0.5 + 0.5, 0.5 - currentNDC.y * 0.5);
    float2 prevUV = float2(prevNDC.x * 0.5 + 0.5, 0.5 - prevNDC.y * 0.5);
    return currentUV - prevUV;
}

#endif
