#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0))"

// 512 개의 packed complex (packed real forward FFT1024) FFT 최종 결과가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// W_1024^k = exp(i * 2PI * k / 1024) 를 저장한 twiddle factor 테이블
StructuredBuffer<float2> twiddleBuffer : register(t1);

// 1024 개의 real 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[512];
groupshared float2 fftStageBuffer1[512];

// packed X spectrum 으로부터 complex IFFT 입력 Z[0..511] 를 구한다.
float2 LoadComplexIFFTInput(uint bufferBase, uint binIndex) {
    // X[0] 에는 실수부에는 DC 항이, 허수부에는 Nyquist 항이 담겨있다.
    if (binIndex == 0) {
        float2 dcNyquist = inputBuffer[bufferBase];
        return 0.5 * float2(dcNyquist.x + dcNyquist.y, dcNyquist.x - dcNyquist.y);
    } else {
        // 0 < k < 512
        // X[k] 와 X[512 - k] 로부터 Z[k] 를 구한다.
        float2 Wk = twiddleBuffer[binIndex];
        return RealIFFTPreProcess(inputBuffer[bufferBase + binIndex], inputBuffer[bufferBase + 512u - binIndex], Wk);
    }
}

void RunRealIFFT1024Stage0_Buffer1(uint butterflyIndex, uint bufferBase) {
    uint binIndex0 = butterflyIndex;
    uint binIndex1 = binIndex0 + 64u;
    uint binIndex2 = binIndex1 + 64u;
    uint binIndex3 = binIndex2 + 64u;
    uint binIndex4 = binIndex3 + 64u;
    uint binIndex5 = binIndex4 + 64u;
    uint binIndex6 = binIndex5 + 64u;
    uint binIndex7 = binIndex6 + 64u;

    // packed spectrum X[k] 에서 complex FFT512 입력 Z[k] 를 복원한다.
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
    fftStageBuffer1[butterflyIndex + 64u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 128u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 192u] = stageOutput3;
    fftStageBuffer1[butterflyIndex + 256u] = stageOutput4;
    fftStageBuffer1[butterflyIndex + 320u] = stageOutput5;
    fftStageBuffer1[butterflyIndex + 384u] = stageOutput6;
    fftStageBuffer1[butterflyIndex + 448u] = stageOutput7;
}

void RunRadix8Stage1_Buffer1To0(uint butterflyIndex) {
    uint twiddleIndex = butterflyIndex >> 3u;
    uint laneIndex = butterflyIndex & 7u;
    uint inputIndex0 = laneIndex + (twiddleIndex << 6u);

    float2 w1;
    float2 w2;
    float2 w3;
    float2 w4;
    float2 w5;
    float2 w6;
    float2 w7;
    GetRadix8Twiddles(twiddleIndex, 1.0f / 64.0f, 1.0f, w1, w2, w3, w4, w5, w6, w7);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 8u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 16u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 24u]);
    float2 a4 = ComplexMul(w4, fftStageBuffer1[inputIndex0 + 32u]);
    float2 a5 = ComplexMul(w5, fftStageBuffer1[inputIndex0 + 40u]);
    float2 a6 = ComplexMul(w6, fftStageBuffer1[inputIndex0 + 48u]);
    float2 a7 = ComplexMul(w7, fftStageBuffer1[inputIndex0 + 56u]);

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
    fftStageBuffer0[butterflyIndex + 64u] = stageOutput1;
    fftStageBuffer0[butterflyIndex + 128u] = stageOutput2;
    fftStageBuffer0[butterflyIndex + 192u] = stageOutput3;
    fftStageBuffer0[butterflyIndex + 256u] = stageOutput4;
    fftStageBuffer0[butterflyIndex + 320u] = stageOutput5;
    fftStageBuffer0[butterflyIndex + 384u] = stageOutput6;
    fftStageBuffer0[butterflyIndex + 448u] = stageOutput7;
}

