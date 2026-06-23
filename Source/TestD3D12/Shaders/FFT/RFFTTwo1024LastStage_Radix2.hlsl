#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// 1024 개의 complex FFT 중간 결과가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 최종 FFT 결과는 두 벌의 A[0..1023], B[0..1023] 이지만 각각 켤레 대칭이므로,
// 최종 출력은 512 개의 packed complex 데이터 두 벌로 저장한다.
// output[base + 0].x = A[0], output[base + 0].y = A[512], output[base + 1..511] = A[1..511]
// output[base + 512].x = B[0], output[base + 512].y = B[512], output[base + 513..1023] = B[1..511]
RWStructuredBuffer<float2> outputBuffer : register(u0);

// Forward two-real FFT postprocess 는 complex FFT1024 결과 Z[k] 와 Z[1024 - k] 를 함께 사용한다.
groupshared float2 fftOutput[1024];

void StorePackedSpectra(uint bufferBase, uint binIndex) {
    uint aSpectrumBase = bufferBase;
    uint bSpectrumBase = bufferBase + 512;

    // A[0], B[0] 각각 실수부에는 DC 항을, 허수부에는 Nyquist 항을 담는다.
    if (binIndex == 0) {
        outputBuffer[aSpectrumBase] = float2(fftOutput[0].x, fftOutput[512].x);
        outputBuffer[bSpectrumBase] = float2(fftOutput[0].y, fftOutput[512].y);
        return;
    }

    // 0 < k < 512
    // Z[k] 와 Z[N-k] 로부터 A[k] 와 B[k] 를 언패킹
    // Z[k] = A[k] + iB[k]
    // A[k] =      (Z[k] + conj(Z[N - k])) / 2
    // B[k] = -i * (Z[k] - conj(Z[N - k])) / 2
    float2 Zk = fftOutput[binIndex];
    float2 ZMirror = fftOutput[1024 - binIndex];
    float2 conjZMirror = ComplexConj(ZMirror);
    float2 Ak = 0.5 * (Zk + conjZMirror);
    float2 diff = Zk - conjZMirror;
    float2 Bk = 0.5 * float2(diff.y, -diff.x);

    outputBuffer[aSpectrumBase + binIndex] = Ak;
    outputBuffer[bSpectrumBase + binIndex] = Bk;
}

// Forward TwoReal1024 마지막 스테이지. complex FFT1024 radix-2 마지막 stage 를 수행한 뒤 두 packed real spectrum 으로 분리한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix2_1024BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 1024-point two-real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix2_1024ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix2_1024ButterflyCount;

    // 현재 two-real FFT1024 블록에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex * 1024;

    // complex FFT1024 마지막 스테이지는 span 1024, inputStride 1 로 고정된다.
    uint inputIndex0 = bufferBase + (butterflyIndex << 1u);
    uint inputIndex1 = inputIndex0 + 1;

    uint outputIndex0 = butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix2_1024ButterflyCount;

    // twiddle factor W 계산
    float angle = -TWO_PI * float(butterflyIndex) / float(1024);
    float sinAngle;
    float cosAngle;
    sincos(angle, sinAngle, cosAngle);
    float2 twiddle = float2(cosAngle, sinAngle);

    // complex FFT1024 radix-2 마지막 스테이지
    float2 a = inputBuffer[inputIndex0];
    float2 b = ComplexMul(twiddle, inputBuffer[inputIndex1]);

    // 모든 Z[k] 가 shared memory 에 기록되어야 각 thread 가 Z[1024 - k] 를 안전하게 읽을 수 있다.
    fftOutput[outputIndex0] = a + b;
    fftOutput[outputIndex1] = a - b;

    GroupMemoryBarrierWithGroupSync();

    // packed spectrum 은 k = 0..511 만 저장한다. k = 512 는 output[0].y 에 들어간다.
    StorePackedSpectra(bufferBase, outputIndex0);
}
