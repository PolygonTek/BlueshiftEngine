#ifndef __OCEAN_SURFACE_COMMON_HLSLI__
#define __OCEAN_SURFACE_COMMON_HLSLI__

#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "OceanFieldCommon.hlsli"
#include "../GeometryClipmap.hlsli"
#include "../CommonBindless.hlsli"
#include "../SurfaceOutput.hlsli"
#include "../PBR/BRDFLibrary.hlsli"
#include "../PBR/SpecularAA.hlsli"

#define OCEAN_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    "CBV(b0), " \
    "CBV(b1), " \
    "CBV(b2), " \
    "RootConstants(num32BitConstants=1, b3), " \
    "DescriptorTable(SRV(t0, numDescriptors = 6)), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

cbuffer FrameCB : register(b0) {
    FrameConstants frameConstants;
};

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

cbuffer OceanSurfaceCB : register(b2) {
    OceanSurfaceParams oceanSurfaceParams;
};

cbuffer OceanSurfaceDrawCB : register(b3) {
    uint oceanSurfacePatchInstanceOffset;
};

StructuredBuffer<GeometryClipmapPatch> surfacePatchInstances : register(t0);
Texture2DArray<float4> displacementAndDerivativeMap : register(t1);
Texture2DArray<float> foamHistoryMap : register(t2);
Texture2D<float4> foamDetailMap : register(t3);
Texture2D<float4> sceneColorTexture : register(t4);
Texture2D<float> sceneDepthTexture : register(t5);

#if !defined(OCEAN_PREPASS) || defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    #define OCEAN_USE_SURFACE_INPUT 1
#else
    #define OCEAN_USE_SURFACE_INPUT 0
#endif

static const float OceanCascadeLodThreshold = 0.05f;

struct PSInput {
    float4 position : SV_POSITION;
#if OCEAN_USE_SURFACE_INPUT
    float3 worldPosition : TEXCOORD0;
    float2 samplePositionXY : TEXCOORD1;
#endif
#ifdef OUTPUT_VELOCITY
    float4 currUnjitteredClipPos : TEXCOORD2;
    float4 prevUnjitteredClipPos : TEXCOORD3;
#endif
};

float GetCascadePatchSize(uint cascadeIndex) {
    return oceanSurfaceParams.cascadePatchSizes[cascadeIndex];
}

// 카메라 거리에 따라 각 cascade 의 기여도를 smoothstep 곡선으로 감쇠한다.
float4 GetOceanCascadeLodWeights(float viewDistance) {
    const float4 fadeStartDistances = oceanSurfaceParams.cascadePatchSizes * oceanSurfaceParams.cascadeFadeScale;
    const float4 fadeFactors = (viewDistance - fadeStartDistances) / fadeStartDistances;
    return 1.0f - smoothstep(0.0f, 1.0f, fadeFactors);
}

// 각 cascade 의 displacement 맵을 정점 해상도에 맞는 mip 에서 샘플링하여 합성한다.
float3 SampleOceanDisplacement(float2 worldPositionXY, float4 cascadeLodWeights, float vertexMipLevelOffset) {
    float3 displacement = 0.0f;

    [unroll]
    for (uint cascadeIndex = 0; cascadeIndex < OceanCascadeCount; ++cascadeIndex) {
        const float cascadeLodWeight = cascadeLodWeights[cascadeIndex];

        if (cascadeLodWeight > OceanCascadeLodThreshold) {
            const float2 uv = worldPositionXY / GetCascadePatchSize(cascadeIndex);
            const float displacementSlice = float(2u * cascadeIndex);
            const float mipLevel = oceanSurfaceParams.cascadeVertexMipLevels[cascadeIndex] + vertexMipLevelOffset;
            displacement += displacementAndDerivativeMap.SampleLevel(linearWrapSampler, float3(uv, displacementSlice), mipLevel).xyz * cascadeLodWeight;
        }
    }
    return displacement * oceanSurfaceParams.displacementScale;
}

// geometry clipmap 은 XY 와 전환 규칙만 제공하고 ocean 이 XYZ 변위와 필터링을 평가한다.
float3 SampleOceanClipmapPosition(float2 positionXY, uint level) {
    const float3 basePosition = float3(positionXY, oceanSurfaceParams.surfaceHeight);
    const float viewDistance = length(cameraConstants.cameraOrigin - basePosition);
    const float4 cascadeLodWeights = GetOceanCascadeLodWeights(viewDistance);
    return basePosition + SampleOceanDisplacement(positionXY, cascadeLodWeights, float(level));
}

