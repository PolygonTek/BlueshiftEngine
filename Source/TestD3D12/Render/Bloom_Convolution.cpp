// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Bloom.h"
#include "Render/RenderInternal.h"
#include "Render/GpuTiming.h"
#include "Render/GpuFFT.h"
#include "Shaders/ShaderInterop.h"

bool Bloom::DispatchConvolutionBloom(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuFFT &gpuFFT, GpuTiming &gpuTiming, RHI::Texture *inputColorTexture, const BloomFrameData &data, const RenderFrameSettings &renderFrameSettings) const {
    PROFILER_CPU_SCOPED_EVENT("Bloom::DispatchConvolutionBloom");

    if (data.renderWidth == 0 || data.renderHeight == 0 || data.fftBloomSize == 0) {
        return false;
    }

    // 커널 버퍼 업데이트가 필요한 경우에만 실행
    if (data.fftBloomKernelDirty) {
        if (!data.fftBloomKernelSourceTexture) {
            return false;
        }

        // 커널 이미지로 버퍼 준비 (축소 스케일, 회전)
        if (!DispatchConvolutionBloomKernelPrepare(commandList, frameThreadData, gpuTiming, data)) {
            return false;
        }

        // 커널 버퍼 FFT
        if (!DispatchConvolutionBloomFFT(commandList, gpuFFT, gpuTiming, data, data.fftBloomKernelSpatialBuffer, data.fftBloomKernelSpectrumBuffer)) {
            return false;
        }
    }

    // 화면 이미지에 bloom prefilter 를 적용하고, 축소 샘플링에 사용할 밉체인을 만든다.
    if (!DispatchConvolutionBloomScreenPrefilter(commandList, frameThreadData, gpuTiming, inputColorTexture, data, renderFrameSettings)) {
        return false;
    }

    // 화면 이미지로 버퍼 준비 (축소 스케일)
    if (!DispatchConvolutionBloomScreenPrepare(commandList, frameThreadData, gpuTiming, data)) {
        return false;
    }

    // 화면 버퍼 FFT
    if (!DispatchConvolutionBloomFFT(commandList, gpuFFT, gpuTiming, data, data.fftBloomScreenSpatialBuffer, data.fftBloomScreenSpectrumBuffer)) {
        return false;
    }

    // 화면 스펙트럼과 커널 스펙트럼의 Hadamard 곱 (fftBloomWorkBuffer 에 출력)
    if (!DispatchConvolutionBloomMultiplySpectrum(commandList, frameThreadData, gpuFFT, gpuTiming, data, renderFrameSettings)) {
        return false;
    }

    // 결과를 IFFT
    if (!DispatchConvolutionBloomIFFT(commandList, gpuFFT, gpuTiming, data, data.fftBloomWorkBuffer, data.fftBloomScreenSpatialBuffer)) {
        return false;
    }

    // IFFT 결과 버퍼를 스크린 half-res 텍스쳐 (fftBloomConvolutionOutputTexture) 에 resolve
    if (!DispatchConvolutionBloomResolve(commandList, frameThreadData, gpuTiming, data)) {
        return false;
    }

    return true;
}

