#include "ShaderInterop.h"
#include "FFT/FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0)), " \
    "CBV(b0)"

cbuffer BloomCB : register(b0) {
    BloomParams bloomParams;
}

// 입력/출력 버퍼는 transpose 된 dual-real FFT plane 두 개 (RG, B0) 가 연속으로 배치된 형태이다.
// 각 plane 은 두 real signal 의 FFT spectrum 을 packed complex 형태로 담고 있다.
StructuredBuffer<float2> kernelSpectrumBuffer : register(t0);
StructuredBuffer<float2> screenSpectrumBuffer : register(t1);
RWStructuredBuffer<float2> outputSpectrumBuffer : register(u0);

float2 PackDualRealProduct(uint columnBase, uint columnIndex, uint fftSize) {
    uint halfSize = fftSize >> 1u;
    uint index = columnBase + columnIndex;

    float2 screenValue = screenSpectrumBuffer[index];
    float2 kernelValue = kernelSpectrumBuffer[index];

    // packed dual-real spectrum 의 DC/Nyquist 항은 각각 실수 성분에 들어있다.
    if (columnIndex == 0 || columnIndex == halfSize) {
        return screenValue * kernelValue * BP_BLOOM_KERNEL_SCALE;
    }

    // 대칭 인덱스
    uint mirrorIndex = columnBase + fftSize - columnIndex;

    float2 screenMirror = screenSpectrumBuffer[mirrorIndex];
    float2 kernelMirror = kernelSpectrumBuffer[mirrorIndex];

    // Z[k] = A[k] + iB[k] 로 packed 된 두 real spectrum 을 A/B 로 분리한다.
    float2 screenConjMirror = ComplexConj(screenMirror);
    float2 screenA = 0.5 * (screenValue + screenConjMirror);
    float2 screenDiff = screenValue - screenConjMirror;
    float2 screenB = 0.5 * float2(screenDiff.y, -screenDiff.x);

    float2 kernelConjMirror = ComplexConj(kernelMirror);
    float2 kernelA = 0.5 * (kernelValue + kernelConjMirror);
    float2 kernelDiff = kernelValue - kernelConjMirror;
    float2 kernelB = 0.5 * float2(kernelDiff.y, -kernelDiff.x);

    float2 outputA = ComplexMul(screenA, kernelA);
    float2 outputB = ComplexMul(screenB, kernelB);

    // A/B 를 다시 Z[k] = A[k] + iB[k] 형태로 packing 한다.
    return (outputA + float2(-outputB.y, outputB.x)) * BP_BLOOM_KERNEL_SCALE;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(ConvolutionBloomSpectrumBlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    static const uint ColorPairCount = 2; // RG, B0

    uint fftSize = bloomParams.outputSize.x;
    uint matrixElementCount = fftSize * fftSize;
    uint elementCount = matrixElementCount * ColorPairCount;
    uint elementIndex = dispatchThreadId.x;
    if (elementIndex >= elementCount) {
        return;
    }

    uint halfSize = fftSize >> 1u;
    uint planeElementIndex = elementIndex % matrixElementCount;

    // 각 plane 은 transpose 되어있으므로 row 를 column 이라고 생각하면 됨.
    uint planeRowIndex = planeElementIndex / fftSize;

    // 각 spectrum 의 첫번째 요소는 실수값인 (DC, Nyquist) 으로 packing 되어있다.
    if (planeRowIndex == 0 || planeRowIndex == halfSize) {
        uint columnIndex = planeElementIndex - planeRowIndex * fftSize;
        uint columnBase = elementIndex - columnIndex;
        outputSpectrumBuffer[elementIndex] = PackDualRealProduct(columnBase, columnIndex, fftSize);
    } else {
        // 나머지 elementIndex 는 complex 값이므로 그냥 곱하기만 하면된다.
        outputSpectrumBuffer[elementIndex] = ComplexMul(screenSpectrumBuffer[elementIndex], kernelSpectrumBuffer[elementIndex]) * BP_BLOOM_KERNEL_SCALE;
    }
}
