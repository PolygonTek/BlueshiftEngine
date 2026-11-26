#ifndef __FOG_ENVIRONMENT_HLSLI__
#define __FOG_ENVIRONMENT_HLSLI__

#include "../BindlessResources.hlsli"

#define SH_BAND_COUNT 3
#include "../SphericalHarmonics/SphericalHarmonics.hlsli"
#undef SH_BAND_COUNT

struct FogEnvironmentSHBands {
    float3 band0;
    float3 band1;
    float3 band2;
};

// 시선 방향 (-wo) 에서 환경광 SH 의 band 별 기여를 계산한다.
FogEnvironmentSHBands FogSampleEnvironmentSHBands(float3 wo, CameraConstants camera) {
    const float3 direction = mul(camera.envLightAxis, float4(-wo, 0.0f));
    float basis[SHCoefficientCount];
    EvaluateSHBasis(direction, basis);

    const Buffer<float4> coefficients = GetBufferFloat4(camera.envLightRadianceSHBufferIndex);
    const float3 radianceScale = camera.envLightColor * camera.envLightIntensity;
    FogEnvironmentSHBands bands;
    bands.band0 = (coefficients[0].rgb * basis[0] * radianceScale);
    bands.band1 = (coefficients[1].rgb * basis[1] + coefficients[2].rgb * basis[2] + coefficients[3].rgb * basis[3]) * radianceScale;
    bands.band2 = (coefficients[4].rgb * basis[4] + coefficients[5].rgb * basis[5] + coefficients[6].rgb * basis[6] + coefficients[7].rgb * basis[7] + coefficients[8].rgb * basis[8]) * radianceScale;
    return bands;
}

// 공통 anisotropy 에 따른 Henyey-Greenstein 가중치로 SH band 를 합산해 환경광의 산란 radiance 를 구한다.
float3 FogEvaluateEnvironmentScatteringRadiance(FogEnvironmentSHBands bands, float anisotropy) {
    // 환경광 SH 와 HG phase 를 구면 적분하면 SH 의 직교성에 의해 band l 의 가중치는 g^l 이 된다.
    // 정규화된 HG 의 band 가중치는 (1, g, g^2).
    return max(bands.band0 + anisotropy * bands.band1 + (anisotropy * anisotropy) * bands.band2, 0.0f);
}

#endif // __FOG_ENVIRONMENT_HLSLI__
