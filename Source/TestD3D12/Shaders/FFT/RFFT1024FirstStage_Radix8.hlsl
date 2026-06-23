#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// 1024 개의 실수 입력 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float> inputBuffer : register(t0);

// 512 개의 packed complex FFT 중간 결과가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// Forward Real FFT1024 첫 stage. 실수 입력을 512 개의 complex 값으로 pack 한 뒤 complex FFT512 radix-8 stage 0 을 수행한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix8_512BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 1024-point real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix8_512ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix8_512ButterflyCount;

    // 현재 real FFT1024 블록에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint inputBase = fftBlockIndex * 1024;
    uint outputBase = fftBlockIndex * 512;

    // complex FFT512 radix-8 stage 0 의 여덟 입력 원소 사이의 간격 (stockham)
    uint inputStride = 512 / 8;

    uint packedIndex0 = butterflyIndex;
    uint packedIndex1 = packedIndex0 + inputStride;
    uint packedIndex2 = packedIndex1 + inputStride;
    uint packedIndex3 = packedIndex2 + inputStride;
    uint packedIndex4 = packedIndex3 + inputStride;
    uint packedIndex5 = packedIndex4 + inputStride;
    uint packedIndex6 = packedIndex5 + inputStride;
    uint packedIndex7 = packedIndex6 + inputStride;

    uint realIndex0 = inputBase + (packedIndex0 << 1u);
    uint realIndex1 = inputBase + (packedIndex1 << 1u);
    uint realIndex2 = inputBase + (packedIndex2 << 1u);
    uint realIndex3 = inputBase + (packedIndex3 << 1u);
    uint realIndex4 = inputBase + (packedIndex4 << 1u);
    uint realIndex5 = inputBase + (packedIndex5 << 1u);
    uint realIndex6 = inputBase + (packedIndex6 << 1u);
    uint realIndex7 = inputBase + (packedIndex7 << 1u);

    // 결과 complex 원소 인덱스
    uint outputIndex0 = outputBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix8_512ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix8_512ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix8_512ButterflyCount;
    uint outputIndex4 = outputIndex3 + FFTRadix8_512ButterflyCount;
    uint outputIndex5 = outputIndex4 + FFTRadix8_512ButterflyCount;
    uint outputIndex6 = outputIndex5 + FFTRadix8_512ButterflyCount;
    uint outputIndex7 = outputIndex6 + FFTRadix8_512ButterflyCount;

    // z[n] = x[2n] + i * x[2n + 1] 로 pack 한다.
    float2 z0 = float2(inputBuffer[realIndex0], inputBuffer[realIndex0 + 1]);
    float2 z1 = float2(inputBuffer[realIndex1], inputBuffer[realIndex1 + 1]);
    float2 z2 = float2(inputBuffer[realIndex2], inputBuffer[realIndex2 + 1]);
    float2 z3 = float2(inputBuffer[realIndex3], inputBuffer[realIndex3 + 1]);
    float2 z4 = float2(inputBuffer[realIndex4], inputBuffer[realIndex4 + 1]);
    float2 z5 = float2(inputBuffer[realIndex5], inputBuffer[realIndex5 + 1]);
    float2 z6 = float2(inputBuffer[realIndex6], inputBuffer[realIndex6 + 1]);
    float2 z7 = float2(inputBuffer[realIndex7], inputBuffer[realIndex7 + 1]);

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
