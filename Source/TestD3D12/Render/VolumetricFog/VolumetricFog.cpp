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
#include "VolumetricFog.h"
#include "Render/Backend/RenderBackend.h"
#include "Render/RenderInternal.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderPipelineBootstrap.h"
#include "Render/ShaderPipeline/ShaderTemplateManager.h"

void VolumetricFog::Init() {
    const char *baseDir = "Source/TestD3D12/Shaders";
    const RHI::ShaderModel computeShaderModel = RHI::Renderer::MinimumShaderModel();

    injectShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_vfogInject", computeShaderModel, "Fog/VFogInject.hlsl", baseDir);
    integrateShaderTemplate = shaderTemplateManager.CreateComputeShaderTemplate("_vfogIntegrate", computeShaderModel, "Fog/VFogIntegrate.hlsl", baseDir);

    injectPipeline = shaderPipelineCache.GetComputePipeline(injectShaderTemplate, {}, true);
    integratePipeline = shaderPipelineCache.GetComputePipeline(integrateShaderTemplate, {}, true);
}

void VolumetricFog::TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const {
    static constexpr char groupName[] = "VolumetricFog";

    bootstrap.TrackPipeline(groupName, injectPipeline);
    bootstrap.TrackPipeline(groupName, integratePipeline);
}

void VolumetricFog::UpdateFramePSOs() {
    frameInjectPSO = ShaderPipeline::GetReadyPSO(injectPipeline);
    frameIntegratePSO = ShaderPipeline::GetReadyPSO(integratePipeline);
}

void VolumetricFog::Shutdown() {
    injectShaderTemplate.reset();
    integrateShaderTemplate.reset();

    injectPipeline.reset();
    integratePipeline.reset();

    frameInjectPSO = nullptr;
    frameIntegratePSO = nullptr;
}

bool VolumetricFog::Dispatch(RenderBackend &backend, uint32_t localVolumeBoundsBufferIndex) const {
    PROFILER_CPU_SCOPED_EVENT("VolumetricFog::Dispatch");

    const FrameCamera &frameCamera = *backend.GetFrameCamera();
    const FrameVolumetricFog &data = frameCamera.fog.volumetric;

    const bool volumeReady = frameInjectPSO && frameIntegratePSO &&
        data.sourceTexture && data.sourceTexture->GetStatus() == RHI::GPUResourceStatus::Ready && data.sourceTexture->HasValidView(RHI::ViewType::SRV) && data.sourceTexture->HasValidView(RHI::ViewType::UAV) &&
        data.integratedTexture && data.integratedTexture->GetStatus() == RHI::GPUResourceStatus::Ready && data.integratedTexture->HasValidView(RHI::ViewType::SRV) && data.integratedTexture->HasValidView(RHI::ViewType::UAV);
    if (!volumeReady) {
        return false;
    }

    const VFogParams &params = data.vfogParams;
    const uint32_t groupsX = BE1::DivCeil(params.tileCount.x, 8u);
    const uint32_t groupsY = BE1::DivCeil(params.tileCount.y, 8u);

    // Inject scattering, emission and extinction into the fog volume.
    const uint32_t noiseFrameIndex = DispatchInject(backend, groupsX, groupsY, localVolumeBoundsBufferIndex);

    // Integrate scattering, emission and transmittance along each view ray.
    DispatchIntegrate(backend, groupsX, groupsY);

    RHI::renderer->Barrier(backend.GetCommandList(), RHI::Renderer::MakeTextureBarrier(data.integratedTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource));

    // History 를 생성한 viewSerial 과 다음 노이즈 인덱스를 기록한다.
    if (data.historyRenderState) {
        data.historyRenderState->producedSerial = frameCamera.viewSerial;
        data.historyRenderState->nextNoiseFrameIndex = (noiseFrameIndex + 1u) & 63u;
    }
    return true;
}

