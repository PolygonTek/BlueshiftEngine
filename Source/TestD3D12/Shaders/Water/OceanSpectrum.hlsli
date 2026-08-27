#ifndef __OCEAN_SPECTRUM_HLSLI__
#define __OCEAN_SPECTRUM_HLSLI__

#include "../Common.hlsli"

float2 GetWaveVector(uint2 pq, uint spectrumSize, float patchSize) {
    int2 tpq = int2(pq) - int(spectrumSize >> 1u);
    float waveNumberStep = TWO_PI / patchSize;
    return float2(tpq) * waveNumberStep;
}

// k, gravity, depth 로 (omega, d(omega)/dk) 를 반환한다.
float2 DispersionRelation(float waveNumber, float gravity, float depth) {
    float dimensionlessDepth = min(waveNumber * depth, 20.0f);
    float tanhDepth = tanh(dimensionlessDepth);
    float omega = sqrt(gravity * waveNumber * tanhDepth);
    float frequencyDerivative = 0.5f * gravity * (tanhDepth + dimensionlessDepth * (1.0f - tanhDepth * tanhDepth)) / omega;
    return float2(omega, frequencyDerivative);
}

float PhillipsSpectrum(float2 waveVector, float2 windDirection, float windSpeed, float gravity, float spectrumScale, float smallWaveDamping, float oppositeWaveDamping) {
    float waveNumberSquared = dot(waveVector, waveVector);
    if (waveNumberSquared <= 0.0f) {
        return 0.0f;
    }

    float inverseWaveNumberSquared = rcp(waveNumberSquared);
    float inverseWaveNumberFourth = inverseWaveNumberSquared * inverseWaveNumberSquared;
    float2 waveDirection = waveVector * rsqrt(waveNumberSquared);

    // windDirection 은 CPU 에서 normalize 해서 전달된다.
    float alignment = dot(waveDirection, windDirection);
    float windLength = windSpeed * windSpeed / gravity;
    float windLengthSquared = windLength * windLength;
    float dampingLengthSquared = smallWaveDamping * smallWaveDamping;

    float spectrum = spectrumScale;
    spectrum *= exp(-inverseWaveNumberSquared / windLengthSquared);
    spectrum *= alignment * alignment;
    spectrum *= inverseWaveNumberFourth;
    spectrum *= exp(-waveNumberSquared * dampingLengthSquared); // 짧은 파장 억제

    // Tessendorf 의 기본 Phillips spectrum 은 alignment^2 때문에 순풍/역풍 방향에 대해 대칭이다.
    // 아래는 modified Phillips spectrum 에서 사용하는 경험적 확장으로,
    // 역풍 방향의 파동만 oppositeWaveDamping 만큼 추가로 감쇠한다.
    // 1.0 이면 기본 Phillips spectrum 과 같고, 0.0 이면 역풍 방향을 완전히 제거한다.
    if (alignment < 0.0f) {
        spectrum *= saturate(oppositeWaveDamping);
    }

    return max(spectrum, 0.0f);
}

// 1차원 각주파수 스펙트럼 S(omega)
float JONSWAPSpectrum(float omega, float peakOmega, float alpha, float gravity) {
    if (omega <= 0.0f) {
        return 0.0f;
    }

    const float gamma = 3.3f;
    float sigma = omega <= peakOmega ? 0.07f : 0.09f;
    float normalizedPeakDistance = (omega - peakOmega) / (sigma * peakOmega);
    float peakEnhancement = exp(-0.5f * normalizedPeakDistance * normalizedPeakDistance);

    float peakRatio = peakOmega / omega;
    float peakRatioSquared = peakRatio * peakRatio;
    float peakRatioFourth = peakRatioSquared * peakRatioSquared;
    float omegaSquared = omega * omega;
    float omegaFourth = omegaSquared * omegaSquared;

    return alpha * gravity * gravity / (omegaFourth * omega) * exp(-1.25f * peakRatioFourth) * pow(gamma, peakEnhancement);
}

// TMA 는 JONSWAP 각주파수 스펙트럼에 유한 수심 보정을 적용한다.
float TMASpectrum(float omega, float peakOmega, float alpha, float gravity, float depth) {
    float dimensionlessFrequency = min(omega * sqrt(depth / gravity), 2.0f);
    float depthCorrection;

    if (dimensionlessFrequency <= 1.0f) {
        depthCorrection = 0.5f * dimensionlessFrequency * dimensionlessFrequency;
    } else {
        float distanceFromUpperLimit = 2.0f - dimensionlessFrequency;
        depthCorrection = 1.0f - 0.5f * distanceFromUpperLimit * distanceFromUpperLimit;
    }

    return JONSWAPSpectrum(omega, peakOmega, alpha, gravity) * depthCorrection;
}

float LonguetHigginsNormalization(float spreadingExponent) {
    if (spreadingExponent < 0.4f) {
        return INV_TWO_PI + spreadingExponent * (0.220636f + spreadingExponent * (-0.109f + spreadingExponent * 0.09f));
    }

    float sqrtExponent = sqrt(spreadingExponent);
    return rsqrt(PI) * (0.5f * sqrtExponent + 0.0625f / sqrtExponent);
}

// 방향 분포 D(omega, theta). cosTheta 는 파동 진행 방향과 바람 방향의 내적값이다.
float HasselmannDirectionalSpreadWithSwell(float omega, float peakOmega, float windSpeed, float gravity, float cosTheta, float swell) {
    if (omega <= 0.0f) {
        return INV_TWO_PI;
    }

    float frequencyRatio = omega / peakOmega;
    float spreadingExponent;

    if (omega <= peakOmega) {
        spreadingExponent = 6.97f * pow(frequencyRatio, 4.06f);
    } else {
        float windDependency = peakOmega * windSpeed / gravity - 1.17f;
        spreadingExponent = 9.77f * pow(frequencyRatio, -2.33f - 1.45f * windDependency);
    }

    float clampedSwell = saturate(swell);
    spreadingExponent += 16.0f * tanh(peakOmega / omega) * clampedSwell * clampedSwell;

    // cos^2(theta / 2) = (1 + cos(theta)) / 2 이므로 acos/cos 계산이 필요 없다.
    float halfAngleCosSquared = saturate(0.5f * (1.0f + clamp(cosTheta, -1.0f, 1.0f)));
    return LonguetHigginsNormalization(spreadingExponent) * pow(halfAngleCosSquared, spreadingExponent);
}

#endif // __OCEAN_SPECTRUM_HLSLI__
