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

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(MultiplySpectrum2DBlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint planeElementCount = multiplyParams.fftSize * multiplyParams.fftSize;
    const uint elementCount = planeElementCount * multiplyParams.planeCount;
    const uint elementIndex = dispatchThreadId.x;
    if (elementIndex >= elementCount) {
        return;
    }

    // complex 값끼리 곱한다.
    outputSpectrumBuffer[elementIndex] = ComplexMul(lhsSpectrumBuffer[elementIndex], rhsSpectrumBuffer[elementIndex]) * multiplyParams.scale;
}