uint32_t VolumetricFog::DispatchInject(RenderBackend &backend, uint32_t groupsX, uint32_t groupsY, uint32_t localVolumeBoundsBufferIndex) const {
    const FrameCamera &frameCamera = *backend.GetFrameCamera();
    const FrameVolumetricFog &data = frameCamera.fog.volumetric;
    VFogLightingParams lighting = data.vfogLightingParams;
    VFogTemporalParams temporal = data.vfogTemporalParams;
    RHI::CommandList *cmd = backend.GetCommandList();

    GpuTiming::ScopedTimer timer(backend.GetGpuTiming(), cmd, GpuTiming::Tag::VFogInject);

    lighting.localVolumeBoundsBufferIndex = frameCamera.numFrameLocalFogVolumes > 0 && (lighting.flags & VFogFlagLocalVolumeFroxelCulling) != 0 ? localVolumeBoundsBufferIndex : 0;
    if (lighting.localVolumeBoundsBufferIndex == 0) {
        lighting.flags &= ~VFogFlagLocalVolumeFroxelCulling;
    }

    const bool historyProduced = data.historyRenderState && frameCamera.viewSerial > 1 && data.historyRenderState->producedSerial == frameCamera.viewSerial - 1; // 직전 카메라 렌더링에서 history 가 생성됐는지 확인한다.
    const bool historyTextureReady = data.historyTexture && data.historyTexture->GetStatus() == RHI::GPUResourceStatus::Ready && data.historyTexture->HasValidView(RHI::ViewType::SRV);
    temporal.historyValid = temporal.historyValid && historyProduced && historyTextureReady ? 1u : 0u;

    if (temporal.historyValid != 0) {
        temporal.historyTextureIndex = data.historyTexture->GetBindlessIndex(RHI::ViewType::SRV);
        temporal.noiseFrameIndex = data.historyRenderState->nextNoiseFrameIndex;
    } else {
        temporal.noiseFrameIndex = 0;
    }

    const RHI::ConstantBuffer *temporalConstantBuffer = backend.GetFrameThreadData()->AllocConstant(sizeof(temporal), &temporal);

    RHI::renderer->Barrier(cmd, RHI::Renderer::MakeTextureBarrier(data.sourceTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess));
    RHI::renderer->SetPSO(cmd, frameInjectPSO);
    RHI::renderer->SetTexture(cmd, 0, true, data.sourceTexture);
    RHI::renderer->SetConstantBuffer(cmd, 0, backend.GetFrameConstantBuffer());
    RHI::renderer->SetConstantBuffer(cmd, 1, backend.GetCameraConstantBuffer());
    RHI::renderer->SetConstants(cmd, 3, &lighting, sizeof(lighting), 0);
    RHI::renderer->SetConstantBuffer(cmd, 4, temporalConstantBuffer);
    RHI::renderer->Dispatch(cmd, groupsX, groupsY, data.vfogParams.sliceCount);

    return temporal.noiseFrameIndex;
}

void VolumetricFog::DispatchIntegrate(RenderBackend &backend, uint32_t groupsX, uint32_t groupsY) const {
    const FrameVolumetricFog &data = backend.GetFrameCamera()->fog.volumetric;
    RHI::CommandList *cmd = backend.GetCommandList();

    GpuTiming::ScopedTimer timer(backend.GetGpuTiming(), cmd, GpuTiming::Tag::VFogIntegrate);

    const RHI::GPUBarrier barriers[] = {
        RHI::Renderer::MakeTextureBarrier(data.sourceTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(data.integratedTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(cmd, barriers, COUNT_OF(barriers));
    RHI::renderer->SetPSO(cmd, frameIntegratePSO);
    RHI::renderer->SetTexture(cmd, 0, false, data.sourceTexture);
    RHI::renderer->SetTexture(cmd, 0, true, data.integratedTexture);
    RHI::renderer->SetConstantBuffer(cmd, 1, backend.GetCameraConstantBuffer());
    RHI::renderer->Dispatch(cmd, groupsX, groupsY, 1);
}
