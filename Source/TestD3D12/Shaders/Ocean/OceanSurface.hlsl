#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "OceanFieldCommon.hlsli"
#include "../GeometryClipmap.hlsli"
#include "../CommonBindless.hlsli"
#include "../SurfacePassOutput.hlsli"
#include "../PBR/BRDFLibrary.hlsli"
#include "../PBR/SpecularAA.hlsli"
#if defined(INDIRECT_LIGHTING) && USE_SSR && !defined(SURFACE_PREPASS)
    #include "../SSR/SSRComposite.hlsli"
#endif

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

#include "../Lighting/ShadowLibrary.hlsli"

static const float OceanCascadeLodThreshold = 0.05f;

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float2 samplePositionXY : TEXCOORD1;
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

#ifdef INDIRECT_LIGHTING
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

// Diffuse irradiance cubemap 에는 Lambert BRDF 의 1 / PI 가 이미 포함되어 있다.
float3 SampleOceanDiffuseLighting(float3 normal) {
    if (cameraConstants.envLightDiffuseIrradianceCubeMapIndex != 0u) {
        const TextureCube<float4> diffuseIrradianceCubeMap = GetTextureCube(cameraConstants.envLightDiffuseIrradianceCubeMapIndex);
        const float3 cubeMapDirection = mul(cameraConstants.envLightAxis, float4(normal, 0.0f));
        return diffuseIrradianceCubeMap.SampleLevel(linearClampSampler, cubeMapDirection, 0.0f).rgb * cameraConstants.envLightColor * cameraConstants.envLightIntensity;
    }

    return 0.0f;
}
#endif

float2 ProjectOceanWorldPositionToUV(float3 worldPos) {
    const float4 clipPos = mul(cameraConstants.viewProjMatrix, float4(worldPos, 1.0f));
    const float2 ndc = clipPos.xy / max(clipPos.w, FLT_MIN);
    return ndc * float2(0.5f, -0.5f) + 0.5f;
}

uint2 GetScenePixelCoord(float2 uv, uint2 textureSize) {
    return min(uint2(saturate(uv) * float2(textureSize)), textureSize - 1u);
}

float LoadSceneDepth(float2 uv, uint2 textureSize) {
    return sceneDepthTexture.Load(int3(GetScenePixelCoord(uv, textureSize), 0));
}

bool IsSceneFarDepth(float depth) {
    return abs(depth - cameraConstants.depthClearValue) <= 1.0e-6f;
}

float3 SampleOceanRefraction(float2 pixelPosition, float3 worldPosition, float viewDistance, float3 incidentVector, float3 normal, float eta, float3 waterBodyColor, out bool totalInternalReflection, out float refractionContribution) {
    refractionContribution = 0.0f;
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
        const float3 baseWorldPosition = ReconstructWorldPosition(baseUV, baseDepth, cameraConstants.invViewProjMatrix);
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
        const float3 candidateWorldPosition = ReconstructWorldPosition(candidateUV, candidateDepth, cameraConstants.invViewProjMatrix);
        const float candidateViewDistance = length(candidateWorldPosition - cameraConstants.cameraOrigin);
        const float depthFadeDistance = max(2.0f * fwidth(viewDistance), 0.02f);
        refractionValidity = smoothstep(0.0f, depthFadeDistance, candidateViewDistance - viewDistance);
    }

    const float2 sampleUV = lerp(baseUV, candidateUV, refractionValidity);

    // 최종 color 와 같은 UV 의 depth 로 물을 통과한 거리를 계산한다.
    float waterThickness = deepWaterThickness;
    const float sampleDepth = LoadSceneDepth(sampleUV, snapshotSize);
    if (!IsSceneFarDepth(sampleDepth)) {
        const float3 sampleWorldPosition = ReconstructWorldPosition(sampleUV, sampleDepth, cameraConstants.invViewProjMatrix);
        const float sampleViewDistance = length(sampleWorldPosition - cameraConstants.cameraOrigin);
        waterThickness = max(sampleViewDistance - viewDistance, 0.0f);
    }

    // 물을 통과한 거리에 따라 scene color 를 감쇠시키고 굴절을 끈 경우와 같은 water body color 로 수렴시킨다.
    const float3 sceneColor = sceneColorTexture.SampleLevel(pointClampSampler, sampleUV, 0.0f).rgb;
    const float absorptionScale = min(waterThickness / oceanSurfaceParams.absorptionDistance, 8.0f);
    const float3 transmittance = pow(oceanSurfaceParams.waterColor, absorptionScale);
    refractionContribution = saturate(max(transmittance.r, max(transmittance.g, transmittance.b)));
    return sceneColor * transmittance + waterBodyColor * (1.0f - transmittance);
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
    result.worldPosition = worldPosition;
    result.samplePositionXY = samplePositionXY;

