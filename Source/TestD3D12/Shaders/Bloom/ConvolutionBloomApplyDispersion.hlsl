#include "../ShaderInterop.h"
#include "../Colors.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

static const uint ConvolutionBloomDispersionMaxSampleCount = 128;

SamplerState linearClampSampler : register(s100);

cbuffer BloomCB : register(b0) {
    BloomParams bloomParams;
}

Texture2D<float> diffractionPSFTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

float SampleDiffractionPSF(float2 uv) {
    if (any(uv < 0.0) || any(uv > 1.0)) {
        return 0.0;
    }
    return max(diffractionPSFTexture.SampleLevel(linearClampSampler, uv, 0), 0.0);
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(BloomBlockSizeX, BloomBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= bloomParams.outputSize.x || pixelCoord.y >= bloomParams.outputSize.y) {
        return;
    }

    float wavelengthRangeNm = BP_BLOOM_DISPERSION_MAX_WAVELENGTH_NM - BP_BLOOM_DISPERSION_MIN_WAVELENGTH_NM;
    uint sampleCount = min((uint)max(BP_BLOOM_DISPERSION_SAMPLE_COUNT + 0.5, 1.0), ConvolutionBloomDispersionMaxSampleCount);

    // 레퍼런스 파장을 기준으로 파장이 더 작으면 더 적게 퍼지고, 더 크면 더 많이 퍼지도록 한다.
    float referenceWavelengthNm = max(BP_BLOOM_DISPERSION_REFERENCE_WAVELENGTH_NM, 0.0001);
    float deltaWavelengthNm = wavelengthRangeNm / float(sampleCount);
    float2 outputUv = (float2(pixelCoord) + 0.5) * bloomParams.outputTexelSize;
    float2 centeredUv = outputUv - 0.5;

    float3 XYZ = float3(0.0, 0.0, 0.0);
    float3 whiteXYZ = float3(0.0, 0.0, 0.0);

    // 각 파장에 따른 회절 패턴을 radial 스케일해서 XYZ 에 누적한다.
    [loop]
    for (uint sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
        float wavelengthNm = BP_BLOOM_DISPERSION_MIN_WAVELENGTH_NM + (float(sampleIndex) + 0.5) * deltaWavelengthNm;
        float3 cmf = SampleCIE1931CMF(wavelengthNm) * deltaWavelengthNm; // 모든 파장에 대한 적분식이므로 deltaWavelengthNm 를 곱한다.

        // amount 0 은 분산이 없고, amount 1 은 wavelength/reference 비율을 그대로 사용한다.
        float wavelengthScale = wavelengthNm / referenceWavelengthNm;
        float dispersionScale = lerp(1.0, wavelengthScale, saturate(BP_BLOOM_DISPERSION_AMOUNT));
        float2 sourceUv = 0.5 + centeredUv / dispersionScale;

        // 2D radial scale 로 인해 변하는 면적을 보정해서 파장별 PSF 의 총 에너지를 일정하게 유지한다.
        float intensity = SampleDiffractionPSF(sourceUv) / (dispersionScale * dispersionScale);

        XYZ += cmf * intensity;
        whiteXYZ += cmf;
    }

    // 최종적으로 RGB 로 변환
    float3 rgb = XYZToLinearRGB(XYZ);

    // 누적 결과의 색상 치우침이 없도록 white normalization
    float3 whiteRgb = max(XYZToLinearRGB(whiteXYZ), float3(0.0001, 0.0001, 0.0001));
    rgb = rgb / whiteRgb * BP_BLOOM_DISPERSION_INTENSITY_SCALE;

    outputTexture[pixelCoord] = float4(max(rgb, 0.0), 1.0);
}
