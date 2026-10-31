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
#include "GpuSphericalHarmonics.h"
#include "ShaderPipeline/ShaderPipelineBootstrap.h"
#include "ShaderPipeline/ShaderTemplateManager.h"
#include "ShaderPipeline/ShaderPipeline.h"
#include "ShaderPipeline/ShaderPipelineCache.h"
#include "Shaders/ShaderInterop.h"

void GpuSphericalHarmonics::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel shaderModel = RHI::Renderer::MinimumShaderModel();
    constexpr bool asyncBuild = true;

    sh2Resources.coefficientCount = SH2CoefficientCount;
    sh2Resources.projectShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_projectCubemapSH2Shader", shaderModel, "SphericalHarmonics/ProjectCubemapSH.hlsl", baseDir, "CSMain", "SH_BAND_COUNT=2");
    sh2Resources.reduceShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_reduceCubemapSH2Shader", shaderModel, "SphericalHarmonics/ReduceCubemapSH.hlsl", baseDir, "CSMain", "SH_BAND_COUNT=2");
    sh2Resources.reconstructShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_reconstructCubemapSH2Shader", shaderModel, "SphericalHarmonics/ReconstructCubemapSH.hlsl", baseDir, "CSMain", "SH_BAND_COUNT=2");
    sh2Resources.projectPipeline = shaderPipelineCache.GetComputePipeline(sh2Resources.projectShaderTemplate, {}, asyncBuild);
    sh2Resources.reducePipeline = shaderPipelineCache.GetComputePipeline(sh2Resources.reduceShaderTemplate, {}, asyncBuild);
    sh2Resources.reconstructPipeline = shaderPipelineCache.GetComputePipeline(sh2Resources.reconstructShaderTemplate, {}, asyncBuild);

    sh3Resources.coefficientCount = SH3CoefficientCount;
    sh3Resources.projectShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_projectCubemapSH3Shader", shaderModel, "SphericalHarmonics/ProjectCubemapSH.hlsl", baseDir, "CSMain", "SH_BAND_COUNT=3");
    sh3Resources.reduceShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_reduceCubemapSH3Shader", shaderModel, "SphericalHarmonics/ReduceCubemapSH.hlsl", baseDir, "CSMain", "SH_BAND_COUNT=3");
    sh3Resources.reconstructShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_reconstructCubemapSH3Shader", shaderModel, "SphericalHarmonics/ReconstructCubemapSH.hlsl", baseDir, "CSMain", "SH_BAND_COUNT=3");
    sh3Resources.projectPipeline = shaderPipelineCache.GetComputePipeline(sh3Resources.projectShaderTemplate, {}, asyncBuild);
    sh3Resources.reducePipeline = shaderPipelineCache.GetComputePipeline(sh3Resources.reduceShaderTemplate, {}, asyncBuild);
    sh3Resources.reconstructPipeline = shaderPipelineCache.GetComputePipeline(sh3Resources.reconstructShaderTemplate, {}, asyncBuild);

    faceSHCoefficientBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, FaceSH3CoefficientBufferSize, nullptr);
    if (!faceSHCoefficientBuffer) {
        return;
    }

    RHI::renderer->SetName(faceSHCoefficientBuffer, "GpuSphericalHarmonics::FaceSHCoefficientBuffer");
    sh2Resources.faceCoefficientSRV = RHI::renderer->CreateView(faceSHCoefficientBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, SHCoefficientStride, 0, FaceSH2CoefficientBufferSize);
    sh2Resources.faceCoefficientUAV = RHI::renderer->CreateView(faceSHCoefficientBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, SHCoefficientStride, 0, FaceSH2CoefficientBufferSize);
    sh3Resources.faceCoefficientSRV = RHI::renderer->CreateView(faceSHCoefficientBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, SHCoefficientStride, 0, FaceSH3CoefficientBufferSize);
    sh3Resources.faceCoefficientUAV = RHI::renderer->CreateView(faceSHCoefficientBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, SHCoefficientStride, 0, FaceSH3CoefficientBufferSize);

    if (!sh2Resources.faceCoefficientSRV.IsValid() || !sh2Resources.faceCoefficientUAV.IsValid() ||
        !sh3Resources.faceCoefficientSRV.IsValid() || !sh3Resources.faceCoefficientUAV.IsValid()) {
        RHI::renderer->DestroyBuffer(faceSHCoefficientBuffer);
        faceSHCoefficientBuffer = nullptr;
        sh2Resources.faceCoefficientSRV = RHI::ViewID::InvalidID;
        sh2Resources.faceCoefficientUAV = RHI::ViewID::InvalidID;
        sh3Resources.faceCoefficientSRV = RHI::ViewID::InvalidID;
        sh3Resources.faceCoefficientUAV = RHI::ViewID::InvalidID;
        return;
    }

    shCoefficientBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, SH3CoefficientBufferSize, nullptr);
    if (!shCoefficientBuffer) {
        return;
    }

    RHI::renderer->SetName(shCoefficientBuffer, "GpuSphericalHarmonics::SHCoefficientBuffer");
    sh2Resources.coefficientSRV = RHI::renderer->CreateView(shCoefficientBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, SHCoefficientStride, 0, SH2CoefficientBufferSize);
    sh2Resources.coefficientUAV = RHI::renderer->CreateView(shCoefficientBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, SHCoefficientStride, 0, SH2CoefficientBufferSize);
    sh3Resources.coefficientSRV = RHI::renderer->CreateView(shCoefficientBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, SHCoefficientStride, 0, SH3CoefficientBufferSize);
    sh3Resources.coefficientUAV = RHI::renderer->CreateView(shCoefficientBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, SHCoefficientStride, 0, SH3CoefficientBufferSize);

    if (!sh2Resources.coefficientSRV.IsValid() || !sh2Resources.coefficientUAV.IsValid() ||
        !sh3Resources.coefficientSRV.IsValid() || !sh3Resources.coefficientUAV.IsValid()) {
        RHI::renderer->DestroyBuffer(shCoefficientBuffer);
        shCoefficientBuffer = nullptr;
        sh2Resources.coefficientSRV = RHI::ViewID::InvalidID;
        sh2Resources.coefficientUAV = RHI::ViewID::InvalidID;
        sh3Resources.coefficientSRV = RHI::ViewID::InvalidID;
        sh3Resources.coefficientUAV = RHI::ViewID::InvalidID;
    }
}

