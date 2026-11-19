#ifndef __HEIGHT_FOG_COMMON_HLSLI__
#define __HEIGHT_FOG_COMMON_HLSLI__

#include "../ShaderInterop.h"
#include "FogMedium.hlsli"
#include "FogEnvironment.hlsli"

// 기준 높이에서 1 인 상대 밀도는 월드 공간 높이에 따라 지수적으로 변한다.
float HeightFogSampleDensity(float height, HeightFogParams hfogParams) {
    [branch]
    if (hfogParams.heightFalloff > 0.0f) {
        // density = 2^{-falloff * (h - h0)}
        const float exponent = clamp(-hfogParams.heightFalloff * (height - hfogParams.baseHeight), -80.0f, 80.0f);
        return exp2(exponent);
    }
    return 1.0f;
}

struct HeightFogRay {
    float originHeight;         // 월드 공간에서 ray 의 시작점 높이
    float heightPerViewDepth;   // 뷰 공간 깊이당 ray 의 높이 변화량
    float lengthPerViewDepth;   // 뷰 공간 깊이당 ray 의 길이 변화량
    float3 direction;           // ray 방향
};

// Raster UV includes jitter, matching the camera's projection coefficients.
HeightFogRay HeightFogGetRay(float2 rasterUV, CameraConstants camera) {
    const bool orthographic = camera.IsOrthogonal();
    const float2 rayXY = rasterUV * camera.uvToViewScaleBiasXY.xy + camera.uvToViewScaleBiasXY.zw;
    const float3 viewRay = float3(orthographic ? float2(0, 0) : rayXY, -1.0f);
    const float3 worldRay = mul(viewRay, (float3x3)camera.viewRotation);

    HeightFogRay ray;
    ray.originHeight = camera.cameraOrigin.z;
    if (orthographic) {
        ray.originHeight += mul(float3(rayXY, 0), (float3x3)camera.viewRotation).z;
    }
    ray.heightPerViewDepth = worldRay.z;
    ray.lengthPerViewDepth = length(viewRay);
    ray.direction = worldRay / ray.lengthPerViewDepth;
    return ray;
}

// 광선 구간의 상대 밀도를 적분한다.
float HeightFogIntegratedDensity(float startHeight, float heightDelta, float distance, HeightFogParams hfogParams) {
    if (distance <= 0.0f) {
        return 0.0f;
    }

    // view ray 구간의 시작점과 끝점 중 낮은 높이를 고른다.
    const float minHeight = startHeight + min(heightDelta, 0.0f);

    // view ray 구간의 낮은 높이에서의 exponential height fog 의 상대 밀도를 구한다.
    const float density = HeightFogSampleDensity(minHeight, hfogParams);

    // view ray 구간의 평균 상대 밀도를 구한다.
    // x 가 작으면 (1 - exp(-x)) / x 를 테일러 전개로 근사해 상쇄 오차와 0 나눗셈을 피한다.
    const float x = LN2 * hfogParams.heightFalloff * abs(heightDelta);
    const float average = x < 1.0e-3f ? 1.0f - x * (0.5f - x / 6.0f) : (1.0f - exp(-x)) / x;
    const float averageDensity = density * average;

    return averageDensity * distance;
}

// 뷰 공간 깊이 구간 [startDepth, endDepth] 에 대해 HeightFog 적분을 수행한다.
// RGB: 누적 산란광과 발광, A: 누적 투과율
float4 HeightFogIntegrate(HeightFogRay ray, float startDepth, float endDepth, CameraConstants camera, float3 directionalRadiance, float3 directionalDirection) {
    const HeightFogParams hfogParams = camera.heightFogParams;

    // 뷰 공간 깊이 구간 [startDepth, endDepth] 에 대해 상대 밀도를 적분한다.
    const float depthLength = max(endDepth - startDepth, 0.0f);
    const float startHeight = ray.originHeight + startDepth * ray.heightPerViewDepth;
    const float integratedDensity = HeightFogIntegratedDensity(startHeight, depthLength * ray.heightPerViewDepth, depthLength * ray.lengthPerViewDepth, hfogParams);

    // exp(-80) 이하의 투과율은 화면에 기여하지 않는다.
    const float opticalDepth = min(hfogParams.medium.baseExtinction * integratedDensity, 80.0f);

    // phase-weighted incident radiance 를 구한다. 광선 구간 전체에 같은 Q 를 적용한다.
    float3 incidentRadiance = hfogParams.ambientRadiance;
    [branch]
    if (opticalDepth > 0.0f && any(hfogParams.medium.scatteringAlbedo > 0.0f)) {
        incidentRadiance += directionalRadiance * HenyeyGreensteinPhase(dot(directionalDirection, -ray.direction), camera.fogAnisotropy); // Q

        [branch]
        if (camera.envLightRadianceSHBufferIndex != 0 && camera.envLightIntensity > 0.0f && any(camera.envLightColor > 0.0f)) {
            const FogEnvironmentSHBands environmentSHBands = FogSampleEnvironmentSHBands(-ray.direction, camera);
            incidentRadiance += FogEvaluateEnvironmentScatteringRadiance(environmentSHBands, camera.fogAnisotropy);
        }
    }

    // radiance_camera = alpha * Q * (1 - exp(-tau))
    const float scatteringIntegral = -Expm1(-opticalDepth);
    float3 radiance = hfogParams.medium.scatteringAlbedo * incidentRadiance * scatteringIntegral;

    [branch]
    if (any(hfogParams.medium.baseEmission > 0.0f)) {
        // 산란과 발광은 같은 밀도 분포를 사용한다.
        const float emissionIntegral = opticalDepth < 1e-3f ? integratedDensity * (1.0f - opticalDepth * (0.5f - opticalDepth / 6.0f)) : scatteringIntegral / hfogParams.medium.baseExtinction;
        radiance += hfogParams.medium.baseEmission * emissionIntegral;
    }
    return float4(radiance, exp(-opticalDepth));
}

#endif // __HEIGHT_FOG_COMMON_HLSLI__
