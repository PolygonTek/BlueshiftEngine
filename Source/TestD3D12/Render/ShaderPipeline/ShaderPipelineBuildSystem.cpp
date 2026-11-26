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
#include "Profiler.h"
#include "ShaderPipelineBuildSystem.h"
#include "ShaderPipeline.h"
#include "Core/CVars.h"
#include "Platform/PlatformProcess.h"
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"

ShaderPipelineBuildSystem shaderPipelineBuildSystem;

static CVAR_MINMAX(shaderPipelineBuild_delayMsec, "0", BE1::CVar::Flag::Integer, "artificial delay per shader pipeline build in milliseconds", 0, 5000);
static CVAR(shaderPipelineBuild_forceSynchronous, "0", BE1::CVar::Flag::Bool, "force shader pipeline build requests to run synchronously");

void ShaderPipelineBuildSystem::Init() {
    assert(BE1::Engine::IsInMainThread());

    workerDelayMsec.store(shaderPipelineBuild_delayMsec.GetInteger(), std::memory_order_relaxed);
    shaderPipelineBuild_delayMsec.ClearModified();

    BE1::ScopedLock lock(requestMutex);

    if (thread) {
        return;
    }

    stopping = false;
    thread = BE1::PlatformThread::Start(ThreadProc, this);
}

void ShaderPipelineBuildSystem::Shutdown() {
    assert(BE1::Engine::IsInMainThread());

    {
        BE1::ScopedLock lock(requestMutex);

        if (!thread) {
            return;
        }

        stopping = true;
        requestCondition.Broadcast();
    }

    // 현재 진행 중인 빌드가 끝날 때까지 기다리고, 큐에 남은 빌드는 시작하지 않는다.
    BE1::PlatformThread::Join(thread);

    {
        BE1::ScopedLock lock(requestMutex);

        thread = nullptr;
        stopping = false;
    }

    // 요청 등록과 우선순위 변경은 메인 스레드 전용이므로 락 없이 빌드 요청을 취소한다.
    for (const BuildRequest &request : requests) {
        CancelBuild(request.shaderPipeline, request.isRebuild, request.serial);
    }
    requests.Clear();
    nextRequestSequence = 0;

    // 빌드 완료 처리도 모두 취소한다.
    for (const BuildCompletion &completion : completions) {
        CancelBuild(completion.shaderPipeline, completion.isRebuild, completion.serial);
    }
    completions.Clear();

    // 처리 중인 완료 결과는 비어있어야 한다.
    assert(processingCompletions.IsEmpty());
    processingCompletions.Clear();

    // 리빌드 완료 처리도 모두 취소한다.
    for (const BuildCompletion &completion : rebuildCompletions) {
        CancelBuild(completion.shaderPipeline, completion.isRebuild, completion.serial);
    }
    rebuildCompletions.Clear();
}

bool ShaderPipelineBuildSystem::IsAsyncAllowed() {
    assert(BE1::Engine::IsInMainThread());

    return !shaderPipelineBuild_forceSynchronous.GetBool();
}

bool ShaderPipelineBuildSystem::EnqueueBuild(SharedPtr<ShaderPipeline> shaderPipeline, ShaderCompileData &&compileData, bool isRebuild, uint32_t serial, BuildPriority priority) {
    assert(BE1::Engine::IsInMainThread());

    if (!shaderPipeline || !shaderPipeline->IsBuilding() || shaderPipeline->GetBuildSerial() != serial) {
        return false;
    }

    {
        BE1::ScopedLock lock(requestMutex);

        if (!thread || stopping) {
            return false;
        }

        // 재사용 슬롯의 모든 필드를 덮어써서 빌드 요청 큐에 집어넣는다.
        BuildRequest &request = requests.Alloc();
        request.shaderPipeline = std::move(shaderPipeline);
        request.compileData = std::move(compileData);
        request.isRebuild = isRebuild;
        request.serial = serial;
        request.priority = priority;
        request.sequence = nextRequestSequence++;

        requestCondition.Signal();
    }

    return true;
}

