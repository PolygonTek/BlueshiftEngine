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

#include "Containers/HashIndex.h"
#include "RHI/RHI.h"
#include "ShaderFeatureSet.h"
#include "ShaderPipeline.h"
#include "VertexInputLayoutFactory.h"

class Material;
class RenderFrameData;
struct RenderPass;
struct VisCamera;
struct VisSurf;
enum class RenderPassType : uint8_t;

class PassPipelineResolver {
public:
    struct ResolveResult {
        VertexInputLayoutId     inputLayoutId = VertexInputLayoutId::XyzStColorNormalTangent;
        RHI::PipelineState *    pso = nullptr;
    };

    void                        Begin(const GraphicsPipelineDesc &pipelineDesc, uint32_t maxEntries);

    ResolveResult               Resolve(const RenderPass *renderPass, const VisSurf *visSurf, const ShaderFeatureSet &featureSet);

private:
    struct Entry {
        const Material *        material = nullptr;
        uint32_t                featureFlags = 0;
        VertexInputLayoutId     inputLayoutId = VertexInputLayoutId::XyzStColorNormalTangent;
        RHI::PipelineState *    pso = nullptr;
    };

    const GraphicsPipelineDesc &GetPipelineDesc(VertexInputLayoutId inputLayoutId);

    static constexpr int        NumInputLayouts = static_cast<int>(VertexInputLayoutId::Count);

    GraphicsPipelineDesc        basePipelineDesc;
    GraphicsPipelineDesc        pipelineDescs[NumInputLayouts];
    bool                        pipelineDescValid[NumInputLayouts] = {};

    BE1::Array<Entry>           entries;
    BE1::HashIndex              entriesHash { 256, 256 };
};

class RenderPassBuilder {
public:
    void                        BuildMain(RenderFrameData *frameData, VisCamera *visCamera);
    void                        BuildGUI(RenderFrameData *frameData, VisCamera *visCamera);

private:
    RenderPass *                AllocRenderPass(RenderFrameData *frameData, VisCamera *visCamera, RenderPassType type, ShaderFeatureSet passFeatureSet, VertexInputLayoutId baseInputLayoutId, int maxDrawSurfs);
    void                        AddPassDrawSurf(RenderFrameData *frameData, RenderPass *renderPass, const VisSurf *visSurf, uint64_t sortKey, VertexInputLayoutId inputLayoutId, RHI::PipelineState *pso);
    void                        SortPassDrawSurfs(RenderFrameData *frameData, RenderPass *renderPass);

    PassPipelineResolver        opaquePipelineResolver;
    PassPipelineResolver        blendPipelineResolver;
    PassPipelineResolver        skyboxPipelineResolver;
    PassPipelineResolver        guiPipelineResolver;
};
