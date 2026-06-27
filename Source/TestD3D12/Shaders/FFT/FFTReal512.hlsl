#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0))"

// 512 개의 실수 입력 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float> inputBuffer : register(t0);

// W_512^k = exp(i * 2PI * k / 512) 를 저장한 twiddle factor 테이블
StructuredBuffer<float2> twiddleBuffer : register(t1);

// 256 개의 packed complex (packed real forward FFT512) FFT 최종 결과가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[256];
groupshared float2 fftStageBuffer1[256];

float2 LoadPackedRealFFTOutput(uint binIndex) {
    // packed X[0] 의 실수부에는 DC 항을, 허수부에는 Nyquist 항을 담는다.
    if (binIndex == 0) {
        float2 Z0 = fftStageBuffer0[0];
        return float2(Z0.x + Z0.y, Z0.x - Z0.y);
    } else {
        // 0 < k < 256
        // Z[k] 와 Z[256 - k] 로 packed X[k] 를 구한다.
        float2 Wk = twiddleBuffer[binIndex];
        return RealFFTPostProcess(fftStageBuffer0[binIndex], fftStageBuffer0[256u - binIndex], Wk);
    }
}

void RunRealFFT512Stage0_Buffer1(uint butterflyIndex, uint inputBase) {
    uint packedIndex0 = butterflyIndex;
    uint packedIndex1 = packedIndex0 + 64u;
    uint packedIndex2 = packedIndex1 + 64u;
    uint packedIndex3 = packedIndex2 + 64u;

    uint realIndex0 = inputBase + (packedIndex0 << 1u);
    uint realIndex1 = inputBase + (packedIndex1 << 1u);
    uint realIndex2 = inputBase + (packedIndex2 << 1u);
    uint realIndex3 = inputBase + (packedIndex3 << 1u);

    // z[n] = x[2n] + i * x[2n + 1] 로 pack 한다.
    float2 z0 = float2(inputBuffer[realIndex0], inputBuffer[realIndex0 + 1u]);
    float2 z1 = float2(inputBuffer[realIndex1], inputBuffer[realIndex1 + 1u]);
    float2 z2 = float2(inputBuffer[realIndex2], inputBuffer[realIndex2 + 1u]);
    float2 z3 = float2(inputBuffer[realIndex3], inputBuffer[realIndex3 + 1u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(z0, z1, z2, z3, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

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
    GetRadix4Twiddles(twiddleIndex, 1.0f / 16.0f, -1.0f, w1, w2, w3);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 16u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 32u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 48u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

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
    GetRadix4Twiddles(twiddleIndex, 1.0f / 64.0f, -1.0f, w1, w2, w3);

    float2 a0 = fftStageBuffer0[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer0[inputIndex0 + 4u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer0[inputIndex0 + 8u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer0[inputIndex0 + 12u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    fftStageBuffer1[butterflyIndex] = stageOutput0;
    fftStageBuffer1[butterflyIndex + 64u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 128u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 192u] = stageOutput3;
}

void RunRadix4Stage3_Buffer1To0(uint butterflyIndex) {
    uint inputIndex0 = butterflyIndex << 2u;

    float2 w1;
    float2 w2;
    float2 w3;
    GetRadix4Twiddles(butterflyIndex, 1.0f / 256.0f, -1.0f, w1, w2, w3);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 1u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 2u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 3u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    // 모든 Z[k] 가 shared memory 에 기록되어야 각 thread 가 Z[256 - k] 를 안전하게 읽을 수 있다.
    fftStageBuffer0[butterflyIndex] = stageOutput0;
    fftStageBuffer0[butterflyIndex + 64u] = stageOutput1;
    fftStageBuffer0[butterflyIndex + 128u] = stageOutput2;
    fftStageBuffer0[butterflyIndex + 192u] = stageOutput3;
}

// Forward Real FFT512. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix4_256BlockSize, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint inputBase = groupId.x * 512u;
    uint outputBase = groupId.x << 8;

    RunRealFFT512Stage0_Buffer1(butterflyIndex, inputBase);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage3_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    // 최종 FFT 결과는 X[0..511] 이지만 켤레 대칭이므로, 최종 출력은 256 개의 packed complex 데이터로 저장한다.
    // 출력 데이터의 첫번째 원소는 실수부에 DC 항 (X[0]), 허수부에 Nyquist 항 (X[256]) 이 들어가고, 나머지 복소수 X[1..255] 는 순차적으로 들어간다.
    // packed X[0] = (X[0], X[256]), packed X[1..255] = X[1..255]
    outputBuffer[outputBase + butterflyIndex] = LoadPackedRealFFTOutput(butterflyIndex);
    outputBuffer[outputBase + butterflyIndex + 64u] = LoadPackedRealFFTOutput(butterflyIndex + 64u);
    outputBuffer[outputBase + butterflyIndex + 128u] = LoadPackedRealFFTOutput(butterflyIndex + 128u);
    outputBuffer[outputBase + butterflyIndex + 192u] = LoadPackedRealFFTOutput(butterflyIndex + 192u);
}
