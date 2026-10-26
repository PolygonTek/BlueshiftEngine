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
#include "RenderPassDepthPSOResolver.h"
#include "Render/RenderInternal.h"
#include "Render/ShaderPipeline/ShaderPermutationKey.h"
#include "Render/ShaderPipeline/ShaderPipelineCache.h"
#include "Render/ShaderPipeline/ShaderTemplate.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"

void RenderPassDepthPSOResolver::Begin(const RenderPass *renderPass, uint32_t initialPSOEntryCapacity, const SharedPtr<ShaderTemplate> &shaderTemplate) {
    assert(renderPass);

    passFeatureSet = renderPass->featureSet;
    basePipelineDesc = renderPass->graphicsPipelineDesc;
    baseInputLayoutId = renderPass->baseInputLayoutId;

    if (this->shaderTemplate != shaderTemplate) {
        this->shaderTemplate = shaderTemplate;
    }

    memset(pipelineDescValid, 0, sizeof(pipelineDescValid));

    psoEntries.Reserve(BE1::Max(initialPSOEntryCapacity, 1u));
    psoEntries.SetCount(0, false);
    psoEntriesHash.Clear();
}

ShaderFeatureSet RenderPassDepthPSOResolver::BuildDepthFeatureSet(const ShaderFeatureSet &featureSet) {
    ShaderFeatureSet depthFeatureSet;
    depthFeatureSet.flags = featureSet.flags & (ShaderFeatureSet::Perforated | ShaderFeatureSet::GpuSkinningWeightMask | ShaderFeatureSet::OutputVelocity | ShaderFeatureSet::OutputNormal);
    return depthFeatureSet;
}

const GraphicsPipelineDesc &RenderPassDepthPSOResolver::GetGraphicsPipelineDesc(VertexInputLayoutId inputLayoutId) {
    const int inputLayoutIndex = static_cast<int>(inputLayoutId);
    assert(inputLayoutIndex >= 0 && inputLayoutIndex < COUNT_OF(pipelineDescs));

    if (pipelineDescValid[inputLayoutIndex] == false) {
        pipelineDescValid[inputLayoutIndex] = true;

        pipelineDescs[inputLayoutIndex] = basePipelineDesc;
        pipelineDescs[inputLayoutIndex].inputLayout = &VertexInputLayoutFactory::Get(inputLayoutId);
    }

    return pipelineDescs[inputLayoutIndex];
}

RenderPassDepthPSOResolver::PSOEntry *RenderPassDepthPSOResolver::FindPSOEntry(const ShaderFeatureSet &featureSet, VertexInputLayoutId inputLayoutId, RHI::CullMode cullMode, int &hashKey) {
    // featureSet, input layout id, cull mode 로 hash 키를 만든다.
    uint64_t hash = featureSet.flags;
    hash = hash_combine(hash, static_cast<int>(inputLayoutId));
    hash = hash_combine(hash, static_cast<int>(cullMode));

    hashKey = psoEntriesHash.GenerateHash(hash);
    for (int index = psoEntriesHash.First(hashKey); index != -1; index = psoEntriesHash.Next(index)) {
        PSOEntry &entry = psoEntries[index];
        if (entry.featureFlags == featureSet.flags && entry.inputLayoutId == inputLayoutId && entry.cullMode == cullMode) {
            return &entry;
        }
    }

    return nullptr;
}

ShaderPermutationKey RenderPassDepthPSOResolver::GetPermutationKey(ShaderTemplate *shaderTemplate, const ShaderFeatureSet &featureSet) {
    for (const PermutationKeyEntry &entry : permutationKeyEntries) {
        if (entry.featureFlags == featureSet.flags) {
            return entry.permutationKey;
        }
    }

    // Depth 계열 shader template 은 property define 이 없으므로 featureSet 만으로 permutation key 를 만든다.
    ShaderPermutationKey permutationKey = ShaderPermutationKey::GeneratePermutationKey({}, featureSet);

    // ShaderTemplate::PrepareShaderCompileData 에서 permutation 을 찾을 수 있도록 featureSet 별로 한 번만 RegisterPermutation 한다.
    ShaderPermutationKey registeredPermutationKey = shaderTemplate->RegisterPermutation({}, featureSet);
    assert(registeredPermutationKey == permutationKey);

    PermutationKeyEntry &entry = permutationKeyEntries.Alloc();
    entry.featureFlags = featureSet.flags;
    entry.permutationKey = permutationKey;

    return permutationKey;
}

