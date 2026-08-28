#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../CommonBindless.hlsli"
#include "../CameraPassOutputCommon.hlsli"

#define OCEAN_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    "CBV(b0), " \
    "CBV(b1), " \
    "CBV(b2), " \
    "RootConstants(num32BitConstants=1, b3), " \
    "DescriptorTable(SRV(t0, numDescriptors = 5)), " \
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
    uint oceanSurfaceTileInstanceOffset;
};

Texture2DArray<float4> displacementAndDerivativeMap : register(t0);
Texture2DArray<float> foamHistoryMap : register(t1);
Texture2D<float4> foamDetailMap : register(t2);
Texture2D<float4> waterDetailNormalMap : register(t3);
StructuredBuffer<OceanSurfaceTileInstance> surfaceTileInstances : register(t4);

static const float OceanCascadeLodThreshold = 0.05f;

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float2 samplePositionXY : TEXCOORD1;
#ifdef OUTPUT_VELOCITY
    float4 currentVelocityPosition : TEXCOORD2;
    float4 prevVelocityPosition : TEXCOORD3;
#endif
};

float GetCascadePatchSize(uint cascadeIndex) {
    return oceanSurfaceParams.cascadePatchSizes[cascadeIndex];
}

uint GetOceanSurfaceGridLodStep(uint lodLevel) {
    return lodLevel == 0u ? 1u : (lodLevel == 1u ? 3u : 5u);
}

float GetOceanSurfaceGridLodMipOffset(uint lodLevel) {
    return lodLevel == 0u ? 0.0f : (lodLevel == 1u ? 1.5849625f : 2.3219281f);
}

uint GetOceanSurfaceNeighborLodLevel(uint packedNeighborLodLevels, uint bitOffset) {
    return (packedNeighborLodLevels >> bitOffset) & 0xffu;
}

uint SnapOceanSurfaceGridCoordinate(uint coordinate, uint gridStep) {
    return min(((coordinate + gridStep / 2u) / gridStep) * gridStep, OceanSurfaceGridResolution - 1u);
}