void GpuSphericalHarmonics::Shutdown() {
    if (shCoefficientBuffer) {
        RHI::renderer->DestroyBuffer(shCoefficientBuffer);
        shCoefficientBuffer = nullptr;
    }
    sh2Resources.coefficientSRV = RHI::ViewID::InvalidID;
    sh2Resources.coefficientUAV = RHI::ViewID::InvalidID;
    sh3Resources.coefficientSRV = RHI::ViewID::InvalidID;
    sh3Resources.coefficientUAV = RHI::ViewID::InvalidID;
    shCoefficientBufferState = RHI::GPUResourceState::Undefined;

    if (faceSHCoefficientBuffer) {
        RHI::renderer->DestroyBuffer(faceSHCoefficientBuffer);
        faceSHCoefficientBuffer = nullptr;
    }
    sh2Resources.faceCoefficientSRV = RHI::ViewID::InvalidID;
    sh2Resources.faceCoefficientUAV = RHI::ViewID::InvalidID;
    sh3Resources.faceCoefficientSRV = RHI::ViewID::InvalidID;
    sh3Resources.faceCoefficientUAV = RHI::ViewID::InvalidID;
    faceSHCoefficientBufferState = RHI::GPUResourceState::Undefined;

    sh2Resources.projectPipeline.reset();
    sh2Resources.reducePipeline.reset();
    sh2Resources.reconstructPipeline.reset();
    sh3Resources.projectPipeline.reset();
    sh3Resources.reducePipeline.reset();
    sh3Resources.reconstructPipeline.reset();

    sh2Resources.projectShaderTemplate.reset();
    sh2Resources.reduceShaderTemplate.reset();
    sh2Resources.reconstructShaderTemplate.reset();
    sh3Resources.projectShaderTemplate.reset();
    sh3Resources.reduceShaderTemplate.reset();
    sh3Resources.reconstructShaderTemplate.reset();

    sh2Resources.coefficientCount = 0;
    sh3Resources.coefficientCount = 0;
}

void GpuSphericalHarmonics::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "GpuSphericalHarmonics";

    bootstrap.TrackPipeline(groupName, sh2Resources.projectPipeline);
    bootstrap.TrackPipeline(groupName, sh2Resources.reducePipeline);
    bootstrap.TrackPipeline(groupName, sh2Resources.reconstructPipeline);
    bootstrap.TrackPipeline(groupName, sh3Resources.projectPipeline);
    bootstrap.TrackPipeline(groupName, sh3Resources.reducePipeline);
    bootstrap.TrackPipeline(groupName, sh3Resources.reconstructPipeline);
}

static bool ValidateCubemapSHInput(const RHI::Texture *cubeMap, RHI::ViewID cubeMapSRV) {
    if (!cubeMap || cubeMap->GetStatus() != RHI::GPUResourceStatus::Ready ||
        cubeMap->type != RHI::TextureType::TextureCube || cubeMap->GetArraySize() != 6 || cubeMap->GetSampleCount() != 1) {
        return false;
    }

    RHI::TextureViewInfo viewInfo;
    return cubeMap->GetViewInfo(RHI::ViewType::SRV, cubeMapSRV, viewInfo) &&
        viewInfo.firstSlice == 0 && viewInfo.sliceCount == 6 && viewInfo.mipCount > 0;
}

