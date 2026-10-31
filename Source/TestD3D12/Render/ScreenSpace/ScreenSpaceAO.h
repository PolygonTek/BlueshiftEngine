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
struct FrameScreenSpaceAO;

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
    static bool                     HasRequiredResources(const FrameScreenSpaceAO &data, const RenderFrameSettings &settings);

                                    // 필요한 PSO 가 준비됐는지 확인하고 설정에 맞는 실행 파라미터를 구성한다.
    bool                            PrepareDispatchParams(const RenderFrameSettings &settings, DispatchParams &params) const;

                                    // 동일한 설정으로 HasRequiredResources() 와 PrepareDispatchParams() 가 성공하고, 단일 샘플 입력이 준비되어 있어야 한다.
                                    // 입력 및 임시 리소스는 호출 전후에 ShaderResource 상태여야 한다.
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
