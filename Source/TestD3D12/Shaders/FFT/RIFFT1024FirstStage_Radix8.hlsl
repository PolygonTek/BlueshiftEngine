#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0))"

// 512 개의 packed complex (packed real forward FFT1024) FFT 최종 결과가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// W_1024^k = exp(i * 2PI * k / 1024) 를 저장한 twiddle factor 테이블
StructuredBuffer<float2> twiddleBuffer : register(t1);

// 512 개의 complex IFFT 중간 결과가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// packed X spectrum 으로부터 complex IFFT 입력 Z[0..511] 를 구한다.
float2 LoadComplexIFFTInput(uint bufferBase, uint binIndex) {
    // X[0] 에는 실수부에는 DC 항이, 허수부에는 Nyquist 항이 담겨있다.
    if (binIndex == 0) {
        float2 dcNyquist = inputBuffer[bufferBase];
        return 0.5 * float2(dcNyquist.x + dcNyquist.y, dcNyquist.x - dcNyquist.y);
    }
    // 0 < k < 512
    // X[k] 와 X[512 - k] 로부터 Z[k] 를 구한다.
    float2 Wk = twiddleBuffer[binIndex];
    return RealIFFTPreProcess(inputBuffer[bufferBase + binIndex], inputBuffer[bufferBase + 512 - binIndex], Wk);
}

// Inverse Real FFT1024 첫 stage. packed spectrum 을 complex FFT512 입력 Z[k] 로 복원한 뒤 radix-8 stage 0 을 수행한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_512BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 1024-point real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix8_512ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix8_512ButterflyCount;

    // 현재 real FFT1024 블록에서 사용할 packed spectrum/complex 중간 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex * 512;

    // complex FFT512 radix-8 stage 0 의 여덟 입력 원소 사이의 간격 (stockham)
    uint inputStride = 512 / 8;

    uint binIndex0 = butterflyIndex;
    uint binIndex1 = binIndex0 + inputStride;
    uint binIndex2 = binIndex1 + inputStride;
    uint binIndex3 = binIndex2 + inputStride;
    uint binIndex4 = binIndex3 + inputStride;
    uint binIndex5 = binIndex4 + inputStride;
    uint binIndex6 = binIndex5 + inputStride;
    uint binIndex7 = binIndex6 + inputStride;

    uint outputIndex0 = bufferBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix8_512ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix8_512ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix8_512ButterflyCount;
    uint outputIndex4 = outputIndex3 + FFTRadix8_512ButterflyCount;
    uint outputIndex5 = outputIndex4 + FFTRadix8_512ButterflyCount;
    uint outputIndex6 = outputIndex5 + FFTRadix8_512ButterflyCount;
    uint outputIndex7 = outputIndex6 + FFTRadix8_512ButterflyCount;

    // packed spectrum X[k] 에서 complex FFT512 입력 Z[k] 를 복원한다.
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
