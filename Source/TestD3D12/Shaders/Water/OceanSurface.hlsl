#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "OceanFieldCommon.hlsli"
#include "../CommonBindless.hlsli"
#include "../CameraPassOutputCommon.hlsli"
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
    uint oceanSurfaceTileInstanceOffset;
};

StructuredBuffer<OceanSurfaceTileInstance> surfaceTileInstances : register(t0);
Texture2DArray<float4> displacementAndDerivativeMap : register(t1);
Texture2DArray<float> foamHistoryMap : register(t2);
Texture2D<float4> foamDetailMap : register(t3);
Texture2D<float4> sceneColorTexture : register(t4);
Texture2D<float> sceneDepthTexture : register(t5);

#include "../Lighting/ShadowLibrary.hlsli"

static const float OceanCascadeLodThreshold = 0.05f;
static const float2 OceanRefractionHistoryRejectVelocity = float2(2.0f, 2.0f);

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
    if (cameraConstants.envLightSpecularCubeMapIndex != 0u && cameraConstants.envLightSpecularMipCount > 0u) {
        const TextureCube<float4> specularCubeMap = GetTextureCube(cameraConstants.envLightSpecularCubeMapIndex);
        const float3 cubeMapDirection = mul(cameraConstants.envLightAxis, float4(reflectionDir, 0.0f));
        const float mipLevel = roughness * float(cameraConstants.envLightSpecularMipCount - 1u);
        return specularCubeMap.SampleLevel(linearClampSampler, cubeMapDirection, mipLevel).rgb * cameraConstants.envLightColor * cameraConstants.envLightIntensity;
    }

    const float skyGradient = saturate(reflectionDir.z * 0.5f + 0.5f);
    return lerp(float3(0.055f, 0.085f, 0.12f), float3(0.28f, 0.48f, 0.78f), skyGradient);
}

float2 ProjectOceanWorldPositionToUV(float3 worldPosition) {
    const float4 clipPosition = mul(cameraConstants.viewProjMatrix, float4(worldPosition, 1.0f));
    const float2 ndcPosition = clipPosition.xy / max(clipPosition.w, FLT_MIN);
    return ndcPosition * float2(0.5f, -0.5f) + 0.5f;
}

uint2 GetScenePixelCoord(float2 uv, uint2 textureSize) {
    return min(uint2(saturate(uv) * float2(textureSize)), textureSize - 1u);
}

float LoadSceneDepth(float2 uv, uint2 textureSize) {
    return sceneDepthTexture.Load(int3(GetScenePixelCoord(uv, textureSize), 0));
}

bool IsSceneFarDepth(float depth) {
    return abs(depth - cameraConstants.ndcFarZ) <= 1.0e-6f;
}

