#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer FFTStageCB : register(b0) {
    FFTStageParams fftParams;
}

// 2048 개의 복소수 FFT 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 2048 개의 복소수 FFT 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[2048];
groupshared float2 fftStageBuffer1[2048];

void RunRadix8Stage0_Buffer0To1(uint butterflyIndex) {
    float2 a0 = fftStageBuffer0[butterflyIndex];
    float2 a1 = fftStageBuffer0[butterflyIndex + 256u];
    float2 a2 = fftStageBuffer0[butterflyIndex + 512u];
    float2 a3 = fftStageBuffer0[butterflyIndex + 768u];
    float2 a4 = fftStageBuffer0[butterflyIndex + 1024u];
    float2 a5 = fftStageBuffer0[butterflyIndex + 1280u];
    float2 a6 = fftStageBuffer0[butterflyIndex + 1536u];
    float2 a7 = fftStageBuffer0[butterflyIndex + 1792u];

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
    fftStageBuffer1[butterflyIndex + 256u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 512u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 768u] = stageOutput3;
    fftStageBuffer1[butterflyIndex + 1024u] = stageOutput4;
    fftStageBuffer1[butterflyIndex + 1280u] = stageOutput5;
    fftStageBuffer1[butterflyIndex + 1536u] = stageOutput6;
    fftStageBuffer1[butterflyIndex + 1792u] = stageOutput7;
}

void RunRadix8Stage1_Buffer1To0(uint butterflyIndex) {
    uint twiddleIndex = butterflyIndex >> 5u;
    uint laneIndex = butterflyIndex & 31u;
    uint inputIndex0 = laneIndex + (twiddleIndex << 8u);

    float2 w1;
    float2 w2;
    float2 w3;
    float2 w4;
    float2 w5;
    float2 w6;
    float2 w7;
    GetRadix8Twiddles(twiddleIndex, 1.0f / 64.0f, fftParams.sign, w1, w2, w3, w4, w5, w6, w7);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 32u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 64u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 96u]);
    float2 a4 = ComplexMul(w4, fftStageBuffer1[inputIndex0 + 128u]);
    float2 a5 = ComplexMul(w5, fftStageBuffer1[inputIndex0 + 160u]);
    float2 a6 = ComplexMul(w6, fftStageBuffer1[inputIndex0 + 192u]);
    float2 a7 = ComplexMul(w7, fftStageBuffer1[inputIndex0 + 224u]);

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
    fftStageBuffer0[butterflyIndex + 256u] = stageOutput1;
    fftStageBuffer0[butterflyIndex + 512u] = stageOutput2;
    fftStageBuffer0[butterflyIndex + 768u] = stageOutput3;
    fftStageBuffer0[butterflyIndex + 1024u] = stageOutput4;
    fftStageBuffer0[butterflyIndex + 1280u] = stageOutput5;
    fftStageBuffer0[butterflyIndex + 1536u] = stageOutput6;
    fftStageBuffer0[butterflyIndex + 1792u] = stageOutput7;
}

void RunRadix8Stage2_Buffer0To1(uint butterflyIndex) {
    uint twiddleIndex = butterflyIndex >> 2u;
    uint laneIndex = butterflyIndex & 3u;
    uint inputIndex0 = laneIndex + (twiddleIndex << 5u);

    float2 w1;
    float2 w2;
    float2 w3;
    float2 w4;
    float2 w5;
    float2 w6;
    float2 w7;
    GetRadix8Twiddles(twiddleIndex, 1.0f / 512.0f, fftParams.sign, w1, w2, w3, w4, w5, w6, w7);

    float2 a0 = fftStageBuffer0[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer0[inputIndex0 + 4u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer0[inputIndex0 + 8u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer0[inputIndex0 + 12u]);
    float2 a4 = ComplexMul(w4, fftStageBuffer0[inputIndex0 + 16u]);
    float2 a5 = ComplexMul(w5, fftStageBuffer0[inputIndex0 + 20u]);
    float2 a6 = ComplexMul(w6, fftStageBuffer0[inputIndex0 + 24u]);
    float2 a7 = ComplexMul(w7, fftStageBuffer0[inputIndex0 + 28u]);

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
    fftStageBuffer1[butterflyIndex + 256u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 512u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 768u] = stageOutput3;
    fftStageBuffer1[butterflyIndex + 1024u] = stageOutput4;
    fftStageBuffer1[butterflyIndex + 1280u] = stageOutput5;
    fftStageBuffer1[butterflyIndex + 1536u] = stageOutput6;
    fftStageBuffer1[butterflyIndex + 1792u] = stageOutput7;
}

void RunFinalRadix4Butterfly(uint butterflyIndex, uint bufferBase) {
    uint inputIndex0 = butterflyIndex << 2u;

    float2 w1;
    float2 w2;
    float2 w3;
    GetRadix4Twiddles(butterflyIndex, 1.0f / 2048.0f, fftParams.sign, w1, w2, w3);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 1u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 2u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 3u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, fftParams.sign, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    // 역변환의 마지막 스테이지에서 정규화를 한다 (정변환은 scale = 1, 역변환은 scale = 1/2048)
    outputBuffer[bufferBase + butterflyIndex] = stageOutput0 * fftParams.scale;
    outputBuffer[bufferBase + butterflyIndex + 512u] = stageOutput1 * fftParams.scale;
    outputBuffer[bufferBase + butterflyIndex + 1024u] = stageOutput2 * fftParams.scale;
    outputBuffer[bufferBase + butterflyIndex + 1536u] = stageOutput3 * fftParams.scale;
}

void RunRadix4Stage3_Buffer1ToOutput(uint butterflyIndex, uint bufferBase) {
    RunFinalRadix4Butterfly(butterflyIndex, bufferBase);
    RunFinalRadix4Butterfly(butterflyIndex + 256u, bufferBase);
}

// Radix-8 Stockham autosort FFT2048 with a final radix-4 stage. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(256, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint bufferBase = groupId.x << 11;

    fftStageBuffer0[butterflyIndex] = inputBuffer[bufferBase + butterflyIndex];
    fftStageBuffer0[butterflyIndex + 256u] = inputBuffer[bufferBase + butterflyIndex + 256u];
    fftStageBuffer0[butterflyIndex + 512u] = inputBuffer[bufferBase + butterflyIndex + 512u];
    fftStageBuffer0[butterflyIndex + 768u] = inputBuffer[bufferBase + butterflyIndex + 768u];
    fftStageBuffer0[butterflyIndex + 1024u] = inputBuffer[bufferBase + butterflyIndex + 1024u];
    fftStageBuffer0[butterflyIndex + 1280u] = inputBuffer[bufferBase + butterflyIndex + 1280u];
    fftStageBuffer0[butterflyIndex + 1536u] = inputBuffer[bufferBase + butterflyIndex + 1536u];
    fftStageBuffer0[butterflyIndex + 1792u] = inputBuffer[bufferBase + butterflyIndex + 1792u];

    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage0_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage3_Buffer1ToOutput(butterflyIndex, bufferBase);
}
