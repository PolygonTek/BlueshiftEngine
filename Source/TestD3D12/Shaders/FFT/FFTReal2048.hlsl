#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0))"

// 2048 개의 실수 입력 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float> inputBuffer : register(t0);

// W_2048^k = exp(i * 2PI * k / 2048) 를 저장한 twiddle factor 테이블
StructuredBuffer<float2> twiddleBuffer : register(t1);

// 1024 개의 packed complex (packed real forward FFT2048) FFT 최종 결과가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[1024];
groupshared float2 fftStageBuffer1[1024];

float2 LoadPackedRealFFTOutput(uint binIndex) {
    // packed X[0] 의 실수부에는 DC 항을, 허수부에는 Nyquist 항을 담는다.
    if (binIndex == 0) {
        float2 Z0 = fftStageBuffer0[0];
        return float2(Z0.x + Z0.y, Z0.x - Z0.y);
    } else {
        // 0 < k < 1024
        // Z[k] 와 Z[1024 - k] 로 packed X[k] 를 구한다.
        float2 Wk = twiddleBuffer[binIndex];
        return RealFFTPostProcess(fftStageBuffer0[binIndex], fftStageBuffer0[1024u - binIndex], Wk);
    }
}

void RunRealFFT2048Stage0_Buffer1(uint butterflyIndex, uint inputBase) {
    uint packedIndex0 = butterflyIndex;
    uint packedIndex1 = packedIndex0 + 128u;
    uint packedIndex2 = packedIndex1 + 128u;
    uint packedIndex3 = packedIndex2 + 128u;
    uint packedIndex4 = packedIndex3 + 128u;
    uint packedIndex5 = packedIndex4 + 128u;
    uint packedIndex6 = packedIndex5 + 128u;
    uint packedIndex7 = packedIndex6 + 128u;

    uint realIndex0 = inputBase + (packedIndex0 << 1u);
    uint realIndex1 = inputBase + (packedIndex1 << 1u);
    uint realIndex2 = inputBase + (packedIndex2 << 1u);
    uint realIndex3 = inputBase + (packedIndex3 << 1u);
    uint realIndex4 = inputBase + (packedIndex4 << 1u);
    uint realIndex5 = inputBase + (packedIndex5 << 1u);
    uint realIndex6 = inputBase + (packedIndex6 << 1u);
    uint realIndex7 = inputBase + (packedIndex7 << 1u);

    // z[n] = x[2n] + i * x[2n + 1] 로 pack 한다.
    float2 z0 = float2(inputBuffer[realIndex0], inputBuffer[realIndex0 + 1u]);
    float2 z1 = float2(inputBuffer[realIndex1], inputBuffer[realIndex1 + 1u]);
    float2 z2 = float2(inputBuffer[realIndex2], inputBuffer[realIndex2 + 1u]);
    float2 z3 = float2(inputBuffer[realIndex3], inputBuffer[realIndex3 + 1u]);
    float2 z4 = float2(inputBuffer[realIndex4], inputBuffer[realIndex4 + 1u]);
    float2 z5 = float2(inputBuffer[realIndex5], inputBuffer[realIndex5 + 1u]);
    float2 z6 = float2(inputBuffer[realIndex6], inputBuffer[realIndex6 + 1u]);
    float2 z7 = float2(inputBuffer[realIndex7], inputBuffer[realIndex7 + 1u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    float2 stageOutput4;
    float2 stageOutput5;
    float2 stageOutput6;
    float2 stageOutput7;
    Radix8Butterfly(z0, z1, z2, z3, z4, z5, z6, z7, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    fftStageBuffer1[butterflyIndex] = stageOutput0;
    fftStageBuffer1[butterflyIndex + 128u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 256u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 384u] = stageOutput3;
    fftStageBuffer1[butterflyIndex + 512u] = stageOutput4;
    fftStageBuffer1[butterflyIndex + 640u] = stageOutput5;
    fftStageBuffer1[butterflyIndex + 768u] = stageOutput6;
    fftStageBuffer1[butterflyIndex + 896u] = stageOutput7;
}

void RunRadix8Stage1_Buffer1To0(uint butterflyIndex) {
    uint twiddleIndex = butterflyIndex >> 4u;
    uint laneIndex = butterflyIndex & 15u;
    uint inputIndex0 = laneIndex + (twiddleIndex << 7u);

    float2 w1;
    float2 w2;
    float2 w3;
    float2 w4;
    float2 w5;
    float2 w6;
    float2 w7;
    GetRadix8Twiddles(twiddleIndex, 1.0f / 64.0f, -1.0f, w1, w2, w3, w4, w5, w6, w7);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 16u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 32u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 48u]);
    float2 a4 = ComplexMul(w4, fftStageBuffer1[inputIndex0 + 64u]);
    float2 a5 = ComplexMul(w5, fftStageBuffer1[inputIndex0 + 80u]);
    float2 a6 = ComplexMul(w6, fftStageBuffer1[inputIndex0 + 96u]);
    float2 a7 = ComplexMul(w7, fftStageBuffer1[inputIndex0 + 112u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    float2 stageOutput4;
    float2 stageOutput5;
    float2 stageOutput6;
    float2 stageOutput7;
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    fftStageBuffer0[butterflyIndex] = stageOutput0;
    fftStageBuffer0[butterflyIndex + 128u] = stageOutput1;
    fftStageBuffer0[butterflyIndex + 256u] = stageOutput2;
    fftStageBuffer0[butterflyIndex + 384u] = stageOutput3;
    fftStageBuffer0[butterflyIndex + 512u] = stageOutput4;
    fftStageBuffer0[butterflyIndex + 640u] = stageOutput5;
    fftStageBuffer0[butterflyIndex + 768u] = stageOutput6;
    fftStageBuffer0[butterflyIndex + 896u] = stageOutput7;
}

void RunRadix8Stage2_Buffer0To1(uint butterflyIndex) {
    uint twiddleIndex = butterflyIndex >> 1u;
    uint laneIndex = butterflyIndex & 1u;
    uint inputIndex0 = laneIndex + (twiddleIndex << 4u);

    float2 w1;
    float2 w2;
    float2 w3;
    float2 w4;
    float2 w5;
    float2 w6;
    float2 w7;
    GetRadix8Twiddles(twiddleIndex, 1.0f / 512.0f, -1.0f, w1, w2, w3, w4, w5, w6, w7);

    float2 a0 = fftStageBuffer0[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer0[inputIndex0 + 2u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer0[inputIndex0 + 4u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer0[inputIndex0 + 6u]);
    float2 a4 = ComplexMul(w4, fftStageBuffer0[inputIndex0 + 8u]);
    float2 a5 = ComplexMul(w5, fftStageBuffer0[inputIndex0 + 10u]);
    float2 a6 = ComplexMul(w6, fftStageBuffer0[inputIndex0 + 12u]);
    float2 a7 = ComplexMul(w7, fftStageBuffer0[inputIndex0 + 14u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    float2 stageOutput4;
    float2 stageOutput5;
    float2 stageOutput6;
    float2 stageOutput7;
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    fftStageBuffer1[butterflyIndex] = stageOutput0;
    fftStageBuffer1[butterflyIndex + 128u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 256u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 384u] = stageOutput3;
    fftStageBuffer1[butterflyIndex + 512u] = stageOutput4;
    fftStageBuffer1[butterflyIndex + 640u] = stageOutput5;
    fftStageBuffer1[butterflyIndex + 768u] = stageOutput6;
    fftStageBuffer1[butterflyIndex + 896u] = stageOutput7;
}

void GetFinalRadix2Twiddles(uint k, out float2 wk, out float2 wk128, out float2 wk256, out float2 wk384) {
    float s;
    float c;
    sincos(-TWO_PI * float(k) / 1024.0f, s, c);

    wk = float2(c, s);

    float halfSqrt2 = 0.5f * SQRT2;
    wk128 = ComplexMul(wk, float2(halfSqrt2, -halfSqrt2));
    wk256 = ComplexMul(wk, float2(0.0f, -1.0f));
    wk384 = ComplexMul(wk, float2(-halfSqrt2, -halfSqrt2));
}

void RunFinalRadix2ButterflyTo0(uint butterflyIndex, float2 twiddle) {
    uint inputIndex0 = butterflyIndex << 1u;

    float2 a = fftStageBuffer1[inputIndex0];
    float2 b = ComplexMul(twiddle, fftStageBuffer1[inputIndex0 + 1u]);

    // 모든 Z[k] 가 shared memory 에 기록되어야 각 thread 가 Z[1024 - k] 를 안전하게 읽을 수 있다.
    fftStageBuffer0[butterflyIndex] = a + b;
    fftStageBuffer0[butterflyIndex + 512u] = a - b;
}

void RunRadix2Stage3_Buffer1To0(uint butterflyIndex) {
    float2 wk;
    float2 wk128;
    float2 wk256;
    float2 wk384;
    GetFinalRadix2Twiddles(butterflyIndex, wk, wk128, wk256, wk384);

    RunFinalRadix2ButterflyTo0(butterflyIndex, wk);
    RunFinalRadix2ButterflyTo0(butterflyIndex + 128u, wk128);
    RunFinalRadix2ButterflyTo0(butterflyIndex + 256u, wk256);
    RunFinalRadix2ButterflyTo0(butterflyIndex + 384u, wk384);
}

// Forward Real FFT2048. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_1024BlockSize, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint inputBase = groupId.x * 2048u;
    uint outputBase = groupId.x << 10;

    RunRealFFT2048Stage0_Buffer1(butterflyIndex, inputBase);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix2Stage3_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    // 최종 FFT 결과는 X[0..2047] 이지만 켤레 대칭이므로, 최종 출력은 1024 개의 packed complex 데이터로 저장한다.
    // 출력 데이터의 첫번째 원소는 실수부에 DC 항 (X[0]), 허수부에 Nyquist 항 (X[1024]) 이 들어가고, 나머지 복소수 X[1..1023] 는 순차적으로 들어간다.
    // packed X[0] = (X[0], X[1024]), packed X[1..1023] = X[1..1023]
    outputBuffer[outputBase + butterflyIndex] = LoadPackedRealFFTOutput(butterflyIndex);
    outputBuffer[outputBase + butterflyIndex + 128u] = LoadPackedRealFFTOutput(butterflyIndex + 128u);
    outputBuffer[outputBase + butterflyIndex + 256u] = LoadPackedRealFFTOutput(butterflyIndex + 256u);
    outputBuffer[outputBase + butterflyIndex + 384u] = LoadPackedRealFFTOutput(butterflyIndex + 384u);
    outputBuffer[outputBase + butterflyIndex + 512u] = LoadPackedRealFFTOutput(butterflyIndex + 512u);
    outputBuffer[outputBase + butterflyIndex + 640u] = LoadPackedRealFFTOutput(butterflyIndex + 640u);
    outputBuffer[outputBase + butterflyIndex + 768u] = LoadPackedRealFFTOutput(butterflyIndex + 768u);
    outputBuffer[outputBase + butterflyIndex + 896u] = LoadPackedRealFFTOutput(butterflyIndex + 896u);
}
