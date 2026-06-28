#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), UAV(u0))"

// 256 개의 complex IFFT 중간 결과가 연속으로 배치된 입력 버퍼
StructuredBuffer<float2> inputBuffer : register(t0);

// 512 개의 real 데이터가 연속으로 배치된 출력 버퍼
RWStructuredBuffer<float> outputBuffer : register(u0);

// Inverse Real FFT512 마지막 스테이지. complex FFT256 radix-4 stage 3 을 수행한 뒤 z[n] 을 x[2n], x[2n + 1] 로 unpack 한다.
[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FFTRadix4_256BlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint threadIndex = dispatchThreadId.x;

    // 512-point real FFT 블럭 인덱스
    uint fftBlockIndex = threadIndex / FFTRadix4_256ButterflyCount;

    // 버터플라이 인덱스
    uint butterflyIndex = threadIndex - fftBlockIndex * FFTRadix4_256ButterflyCount;

    // 현재 real FFT512 블록에서 사용할 입력/출력 버퍼의 시작 오프셋
    uint inputBase = fftBlockIndex * 256;
    uint outputBase = fftBlockIndex * 512;

    // complex FFT256 radix-4 마지막 스테이지는 span 256, inputStride 1 로 고정된다.
    uint inputIndex0 = inputBase + (butterflyIndex << 2u);
    uint inputIndex1 = inputIndex0 + 1;
    uint inputIndex2 = inputIndex1 + 1;
    uint inputIndex3 = inputIndex2 + 1;

    uint outputIndex0 = butterflyIndex;
    uint outputIndex1 = outputIndex0 + FFTRadix4_256ButterflyCount;
    uint outputIndex2 = outputIndex1 + FFTRadix4_256ButterflyCount;
    uint outputIndex3 = outputIndex2 + FFTRadix4_256ButterflyCount;

    // twiddle factor W, W^2, W^3 계산
    float angle = TWO_PI * float(butterflyIndex) / float(256);
    float sinAngle;
    float cosAngle;
    sincos(angle, sinAngle, cosAngle);

    float2 twiddle1 = float2(cosAngle, sinAngle);
    float2 twiddle2 = ComplexMul(twiddle1, twiddle1);
    float2 twiddle3 = ComplexMul(twiddle2, twiddle1);

    // complex FFT256 radix-4 마지막 스테이지
    float2 a0 = inputBuffer[inputIndex0];
    float2 a1 = ComplexMul(twiddle1, inputBuffer[inputIndex1]);
    float2 a2 = ComplexMul(twiddle2, inputBuffer[inputIndex2]);
    float2 a3 = ComplexMul(twiddle3, inputBuffer[inputIndex3]);

    // radix-4 버터플라이 연산
    float2 stageOutput0;
    float2 stageOutput1;
    float2 stageOutput2;
    float2 stageOutput3;
    Radix4Butterfly(a0, a1, a2, a3, 1.0, stageOutput0, stageOutput1, stageOutput2, stageOutput3);

    // 역변환의 마지막 스테이지에서 정규화를 한다.
    stageOutput0 *= 1.0 / 256.0;
    stageOutput1 *= 1.0 / 256.0;
    stageOutput2 *= 1.0 / 256.0;
    stageOutput3 *= 1.0 / 256.0;

    uint realIndex0 = outputBase + (outputIndex0 << 1u);
    uint realIndex1 = outputBase + (outputIndex1 << 1u);
    uint realIndex2 = outputBase + (outputIndex2 << 1u);
    uint realIndex3 = outputBase + (outputIndex3 << 1u);

    // z[n] 을 x[2n], x[2n + 1] 로 unpack 한다.
    outputBuffer[realIndex0] = stageOutput0.x;      // Re(z[outputIndex0])
    outputBuffer[realIndex0 + 1] = stageOutput0.y;  // Im(z[outputIndex0])
    outputBuffer[realIndex1] = stageOutput1.x;      // Re(z[outputIndex1])
    outputBuffer[realIndex1 + 1] = stageOutput1.y;  // Im(z[outputIndex1])
    outputBuffer[realIndex2] = stageOutput2.x;      // Re(z[outputIndex2])
    outputBuffer[realIndex2 + 1] = stageOutput2.y;  // Im(z[outputIndex2])
    outputBuffer[realIndex3] = stageOutput3.x;      // Re(z[outputIndex3])
    outputBuffer[realIndex3 + 1] = stageOutput3.y;  // Im(z[outputIndex3])
}
