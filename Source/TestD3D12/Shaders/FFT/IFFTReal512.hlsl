#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0))"

// 256 개의 packed complex (packed real forward FFT512) FFT 최종 결과가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// W_512^k = exp(i * 2PI * k / 512) 를 저장한 twiddle factor 테이블
StructuredBuffer<float2> twiddleBuffer : register(t1);

// 512 개의 real 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[256];
groupshared float2 fftStageBuffer1[256];

// packed X spectrum 으로부터 complex IFFT 입력 Z[0..255] 를 구한다.
float2 LoadComplexIFFTInput(uint bufferBase, uint binIndex) {
    // X[0] 에는 실수부에는 DC 항이, 허수부에는 Nyquist 항이 담겨있다.
    if (binIndex == 0) {
        float2 dcNyquist = inputBuffer[bufferBase];
        return 0.5 * float2(dcNyquist.x + dcNyquist.y, dcNyquist.x - dcNyquist.y);
    } else {
        // 0 < k < 256
        // X[k] 와 X[256 - k] 로부터 Z[k] 를 구한다.
        float2 Wk = twiddleBuffer[binIndex];
        return RealIFFTPreProcess(inputBuffer[bufferBase + binIndex], inputBuffer[bufferBase + 256u - binIndex], Wk);
    }
}

void RunRealIFFT512Stage0_Buffer1(uint butterflyIndex, uint bufferBase) {
    uint binIndex0 = butterflyIndex;
    uint binIndex1 = binIndex0 + 64u;
    uint binIndex2 = binIndex1 + 64u;
    uint binIndex3 = binIndex2 + 64u;

    // packed spectrum X[k] 에서 complex FFT256 입력 Z[k] 를 복원한다.
    float2 z0 = LoadComplexIFFTInput(bufferBase, binIndex0);
    float2 z1 = LoadComplexIFFTInput(bufferBase, binIndex1);
    float2 z2 = LoadComplexIFFTInput(bufferBase, binIndex2);
    float2 z3 = LoadComplexIFFTInput(bufferBase, binIndex3);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(z0, z1, z2, z3, 1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

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
    GetRadix4Twiddles(twiddleIndex, 1.0f / 16.0f, 1.0f, w1, w2, w3);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 16u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 32u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 48u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, 1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

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
    GetRadix4Twiddles(twiddleIndex, 1.0f / 64.0f, 1.0f, w1, w2, w3);

    float2 a0 = fftStageBuffer0[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer0[inputIndex0 + 4u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer0[inputIndex0 + 8u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer0[inputIndex0 + 12u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, 1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    fftStageBuffer1[butterflyIndex] = stageOutput0;
    fftStageBuffer1[butterflyIndex + 64u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 128u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 192u] = stageOutput3;
}

void RunRadix4Stage3_Buffer1ToOutput(uint butterflyIndex, uint outputBase) {
    uint inputIndex0 = butterflyIndex << 2u;

    float2 w1;
    float2 w2;
    float2 w3;
    GetRadix4Twiddles(butterflyIndex, 1.0f / 256.0f, 1.0f, w1, w2, w3);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 1u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 2u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 3u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, 1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    // 역변환의 마지막 스테이지에서 정규화를 한다.
    stageOutput0 *= 1.0f / 256.0f;
    stageOutput1 *= 1.0f / 256.0f;
    stageOutput2 *= 1.0f / 256.0f;
    stageOutput3 *= 1.0f / 256.0f;

    uint outputIndex0 = butterflyIndex;
    uint outputIndex1 = outputIndex0 + 64u;
    uint outputIndex2 = outputIndex1 + 64u;
    uint outputIndex3 = outputIndex2 + 64u;

    uint realIndex0 = outputBase + (outputIndex0 << 1u);
    uint realIndex1 = outputBase + (outputIndex1 << 1u);
    uint realIndex2 = outputBase + (outputIndex2 << 1u);
    uint realIndex3 = outputBase + (outputIndex3 << 1u);

    // z[n] 을 x[2n], x[2n + 1] 로 unpack 한다.
    outputBuffer[realIndex0] = stageOutput0.x;      // Re(z[outputIndex0])
    outputBuffer[realIndex0 + 1u] = stageOutput0.y; // Im(z[outputIndex0])
    outputBuffer[realIndex1] = stageOutput1.x;      // Re(z[outputIndex1])
    outputBuffer[realIndex1 + 1u] = stageOutput1.y; // Im(z[outputIndex1])
    outputBuffer[realIndex2] = stageOutput2.x;      // Re(z[outputIndex2])
    outputBuffer[realIndex2 + 1u] = stageOutput2.y; // Im(z[outputIndex2])
    outputBuffer[realIndex3] = stageOutput3.x;      // Re(z[outputIndex3])
    outputBuffer[realIndex3 + 1u] = stageOutput3.y; // Im(z[outputIndex3])
}

// Inverse Real FFT512. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix4_256BlockSize, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint bufferBase = groupId.x << 8;
    uint outputBase = groupId.x * 512u;

    RunRealIFFT512Stage0_Buffer1(butterflyIndex, bufferBase);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage3_Buffer1ToOutput(butterflyIndex, outputBase);
}
