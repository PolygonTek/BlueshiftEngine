#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// 512 개의 실수 입력 데이터가 연속으로 배치된 입력 버퍼
StructuredBuffer<float> inputBuffer : register(t0);

// 256 개의 packed complex FFT 중간 결과가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float2> outputBuffer : register(u0);

// Forward Real FFT512 첫 stage. 실수 입력을 256 개의 complex 값으로 pack 한 뒤 complex FFT256 radix-4 stage 0 을 수행한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix4_256BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 512-point real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix4_256ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix4_256ButterflyCount;

    // 현재 real FFT512 블록에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint inputBase = fftBlockIndex * 512;
    uint outputBase = fftBlockIndex * 256;

    // complex FFT256 radix-4 stage 0 의 네 입력 원소 사이의 간격 (stockham)
    uint inputStride = 256 / 4;

    uint packedIndex0 = butterflyIndex;
    uint packedIndex1 = packedIndex0 + inputStride;
    uint packedIndex2 = packedIndex1 + inputStride;
    uint packedIndex3 = packedIndex2 + inputStride;

    uint realIndex0 = inputBase + (packedIndex0 << 1u);
    uint realIndex1 = inputBase + (packedIndex1 << 1u);
    uint realIndex2 = inputBase + (packedIndex2 << 1u);
    uint realIndex3 = inputBase + (packedIndex3 << 1u);

    // 결과 complex 원소 인덱스
    uint outputIndex0 = outputBase + butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix4_256ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix4_256ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix4_256ButterflyCount;

    // z[n] = x[2n] + i * x[2n + 1] 로 pack 한다.
    float2 z0 = float2(inputBuffer[realIndex0], inputBuffer[realIndex0 + 1]);
    float2 z1 = float2(inputBuffer[realIndex1], inputBuffer[realIndex1 + 1]);
    float2 z2 = float2(inputBuffer[realIndex2], inputBuffer[realIndex2 + 1]);
    float2 z3 = float2(inputBuffer[realIndex3], inputBuffer[realIndex3 + 1]);

    // radix-4 버터플라이 연산
    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(z0, z1, z2, z3, -1.0, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    outputBuffer[outputIndex0] = stageOutput0;
    outputBuffer[outputIndex1] = stageOutput1;
    outputBuffer[outputIndex2] = stageOutput2;
    outputBuffer[outputIndex3] = stageOutput3;
}