void RunRadix8Stage2_Buffer0ToOutput(uint butterflyIndex, uint outputBase) {
    uint inputIndex0 = butterflyIndex << 3u;

    float2 w1;
    float2 w2;
    float2 w3;
    float2 w4;
    float2 w5;
    float2 w6;
    float2 w7;
    GetRadix8Twiddles(butterflyIndex, 1.0f / 512.0f, 1.0f, w1, w2, w3, w4, w5, w6, w7);

    float2 a0 = fftStageBuffer0[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer0[inputIndex0 + 1u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer0[inputIndex0 + 2u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer0[inputIndex0 + 3u]);
    float2 a4 = ComplexMul(w4, fftStageBuffer0[inputIndex0 + 4u]);
    float2 a5 = ComplexMul(w5, fftStageBuffer0[inputIndex0 + 5u]);
    float2 a6 = ComplexMul(w6, fftStageBuffer0[inputIndex0 + 6u]);
    float2 a7 = ComplexMul(w7, fftStageBuffer0[inputIndex0 + 7u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    float2 stageOutput4;
    float2 stageOutput5;
    float2 stageOutput6;
    float2 stageOutput7;
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, 1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    // 역변환의 마지막 스테이지에서 정규화를 한다.
    stageOutput0 *= 1.0f / 512.0f;
    stageOutput1 *= 1.0f / 512.0f;
    stageOutput2 *= 1.0f / 512.0f;
    stageOutput3 *= 1.0f / 512.0f;
    stageOutput4 *= 1.0f / 512.0f;
    stageOutput5 *= 1.0f / 512.0f;
    stageOutput6 *= 1.0f / 512.0f;
    stageOutput7 *= 1.0f / 512.0f;

    uint outputIndex0 = butterflyIndex;
    uint outputIndex1 = outputIndex0 + 64u;
    uint outputIndex2 = outputIndex1 + 64u;
    uint outputIndex3 = outputIndex2 + 64u;
    uint outputIndex4 = outputIndex3 + 64u;
    uint outputIndex5 = outputIndex4 + 64u;
    uint outputIndex6 = outputIndex5 + 64u;
    uint outputIndex7 = outputIndex6 + 64u;

    uint realIndex0 = outputBase + (outputIndex0 << 1u);
    uint realIndex1 = outputBase + (outputIndex1 << 1u);
    uint realIndex2 = outputBase + (outputIndex2 << 1u);
    uint realIndex3 = outputBase + (outputIndex3 << 1u);
    uint realIndex4 = outputBase + (outputIndex4 << 1u);
    uint realIndex5 = outputBase + (outputIndex5 << 1u);
    uint realIndex6 = outputBase + (outputIndex6 << 1u);
    uint realIndex7 = outputBase + (outputIndex7 << 1u);

    // z[n] 을 x[2n], x[2n + 1] 로 unpack 한다.
    outputBuffer[realIndex0] = stageOutput0.x;      // Re(z[outputIndex0])
    outputBuffer[realIndex0 + 1u] = stageOutput0.y; // Im(z[outputIndex0])
    outputBuffer[realIndex1] = stageOutput1.x;      // Re(z[outputIndex1])
    outputBuffer[realIndex1 + 1u] = stageOutput1.y; // Im(z[outputIndex1])
    outputBuffer[realIndex2] = stageOutput2.x;      // Re(z[outputIndex2])
    outputBuffer[realIndex2 + 1u] = stageOutput2.y; // Im(z[outputIndex2])
    outputBuffer[realIndex3] = stageOutput3.x;      // Re(z[outputIndex3])
    outputBuffer[realIndex3 + 1u] = stageOutput3.y; // Im(z[outputIndex3])
    outputBuffer[realIndex4] = stageOutput4.x;      // Re(z[outputIndex4])
    outputBuffer[realIndex4 + 1u] = stageOutput4.y; // Im(z[outputIndex4])
    outputBuffer[realIndex5] = stageOutput5.x;      // Re(z[outputIndex5])
    outputBuffer[realIndex5 + 1u] = stageOutput5.y; // Im(z[outputIndex5])
    outputBuffer[realIndex6] = stageOutput6.x;      // Re(z[outputIndex6])
    outputBuffer[realIndex6 + 1u] = stageOutput6.y; // Im(z[outputIndex6])
    outputBuffer[realIndex7] = stageOutput7.x;      // Re(z[outputIndex7])
    outputBuffer[realIndex7 + 1u] = stageOutput7.y; // Im(z[outputIndex7])
}

// Inverse Real FFT1024. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_512BlockSize, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint bufferBase = groupId.x << 9;
    uint outputBase = groupId.x * 1024u;

    RunRealIFFT1024Stage0_Buffer1(butterflyIndex, bufferBase);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage2_Buffer0ToOutput(butterflyIndex, outputBase);
}
