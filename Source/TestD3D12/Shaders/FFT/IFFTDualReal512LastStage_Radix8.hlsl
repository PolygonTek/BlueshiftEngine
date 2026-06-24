#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// 512 개의 complex IFFT 중간 결과가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 512 개의 interleaved (512 x 2) 실수 데이터가 연속으로 배치된 출력 버퍼.
RWStructuredBuffer<float2> outputBuffer : register(u0);

// Inverse DualReal512 마지막 스테이지. complex FFT512 radix-8 stage 2 를 수행한 뒤 a/b real signal 로 unpack 한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_512BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 512-point dual-real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix8_512ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix8_512ButterflyCount;

    // 현재 dual-real FFT512 블록에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex * 512;

    // complex FFT512 radix-8 마지막 스테이지는 span 512, inputStride 1 로 고정된다.
    uint inputIndex0 = bufferBase + (butterflyIndex << 3u);
    uint inputIndex1 = inputIndex0 + 1;
    uint inputIndex2 = inputIndex1 + 1;
    uint inputIndex3 = inputIndex2 + 1;
    uint inputIndex4 = inputIndex3 + 1;
    uint inputIndex5 = inputIndex4 + 1;
    uint inputIndex6 = inputIndex5 + 1;
    uint inputIndex7 = inputIndex6 + 1;

    uint outputIndex0 = butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix8_512ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix8_512ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix8_512ButterflyCount;
    uint outputIndex4 = outputIndex3 + FFTRadix8_512ButterflyCount;
    uint outputIndex5 = outputIndex4 + FFTRadix8_512ButterflyCount;
    uint outputIndex6 = outputIndex5 + FFTRadix8_512ButterflyCount;
    uint outputIndex7 = outputIndex6 + FFTRadix8_512ButterflyCount;

    // twiddle factor W, W^2, ..., W^7 계산
    float angle = TWO_PI * float(butterflyIndex) / float(512);
    float sinAngle;
    float cosAngle;
    sincos(angle, sinAngle, cosAngle);

    float2 twiddle1 = float2(cosAngle, sinAngle);
    float2 twiddle2 = ComplexMul(twiddle1, twiddle1);
    float2 twiddle3 = ComplexMul(twiddle2, twiddle1);
    float2 twiddle4 = ComplexMul(twiddle3, twiddle1);
    float2 twiddle5 = ComplexMul(twiddle4, twiddle1);
    float2 twiddle6 = ComplexMul(twiddle5, twiddle1);
    float2 twiddle7 = ComplexMul(twiddle6, twiddle1);

    // complex FFT512 radix-8 마지막 스테이지
    float2 a0 = inputBuffer[inputIndex0];
    float2 a1 = ComplexMul(twiddle1, inputBuffer[inputIndex1]);
    float2 a2 = ComplexMul(twiddle2, inputBuffer[inputIndex2]);
    float2 a3 = ComplexMul(twiddle3, inputBuffer[inputIndex3]);
    float2 a4 = ComplexMul(twiddle4, inputBuffer[inputIndex4]);
    float2 a5 = ComplexMul(twiddle5, inputBuffer[inputIndex5]);
    float2 a6 = ComplexMul(twiddle6, inputBuffer[inputIndex6]);
    float2 a7 = ComplexMul(twiddle7, inputBuffer[inputIndex7]);

    // radix-8 버터플라이 연산
    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    float2 stageOutput4;
    float2 stageOutput5;
    float2 stageOutput6;
    float2 stageOutput7;
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, 1.0, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    // 역변환의 마지막 스테이지에서 정규화를 한다.
    stageOutput0 *= 1.0 / 512.0;
    stageOutput1 *= 1.0 / 512.0;
    stageOutput2 *= 1.0 / 512.0;
    stageOutput3 *= 1.0 / 512.0;
    stageOutput4 *= 1.0 / 512.0;
    stageOutput5 *= 1.0 / 512.0;
    stageOutput6 *= 1.0 / 512.0;
    stageOutput7 *= 1.0 / 512.0;

    outputBuffer[bufferBase + outputIndex0] = stageOutput0;
    outputBuffer[bufferBase + outputIndex1] = stageOutput1;
    outputBuffer[bufferBase + outputIndex2] = stageOutput2;
    outputBuffer[bufferBase + outputIndex3] = stageOutput3;
    outputBuffer[bufferBase + outputIndex4] = stageOutput4;
    outputBuffer[bufferBase + outputIndex5] = stageOutput5;
    outputBuffer[bufferBase + outputIndex6] = stageOutput6;
    outputBuffer[bufferBase + outputIndex7] = stageOutput7;
}
