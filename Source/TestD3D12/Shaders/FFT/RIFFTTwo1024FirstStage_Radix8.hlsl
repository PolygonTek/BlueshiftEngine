#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// Inverse TwoReal1024 입력 spectrum 이 packed spectrum 두 벌로 저장된 입력 버퍼
// input[base + 0..511] = A spectrum, input[base + 512..1023] = B spectrum
StructuredBuffer<float2> inputBuffer : register(t0);

// 1024 개의 complex IFFT 중간 결과가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// packed A/B spectrum 으로부터 complex IFFT 입력 Z[0..1023] 를 구한다.
float2 LoadComplexIFFTInput(uint bufferBase, uint binIndex) {
    uint aSpectrumBase = bufferBase;
    uint bSpectrumBase = bufferBase + 512;

    // Z[0], DC 성분은 A[0], B[0] 의 실수 성분에서 가져온다.
    if (binIndex == 0) {
        return float2(inputBuffer[aSpectrumBase].x, inputBuffer[bSpectrumBase].x);
    }
    // Z[512], Nyquist 성분은 A[0], B[0] 의 허수 성분에서 가져온다.
    if (binIndex == 512) {
        return float2(inputBuffer[aSpectrumBase].y, inputBuffer[bSpectrumBase].y);
    }
    // 0 < k < 512
    // Z[k] = A[k] + i * B[k]
    if (binIndex < 512) {
        float2 Ak = inputBuffer[aSpectrumBase + binIndex];
        float2 Bk = inputBuffer[bSpectrumBase + binIndex];
        return Ak + float2(-Bk.y, Bk.x);
    }
    // 512 < k < 1024
    // Z[k] = A[k] + i * B[k]
    //      = conj(A[1024 - k]) + i * conj(B[1024 - k])
    uint mirrorIndex = 1024 - binIndex;
    float2 ANk = inputBuffer[aSpectrumBase + mirrorIndex];
    float2 BNk = inputBuffer[bSpectrumBase + mirrorIndex];
    return float2(ANk.x + BNk.y, -ANk.y + BNk.x);
}

// Inverse TwoReal1024 첫 stage. 두 packed spectrum 을 complex FFT1024 입력 Z[k] 로 합성한 뒤 radix-8 stage 0 을 수행한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_1024BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 1024-point two-real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix8_1024ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix8_1024ButterflyCount;

    // 현재 two-real FFT1024 블록에서 사용할 packed spectrum/complex 중간 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex * 1024;

    // complex FFT1024 radix-8 stage 0 의 여덟 입력 원소 사이의 간격 (stockham)
    uint inputStride = 1024 / 8;

    uint binIndex0 = butterflyIndex;
    uint binIndex1 = binIndex0 + inputStride;
    uint binIndex2 = binIndex1 + inputStride;
    uint binIndex3 = binIndex2 + inputStride;
    uint binIndex4 = binIndex3 + inputStride;
    uint binIndex5 = binIndex4 + inputStride;
    uint binIndex6 = binIndex5 + inputStride;
    uint binIndex7 = binIndex6 + inputStride;

    uint outputIndex0 = bufferBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix8_1024ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix8_1024ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix8_1024ButterflyCount;
    uint outputIndex4 = outputIndex3 + FFTRadix8_1024ButterflyCount;
    uint outputIndex5 = outputIndex4 + FFTRadix8_1024ButterflyCount;
    uint outputIndex6 = outputIndex5 + FFTRadix8_1024ButterflyCount;
    uint outputIndex7 = outputIndex6 + FFTRadix8_1024ButterflyCount;

    float2 Z0 = LoadComplexIFFTInput(bufferBase, binIndex0);
    float2 Z1 = LoadComplexIFFTInput(bufferBase, binIndex1);
    float2 Z2 = LoadComplexIFFTInput(bufferBase, binIndex2);
    float2 Z3 = LoadComplexIFFTInput(bufferBase, binIndex3);
    float2 Z4 = LoadComplexIFFTInput(bufferBase, binIndex4);
    float2 Z5 = LoadComplexIFFTInput(bufferBase, binIndex5);
    float2 Z6 = LoadComplexIFFTInput(bufferBase, binIndex6);
    float2 Z7 = LoadComplexIFFTInput(bufferBase, binIndex7);

    // radix-8 버터플라이 연산
    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    float2 stageOutput4;
    float2 stageOutput5;
    float2 stageOutput6;
    float2 stageOutput7;
    Radix8Butterfly(Z0, Z1, Z2, Z3, Z4, Z5, Z6, Z7, 1.0, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    outputBuffer[outputIndex0] = stageOutput0;
    outputBuffer[outputIndex1] = stageOutput1;
    outputBuffer[outputIndex2] = stageOutput2;
    outputBuffer[outputIndex3] = stageOutput3;
    outputBuffer[outputIndex4] = stageOutput4;
    outputBuffer[outputIndex5] = stageOutput5;
    outputBuffer[outputIndex6] = stageOutput6;
    outputBuffer[outputIndex7] = stageOutput7;
}
