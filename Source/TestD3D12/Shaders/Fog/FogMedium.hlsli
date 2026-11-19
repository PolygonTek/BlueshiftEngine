#ifndef __FOG_MEDIUM_HLSLI__
#define __FOG_MEDIUM_HLSLI__

#include "../Common.hlsli"
#include "FogInterop.h"

// Anisotropic Henyey-Greenstein phase function.
float HenyeyGreensteinPhase(float cosTheta, float g) {
    const float g2 = g * g;
    const float denom = 1.0f + g2 - 2.0f * g * clamp(cosTheta, -1.0f, 1.0f);
    return (1.0f - g2) / (4.0f * PI * denom * sqrt(denom));
}

// 특정 위치에서 샘플링한 하나의 매질 속성
struct FogMedium {
    float extinctionCoeff;
    float3 scatteringCoeff;
    float3 emission;
};

// 공간 배율과 기준 매질 파라미터로 매질 속성 (소멸계수, 산란계수, 단위 거리당 발광량) 을 계산한다.
FogMedium FogSampleMedium(float weight, MediumParams params) {
    FogMedium medium;
    medium.extinctionCoeff = min(weight * params.baseExtinction, HALF_MAX); // 공간 배율에 기준 소멸계수를 곱한다. 기존 FP16 범위를 유지한다.
    medium.scatteringCoeff = medium.extinctionCoeff * params.scatteringAlbedo;
    medium.emission = weight * params.baseEmission;
    return medium;
}

// ambient/emissive source 와 소멸계수를 누적한다.
// RGB += sigma_s * ambientRadiance + emission, A += sigma_t
void FogAccumulateAmbientEmissionAndExtinction(FogMedium medium, float3 ambientRadiance, inout float4 sourceExtinction) {
    float3 sourceTerm = medium.scatteringCoeff * ambientRadiance + medium.emission;
    sourceExtinction.rgb += sourceTerm;
    sourceExtinction.a += medium.extinctionCoeff;
}

#endif // __FOG_MEDIUM_HLSLI__