bool Bloom::DispatchConvolutionBloomKernelPrepare(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, const BloomFrameData &data) const {
    PROFILER_CPU_SCOPED_EVENT("Bloom::DispatchConvolutionBloomKernelPrepare");

    if (data.fftBloomSize == 0) {
        return false;
    }

    RHI::Texture *sourceTexture = data.fftBloomKernelSourceTexture;
    RHI::Buffer *spatialBuffer = data.fftBloomKernelSpatialBuffer;
    const uint32_t inputW = sourceTexture->GetWidth();
    const uint32_t inputH = sourceTexture->GetHeight();
    const uint32_t outputW = data.fftBloomSize;
    const uint32_t outputH = data.fftBloomSize;
    if (inputW == 0 || inputH == 0 || outputW == 0 || outputH == 0) {
        return false;
    }

    float rotationSin;
    float rotationCos;
    BE1::Math::SinCos(DEG2RAD(data.fftBloomKernelRotation), rotationSin, rotationCos);

    // 회전과 non-uniform kernel scale 을 적용한 뒤 출력 texel 하나가 덮는 source kernel texel footprint 로 mip level 을 계산한다.
    const float mappedKernelScaleX = BE1::Max(data.fftBloomMappedKernelScale.x, 0.0001f);
    const float mappedKernelScaleY = BE1::Max(data.fftBloomMappedKernelScale.y, 0.0001f);
    const float sourceToOutputScaleX = static_cast<float>(inputW) / static_cast<float>(outputW);
    const float sourceToOutputScaleY = static_cast<float>(inputH) / static_cast<float>(outputH);
    const float sourceFootprintXX = sourceToOutputScaleX * rotationCos / mappedKernelScaleX;
    const float sourceFootprintXY = sourceToOutputScaleY * rotationSin / mappedKernelScaleX;
    const float sourceFootprintYX = sourceToOutputScaleX * rotationSin / mappedKernelScaleY;
    const float sourceFootprintYY = sourceToOutputScaleY * rotationCos / mappedKernelScaleY;
    const float sourceFootprintX = BE1::Math::Sqrt(sourceFootprintXX * sourceFootprintXX + sourceFootprintXY * sourceFootprintXY);
    const float sourceFootprintY = BE1::Math::Sqrt(sourceFootprintYX * sourceFootprintYX + sourceFootprintYY * sourceFootprintYY);
    const float kernelMipLevel = BE1::Max(BE1::Math::Log2(BE1::Max(sourceFootprintX, sourceFootprintY)), 0.0f);

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params0 = float4(rotationCos, rotationSin, data.fftBloomMappedKernelScale.x, data.fftBloomMappedKernelScale.y);
    bloomParams.params1.x = kernelMipLevel;

    RHI::ConstantBuffer *bloomConstantBuffer = frameThreadData->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::ConvolutionBloomKernelPrepare);

    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, frameConvolutionBloomKernelPreparePSO);
    RHI::renderer->SetTexture(commandList, 0, false, sourceTexture);
    RHI::renderer->SetBuffer(commandList, 0, true, spatialBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool Bloom::DispatchConvolutionBloomScreenPrefilter(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, RHI::Texture *inputColorTexture, const BloomFrameData &data, const RenderFrameSettings &renderFrameSettings) const {
    PROFILER_CPU_SCOPED_EVENT("Bloom::DispatchConvolutionBloomScreenPrefilter");

    RHI::Texture *inputTexture = inputColorTexture;
    RHI::Texture *outputTexture = data.fftBloomConvolutionOutputTexture;
    const uint32_t inputW = inputTexture ? inputTexture->GetWidth() : 0;
    const uint32_t inputH = inputTexture ? inputTexture->GetHeight() : 0;
    const uint32_t outputW = data.fftBloomConvolutionOutputWidth;
    const uint32_t outputH = data.fftBloomConvolutionOutputHeight;
    const uint32_t mipLevelCount = data.fftBloomConvolutionOutputMipCount;
    if (inputW == 0 || inputH == 0 || outputW == 0 || outputH == 0 || mipLevelCount == 0 || !data.fftBloomConvolutionOutputMipUAVs[0].IsValid()) {
        return false;
    }

    const float bloomThreshold = renderFrameSettings.bloomThreshold;
    const float bloomSoftThreshold = renderFrameSettings.bloomSoftThreshold;
    const float convolutionBloomPrefilterRadius = renderFrameSettings.convolutionBloomPrefilterRadius;

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);

    // RotatedGrid5Tap 경로를 사용하여 작은 화면 영역의 고립된 고휘도 샘플을 Karis weight 로 억제한다.
    bloomParams.params0 = float4(bloomThreshold, bloomSoftThreshold, 0.0f, convolutionBloomPrefilterRadius);

    RHI::ConstantBuffer *bloomConstantBuffer = frameThreadData->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::ConvolutionBloomScreenPrefilter);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess, 0, 0)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, frameBloomPrefilterPSO);
    RHI::renderer->SetTexture(commandList, 0, false, inputTexture);
    RHI::renderer->SetTexture(commandList, 0, true, outputTexture, data.fftBloomConvolutionOutputMipUAVs[0]);
    RHI::renderer->SetConstantBuffer(commandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource, 0, 0)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    if (mipLevelCount > 1) {
        RHI::renderer->GenerateMipChain(commandList, outputTexture, data.fftBloomConvolutionOutputMipSRVs, data.fftBloomConvolutionOutputMipUAVs, false);
    }

    return true;
}