static bool ValidateSHCoefficientView(const RHI::Buffer *coefficients, RHI::ViewType viewType, RHI::ViewID viewID, uint32_t coefficientCount) {
    if (!coefficients || coefficients->GetStatus() != RHI::GPUResourceStatus::Ready || coefficientCount == 0) {
        return false;
    }

    RHI::BufferViewInfo viewInfo;
    return coefficients->GetViewInfo(viewType, viewID, viewInfo) &&
        viewInfo.format == RHI::Format::Unknown && !viewInfo.raw &&
        viewInfo.elementCount == coefficientCount &&
        viewInfo.structureByteStride == GpuSphericalHarmonics::SHCoefficientStride;
}

static bool ValidateCubemapSHOutput(const RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, RHI::TextureViewInfo &outViewInfo) {
    if (!outputCubeMap || outputCubeMap->GetStatus() != RHI::GPUResourceStatus::Ready ||
        outputCubeMap->type != RHI::TextureType::TextureCube || outputCubeMap->GetArraySize() != 6 || outputCubeMap->GetSampleCount() != 1) {
        return false;
    }

    return outputCubeMap->GetViewInfo(RHI::ViewType::UAV, outputUAV, outViewInfo) &&
        outViewInfo.firstSlice == 0 && outViewInfo.sliceCount == 6 && outViewInfo.mipCount == 1;
}

bool GpuSphericalHarmonics::ProjectCubemapSH2(RHI::CommandList *commandList, const RHI::Texture *cubeMap, RHI::Buffer *outputCoefficients, RHI::ViewID outputUAV, RHI::ViewID cubeMapSRV) {
    PROFILER_CPU_SCOPED_EVENT("GpuSphericalHarmonics::ProjectCubemapSH2");

    return ProjectCubemapSH(commandList, cubeMap, outputCoefficients, outputUAV, cubeMapSRV, sh2Resources);
}

bool GpuSphericalHarmonics::ProjectCubemapSH3(RHI::CommandList *commandList, const RHI::Texture *cubeMap, RHI::Buffer *outputCoefficients, RHI::ViewID outputUAV, RHI::ViewID cubeMapSRV) {
    PROFILER_CPU_SCOPED_EVENT("GpuSphericalHarmonics::ProjectCubemapSH3");

    return ProjectCubemapSH(commandList, cubeMap, outputCoefficients, outputUAV, cubeMapSRV, sh3Resources);
}

bool GpuSphericalHarmonics::ReconstructCubemapSH2(RHI::CommandList *commandList, const RHI::Buffer *inputCoefficients, RHI::ViewID inputSRV, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec2 &bandFactors) {
    PROFILER_CPU_SCOPED_EVENT("GpuSphericalHarmonics::ReconstructCubemapSH2");

    return ReconstructCubemapSH(commandList, inputCoefficients, inputSRV, outputCubeMap, outputUAV,
        BE1::Vec3(bandFactors.x, bandFactors.y, 0.0f), sh2Resources);
}

bool GpuSphericalHarmonics::ReconstructCubemapSH3(RHI::CommandList *commandList, const RHI::Buffer *inputCoefficients, RHI::ViewID inputSRV, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec3 &bandFactors) {
    PROFILER_CPU_SCOPED_EVENT("GpuSphericalHarmonics::ReconstructCubemapSH3");

    return ReconstructCubemapSH(commandList, inputCoefficients, inputSRV, outputCubeMap, outputUAV, bandFactors, sh3Resources);
}

bool GpuSphericalHarmonics::FilterCubemapSH2(RHI::CommandList *commandList, const RHI::Texture *inputCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec2 &bandFactors, RHI::ViewID inputSRV) {
    PROFILER_CPU_SCOPED_EVENT("GpuSphericalHarmonics::FilterCubemapSH2");

    return FilterCubemapSH(commandList, inputCubeMap, outputCubeMap, outputUAV,
        BE1::Vec3(bandFactors.x, bandFactors.y, 0.0f), inputSRV, sh2Resources);
}

bool GpuSphericalHarmonics::FilterCubemapSH3(RHI::CommandList *commandList, const RHI::Texture *inputCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec3 &bandFactors, RHI::ViewID inputSRV) {
    PROFILER_CPU_SCOPED_EVENT("GpuSphericalHarmonics::FilterCubemapSH3");

    return FilterCubemapSH(commandList, inputCubeMap, outputCubeMap, outputUAV, bandFactors, inputSRV, sh3Resources);
}

