#include "../ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), UAV(u0)), " \
    "CBV(b0)"

cbuffer BloomCB : register(b0) {
    BloomParams bloomParams;
}

// grayscale aperture 이미지의 2D FFT 결과인 입력 버퍼
StructuredBuffer<float2> diffractionSpectrumBuffer : register(t0);
StructuredBuffer<float2> apertureEnergyBuffer : register(t1);

// 입력 FFT 버퍼로 abs(FFT)^2 후에 circular shift 하는 출력 텍스처
RWTexture2D<float> outputTexture : register(u0);

static const float DiffractionPSFStoragePeak = 32768.0;

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(BloomBlockSizeX, BloomBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= bloomParams.outputSize.x || pixelCoord.y >= bloomParams.outputSize.y) {
        return;
    }

    if (pixelCoord.x >= bloomParams.inputSize.x || pixelCoord.y >= bloomParams.inputSize.y) {
        outputTexture[pixelCoord] = 0.0;
        return;
    }

    // (0, 0) DC 성분이 중앙에 오도록 좌표를 circular shift
    uint2 spectrumCoord = (pixelCoord + bloomParams.inputSize / 2) % bloomParams.inputSize;

    // FFTComplex2D 출력은 transposed 상태이므로 (x, y) spectrum 을 y + x * width 위치에서 읽는다.
    uint spectrumIndex = spectrumCoord.x * bloomParams.inputSize.y + spectrumCoord.y;
    
    // 비정규화 forward FFT 에서 abs(FFT)^2 / apertureEnergy 의 이론적 상한은 2D FFT sample count (width * height) 이다.
    // R16F 저장 범위와 정밀도를 안정적으로 사용하도록 해상도와 무관하게 최대값을 DiffractionPSFStoragePeak 로 맞춘다.
    float2 field = diffractionSpectrumBuffer[spectrumIndex];
    float apertureEnergy = max(apertureEnergyBuffer[0].x, 0.000001);
    float pixelCount = float(bloomParams.inputSize.x) * float(bloomParams.inputSize.y);
    float intensity = dot(field, field) / apertureEnergy * (DiffractionPSFStoragePeak / pixelCount);
    outputTexture[pixelCoord] = max(intensity, 0.0);
}
