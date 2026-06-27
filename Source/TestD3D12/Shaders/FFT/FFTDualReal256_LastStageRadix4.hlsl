#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// 256 개의 complex FFT 중간 결과가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 최종 FFT 결과는 두 벌의 A[0..255], B[0..255] 이지만 각각 켤레 대칭이므로,
// 최종 출력은 128 개의 packed complex 데이터 두 벌로 저장한다.
// output[base + 0].x = A[0], output[base + 0].y = A[128], output[base + 1..127] = A[1..127]
// output[base + 128].x = B[0], output[base + 128].y = B[128], output[base + 129..255] = B[1..127]
RWStructuredBuffer<float2> outputBuffer : register(u0);

// Forward dual-real FFT postprocess 는 complex FFT256 결과 Z[k] 와 Z[256 - k] 를 함께 사용한다.
groupshared float2 fftOutput[256];

void StorePackedSpectra(uint bufferBase, uint binIndex) {
    uint aSpectrumBase = bufferBase;
    uint bSpectrumBase = bufferBase + 128;

    // A[0], B[0] 각각 실수부에는 DC 항을, 허수부에는 Nyquist 항을 담는다.
    if (binIndex == 0) {
        outputBuffer[aSpectrumBase] = float2(fftOutput[0].x, fftOutput[128].x);
        outputBuffer[bSpectrumBase] = float2(fftOutput[0].y, fftOutput[128].y);
        return;
    }

    // 0 < k < 128
    // Z[k] 와 Z[N-k] 로부터 A[k] 와 B[k] 를 언패킹
    // Z[k] = A[k] + iB[k]
    // A[k] =      (Z[k] + conj(Z[N - k])) / 2
    // B[k] = -i * (Z[k] - conj(Z[N - k])) / 2
    float2 Zk = fftOutput[binIndex];
    float2 ZMirror = fftOutput[256 - binIndex];
    float2 conjZMirror = ComplexConj(ZMirror);
    float2 Ak = 0.5 * (Zk + conjZMirror);
    float2 diff = Zk - conjZMirror;
    float2 Bk = 0.5 * float2(diff.y, -diff.x);

    outputBuffer[aSpectrumBase + binIndex] = Ak;
    outputBuffer[bSpectrumBase + binIndex] = Bk;
}

// Forward DualReal256 마지막 스테이지. complex FFT256 radix-4 stage 3 을 수행한 뒤 두 packed real spectrum 으로 분리한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix4_256BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 256-point dual-real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix4_256ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix4_256ButterflyCount;

    // 현재 dual-real FFT256 블록에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex * 256;

    // complex FFT256 radix-4 마지막 스테이지는 span 256, inputStride 1 로 고정된다.
    uint inputIndex0 = bufferBase + (butterflyIndex << 2u);
    uint inputIndex1 = inputIndex0 + 1;
    uint inputIndex2 = inputIndex1 + 1;
    uint inputIndex3 = inputIndex2 + 1;

    uint outputIndex0 = butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix4_256ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix4_256ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix4_256ButterflyCount;

    // twiddle factor W, W^2, W^3 계산
    float angle = -TWO_PI * float(butterflyIndex) / float(256);
    float sinAngle;
    float cosAngle;
    sincos(angle, sinAngle, cosAngle);

    float2 twiddle1 = float2(cosAngle, sinAngle);
    float2 twiddle2 = ComplexMul(twiddle1, twiddle1);
    float2 twiddle3 = ComplexMul(twiddle2, twiddle1);

    // complex FFT256 radix-4 마지막 스테이지
    float2 a0 = inputBuffer[inputIndex0];
    float2 a1 = ComplexMul(twiddle1, inputBuffer[inputIndex1]);
    float2 a2 = ComplexMul(twiddle2, inputBuffer[inputIndex2]);
    float2 a3 = ComplexMul(twiddle3, inputBuffer[inputIndex3]);

    // radix-4 버터플라이 연산
    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, -1.0, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    // 모든 Z[k] 가 shared memory 에 기록되어야 각 thread 가 Z[256 - k] 를 안전하게 읽을 수 있다.
    fftOutput[outputIndex0] = stageOutput0;
    fftOutput[outputIndex1] = stageOutput1;
    fftOutput[outputIndex2] = stageOutput2;
    fftOutput[outputIndex3] = stageOutput3;

    GroupMemoryBarrierWithGroupSync();

    // packed spectrum 은 k = 0..127 만 저장한다. k = 128 은 output[0].y 에 들어간다.
    StorePackedSpectra(bufferBase, outputIndex0);
    StorePackedSpectra(bufferBase, outputIndex1);
}