[RootSignature(OCEAN_ROOT_SIGNATURE)]
PSInput VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
    PSInput result;

    const GeometryClipmapPatch patch = surfacePatchInstances[oceanSurfacePatchInstanceOffset + instanceID];
    const GeometryClipmapVertex vertex = GetGeometryClipmapVertex(vertexID, patch, cameraConstants.cameraOrigin.xy);
    float2 samplePositionXY;
    float3 worldPosition;
    // 전환 구간 밖은 한 번만 샘플링한다. 경계에서는 parent 와 같은 좌표/mip/거리 가중치를 사용한다.
    [branch]
    if (vertex.morph <= 0.0f) {
        samplePositionXY = vertex.position;
        worldPosition = SampleOceanClipmapPosition(vertex.position, patch.level);
    } else if (vertex.morph >= 1.0f) {
        samplePositionXY = vertex.parentPosition;
        worldPosition = SampleOceanClipmapPosition(vertex.parentPosition, patch.level + 1u);
    } else {
        const float3 finePosition = SampleOceanClipmapPosition(vertex.position, patch.level);
        const float3 parentPosition = SampleOceanClipmapPosition(vertex.parentPosition, patch.level + 1u);
        worldPosition = MorphGeometryClipmapPosition(finePosition, parentPosition, vertex.morph);
        samplePositionXY = lerp(vertex.position, vertex.parentPosition, vertex.morph);
    }
    const float4 worldPosition4 = float4(worldPosition, 1.0f);

    // 변위가 적용된 world position 과 surface sampling 좌표를 pixel shader 로 전달한다.
    result.position = mul(cameraConstants.viewProjMatrix, worldPosition4);
#if OCEAN_USE_SURFACE_INPUT
    result.worldPosition = worldPosition;
    result.samplePositionXY = samplePositionXY;
#endif

#ifdef OUTPUT_VELOCITY
    // 이전 displacement texture 는 보존하지 않으므로 우선 카메라 이동에 의한 velocity 만 기록한다.
    result.currUnjitteredClipPos = mul(cameraConstants.unjitteredViewProjMatrix, worldPosition4);
    result.prevUnjitteredClipPos = mul(cameraConstants.prevUnjitteredViewProjMatrix, worldPosition4);
#endif
    return result;
}

#if OCEAN_USE_SURFACE_INPUT
struct OceanSurfaceInput {
    float3 viewDir;
    float viewDistance;
    float3 normal;
    float roughness;
    float foamComplement;
    bool foamEnabled;
};

// Main 과 Prepass 가 같은 displacement derivative, 양면 normal, Specular AA 를 사용한다.
OceanSurfaceInput PrepareOceanSurfaceInput(PSInput input, bool isFrontFace) {
    const bool foamEnabled = (oceanSurfaceParams.surfaceOptions & OceanSurfaceOptionBitFoamEnabled) != 0u;
    const float3 viewVector = cameraConstants.cameraOrigin - input.worldPosition;
    const float viewDistance = length(viewVector);
    const float4 cascadeLodWeights = GetOceanCascadeLodWeights(viewDistance);
    float Dxdy = 0.0f;
    float4 derivatives = 0.0f;
    float foamComplement = 1.0f;

    // 활성 cascade 의 displacement derivative 와 foam history 를 pixel 단위로 합성한다.
    [unroll]
    for (uint cascadeIndex = 0; cascadeIndex < OceanCascadeCount; ++cascadeIndex) {
        const float cascadeLodWeight = cascadeLodWeights[cascadeIndex];
        if (cascadeLodWeight > OceanCascadeLodThreshold) {
            const float2 uv = input.samplePositionXY / GetCascadePatchSize(cascadeIndex);
            const float displacementSlice = float(2u * cascadeIndex);
            const float derivativeSlice = displacementSlice + 1.0f;

            Dxdy += displacementAndDerivativeMap.Sample(linearWrapSampler, float3(uv, displacementSlice)).w * cascadeLodWeight;
            derivatives += displacementAndDerivativeMap.Sample(linearWrapSampler, float3(uv, derivativeSlice)) * cascadeLodWeight;

            if (foamEnabled) {
                const float cascadeFoam = saturate(foamHistoryMap.Sample(linearWrapSampler, float3(uv, float(cascadeIndex))));
                foamComplement *= 1.0f - cascadeFoam * cascadeLodWeight;
            }
        }
    }

    Dxdy *= oceanSurfaceParams.displacementScale;
    derivatives *= oceanSurfaceParams.displacementScale;

    // 합성한 derivative 의 Jacobian 으로 변위된 수면의 geometric normal 을 복원한다.
    const float3 geometricNormal = CalculateOceanSurfaceNormal(Dxdy, derivatives);

    const float3 V = viewVector / max(viewDistance, 0.0001f);
    float3 normal = geometricNormal * (isFrontFace ? 1.0f : -1.0f);
    if (dot(normal, V) < 0.0f) {
        normal = -normal;
    }

    float specularRoughness = clamp(oceanSurfaceParams.roughness, MinRoughness, 1.0f);
    if ((cameraConstants.flags & ShaderCameraFlagUseSpecularAA) != 0u) {
        specularRoughness = ApplySpecularAA(specularRoughness, ComputeSpecularAAKernelRoughness(geometricNormal, frameConstants.specularAAVariance, frameConstants.specularAALimit));
    }

    OceanSurfaceInput surface;
    surface.viewDir = V;
    surface.viewDistance = viewDistance;
    surface.normal = normal;
    surface.roughness = specularRoughness;
    surface.foamComplement = foamComplement;
    surface.foamEnabled = foamEnabled;
    return surface;
}

#endif // OCEAN_USE_SURFACE_INPUT

#endif // __OCEAN_SURFACE_COMMON_HLSLI__
