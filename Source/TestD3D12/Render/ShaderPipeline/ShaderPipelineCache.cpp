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
#include "ShaderPipelineCache.h"
#include "ShaderTemplate.h"
#include "LoadingSystem.h"

ShaderPipelineCache       shaderPipelineCache;

void ShaderPipelineCache::ClearAllPipelines() {
    for (CachedGraphicsPipeline &cachedGraphicsPipeline : cachedGraphicsPipelines) {
        if (cachedGraphicsPipeline.shaderPipeline) {
            cachedGraphicsPipeline.shaderPipeline->Reset();
        }
    }
    cachedGraphicsPipelines.Clear();
    cachedGraphicsPipelineHash.Clear();

    for (CachedComputePipeline &cachedComputePipeline : cachedComputePipelines) {
        if (cachedComputePipeline.shaderPipeline) {
            cachedComputePipeline.shaderPipeline->Reset();
        }
    }
    cachedComputePipelines.Clear();
}

void ShaderPipelineCache::ClearShaderTemplatePipelines(const ShaderTemplate *shaderTemplate) {
    if (!shaderTemplate) {
        return;
    }

    for (int i = cachedGraphicsPipelines.Count() - 1; i >= 0; --i) {
        CachedGraphicsPipeline &cachedGraphicsPipeline = cachedGraphicsPipelines[i];
        if (cachedGraphicsPipeline.shaderTemplatePtr == shaderTemplate) {
            if (cachedGraphicsPipeline.shaderPipeline) {
                cachedGraphicsPipeline.shaderPipeline->Reset();
            }
            RemoveCachedGraphicsPipelineIndex(i);
        }
    }

    for (int i = cachedComputePipelines.Count() - 1; i >= 0; --i) {
        CachedComputePipeline &cachedComputePipeline = cachedComputePipelines[i];
        if (cachedComputePipeline.shaderTemplatePtr == shaderTemplate) {
            if (cachedComputePipeline.shaderPipeline) {
                cachedComputePipeline.shaderPipeline->Reset();
            }
            RemoveCachedComputePipelineIndex(i);
        }
    }
}

void ShaderPipelineCache::RebuildAllPipelines() {
    for (int i = cachedGraphicsPipelines.Count() - 1; i >= 0; --i) {
        CachedGraphicsPipeline &cachedGraphicsPipeline = cachedGraphicsPipelines[i];
        SharedPtr<ShaderTemplate> shaderTemplate = cachedGraphicsPipeline.shaderTemplateRef.lock();
        if (!shaderTemplate) {
            if (cachedGraphicsPipeline.shaderPipeline) {
                cachedGraphicsPipeline.shaderPipeline->Reset();
            }
            RemoveCachedGraphicsPipelineIndex(i);
            continue;
        }

        if (cachedGraphicsPipeline.shaderPipeline) {
            if (cachedGraphicsPipeline.shaderPipeline->GetPSO()) {
                if (LoadingSystem::IsAsyncAllowed()) {
                    cachedGraphicsPipeline.shaderPipeline->RebuildAsync(shaderTemplate);
                } else {
                    cachedGraphicsPipeline.shaderPipeline->Rebuild(shaderTemplate);
                }
            } else if (LoadingSystem::IsAsyncAllowed()) {
                cachedGraphicsPipeline.shaderPipeline->BuildAsync(shaderTemplate);
            } else {
                cachedGraphicsPipeline.shaderPipeline->Build(shaderTemplate);
            }
        }
    }

    for (int i = cachedComputePipelines.Count() - 1; i >= 0; --i) {
        CachedComputePipeline &cachedComputePipeline = cachedComputePipelines[i];
        SharedPtr<ShaderTemplate> shaderTemplate = cachedComputePipeline.shaderTemplateRef.lock();
        if (!shaderTemplate) {
            if (cachedComputePipeline.shaderPipeline) {
                cachedComputePipeline.shaderPipeline->Reset();
            }
            RemoveCachedComputePipelineIndex(i);
            continue;
        }

        if (cachedComputePipeline.shaderPipeline) {
            if (cachedComputePipeline.shaderPipeline->GetPSO()) {
                if (LoadingSystem::IsAsyncAllowed()) {
                    cachedComputePipeline.shaderPipeline->RebuildAsync(shaderTemplate);
                } else {
                    cachedComputePipeline.shaderPipeline->Rebuild(shaderTemplate);
                }
            } else if (LoadingSystem::IsAsyncAllowed()) {
                cachedComputePipeline.shaderPipeline->BuildAsync(shaderTemplate);
            } else {
                cachedComputePipeline.shaderPipeline->Build(shaderTemplate);
            }
        }
    }
}

