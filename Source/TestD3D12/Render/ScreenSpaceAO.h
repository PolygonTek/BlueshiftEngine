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

#pragma once

#include "RHI/RHI.h"
#include "Render/RenderFrameSettings.h"

class GpuTiming;
class ShaderTemplate;
class ShaderPipeline;
class ShaderPipelineBootstrap;
struct ScreenSpaceAOFrameData;

class ScreenSpaceAO {
public:
    struct DispatchParams {
        bool                        useHalfResolution = false;
        bool                        useBlur = false;
        uint32_t                    denoisePassCount = 1;
    };

    void                            Init();
    void                            Shutdown();

    void                            TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const;

    void                            UpdateFramePSOs(const RenderFrameSettings &settings);
    static bool                     HasRequiredResources(const ScreenSpaceAOFrameData &data);

                                    // Requires successful HasRequiredResources() and resources prepared for these settings.
    bool                            PrepareDispatch(const ScreenSpaceAOFrameData &data, const RenderFrameSettings &settings, DispatchParams &params) const;

                                    // Requires successful PrepareDispatch() and ready single-sample inputs.
                                    // Inputs and scratch resources enter and leave in ShaderResource state.
                                    // Records commands only; does not submit or select a command queue.
    void                            Dispatch(RHI::CommandList *commandList, const RHI::ConstantBuffer *cameraConstantBuffer, GpuTiming &gpuTiming, uint32_t frameIndex, const ScreenSpaceAOFrameData &data, const RenderFrameSettings &settings, const DispatchParams &params) const;
    bool                            DispatchDebugPreview(RHI::CommandList *commandList, const ScreenSpaceAOFrameData &data, const RenderFrameSettings &settings) const;

private:
    void                            DispatchSSAO(RHI::CommandList *commandList, const RHI::ConstantBuffer *cameraConstantBuffer, GpuTiming &gpuTiming, uint32_t frameIndex, const ScreenSpaceAOFrameData &data, const RenderFrameSettings &settings, const DispatchParams &params) const;
    void                            DispatchGTAO(RHI::CommandList *commandList, const RHI::ConstantBuffer *cameraConstantBuffer, GpuTiming &gpuTiming, uint32_t frameIndex, const ScreenSpaceAOFrameData &data, const RenderFrameSettings &settings, const DispatchParams &params) const;
    void                            DispatchGTAOPrefilterDepth(RHI::CommandList *commandList, const RHI::ConstantBuffer *cameraConstantBuffer, GpuTiming &gpuTiming, const ScreenSpaceAOFrameData &data, const RenderFrameSettings &settings, RHI::GPUResourceState depthInputState, bool useHalfResolution) const;
    void                            DispatchGTAOMain(RHI::CommandList *commandList, const RHI::ConstantBuffer *cameraConstantBuffer, GpuTiming &gpuTiming, uint32_t frameIndex, const ScreenSpaceAOFrameData &data, const RenderFrameSettings &settings, bool useHalfResolution) const;
    RHI::Texture *                  DispatchGTAODenoise(RHI::CommandList *commandList, GpuTiming &gpuTiming, const ScreenSpaceAOFrameData &data, const RenderFrameSettings &settings, uint32_t denoisePassCount, bool useHalfResolution, RHI::GPUResourceState &rawAOState, RHI::GPUResourceState &tempAOState) const;
    void                            DispatchSSAOCompute(RHI::CommandList *commandList, const RHI::ConstantBuffer *cameraConstantBuffer, GpuTiming &gpuTiming, uint32_t frameIndex, const ScreenSpaceAOFrameData &data, const RenderFrameSettings &settings, RHI::PipelineState *pso, RHI::Texture *depthTexture, RHI::Texture *normalTexture, RHI::Texture *outputAOTexture, RHI::Texture *outputHalfDepthTexture, RHI::GPUResourceState depthInputState) const;
    void                            DispatchAOBilateralBlur(RHI::CommandList *commandList, const RHI::ConstantBuffer *cameraConstantBuffer, GpuTiming &gpuTiming, const ScreenSpaceAOFrameData &data, RHI::Texture *depthTexture, RHI::Texture *inputAOTexture, RHI::Texture *tempAOTexture, RHI::Texture *outputAOTexture, float radius) const;
    void                            DispatchAOUpsample(RHI::CommandList *commandList, const RHI::ConstantBuffer *cameraConstantBuffer, GpuTiming &gpuTiming, const ScreenSpaceAOFrameData &data, RHI::Texture *depthTexture, RHI::Texture *inputHalfAOTexture, RHI::Texture *inputHalfDepthTexture, RHI::Texture *outputAOTexture, float radius) const;

                                    // Ambient Occlusion (Common) shader templates
    SharedPtr<ShaderTemplate>       aoBilateralHorizontalShaderTemplate;
    SharedPtr<ShaderTemplate>       aoBilateralVerticalShaderTemplate;
    SharedPtr<ShaderTemplate>       aoUpsampleShaderTemplate;

                                    // SSAO shader templates
    SharedPtr<ShaderTemplate>       ssao16FullResShaderTemplate;
    SharedPtr<ShaderTemplate>       ssao16HalfResShaderTemplate;
    SharedPtr<ShaderTemplate>       ssao8FullResShaderTemplate;
    SharedPtr<ShaderTemplate>       ssao8HalfResShaderTemplate;

                                    // GTAO shader templates
    SharedPtr<ShaderTemplate>       gtaoPrefilterShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoMainLowShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoMainMediumShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoMainHighShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoMainUltraShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoDenoiseShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoDebugShaderTemplate;

                                    // Ambient Occlusion (Common)
    SharedPtr<ShaderPipeline>       aoBilateralHorizontalPipeline;
    SharedPtr<ShaderPipeline>       aoBilateralVerticalPipeline;
    SharedPtr<ShaderPipeline>       aoUpsamplePipeline;

                                    // SSAO
    SharedPtr<ShaderPipeline>       ssao16FullResPipeline;
    SharedPtr<ShaderPipeline>       ssao16HalfResPipeline;
    SharedPtr<ShaderPipeline>       ssao8FullResPipeline;
    SharedPtr<ShaderPipeline>       ssao8HalfResPipeline;

                                    // GTAO
    SharedPtr<ShaderPipeline>       gtaoPrefilterPipeline;
    SharedPtr<ShaderPipeline>       gtaoMainLowPipeline;
    SharedPtr<ShaderPipeline>       gtaoMainMediumPipeline;
    SharedPtr<ShaderPipeline>       gtaoMainHighPipeline;
    SharedPtr<ShaderPipeline>       gtaoMainUltraPipeline;
    SharedPtr<ShaderPipeline>       gtaoDenoisePipeline;
    SharedPtr<ShaderPipeline>       gtaoDebugPipeline;

    RHI::PipelineState *            frameAOBilateralHorizontalPSO = nullptr;
    RHI::PipelineState *            frameAOBilateralVerticalPSO = nullptr;
    RHI::PipelineState *            frameAOUpsamplePSO = nullptr;

    RHI::PipelineState *            frameSSAOPSO = nullptr;
    RHI::PipelineState *            frameSSAOHalfResolutionPSO = nullptr;

    RHI::PipelineState *            frameGTAOPrefilterPSO = nullptr;
    RHI::PipelineState *            frameGTAOMainPSO = nullptr;
    RHI::PipelineState *            frameGTAODenoisePSO = nullptr;
    RHI::PipelineState *            frameGTAODebugPSO = nullptr;
};
