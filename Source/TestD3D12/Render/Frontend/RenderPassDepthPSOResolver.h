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
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/ShaderPermutationKey.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"

class ShaderTemplate;
struct RenderPass;
struct VisSurf;

class RenderPassDepthPSOResolver {
public:
    struct ResolveResult {
        VertexInputLayoutId         inputLayoutId;
        RHI::PipelineState *        pso = nullptr;
    };

    void                            Begin(const RenderPass *renderPass, uint32_t initialPSOEntryCapacity, const SharedPtr<ShaderTemplate> &shaderTemplate);

    ResolveResult                   Resolve(const VisSurf *visSurf, bool async = true);

private:
    struct PSOEntry {
        uint32_t                    featureFlags = 0;
        VertexInputLayoutId         inputLayoutId;
        RHI::CullMode               cullMode;
        RHI::PipelineState *        pso = nullptr;
    };

    struct PermutationKeyEntry {
        uint32_t                    featureFlags = 0;
        ShaderPermutationKey        permutationKey;
    };

                                    // depth pass 에 필요한 shader features 만 남긴다.
    static ShaderFeatureSet         BuildDepthFeatureSet(const ShaderFeatureSet &featureSet);

                                    // basePipelineDesc 를 기반으로 input layout 별 graphics pipeline desc 를 얻는다.
    const GraphicsPipelineDesc &    GetGraphicsPipelineDesc(VertexInputLayoutId inputLayoutId);

    PSOEntry *                      FindPSOEntry(const ShaderFeatureSet &featureSet, VertexInputLayoutId inputLayoutId, RHI::CullMode cullMode, int &hashKey);
    ShaderPermutationKey            GetPermutationKey(ShaderTemplate *shaderTemplate, const ShaderFeatureSet &featureSet);

    static constexpr int            NumInputLayouts = static_cast<int>(VertexInputLayoutId::Count);

    ShaderFeatureSet                passFeatureSet;                             // RenderPass 의 feature set
    VertexInputLayoutId             baseInputLayoutId;                          // RenderPass 의 기반 input layout id
    GraphicsPipelineDesc            basePipelineDesc;                           // RenderPass 의 기반 graphics pipeline desc
    SharedPtr<ShaderTemplate>       shaderTemplate;

    GraphicsPipelineDesc            pipelineDescs[NumInputLayouts];             // input layout 별로 graphics pipeline desc 를 캐시한다.
    bool                            pipelineDescValid[NumInputLayouts] = {};    // input layout 별로 graphics pipeline desc 가 유효한지 여부

    BE1::Array<PSOEntry>            psoEntries { 16 };
    BE1::HashIndex                  psoEntriesHash { 1024, 64 };

    BE1::Array<PermutationKeyEntry> permutationKeyEntries;
};
