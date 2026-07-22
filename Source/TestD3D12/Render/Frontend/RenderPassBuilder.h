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
#include "RenderPassDepthPSOResolver.h"
#include "RenderPassPSOResolver.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"

class RenderFrameData;
struct RenderPass;
struct RenderPassDrawSurf;
struct VisCamera;
struct VisSurf;
enum class RenderPassType : uint8_t;

class RenderPassBuilder {
public:
    void                            BuildMain(RenderFrameData *frameData, VisCamera *visCamera);
    void                            BuildGUI(RenderFrameData *frameData, VisCamera *visCamera);

private:
    void                            AllocRenderPassDrawSurfs(RenderFrameData *frameData, VisCamera *visCamera, uint32_t maxDrawSurfs);
    RenderPass *                    AllocRenderPass(VisCamera *visCamera, RenderPassType type, ShaderFeatureSet passFeatureSet, VertexInputLayoutId passBaseInputLayoutId, uint32_t maxDrawSurfs);
    void                            AddRenderPassDrawSurf(RenderPass *renderPass, const VisSurf *visSurf, uint64_t sortKey, VertexInputLayoutId inputLayoutId, RHI::PipelineState *pso);
    void                            SortRenderPassDrawSurfs(RenderFrameData *frameData, RenderPass *renderPass);
    void                            RadixSortDrawSurfs(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, const RenderPassDrawSurf **tempDrawSurfs);

    RenderPassDepthPSOResolver      depthPSOResolver;
    RenderPassPSOResolver           opaquePSOResolver;
    RenderPassPSOResolver           blendPSOResolver;
    RenderPassPSOResolver           skyboxPSOResolver;
    RenderPassPSOResolver           guiPSOResolver;
};
