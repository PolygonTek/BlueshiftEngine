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

#include "Containers/Array.h"
#include "Containers/HashIndex.h"
#include "RHI/RHI.h"
#include "Render/ShaderPipeline/ShaderFeatureSet.h"
#include "Render/ShaderPipeline/ShaderProgram.h"
#include "Render/ShaderPipeline/GraphicsPipelineDesc.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"

class Material;
struct RenderPass;
struct FrameSurf;

class RenderPassPSOResolver {
public:
    struct ResolveResult {
        VertexInputLayoutId         inputLayoutId;
        RHI::PipelineState *        pso = nullptr;
    };

    void                            Prepare(const RenderPass *renderPass, ShaderProgram program, uint32_t initialPSOEntryCapacity);

    ResolveResult                   Resolve(const FrameSurf *frameSurf, const ShaderFeatureSet &featureSet, bool async = true) { return Resolve(frameSurf, featureSet, baseInputLayoutId, async); }

                                    // 지정한 기본 input layout 에 GPU skinning 을 적용한 뒤 PSO 를 찾는다.
    ResolveResult                   Resolve(const FrameSurf *frameSurf, const ShaderFeatureSet &featureSet, VertexInputLayoutId inputLayoutId, bool async = true);

private:
    struct PSOEntry {
        const Material *            material = nullptr;
        uint32_t                    featureFlags = 0;
        VertexInputLayoutId         inputLayoutId;
        RHI::PipelineState *        pso = nullptr;
    };

                                    // basePipelineDesc 를 기반으로 input layout 별 graphics pipeline desc 를 얻는다.
    const GraphicsPipelineDesc &    GetGraphicsPipelineDesc(VertexInputLayoutId inputLayoutId);

    PSOEntry *                      FindPSOEntry(const Material *material, const ShaderFeatureSet &featureSet, VertexInputLayoutId inputLayoutId, int &hashKey);

    static constexpr int            NumInputLayouts = static_cast<int>(VertexInputLayoutId::Count);

    ShaderProgram                   program = ShaderProgram::Main;              // Prepare 사이에는 고정되며 Prepare 시 PSO 캐시를 비운다.
    VertexInputLayoutId             baseInputLayoutId;                          // RenderPass 의 기반 input layout id
    GraphicsPipelineDesc            basePipelineDesc;                           // RenderPass 의 기반 graphics pipeline desc

    GraphicsPipelineDesc            pipelineDescs[NumInputLayouts];             // input layout 별로 graphics pipeline desc 를 캐시한다.
    bool                            pipelineDescValid[NumInputLayouts] = {};    // input layout 별로 graphics pipeline desc 가 유효한지 여부

    BE1::Array<PSOEntry>            psoEntries { 64 };
    BE1::HashIndex                  psoEntriesHash { 1024, 64 };
};
