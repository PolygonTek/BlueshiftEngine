#include "../ShaderInterop.h"
#include "../Random.hlsli"
#include "OceanSpectrum.hlsli"

#define OCEAN_SPECTRUM_MODEL_PHILLIPS 0
#define OCEAN_SPECTRUM_MODEL_TMA      1

#define OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_POSITIVE_COSINE_SQUARED 0
#define OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_HASSELMANN              1
#define OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_DONELAN_BANNER          2
#define OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_MITSUYASU               3

#ifndef OCEAN_SPECTRUM_MODEL
#define OCEAN_SPECTRUM_MODEL OCEAN_SPECTRUM_MODEL_PHILLIPS
#endif

#if OCEAN_SPECTRUM_MODEL != OCEAN_SPECTRUM_MODEL_PHILLIPS && OCEAN_SPECTRUM_MODEL != OCEAN_SPECTRUM_MODEL_TMA
#error Invalid OCEAN_SPECTRUM_MODEL.
#endif

#ifndef OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL
#define OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_HASSELMANN
#endif

#if OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL != OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_POSITIVE_COSINE_SQUARED && \
    OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL != OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_HASSELMANN && \
    OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL != OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_DONELAN_BANNER && \
    OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL != OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_MITSUYASU
#error Invalid OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL.
#endif

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(UAV(u0)), " \
    "CBV(b0)"

#if OCEAN_SPECTRUM_MODEL == OCEAN_SPECTRUM_MODEL_PHILLIPS
cbuffer OceanPhillipsInitialSpectrumCB : register(b0) {
    OceanPhillipsInitialSpectrumParams spectrumParams;
}
#elif OCEAN_SPECTRUM_MODEL == OCEAN_SPECTRUM_MODEL_TMA
cbuffer OceanTMAInitialSpectrumCB : register(b0) {
    OceanTMAInitialSpectrumParams spectrumParams;
}
#endif

// RG: h0(k), BA: conj(h0(-k))
RWTexture2DArray<float4> initialSpectrum : register(u0);

float ComputeSpectrumDensity(float2 waveVector) {
#if OCEAN_SPECTRUM_MODEL == OCEAN_SPECTRUM_MODEL_PHILLIPS
    return PhillipsSpectrum(waveVector, spectrumParams.windDirection, spectrumParams.windSpeed, spectrumParams.gravity, spectrumParams.spectrumScale, spectrumParams.smallWaveDamping, spectrumParams.oppositeWaveDamping);
#elif OCEAN_SPECTRUM_MODEL == OCEAN_SPECTRUM_MODEL_TMA
    float waveNumberSquared = dot(waveVector, waveVector);
    if (waveNumberSquared <= 0.0f) {
        return 0.0f;
    }

    float inverseWaveNumber = rsqrt(waveNumberSquared);
    float waveNumber = waveNumberSquared * inverseWaveNumber;
    float2 dispersion = DispersionRelation(waveNumber, spectrumParams.gravity, spectrumParams.depth);
    float2 waveDirection = waveVector * inverseWaveNumber;
    float cosTheta = dot(waveDirection, spectrumParams.windDirection);

    float spectrumDensity = spectrumParams.spectrumScale;
    spectrumDensity *= TMASpectrum(dispersion.x, spectrumParams.peakOmega, spectrumParams.alpha, spectrumParams.gravity, spectrumParams.depth);
#if OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL == OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_POSITIVE_COSINE_SQUARED
    float directionalSpread = PositiveCosineSquaredDirectionalSpread(cosTheta);
#elif OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL == OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_HASSELMANN
    float directionalSpread = HasselmannDirectionalSpreadWithSwell(dispersion.x, spectrumParams.peakOmega, spectrumParams.windSpeed, spectrumParams.gravity, cosTheta, spectrumParams.swell);
#elif OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL == OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_DONELAN_BANNER
    float directionalSpread = DonelanBannerDirectionalSpread(dispersion.x, spectrumParams.peakOmega, cosTheta);
#elif OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL == OCEAN_TMA_DIRECTIONAL_SPREAD_MODEL_MITSUYASU
    float directionalSpread = MitsuyasuDirectionalSpread(dispersion.x, spectrumParams.peakOmega, spectrumParams.windSpeed, spectrumParams.gravity, cosTheta);
#endif
    // spread = 0 은 선택한 방향 분포, 1 은 모든 방향이 동일한 등방 분포다.
    directionalSpread = lerp(directionalSpread, INV_TWO_PI, spectrumParams.spread);
    spectrumDensity *= directionalSpread;
    spectrumDensity *= dispersion.y * inverseWaveNumber;

    float dampingLengthSquared = spectrumParams.smallWaveDamping * spectrumParams.smallWaveDamping;
    spectrumDensity *= exp(-waveNumberSquared * dampingLengthSquared);
    return max(spectrumDensity, 0.0f);
#else
    return 0.0f;
#endif
}

