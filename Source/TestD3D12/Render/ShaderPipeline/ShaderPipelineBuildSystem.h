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

#include <atomic>

#include "ShaderCompileData.h"
#include "ShaderPipeline.h"

/*
---------------------------------------------------------------------------------------------------

    Shader Pipeline Build System

    백그라운드 셰이더 컴파일과 PSO 생성을 담당한다.
    요청 등록과 결과 적용은 메인 스레드에서 수행하며, 워커는 사용 중인 PSO 를 교체하지 않는다.

---------------------------------------------------------------------------------------------------
*/

class ShaderPipelineBuildSystem {
    friend class RenderSystem;

public:
    enum class BuildPriority : int8_t {
        Background                  = 0,
        Normal                      = 10,
        High                        = 20,
        Critical                    = 30
    };

    ShaderPipelineBuildSystem() = default;
    ShaderPipelineBuildSystem(const ShaderPipelineBuildSystem &) = delete;
    ShaderPipelineBuildSystem &operator=(const ShaderPipelineBuildSystem &) = delete;

    void                            Init();
    void                            Shutdown();

    static bool                     IsAsyncAllowed();

    bool                            EnqueueBuild(SharedPtr<ShaderPipeline> shaderPipeline, ShaderCompileData &&compileData, bool isRebuild, uint32_t serial, BuildPriority priority = BuildPriority::High);
    bool                            BoostBuild(const ShaderPipeline *shaderPipeline, uint32_t serial, BuildPriority priority);

                                    /// 최초 빌드 결과를 처리하고, 리빌드 결과는 안전한 교체 시점까지 미룬다.
    void                            ProcessBuildCompletions();

                                    /// 동기 리빌드 대기를 위해 렌더 완료 대기와 결과 적용을 함께 수행한다.
    void                            ProcessPendingRebuilds();

private:
    struct BuildRequest {
        SharedPtr<ShaderPipeline>   shaderPipeline;
        ShaderCompileData           compileData;
        uint32_t                    serial = 0;
        uint64_t                    sequence = 0;                       // 큐에 들어온 순서
        BuildPriority               priority = BuildPriority::High;
        bool                        isRebuild = false;
    };

    struct BuildCompletion {
        SharedPtr<ShaderPipeline>   shaderPipeline;
        ShaderPipeline::InternalData pipelineData;
        uint32_t                    serial = 0;
        bool                        isRebuild = false;
    };

    bool                            PopNextRequest(BuildRequest &outRequest);
    void                            Run();
    void                            PushCompletion(BuildCompletion &&completion);

    bool                            CollectPendingRebuilds(BE1::Array<BuildCompletion> &batch);
    void                            ApplyPendingRebuilds(BE1::Array<BuildCompletion> &batch);

    bool                            TryDiscardInvalidCompletion(BuildCompletion &completion);
    void                            ProcessBuildCompletion(BuildCompletion &completion);

    static void                     CancelBuild(const SharedPtr<ShaderPipeline> &shaderPipeline, bool isRebuild, uint32_t serial);

    static bool                     IsHigherPriority(const BuildRequest &lhs, const BuildRequest &rhs);
    static unsigned int             ThreadProc(void *param);

    BE1::PlatformThread *           thread = nullptr;
    BE1::Mutex                      requestMutex;
    BE1::Mutex                      completionMutex;
    BE1::Condition                  requestCondition;

    BE1::Array<BuildRequest>        requests;
    BE1::Array<BuildCompletion>     completions;
    BE1::Array<BuildCompletion>     processingCompletions;
    BE1::Array<BuildCompletion>     rebuildCompletions;
    uint64_t                        nextRequestSequence = 0;
    std::atomic<int>                workerDelayMsec { 0 };

    bool                            stopping = false;
};

BE_INLINE bool ShaderPipelineBuildSystem::IsHigherPriority(const BuildRequest &lhs, const BuildRequest &rhs) {
    if (lhs.priority != rhs.priority) {
        return lhs.priority > rhs.priority;
    }
    return lhs.sequence < rhs.sequence;
}

extern ShaderPipelineBuildSystem    shaderPipelineBuildSystem;