void ShaderPipelineCache::RebuildShaderTemplatePipelines(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!shaderTemplate) {
        return;
    }

    for (CachedGraphicsPipeline &cachedGraphicsPipeline : cachedGraphicsPipelines) {
        if (cachedGraphicsPipeline.shaderTemplatePtr == shaderTemplate.get() && cachedGraphicsPipeline.shaderPipeline) {
            if (cachedGraphicsPipeline.shaderPipeline->GetPSO()) {
                if (LoadingSystem::IsAsyncAllowed()) {
                    cachedGraphicsPipeline.shaderPipeline->RebuildAsync(shaderTemplate);
                } else {
                    cachedGraphicsPipeline.shaderPipeline->Rebuild(shaderTemplate);
                }
            } else if (LoadingSystem::IsAsyncAllowed()) {
                cachedGraphicsPipeline.shaderPipeline->BuildAsync(shaderTemplate);
            } else {
                cachedGraphicsPipeline.shaderPipeline->Build(shaderTemplate);
            }
        }
    }

    for (CachedComputePipeline &cachedComputePipeline : cachedComputePipelines) {
        if (cachedComputePipeline.shaderTemplatePtr == shaderTemplate.get() && cachedComputePipeline.shaderPipeline) {
            if (cachedComputePipeline.shaderPipeline->GetPSO()) {
                if (LoadingSystem::IsAsyncAllowed()) {
                    cachedComputePipeline.shaderPipeline->RebuildAsync(shaderTemplate);
                } else {
                    cachedComputePipeline.shaderPipeline->Rebuild(shaderTemplate);
                }
            } else if (LoadingSystem::IsAsyncAllowed()) {
                cachedComputePipeline.shaderPipeline->BuildAsync(shaderTemplate);
            } else {
                cachedComputePipeline.shaderPipeline->Build(shaderTemplate);
            }
        }
    }
}

const ShaderPipelineCache::CachedGraphicsPipeline *ShaderPipelineCache::GetOrCreateGraphicsPipelineEntry(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, const GraphicsPipelineDesc &graphicsPipelineDesc, bool async) {
    if (!shaderTemplate) {
        return nullptr;
    }

    // ShaderTemplate/ShaderPermutationKey/GraphicsPipelineDesc 조합으로 캐싱된 그래픽스 파이프라인을 찾는다.
    const uint64_t hash = GenerateGraphicsPipelineCacheHash(shaderTemplate.get(), permutationKey, graphicsPipelineDesc);

    const CachedGraphicsPipeline *entry = FindCachedGraphicsPipeline(shaderTemplate.get(), hash);
    if (!entry) {
        CachedGraphicsPipeline &cachedGraphicsPipeline = cachedGraphicsPipelines.Alloc();
        cachedGraphicsPipeline.shaderTemplatePtr = shaderTemplate.get();
        cachedGraphicsPipeline.shaderTemplateRef = shaderTemplate;
        cachedGraphicsPipeline.graphicsPipelineCacheHash = hash;
        cachedGraphicsPipeline.shaderPipeline = MakeShared<ShaderPipeline>(ShaderPipeline::Type::Graphics, permutationKey, graphicsPipelineDesc);

        const int cachedPipelineIndex = cachedGraphicsPipelines.Count() - 1;
        cachedGraphicsPipelineHash.Add(cachedGraphicsPipelineHash.GenerateHash(hash), cachedPipelineIndex);

        entry = &cachedGraphicsPipeline;
    }

    ShaderPipeline *graphicsPipeline = entry->shaderPipeline.get();
    if (graphicsPipeline->GetState() == ShaderPipeline::State::Empty) {
        if (async && LoadingSystem::IsAsyncAllowed()) {
            graphicsPipeline->BuildAsync(shaderTemplate);
        } else {
            graphicsPipeline->Build(shaderTemplate);
        }
    }

    return entry;
}

