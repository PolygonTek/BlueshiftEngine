#include "VFogCommon.hlsli"
#include "HeightFogCommon.hlsli"
#include "../CommonBindless.hlsli"
#include "../STBN.hlsli"

#define VFOG_INJECT_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    "DescriptorTable(UAV(u0)), " \
    "CBV(b0), " \
    "CBV(b1), " \
    "RootConstants(num32BitConstants=4, b3), " \
    "CBV(b4), " \
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

cbuffer VFogTemporalCB : register(b4) {
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

// 현재 주입할 필드는 height fog 하나다. 새 필드는 산란계수와 anisotropy 를 보관하고
// 아래 샘플링, 산란 유무 검사, 공통 source 누적에 함께 추가한다.
struct VFogMedia {
    float3 heightFogScatteringCoeff;
    float heightFogAnisotropy;
    float4 sourceExtinction; // RGB: 누적 산란/발광 source, A: 합산 소멸계수
};

// 산란 가능한 매질이 있는지 확인한다.
bool VFogHasScattering(VFogMedia media) {
    return any(media.heightFogScatteringCoeff > 0.0f);
}

// 월드 위치에서 매질을 샘플링하고 ambient/emissive source 와 소멸계수를 한 번 합산한다.
// 밀도나 anisotropy 를 먼저 평균내지 않고 매질별 계수를 누적한다.
VFogMedia VFogInitializeMedia(float3 worldPosition, HeightFogParams heightFogParams) {
    VFogMedia media = (VFogMedia)0;

    // Height fog 의 밀도를 샘플링하고 매질 속성을 계산한다.
    const float density = HeightFogSampleDensity(worldPosition.z, heightFogParams);
    const FogMedium heightFogMedium = FogSampleMedium(density, heightFogParams.medium);

    // ambient/emissive source 와 소멸계수를 누적하고, 광원 산란에 필요한 속성을 보관한다.
    FogAccumulateAmbientEmissionAndExtinction(heightFogMedium, heightFogParams.ambientRadiance, media.sourceExtinction);
    media.heightFogScatteringCoeff = heightFogMedium.scatteringCoeff;
    media.heightFogAnisotropy = heightFogMedium.anisotropy;
    return media;
}

// 입사광의 산란 기여를 source 에 누적한다.
void VFogAccumulateScatteringSource(float3 incidentRadiance, float lightWeight, float cosTheta, inout VFogMedia media) {
    const float phase = HenyeyGreensteinPhase(cosTheta, media.heightFogAnisotropy);
    media.sourceExtinction.rgb += media.heightFogScatteringCoeff * incidentRadiance * (lightWeight * phase);
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

// Sample point/spot incident radiance once, then accumulate each medium's scattering source for outgoing direction wo.
void AccumulateVFogLocalLight(uint packedLightRef, float3 worldPosition, float3 wo, float distanceBiasSquared, bool useShadows, inout ShadowSamplingContext shadowSampling, inout VFogMedia media) {
    const uint lightType = packedLightRef & ShaderLightTypeMask;
    const uint lightIndex = packedLightRef >> ShaderLightTypeBitCount;
    float3 lightVector, lightIntensity;
    float inverseRadiusSquared;
    uint shadowIndexAndFlags;

    ShaderSpotLight spotLight = (ShaderSpotLight)0;
    if (lightType == ShaderLightTypePoint) {
        const ShaderPointLight pointLight = GetShaderPointLightTable(frameConstants.shaderPointLightTableIndex)[lightIndex];
        lightVector = worldPosition - pointLight.origin;
        lightIntensity = pointLight.intensityRGB;
        inverseRadiusSquared = pointLight.invRadiusSquared;
        shadowIndexAndFlags = pointLight.shadowIndexAndFlags;
    } else if (lightType == ShaderLightTypeSpot) {
        spotLight = GetShaderSpotLightTable(frameConstants.shaderSpotLightTableIndex)[lightIndex];
        lightVector = worldPosition - spotLight.origin;
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
        shadow = lightType == ShaderLightTypePoint ?
            SamplePointShadowVisibility(shadowIndex, shadowSampling, worldPosition) :
            SampleSpotShadowVisibility(shadowIndex, shadowSampling, worldPosition);
    }
    VFogAccumulateScatteringSource(lightIntensity, shadow * attenuation, dot(wi, wo), media);
}

// 현재 소멸계수에 맞춰 history 의 산란/발광 source 를 보정한 뒤 누적한다.
float4 BlendHistory(float4 currentSourceExtinction, float4 historySourceExtinction, float historyWeight) {
    if (currentSourceExtinction.a <= 0.0f || historySourceExtinction.a <= 0.0f) {
        // 소멸이 없는 자체 발광은 밀도 비율을 복원할 수 없으므로 현재 source 를 사용한다.
        return currentSourceExtinction;
    }

    const float extinctionRatio = currentSourceExtinction.a / historySourceExtinction.a;

    // 밀도 보정 후에도 FP16 저장 범위를 넘지 않도록 제한한다.
    const float3 historySource = min(historySourceExtinction.rgb * extinctionRatio, 65504.0f);
    const float3 source = lerp(currentSourceExtinction.rgb, historySource, historyWeight);

    return float4(source, currentSourceExtinction.a);
}

// 프록셀 history 를 재투영해 현재 밀도에 맞춘 산란/발광 source 를 temporal 누적한다.
float4 AccumulateVFogHistory(uint3 id, float4 currentSourceExtinction, float3 centerWorldPosition, float3 centerWorldOffset) {
    if (vfogTemporalParams.historyValid == 0 || vfogTemporalParams.historyWeight <= 0.0f) {
        return currentSourceExtinction;
    }

    const Texture3D<float4> historyTexture = GetTexture3D(vfogTemporalParams.historyTextureIndex);

    // 카메라가 정지해 있다면 같은 texel 의 history 를 직접 읽어 누적한다.
    if (cameraConstants.cameraUnchanged != 0) {
        const float4 historySourceExtinction = historyTexture.Load(int4(id, 0));
        return BlendHistory(currentSourceExtinction, historySourceExtinction, vfogTemporalParams.historyWeight);
    }

    const VFogParams vfogParams = cameraConstants.vfogParams;
    const float3 prevToPosition = centerWorldPosition - cameraConstants.prevCameraOrigin;
    const float prevDepth = dot(prevToPosition, cameraConstants.prevCameraForward);
    if (prevDepth < vfogParams.nearDepth || prevDepth > vfogParams.farDepth) {
        return currentSourceExtinction;
    }

    // 프록셀 중심의 월드 위치를 이전 프레임 카메라로 재투영하여 UVW 좌표를 구한다.
    const float4 position = float4(centerWorldPosition, 1.0f);
    const float prevW = dot(cameraConstants.prevUnjitteredViewProjMatrix[3], position);
    if (prevW <= 0.0f) {
        return currentSourceExtinction;
    }
    const float2 prevClipPosition = float2(dot(cameraConstants.prevUnjitteredViewProjMatrix[0], position), dot(cameraConstants.prevUnjitteredViewProjMatrix[1], position));
    const float2 prevNDC = prevClipPosition / prevW;
    const float2 historyUV = prevNDC * float2(0.5f, -0.5f) + 0.5f;
    const float3 historyUVW = float3(historyUV, VFogDepthToSlice(prevDepth, vfogParams) / vfogParams.sliceCount);

    // 재투영한 UVW 를 볼륨 텍스쳐의 반 텍셀 안쪽으로 범위로 제한한다.
    const float3 halfTexel = 0.5f * float3(vfogParams.tileCountRcp, 1.0f / vfogParams.sliceCount);
    if (any(historyUVW < halfTexel) || any(historyUVW > 1.0f - halfTexel)) {
        return currentSourceExtinction;
    }

    const bool isOrthographic = cameraConstants.IsOrthogonal();

    // 현재 프레임 카메라 위치에서의 프록셀 중심 시선 벡터
    const float3 wo = isOrthographic ? -cameraConstants.cameraForward : -normalize(centerWorldOffset);

    // 이전 프레임 카메라 위치에서의 프록셀 중심 시선 벡터
    const float3 prevWo = isOrthographic ? -cameraConstants.prevCameraForward : -normalize(prevToPosition);

    // cos(theta)^8 로 history weight 를 조정한다.
    // History RGB 에는 이전 카메라 위치로 향하는 산란광의 phase 가 반영되어 있어, 방향에 따른 밝기 변화는 재투영만으로 보정할 수 없다.
    // 원근 투영은 현재 프레임의 프록셀 중심에서 현재/이전 카메라 위치로 향하는 방향을 비교하므로, 제자리 회전만으로는 가중치를 줄이지 않는다.
    // 직교 투영은 현재/이전 프레임의 -cameraForward 를 비교한다.
    // 이 방향의 차이가 클수록 이전 밝기의 잔상을 줄이도록 history weight 를 낮춘다.
    float viewWeight = saturate(dot(wo, prevWo));
    viewWeight *= viewWeight;
    viewWeight *= viewWeight;
    viewWeight *= viewWeight;

    const float4 historySourceExtinction = historyTexture.SampleLevel(linearClampSampler, historyUVW, 0);
    return BlendHistory(currentSourceExtinction, historySourceExtinction, vfogTemporalParams.historyWeight * viewWeight);
}

// 조명 위치와 광원/그림자는 한 번 샘플링하고, 매질별 phase 와 산란계수를 적용해 source 를 누적한다.
void AccumulateVFogSceneLighting(uint3 id, float2 centerRayXY, inout VFogMedia media) {
    const VFogParams vfogParams = cameraConstants.vfogParams;
    const bool isOrthographic = cameraConstants.IsOrthogonal();

    // 프록셀 내부의 상대 샘플 위치. Jitter 를 끄면 기존 중앙 샘플링을 사용한다.
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
    const uint2 pixelCoord = uint2(sampleUV * cameraConstants.renderSize);

    // 지수 분포의 프록셀 경계는 유지하고, 프록셀 내부에서 실제 깊이에 대해 균등하게 샘플링한다.
    const float sliceNearDepth = VFogSliceToDepth(id.z, vfogParams);
    const float sliceFarDepth = VFogSliceToDepth(id.z + 1u, vfogParams);
    const float depth = lerp(sliceNearDepth, sliceFarDepth, sampleOffset.z);

    //  Perspective: 샘플 광선과 뷰 공간 z = -1 평면의 교점 XY
    // Orthographic: 샘플의 뷰 공간 XY 위치 (깊이에 무관)
    const float2 rayXY = VFogGetRayXY(sampleUV, cameraConstants);

    // 뷰 공간에서의 프록셀 샘플 위치
    const float3 viewPosition = float3(rayXY * (isOrthographic ? 1.0f : depth), -depth);

    // 월드 공간에서의 프록셀 샘플 위치
    const float3 worldPosition = cameraConstants.cameraOrigin + mul(viewPosition, (float3x3)cameraConstants.viewRotation);

    // 프록셀 샘플 위치에서 카메라 쪽으로 향하는 월드 공간 단위 방향
    const float3 wo = isOrthographic ? -cameraConstants.cameraForward : -mul(normalize(float3(rayXY, -1.0f)), (float3x3)cameraConstants.viewRotation);

    const bool useShadows = (vfogLightingParams.flags & VFogFlagShadows) != 0;
    ShadowSamplingContext shadowSampling = InitShadowSamplingContext(pixelCoord);

    // Directional 라이트의 산란 source 누적
    if ((cameraConstants.flags & ShaderCameraFlagUseDirectionalLight) != 0 && any(frameConstants.directionalLight.intensityRGB > 0.0f)) {
        const uint requiredShadowFlags = ShaderLightFlagCastShadows | ShaderLightFlagCastVolumetricShadows;
        const bool castVolumetricShadows = (frameConstants.directionalLight.flags & requiredShadowFlags) == requiredShadowFlags;
        const float shadow = useShadows && castVolumetricShadows ? SampleDirectionalShadowVisibility(shadowSampling, worldPosition) : 1.0f;
        VFogAccumulateScatteringSource(frameConstants.directionalLight.intensityRGB, shadow, dot(frameConstants.directionalLight.direction, wo), media);
    }

    // Point/Spot 라이트의 산란 source 누적
    const bool useZBin = (cameraConstants.flags & ShaderCameraFlagUseLightZBin) != 0;
    ShaderLightIterator iterator = BeginShaderLightIterationLinearDepth(pixelCoord, depth, useZBin, false);
    ShaderLightRef lightRef;
    if (NextShaderLight(iterator, lightRef)) {
        // distanceBiasSquared 를 로컬 광원이 있는 프록셀에서 한 번만 계산하고 모든 광원 계산에 공유한다.
        float distanceBiasSquared = 0.0f;
        if (vfogLightingParams.lightDistanceBiasScale > 0.0f) {
            // 프록셀 최소 AABB 대각선 기반의 보수적인 바이어스로 광원 근처의 역제곱 감쇠 피크를 완화한다.
            // 지터와 무관한 경계로 바이어스를 안정화하고, 기존 슬라이스 경계를 재사용해 추가 위치 재구성을 피한다.
            // distanceBiasSquared 에 제곱이 필요하므로 scale 도 제곱한다.
            const float scale = vfogLightingParams.lightDistanceBiasScale;
            distanceBiasSquared = VFogFroxelDiagonalSquared(centerRayXY, sliceNearDepth, sliceFarDepth) * scale * scale;
        }
        [loop]
        do {
            AccumulateVFogLocalLight(lightRef.packedLightRef, worldPosition, wo, distanceBiasSquared, useShadows, shadowSampling, media);
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

    // 프록셀 중심의 월드 위치를 계산한다.
    const float2 centerUV = VFogTileCenterUV(id.xy, vfogParams);
    const float centerDepth = VFogSliceToDepth(id.z + 0.5f, vfogParams);
    const float2 centerRayXY = VFogGetRayXY(centerUV, cameraConstants);
    const float3 centerViewPosition = float3(centerRayXY * (isOrthographic ? 1.0f : centerDepth), -centerDepth);
    const float3 centerWorldOffset = mul(centerViewPosition, (float3x3)cameraConstants.viewRotation);
    const float3 centerWorldPosition = cameraConstants.cameraOrigin + centerWorldOffset;

    // 밀도는 jitter 없는 프록셀 중심에서 샘플링하여 매질과 ambient source 를 안정적으로 유지한다.
    VFogMedia media = VFogInitializeMedia(centerWorldPosition, cameraConstants.heightFogParams);
    [branch]
    if (VFogHasScattering(media)) {
        AccumulateVFogSceneLighting(id, centerRayXY, media);
    }

    // 매질별 ambient/발광과 광원별 산란을 누적한 source 를 저장한다.
    // 각 프록셀에 대해 RGB = 산란/발광 source, A = extinction 계수를 저장한다.
    // 밝은 광원이나 겹치는 매질의 합이 FP16 의 최대 유한값 (65504) 을 넘지 않도록 제한한다.
    const float4 sourceExtinction = min(media.sourceExtinction, 65504.0f);
    outputSourceTexture[id] = AccumulateVFogHistory(id, sourceExtinction, centerWorldPosition, centerWorldOffset);
}
