#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// 2048 개의 interleaved (2048 x 2) 실수 입력 데이터가 연속으로 배치된 입력 버퍼.
StructuredBuffer<float2> inputBuffer : register(t0);

// 최종 FFT 결과는 두 벌의 A[0..2047], B[0..2047] 이지만 각각 켤레 대칭이므로,
// 최종 출력은 1024 개의 packed complex 데이터 두 벌로 저장한다.
// output[base + 0].x = A[0], output[base + 0].y = A[1024], output[base + 1..1023] = A[1..1023]
// output[base + 1024].x = B[0], output[base + 1024].y = B[1024], output[base + 1025..2047] = B[1..1023]
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[2048];
groupshared float2 fftStageBuffer1[2048];

void StorePackedSpectra(uint bufferBase, uint binIndex) {
    uint aSpectrumBase = bufferBase;
    uint bSpectrumBase = bufferBase + 1024u;

    // A[0], B[0] 각각 실수부에는 DC 항을, 허수부에는 Nyquist 항을 담는다.
    if (binIndex == 0) {
        outputBuffer[aSpectrumBase] = float2(fftStageBuffer0[0].x, fftStageBuffer0[1024].x);
        outputBuffer[bSpectrumBase] = float2(fftStageBuffer0[0].y, fftStageBuffer0[1024].y);
    } else {
        // 0 < k < 1024
        // Z[k] 와 Z[N-k] 로부터 A[k] 와 B[k] 를 언패킹
        // Z[k] = A[k] + iB[k]
        // A[k] =      (Z[k] + conj(Z[N - k])) / 2
        // B[k] = -i * (Z[k] - conj(Z[N - k])) / 2
        float2 Zk = fftStageBuffer0[binIndex];
        float2 ZMirror = fftStageBuffer0[2048u - binIndex];
        float2 conjZMirror = ComplexConj(ZMirror);
        float2 Ak = 0.5 * (Zk + conjZMirror);
        float2 diff = Zk - conjZMirror;
        float2 Bk = 0.5 * float2(diff.y, -diff.x);

        outputBuffer[aSpectrumBase + binIndex] = Ak;
        outputBuffer[bSpectrumBase + binIndex] = Bk;
    }
}

void RunDualReal2048Stage0_Buffer1(uint butterflyIndex, uint bufferBase) {
    uint inputIndex0 = bufferBase + butterflyIndex;
    uint inputIndex1 = inputIndex0 + 256u;
    uint inputIndex2 = inputIndex1 + 256u;
    uint inputIndex3 = inputIndex2 + 256u;
    uint inputIndex4 = inputIndex3 + 256u;
    uint inputIndex5 = inputIndex4 + 256u;
    uint inputIndex6 = inputIndex5 + 256u;
    uint inputIndex7 = inputIndex6 + 256u;

    // z[n] = a[n] + i * b[n] 으로 pack 한다.
    float2 z0 = inputBuffer[inputIndex0];
    float2 z1 = inputBuffer[inputIndex1];
    float2 z2 = inputBuffer[inputIndex2];
    float2 z3 = inputBuffer[inputIndex3];
    float2 z4 = inputBuffer[inputIndex4];
    float2 z5 = inputBuffer[inputIndex5];
    float2 z6 = inputBuffer[inputIndex6];
    float2 z7 = inputBuffer[inputIndex7];

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
    GetRadix8Twiddles(twiddleIndex, 1.0f / 64.0f, -1.0f, w1, w2, w3, w4, w5, w6, w7);

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
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

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
    GetRadix8Twiddles(twiddleIndex, 1.0f / 512.0f, -1.0f, w1, w2, w3, w4, w5, w6, w7);

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
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    fftStageBuffer1[butterflyIndex] = stageOutput0;
    fftStageBuffer1[butterflyIndex + 256u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 512u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 768u] = stageOutput3;
    fftStageBuffer1[butterflyIndex + 1024u] = stageOutput4;
    fftStageBuffer1[butterflyIndex + 1280u] = stageOutput5;
    fftStageBuffer1[butterflyIndex + 1536u] = stageOutput6;
    fftStageBuffer1[butterflyIndex + 1792u] = stageOutput7;
}

void RunFinalRadix4ButterflyTo0(uint butterflyIndex) {
    uint inputIndex0 = butterflyIndex << 2u;

    float2 w1;
    float2 w2;
    float2 w3;
    GetRadix4Twiddles(butterflyIndex, 1.0f / 2048.0f, -1.0f, w1, w2, w3);

    float2 a0 = fftStageBuffer1[inputIndex0];
    float2 a1 = ComplexMul(w1, fftStageBuffer1[inputIndex0 + 1u]);
    float2 a2 = ComplexMul(w2, fftStageBuffer1[inputIndex0 + 2u]);
    float2 a3 = ComplexMul(w3, fftStageBuffer1[inputIndex0 + 3u]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    // 모든 Z[k] 가 shared memory 에 기록되어야 각 thread 가 Z[2048 - k] 를 안전하게 읽을 수 있다.
    fftStageBuffer0[butterflyIndex] = stageOutput0;
    fftStageBuffer0[butterflyIndex + 512u] = stageOutput1;
    fftStageBuffer0[butterflyIndex + 1024u] = stageOutput2;
    fftStageBuffer0[butterflyIndex + 1536u] = stageOutput3;
}

void RunRadix4Stage3_Buffer1To0(uint butterflyIndex) {
    RunFinalRadix4ButterflyTo0(butterflyIndex);
    RunFinalRadix4ButterflyTo0(butterflyIndex + 256u);
}

// Forward DualReal2048. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(256, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint bufferBase = groupId.x << 11;

    RunDualReal2048Stage0_Buffer1(butterflyIndex, bufferBase);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix4Stage3_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    // packed spectrum 은 k = 0..1023 만 저장한다. k = 1024 는 output[0].y 에 들어간다.
    StorePackedSpectra(bufferBase, butterflyIndex);
    StorePackedSpectra(bufferBase, butterflyIndex + 256u);
    StorePackedSpectra(bufferBase, butterflyIndex + 512u);
    StorePackedSpectra(bufferBase, butterflyIndex + 768u);
}
