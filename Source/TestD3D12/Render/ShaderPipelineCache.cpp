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

ShaderPipelineCache       shaderPipelineCache;

SharedPtr<ShaderPipeline> ShaderPipelineCache::FindCachedGraphicsPipeline(const ShaderTemplate *shaderTemplate, uint64_t hash) const {
    const int hashKey = cachedGraphicsPipelineHash.GenerateHash(hash);

    for (int index = cachedGraphicsPipelineHash.First(hashKey); index != -1; index = cachedGraphicsPipelineHash.Next(index)) {
        const CachedGraphicsPipeline &cachedPipeline = cachedGraphicsPipelines[index];
        if (cachedPipeline.shaderTemplate == shaderTemplate && cachedPipeline.hash == hash) {
            return cachedPipeline.pipeline;
        }
    }

    return nullptr;
}

void ShaderPipelineCache::RemoveCachedGraphicsPipelineIndex(int index) {
    CachedGraphicsPipeline &cachedPipeline = cachedGraphicsPipelines[index];

    const int hashKey = cachedGraphicsPipelineHash.GenerateHash(cachedPipeline.hash);
    cachedGraphicsPipelineHash.RemoveIndex(hashKey, index);
    cachedGraphicsPipelines.RemoveIndex(index);
}

SharedPtr<ShaderPipeline> ShaderPipelineCache::FindCachedComputePipeline(const ShaderTemplate *shaderTemplate, ShaderPermutationKey permutationKey) const {
    for (const CachedComputePipeline &cachedPipeline : cachedComputePipelines) {
        if (cachedPipeline.shaderTemplate == shaderTemplate && cachedPipeline.permutationKey == permutationKey) {
            return cachedPipeline.pipeline;
        }
    }

    return nullptr;
}

uint64_t ShaderPipelineCache::GenerateGraphicsPipelineCacheHash(const ShaderTemplate *shaderTemplate, ShaderPermutationKey permutationKey, const ShaderPipelineDesc &pipelineDesc) {
    uint64_t hash = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(shaderTemplate));
    hash = hash_combine(hash, pipelineDesc.GenerateGraphicsPipelineHash(permutationKey));
    return hash;
}

SharedPtr<ShaderPipeline> ShaderPipelineCache::GetGraphicsPipeline(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, const ShaderPipelineDesc &pipelineDesc) {
    if (!shaderTemplate) {
        return nullptr;
    }

    const uint64_t hash = GenerateGraphicsPipelineCacheHash(shaderTemplate.get(), permutationKey, pipelineDesc);

    SharedPtr<ShaderPipeline> pipeline = FindCachedGraphicsPipeline(shaderTemplate.get(), hash);
    if (!pipeline) {
        CachedGraphicsPipeline &cachedPipeline = cachedGraphicsPipelines.Alloc();
        cachedPipeline.shaderTemplate = shaderTemplate.get();
        cachedPipeline.shaderTemplateRef = shaderTemplate;
        cachedPipeline.hash = hash;
        cachedPipeline.pipeline = MakeShared<ShaderPipeline>(ShaderPipeline::Type::Graphics, permutationKey, pipelineDesc);

        const int cachedPipelineIndex = cachedGraphicsPipelines.Count() - 1;
        cachedGraphicsPipelineHash.Add(cachedGraphicsPipelineHash.GenerateHash(hash), cachedPipelineIndex);

        pipeline = cachedPipeline.pipeline;
    }

    if (pipeline->GetState() == ShaderPipeline::State::Empty) {
        pipeline->EnsureBuildAsync(shaderTemplate);
    }

    return pipeline;
}

SharedPtr<ShaderPipeline> ShaderPipelineCache::GetComputePipeline(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey) {
    if (!shaderTemplate) {
        return nullptr;
    }

    SharedPtr<ShaderPipeline> pipeline = FindCachedComputePipeline(shaderTemplate.get(), permutationKey);
    if (!pipeline) {
        CachedComputePipeline &cachedPipeline = cachedComputePipelines.Alloc();
        cachedPipeline.shaderTemplate = shaderTemplate.get();
        cachedPipeline.shaderTemplateRef = shaderTemplate;
        cachedPipeline.permutationKey = permutationKey;
        cachedPipeline.pipeline = MakeShared<ShaderPipeline>(ShaderPipeline::Type::Compute, permutationKey);

        pipeline = cachedPipeline.pipeline;
    }

    if (!pipeline->IsReady()) {
        pipeline->EnsureBuildAsync(shaderTemplate);
    }

    return pipeline;
}

