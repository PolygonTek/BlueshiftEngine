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
#include "ConvolutionBloomKernelGenerator.h"
#include "Render/Texture/Texture.h"
#include "GpuFFT.h"
#include "RenderSystem.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"
#include "Shaders/ShaderInterop.h"

void ConvolutionBloomKernelGenerator::Init() {
    pipelinesReady = false;
    pipelineBuildFailed = false;

    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    aperturePrepareShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_convolutionBloomAperturePrepareShader", computeShaderModel, "Bloom/ConvolutionBloomAperturePrepare.hlsl", baseDir);
    reduceApertureEnergyShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_convolutionBloomReduceApertureEnergyShader", computeShaderModel, "Bloom/ConvolutionBloomReduceApertureEnergy.hlsl", baseDir);
    resolveDiffractionPSFShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_convolutionBloomResolveDiffractionPSFShader", computeShaderModel, "Bloom/ConvolutionBloomResolveDiffractionPSF.hlsl", baseDir);
    applyDispersionShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_convolutionBloomApplyDispersionShader", computeShaderModel, "Bloom/ConvolutionBloomApplyDispersion.hlsl", baseDir);

    static constexpr bool asyncPipelineBuild = true;
    aperturePreparePipeline = shaderPipelineCache.GetComputePipeline(aperturePrepareShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    reduceApertureEnergyPipeline = shaderPipelineCache.GetComputePipeline(reduceApertureEnergyShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    resolveDiffractionPSFPipeline = shaderPipelineCache.GetComputePipeline(resolveDiffractionPSFShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);
    applyDispersionPipeline = shaderPipelineCache.GetComputePipeline(applyDispersionShaderTemplate, ShaderPermutationKey(), asyncPipelineBuild);

    EnsureResources();
}

void ConvolutionBloomKernelGenerator::UpdatePipelineBuildState() {
    if (pipelinesReady || pipelineBuildFailed) {
        return;
    }

    const ShaderPipeline *pipelines[] = {
        aperturePreparePipeline.get(),
        reduceApertureEnergyPipeline.get(),
        resolveDiffractionPSFPipeline.get(),
        applyDispersionPipeline.get()
    };

    bool allReady = true;

    for (const ShaderPipeline *pipeline : pipelines) {
        if (!pipeline) {
            pipelineBuildFailed = true;
            break;
        }

        switch (pipeline->GetState()) {
        case ShaderPipeline::State::Ready:
        case ShaderPipeline::State::Rebuilding:
            break;
        case ShaderPipeline::State::Building:
            allReady = false;
            break;
        case ShaderPipeline::State::Empty:
        case ShaderPipeline::State::Failed:
            pipelineBuildFailed = true;
            break;
        }

        if (pipelineBuildFailed) {
            break;
        }
    }

    if (pipelineBuildFailed) {
        BE_WARNLOG("ConvolutionBloomKernelGenerator::UpdatePipelineBuildState: pipeline build failed\n");
        return;
    }

    pipelinesReady = allReady;
}

void ConvolutionBloomKernelGenerator::Shutdown() {
    pipelinesReady = false;
    pipelineBuildFailed = false;

    DestroyResources();
    apertureSourceTexture.reset();

    aperturePreparePipeline.reset();
    reduceApertureEnergyPipeline.reset();
    resolveDiffractionPSFPipeline.reset();
    applyDispersionPipeline.reset();

    aperturePrepareShaderTemplate.reset();
    reduceApertureEnergyShaderTemplate.reset();
    resolveDiffractionPSFShaderTemplate.reset();
    applyDispersionShaderTemplate.reset();
}

void ConvolutionBloomKernelGenerator::SetApertureTexture(const SharedPtr<Texture> &texture) {
    if (apertureSourceTexture == texture) {
        return;
    }

    apertureSourceTexture = texture;
    kernelDirty = true;
}

void ConvolutionBloomKernelGenerator::SetApertureScale(float scale) {
    scale = BE1::Min(BE1::Max(scale, 0.02f), 1.0f);
    if (apertureScale == scale) {
        return;
    }

    apertureScale = scale;
    kernelDirty = true;
}

void ConvolutionBloomKernelGenerator::SetDiffractionPSFScale(float scale) {
    scale = BE1::Max(scale, 0.0f);
    if (diffractionPSFScale == scale) {
        return;
    }

    diffractionPSFScale = scale;
    kernelDirty = true;
}

void ConvolutionBloomKernelGenerator::SetDispersionAmount(float amount) {
    amount = BE1::Max(amount, 0.0f);
    if (dispersionAmount == amount) {
        return;
    }

    dispersionAmount = amount;
    kernelDirty = true;
}

bool ConvolutionBloomKernelGenerator::GenerateKernel(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData) {
    PROFILER_CPU_SCOPED_EVENT("ConvolutionBloomKernelGenerator::GenerateKernel");

    if (!kernelDirty && HasResources()) {
        return true;
    }

    if (!apertureSourceTexture || !apertureSourceTexture->IsReadyForRendering()) {
        return false;
    }

    if (!EnsureResources()) {
        return false;
    }

    // aperture 버퍼 준비 (apertureSpatialBuffer 에 출력)
    if (!DispatchAperturePrepare(commandList, frameThreadData)) {
        return false;
    }

    RHI::Buffer *apertureEnergyBuffer = nullptr;
    if (!DispatchReduceApertureEnergy(commandList, frameThreadData, &apertureEnergyBuffer)) {
        return false;
    }

    GpuFFT::FFTSize fftSizeEnum;
    if (!GpuFFT::GetGpuFFTSize(KernelSize, fftSizeEnum)) {
        return false;
    }

    // aperture 버퍼 FFT 하여 회절 (diffraction) 적용
    if (!renderSystem->GetGpuFFT().DispatchFFTComplex2D(commandList, frameThreadData, apertureSpatialBuffer, apertureSpectrumBuffer, workBuffer, fftSizeEnum, 1)) {
        return false;
    }

    // circular shift 와 abs(FFT)^2 을 적용해서 intensity 를 표현한다.
    if (!DispatchResolveDiffractionPSF(commandList, frameThreadData, apertureEnergyBuffer)) {
        return false;
    }

    // 분산 (dispersion) 적용
    if (!DispatchApplyDispersion(commandList, frameThreadData)) {
        return false;
    }

    kernelDirty = false;
    return true;
}

bool ConvolutionBloomKernelGenerator::ReadKernelImage(BE1::Image &outImage) const {
    PROFILER_CPU_SCOPED_EVENT("ConvolutionBloomKernelGenerator::ReadKernelImage");

    if (kernelDirty || !kernelTexture) {
        return false;
    }

    const uint32_t width = kernelTexture->GetWidth();
    const uint32_t height = kernelTexture->GetHeight();
    if (width == 0 || height == 0) {
        return false;
    }

    static constexpr BE1::Image::Format KernelImageFormat = BE1::Image::Format::R16G16B16A16_FLOAT;

    outImage.Create2D(width, height, 1, KernelImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    if (!RHI::renderer->GetTextureImage2D(kernelTexture, 0, KernelImageFormat, outImage.GetPixels())) {
        outImage.Clear();
        return false;
    }

    return true;
}

static RHI::Buffer *CreateGeneratorStructuredBuffer(const char *debugName, uint64_t elementCount) {
    if (elementCount == 0 || elementCount > UINT64_MAX / sizeof(float2)) {
        return nullptr;
    }

    const uint64_t bufferSize = elementCount * sizeof(float2);
    RHI::Buffer *buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, bufferSize, nullptr);
    if (!buffer) {
        BE_WARNLOG("ConvolutionBloomKernelGenerator::EnsureResources: failed to create %s\n", debugName);
        return nullptr;
    }

    RHI::renderer->SetName(buffer, debugName);

    RHI::ViewID srvViewID = RHI::renderer->CreateView(buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(float2), 0, bufferSize);
    if (!srvViewID.IsValid()) {
        BE_WARNLOG("ConvolutionBloomKernelGenerator::EnsureResources: failed to create %s SRV\n", debugName);
        RHI::renderer->DestroyBuffer(buffer);
        return nullptr;
    }

    RHI::ViewID uavViewID = RHI::renderer->CreateView(buffer, RHI::ViewType::UAV, RHI::Format::Unknown, sizeof(float2), 0, bufferSize);
    if (!uavViewID.IsValid()) {
        BE_WARNLOG("ConvolutionBloomKernelGenerator::EnsureResources: failed to create %s UAV\n", debugName);
        RHI::renderer->DestroyBuffer(buffer);
        return nullptr;
    }

    return buffer;
}

static RHI::Texture *CreateGeneratorTexture(const char *debugName, uint32_t size, BE1::Image::Format imageFormat) {
    if (size == 0) {
        return nullptr;
    }

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->InitFromMemory(size, size, 1, 1, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    RHI::Texture *texture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!texture) {
        BE_WARNLOG("ConvolutionBloomKernelGenerator::EnsureResources: failed to create %s\n", debugName);
        return nullptr;
    }

    RHI::renderer->SetName(texture, debugName);
    return texture;
}

bool ConvolutionBloomKernelGenerator::EnsureResources() {
    if (HasResources()) {
        return true;
    }

    DestroyResources();

    const uint64_t pixelCount = uint64_t(KernelSize) * uint64_t(KernelSize);

    apertureSpatialBuffer = CreateGeneratorStructuredBuffer("ConvolutionBloomKernelGenerator::apertureSpatialBuffer", pixelCount);
    if (!apertureSpatialBuffer) {
        DestroyResources();
        return false;
    }

    apertureSpectrumBuffer = CreateGeneratorStructuredBuffer("ConvolutionBloomKernelGenerator::apertureSpectrumBuffer", pixelCount);
    if (!apertureSpectrumBuffer) {
        DestroyResources();
        return false;
    }

    workBuffer = CreateGeneratorStructuredBuffer("ConvolutionBloomKernelGenerator::workBuffer", pixelCount);
    if (!workBuffer) {
        DestroyResources();
        return false;
    }

    const uint64_t apertureEnergyElementCount = BE1::DivCeil(pixelCount, ConvolutionBloomApertureEnergyReduceBlockSize);
    apertureEnergyBuffer0 = CreateGeneratorStructuredBuffer("ConvolutionBloomKernelGenerator::apertureEnergyBuffer0", apertureEnergyElementCount);
    if (!apertureEnergyBuffer0) {
        DestroyResources();
        return false;
    }

    apertureEnergyBuffer1 = CreateGeneratorStructuredBuffer("ConvolutionBloomKernelGenerator::apertureEnergyBuffer1", apertureEnergyElementCount);
    if (!apertureEnergyBuffer1) {
        DestroyResources();
        return false;
    }

    const BE1::Image::Format diffractionPSFFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16_FLOAT);
    diffractionPSFTexture = CreateGeneratorTexture("ConvolutionBloomKernelGenerator::diffractionPSFTexture", KernelSize, diffractionPSFFormat);
    if (!diffractionPSFTexture) {
        DestroyResources();
        return false;
    }

    const BE1::Image::Format kernelFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    kernelTexture = CreateGeneratorTexture("ConvolutionBloomKernelGenerator::kernelTexture", KernelSize, kernelFormat);
    if (!kernelTexture) {
        DestroyResources();
        return false;
    }

    kernelDirty = true;
    return true;
}

void ConvolutionBloomKernelGenerator::DestroyResources() {
    if (apertureSpatialBuffer) {
        RHI::renderer->DestroyBuffer(apertureSpatialBuffer);
        apertureSpatialBuffer = nullptr;
    }
    if (apertureSpectrumBuffer) {
        RHI::renderer->DestroyBuffer(apertureSpectrumBuffer);
        apertureSpectrumBuffer = nullptr;
    }
    if (workBuffer) {
        RHI::renderer->DestroyBuffer(workBuffer);
        workBuffer = nullptr;
    }
    if (apertureEnergyBuffer0) {
        RHI::renderer->DestroyBuffer(apertureEnergyBuffer0);
        apertureEnergyBuffer0 = nullptr;
    }
    if (apertureEnergyBuffer1) {
        RHI::renderer->DestroyBuffer(apertureEnergyBuffer1);
        apertureEnergyBuffer1 = nullptr;
    }
    if (diffractionPSFTexture) {
        RHI::renderer->DestroyTexture(diffractionPSFTexture);
        diffractionPSFTexture = nullptr;
    }
    if (kernelTexture) {
        RHI::renderer->DestroyTexture(kernelTexture);
        kernelTexture = nullptr;
    }

    kernelDirty = true;
}

bool ConvolutionBloomKernelGenerator::HasResources() const {
    return apertureSpatialBuffer && apertureSpectrumBuffer && workBuffer &&
        apertureEnergyBuffer0 && apertureEnergyBuffer1 &&
        diffractionPSFTexture && kernelTexture;
}

bool ConvolutionBloomKernelGenerator::DispatchAperturePrepare(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData) {
    PROFILER_CPU_SCOPED_EVENT("ConvolutionBloomKernelGenerator::DispatchAperturePrepare");

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(aperturePreparePipeline);
    if (!pso) {
        return false;
    }

    RHI::Texture *sourceTexture = apertureSourceTexture ? apertureSourceTexture->GetRHITexture() : nullptr;
    RHI::Buffer *outputBuffer = apertureSpatialBuffer;
    if (!sourceTexture || !outputBuffer) {
        return false;
    }

    const uint32_t outputW = KernelSize;
    const uint32_t outputH = KernelSize;
    const uint32_t inputW = sourceTexture->GetWidth();
    const uint32_t inputH = sourceTexture->GetHeight();
    if (inputW == 0 || inputH == 0) {
        return false;
    }

    const float clampedApertureScale = BE1::Max(apertureScale, 0.0001f);
    const float sourceTexelsPerOutputTexelX = static_cast<float>(inputW) / (static_cast<float>(outputW) * clampedApertureScale);
    const float sourceTexelsPerOutputTexelY = static_cast<float>(inputH) / (static_cast<float>(outputH) * clampedApertureScale);
    const float sourceTexelsPerOutputTexel = BE1::Max(sourceTexelsPerOutputTexelX, sourceTexelsPerOutputTexelY);
    const float apertureMipLevel = BE1::Max(BE1::Math::Log(2.0f, sourceTexelsPerOutputTexel), 0.0f);

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params0 = float4(clampedApertureScale, apertureMipLevel, 0.0f, 0.0f);

    RHI::ConstantBuffer *bloomConstantBuffer = frameThreadData->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(sourceTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, false, sourceTexture);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(sourceTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::Undefined)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool ConvolutionBloomKernelGenerator::DispatchReduceApertureEnergy(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer **outEnergyBuffer) {
    PROFILER_CPU_SCOPED_EVENT("ConvolutionBloomKernelGenerator::DispatchReduceApertureEnergy");

    if (!outEnergyBuffer) {
        return false;
    }

    RHI::Buffer *energyBuffers[2] = {
        apertureEnergyBuffer0,
        apertureEnergyBuffer1
    };

    if (!apertureSpatialBuffer || !energyBuffers[0] || !energyBuffers[1]) {
        return false;
    }

    const uint64_t pixelCount = uint64_t(KernelSize) * uint64_t(KernelSize);
    uint32_t inputElementCount = static_cast<uint32_t>(pixelCount);
    RHI::Buffer *inputBuffer = apertureSpatialBuffer;
    RHI::GPUResourceState inputState = RHI::GPUResourceState::Undefined;
    bool inputIsPartialEnergy = false;
    uint32_t outputBufferIndex = 0;

    for (;;) {
        RHI::Buffer *outputBuffer = energyBuffers[outputBufferIndex];
        if (!DispatchReduceApertureEnergyPass(commandList, frameThreadData, inputBuffer, outputBuffer, inputElementCount, inputIsPartialEnergy, inputState)) {
            return false;
        }

        const uint32_t outputElementCount = BE1::DivCeil(inputElementCount, ConvolutionBloomApertureEnergyReduceBlockSize);
        if (outputElementCount <= 1) {
            *outEnergyBuffer = outputBuffer;
            return true;
        }

        inputBuffer = outputBuffer;
        inputState = RHI::GPUResourceState::UnorderedAccess;
        inputIsPartialEnergy = true;
        inputElementCount = outputElementCount;
        outputBufferIndex ^= 1;
    }
}

bool ConvolutionBloomKernelGenerator::DispatchReduceApertureEnergyPass(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t inputElementCount, bool inputIsPartialEnergy, RHI::GPUResourceState inputState) {
    PROFILER_CPU_SCOPED_EVENT("ConvolutionBloomKernelGenerator::DispatchReduceApertureEnergyPass");

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(reduceApertureEnergyPipeline);
    if (!pso || !inputBuffer || !outputBuffer || inputBuffer == outputBuffer || inputElementCount == 0) {
        return false;
    }

    const uint32_t outputElementCount = BE1::DivCeil(inputElementCount, ConvolutionBloomApertureEnergyReduceBlockSize);
    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputElementCount, inputIsPartialEnergy ? 1u : 0u);
    bloomParams.outputSize = uint2(outputElementCount, 1);

    RHI::ConstantBuffer *bloomConstantBuffer = frameThreadData->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(inputBuffer, inputState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(outputBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, inputBuffer);
    RHI::renderer->SetBuffer(commandList, 0, true, outputBuffer);
    RHI::renderer->SetConstantBuffer(commandList, 0, bloomConstantBuffer);
    RHI::renderer->Dispatch(commandList, outputElementCount, 1, 1);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(inputBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined));

    return true;
}

bool ConvolutionBloomKernelGenerator::DispatchResolveDiffractionPSF(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData, RHI::Buffer *apertureEnergyBuffer) {
    PROFILER_CPU_SCOPED_EVENT("ConvolutionBloomKernelGenerator::DispatchResolveDiffractionPSF");

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(resolveDiffractionPSFPipeline);
    if (!pso) {
        return false;
    }

    RHI::Buffer *spectrumBuffer = apertureSpectrumBuffer;
    RHI::Texture *outputTexture = diffractionPSFTexture;
    if (!spectrumBuffer || !apertureEnergyBuffer || !outputTexture) {
        return false;
    }

    const uint32_t inputW = KernelSize;
    const uint32_t inputH = KernelSize;
    const uint32_t outputW = outputTexture->GetWidth();
    const uint32_t outputH = outputTexture->GetHeight();
    if (outputW == 0 || outputH == 0) {
        return false;
    }

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params0 = float4(BE1::Max(diffractionPSFScale, 0.0f), 0.0f, 0.0f, 0.0f);

    RHI::ConstantBuffer *bloomConstantBuffer = frameThreadData->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spectrumBuffer, RHI::GPUResourceState::Undefined, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeBufferBarrier(apertureEnergyBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, spectrumBuffer);
    RHI::renderer->SetBuffer(commandList, 1, false, apertureEnergyBuffer);
    RHI::renderer->SetTexture(commandList, 0, true, outputTexture);
    RHI::renderer->SetConstantBuffer(commandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeBufferBarrier(spectrumBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeBufferBarrier(apertureEnergyBuffer, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::Undefined),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}

bool ConvolutionBloomKernelGenerator::DispatchApplyDispersion(RHI::CommandList *commandList, RHI::FrameThreadData *frameThreadData) {
    PROFILER_CPU_SCOPED_EVENT("ConvolutionBloomKernelGenerator::DispatchApplyDispersion");

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(applyDispersionPipeline);
    if (!pso) {
        return false;
    }

    RHI::Texture *inputTexture = diffractionPSFTexture;
    RHI::Texture *outputTexture = kernelTexture;
    if (!inputTexture || !outputTexture) {
        return false;
    }

    const uint32_t inputW = inputTexture->GetWidth();
    const uint32_t inputH = inputTexture->GetHeight();
    const uint32_t outputW = outputTexture->GetWidth();
    const uint32_t outputH = outputTexture->GetHeight();
    if (inputW == 0 || inputH == 0 || outputW == 0 || outputH == 0) {
        return false;
    }

    static constexpr float MinWavelengthNm = 380.0f;
    static constexpr float MaxWavelengthNm = 780.0f;
    static constexpr float ReferenceWavelengthNm = 550.0f;
    static constexpr float DispersionSampleCount = 81.0f;
    static constexpr float IntensityScale = 1.0f;

    BloomParams bloomParams = {};
    bloomParams.inputSize = uint2(inputW, inputH);
    bloomParams.outputSize = uint2(outputW, outputH);
    bloomParams.inputTexelSize = float2(1.0f / inputW, 1.0f / inputH);
    bloomParams.outputTexelSize = float2(1.0f / outputW, 1.0f / outputH);
    bloomParams.params0 = float4(BE1::Max(dispersionAmount, 0.0f), MinWavelengthNm, MaxWavelengthNm, ReferenceWavelengthNm);
    bloomParams.params1 = float4(DispersionSampleCount, IntensityScale, 0.0f, 0.0f);

    RHI::ConstantBuffer *bloomConstantBuffer = frameThreadData->AllocConstant(sizeof(BloomParams), &bloomParams);
    if (!bloomConstantBuffer) {
        return false;
    }

    RHI::GPUBarrier startBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(commandList, startBarriers, COUNT_OF(startBarriers));

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetTexture(commandList, 0, false, inputTexture);
    RHI::renderer->SetTexture(commandList, 0, true, outputTexture);
    RHI::renderer->SetConstantBuffer(commandList, 0, bloomConstantBuffer);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputW, BloomBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputH, BloomBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(outputTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource)
    };
    RHI::renderer->Barrier(commandList, endBarriers, COUNT_OF(endBarriers));

    return true;
}
