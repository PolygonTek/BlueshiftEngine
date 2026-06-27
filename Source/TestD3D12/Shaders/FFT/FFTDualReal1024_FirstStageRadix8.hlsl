#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// 1024 개의 interleaved (1024 x 2) 실수 입력 데이터가 연속으로 배치된 입력 버퍼.
StructuredBuffer<float2> inputBuffer : register(t0);

// 1024 개의 packed complex FFT 중간 결과가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// Forward DualReal1024 첫 stage. 두 real signal 을 하나의 complex signal 로 pack 한 뒤 radix-8 stage 0 을 수행한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_1024BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 1024-point dual-real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix8_1024ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix8_1024ButterflyCount;

    // 현재 dual-real FFT1024 블록에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint bufferBase = fftBlockIndex * 1024;

    // complex FFT1024 radix-8 stage 0 의 여덟 입력 원소 사이의 간격 (stockham)
    uint inputStride = 1024 / 8;

    uint inputIndex0 = bufferBase + butterflyIndex;
    uint inputIndex1 = inputIndex0 + inputStride;
    uint inputIndex2 = inputIndex1 + inputStride;
    uint inputIndex3 = inputIndex2 + inputStride;
    uint inputIndex4 = inputIndex3 + inputStride;
    uint inputIndex5 = inputIndex4 + inputStride;
    uint inputIndex6 = inputIndex5 + inputStride;
    uint inputIndex7 = inputIndex6 + inputStride;

    // 결과 complex 원소 인덱스
    uint outputIndex0 = bufferBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix8_1024ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix8_1024ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix8_1024ButterflyCount;
    uint outputIndex4 = outputIndex3 + FFTRadix8_1024ButterflyCount;
    uint outputIndex5 = outputIndex4 + FFTRadix8_1024ButterflyCount;
    uint outputIndex6 = outputIndex5 + FFTRadix8_1024ButterflyCount;
    uint outputIndex7 = outputIndex6 + FFTRadix8_1024ButterflyCount;

    // z[n] = a[n] + i * b[n] 으로 pack 한다.
    float2 z0 = inputBuffer[inputIndex0];
    float2 z1 = inputBuffer[inputIndex1];
    float2 z2 = inputBuffer[inputIndex2];
    float2 z3 = inputBuffer[inputIndex3];
    float2 z4 = inputBuffer[inputIndex4];
    float2 z5 = inputBuffer[inputIndex5];
    float2 z6 = inputBuffer[inputIndex6];
    float2 z7 = inputBuffer[inputIndex7];

    // radix-8 버터플라이 연산
    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    float2 stageOutput4;
    float2 stageOutput5;
    float2 stageOutput6;
    float2 stageOutput7;
    Radix8Butterfly(z0, z1, z2, z3, z4, z5, z6, z7, -1.0, stageOutput0, stageOutput1, stageOutput2, stageOutput3, stageOutput4, stageOutput5, stageOutput6, stageOutput7);

    outputBuffer[outputIndex0] = stageOutput0;
    outputBuffer[outputIndex1] = stageOutput1;
    outputBuffer[outputIndex2] = stageOutput2;
    outputBuffer[outputIndex3] = stageOutput3;
    outputBuffer[outputIndex4] = stageOutput4;
    outputBuffer[outputIndex5] = stageOutput5;
    outputBuffer[outputIndex6] = stageOutput6;
    outputBuffer[outputIndex7] = stageOutput7;
}
