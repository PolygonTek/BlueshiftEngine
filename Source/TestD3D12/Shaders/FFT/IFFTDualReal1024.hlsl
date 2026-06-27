#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// Inverse DualReal1024 입력 spectrum 이 packed spectrum 두 벌로 저장된 입력 버퍼
// input[base + 0..511] = A spectrum, input[base + 512..1023] = B spectrum
StructuredBuffer<float2> inputBuffer : register(t0);

// 1024 개의 interleaved (1024 x 2) 실수 데이터가 연속으로 배치된 출력 버퍼.
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[1024];
groupshared float2 fftStageBuffer1[1024];

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

// packed A/B spectrum 으로부터 complex IFFT 입력 Z[0..1023] 를 구한다.
float2 LoadComplexIFFTInput(uint bufferBase, uint binIndex) {
    uint aSpectrumBase = bufferBase;
    uint bSpectrumBase = bufferBase + 512u;

    // Z[0], DC 성분은 A[0], B[0] 의 실수 성분에서 가져온다.
    if (binIndex == 0) {
        return float2(inputBuffer[aSpectrumBase].x, inputBuffer[bSpectrumBase].x);
    } else if (binIndex == 512u) {
        // Z[512], Nyquist 성분은 A[0], B[0] 의 허수 성분에서 가져온다.
        return float2(inputBuffer[aSpectrumBase].y, inputBuffer[bSpectrumBase].y);
    } else if (binIndex < 512u) {
        // 0 < k < 512
        // Z[k] = A[k] + i * B[k]
        float2 Ak = inputBuffer[aSpectrumBase + binIndex];
        float2 Bk = inputBuffer[bSpectrumBase + binIndex];
        return Ak + float2(-Bk.y, Bk.x);
    } else {
        // 512 < k < 1024
        // Z[k] = A[k] + i * B[k]
        //      = conj(A[1024 - k]) + i * conj(B[1024 - k])
        uint mirrorIndex = 1024u - binIndex;
        float2 ANk = inputBuffer[aSpectrumBase + mirrorIndex];
        float2 BNk = inputBuffer[bSpectrumBase + mirrorIndex];
        return float2(ANk.x + BNk.y, -ANk.y + BNk.x);
    }
}

void RunDualRealIFFT1024Stage0_Buffer1(uint butterflyIndex, uint bufferBase) {
    uint binIndex0 = butterflyIndex;
    uint binIndex1 = binIndex0 + 128u;
    uint binIndex2 = binIndex1 + 128u;
    uint binIndex3 = binIndex2 + 128u;
    uint binIndex4 = binIndex3 + 128u;
    uint binIndex5 = binIndex4 + 128u;
    uint binIndex6 = binIndex5 + 128u;
    uint binIndex7 = binIndex6 + 128u;

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

void RunFinalRadix2ButterflyToOutput(uint butterflyIndex, uint bufferBase, float2 twiddle) {
    uint inputIndex0 = butterflyIndex << 1u;

    float2 a = fftStageBuffer1[inputIndex0];
    float2 b = ComplexMul(twiddle, fftStageBuffer1[inputIndex0 + 1u]);

    outputBuffer[bufferBase + butterflyIndex] = (a + b) * (1.0f / 1024.0f);
    outputBuffer[bufferBase + butterflyIndex + 512u] = (a - b) * (1.0f / 1024.0f);
}

void RunRadix2Stage3_Buffer1ToOutput(uint butterflyIndex, uint bufferBase) {
    float2 wk;
    float2 wk128;
    float2 wk256;
    float2 wk384;
    GetFinalRadix2Twiddles(butterflyIndex, wk, wk128, wk256, wk384);

    RunFinalRadix2ButterflyToOutput(butterflyIndex, bufferBase, wk);
    RunFinalRadix2ButterflyToOutput(butterflyIndex + 128u, bufferBase, wk128);
    RunFinalRadix2ButterflyToOutput(butterflyIndex + 256u, bufferBase, wk256);
    RunFinalRadix2ButterflyToOutput(butterflyIndex + 384u, bufferBase, wk384);
}

// Inverse DualReal1024. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_1024BlockSize, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint bufferBase = groupId.x << 10;

    RunDualRealIFFT1024Stage0_Buffer1(butterflyIndex, bufferBase);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix2Stage3_Buffer1ToOutput(butterflyIndex, bufferBase);
}
