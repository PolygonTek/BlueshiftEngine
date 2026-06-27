#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// Inverse DualReal256 입력 spectrum 이 packed spectrum 두 벌로 저장된 입력 버퍼
// input[base + 0..127] = A spectrum, input[base + 128..255] = B spectrum
StructuredBuffer<float2> inputBuffer : register(t0);

// 256 개의 complex IFFT 중간 결과가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// packed A/B spectrum 으로부터 complex IFFT 입력 Z[0..255] 를 구한다.
float2 LoadComplexIFFTInput(uint bufferBase, uint binIndex) {
    uint aSpectrumBase = bufferBase;
    uint bSpectrumBase = bufferBase + 128;

    // Z[0], DC 성분은 A[0], B[0] 의 실수 성분에서 가져온다.
    if (binIndex == 0) {
        return float2(inputBuffer[aSpectrumBase].x, inputBuffer[bSpectrumBase].x);
    }
    // Z[128], Nyquist 성분은 A[0], B[0] 의 허수 성분에서 가져온다.
    if (binIndex == 128) {
        return float2(inputBuffer[aSpectrumBase].y, inputBuffer[bSpectrumBase].y);
    }
    // 0 < k < 128
    // Z[k] = A[k] + i * B[k]
    if (binIndex < 128) {
        float2 Ak = inputBuffer[aSpectrumBase + binIndex];
        float2 Bk = inputBuffer[bSpectrumBase + binIndex];
        return Ak + float2(-Bk.y, Bk.x);
    }
    // 128 < k < 256
    // Z[k] = A[k] + i * B[k]
    //      = conj(A[256 - k]) + i * conj(B[256 - k])
    uint mirrorIndex = 256 - binIndex;
    float2 ANk = inputBuffer[aSpectrumBase + mirrorIndex];
    float2 BNk = inputBuffer[bSpectrumBase + mirrorIndex];
    return float2(ANk.x + BNk.y, -ANk.y + BNk.x);
}

// Inverse DualReal256 첫 stage. 두 packed spectrum 을 complex FFT256 입력 Z[k] 로 합성한 뒤 radix-4 stage 0 을 수행한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix4_256BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 256-point dual-real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix4_256ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix4_256ButterflyCount;

    // 현재 dual-real FFT256 블록에서 사용할 packed spectrum/complex 중간 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex * 256;

    // complex FFT256 radix-4 stage 0 의 네 입력 원소 사이의 간격 (stockham)
    uint inputStride = 256 / 4;

    uint binIndex0 = butterflyIndex;
    uint binIndex1 = binIndex0 + inputStride;
    uint binIndex2 = binIndex1 + inputStride;
    uint binIndex3 = binIndex2 + inputStride;

    uint outputIndex0 = bufferBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix4_256ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix4_256ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix4_256ButterflyCount;

    float2 Z0 = LoadComplexIFFTInput(bufferBase, binIndex0);
    float2 Z1 = LoadComplexIFFTInput(bufferBase, binIndex1);
    float2 Z2 = LoadComplexIFFTInput(bufferBase, binIndex2);
    float2 Z3 = LoadComplexIFFTInput(bufferBase, binIndex3);

    // radix-4 버터플라이 연산
    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(Z0, Z1, Z2, Z3, 1.0, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    outputBuffer[outputIndex0] = stageOutput0;
    outputBuffer[outputIndex1] = stageOutput1;
    outputBuffer[outputIndex2] = stageOutput2;
    outputBuffer[outputIndex3] = stageOutput3;
}
