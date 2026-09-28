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

#include "Precompiled.h"
#include "RenderPassPSOResolver.h"
#include "Render/RenderInternal.h"
#include "Render/Material/Material.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"

void RenderPassPSOResolver::Begin(const RenderPass *renderPass, uint32_t initialPSOEntryCapacity) {
    assert(renderPass);

    basePipelineDesc = renderPass->graphicsPipelineDesc;
    baseInputLayoutId = renderPass->baseInputLayoutId;

    memset(pipelineDescValid, 0, sizeof(pipelineDescValid));

    psoEntries.Reserve(BE1::Max(initialPSOEntryCapacity, 1u));
    psoEntries.SetCount(0, false);
    psoEntriesHash.Clear();
}

const GraphicsPipelineDesc &RenderPassPSOResolver::GetGraphicsPipelineDesc(VertexInputLayoutId inputLayoutId) {
    const int inputLayoutIndex = static_cast<int>(inputLayoutId);
    assert(inputLayoutIndex >= 0 && inputLayoutIndex < COUNT_OF(pipelineDescs));

    if (pipelineDescValid[inputLayoutIndex] == false) {
        pipelineDescValid[inputLayoutIndex] = true;

        pipelineDescs[inputLayoutIndex] = basePipelineDesc;
        pipelineDescs[inputLayoutIndex].inputLayout = &VertexInputLayoutFactory::Get(inputLayoutId);
    }

    return pipelineDescs[inputLayoutIndex];
}

RenderPassPSOResolver::PSOEntry *RenderPassPSOResolver::FindPSOEntry(const Material *material, const ShaderFeatureSet &featureSet, VertexInputLayoutId inputLayoutId, int &hashKey) {
    // material pointer, featureSet, input layout id 로 hash 키를 만든다.
    uint64_t hash = reinterpret_cast<uintptr_t>(material);
    hash = hash_combine(hash, featureSet.flags);
    hash = hash_combine(hash, static_cast<int>(inputLayoutId));

    hashKey = psoEntriesHash.GenerateHash(hash);
    for (int index = psoEntriesHash.First(hashKey); index != -1; index = psoEntriesHash.Next(index)) {
        PSOEntry &entry = psoEntries[index];
        if (entry.material == material && entry.featureFlags == featureSet.flags && entry.inputLayoutId == inputLayoutId) {
            return &entry;
        }
    }

    return nullptr;
}

RenderPassPSOResolver::ResolveResult RenderPassPSOResolver::Resolve(const FrameSurf *frameSurf, const ShaderFeatureSet &featureSet, bool async) {
    assert(frameSurf);
    assert(frameSurf->material);
    assert(frameSurf->subMesh);

    ResolveResult result;
    result.inputLayoutId = baseInputLayoutId;

    const int maxVertexWeights = frameSurf->subMesh->MaxVertexWeights();
    if (maxVertexWeights > 0 && (featureSet.flags & ShaderFeatureSet::GpuSkinningWeightMask) != 0) {
        result.inputLayoutId = VertexInputLayoutFactory::GetIdForVertexWeights(baseInputLayoutId, maxVertexWeights);
    }

    const Material *material = frameSurf->material;
    int hashKey;
    if (PSOEntry *entry = FindPSOEntry(material, featureSet, result.inputLayoutId, hashKey)) {
        result.pso = entry->pso;
        return result;
    }

    // hash 에 존재하지 않는다면, input layout 에 맞는 graphics pipeline desc 를 준비하고,
    const GraphicsPipelineDesc &graphicsPipelineDesc = GetGraphicsPipelineDesc(result.inputLayoutId);

    // material->GetGraphicsPipeline 호출로 새 ShaderPipeline 을 얻고, 준비된 PSO 까지 얻는다.
    result.pso = ShaderPipeline::GetReadyPSO(material->GetGraphicsPipeline(featureSet, graphicsPipelineDesc, async));

    // material pointer, featureSet, input layout id 로 새 PSO entry 를 추가한다.
    RenderPassPSOResolver::PSOEntry &entry = psoEntries.Alloc();
    entry.material = material;
    entry.featureFlags = featureSet.flags;
    entry.inputLayoutId = result.inputLayoutId;
    entry.pso = result.pso;
    psoEntriesHash.Add(hashKey, psoEntries.Count() - 1);

    return result;
}
