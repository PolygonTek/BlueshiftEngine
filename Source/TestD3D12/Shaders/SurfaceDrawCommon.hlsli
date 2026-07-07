#ifndef __SURFACEDRAWCOMMON_HLSLI__
#define __SURFACEDRAWCOMMON_HLSLI__

#include "Common.hlsli"

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

#endif
