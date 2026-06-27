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

// 512 개의 복소수 FFT 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 512 개의 복소수 FFT 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// Radix-8 Stockham autosort FFT. Each dispatch processes one stageIndex in [0, 2].
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_512BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 512-point FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix8_512ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix8_512ButterflyCount;

    // 현재 FFT512 블록에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex << 9;

    // 스테이지 별 버터플라이 그룹의 사이즈: 8, 64, 512
    uint span = 8u << (fftStageIndex * 3u);

    // 하나의 버터플라이가 읽는 여덟 입력 원소 사이의 간격 (Stockham)
    uint inputStride = 512 / span;

    // 현재 스테이지에서의 twiddle factor 의 지수 = W_{span}^{twiddle_index}
    uint twiddleIndex = butterflyIndex / inputStride;

    // 같은 twiddle factor 를 사용하는 버터플라이 그룹의 로컬 인덱스
    uint laneIndex = butterflyIndex - twiddleIndex * inputStride;

    uint inputIndex0 = bufferBase + laneIndex + inputStride * (twiddleIndex << 3u);
    uint inputIndex1 = inputIndex0 + inputStride;
    uint inputIndex2 = inputIndex1 + inputStride;
    uint inputIndex3 = inputIndex2 + inputStride;
    uint inputIndex4 = inputIndex3 + inputStride;
    uint inputIndex5 = inputIndex4 + inputStride;
    uint inputIndex6 = inputIndex5 + inputStride;
    uint inputIndex7 = inputIndex6 + inputStride;

    uint outputIndex0 = bufferBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix8_512ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix8_512ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix8_512ButterflyCount;
    uint outputIndex4 = outputIndex3 + FFTRadix8_512ButterflyCount;
    uint outputIndex5 = outputIndex4 + FFTRadix8_512ButterflyCount;
    uint outputIndex6 = outputIndex5 + FFTRadix8_512ButterflyCount;
    uint outputIndex7 = outputIndex6 + FFTRadix8_512ButterflyCount;

    // twiddle factor W, W^2, ..., W^7 계산
    float angle = fftParams.sign * TWO_PI * float(twiddleIndex) / float(span);
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

    // stage twiddle 적용
    float2 a0 = inputBuffer[inputIndex0];
    float2 a1 = ComplexMul(twiddle1, inputBuffer[inputIndex1]);
    float2 a2 = ComplexMul(twiddle2, inputBuffer[inputIndex2]);
    float2 a3 = ComplexMul(twiddle3, inputBuffer[inputIndex3]);
    float2 a4 = ComplexMul(twiddle4, inputBuffer[inputIndex4]);
    float2 a5 = ComplexMul(twiddle5, inputBuffer[inputIndex5]);
    float2 a6 = ComplexMul(twiddle6, inputBuffer[inputIndex6]);
    float2 a7 = ComplexMul(twiddle7, inputBuffer[inputIndex7]);

    // radix-8 버터플라이는 even/odd Radix-4 결과를 8-point twiddle 로 합성한다.
    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    float2 stageOutput4;
    float2 stageOutput5;
    float2 stageOutput6;
    float2 stageOutput7;
    Radix8Butterfly(a0, a1, a2, a3, a4, a5, a6, a7, fftParams.sign, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    // 역변환의 마지막 스테이지에서 정규화를 한다. (정변환은 scale = 1, 역변환은 scale = 1/512)
    if (fftStageIndex == FFTRadix8_512StageCount - 1) {
        stageOutput0 *= fftParams.scale;
        stageOutput1 *= fftParams.scale;
        stageOutput2 *= fftParams.scale;
        stageOutput3 *= fftParams.scale;
        stageOutput4 *= fftParams.scale;
        stageOutput5 *= fftParams.scale;
        stageOutput6 *= fftParams.scale;
        stageOutput7 *= fftParams.scale;
    }

    outputBuffer[outputIndex0] = stageOutput0;
    outputBuffer[outputIndex1] = stageOutput1;
    outputBuffer[outputIndex2] = stageOutput2;
    outputBuffer[outputIndex3] = stageOutput3;
    outputBuffer[outputIndex4] = stageOutput4;
    outputBuffer[outputIndex5] = stageOutput5;
    outputBuffer[outputIndex6] = stageOutput6;
    outputBuffer[outputIndex7] = stageOutput7;
}