float3 SampleOceanRefraction(float2 pixelPosition, float3 worldPosition, float viewDistance, float3 incidentVector, float3 normal, float eta, float3 waterBodyColor, out bool totalInternalReflection) {
    const uint2 snapshotSize = uint2(oceanSurfaceParams.sceneSnapshotSize);

    // 수면 위치를 scene texture 의 texel 중심에 맞춘다.
    const float2 baseUV = pixelPosition * oceanSurfaceParams.sceneSizeRcp;
    const float deepWaterThickness = oceanSurfaceParams.absorptionDistance * 4.0f;

    // Snell 법칙의 판별식이 음수이면 투과광 없이 전반사로 처리한다.
    const float NdotI = dot(normal, incidentVector);
    const float refractionK = 1.0f - eta * eta * (1.0f - NdotI * NdotI);
    totalInternalReflection = refractionK <= 0.0f;
    if (totalInternalReflection) {
        return waterBodyColor;
    }

    const float3 refractionDirection = refract(incidentVector, normal, eta);
    const float refractionTraceDistance = oceanSurfaceParams.refractionDistance;

    // 얕은 물에서는 수면 아래 물체까지의 거리에 비례하여 굴절 왜곡량을 줄인다.
    float refractionDepthFactor = 1.0f;
    const float baseDepth = LoadSceneDepth(baseUV, snapshotSize);
    if (!IsSceneFarDepth(baseDepth)) {
        const float3 baseWorldPosition = ReconstructPosition(baseUV, baseDepth, cameraConstants.invViewProjMatrix);
        const float baseViewDistance = length(baseWorldPosition - cameraConstants.cameraOrigin);
        const float baseWaterThickness = max(baseViewDistance - viewDistance, 0.0f);
        refractionDepthFactor = saturate(baseWaterThickness / max(refractionTraceDistance, 0.0001f));
    }

    // 평평한 수면의 굴절 이동을 제거하고 파도 normal 로 추가되는 화면 왜곡만 사용한다.
    // 기준 normal 에서 전반사가 발생하는 임계 구간은 offset 을 0 으로 만들어 불연속을 피한다.
    const float3 referenceNormal = float3(0.0f, 0.0f, normal.z >= 0.0f ? 1.0f : -1.0f);
    const float referenceNdotI = dot(referenceNormal, incidentVector);
    const float referenceRefractionK = 1.0f - eta * eta * (1.0f - referenceNdotI * referenceNdotI);
    const float2 projectedRefractedUV = ProjectOceanWorldPositionToUV(worldPosition + refractionDirection * refractionTraceDistance);
    float2 projectedReferenceUV = projectedRefractedUV;
    if (referenceRefractionK > 0.0f) {
        const float3 referenceRefractionDirection = refract(incidentVector, referenceNormal, eta);
        projectedReferenceUV = ProjectOceanWorldPositionToUV(worldPosition + referenceRefractionDirection * refractionTraceDistance);
    }

    // 굴절점이 화면 밖으로 나가면 가장 가까운 scene texture 의 texel 중심으로 제한한다.
    const float2 halfTexelSize = oceanSurfaceParams.sceneSnapshotSizeRcp * 0.5f;
    const float2 pnoOffset = (projectedRefractedUV - projectedReferenceUV) * refractionDepthFactor;
    const float2 candidateUV = clamp(baseUV + pnoOffset, halfTexelSize, 1.0f - halfTexelSize);

    // 굴절 위치가 foreground 를 가리키면 hard fallback 하지 않고 offset 을 연속적으로 줄인다.
    float refractionValidity = 1.0f;
    const float candidateDepth = LoadSceneDepth(candidateUV, snapshotSize);
    if (!IsSceneFarDepth(candidateDepth)) {
        const float3 candidateWorldPosition = ReconstructPosition(candidateUV, candidateDepth, cameraConstants.invViewProjMatrix);
        const float candidateViewDistance = length(candidateWorldPosition - cameraConstants.cameraOrigin);
        const float depthFadeDistance = max(2.0f * fwidth(viewDistance), 0.02f);
        refractionValidity = smoothstep(0.0f, depthFadeDistance, candidateViewDistance - viewDistance);
    }

    const float2 sampleUV = lerp(baseUV, candidateUV, refractionValidity);

    // 최종 color 와 같은 UV 의 depth 로 물을 통과한 거리를 계산한다.
    float waterThickness = deepWaterThickness;
    const float sampleDepth = LoadSceneDepth(sampleUV, snapshotSize);
    if (!IsSceneFarDepth(sampleDepth)) {
        const float3 sampleWorldPosition = ReconstructPosition(sampleUV, sampleDepth, cameraConstants.invViewProjMatrix);
        const float sampleViewDistance = length(sampleWorldPosition - cameraConstants.cameraOrigin);
        waterThickness = max(sampleViewDistance - viewDistance, 0.0f);
    }

    // 물을 통과한 거리에 따라 scene color 를 감쇠시키고 굴절을 끈 경우와 같은 water body color 로 수렴시킨다.
    const float3 sceneColor = sceneColorTexture.SampleLevel(pointClampSampler, sampleUV, 0.0f).rgb;
    const float absorptionScale = min(waterThickness / oceanSurfaceParams.absorptionDistance, 8.0f);
    const float3 transmittance = pow(oceanSurfaceParams.waterColor, absorptionScale);
    return sceneColor * transmittance + waterBodyColor * (1.0f - transmittance);
}

