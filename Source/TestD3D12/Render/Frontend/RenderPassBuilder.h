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

class RenderFrame;
struct FrameCascadedShadow;
struct FrameShadowAtlas;
struct RenderPass;
struct DrawSurf2;
struct FrameCamera;
struct FrameSurf;
enum class RenderPassType : uint8_t;

class RenderPassBuilder {
public:
                                    // 대상 포맷/depth state/bias 는 호출자가 graphicsPipelineDesc 에 준비한다.
                                    // mask 가 0 이면 이미 선별된 전체 목록을, 아니면 해당 cascade bit 가 설정된 surface 를 사용한다.
    void                            BuildShadowPass(RenderFrame *frame, RenderPass *shadowPass, uint8_t shadowCascadeMask, const FrameSurf *const *frameSurfs, uint32_t numFrameSurfs, bool allowAsyncPSOBuild);

                                    // 해당 cascade 의 DSV, bias, surface mask 로 depth pass 를 구성한다.
    void                            BuildShadowCascadePass(RenderFrame *frame, FrameCascadedShadow *cascadedShadow, uint32_t cascadeIndex, const FrameSurf *const *frameSurfs, uint32_t numFrameSurfs, bool allowAsyncPSOBuild);

                                    // 해당 local shadow view 의 surface 목록과 광원 bias 로 depth pass 를 구성한다.
    void                            BuildLocalShadowPass(RenderFrame *frame, FrameShadowAtlas *shadowAtlas, uint32_t shadowViewIndex, bool allowAsyncPSOBuild);
    void                            BuildScenePasses(RenderFrame *frame, FrameCamera *frameCamera);
    void                            BuildGuiPass(RenderFrame *frame, FrameCamera *frameCamera);

private:
    struct ScenePassCounts;
    struct ScenePasses;

    static ScenePassCounts          CountSceneDrawSurfs(const FrameCamera *frameCamera, bool useDepthPrepass);
    ScenePasses                     CreateScenePasses(FrameCamera *frameCamera, const ScenePassCounts &counts);
    void                            AddSceneDrawSurfs(const FrameCamera *frameCamera, const ScenePasses &passes);
    void                            AddOpaqueDrawSurfs(const FrameSurf *frameSurf, const ScenePasses &passes, bool useSurfacePrepass, bool directLighting, bool indirectLighting, bool allowAsyncPSOBuild);
    void                            SortSceneDrawSurfs(RenderFrame *frame, const ScenePasses &passes);

    void                            AllocDrawSurfs(RenderFrame *frame, FrameCamera *frameCamera, uint32_t maxDrawSurfs);
    RenderPass *                    AllocRenderPass(FrameCamera *frameCamera, RenderPassType type, ShaderFeatureSet passFeatureSet, VertexInputLayoutId passBaseInputLayoutId, uint32_t maxDrawSurfs);
    void                            AddDrawSurf(RenderPass *renderPass, const FrameSurf *frameSurf, uint64_t sortKey, VertexInputLayoutId inputLayoutId, RHI::PipelineState *pso);
    void                            SortDrawSurfs(RenderFrame *frame, RenderPass *renderPass);
    void                            RadixSortDrawSurfs(const DrawSurf2 **drawSurfs, uint32_t numDrawSurfs, const DrawSurf2 **tempDrawSurfs);

    RenderPassDepthPSOResolver      depthPrepassPSOResolver;
    RenderPassDepthPSOResolver      shadowPSOResolver;
    RenderPassPSOResolver           surfacePrepassPSOResolver;
    RenderPassPSOResolver           transparentSSRPrepassPSOResolver;
    RenderPassPSOResolver           opaquePSOResolver;
    RenderPassPSOResolver           blendPSOResolver;
    RenderPassPSOResolver           skyboxPSOResolver;
    RenderPassPSOResolver           guiPSOResolver;
};
