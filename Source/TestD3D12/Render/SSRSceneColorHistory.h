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

class RenderBackend;
class ShaderTemplate;
class ShaderPipeline;
class ShaderPipelineBootstrap;

class SSRSceneColorHistory {
public:
    void                            Init();
    void                            Shutdown();

    void                            TrackBootstrapPipelines(ShaderPipelineBootstrap &bootstrap) const;

    void                            UpdateFramePSOs();

    bool                            Dispatch(RenderBackend &backend) const;

private:
    SharedPtr<ShaderTemplate>       genMipShaderTemplate;
    SharedPtr<ShaderTemplate>       genMipDepthShaderTemplate;

    SharedPtr<ShaderPipeline>       genMipPipeline;
    SharedPtr<ShaderPipeline>       genMipDepthPipeline;

    RHI::PipelineState *            frameGenMipPSO = nullptr;
    RHI::PipelineState *            frameGenMipDepthPSO = nullptr;
};