[RootSignature(OCEAN_ROOT_SIGNATURE)]
PSInput VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
    PSInput result;

    // 타일과 이웃 타일의 LOD 를 반영하여 경계 정점을 coarse 격자에 맞춘다.
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

    // 카메라 거리와 정점 LOD 에 맞는 cascade 및 mip 에서 displacement 를 합성한다.
    const float viewDistance = length(cameraConstants.cameraOrigin - baseWorldPosition);
    const float4 cascadeLodWeights = GetOceanCascadeLodWeights(viewDistance);
    const float vertexMipLevelOffset = GetOceanSurfaceGridLodMipOffset(vertexLodLevel);
    const float3 displacement = SampleOceanDisplacement(samplePositionXY, cascadeLodWeights, vertexMipLevelOffset);
    const float3 worldPosition = float3(samplePositionXY + displacement.xy, oceanSurfaceParams.surfaceHeight + displacement.z);
    const float4 worldPosition4 = float4(worldPosition, 1.0f);

    // 변위가 적용된 world position 과 surface sampling 좌표를 pixel shader 로 전달한다.
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
CameraPassPSOutput PSMain(PSInput input, bool isFrontFace : SV_IsFrontFace) {
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
    const float NdotV = saturate(dot(normal, V));
    float fresnel = oceanSurfaceParams.fresnelF0 + (1.0f - oceanSurfaceParams.fresnelF0) * Pow5(1.0f - NdotV);

    float specularRoughness = clamp(oceanSurfaceParams.roughness, MinRoughness, 1.0f);
    if ((cameraConstants.flags & ShaderCameraFlagUseSpecularAA) != 0u) {
        specularRoughness = ApplySpecularAA(specularRoughness, ComputeSpecularAAKernelRoughness(normal));
    }

    // Fresnel 계수로 환경 반사와 선택적인 scene refraction 을 합성한다.
    const float3 reflectionDir = reflect(-V, normal);
    const float3 reflection = SampleOceanReflection(reflectionDir, specularRoughness);
    const float waterBodyFactor = lerp(0.35f, 1.0f, NdotV);
    const float3 waterBodyColor = oceanSurfaceParams.waterColor * waterBodyFactor;
    float3 transmission = waterBodyColor;
    bool refractionApplied = false;
    if ((oceanSurfaceParams.surfaceOptions & OceanSurfaceOptionBitRefractionEnabled) != 0u) {
        const float eta = isFrontFace ? oceanSurfaceParams.airToWaterEta : oceanSurfaceParams.waterToAirEta;
        bool totalInternalReflection;
        transmission = SampleOceanRefraction(input.position.xy, input.worldPosition, viewDistance, -V, normal, eta, waterBodyColor, totalInternalReflection);
        refractionApplied = !totalInternalReflection;
        if (totalInternalReflection) {
            fresnel = 1.0f;
        }
    }
    float3 color = lerp(transmission, reflection, fresnel);

    if ((cameraConstants.flags & ShaderCameraFlagUseDirectionalLight) != 0u) {
        const float3 L = -frameConstants.directionalLight.direction;
        const float NdotL = saturate(dot(normal, L));

        const ShadowLighting shadowLighting = EvaluateDirectionalShadowLighting((uint2)input.position.xy, input.worldPosition);
        const float3 lightColor = frameConstants.directionalLight.lightColor * shadowLighting;

        // 얇은 파도 마루를 통과한 직접광은 광원이 시선 반대편에 있을수록 강하게 보인다.
        const float normalizedCrestHeight = saturate((input.worldPosition.z - oceanSurfaceParams.surfaceHeight) / oceanSurfaceParams.scatterHeight);
        const float crestScatter = normalizedCrestHeight * normalizedCrestHeight;
        const float backLight = saturate(dot(V, -L));
        const float scatterPhase = backLight * backLight;
        const float scatterAmount = (1.0f - fresnel) * crestScatter * scatterPhase;
        const float3 relativeScatter = oceanSurfaceParams.scatterColor * (1.0f - oceanSurfaceParams.waterColor);
        color += lightColor * relativeScatter * scatterAmount;
        
        if (NdotL > 0.0f) {
            color += lightColor * oceanSurfaceParams.waterColor * (0.02f * NdotL);

            const float3 halfVectorUnnormalized = L + V;
            const float halfVectorLengthSquared = dot(halfVectorUnnormalized, halfVectorUnnormalized);
            if (halfVectorLengthSquared > FLT_MIN) {
                const float3 H = halfVectorUnnormalized * rsqrt(halfVectorLengthSquared);
                const float NdotH = saturate(dot(normal, H));
                const float VdotH = saturate(dot(V, H));
                const float linearRoughness = specularRoughness * specularRoughness;
                const float distribution = D_GGX(NdotH, linearRoughness);
                const float visibility = G_SmithGGXCorrelated(max(NdotV, 1e-4f), NdotL, linearRoughness);
                const float sunFresnel = oceanSurfaceParams.fresnelF0 + (1.0f - oceanSurfaceParams.fresnelF0) * Pow5(1.0f - VdotH);
                const float sunSpecular = distribution * visibility * sunFresnel * NdotL * PI;

                color += lightColor * sunSpecular;
            }
        }
    }

    // 낮은 history 값이 넓은 회색 막처럼 보이지 않도록 cascade 합성 결과의 대비를 높인다.
    float foam = smoothstep(0.0f, 1.0f, 1.0f - foamComplement);

    if (foamEnabled && oceanSurfaceParams.foamDetailStrength > 0.0f) {
        const float2 foamDetailUV = input.samplePositionXY / oceanSurfaceParams.foamDetailMapSize;
        const float foamDetail = foamDetailMap.Sample(linearWrapSampler, foamDetailUV).a;
        foam *= lerp(1.0f, foamDetail, oceanSurfaceParams.foamDetailStrength);
    }

    color = lerp(color, oceanSurfaceParams.foamColor, foam);

    CameraPassPSOutput result;
    result.color = float4(max(color, 0.0f), 1.0f);

#ifdef OUTPUT_NORMAL
    result.normal = EncodeOctahedralNormal(mul((float3x3)cameraConstants.viewRotation, normal));
#endif
#ifdef OUTPUT_VELOCITY
    // 이전 ocean field 와 굴절 UV 가 없으므로 굴절 픽셀은 화면 밖으로 reprojection 하여 TAA history 를 사용하지 않는다.
    result.velocity = refractionApplied ? OceanRefractionHistoryRejectVelocity : CalcVelocity(input.currentVelocityPosition, input.prevVelocityPosition);
#endif
    return result;
}