bool ShaderPipelineBuildSystem::BoostBuild(const ShaderPipeline *shaderPipeline, uint32_t serial, BuildPriority priority) {
    assert(BE1::Engine::IsInMainThread());

    if (!shaderPipeline) {
        return false;
    }

    BE1::ScopedLock lock(requestMutex);
    bool boosted = false;

    for (BuildRequest &request : requests) {
        if (request.shaderPipeline.get() != shaderPipeline || request.serial != serial) {
            continue;
        }

        if (priority > request.priority) {
            request.priority = priority;
            boosted = true;
        }
    }

    if (boosted) {
        requestCondition.Signal();
    }
    return boosted;
}

bool ShaderPipelineBuildSystem::PopNextRequest(BuildRequest &outRequest) {
    BE1::ScopedLock lock(requestMutex);

    requestCondition.Wait(lock, [this] {
        return stopping || !requests.IsEmpty();
    });

    if (stopping) {
        return false;
    }

    int bestIndex = 0;
    const int numRequests = requests.Count();
    for (int i = 1; i < numRequests; i++) {
        if (IsHigherPriority(requests[i], requests[bestIndex])) {
            bestIndex = i;
        }
    }

    outRequest = std::move(requests[bestIndex]);
    requests.RemoveIndexFast(bestIndex);
    return true;
}

void ShaderPipelineBuildSystem::Run() {
    while (1) {
        BuildRequest request;
        if (!PopNextRequest(request)) {
            break;
        }

        PROFILER_CPU_SCOPED_EVENT("ShaderPipelineBuildSystem::Run");

        const int delayMsec = workerDelayMsec.load(std::memory_order_relaxed);
        if (delayMsec > 0) {
            BE1::PlatformProcess::Sleep(MILLI2SEC(delayMsec));
        }

        if (!request.shaderPipeline->IsBuilding() || request.serial != request.shaderPipeline->GetBuildSerial()) {
            continue;
        }

        BuildCompletion completion;
        completion.shaderPipeline = std::move(request.shaderPipeline);
        completion.isRebuild = request.isRebuild;
        completion.serial = request.serial;
        completion.shaderPipeline->Prepare(request.compileData, completion.pipelineData);

        PushCompletion(std::move(completion));
    }
}

void ShaderPipelineBuildSystem::PushCompletion(BuildCompletion &&completion) {
    BE1::ScopedLock lock(completionMutex);
    completions.Append(std::move(completion));
}

void ShaderPipelineBuildSystem::ProcessBuildCompletions() {
    assert(BE1::Engine::IsInMainThread());

    if (shaderPipelineBuild_delayMsec.IsModified()) {
        workerDelayMsec.store(shaderPipelineBuild_delayMsec.GetInteger(), std::memory_order_relaxed);
        shaderPipelineBuild_delayMsec.ClearModified();
    }

    assert(processingCompletions.IsEmpty());

    {
        BE1::ScopedLock lock(completionMutex);
        processingCompletions.Swap(completions);
    }

    for (BuildCompletion &completion : processingCompletions) {
        // 리빌드는 ShaderPipeline 이 보유한 PSO 를 새로 빌드한 PSO 로 교체하므로,
        // 렌더 완료 후 적용하도록 ProcessPendingRebuilds() 로 미룬다.
        if (completion.isRebuild) {
            rebuildCompletions.Append(std::move(completion));
            continue;
        }

        ProcessBuildCompletion(completion);
        completion = {};
    }

    // 원소를 모두 이동하거나 정리했으므로 배열 용량만 재사용한다.
    processingCompletions.SetCount(0, false);
}