bool GpuSphericalHarmonics::ProjectCubemapSH(RHI::CommandList *commandList, const RHI::Texture *cubeMap, RHI::Buffer *outputCoefficients, RHI::ViewID outputUAV, RHI::ViewID cubeMapSRV, const SHBandResources &bandResources) {
    if (!ValidateCubemapSHInput(cubeMap, cubeMapSRV) ||
        !ValidateSHCoefficientView(outputCoefficients, RHI::ViewType::UAV, outputUAV, bandResources.coefficientCount) ||
        !faceSHCoefficientBuffer) {
        return false;
    }

    RHI::PipelineState *projectPSO = ShaderPipeline::GetReadyPSO(bandResources.projectPipeline);
    RHI::PipelineState *reducePSO = ShaderPipeline::GetReadyPSO(bandResources.reducePipeline);
    if (!projectPSO || !reducePSO) {
        return false;
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(faceSHCoefficientBuffer, faceSHCoefficientBufferState, RHI::GPUResourceState::UnorderedAccess));

    RHI::renderer->SetPSO(commandList, projectPSO);
    RHI::renderer->SetTexture(commandList, 0, false, cubeMap, cubeMapSRV);
    RHI::renderer->SetBuffer(commandList, 0, true, faceSHCoefficientBuffer, bandResources.faceCoefficientUAV);
    RHI::renderer->Dispatch(commandList, 6, 1, 1);

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(faceSHCoefficientBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute));

    RHI::renderer->SetPSO(commandList, reducePSO);
    RHI::renderer->SetBuffer(commandList, 0, false, faceSHCoefficientBuffer, bandResources.faceCoefficientSRV);
    RHI::renderer->SetBuffer(commandList, 0, true, outputCoefficients, outputUAV);
    RHI::renderer->Dispatch(commandList, 1, 1, 1);

    faceSHCoefficientBufferState = RHI::GPUResourceState::ShaderResourceCompute;
    return true;
}

bool GpuSphericalHarmonics::ReconstructCubemapSH(RHI::CommandList *commandList, const RHI::Buffer *inputCoefficients, RHI::ViewID inputSRV, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec3 &bandFactors, const SHBandResources &bandResources) {
    RHI::TextureViewInfo outputViewInfo;
    if (!ValidateSHCoefficientView(inputCoefficients, RHI::ViewType::SRV, inputSRV, bandResources.coefficientCount) ||
        !ValidateCubemapSHOutput(outputCubeMap, outputUAV, outputViewInfo)) {
        return false;
    }

    RHI::PipelineState *pso = ShaderPipeline::GetReadyPSO(bandResources.reconstructPipeline);
    if (!pso) {
        return false;
    }

    ReconstructCubemapSHParams params = {};
    params.bandFactors = float3(bandFactors);

    RHI::renderer->SetPSO(commandList, pso);
    RHI::renderer->SetBuffer(commandList, 0, false, inputCoefficients, inputSRV);
    RHI::renderer->SetTexture(commandList, 0, true, outputCubeMap, outputUAV);
    RHI::renderer->SetConstants(commandList, 0, &params, sizeof(params), 0);

    const uint32_t threadGroupCountX = BE1::DivCeil(outputViewInfo.width, IBLIntegrationBlockSizeX);
    const uint32_t threadGroupCountY = BE1::DivCeil(outputViewInfo.height, IBLIntegrationBlockSizeY);
    RHI::renderer->Dispatch(commandList, threadGroupCountX, threadGroupCountY, 6);
    return true;
}

bool GpuSphericalHarmonics::FilterCubemapSH(RHI::CommandList *commandList, const RHI::Texture *inputCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec3 &bandFactors, RHI::ViewID inputSRV, const SHBandResources &bandResources) {
    RHI::TextureViewInfo outputViewInfo;
    if (inputCubeMap == outputCubeMap || !ValidateCubemapSHInput(inputCubeMap, inputSRV) ||
        !ValidateCubemapSHOutput(outputCubeMap, outputUAV, outputViewInfo) ||
        !shCoefficientBuffer) {
        return false;
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(shCoefficientBuffer, shCoefficientBufferState, RHI::GPUResourceState::UnorderedAccess));
    shCoefficientBufferState = RHI::GPUResourceState::UnorderedAccess;

    if (!ProjectCubemapSH(commandList, inputCubeMap, shCoefficientBuffer, bandResources.coefficientUAV, inputSRV, bandResources)) {
        return false;
    }

    RHI::renderer->Barrier(commandList, RHI::Renderer::MakeBufferBarrier(shCoefficientBuffer, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute));
    shCoefficientBufferState = RHI::GPUResourceState::ShaderResourceCompute;

    return ReconstructCubemapSH(commandList, shCoefficientBuffer, bandResources.coefficientSRV, outputCubeMap, outputUAV, bandFactors, bandResources);
}
