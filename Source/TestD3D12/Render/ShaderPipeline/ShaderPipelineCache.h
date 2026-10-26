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
#include "ShaderPipeline.h"

class ShaderTemplate;

/*
---------------------------------------------------------------------------------------------------

    ShaderPipelineCache

    - Graphics shader pipeline 은 ShaderTemplate/ShaderPermutationKey/GraphicsPipelineDesc 조합으로 ShaderPipeline 을 캐싱하고 재사용한다.
    - Compute shader pipeline 은 ShaderTemplate/ShaderPermutationKey 조합으로 ShaderPipeline 을 캐싱하고 재사용한다.
    - ShaderPipeline 은 ClearShaderTemplatePipelines/ClearAllPipelines 를 통해 Clear 될 수 있다. (Empty 상태로 바뀜)

---------------------------------------------------------------------------------------------------
*/

class ShaderPipelineCache {
public:
                                    // shaderTemplate, permutationKey, graphicsPipelineDesc 로 캐시된 그래픽스 ShaderPipeline 을 리턴한다. 캐시된게 없다면 빌드를 요청한다.
    SharedPtr<ShaderPipeline>       GetGraphicsPipeline(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, const GraphicsPipelineDesc &graphicsPipelineDesc, bool async = true);

                                    // GetGraphicsPipeline 과 같은 방식으로 준비하고, 캐시가 소유하는 ShaderPipeline 포인터를 리턴한다. 반환된 포인터는 해당 캐시 항목이 제거될 때까지 유효하다.
    ShaderPipeline *                GetGraphicsPipelinePtr(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, const GraphicsPipelineDesc &graphicsPipelineDesc, bool async = true);

                                    // shaderTemplate, permutationKey 로 캐시된 컴퓨트 ShaderPipeline 을 리턴한다. 캐시된게 없다면 빌드를 요청한다.
    SharedPtr<ShaderPipeline>       GetComputePipeline(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, bool async = true);

                                    // GetComputePipeline 과 같은 방식으로 준비하고, 캐시가 소유하는 ShaderPipeline 포인터를 리턴한다. 반환된 포인터는 해당 캐시 항목이 제거될 때까지 유효하다.
    ShaderPipeline *                GetComputePipelinePtr(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, bool async = true);

                                    // shaderTemplate 으로 캐싱된 모든 ShaderPipeline 을 제거한다. (빌드 중인 ShaderPipeline 은 폐기된다)
    void                            ClearShaderTemplatePipelines(const ShaderTemplate *shaderTemplate);

                                    // 캐싱된 모든 ShaderPipeline 을 제거한다. (빌드 중인 ShaderPipeline 은 폐기된다)
    void                            ClearAllPipelines();

                                    // shaderTemplate 으로 캐싱된 모든 ShaderPipeline 을 재빌드한다.
    void                            RebuildShaderTemplatePipelines(const SharedPtr<ShaderTemplate> &shaderTemplate);

                                    // 캐싱된 모든 ShaderPipeline 을 재빌드한다.
    void                            RebuildAllPipelines();

private:
    struct CachedGraphicsPipeline {
        const ShaderTemplate *      shaderTemplatePtr = nullptr;
        WeakPtr<ShaderTemplate>     shaderTemplateRef;
        uint64_t                    graphicsPipelineCacheHash = 0;
        SharedPtr<ShaderPipeline>   shaderPipeline;
    };

    struct CachedComputePipeline {
        const ShaderTemplate *      shaderTemplatePtr = nullptr;
        WeakPtr<ShaderTemplate>     shaderTemplateRef;
        ShaderPermutationKey        permutationKey;
        SharedPtr<ShaderPipeline>   shaderPipeline;
    };

    const CachedGraphicsPipeline *  GetOrCreateGraphicsPipelineEntry(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, const GraphicsPipelineDesc &graphicsPipelineDesc, bool async);
    const CachedGraphicsPipeline *  FindCachedGraphicsPipeline(const ShaderTemplate *shaderTemplate, uint64_t graphicsPipelineCacheHash) const;
    const CachedComputePipeline *   GetOrCreateComputePipelineEntry(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, bool async);
    const CachedComputePipeline *   FindCachedComputePipeline(const ShaderTemplate *shaderTemplate, ShaderPermutationKey permutationKey) const;
    void                            RemoveCachedGraphicsPipelineIndex(int index);
    void                            RemoveCachedComputePipelineIndex(int index);

    static uint64_t                 GenerateGraphicsPipelineCacheHash(const ShaderTemplate *shaderTemplate, ShaderPermutationKey permutationKey, const GraphicsPipelineDesc &graphicsPipelineDesc);

    BE1::Array<CachedGraphicsPipeline> cachedGraphicsPipelines;
    BE1::HashIndex                  cachedGraphicsPipelineHash{ 1024, 1024 };
    BE1::Array<CachedComputePipeline> cachedComputePipelines;
};

extern ShaderPipelineCache          shaderPipelineCache;

BE_INLINE SharedPtr<ShaderPipeline> ShaderPipelineCache::GetGraphicsPipeline(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, const GraphicsPipelineDesc &graphicsPipelineDesc, bool async) {
    const CachedGraphicsPipeline *entry = GetOrCreateGraphicsPipelineEntry(shaderTemplate, permutationKey, graphicsPipelineDesc, async);
    return entry ? entry->shaderPipeline : nullptr;
}

BE_INLINE ShaderPipeline *ShaderPipelineCache::GetGraphicsPipelinePtr(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, const GraphicsPipelineDesc &graphicsPipelineDesc, bool async) {
    const CachedGraphicsPipeline *entry = GetOrCreateGraphicsPipelineEntry(shaderTemplate, permutationKey, graphicsPipelineDesc, async);
    return entry ? entry->shaderPipeline.get() : nullptr;
}

BE_INLINE SharedPtr<ShaderPipeline> ShaderPipelineCache::GetComputePipeline(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, bool async) {
    const CachedComputePipeline *entry = GetOrCreateComputePipelineEntry(shaderTemplate, permutationKey, async);
    return entry ? entry->shaderPipeline : nullptr;
}

BE_INLINE ShaderPipeline *ShaderPipelineCache::GetComputePipelinePtr(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, bool async) {
    const CachedComputePipeline *entry = GetOrCreateComputePipelineEntry(shaderTemplate, permutationKey, async);
    return entry ? entry->shaderPipeline.get() : nullptr;
}