const ShaderPipelineCache::CachedGraphicsPipeline *ShaderPipelineCache::FindCachedGraphicsPipeline(const ShaderTemplate *shaderTemplate, uint64_t graphicsPipelineCacheHash) const {
    const int hashKey = cachedGraphicsPipelineHash.GenerateHash(graphicsPipelineCacheHash);

    for (int index = cachedGraphicsPipelineHash.First(hashKey); index != -1; index = cachedGraphicsPipelineHash.Next(index)) {
        const CachedGraphicsPipeline &cachedPipeline = cachedGraphicsPipelines[index];
        if (cachedPipeline.shaderTemplatePtr == shaderTemplate && cachedPipeline.graphicsPipelineCacheHash == graphicsPipelineCacheHash) {
            return &cachedPipeline;
        }
    }

    return nullptr;
}

void ShaderPipelineCache::RemoveCachedGraphicsPipelineIndex(int index) {
    CachedGraphicsPipeline &cachedPipeline = cachedGraphicsPipelines[index];

    const int hashKey = cachedGraphicsPipelineHash.GenerateHash(cachedPipeline.graphicsPipelineCacheHash);
    cachedGraphicsPipelineHash.RemoveIndex(hashKey, index);
    cachedGraphicsPipelines.RemoveIndex(index);
}

const ShaderPipelineCache::CachedComputePipeline *ShaderPipelineCache::GetOrCreateComputePipelineEntry(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, bool async) {
    if (!shaderTemplate) {
        return nullptr;
    }

    // ShaderTemplate/ShaderPermutationKey 조합으로 캐싱된 컴퓨트 파이프라인을 찾는다.
    const CachedComputePipeline *entry = FindCachedComputePipeline(shaderTemplate.get(), permutationKey);
    if (!entry) {
        CachedComputePipeline &cachedComputePipeline = cachedComputePipelines.Alloc();
        cachedComputePipeline.shaderTemplatePtr = shaderTemplate.get();
        cachedComputePipeline.shaderTemplateRef = shaderTemplate;
        cachedComputePipeline.permutationKey = permutationKey;
        cachedComputePipeline.shaderPipeline = MakeShared<ShaderPipeline>(ShaderPipeline::Type::Compute, permutationKey);

        entry = &cachedComputePipeline;
    }

    ShaderPipeline *computePipeline = entry->shaderPipeline.get();
    if (computePipeline->GetState() == ShaderPipeline::State::Empty) {
        if (async && LoadingSystem::IsAsyncAllowed()) {
            computePipeline->BuildAsync(shaderTemplate);
        } else {
            computePipeline->Build(shaderTemplate);
        }
    }

    return entry;
}

const ShaderPipelineCache::CachedComputePipeline *ShaderPipelineCache::FindCachedComputePipeline(const ShaderTemplate *shaderTemplate, ShaderPermutationKey permutationKey) const {
    for (const CachedComputePipeline &cachedPipeline : cachedComputePipelines) {
        if (cachedPipeline.shaderTemplatePtr == shaderTemplate && cachedPipeline.permutationKey == permutationKey) {
            return &cachedPipeline;
        }
    }

    return nullptr;
}

void ShaderPipelineCache::RemoveCachedComputePipelineIndex(int index) {
    cachedComputePipelines.RemoveIndexFast(index);
}

uint64_t ShaderPipelineCache::GenerateGraphicsPipelineCacheHash(const ShaderTemplate *shaderTemplate, ShaderPermutationKey permutationKey, const GraphicsPipelineDesc &graphicsPipelineDesc) {
    uint64_t hash = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(shaderTemplate));
    hash = hash_combine(hash, graphicsPipelineDesc.GenerateGraphicsPipelineHash(permutationKey));
    return hash;
}