RenderPassDepthPSOResolver::ResolveResult RenderPassDepthPSOResolver::Resolve(const FrameSurf *frameSurf, bool async) {
    assert(frameSurf);
    assert(frameSurf->subMesh);
    assert(frameSurf->material);

    ResolveResult result;
    result.inputLayoutId = baseInputLayoutId;

    if (!shaderTemplate) {
        return result;
    }

    ShaderFeatureSet featureSet = BuildDepthFeatureSet(frameSurf->baseFeatureSet | passFeatureSet);
    featureSet = shaderTemplate->MaskSupportedFeatures(featureSet);

    const int maxVertexWeights = frameSurf->subMesh->MaxVertexWeights();
    if (maxVertexWeights > 0 && (featureSet.flags & ShaderFeatureSet::GpuSkinningWeightMask) != 0) {
        result.inputLayoutId = VertexInputLayoutFactory::GetIdForVertexWeights(baseInputLayoutId, maxVertexWeights);
    }

    const RHI::CullMode cullMode = frameSurf->material->GetCullMode();
    int hashKey;
    if (PSOEntry *entry = FindPSOEntry(featureSet, result.inputLayoutId, cullMode, hashKey)) {
        result.pso = entry->pso;
        return result;
    }

    // hash 에 존재하지 않는다면, input layout 에 맞는 graphics pipeline desc 를 준비하고,
    GraphicsPipelineDesc graphicsPipelineDesc = GetGraphicsPipelineDesc(result.inputLayoutId);

    // Opaque depth surface 는 fragment shader 없이 depth 를 기록할 수 있다.
    // Alpha cutoff, velocity 또는 normal 출력이 필요하면 fragment shader 를 유지한다.
    graphicsPipelineDesc.fragmentShaderEnabled = featureSet.HasFeature(ShaderFeature::Perforated) || featureSet.HasFeature(ShaderFeature::OutputVelocity) || featureSet.HasFeature(ShaderFeature::OutputNormal);

    // 재질별 cull mode 를 유지한다. 양면 alpha-cutoff surface 도 양쪽에서 depth 를 기록해야 한다.
    switch (cullMode) {
    case RHI::CullMode::Back:
        graphicsPipelineDesc.rasterizerStateType = RHI::RasterizerStateType::SolidFrontSided;
        break;
    case RHI::CullMode::Front:
        graphicsPipelineDesc.rasterizerStateType = RHI::RasterizerStateType::SolidBackSided;
        break;
    case RHI::CullMode::None:
        graphicsPipelineDesc.rasterizerStateType = RHI::RasterizerStateType::SolidTwoSided;
        break;
    }

    ShaderPermutationKey permutationKey = GetPermutationKey(shaderTemplate.get(), featureSet);
    result.pso = ShaderPipeline::GetReadyPSO(shaderPipelineCache.GetGraphicsPipelinePtr(shaderTemplate, permutationKey, graphicsPipelineDesc, async));

    // featureSet, input layout id, cull mode 로 새 PSO entry 를 추가한다.
    RenderPassDepthPSOResolver::PSOEntry &entry = psoEntries.Alloc();
    entry.featureFlags = featureSet.flags;
    entry.inputLayoutId = result.inputLayoutId;
    entry.cullMode = cullMode;
    entry.pso = result.pso;
    psoEntriesHash.Add(hashKey, psoEntries.Count() - 1);

    return result;
}