// h̃0(-k) 를 생성하기 위해 -k 에 대응하는 배열 index (>= 0) 를 구한다.
// 파수 벡터 -k 는 DFT bin index 는 -p, -q 로 만들어진다.
// -p ≡ N-p (mod N), -q ≡ N-q (mod N).
uint2 GetNegatedBinIndex(uint2 pq, uint spectrumSize) {
    return uint2(
        pq.x == 0u ? 0u : spectrumSize - pq.x,
        pq.y == 0u ? 0u : spectrumSize - pq.y);
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(OceanInitialSpectrumBlockSizeX, OceanInitialSpectrumBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint spectrumWidth;
    uint spectrumHeight;
    uint spectrumSliceCount;
    initialSpectrum.GetDimensions(spectrumWidth, spectrumHeight, spectrumSliceCount);

    uint2 pq = dispatchThreadId.xy;
    uint cascadeIndex = dispatchThreadId.z;
    if (cascadeIndex >= min(spectrumSliceCount, OceanCascadeCount) || any(pq >= uint2(spectrumWidth, spectrumHeight))) {
        return;
    }

    OceanInitialSpectrumCascadeParams cascadeParams = spectrumParams.cascadeParams[cascadeIndex];
    const uint3 outputCoord = uint3(pq, cascadeIndex);

    float2 waveVector1 = GetWaveVector(pq, spectrumWidth, cascadeParams.patchSize);

    // cascade 가 담당하는 방사형 파수 대역 [minWaveNumber, maxWaveNumber) 밖의 성분을 제거한다.
    float waveNumberSquared = dot(waveVector1, waveVector1);
    float minWaveNumberSquared = cascadeParams.minWaveNumber * cascadeParams.minWaveNumber;
    float maxWaveNumberSquared = cascadeParams.maxWaveNumber * cascadeParams.maxWaveNumber;
    if (waveNumberSquared < minWaveNumberSquared || waveNumberSquared >= maxWaveNumberSquared) {
        initialSpectrum[outputCoord] = 0.0f;
        return;
    }

    uint2 npq = GetNegatedBinIndex(pq, spectrumWidth);
    float2 waveVector2 = GetWaveVector(npq, spectrumWidth, cascadeParams.patchSize);

    float2 xi1 = GaussianRandom(pq, cascadeParams.randomSeed);
    float2 xi2 = GaussianRandom(npq, cascadeParams.randomSeed);

    float p1 = ComputeSpectrumDensity(waveVector1);
    float p2 = ComputeSpectrumDensity(waveVector2);

    float deltaK = TWO_PI / cascadeParams.patchSize;
    float amplitudeScale = deltaK * INV_SQRT2;

    float2 h0 = xi1 * sqrt(p1) * amplitudeScale;
    float2 h0minusK = xi2 * sqrt(p2) * amplitudeScale;
    float2 h0minusKConj = float2(h0minusK.x, -h0minusK.y);
    initialSpectrum[outputCoord] = float4(h0, h0minusKConj);
}
