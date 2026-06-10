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

    ShaderPipeline 은 ShaderTemplate::Reload 나 ClearShaderTemplate 등을 통해 Clear 될 수 있다. (Empty 상태로 바뀜)
    외부에서 SharedPtr<ShaderPipeline> 을 가지고 있으면 기본적으로 이런 대응이 자동으로 처리되지 않는다.

---------------------------------------------------------------------------------------------------
*/

class ShaderPipelineCache {
public:
                                    // shaderTemplate, permutationKey, pipelineDesc 로 캐시된 그래픽스 ShaderPipeline 를 리턴한다. 캐시된게 없다면 빌드를 요청한다.
    SharedPtr<ShaderPipeline>       GetGraphicsPipeline(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey, const ShaderPipelineDesc &pipelineDesc);

                                    // shaderTemplate, permutationKey 로 캐시된 컴퓨트 ShaderPipeline 를 리턴한다. 캐시된게 없다면 빌드를 요청한다.
    SharedPtr<ShaderPipeline>       GetComputePipeline(const SharedPtr<ShaderTemplate> &shaderTemplate, ShaderPermutationKey permutationKey);

                                    // shaderTemplate 으로 캐싱된 파이프라인을 제거한다. (빌드 중인 파이프라인은 취소된다)
    void                            ClearShaderTemplate(const ShaderTemplate *shaderTemplate);

                                    // 캐싱된 모든 파이프라인을 제거한다. (빌드 중인 파이프라인은 취소된다)
    void                            ClearAll();

                                    // shaderTemplate 으로 캐싱된 파이프라인을 재빌드한다.
    void                            RebuildShaderTemplate(const SharedPtr<ShaderTemplate> &shaderTemplate);

                                    // 캐싱된 모든 파이프라인을 재빌드한다.
    void                            RebuildAll();

private:
    struct CachedGraphicsPipeline {
        const ShaderTemplate *      shaderTemplate = nullptr;
        WeakPtr<ShaderTemplate>     shaderTemplateRef;
        uint64_t                    hash = 0;
        SharedPtr<ShaderPipeline>   pipeline;
    };

    struct CachedComputePipeline {
        const ShaderTemplate *      shaderTemplate = nullptr;
        WeakPtr<ShaderTemplate>     shaderTemplateRef;
        ShaderPermutationKey        permutationKey;
        SharedPtr<ShaderPipeline>   pipeline;
    };

    SharedPtr<ShaderPipeline>       FindCachedGraphicsPipeline(const ShaderTemplate *shaderTemplate, uint64_t hash) const;
    SharedPtr<ShaderPipeline>       FindCachedComputePipeline(const ShaderTemplate *shaderTemplate, ShaderPermutationKey permutationKey) const;
    void                            RemoveCachedGraphicsPipelineIndex(int index);

    static uint64_t                 GenerateGraphicsPipelineCacheHash(const ShaderTemplate *shaderTemplate, ShaderPermutationKey permutationKey, const ShaderPipelineDesc &pipelineDesc);

    BE1::Array<CachedGraphicsPipeline> cachedGraphicsPipelines;
    BE1::HashIndex                 cachedGraphicsPipelineHash{ 1024, 1024 };
    BE1::Array<CachedComputePipeline> cachedComputePipelines;
};

extern ShaderPipelineCache          shaderPipelineCache;