// 고해상도 타일의 경계 정점을 coarse 이웃의 정점 위치와 displacement mip 에 맞춘다.
void StitchOceanSurfaceGridEdges(inout uint2 gridCoord, OceanSurfaceTileInstance tileInstance, out uint vertexLodLevel) {
    vertexLodLevel = tileInstance.lodLevel;

    if (gridCoord.x == 0u) {
        const uint neighborLodLevel = GetOceanSurfaceNeighborLodLevel(tileInstance.packedNeighborLodLevels, 0u);
        if (neighborLodLevel > vertexLodLevel) {
            gridCoord.y = SnapOceanSurfaceGridCoordinate(gridCoord.y, GetOceanSurfaceGridLodStep(neighborLodLevel));
            vertexLodLevel = neighborLodLevel;
        }
    } else if (gridCoord.x == OceanSurfaceGridResolution - 1u) {
        const uint neighborLodLevel = GetOceanSurfaceNeighborLodLevel(tileInstance.packedNeighborLodLevels, 8u);
        if (neighborLodLevel > vertexLodLevel) {
            gridCoord.y = SnapOceanSurfaceGridCoordinate(gridCoord.y, GetOceanSurfaceGridLodStep(neighborLodLevel));
            vertexLodLevel = neighborLodLevel;
        }
    }

    if (gridCoord.y == 0u) {
        const uint neighborLodLevel = GetOceanSurfaceNeighborLodLevel(tileInstance.packedNeighborLodLevels, 16u);
        if (neighborLodLevel > vertexLodLevel) {
            gridCoord.x = SnapOceanSurfaceGridCoordinate(gridCoord.x, GetOceanSurfaceGridLodStep(neighborLodLevel));
            vertexLodLevel = neighborLodLevel;
        }
    } else if (gridCoord.y == OceanSurfaceGridResolution - 1u) {
        const uint neighborLodLevel = GetOceanSurfaceNeighborLodLevel(tileInstance.packedNeighborLodLevels, 24u);
        if (neighborLodLevel > vertexLodLevel) {
            gridCoord.x = SnapOceanSurfaceGridCoordinate(gridCoord.x, GetOceanSurfaceGridLodStep(neighborLodLevel));
            vertexLodLevel = neighborLodLevel;
        }
    }
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

// 환경광 specular cubemap 을 거칠기에 맞는 mip 에서 샘플링하여 수면 반사광을 계산한다.
float3 SampleOceanReflection(float3 reflectionDir, float roughness) {
    if (cameraConstants.environmentLightSpecularCubeMapIndex != 0u && cameraConstants.environmentLightSpecularMipCount > 0u) {
        const TextureCube<float4> specularCubeMap = GetTextureCube(cameraConstants.environmentLightSpecularCubeMapIndex);
        const float3 cubeMapDirection = mul(cameraConstants.environmentLightAxis, float4(reflectionDir, 0.0f));
        const float mipLevel = roughness * float(cameraConstants.environmentLightSpecularMipCount - 1u);
        return specularCubeMap.SampleLevel(linearClampSampler, cubeMapDirection, mipLevel).rgb * cameraConstants.environmentLightColor * cameraConstants.environmentLightIntensity;
    }

    const float skyGradient = saturate(reflectionDir.z * 0.5f + 0.5f);
    return lerp(float3(0.055f, 0.085f, 0.12f), float3(0.28f, 0.48f, 0.78f), skyGradient);
}

[RootSignature(OCEAN_ROOT_SIGNATURE)]
PSInput VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
    PSInput result;

    uint2 gridCoord = uint2(vertexID % OceanSurfaceGridResolution, vertexID / OceanSurfaceGridResolution);
    const OceanSurfaceTileInstance tileInstance = surfaceTileInstances[oceanSurfaceTileInstanceOffset + instanceID];
    uint vertexLodLevel;
    StitchOceanSurfaceGridEdges(gridCoord, tileInstance, vertexLodLevel);

    const float gridHalfExtent = float(OceanSurfaceGridResolution - 1u) * 0.5f;
    const float2 gridPosition = float2(gridCoord) - gridHalfExtent;
    const float2 tiledGridPosition = gridPosition + tileInstance.tilePosition * float(OceanSurfaceGridResolution - 1u);

    // 활성 LOD stride 의 최소공배수 간격으로 이동하여 모든 LOD 의 world-space 정점 격자 위상을 유지한다.
    const float2 snappedCameraPosition = floor(cameraConstants.cameraOrigin.xy / oceanSurfaceParams.gridSnapSpacing + 0.5f) * oceanSurfaceParams.gridSnapSpacing;
    const float2 samplePositionXY = snappedCameraPosition + tiledGridPosition * oceanSurfaceParams.gridSpacing;
    const float3 baseWorldPosition = float3(samplePositionXY, oceanSurfaceParams.surfaceHeight);
    const float viewDistance = length(cameraConstants.cameraOrigin - baseWorldPosition);
    const float4 cascadeLodWeights = GetOceanCascadeLodWeights(viewDistance);
    const float vertexMipLevelOffset = GetOceanSurfaceGridLodMipOffset(vertexLodLevel);
    const float3 displacement = SampleOceanDisplacement(samplePositionXY, cascadeLodWeights, vertexMipLevelOffset);
    const float3 worldPosition = float3(samplePositionXY + displacement.xy, oceanSurfaceParams.surfaceHeight + displacement.z);
    const float4 worldPosition4 = float4(worldPosition, 1.0f);

    result.position = mul(cameraConstants.viewProjMatrix, worldPosition4);
    result.worldPosition = worldPosition;
    result.samplePositionXY = samplePositionXY;

#ifdef OUTPUT_VELOCITY
    // 이전 displacement texture 는 보존하지 않으므로 우선 카메라 이동에 의한 velocity 만 기록한다.
    result.currentVelocityPosition = mul(cameraConstants.unjitteredViewProjMatrix, worldPosition4);
    result.prevVelocityPosition = mul(cameraConstants.prevUnjitteredViewProjMatrix, worldPosition4);
#endif
    return result;
}