void ShaderPipelineCache::ClearAll() {
    for (CachedGraphicsPipeline &cachedPipeline : cachedGraphicsPipelines) {
        if (cachedPipeline.pipeline) {
            cachedPipeline.pipeline->Reset();
        }
    }
    cachedGraphicsPipelines.Clear();
    cachedGraphicsPipelineHash.Clear();

    for (CachedComputePipeline &cachedPipeline : cachedComputePipelines) {
        if (cachedPipeline.pipeline) {
            cachedPipeline.pipeline->Reset();
        }
    }
    cachedComputePipelines.Clear();
}

void ShaderPipelineCache::ClearShaderTemplate(const ShaderTemplate *shaderTemplate) {
    if (!shaderTemplate) {
        return;
    }

    for (int i = cachedGraphicsPipelines.Count() - 1; i >= 0; --i) {
        CachedGraphicsPipeline &cachedPipeline = cachedGraphicsPipelines[i];
        if (cachedPipeline.shaderTemplate == shaderTemplate) {
            if (cachedPipeline.pipeline) {
                cachedPipeline.pipeline->Reset();
            }
            RemoveCachedGraphicsPipelineIndex(i);
        }
    }

    for (int i = cachedComputePipelines.Count() - 1; i >= 0; --i) {
        CachedComputePipeline &cachedPipeline = cachedComputePipelines[i];
        if (cachedPipeline.shaderTemplate == shaderTemplate) {
            if (cachedPipeline.pipeline) {
                cachedPipeline.pipeline->Reset();
            }
            cachedComputePipelines.RemoveIndexFast(i);
        }
    }
}

void ShaderPipelineCache::RebuildAll() {
    for (int i = cachedGraphicsPipelines.Count() - 1; i >= 0; --i) {
        CachedGraphicsPipeline &cachedPipeline = cachedGraphicsPipelines[i];
        SharedPtr<ShaderTemplate> shaderTemplate = cachedPipeline.shaderTemplateRef.lock();
        if (!shaderTemplate) {
            if (cachedPipeline.pipeline) {
                cachedPipeline.pipeline->Reset();
            }
            RemoveCachedGraphicsPipelineIndex(i);
            continue;
        }

        if (cachedPipeline.pipeline) {
            cachedPipeline.pipeline->BuildAsync(shaderTemplate);
        }
    }

    for (int i = cachedComputePipelines.Count() - 1; i >= 0; --i) {
        CachedComputePipeline &cachedPipeline = cachedComputePipelines[i];
        SharedPtr<ShaderTemplate> shaderTemplate = cachedPipeline.shaderTemplateRef.lock();
        if (!shaderTemplate) {
            if (cachedPipeline.pipeline) {
                cachedPipeline.pipeline->Reset();
            }
            cachedComputePipelines.RemoveIndexFast(i);
            continue;
        }

        if (cachedPipeline.pipeline) {
            cachedPipeline.pipeline->BuildAsync(shaderTemplate);
        }
    }
}

void ShaderPipelineCache::RebuildShaderTemplate(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!shaderTemplate) {
        return;
    }

    for (CachedGraphicsPipeline &cachedPipeline : cachedGraphicsPipelines) {
        if (cachedPipeline.shaderTemplate == shaderTemplate.get() && cachedPipeline.pipeline) {
            cachedPipeline.pipeline->BuildAsync(shaderTemplate);
        }
    }

    for (CachedComputePipeline &cachedPipeline : cachedComputePipelines) {
        if (cachedPipeline.shaderTemplate == shaderTemplate.get() && cachedPipeline.pipeline) {
            cachedPipeline.pipeline->BuildAsync(shaderTemplate);
        }
    }
}
