#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer FFTStageCB : register(b0) {
    FFTStageParams fftParams;
}

// 1024 개의 복소수 FFT 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 1024 개의 복소수 FFT 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[1024];
groupshared float2 fftStageBuffer1[1024];

// twiddle factor W, W^2, ..., W^7 계산
void GetRadix8Twiddles(uint twiddleIndex, float invSpan,
    out float2 w1, out float2 w2, out float2 w3, out float2 w4,
    out float2 w5, out float2 w6, out float2 w7) {
    float s;
    float c;
    sincos(fftParams.sign * TWO_PI * float(twiddleIndex) * invSpan, s, c);

    w1 = float2(c, s);
    w2 = ComplexMul(w1, w1);
    w3 = ComplexMul(w2, w1);
    w4 = ComplexMul(w3, w1);
    w5 = ComplexMul(w4, w1);
    w6 = ComplexMul(w5, w1);
    w7 = ComplexMul(w6, w1);
}

void RunRadix8Stage0_Buffer0To1(uint butterflyIndex) {
    float2 a0 = fftStageBuffer0[butterflyIndex];
    float2 a1 = fftStageBuffer0[butterflyIndex + 128u];
    float2 a2 = fftStageBuffer0[butterflyIndex + 256u];
    float2 a3 = fftStageBuffer0[butterflyIndex + 384u];
    float2 a4 = fftStageBuffer0[butterflyIndex + 512u];
    float2 a5 = fftStageBuffer0[butterflyIndex + 640u];
    float2 a6 = fftStageBuffer0[butterflyIndex + 768u];
    float2 a7 = fftStageBuffer0[butterflyIndex + 896u];

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    float2 stageOutput4;
    float2 stageOutput5;
    float2 stageOutput6;
    float2 stageOutput7;
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, fftParams.sign, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

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
    GetRadix8Twiddles(twiddleIndex, 1.0f / 64.0f, w1, w2, w3, w4, w5, w6, w7);

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
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, fftParams.sign, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

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
    GetRadix8Twiddles(twiddleIndex, 1.0f / 512.0f, w1, w2, w3, w4, w5, w6, w7);

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
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, fftParams.sign, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

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
    sincos(fftParams.sign * TWO_PI * float(k) / 1024.0f, s, c);

    wk = float2(c, s);

    float halfSqrt2 = 0.5f * SQRT2;
    wk128 = ComplexMul(wk, float2(halfSqrt2, fftParams.sign * halfSqrt2));
    wk256 = ComplexMul(wk, float2(0.0f, fftParams.sign));
    wk384 = ComplexMul(wk, float2(-halfSqrt2, fftParams.sign * halfSqrt2));
}

void RunFinalRadix2Butterfly(uint butterflyIndex, uint bufferBase, float2 twiddle) {
    uint inputIndex0 = butterflyIndex << 1u;
    uint outputIndex0 = bufferBase + butterflyIndex;

    float2 a = fftStageBuffer1[inputIndex0];
    float2 b = ComplexMul(twiddle, fftStageBuffer1[inputIndex0 + 1u]);

    outputBuffer[outputIndex0] = (a + b) * fftParams.scale;
    outputBuffer[outputIndex0 + 512u] = (a - b) * fftParams.scale;
}

void RunRadix2Stage3_Buffer1ToOutput(uint butterflyIndex, uint bufferBase) {
    float2 wk;
    float2 wk128;
    float2 wk256;
    float2 wk384;
    GetFinalRadix2Twiddles(butterflyIndex, wk, wk128, wk256, wk384);

    RunFinalRadix2Butterfly(butterflyIndex, bufferBase, wk);
    RunFinalRadix2Butterfly(butterflyIndex + 128u, bufferBase, wk128);
    RunFinalRadix2Butterfly(butterflyIndex + 256u, bufferBase, wk256);
    RunFinalRadix2Butterfly(butterflyIndex + 384u, bufferBase, wk384);
}

// Radix-8 Stockham autosort FFT1024 with a final radix-2 stage. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_1024BlockSize, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint bufferBase = groupId.x << 10;

    fftStageBuffer0[butterflyIndex] = inputBuffer[bufferBase + butterflyIndex];
    fftStageBuffer0[butterflyIndex + 128u] = inputBuffer[bufferBase + butterflyIndex + 128u];
    fftStageBuffer0[butterflyIndex + 256u] = inputBuffer[bufferBase + butterflyIndex + 256u];
    fftStageBuffer0[butterflyIndex + 384u] = inputBuffer[bufferBase + butterflyIndex + 384u];
    fftStageBuffer0[butterflyIndex + 512u] = inputBuffer[bufferBase + butterflyIndex + 512u];
    fftStageBuffer0[butterflyIndex + 640u] = inputBuffer[bufferBase + butterflyIndex + 640u];
    fftStageBuffer0[butterflyIndex + 768u] = inputBuffer[bufferBase + butterflyIndex + 768u];
    fftStageBuffer0[butterflyIndex + 896u] = inputBuffer[bufferBase + butterflyIndex + 896u];

    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage0_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix2Stage3_Buffer1ToOutput(butterflyIndex, bufferBase);
}