void ShaderPipelineBuildSystem::ProcessPendingRebuilds() {
    PROFILER_CPU_SCOPED_EVENT("ShaderPipelineBuildSystem::ProcessPendingRebuilds");

    BE1::Array<BuildCompletion> batch;
    if (!CollectPendingRebuilds(batch)) {
        return;
    }

    // 렌더 프런트엔드 프레임 작성 중에는 기존 리소스를 교체하면 안 된다.
    assert(RenderContext::activeContext == nullptr);

    // 모든 렌더 스레드가 자원 사용을 마치기를 기다린다.
    renderSystem->WaitAllRenderCompleted();

    ApplyPendingRebuilds(batch);
}

bool ShaderPipelineBuildSystem::CollectPendingRebuilds(BE1::Array<BuildCompletion> &batch) {
    assert(BE1::Engine::IsInMainThread());
    assert(batch.IsEmpty());

    if (rebuildCompletions.IsEmpty()) {
        return false;
    }

    batch.Swap(rebuildCompletions);

    // 렌더 완료 대기가 필요한지 판단하기 전에 무효화되었거나 빌드에 실패한 결과를 폐기한다.
    for (int i = 0; i < batch.Count();) {
        if (TryDiscardInvalidCompletion(batch[i])) {
            // RemoveIndexFast 는 마지막 원소를 소멸시키지 않으므로 참조를 먼저 해제한다.
            batch[i] = {};
            batch.RemoveIndexFast(i);
        } else {
            i++;
        }
    }

    if (batch.IsEmpty()) {
        batch.Swap(rebuildCompletions);
        return false;
    }

    return true;
}

void ShaderPipelineBuildSystem::ApplyPendingRebuilds(BE1::Array<BuildCompletion> &batch) {
    assert(BE1::Engine::IsInMainThread());
    assert(RenderContext::activeContext == nullptr);

    for (BuildCompletion &completion : batch) {
        // 적용 직전에 ProcessBuildCompletion 에서 시리얼과 빌드 상태를 다시 확인한다.
        ProcessBuildCompletion(completion);
        completion = {};
    }

    // 원소의 참조와 준비 데이터는 정리했고, 배열 용량은 다음 리빌드에서 재사용한다.
    batch.SetCount(0, false);

    if (rebuildCompletions.IsEmpty()) {
        rebuildCompletions.Swap(batch);
    }
}

bool ShaderPipelineBuildSystem::TryDiscardInvalidCompletion(BuildCompletion &completion) {
    const SharedPtr<ShaderPipeline> &pipeline = completion.shaderPipeline;

    if (!pipeline || completion.serial != pipeline->GetBuildSerial() || !pipeline->IsBuilding()) {
        return true;
    }

    if (!completion.pipelineData.pso) {
        CancelBuild(pipeline, completion.isRebuild, completion.serial);
        return true;
    }

    return false;
}

void ShaderPipelineBuildSystem::ProcessBuildCompletion(BuildCompletion &completion) {
    // 배치를 준비한 이후 Reset 또는 다른 빌드 완료로 결과가 무효화되었을 수 있으므로 다시 확인한다.
    if (TryDiscardInvalidCompletion(completion)) {
        return;
    }

    completion.shaderPipeline->ApplyPrepared(completion.pipelineData);
    completion.shaderPipeline->SetState(ShaderPipeline::State::Ready);
}

void ShaderPipelineBuildSystem::CancelBuild(const SharedPtr<ShaderPipeline> &pipeline, bool isRebuild, uint32_t serial) {
    if (pipeline && pipeline->GetBuildSerial() == serial && pipeline->IsBuilding()) {
        pipeline->SetState(isRebuild ? ShaderPipeline::State::Ready : ShaderPipeline::State::Failed);
    }
}

unsigned int ShaderPipelineBuildSystem::ThreadProc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName("ShaderPipelineBuildSystem::ThreadProc");

    BE1::SIMD::SetDenormalFlushMode(true);

    static_cast<ShaderPipelineBuildSystem *>(param)->Run();

    return 0;
}
