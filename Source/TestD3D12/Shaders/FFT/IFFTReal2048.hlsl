#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0))"

// 1024 개의 packed complex (packed real forward FFT2048) FFT 최종 결과가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// W_2048^k = exp(i * 2PI * k / 2048) 를 저장한 twiddle factor 테이블
StructuredBuffer<float2> twiddleBuffer : register(t1);

// 2048 개의 real 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[1024];
groupshared float2 fftStageBuffer1[1024];

// packed X spectrum 으로부터 complex IFFT 입력 Z[0..1023] 를 구한다.
float2 LoadComplexIFFTInput(uint bufferBase, uint binIndex) {
    // X[0] 에는 실수부에는 DC 항이, 허수부에는 Nyquist 항이 담겨있다.
    if (binIndex == 0) {
        float2 dcNyquist = inputBuffer[bufferBase];
        return 0.5 * float2(dcNyquist.x + dcNyquist.y, dcNyquist.x - dcNyquist.y);
    } else {
        // 0 < k < 1024
        // X[k] 와 X[1024 - k] 로부터 Z[k] 를 구한다.
        float2 Wk = twiddleBuffer[binIndex];
        return RealIFFTPreProcess(inputBuffer[bufferBase + binIndex], inputBuffer[bufferBase + 1024u - binIndex], Wk);
    }
}

void RunRealIFFT2048Stage0_Buffer1(uint butterflyIndex, uint bufferBase) {
    uint binIndex0 = butterflyIndex;
    uint binIndex1 = binIndex0 + 128u;
    uint binIndex2 = binIndex1 + 128u;
    uint binIndex3 = binIndex2 + 128u;
    uint binIndex4 = binIndex3 + 128u;
    uint binIndex5 = binIndex4 + 128u;
    uint binIndex6 = binIndex5 + 128u;
    uint binIndex7 = binIndex6 + 128u;

    // packed spectrum X[k] 에서 complex FFT1024 입력 Z[k] 를 복원한다.
    float2 z0 = LoadComplexIFFTInput(bufferBase, binIndex0);
    float2 z1 = LoadComplexIFFTInput(bufferBase, binIndex1);
    float2 z2 = LoadComplexIFFTInput(bufferBase, binIndex2);
    float2 z3 = LoadComplexIFFTInput(bufferBase, binIndex3);
    float2 z4 = LoadComplexIFFTInput(bufferBase, binIndex4);
    float2 z5 = LoadComplexIFFTInput(bufferBase, binIndex5);
    float2 z6 = LoadComplexIFFTInput(bufferBase, binIndex6);
    float2 z7 = LoadComplexIFFTInput(bufferBase, binIndex7);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    float2 stageOutput4;
    float2 stageOutput5;
    float2 stageOutput6;
    float2 stageOutput7;
    Radix8Butterfly(z0, z1, z2, z3, z4, z5, z6, z7, 1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

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
    GetRadix8Twiddles(twiddleIndex, 1.0f / 64.0f, 1.0f, w1, w2, w3, w4, w5, w6, w7);

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
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, 1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

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
    GetRadix8Twiddles(twiddleIndex, 1.0f / 512.0f, 1.0f, w1, w2, w3, w4, w5, w6, w7);

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
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, 1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

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
    sincos(TWO_PI * float(k) / 1024.0f, s, c);

    wk = float2(c, s);

    float halfSqrt2 = 0.5f * SQRT2;
    wk128 = ComplexMul(wk, float2(halfSqrt2, halfSqrt2));
    wk256 = ComplexMul(wk, float2(0.0f, 1.0f));
    wk384 = ComplexMul(wk, float2(-halfSqrt2, halfSqrt2));
}

void StoreRealOutput(uint outputBase, uint outputIndex, float2 value) {
    uint realIndex = outputBase + (outputIndex << 1u);

    outputBuffer[realIndex] = value.x;
    outputBuffer[realIndex + 1u] = value.y;
}

void RunFinalRadix2ButterflyToOutput(uint butterflyIndex, uint outputBase, float2 twiddle) {
    uint inputIndex0 = butterflyIndex << 1u;

    float2 a = fftStageBuffer1[inputIndex0];
    float2 b = ComplexMul(twiddle, fftStageBuffer1[inputIndex0 + 1u]);

    // 역변환의 마지막 스테이지에서 정규화를 한다.
    StoreRealOutput(outputBase, butterflyIndex, (a + b) * (1.0f / 1024.0f));
    StoreRealOutput(outputBase, butterflyIndex + 512u, (a - b) * (1.0f / 1024.0f));
}

void RunRadix2Stage3_Buffer1ToOutput(uint butterflyIndex, uint outputBase) {
    float2 wk;
    float2 wk128;
    float2 wk256;
    float2 wk384;
    GetFinalRadix2Twiddles(butterflyIndex, wk, wk128, wk256, wk384);

    RunFinalRadix2ButterflyToOutput(butterflyIndex, outputBase, wk);
    RunFinalRadix2ButterflyToOutput(butterflyIndex + 128u, outputBase, wk128);
    RunFinalRadix2ButterflyToOutput(butterflyIndex + 256u, outputBase, wk256);
    RunFinalRadix2ButterflyToOutput(butterflyIndex + 384u, outputBase, wk384);
}

// Inverse Real FFT2048. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_1024BlockSize, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint bufferBase = groupId.x << 10;
    uint outputBase = groupId.x * 2048u;

    RunRealIFFT2048Stage0_Buffer1(butterflyIndex, bufferBase);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix2Stage3_Buffer1ToOutput(butterflyIndex, outputBase);
}
