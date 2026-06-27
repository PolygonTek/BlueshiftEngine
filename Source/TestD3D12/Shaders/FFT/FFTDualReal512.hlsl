#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// 512 개의 interleaved (512 x 2) 실수 입력 데이터가 연속으로 배치된 입력 버퍼.
StructuredBuffer<float2> inputBuffer : register(t0);

// 최종 FFT 결과는 두 벌의 A[0..511], B[0..511] 이지만 각각 켤레 대칭이므로,
// 최종 출력은 256 개의 packed complex 데이터 두 벌로 저장한다.
// output[base + 0].x = A[0], output[base + 0].y = A[256], output[base + 1..255] = A[1..255]
// output[base + 256].x = B[0], output[base + 256].y = B[256], output[base + 257..511] = B[1..255]
RWStructuredBuffer<float2> outputBuffer : register(u0);

groupshared float2 fftStageBuffer0[512];
groupshared float2 fftStageBuffer1[512];

void StorePackedSpectra(uint bufferBase, uint binIndex) {
    uint aSpectrumBase = bufferBase;
    uint bSpectrumBase = bufferBase + 256u;

    // A[0], B[0] 각각 실수부에는 DC 항을, 허수부에는 Nyquist 항을 담는다.
    if (binIndex == 0) {
        outputBuffer[aSpectrumBase] = float2(fftStageBuffer1[0].x, fftStageBuffer1[256].x);
        outputBuffer[bSpectrumBase] = float2(fftStageBuffer1[0].y, fftStageBuffer1[256].y);
    } else {
        // 0 < k < 256
        // Z[k] 와 Z[N-k] 로부터 A[k] 와 B[k] 를 언패킹
        // Z[k] = A[k] + iB[k]
        // A[k] =      (Z[k] + conj(Z[N - k])) / 2
        // B[k] = -i * (Z[k] - conj(Z[N - k])) / 2
        float2 Zk = fftStageBuffer1[binIndex];
        float2 ZMirror = fftStageBuffer1[512u - binIndex];
        float2 conjZMirror = ComplexConj(ZMirror);
        float2 Ak = 0.5 * (Zk + conjZMirror);
        float2 diff = Zk - conjZMirror;
        float2 Bk = 0.5 * float2(diff.y, -diff.x);

        outputBuffer[aSpectrumBase + binIndex] = Ak;
        outputBuffer[bSpectrumBase + binIndex] = Bk;
    }
}

void RunDualReal512Stage0_Buffer1(uint butterflyIndex, uint bufferBase) {
    uint inputIndex0 = bufferBase + butterflyIndex;
    uint inputIndex1 = inputIndex0 + 64u;
    uint inputIndex2 = inputIndex1 + 64u;
    uint inputIndex3 = inputIndex2 + 64u;
    uint inputIndex4 = inputIndex3 + 64u;
    uint inputIndex5 = inputIndex4 + 64u;
    uint inputIndex6 = inputIndex5 + 64u;
    uint inputIndex7 = inputIndex6 + 64u;

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
    GetRadix8Twiddles(twiddleIndex, 1.0f / 64.0f, -1.0f, w1, w2, w3, w4, w5, w6, w7);

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
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    fftStageBuffer0[butterflyIndex] = stageOutput0;
    fftStageBuffer0[butterflyIndex + 64u] = stageOutput1;
    fftStageBuffer0[butterflyIndex + 128u] = stageOutput2;
    fftStageBuffer0[butterflyIndex + 192u] = stageOutput3;
    fftStageBuffer0[butterflyIndex + 256u] = stageOutput4;
    fftStageBuffer0[butterflyIndex + 320u] = stageOutput5;
    fftStageBuffer0[butterflyIndex + 384u] = stageOutput6;
    fftStageBuffer0[butterflyIndex + 448u] = stageOutput7;
}

void RunRadix8Stage2_Buffer0To1(uint butterflyIndex) {
    uint inputIndex0 = butterflyIndex << 3u;

    float2 w1;
    float2 w2;
    float2 w3;
    float2 w4;
    float2 w5;
    float2 w6;
    float2 w7;
    GetRadix8Twiddles(butterflyIndex, 1.0f / 512.0f, -1.0f, w1, w2, w3, w4, w5, w6, w7);

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
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, -1.0f, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    // 모든 Z[k] 가 shared memory 에 기록되어야 각 thread 가 Z[512 - k] 를 안전하게 읽을 수 있다.
    fftStageBuffer1[butterflyIndex] = stageOutput0;
    fftStageBuffer1[butterflyIndex + 64u] = stageOutput1;
    fftStageBuffer1[butterflyIndex + 128u] = stageOutput2;
    fftStageBuffer1[butterflyIndex + 192u] = stageOutput3;
    fftStageBuffer1[butterflyIndex + 256u] = stageOutput4;
    fftStageBuffer1[butterflyIndex + 320u] = stageOutput5;
    fftStageBuffer1[butterflyIndex + 384u] = stageOutput6;
    fftStageBuffer1[butterflyIndex + 448u] = stageOutput7;
}

// Forward DualReal512. All stages run in one dispatch.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_512BlockSize, 1, 1)]
void CSMain(uint3 groupThreadId : SV_GroupThreadID, uint3 groupId : SV_GroupID) {
    uint butterflyIndex = groupThreadId.x;
    uint bufferBase = groupId.x << 9;

    RunDualReal512Stage0_Buffer1(butterflyIndex, bufferBase);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage1_Buffer1To0(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    RunRadix8Stage2_Buffer0To1(butterflyIndex);
    GroupMemoryBarrierWithGroupSync();

    // packed spectrum 은 k = 0..255 만 저장한다. k = 256 은 output[0].y 에 들어간다.
    StorePackedSpectra(bufferBase, butterflyIndex);
    StorePackedSpectra(bufferBase, butterflyIndex + 64u);
    StorePackedSpectra(bufferBase, butterflyIndex + 128u);
    StorePackedSpectra(bufferBase, butterflyIndex + 192u);
}
