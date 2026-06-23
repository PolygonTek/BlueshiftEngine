#ifndef _FFTCOMMON_HLSLI_
#define _FFTCOMMON_HLSLI_

#include "../MathConstants.hlsli"

float2 ComplexMul(float2 a, float2 b) {
    return float2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

float2 ComplexConj(float2 a) {
    return float2(a.x, -a.y);
}

void Radix4Butterfly(float2 a0, float2 a1, float2 a2, float2 a3, float sign,
    out float2 output0, out float2 output1, out float2 output2, out float2 output3) {
    float2 t0 = a0 + a2;
    float2 t1 = a0 - a2;
    float2 t2 = a1 + a3;
    float2 t3 = ComplexMul(float2(0.0, sign), a1 - a3);

    output0 = t0 + t2;
    output1 = t1 + t3;
    output2 = t0 - t2;
    output3 = t1 - t3;
}

void Radix8Butterfly(float2 a0, float2 a1, float2 a2, float2 a3, float2 a4, float2 a5, float2 a6, float2 a7, float sign,
    out float2 output0, out float2 output1, out float2 output2, out float2 output3,
    out float2 output4, out float2 output5, out float2 output6, out float2 output7) {
    float2 even0;
    float2 even1;
    float2 even2;
    float2 even3;
    Radix4Butterfly(a0, a2, a4, a6, sign, even0, even1, even2, even3);

    float2 odd0;
    float2 odd1;
    float2 odd2;
    float2 odd3;
    Radix4Butterfly(a1, a3, a5, a7, sign, odd0, odd1, odd2, odd3);

    float halfSqrt2 = 0.5 * SQRT2;
    float2 oddRot0 = odd0;
    float2 oddRot1 = ComplexMul(float2(halfSqrt2, sign * halfSqrt2), odd1);
    float2 oddRot2 = ComplexMul(float2(0.0, sign), odd2);
    float2 oddRot3 = ComplexMul(float2(-halfSqrt2, sign * halfSqrt2), odd3);

    output0 = even0 + oddRot0;
    output1 = even1 + oddRot1;
    output2 = even2 + oddRot2;
    output3 = even3 + oddRot3;
    output4 = even0 - oddRot0;
    output5 = even1 - oddRot1;
    output6 = even2 - oddRot2;
    output7 = even3 - oddRot3;
}

// Real FFT 포스트 프로세스 for 0 < k < N/2
//
// value: Z[k]
// mirrorValue: Z[N/2 - k]
// Wk: W_N^k
//
// X_e[k] =      (Z[k] + conj(Z[N/2 - k])) / 2
// X_o[k] = -i * (Z[k] - conj(Z[N/2 - k])) / 2
//
// X[k] = X_e[k] + conj(W_N^k) * X_o[k]
// W_N^k = exp(i * 2PI * k / N)
float2 RealFFTPostProcess(float2 value, float2 mirrorValue, float2 Wk) {
    float2 conjMirrorValue = ComplexConj(mirrorValue);
    float2 Xe = 0.5 * (value + conjMirrorValue);
    float2 diff = value - conjMirrorValue;
    float2 Xo = 0.5 * float2(diff.y, -diff.x);

    return Xe + ComplexMul(ComplexConj(Wk), Xo);
}

// Real IFFT 프리 프로세스 for 0 < k < N/2
//
// value: X[k]
// mirrorValue: X[N/2 - k]
// Wk: W_N^k
//
// X_e[k] =         (X[k] + conj(X[N/2 - k])) / 2
// X_o[k] = W_N^k * (X[k] - conj(X[N/2 - k])) / 2
//
// Z[k] = X_e[k] + i * X_o[k]
// W_N^k = exp(i * 2PI * k / N)
float2 RealIFFTPreProcess(float2 value, float2 mirrorValue, float2 Wk) {
    float2 conjMirrorValue = ComplexConj(mirrorValue);
    float2 X_e = 0.5 * (value + conjMirrorValue);
    float2 diff = 0.5 * (value - conjMirrorValue);

    float2 X_o = ComplexMul(Wk, diff);
    return X_e + float2(-X_o.y, X_o.x);
}

#endif
