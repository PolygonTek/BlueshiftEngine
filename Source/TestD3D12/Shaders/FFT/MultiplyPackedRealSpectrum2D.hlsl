#include "../ShaderInterop.h"
#include "FFTCommon.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0)), " \
    "CBV(b0)"

cbuffer MultiplySpectrum2DCB : register(b0) {
    MultiplySpectrum2DParams multiplyParams;
}

StructuredBuffer<float2> lhsSpectrumBuffer : register(t0);
StructuredBuffer<float2> rhsSpectrumBuffer : register(t1);
RWStructuredBuffer<float2> outputSpectrumBuffer : register(u0);

#include "MultiplyPackedSpectrum2DCommon.hlsli"

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(MultiplySpectrum2DBlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint planeElementCount = multiplyParams.fftSize * (multiplyParams.fftSize >> 1u);
    const uint elementCount = planeElementCount * multiplyParams.planeCount;
    const uint elementIndex = dispatchThreadId.x;
    if (elementIndex >= elementCount) {
        return;
    }

    const uint planeElementIndex = elementIndex % planeElementCount;
    
    // 각 plane 은 transpose 되어있으므로 row 를 column 이라고 생각하면 됨.
    const uint planeRowIndex = planeElementIndex / multiplyParams.fftSize;

    // 0 번 row 에는 2D real 입력 데이터를 가로축으로 real FFT 한 뒤 spectrum 의 DC/Nyquist 성분들을 packed complex 로 보고 세로축으로 FFT 정변환한 결과가 들어있다.
    if (planeRowIndex == 0) {
        const uint columnIndex = planeElementIndex;
        const uint rowBase = elementIndex - columnIndex;
        outputSpectrumBuffer[elementIndex] = PackDCNyquistRowProduct(rowBase, columnIndex, multiplyParams.fftSize) * multiplyParams.scale;
    } else {
        // 나머지 element 는 complex 값이므로 그냥 곱하기만 하면 된다.
        outputSpectrumBuffer[elementIndex] = ComplexMul(lhsSpectrumBuffer[elementIndex], rhsSpectrumBuffer[elementIndex]) * multiplyParams.scale;
    }
}