#ifdef OUTPUT_VELOCITY
    // 이전 displacement texture 는 보존하지 않으므로 우선 카메라 이동에 의한 velocity 만 기록한다.
    result.currUnjitteredClipPos = mul(cameraConstants.unjitteredViewProjMatrix, worldPosition4);
    result.prevUnjitteredClipPos = mul(cameraConstants.prevUnjitteredViewProjMatrix, worldPosition4);
#endif
    return result;
}

[RootSignature(OCEAN_ROOT_SIGNATURE)]
#ifdef SURFACE_PREPASS
SurfacePrepassPSOutput PSMain(PSInput input, bool isFrontFace : SV_IsFrontFace) {
#else
SurfacePassPSOutput PSMain(PSInput input, bool isFrontFace : SV_IsFrontFace) {
#endif
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
        specularRoughness = ApplySpecularAA(specularRoughness, ComputeSpecularAAKernelRoughness(geometricNormal, frameConstants.specularAAVariance, frameConstants.specularAALimit));
    }

#ifdef SURFACE_PREPASS
    SurfacePrepassPSOutput result;
#if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    result.normalRoughness = float4(0.5f, 0.5f, 1.0f, 0.0f);
#endif
    #ifdef OUTPUT_NORMAL
        result.normalRoughness.xy = EncodeUnitVectorOctahedralUNorm(mul((float3x3)cameraConstants.viewRotation, normal));
    #endif
    #ifdef OUTPUT_VELOCITY
        result.velocity = CalcVelocity(input.currUnjitteredClipPos, input.prevUnjitteredClipPos);
    #endif
    #ifdef OUTPUT_ROUGHNESS
        result.normalRoughness.z = specularRoughness;
    #endif
    return result;
#else

    float3 reflection = 0.0f;
#ifdef INDIRECT_LIGHTING
    // 공용 SSR radiance 를 환경 반사 fallback 과 합성한 뒤 Fresnel 로 선택적인 scene refraction 과 섞는다.
    const float3 reflectionDir = reflect(-V, normal);
    reflection = SampleOceanReflection(reflectionDir, specularRoughness);
#if USE_SSR
    [branch]
    if ((cameraConstants.flags & ShaderCameraFlagUseTransparentSSR) != 0u) {
        reflection = CompositeTransparentSSRRadiance(
            (uint2)input.position.xy, input.position.z, reflection,
            cameraConstants.transparentSSRUpsampledReflectionTextureIndex, cameraConstants.transparentSSRDepthTextureIndex);
    }
#endif
#endif
    // 직접광의 shadow 는 한 번만 평가하고 물 내부색, 파도 마루 산란, 정반사와 거품에 공유한다.
    const bool directionalLightEnabled = (cameraConstants.flags & ShaderCameraFlagUseDirectionalLight) != 0u;
    float3 L = 0.0f;
    float NdotL = 0.0f;
    float3 lightIntensityRGB = 0.0f;
    if (directionalLightEnabled) {
        L = -frameConstants.directionalLight.direction;
        NdotL = saturate(dot(normal, L));
        const ShadowLighting shadowLighting = EvaluateDirectionalShadowLighting((uint2)input.position.xy, input.worldPosition);
        lightIntensityRGB = frameConstants.directionalLight.intensityRGB * shadowLighting;
    }
    const float3 diffuseLightIntensityRGB = lightIntensityRGB * Fd_Lambert();

    // 물 내부색은 평균 수면으로 들어오는 빛으로 조명하며 굴절의 흡수 종착색에도 같은 값을 사용한다.
    float3 waterBodyLighting = diffuseLightIntensityRGB * saturate(L.z);
#ifdef INDIRECT_LIGHTING
    waterBodyLighting += SampleOceanDiffuseLighting(float3(0.0f, 0.0f, 1.0f));
#endif
    const float waterBodyFactor = lerp(0.35f, 1.0f, NdotV);
    const float3 waterBodyColor = oceanSurfaceParams.waterColor * waterBodyFactor * waterBodyLighting;
    float3 transmission = waterBodyColor;
    float refractionContribution = 0.0f;
    if ((oceanSurfaceParams.surfaceOptions & OceanSurfaceOptionBitRefractionEnabled) != 0u) {
        const float eta = isFrontFace ? oceanSurfaceParams.airToWaterEta : oceanSurfaceParams.waterToAirEta;
        bool totalInternalReflection;
        transmission = SampleOceanRefraction(input.position.xy, input.worldPosition, viewDistance, -V, normal, eta, waterBodyColor, totalInternalReflection, refractionContribution);
        if (totalInternalReflection) {
            fresnel = 1.0f;
        }
    }
    float3 color = lerp(transmission, reflection, fresnel);

    if (directionalLightEnabled) {
        // 얇은 파도 마루를 통과한 직접광은 광원이 시선 반대편에 있을수록 강하게 보인다.
        const float normalizedCrestHeight = saturate((input.worldPosition.z - oceanSurfaceParams.surfaceHeight) / oceanSurfaceParams.scatterHeight);
        const float crestScatter = normalizedCrestHeight * normalizedCrestHeight;
        const float backLight = saturate(dot(V, -L));
        const float scatterPhase = backLight * backLight;
        const float scatterAmount = (1.0f - fresnel) * crestScatter * scatterPhase;
        const float3 relativeScatter = oceanSurfaceParams.scatterColor * (1.0f - oceanSurfaceParams.waterColor);
        color += lightIntensityRGB * relativeScatter * scatterAmount;
        
        if (NdotL > 0.0f) {
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
                const float sunSpecular = distribution * visibility * sunFresnel * NdotL;

                color += lightIntensityRGB * sunSpecular;
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

    // 거품은 수면 normal 을 따르는 diffuse 재질로 조명한다. 거품이 없으면 환경광 샘플링을 생략한다.
    [branch]
    if (foam > 0.0f) {
        float3 foamLighting = diffuseLightIntensityRGB * NdotL;
#ifdef INDIRECT_LIGHTING
        foamLighting += SampleOceanDiffuseLighting(normal);
#endif
        const float3 foamColor = oceanSurfaceParams.foamColor * foamLighting;
        color = lerp(color, foamColor, foam);
    }

    SurfacePassPSOutput result;
#if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    result.normalRoughness = float4(0.5f, 0.5f, 1.0f, 0.0f);
#endif
    result.color = float4(max(color, 0.0f), 1.0f);

#ifdef OUTPUT_NORMAL
    result.normalRoughness.xy = EncodeUnitVectorOctahedralUNorm(mul((float3x3)cameraConstants.viewRotation, normal));
#endif
#ifdef OUTPUT_ROUGHNESS
    result.normalRoughness.z = specularRoughness;
#endif
#ifdef OUTPUT_VELOCITY
    // 굴절에도 표면 velocity 를 유지해 jitter 를 누적한다. 이전 굴절 UV 는 없으므로
    // scene refraction 기여도만 별도로 기록하고 TAA 에서 색 변화에 따라 history 비중을 제한한다.
    result.velocity = CalcVelocity(input.currUnjitteredClipPos, input.prevUnjitteredClipPos);
    result.taaReactiveMask = saturate(refractionContribution * (1.0f - fresnel) * (1.0f - foam));
#endif
    return result;
#endif
}
