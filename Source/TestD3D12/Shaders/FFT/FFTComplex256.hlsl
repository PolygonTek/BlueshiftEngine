#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer FFTStageCB : register(b0) {
    FFTStageParams fftParams;
}

// 256 개의 복소수 FFT 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 256 개의 복소수 FFT 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[256];
groupshared float2 fftStageBuffer1[256];

void RunRadix4Stage0_Buffer0To1(uint butterflyIndex) {
    float2 a0 = fftStageBuffer0[butterflyIndex];
    float2 a1 = fftStageBuffer0[butterflyIndex + 64u];
    float2 a2 = fftStageBuffer0[butterflyIndex + 128u];
    float2 a3 = fftStageBuffer0[butterflyIndex + 192u];

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, fftParams.sign, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    fftStageBuffer1[butterflyIndex] = stageOutput0;
    fftStageBuffer1[butterflyIndex + 64u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 128u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 192u] = stageOutput3;
}

void RunRadix4Stage1_Buffer1To0(uint butterflyIndex) {
    uint twiddleIndex = butterflyIndex >> 4u;
    uint laneIndex = butterflyIndex & 15u;
    uint inputIndex0 = laneIndex + (twiddleIndex << 6u);

    float2 w1;
    float2 w2;
    float2 w3;
    GetRadix4Twiddles(twiddleIndex, 1.0f / 16.0f, fftParams.sign, w1, w2, w3);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 16u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 32u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 48u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, fftParams.sign, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    fftStageBuffer0[butterflyIndex] = stageOutput0;
    fftStageBuffer0[butterflyIndex + 64u] = stageOutput1;
    fftStageBuffer0[butterflyIndex + 128u] = stageOutput2;
    fftStageBuffer0[butterflyIndex + 192u] = stageOutput3;
}

void RunRadix4Stage2_Buffer0To1(uint butterflyIndex) {
    uint twiddleIndex = butterflyIndex >> 2u;
    uint laneIndex = butterflyIndex & 3u;
    uint inputIndex0 = laneIndex + (twiddleIndex << 4u);

    float2 w1;
    float2 w2;
    float2 w3;
    GetRadix4Twiddles(twiddleIndex, 1.0f / 64.0f, fftParams.sign, w1, w2, w3);

    float2 a0 = fftStageBuffer0[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer0[inputIndex0 + 4u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer0[inputIndex0 + 8u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer0[inputIndex0 + 12u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, fftParams.sign, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    fftStageBuffer1[butterflyIndex] = stageOutput0;
    fftStageBuffer1[butterflyIndex + 64u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 128u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 192u] = stageOutput3;
}

void RunRadix4Stage3_Buffer1ToOutput(uint butterflyIndex, uint bufferBase) {
    uint inputIndex0 = butterflyIndex << 2u;

    float2 w1;
    float2 w2;
    float2 w3;
    GetRadix4Twiddles(butterflyIndex, 1.0f / 256.0f, fftParams.sign, w1, w2, w3);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 1u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 2u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 3u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, fftParams.sign, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    // 역변환의 마지막 스테이지에서 정규화를 한다 (정변환은 scale = 1, 역변환은 scale = 1/256)
    outputBuffer[bufferBase + butterflyIndex] = stageOutput0 * fftParams.scale;
    outputBuffer[bufferBase + butterflyIndex + 64u] = stageOutput1 * fftParams.scale;
    outputBuffer[bufferBase + butterflyIndex + 128u] = stageOutput2 * fftParams.scale;
    outputBuffer[bufferBase + butterflyIndex + 192u] = stageOutput3 * fftParams.scale;
}

// Radix-4 Stockham autosort FFT256. All four stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix4_256BlockSize, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint bufferBase = groupId.x << 8;

    fftStageBuffer0[butterflyIndex] = inputBuffer[bufferBase + butterflyIndex];
    fftStageBuffer0[butterflyIndex + 64u] = inputBuffer[bufferBase + butterflyIndex + 64u];
    fftStageBuffer0[butterflyIndex + 128u] = inputBuffer[bufferBase + butterflyIndex + 128u];
    fftStageBuffer0[butterflyIndex + 192u] = inputBuffer[bufferBase + butterflyIndex + 192u];

    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage0_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage3_Buffer1ToOutput(butterflyIndex, bufferBase);
}
