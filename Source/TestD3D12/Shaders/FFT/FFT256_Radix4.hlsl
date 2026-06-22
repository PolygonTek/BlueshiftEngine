#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0)), " \
    "CBV(b0)"

cbuffer FFTStageCB : register(b0) {
    FFTStageParams fftParams;
}

// 256 개의 복소수 FFT 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 256 개의 복소수 FFT 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// Radix-4 Stockham autosort FFT. Each dispatch processes one stageIndex in [0, 3].
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix4_256BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 256-point FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix4_256ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix4_256ButterflyCount;

    // 현재 FFT256 블록에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex << 8;

    // 스테이지 별 버터플라이 그룹의 사이즈: 4, 16, 64, 256
    uint span = 4u << (fftParams.stageIndex * 2u);

    // 하나의 버터플라이가 읽는 네 입력 원소 사이의 간격 (stockham)
    uint inputStride = 256 / span;

    // 현재 스테이지에서의 twiddle factor 의 지수 = W_{span}^{twiddle_index}
    uint twiddleIndex = butterflyIndex / inputStride;

    // 같은 twiddle factor 를 사용하는 버터플라이 그룹의 로컬 인덱스
    uint laneIndex = butterflyIndex - twiddleIndex * inputStride;

    uint inputIndex0 = bufferBase + laneIndex + inputStride * (twiddleIndex << 2u);
    uint inputIndex1 = inputIndex0 + inputStride;
    uint inputIndex2 = inputIndex1 + inputStride;
    uint inputIndex3 = inputIndex2 + inputStride;

    uint outputIndex0 = bufferBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix4_256ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix4_256ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix4_256ButterflyCount;

    // twiddle factor W, W^2, W^3 계산
    float angle = fftParams.sign * TWO_PI * float(twiddleIndex) / float(span);
    float sinAngle;
    float cosAngle;
    sincos(angle, sinAngle, cosAngle);

    float2 twiddle1 = float2(cosAngle, sinAngle);
    float2 twiddle2 = ComplexMul(twiddle1, twiddle1);
    float2 twiddle3 = ComplexMul(twiddle2, twiddle1);

    // radix-4 버터플라이 연산
    float2 a0 = inputBuffer[inputIndex0];
    float2 a1 = ComplexMul(twiddle1, inputBuffer[inputIndex1]);
    float2 a2 = ComplexMul(twiddle2, inputBuffer[inputIndex2]);
    float2 a3 = ComplexMul(twiddle3, inputBuffer[inputIndex3]);

    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, fftParams.sign, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    // 역변환의 마지막 스테이지에서 정규화를 한다 (정변환은 scale = 1, 역변환은 scale = 1/256)
    if (fftParams.stageIndex == FFTRadix4_256StageCount - 1) {
        stageOutput0 *= fftParams.scale;
        stageOutput1 *= fftParams.scale;
        stageOutput2 *= fftParams.scale;
        stageOutput3 *= fftParams.scale;
    }

    outputBuffer[outputIndex0] = stageOutput0;
    outputBuffer[outputIndex1] = stageOutput1;
    outputBuffer[outputIndex2] = stageOutput2;
    outputBuffer[outputIndex3] = stageOutput3;
}
