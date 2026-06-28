#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0))"

// 256 개의 packed complex (packed real forward FFT512) FFT 최종 결과가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// W_512^k = exp(i * 2PI * k / 512) 를 저장한 twiddle factor 테이블
StructuredBuffer<float2> twiddleBuffer : register(t1);

// 256 개의 complex IFFT 중간 결과가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// packed X spectrum 으로부터 complex IFFT 입력 Z[0..255] 를 구한다.
float2 LoadComplexIFFTInput(uint bufferBase, uint binIndex) {
    // X[0] 에는 실수부에는 DC 항이, 허수부에는 Nyquist 항이 담겨있다.
    if (binIndex == 0) {
        float2 dcNyquist = inputBuffer[bufferBase];
        return 0.5 * float2(dcNyquist.x + dcNyquist.y, dcNyquist.x - dcNyquist.y);
    }
    // 0 < k < 256
    // X[k] 와 X[256 - k] 로부터 Z[k] 를 구한다.
    float2 Wk = twiddleBuffer[binIndex];
    return RealIFFTPreProcess(inputBuffer[bufferBase + binIndex], inputBuffer[bufferBase + 256 - binIndex], Wk);
}

// Inverse Real FFT512 첫 stage. packed spectrum 을 complex FFT256 입력 Z[k] 로 복원한 뒤 radix-4 stage 0 을 수행한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix4_256BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 512-point real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix4_256ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix4_256ButterflyCount;

    // 현재 real FFT512 블록에서 사용할 packed spectrum/complex 중간 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex * 256;

    // complex FFT256 radix-4 stage 0 의 네 입력 원소 사이의 간격 (Stockham)
    uint inputStride = 256 / 4;

    uint binIndex0 = butterflyIndex;
    uint binIndex1 = binIndex0 + inputStride;
    uint binIndex2 = binIndex1 + inputStride;
    uint binIndex3 = binIndex2 + inputStride;

    uint outputIndex0 = bufferBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix4_256ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix4_256ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix4_256ButterflyCount;

    // packed spectrum X[k] 에서 complex FFT256 입력 Z[k] 를 복원한다.
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