bool Bloom::DispatchConvolutionBloomScreenPrepare(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, const BloomFrameData &data) const {
    PROFILER_CPU_SCOPED_EVENT("Bloom::DispatchConvolutionBloomScreenPrepare");

    if (data.renderWidth == 0 || data.renderHeight == 0 || data.fftBloomSize == 0) {
        return false;
    }

    const uint32_t inputW = data.fftBloomConvolutionOutputWidth;
    const uint32_t inputH = data.fftBloomConvolutionOutputHeight;
    const uint32_t outputW = data.fftBloomSize;
    const uint32_t outputH = data.fftBloomSize;
    const uint32_t mipLevelCount = data.fftBloomConvolutionOutputMipCount;
    const float contentW = BE1::Max(data.fftBloomMappedContentRect.w, 1.0f);
    const float contentH = BE1::Max(data.fftBloomMappedContentRect.h, 1.0f);
    if (inputW == 0 || inputH == 0 || outputW == 0 || outputH == 0 || mipLevelCount == 0) {
        return false;
    }

    const float sourceTexelsPerOutputTexel = BE1::Max(static_cast<float>(inputW) / contentW, static_cast<float>(inputH) / contentH);
    const float screenMipLevel = BE1::Min(BE1::Max(BE1::Math::Log2(sourceTexelsPerOutputTexel), 0.0f), static_cast<float>(mipLevelCount - 1));

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params0.z = screenMipLevel;
    bloomParams.params1 = float4(
        data.fftBloomMappedContentRect.x,
        data.fftBloomMappedContentRect.y,
        data.fftBloomMappedContentRect.w,
        data.fftBloomMappedContentRect.h);

    RHI::ConstantBuffer *bloomConstantBuffer = frameThreadData->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::Texture *inputTexture = data.fftBloomConvolutionOutputTexture;
    RHI::Buffer *outputBuffer = data.fftBloomScreenSpatialBuffer;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::ConvolutionBloomScreenPrepare);

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, frameConvolutionBloomScreenPreparePSO);
    RHI::renderer->SetTexture(commandList, 0, false, inputTexture);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool Bloom::DispatchConvolutionBloomMultiplySpectrum(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuFFT &gpuFFT, GpuTiming &gpuTiming, const BloomFrameData &data, const RenderFrameSettings &renderFrameSettings) const {
    PROFILER_CPU_SCOPED_EVENT("Bloom::DispatchConvolutionBloomMultiplySpectrum");

    if (data.renderWidth == 0 || data.renderHeight == 0 || data.fftBloomSize == 0) {
        return false;
    }

    const uint32_t fftSize = data.fftBloomSize;
    constexpr uint32_t ColorPairCount = 2; // RG, B0

    // padding 때문에 줄어든 kernel footprint 의 면적만큼 커널 에너지를 보정한다.
    const float kernelAreaScale = BE1::Max(data.fftBloomMappedKernelScale.x * data.fftBloomMappedKernelScale.y, 0.000001f);
    const float kernelSampleCount = static_cast<float>(fftSize * fftSize) * kernelAreaScale;
    const float kernelNormalizationFactor = data.fftBloomKernelInverseAverageLuminance / kernelSampleCount;
    const float kernelGain = renderFrameSettings.convolutionBloomKernelGain;
    const float kernelSpectrumScale = kernelGain * kernelNormalizationFactor;

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::ConvolutionBloomMultiplySpectrum);

    return gpuFFT.DispatchMultiplyPackedDualRealSpectrum2D(commandList, frameThreadData,
        data.fftBloomKernelSpectrumBuffer,
        data.fftBloomScreenSpectrumBuffer,
        data.fftBloomWorkBuffer,
        fftSize,
        ColorPairCount,
        kernelSpectrumScale);
}

bool Bloom::DispatchConvolutionBloomResolve(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, GpuTiming &gpuTiming, const BloomFrameData &data) const {
    PROFILER_CPU_SCOPED_EVENT("Bloom::DispatchConvolutionBloomResolve");

    if (data.fftBloomSize == 0) {
        return false;
    }

    const uint32_t inputW = data.renderWidth;
    const uint32_t inputH = data.renderHeight;
    const uint32_t fftSize = data.fftBloomSize;
    const uint32_t outputW = data.fftBloomConvolutionOutputWidth;
    const uint32_t outputH = data.fftBloomConvolutionOutputHeight;
    if (inputW == 0 || inputH == 0 || fftSize == 0 || outputW == 0 || outputH == 0) {
        return false;
    }

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(fftSize, fftSize);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params1 = float4(
        data.fftBloomMappedContentRect.x,
        data.fftBloomMappedContentRect.y,
        data.fftBloomMappedContentRect.w,
        data.fftBloomMappedContentRect.h);

    RHI::ConstantBuffer *bloomConstantBuffer = frameThreadData->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::Buffer *spatialBuffer = data.fftBloomScreenSpatialBuffer;
    RHI::Texture *outputTexture = data.fftBloomConvolutionOutputTexture;

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::ConvolutionBloomResolve);

    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, frameConvolutionBloomResolvePSO);
    RHI::renderer->SetBuffer(commandList, 0, false, spatialBuffer);
    RHI::renderer->SetTexture(commandList, 0, true, outputTexture);
    RHI::renderer->SetConstantBuffer(commandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spatialBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool Bloom::DispatchConvolutionBloomFFT(RHI::CommandList *commandList, GpuFFT &gpuFFT, GpuTiming &gpuTiming, const BloomFrameData &data, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer) const {
    constexpr uint32_t PlaneCount = 2; // RG, B0

    GpuFFT::FFTSize fftSizeEnum;
    if (!GpuFFT::GetGpuFFTSize(data.fftBloomSize, fftSizeEnum)) {
        return false;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::ConvolutionBloomFFT);

    return gpuFFT.DispatchFFTDualReal2D(commandList, inputBuffer, outputBuffer, data.fftBloomWorkBuffer, fftSizeEnum, PlaneCount);
}

bool Bloom::DispatchConvolutionBloomIFFT(RHI::CommandList *commandList, GpuFFT &gpuFFT, GpuTiming &gpuTiming, const BloomFrameData &data, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer) const {
    constexpr uint32_t PlaneCount = 2; // RG, B0

    GpuFFT::FFTSize fftSizeEnum;
    if (!GpuFFT::GetGpuFFTSize(data.fftBloomSize, fftSizeEnum)) {
        return false;
    }

    GpuTiming::ScopedTimer gpuScope(gpuTiming, commandList, GpuTiming::Tag::ConvolutionBloomIFFT);

    return gpuFFT.DispatchIFFTDualReal2D(commandList, inputBuffer, outputBuffer, data.fftBloomScreenSpectrumBuffer, fftSizeEnum, PlaneCount);
}
