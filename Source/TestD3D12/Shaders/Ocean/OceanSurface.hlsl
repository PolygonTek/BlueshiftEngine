#include "OceanSurfaceCommon.hlsli"
#include "../Lighting/ShadowLibrary.hlsli"
#if defined(INDIRECT_LIGHTING) && USE_SSR
    #include "../SSR/SSRComposite.hlsli"
#endif

struct OceanMainPSOutput {
    float4 color : SV_Target0;
#ifdef OUTPUT_VELOCITY
    float2 velocity : SV_Target1;
    float taaReactiveMask : SV_Target2;
#endif
};

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
OceanMainPSOutput PSMain(PSInput input, bool isFrontFace : SV_IsFrontFace) {
    const OceanSurfaceInput surface = PrepareOceanSurfaceInput(input, isFrontFace);
    const float3 V = surface.viewDir;
    const float viewDistance = surface.viewDistance;
    const float3 normal = surface.normal;
    const float specularRoughness = surface.roughness;
    const float foamComplement = surface.foamComplement;
    const bool foamEnabled = surface.foamEnabled;
    const float NdotV = saturate(dot(normal, V));
    float fresnel = oceanSurfaceParams.fresnelF0 + (1.0f - oceanSurfaceParams.fresnelF0) * Pow5(1.0f - NdotV);

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

    OceanMainPSOutput result;
    result.color = float4(max(color, 0.0f), 1.0f);

#ifdef OUTPUT_VELOCITY
    // 굴절에도 표면 velocity 를 유지해 jitter 를 누적한다. 이전 굴절 UV 는 없으므로
    // scene refraction 기여도만 별도로 기록하고 TAA 에서 색 변화에 따라 history 비중을 제한한다.
    result.velocity = CalcVelocity(input.currUnjitteredClipPos, input.prevUnjitteredClipPos);
    result.taaReactiveMask = saturate(refractionContribution * (1.0f - fresnel) * (1.0f - foam));
#endif
    return result;
}
