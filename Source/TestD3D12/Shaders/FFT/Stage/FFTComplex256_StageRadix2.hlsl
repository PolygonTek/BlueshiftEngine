#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0), " \
    "RootConstants(num32BitConstants=1, b1)"

cbuffer FFTStageCB : register(b0) {
    FFTStageParams fftParams;
}

cbuffer FFTStageIndexCB : register(b1) {
    uint fftStageIndex;
}

// 256 개의 복소수 FFT 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 256 개의 복소수 FFT 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// Radix-2 Stockham autosort FFT. Each dispatch processes one stageIndex in [0, 7].
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix2_256BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 256-point FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix2_256ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix2_256ButterflyCount;

    // 현재 FFT256 블럭에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex << 8;
    
    // 스테이지 별 버터플라이 그룹의 사이즈: 2, 4, 8, 16, 32, 64, 128, 256
    uint span = 2u << fftStageIndex;

    // 하나의 버터플라이가 읽는 두 입력 원소 사이의 간격 (Stockham)
    uint inputStride = 256 / span;

    // 현재 스테이지에서의 twiddle factor 의 지수 = W_{span}^{twiddle_index}
    uint twiddleIndex = butterflyIndex / inputStride;

    // 같은 twiddle factor 를 사용하는 버터플라이 그룹의 로컬 인덱스
    uint laneIndex = butterflyIndex - twiddleIndex * inputStride;

    uint inputIndex0 = bufferBase + laneIndex + inputStride * (twiddleIndex << 1u);
    uint inputIndex1 = inputIndex0 + inputStride;

    uint outputIndex0 = bufferBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix2_256ButterflyCount;

    // twiddle factor W 계산
    float angle = fftParams.sign * TWO_PI * float(twiddleIndex) / float(span);
    float sinAngle;
    float cosAngle;
    sincos(angle, sinAngle, cosAngle);
    float2 twiddle = float2(cosAngle, sinAngle);

    // radix-2 버터플라이 연산
    float2 a = inputBuffer[inputIndex0];
    float2 b = ComplexMul(twiddle, inputBuffer[inputIndex1]);

    float2 stageOutput0 = a + b;
    float2 stageOutput1 = a - b;

    // 역변환의 마지막 스테이지에서 정규화를 한다 (정변환은 scale = 1, 역변환은 scale = 1/256)
    if (fftStageIndex == FFTRadix2_256StageCount - 1) {
        stageOutput0 *= fftParams.scale;
        stageOutput1 *= fftParams.scale;
    }

    outputBuffer[outputIndex0] = stageOutput0;
    outputBuffer[outputIndex1] = stageOutput1;
}
