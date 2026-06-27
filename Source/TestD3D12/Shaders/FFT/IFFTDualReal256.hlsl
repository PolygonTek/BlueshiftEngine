#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// Inverse DualReal256 입력 spectrum 이 packed spectrum 두 벌로 저장된 입력 버퍼
// input[base + 0..127] = A spectrum, input[base + 128..255] = B spectrum
StructuredBuffer<float2> inputBuffer : register(t0);

// 256 개의 interleaved (256 x 2) 실수 데이터가 연속으로 배치된 출력 버퍼.
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[256];
groupshared float2 fftStageBuffer1[256];

// packed A/B spectrum 으로부터 complex IFFT 입력 Z[0..255] 를 구한다.
float2 LoadComplexIFFTInput(uint bufferBase, uint binIndex) {
    uint aSpectrumBase = bufferBase;
    uint bSpectrumBase = bufferBase + 128u;

    // Z[0], DC 성분은 A[0], B[0] 의 실수 성분에서 가져온다.
    if (binIndex == 0) {
        return float2(inputBuffer[aSpectrumBase].x, inputBuffer[bSpectrumBase].x);
    // Z[128], Nyquist 성분은 A[0], B[0] 의 허수 성분에서 가져온다.
    } else if (binIndex == 128u) {
        return float2(inputBuffer[aSpectrumBase].y, inputBuffer[bSpectrumBase].y);
    // 0 < k < 128
    // Z[k] = A[k] + i * B[k]
    } else if (binIndex < 128u) {
        float2 Ak = inputBuffer[aSpectrumBase + binIndex];
        float2 Bk = inputBuffer[bSpectrumBase + binIndex];
        return Ak + float2(-Bk.y, Bk.x);
    } else {
        // 128 < k < 256
        // Z[k] = A[k] + i * B[k]
        //      = conj(A[256 - k]) + i * conj(B[256 - k])
        uint mirrorIndex = 256u - binIndex;
        float2 ANk = inputBuffer[aSpectrumBase + mirrorIndex];
        float2 BNk = inputBuffer[bSpectrumBase + mirrorIndex];
        return float2(ANk.x + BNk.y, -ANk.y + BNk.x);
    }
}

void RunDualRealIFFT256Stage0_Buffer1(uint butterflyIndex, uint bufferBase) {
    uint binIndex0 = butterflyIndex;
    uint binIndex1 = binIndex0 + 64u;
    uint binIndex2 = binIndex1 + 64u;
    uint binIndex3 = binIndex2 + 64u;

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

void RunRadix4Stage3_Buffer1ToOutput(uint butterflyIndex, uint bufferBase) {
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

    outputBuffer[bufferBase + butterflyIndex] = stageOutput0;
    outputBuffer[bufferBase + butterflyIndex + 64u] = stageOutput1;
    outputBuffer[bufferBase + butterflyIndex + 128u] = stageOutput2;
    outputBuffer[bufferBase + butterflyIndex + 192u] = stageOutput3;
}

// Inverse DualReal256. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix4_256BlockSize, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint bufferBase = groupId.x << 8;

    RunDualRealIFFT256Stage0_Buffer1(butterflyIndex, bufferBase);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage3_Buffer1ToOutput(butterflyIndex, bufferBase);
}
