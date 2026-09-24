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

class RenderBackend;
class ShaderTemplate;
class ShaderPipeline;
class ShaderPipelineBootstrap;
struct ScreenSpaceAOFrameData;

class ScreenSpaceAO {
public:
    struct DispatchParams {
        bool                        useHalfResolution = false;
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
    void                            Dispatch(RenderBackend &backend, const DispatchParams &params) const;
    bool                            DispatchDebugPreview(RenderBackend &backend) const;

private:
    void                            DispatchGTAOPrefilterDepth(RenderBackend &backend, RHI::GPUResourceState depthInputState, bool useHalfResolution) const;
    void                            DispatchGTAOMain(RenderBackend &backend, bool useHalfResolution) const;
    RHI::Texture *                  DispatchGTAODenoise(RenderBackend &backend, uint32_t denoisePassCount, bool useHalfResolution, RHI::GPUResourceState &rawAOState, RHI::GPUResourceState &tempAOState) const;
    void                            DispatchAOUpsample(RenderBackend &backend, RHI::Texture *depthTexture, RHI::Texture *inputHalfAOTexture, RHI::Texture *inputHalfDepthTexture, RHI::Texture *outputAOTexture, float radius) const;

                                    // AO upsampling shader template
    SharedPtr<ShaderTemplate>       aoUpsampleShaderTemplate;

                                    // GTAO shader templates
    SharedPtr<ShaderTemplate>       gtaoPrefilterShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoMainLowShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoMainMediumShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoMainHighShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoMainUltraShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoDenoiseShaderTemplate;
    SharedPtr<ShaderTemplate>       gtaoDebugShaderTemplate;

                                    // AO upsampling
    SharedPtr<ShaderPipeline>       aoUpsamplePipeline;

                                    // GTAO
    SharedPtr<ShaderPipeline>       gtaoPrefilterPipeline;
    SharedPtr<ShaderPipeline>       gtaoMainLowPipeline;
    SharedPtr<ShaderPipeline>       gtaoMainMediumPipeline;
    SharedPtr<ShaderPipeline>       gtaoMainHighPipeline;
    SharedPtr<ShaderPipeline>       gtaoMainUltraPipeline;
    SharedPtr<ShaderPipeline>       gtaoDenoisePipeline;
    SharedPtr<ShaderPipeline>       gtaoDebugPipeline;

    RHI::PipelineState *            frameAOUpsamplePSO = nullptr;

    RHI::PipelineState *            frameGTAOPrefilterPSO = nullptr;
    RHI::PipelineState *            frameGTAOMainPSO = nullptr;
    RHI::PipelineState *            frameGTAODenoisePSO = nullptr;
    RHI::PipelineState *            frameGTAODebugPSO = nullptr;
};