[RootSignature(OCEAN_ROOT_SIGNATURE)]
CameraPassPSOutput PSMain(PSInput input) {
    float displacementX_dy = 0.0f;
    float4 derivatives = 0.0f;
    float foamComplement = 1.0f;
    const bool foamEnabled = (oceanSurfaceParams.foamOptions & OceanFoamOptionBitEnabled) != 0u;
    const float3 viewVector = cameraConstants.cameraOrigin - input.worldPosition;
    const float viewDistance = length(viewVector);
    const float4 cascadeLodWeights = GetOceanCascadeLodWeights(viewDistance);

    [unroll]
    for (uint cascadeIndex = 0; cascadeIndex < OceanCascadeCount; ++cascadeIndex) {
        const float cascadeLodWeight = cascadeLodWeights[cascadeIndex];
        if (cascadeLodWeight > OceanCascadeLodThreshold) {
            const float2 uv = input.samplePositionXY / GetCascadePatchSize(cascadeIndex);
            const float displacementSlice = float(2u * cascadeIndex);
            const float derivativeSlice = displacementSlice + 1.0f;

            displacementX_dy += displacementAndDerivativeMap.Sample(linearWrapSampler, float3(uv, displacementSlice)).w * cascadeLodWeight;
            derivatives += displacementAndDerivativeMap.Sample(linearWrapSampler, float3(uv, derivativeSlice)) * cascadeLodWeight;

            if (foamEnabled) {
                const float cascadeFoam = saturate(foamHistoryMap.Sample(linearWrapSampler, float3(uv, float(cascadeIndex))));
                foamComplement *= 1.0f - cascadeFoam * cascadeLodWeight;
            }
        }
    }

    displacementX_dy *= oceanSurfaceParams.displacementScale;
    derivatives *= oceanSurfaceParams.displacementScale;

    const float displacementZ_dx = derivatives.x;
    const float displacementZ_dy = derivatives.y;
    const float displacementX_dx = derivatives.z;
    const float displacementY_dy = derivatives.w;

    // 낮은 history 값이 넓은 회색 막처럼 보이지 않도록 cascade 합성 결과의 대비를 높인다.
    float foam = smoothstep(0.0f, 1.0f, 1.0f - foamComplement);

    if (foamEnabled && oceanSurfaceParams.foamDetailStrength > 0.0f) {
        const float2 foamDetailUV = input.samplePositionXY / oceanSurfaceParams.foamDetailMapSize;
        const float foamDetail = foamDetailMap.Sample(linearWrapSampler, foamDetailUV).a;
        foam *= lerp(1.0f, foamDetail, oceanSurfaceParams.foamDetailStrength);
    }

    // Horizontal displacement Jacobian determinant
    const float J = (1.0f + displacementX_dx) * (1.0f + displacementY_dy) - displacementX_dy * displacementX_dy;

    // Surface normal
    const float3 unnormalizedNormal = float3(displacementZ_dy * displacementX_dy - displacementZ_dx * (1.0f + displacementY_dy), displacementZ_dx * displacementX_dy - displacementZ_dy * (1.0f + displacementX_dx), J);
    const float normalLengthSquared = dot(unnormalizedNormal, unnormalizedNormal);
    float3 normal = normalLengthSquared > 0.0f ? unnormalizedNormal * rsqrt(normalLengthSquared) : float3(0.0f, 0.0f, 1.0f);

    if (oceanSurfaceParams.normalStrength > 0.0f) {
        float3 surfaceTangent = float3(1.0f + displacementX_dx, displacementX_dy, displacementZ_dx);
        surfaceTangent -= normal * dot(surfaceTangent, normal);
        const float tangentLengthSquared = dot(surfaceTangent, surfaceTangent);
        if (tangentLengthSquared > 0.0f) {
            surfaceTangent *= rsqrt(tangentLengthSquared);
        } else {
            const float3 referenceAxis = abs(normal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
            surfaceTangent = normalize(cross(referenceAxis, normal));
        }
        const float3 surfaceBitangent = cross(normal, surfaceTangent);

        const float2 normalMapUV = input.samplePositionXY / oceanSurfaceParams.normalMapSize - frac(oceanSurfaceParams.normalMapUVOffset);
        const float2 detailNormalXY = (waterDetailNormalMap.Sample(linearWrapSampler, normalMapUV).xy * 2.0f - 1.0f) * oceanSurfaceParams.normalStrength;
        const float detailNormalZ = sqrt(saturate(1.0f - dot(detailNormalXY, detailNormalXY)));
        normal = normalize(surfaceTangent * detailNormalXY.x + surfaceBitangent * detailNormalXY.y + normal * detailNormalZ);
    }

    const float3 V = viewVector / max(viewDistance, 0.0001f);
    const float NdotV = saturate(dot(normal, V));
    const float fresnel = oceanSurfaceParams.fresnelF0 + (1.0f - oceanSurfaceParams.fresnelF0) * Pow5(1.0f - NdotV);

    const float3 reflectionDir = reflect(-V, normal);
    const float3 reflection = SampleOceanReflection(reflectionDir, oceanSurfaceParams.roughness);
    const float3 waterBodyColor = oceanSurfaceParams.waterColor * lerp(0.35f, 1.0f, NdotV);
    float3 color = lerp(waterBodyColor, reflection, fresnel);

    if ((cameraConstants.flags & ShaderCameraFlagUseDirectionalLight) != 0u) {
        const float3 L = -frameConstants.directionalLight.direction;
        const float NdotL = saturate(dot(normal, L));
        
        if (NdotL > 0.0f) {
            const float3 H = normalize(L + V);
            const float roughnessSquared = max(oceanSurfaceParams.roughness * oceanSurfaceParams.roughness, 0.0004f);
            const float specularPower = max(2.0f / roughnessSquared - 2.0f, 1.0f);
            const float specularNormalization = (specularPower + 2.0f) * (0.125f * INV_PI);
            const float NdotH = saturate(dot(normal, H));
            const float specular = pow(NdotH, specularPower) * specularNormalization;
            const float VdotH = saturate(dot(V, H));
            const float sunFresnel = oceanSurfaceParams.fresnelF0 + (1.0f - oceanSurfaceParams.fresnelF0) * Pow5(1.0f - VdotH);
            
            color += frameConstants.directionalLight.lightColor * NdotL * (oceanSurfaceParams.waterColor * 0.02f + specular * sunFresnel);
        }
    }

    color = lerp(color, oceanSurfaceParams.foamColor, foam);

    CameraPassPSOutput result;
    result.color = float4(max(color, 0.0f), 1.0f);

#ifdef OUTPUT_NORMAL
    result.normal = EncodeOctahedralNormal(normal);
#endif
#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currentVelocityPosition, input.prevVelocityPosition);
#endif
    return result;
}
