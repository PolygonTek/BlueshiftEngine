#include "VFogCommon.hlsli"
#include "HeightFogCommon.hlsli"
#include "LocalFogCommon.hlsli"
#include "FogEnvironment.hlsli"
#include "../BindlessResources.hlsli"
#include "../STBN.hlsli"

#define VFOG_INJECT_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    "DescriptorTable(UAV(u0)), " \
    "CBV(b0), " \
    "CBV(b1), " \
    "RootConstants(num32BitConstants=4, b3), " \
    "RootConstants(num32BitConstants=4, b2), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS
cbuffer FrameCB : register(b0) {
    FrameConstants frameConstants;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

cbuffer VFogLightingCB : register(b3) {
    VFogLightingParams vfogLightingParams;
}

cbuffer VFogTemporalCB : register(b2) {
    VFogTemporalParams vfogTemporalParams;
}

RWTexture3D<float4> outputSourceTexture : register(u0);

// VFogInject 에서는 CSM 디버그 모드를 비활성화한다.
#ifdef DEBUG_CASCADED_SHADOW_MAP
#undef DEBUG_CASCADED_SHADOW_MAP
#endif
#define DEBUG_CASCADED_SHADOW_MAP 0

#include "../Lighting/LightCullingCommon.hlsli"
#include "../Lighting/LightAttenuation.hlsli"
#include "../Lighting/ShadowLibrary.hlsli"

// Height fog 와 로컬 볼륨의 계수를 먼저 합산하고, 공통 anisotropy 로 lighting 을 평가한다.
struct VFogMedia {
    float3 scatteringCoeff; // 모든 매질의 RGB 산란계수 합
    float froxelDiagonalSquared; // 지터와 무관한 프록셀 크기 (m^2). 밀도 LOD 와 광원 거리 보정에 공유한다.
    float4 sourceExtinction; // RGB: 누적 산란/발광 source, A: 합산 소멸계수
};

// 산란 가능한 매질이 있는지 확인한다.
bool VFogHasScattering(VFogMedia media) {
    return any(media.scatteringCoeff > 0.0f);
}

// 프록셀 샘플 위치에서 카메라 쪽으로 향하는 월드 공간 단위 방향
float3 VFogGetOutgoingDirection(float2 rayXY) {
    return cameraConstants.IsOrthogonal() ? -cameraConstants.cameraForward : -mul(normalize(float3(rayXY, -1.0f)), (float3x3)cameraConstants.viewRotation);
}

// 월드 위치에서 Height Fog 와 Local Fog Volume 의 밀도/마스크를 샘플링해 extinction/scattering 계수와 ambient/emissive source 를 합산한다.
VFogMedia VFogInitializeMedia(uint3 froxelID, float3 worldPos, float2 rayXY, float depth, float froxelDiagonalSquared, HeightFogParams heightFogParams) {
    VFogMedia media = (VFogMedia)0;
    media.froxelDiagonalSquared = froxelDiagonalSquared;

    const float rayLengthSquared = cameraConstants.IsOrthogonal() ? 1.0f : 1.0f + dot(rayXY, rayXY);
    const float distanceSquared = depth * depth * rayLengthSquared;
    [branch]
    if (distanceSquared < heightFogParams.maxDistance * heightFogParams.maxDistance) {
        // Height fog 의 밀도를 샘플링하고 매질 속성을 계산한다.
        const float density = HeightFogSampleDensity(worldPos.z, heightFogParams);
        const FogMedium heightFogMedium = FogSampleMedium(density, heightFogParams.medium);

        // Height fog 전용 ambient/emissive source 와 extinction/scattering 계수를 누적한다.
        FogAccumulateAmbientEmissionAndExtinction(heightFogMedium, heightFogParams.ambientRadiance, media.sourceExtinction);
        media.scatteringCoeff = heightFogMedium.scatteringCoeff;
    }

    // 로컬 fog 볼륨의 밀도를 샘플링하고 emissive source 와 extinction/scattering 계수를 누적한다.
    [branch]
    if (cameraConstants.numLocalFogVolumes > 0) {
        const StructuredBuffer<ShaderLocalFogVolume> volumes = GetShaderLocalFogVolumeTable(frameConstants.shaderLocalFogVolumeTableIndex);
        const bool useLocalVolumeFroxelCulling = (vfogLightingParams.flags & VFogFlagLocalVolumeFroxelCulling) != 0;
        [loop]
        for (uint i = 0; i < cameraConstants.numLocalFogVolumes; ++i) {
            const uint volumeIndex = cameraConstants.localFogVolumeTableOffset + i;
            [branch]
            if (useLocalVolumeFroxelCulling) {
                const Buffer<uint> volumeBounds = GetBufferUInt(vfogLightingParams.localVolumeBoundsBufferIndex);
                const uint boundsOffset = volumeIndex * 3u;
                const uint3 packedBounds = uint3(volumeBounds[boundsOffset], volumeBounds[boundsOffset + 1u], volumeBounds[boundsOffset + 2u]);
                const uint3 froxelMin = packedBounds & 0xffffu;
                const uint3 froxelMax = packedBounds >> 16;

                // 매질 데이터를 읽기 전에 프록셀이 볼륨의 [min, max) 범위 안에 있는지 검사한다.
                [branch]
                if (any(froxelID < froxelMin) || any(froxelID >= froxelMax)) {
                    continue;
                }
            }

            const ShaderLocalFogVolume localFog = volumes[volumeIndex];
            const float4 localFogColorDensity = LocalFogSampleColorDensity(worldPos, media.froxelDiagonalSquared, localFog, linearWrapSampler, linearClampSampler);
            if (localFogColorDensity.a <= 0.0f) {
                continue;
            }

            // 밀도로 extinction/scattering 계수와 emission 을 계산한다.
            FogMedium localFogMedium = FogSampleMedium(localFogColorDensity.a, localFog.medium);
            localFogMedium.scatteringCoeff *= localFogColorDensity.rgb;

            // emission 에 의한 source 누적, extinction 계수 누적, scattering rgb 계수 누적.
            media.sourceExtinction.rgb += localFogMedium.emission;
            media.sourceExtinction.a += localFogMedium.extinctionCoeff;
            media.scatteringCoeff += localFogMedium.scatteringCoeff;
        }
    }

    return media;
}

// 입사광의 산란 기여를 source 에 누적한다.
void VFogAccumulateScatteringSource(float3 incidentRadiance, float lightWeight, float cosTheta, inout VFogMedia media) {
    if (lightWeight <= 0.0f) {
        return;
    }

    const float3 phaseWeightedScattering = media.scatteringCoeff * HenyeyGreensteinPhase(cosTheta, cameraConstants.fogAnisotropy);
    media.sourceExtinction.rgb += phaseWeightedScattering * incidentRadiance * lightWeight;
}

// 프록셀을 감싸는 뷰 공간 최소 AABB 의 대각선 길이 제곱을 구한다.
float VFogFroxelDiagonalSquared(float2 centerRayXY, float sliceNearDepth, float sliceFarDepth) {
    const VFogParams vfogParams = cameraConstants.vfogParams;
    const float froxelDepth = sliceFarDepth - sliceNearDepth;
    float2 froxelSizeXY = abs(cameraConstants.uvToViewScaleBiasXY.xy) * vfogParams.tileCountRcp;

    if (!cameraConstants.IsOrthogonal()) {
        froxelSizeXY = froxelSizeXY * sliceFarDepth + max(abs(centerRayXY) - 0.5f * froxelSizeXY, 0.0f) * froxelDepth;
    }
    return dot(froxelSizeXY, froxelSizeXY) + froxelDepth * froxelDepth;
}

// Sample point/spot incident radiance once, then apply the shared phase and combined scattering coefficient for outgoing direction wo.
void AccumulateVFogLocalLight(uint packedLightRef, float3 worldPos, float3 wo, float distanceBiasSquared, bool useShadows, inout ShadowSamplingContext shadowSampling, inout VFogMedia media) {
    const uint lightType = packedLightRef & ShaderLightTypeMask;
    const uint lightIndex = packedLightRef >> ShaderLightTypeBitCount;
    float3 lightVector, lightIntensity;
    float inverseRadiusSquared;
    uint shadowIndexAndFlags;

    ShaderSpotLight spotLight = (ShaderSpotLight)0;
    if (lightType == ShaderLightTypePoint) {
        const ShaderPointLight pointLight = GetShaderPointLightTable(frameConstants.shaderPointLightTableIndex)[lightIndex];
        lightVector = worldPos - pointLight.origin;
        lightIntensity = pointLight.intensityRGB;
        inverseRadiusSquared = pointLight.invRadiusSquared;
        shadowIndexAndFlags = pointLight.shadowIndexAndFlags;
    } else if (lightType == ShaderLightTypeSpot) {
        spotLight = GetShaderSpotLightTable(frameConstants.shaderSpotLightTableIndex)[lightIndex];
        lightVector = worldPos - spotLight.origin;
        lightIntensity = spotLight.intensityRGB;
        inverseRadiusSquared = spotLight.invRadiusSquared;
        shadowIndexAndFlags = spotLight.shadowIndexAndFlags;
    } else {
        return;
    }

    const float distanceSquared = dot(lightVector, lightVector);
    if (distanceSquared <= FLT_MIN || !any(lightIntensity > 0.0f)) {
        return;
    }

    float attenuation = CalcLightDistanceAttenuation(distanceSquared, inverseRadiusSquared, distanceBiasSquared);
    if (attenuation <= 0.0f) { 
        return;
    }
    
    const float3 wi = lightVector * rsqrt(distanceSquared);
    if (lightType == ShaderLightTypeSpot) {
        attenuation *= CalcSpotLightAngularAttenuation(dot(spotLight.direction, wi), spotLight.cosOuterCone, spotLight.invCosConeDifference);
        if (attenuation <= 0.0f) { 
            return; 
        }
    }

    const uint shadowIndex = shadowIndexAndFlags & ShaderLocalShadowIndexMask;
    const uint lightFlags = shadowIndexAndFlags >> ShaderLocalLightFlagsShift;
    const uint requiredShadowFlags = ShaderLightFlagCastShadows | ShaderLightFlagCastVolumetricShadows;
    float shadow = 1.0f;
    if (useShadows && shadowIndex != ShaderLocalShadowIndexInvalid && (lightFlags & requiredShadowFlags) == requiredShadowFlags) {
        shadow = lightType == ShaderLightTypePoint ? SamplePointShadowVisibility(shadowIndex, shadowSampling, worldPos) : SampleSpotShadowVisibility(shadowIndex, shadowSampling, worldPos);
    }
    VFogAccumulateScatteringSource(lightIntensity, shadow * attenuation, dot(wi, wo), media);
}

// 프록셀 history 를 재투영해 산란/발광 source 와 소멸계수를 RGBA 로 temporal 누적한다.
float4 AccumulateVFogHistory(uint3 id, float4 currentSourceExtinction, float3 centerWorldPos, float3 centerWorldOffset) {
    if (vfogTemporalParams.historyValid == 0 || vfogTemporalParams.historyWeight <= 0.0f) {
        return currentSourceExtinction;
    }

    const Texture3D<float4> historyTexture = GetTexture3D(vfogTemporalParams.historyTextureIndex);

    // 카메라가 정지해 있다면 같은 texel 의 history 를 직접 읽어 누적한다.
    if (cameraConstants.cameraUnchanged != 0) {
        const float4 historySourceExtinction = historyTexture.Load(int4(id, 0));
        return lerp(currentSourceExtinction, historySourceExtinction, vfogTemporalParams.historyWeight);
    }

    const VFogParams vfogParams = cameraConstants.vfogParams;
    const float3 prevToPos = centerWorldPos - cameraConstants.prevCameraOrigin;
    const float prevDepth = dot(prevToPos, cameraConstants.prevCameraForward);
    if (prevDepth < vfogParams.nearDepth || prevDepth > vfogParams.farDepth) {
        return currentSourceExtinction;
    }

    // 프록셀 중심의 월드 위치를 이전 프레임 카메라로 재투영하여 UVW 좌표를 구한다.
    const float4 position = float4(centerWorldPos, 1.0f);
    const float prevW = dot(cameraConstants.prevUnjitteredViewProjMatrix[3], position);
    if (prevW <= 0.0f) {
        return currentSourceExtinction;
    }
    const float2 prevClipPos = float2(dot(cameraConstants.prevUnjitteredViewProjMatrix[0], position), dot(cameraConstants.prevUnjitteredViewProjMatrix[1], position));
    const float2 prevNDC = prevClipPos / prevW;
    const float2 historyUV = prevNDC * float2(0.5f, -0.5f) + 0.5f;
    const float3 historyUVW = float3(historyUV, VFogDepthToSlice(prevDepth, vfogParams) / vfogParams.sliceCount);

    // 재투영한 UVW 가 반 텍셀 여백을 둔 볼륨 범위를 벗어나면 history 를 버린다.
    const float3 halfTexel = 0.5f * float3(vfogParams.tileCountRcp, 1.0f / vfogParams.sliceCount);
    if (any(historyUVW < halfTexel) || any(historyUVW > 1.0f - halfTexel)) {
        return currentSourceExtinction;
    }

    const bool isOrthographic = cameraConstants.IsOrthogonal();

    // 현재 프레임 카메라 위치에서의 프록셀 중심 시선 벡터
    const float3 wo = isOrthographic ? -cameraConstants.cameraForward : -normalize(centerWorldOffset);

    // 이전 프레임 카메라 위치에서의 프록셀 중심 시선 벡터
    const float3 prevWo = isOrthographic ? -cameraConstants.prevCameraForward : -normalize(prevToPos);

    // cos(theta)^8 로 history weight 를 조정한다.
    // History RGB 에는 이전 카메라 위치로 향하는 산란광의 phase 가 반영되어 있어, 방향에 따른 밝기 변화는 재투영만으로 보정할 수 없다.
    // 원근 투영은 현재 프레임의 프록셀 중심에서 현재/이전 카메라 위치로 향하는 방향을 비교하므로, 제자리 회전만으로는 가중치를 줄이지 않는다.
    // 직교 투영은 현재/이전 프레임의 -cameraForward 를 비교한다.
    // 이 방향의 차이가 클수록 이전 밝기의 잔상을 줄이도록 history weight 를 낮춘다.
    // 조정한 가중치는 RGB 와 A 에 함께 적용해 source 와 소멸계수의 누적 비율을 유지한다.
    float viewWeight = saturate(dot(wo, prevWo));
    viewWeight *= viewWeight;
    viewWeight *= viewWeight;
    viewWeight *= viewWeight;

    const float4 historySourceExtinction = historyTexture.SampleLevel(linearClampSampler, historyUVW, 0);
    return lerp(currentSourceExtinction, historySourceExtinction, vfogTemporalParams.historyWeight * viewWeight);
}

struct VFogFroxelSample {
    float3 worldPos;        // 프록셀 대표 샘플 월드 위치
    float2 rayXY;           // Perspective: 뷰 공간 z = -1 평면과 광선의 교점 XY, Orthographic: 뷰 공간 XY 위치
    uint2 pixelCoord;       // 렌더 해상도 기준의 샘플 픽셀 좌표
    float depth;            // 샘플의 선형 깊이
    float sliceNearDepth;   // 슬라이스 앞쪽 경계의 뷰 깊이
    float sliceFarDepth;    // 슬라이스 뒤쪽 경계의 뷰 깊이
};

// 지터 설정을 반영해 프록셀 내부의 대표 샘플 위치와 깊이 정보 등을 계산한다.
VFogFroxelSample VFogGetFroxelSample(uint3 id) {
    const VFogParams vfogParams = cameraConstants.vfogParams;
    const bool isOrthographic = cameraConstants.IsOrthogonal();
    VFogFroxelSample sample;

    // 프록셀 내부의 상대 샘플 위치. Jitter 를 끄면 XY 중심과 앞뒤 깊이의 중간점을 사용한다.
    float3 sampleOffset = 0.5f;
    [branch]
    if ((vfogLightingParams.flags & VFogFlagSTBNJitter) != 0) {
        const Texture2DArray<float4> stbnTexture = GetTexture2DArray(vfogLightingParams.stbnVec3TextureIndex);

        // 같은 XY 열의 모든 깊이 슬라이스는 동일한 XYZ 상대 위치를 사용한다.
        const float3 xi = ReadSTBNVec3(stbnTexture, id.xy, vfogTemporalParams.noiseFrameIndex);

        // 8-bit UNORM 값을 양자화 구간 중앙으로 옮겨 프록셀 경계의 0 과 1 을 피한다.
        sampleOffset = (xi * 255.0f + 0.5f) / 256.0f;
    }

    // 화면 공간 타일 내부의 샘플 UV
    const float2 sampleUV = (float2(id.xy) + sampleOffset.xy) * vfogParams.tileCountRcp;

    // 라이트 컬링 및 그림자 샘플링에 사용할 픽셀 좌표
    sample.pixelCoord = uint2(sampleUV * cameraConstants.renderSize);

    // 지수 분포의 프록셀 경계는 유지하고, 프록셀 내부에서 실제 깊이에 대해 균등하게 샘플링한다.
    sample.sliceNearDepth = VFogSliceToDepth(id.z, vfogParams);
    sample.sliceFarDepth = VFogSliceToDepth(id.z + 1u, vfogParams);
    sample.depth = lerp(sample.sliceNearDepth, sample.sliceFarDepth, sampleOffset.z);

    //  Perspective: 샘플 광선과 뷰 공간 z = -1 평면의 교점 XY
    // Orthographic: 샘플의 뷰 공간 XY 위치 (깊이에 무관)
    sample.rayXY = VFogGetRayXY(sampleUV, cameraConstants);

    // 뷰 공간에서의 프록셀 샘플 위치
    const float3 viewPos = float3(sample.rayXY * (isOrthographic ? 1.0f : sample.depth), -sample.depth);

    // 월드 공간에서의 프록셀 샘플 위치
    sample.worldPos = cameraConstants.cameraOrigin + mul(viewPos, (float3x3)cameraConstants.viewRotation);
    return sample;
}

// 산란 가능한 매질에 대해 밀도와 같은 위치에서 광원/그림자를 한 번 샘플링하고, 공통 phase 와 합산 산란계수를 적용한다.
void AccumulateVFogSceneLighting(VFogFroxelSample sample, float2 centerRayXY, inout VFogMedia media) {
    const float3 wo = VFogGetOutgoingDirection(sample.rayXY);

    // 합산한 산란계수와 공통 anisotropy 로 환경광 산란 source 를 추가 누적한다.
    [branch]
    if (cameraConstants.envLightRadianceSHBufferIndex != 0 && cameraConstants.envLightIntensity > 0.0f && any(cameraConstants.envLightColor > 0.0f)) {
        const FogEnvironmentSHBands environmentSHBands = FogSampleEnvironmentSHBands(wo, cameraConstants);
        media.sourceExtinction.rgb += media.scatteringCoeff * FogEvaluateEnvironmentScatteringRadiance(environmentSHBands, cameraConstants.fogAnisotropy);
    }

    const bool useDirectionalLight = (cameraConstants.flags & ShaderCameraFlagUseDirectionalLight) != 0;
    const bool useShadows = (vfogLightingParams.flags & VFogFlagShadows) != 0;

    ShadowSamplingContext shadowSampling = InitShadowSamplingContext(sample.pixelCoord);

    // Directional 라이트의 산란 source 누적
    if (useDirectionalLight && any(frameConstants.directionalLight.intensityRGB > 0.0f)) {
        const uint requiredShadowFlags = ShaderLightFlagCastShadows | ShaderLightFlagCastVolumetricShadows;
        const bool castVolumetricShadows = (frameConstants.directionalLight.flags & requiredShadowFlags) == requiredShadowFlags;
        const float shadow = useShadows && castVolumetricShadows ? SampleDirectionalShadowVisibility(shadowSampling, sample.worldPos) : 1.0f;
        VFogAccumulateScatteringSource(frameConstants.directionalLight.intensityRGB, shadow, dot(frameConstants.directionalLight.direction, wo), media);
    }

    // Point/Spot 라이트의 산란 source 누적
    const bool useZBin = (cameraConstants.flags & ShaderCameraFlagUseLightZBin) != 0;
    ShaderLightIterator iterator = BeginShaderLightIterationLinearDepth(sample.pixelCoord, sample.depth, useZBin, false);
    ShaderLightRef lightRef;
    if (NextShaderLight(iterator, lightRef)) {
        // distanceBiasSquared 를 로컬 광원이 있는 프록셀에서 한 번만 계산하고 모든 광원 계산에 공유한다.
        float distanceBiasSquared = 0.0f;
        if (vfogLightingParams.lightDistanceBiasScale > 0.0f) {
            // 프록셀 최소 AABB 대각선 기반의 보수적인 바이어스로 광원 근처의 역제곱 감쇠 피크를 완화한다.
            // 로컬 fog 가 있으면 밀도 LOD 용 계산을 재사용하고, 없으면 여기서 지터와 무관한 슬라이스 경계로 계산한다.
            float froxelDiagonalSquared = media.froxelDiagonalSquared;
            [branch]
            if (cameraConstants.numLocalFogVolumes == 0) {
                froxelDiagonalSquared = VFogFroxelDiagonalSquared(centerRayXY, sample.sliceNearDepth, sample.sliceFarDepth);
            }
            // distanceBiasSquared 에 제곱이 필요하므로 scale 도 제곱한다.
            const float scale = vfogLightingParams.lightDistanceBiasScale;
            distanceBiasSquared = froxelDiagonalSquared * scale * scale;
        }
        [loop]
        do {
            AccumulateVFogLocalLight(lightRef.packedLightRef, sample.worldPos, wo, distanceBiasSquared, useShadows, shadowSampling, media);
        } while (NextShaderLight(iterator, lightRef));
    }
}

[RootSignature(VFOG_INJECT_ROOT_SIGNATURE)]
[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID) {
    const VFogParams vfogParams = cameraConstants.vfogParams;
    if (any(id.xy >= vfogParams.tileCount) || id.z >= vfogParams.sliceCount) {
        return;
    }

    const bool isOrthographic = cameraConstants.IsOrthogonal();

    // 프록셀의 중심 월드 위치를 구한다.
    const float2 centerUV = VFogTileCenterUV(id.xy, vfogParams);
    const float centerDepth = VFogSliceToDepth(id.z + 0.5f, vfogParams);
    const float2 centerRayXY = VFogGetRayXY(centerUV, cameraConstants);
    const float3 centerViewPos = float3(centerRayXY * (isOrthographic ? 1.0f : centerDepth), -centerDepth);
    const float3 centerWorldOffset = mul(centerViewPos, (float3x3)cameraConstants.viewRotation);
    const float3 centerWorldPos = cameraConstants.cameraOrigin + centerWorldOffset;

    const VFogFroxelSample sample = VFogGetFroxelSample(id);

    // 프록셀의 대각선 제곱 길이를 계산한다.
    float froxelDiagonalSquared = 0.0f;
    [branch]
    if (cameraConstants.numLocalFogVolumes > 0) {
        // 모든 볼륨과 광원에 같은 footprint 를 사용하며, STBN jitter 로 mip 이 흔들리지 않게 한다.
        froxelDiagonalSquared = VFogFroxelDiagonalSquared(centerRayXY, sample.sliceNearDepth, sample.sliceFarDepth);
    }

    // 프록셀의 매질 속성과 ambient/emissive source 를 초기화한다.
    VFogMedia media = VFogInitializeMedia(id, sample.worldPos, sample.rayXY, sample.depth, froxelDiagonalSquared, cameraConstants.heightFogParams);

    // 산란 가능한 매질이 있으면 환경광과 씬 광원의 산란 기여를 누적한다.
    [branch]
    if (VFogHasScattering(media)) {
        AccumulateVFogSceneLighting(sample, centerRayXY, media);
    }

    // 프록셀별 scattering + emissive source (RGB) 와 extinction 계수 (A) 를 temporal 누적해 저장한다.
    // 밝은 광원이나 겹치는 매질의 합이 FP16 의 최대 유한값 (65504) 을 넘지 않도록 제한한다.
    const float4 sourceExtinction = min(media.sourceExtinction, HALF_MAX);
    outputSourceTexture[id] = AccumulateVFogHistory(id, sourceExtinction, centerWorldPos, centerWorldOffset);
}
