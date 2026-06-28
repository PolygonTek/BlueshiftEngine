#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0), " \
    "RootConstants(num32BitConstants=1, b1)"

cbuffer FFTStageCB : register(b0) {
    FFTStageParams fftParams;
}

// 1024 개의 복소수 FFT 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 1024 개의 복소수 FFT 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// FFT1024 마지막 스테이지. radix-2 stage 를 수행하고 scale 을 적용한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix2_1024BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 1024-point FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix2_1024ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix2_1024ButterflyCount;

    // 현재 FFT1024 블록에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex << 10;

    // complex FFT1024 마지막 스테이지는 span 1024, inputStride 1 로 고정된다.
    uint inputIndex0 = bufferBase + (butterflyIndex << 1u);
    uint inputIndex1 = inputIndex0 + 1;

    uint outputIndex0 = bufferBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix2_1024ButterflyCount;

    // twiddle factor W 계산
    float angle = fftParams.sign * TWO_PI * float(butterflyIndex) / float(1024);
    float sinAngle;
    float cosAngle;
    sincos(angle, sinAngle, cosAngle);
    float2 twiddle = float2(cosAngle, sinAngle);

    // radix-2 버터플라이 연산
    float2 a = inputBuffer[inputIndex0];
    float2 b = ComplexMul(twiddle, inputBuffer[inputIndex1]);

    // 마지막 스테이지이므로 정변환은 scale = 1, 역변환은 scale = 1/1024 를 전달한다.
    outputBuffer[outputIndex0] = (a + b) * fftParams.scale;
    outputBuffer[outputIndex1] = (a - b) * fftParams.scale;
}
