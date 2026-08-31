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
float JONSWAPSpectrum(float omega, float peakOmega, float alpha, float gravity, float gamma) {
    if (omega <= 0.0f) {
        return 0.0f;
    }

    float sigma = omega <= peakOmega ? 0.07f : 0.09f;
    float normalizedPeakDistance = (omega - peakOmega) / (sigma * peakOmega);
    float peakEnhancementExponent = exp(-0.5f * normalizedPeakDistance * normalizedPeakDistance);

    float peakRatio = peakOmega / omega;
    float peakRatioSquared = peakRatio * peakRatio;
    float peakRatioFourth = peakRatioSquared * peakRatioSquared;
    float omegaSquared = omega * omega;
    float omegaFourth = omegaSquared * omegaSquared;

    return alpha * gravity * gravity / (omegaFourth * omega) * exp(-1.25f * peakRatioFourth) * pow(gamma, peakEnhancementExponent);
}

// TMA 는 JONSWAP 각주파수 스펙트럼에 유한 수심 보정을 적용한다.
float TMASpectrum(float omega, float peakOmega, float alpha, float gravity, float depth, float gamma) {
    float dimensionlessFrequency = min(omega * sqrt(depth / gravity), 2.0f);
    float depthCorrection;

    if (dimensionlessFrequency <= 1.0f) {
        depthCorrection = 0.5f * dimensionlessFrequency * dimensionlessFrequency;
    } else {
        float distanceFromUpperLimit = 2.0f - dimensionlessFrequency;
        depthCorrection = 1.0f - 0.5f * distanceFromUpperLimit * distanceFromUpperLimit;
    }

    return JONSWAPSpectrum(omega, peakOmega, alpha, gravity, gamma) * depthCorrection;
}

// 정방향 반구에만 에너지를 배분하는 주파수 비의존 cos^2 방향 분포. [-PI, PI] 에서 적분값은 1 이다.
float PositiveCosineSquaredDirectionalSpread(float cosTheta) {
    float positiveCosine = saturate(cosTheta);
    return 2.0f * INV_PI * positiveCosine * positiveCosine;
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

float HyperbolicSecantSquared(float x) {
    // sech^2(x) 를 exp(-2|x|) 형태로 계산하여 cosh 의 불필요한 큰 중간값을 피한다.
    float negativeDoubleExponent = exp(-2.0f * abs(x));
    float denominator = 1.0f + negativeDoubleExponent;
    return 4.0f * negativeDoubleExponent / (denominator * denominator);
}

// Donelan 의 sech^2 방향 분포에 Banner 의 고주파 확장식을 적용한다.
// cosTheta 는 파동 진행 방향과 바람 방향의 내적값이며, 결과는 [-PI, PI] 에서 적분값이 1 이 되도록 정규화한다.
float DonelanBannerDirectionalSpread(float omega, float peakOmega, float cosTheta) {
    if (omega <= 0.0f) {
        return INV_TWO_PI;
    }

    float frequencyRatio = omega / peakOmega;
    float beta;

    if (frequencyRatio < 0.56f) {
        beta = 1.24f;
    } else if (frequencyRatio < 0.95f) {
        beta = 2.61f * pow(frequencyRatio, 1.3f);
    } else if (frequencyRatio < 1.6f) {
        beta = 2.28f * pow(frequencyRatio, -1.3f);
    } else {
        float bannerExponent = -0.4f + 0.8393f * exp(-0.567f * log(frequencyRatio * frequencyRatio));
        beta = pow(10.0f, bannerExponent);
    }

    float theta = acos(clamp(cosTheta, -1.0f, 1.0f));
    float normalization = beta / (2.0f * tanh(beta * PI));
    return normalization * HyperbolicSecantSquared(beta * theta);
}

// Mitsuyasu 방향 분포. peakSpreadingExponent 는 심해 peak phase speed 를 사용한 inverse wave age 에 따라 결정한다.
float MitsuyasuDirectionalSpread(float omega, float peakOmega, float windSpeed, float gravity, float cosTheta) {
    if (omega <= 0.0f) {
        return INV_TWO_PI;
    }

    float frequencyRatio = omega / peakOmega;
    float inverseWaveAge = peakOmega * windSpeed / gravity;
    float peakSpreadingExponent = 11.5f * pow(inverseWaveAge, -2.5f);
    float spreadingExponent;

    if (omega <= peakOmega) {
        spreadingExponent = peakSpreadingExponent * pow(frequencyRatio, 5.0f);
    } else {
        spreadingExponent = peakSpreadingExponent * pow(frequencyRatio, -2.5f);
    }

    float halfAngleCosSquared = saturate(0.5f * (1.0f + clamp(cosTheta, -1.0f, 1.0f)));
    return LonguetHigginsNormalization(spreadingExponent) * pow(halfAngleCosSquared, spreadingExponent);
}

#endif // __OCEAN_SPECTRUM_HLSLI__
