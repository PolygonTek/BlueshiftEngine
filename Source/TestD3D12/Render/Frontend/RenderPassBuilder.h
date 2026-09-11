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
struct VisCascadedShadow;
struct RenderPass;
struct RenderPassDrawSurf;
struct VisCamera;
struct VisSurf;
enum class RenderPassType : uint8_t;

class RenderPassBuilder {
public:
                                    // 대상 포맷/depth state/bias 는 호출자가 graphicsPipelineDesc 에 준비한다.
                                    // 해당 cascade bit 가 설정된 surface 로 패스를 구성한다.
    void                            BuildShadow(RenderFrameData *frameData, RenderPass *shadowPass, uint8_t shadowCascadeMask, const VisSurf *const *visSurfs, uint32_t numVisSurfs, bool allowAsyncPSOBuild);

                                    // 해당 cascade 의 DSV, bias, surface mask 로 depth pass 를 구성한다.
    void                            BuildCascadedShadow(RenderFrameData *frameData, VisCascadedShadow *cascadedShadow, uint32_t cascadeIndex, const VisSurf *const *visSurfs, uint32_t numVisSurfs, bool allowAsyncPSOBuild);
    void                            BuildMain(RenderFrameData *frameData, VisCamera *visCamera);
    void                            BuildGUI(RenderFrameData *frameData, VisCamera *visCamera);

private:
    void                            AllocRenderPassDrawSurfs(RenderFrameData *frameData, VisCamera *visCamera, uint32_t maxDrawSurfs);
    RenderPass *                    AllocRenderPass(VisCamera *visCamera, RenderPassType type, ShaderFeatureSet passFeatureSet, VertexInputLayoutId passBaseInputLayoutId, uint32_t maxDrawSurfs);
    void                            AddRenderPassDrawSurf(RenderPass *renderPass, const VisSurf *visSurf, uint64_t sortKey, VertexInputLayoutId inputLayoutId, RHI::PipelineState *pso);
    void                            SortRenderPassDrawSurfs(RenderFrameData *frameData, RenderPass *renderPass);
    void                            RadixSortDrawSurfs(const RenderPassDrawSurf **drawSurfs, uint32_t numDrawSurfs, const RenderPassDrawSurf **tempDrawSurfs);

    RenderPassDepthPSOResolver      depthPSOResolver;
    RenderPassDepthPSOResolver      shadowPSOResolver;
    RenderPassPSOResolver           opaquePSOResolver;
    RenderPassPSOResolver           blendPSOResolver;
    RenderPassPSOResolver           skyboxPSOResolver;
    RenderPassPSOResolver           guiPSOResolver;
};
