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
#include "LoadingSystem.h"
#include "Core/CVars.h"
#include "Platform/PlatformProcess.h"
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"
#include "Render/Texture/TextureManager.h"

LoadingSystem loadingSystem;

static CVAR_MINMAX(loading_delayMsec, "0", BE1::CVar::Flag::Integer, "artificial delay per loading request in milliseconds", 0, 5000);
static CVAR(loading_forceSynchronous, "0", BE1::CVar::Flag::Bool, "force asynchronous resource load requests to run synchronously");

void LoadingSystem::Init() {
    assert(BE1::Engine::IsInMainThread());

    workerDelayMsec.store(loading_delayMsec.GetInteger(), std::memory_order_relaxed);
    loading_delayMsec.ClearModified();

    BE1::ScopedLock lock(requestMutex);

    if (thread) {
        return;
    }

    stopping = false;
    thread = BE1::PlatformThread::Start(ThreadProc, this);
}

void LoadingSystem::Shutdown() {
    assert(BE1::Engine::IsInMainThread());

    {
        BE1::ScopedLock lock(requestMutex);

        if (!thread) {
            return;
        }

        stopping = true;
        requestCondition.Broadcast();
    }

    // 현재 진행 중인 로딩이 끝날 때까지 기다리고, 큐에 남은 로딩은 시작하지 않는다.
    BE1::PlatformThread::Join(thread);

    {
        BE1::ScopedLock lock(requestMutex);

        thread = nullptr;
        stopping = false;

        // 다른 스레드의 우선순위 변경과 겹치지 않도록 락 안에서 로딩 요청을 취소한다.
        for (const LoadRequest &request : requests) {
            CancelLoad(request);
        }
        requests.Clear();
        nextRequestSequence = 0;
    }

    // 로딩 완료 처리도 모두 취소한다.
    for (const LoadCompletion &completion : completions) {
        CancelLoad(completion);
    }
    completions.Clear();

    // 처리 중인 완료 결과는 비어있어야 한다.
    assert(processingCompletions.IsEmpty());
    processingCompletions.Clear();

    // 리로드 완료 처리도 모두 취소한다.
    for (const LoadCompletion &completion : reloadCompletions) {
        CancelLoad(completion);
    }
    reloadCompletions.Clear();
}

bool LoadingSystem::IsAsyncAllowed() {
    assert(BE1::Engine::IsInMainThread());

    return !loading_forceSynchronous.GetBool();
}

bool LoadingSystem::EnqueueTextureLoad(SharedPtr<Texture> texture, const char *filename, Texture::Flag flags, bool isReload, uint32_t serial, LoadPriority priority) {
    if (!texture || !filename || !filename[0]) {
        return false;
    }

    const uint32_t streamingMinResidentMaxDimension = BE1::HasFlag(flags, Texture::Flag::Streaming) ? textureManager.GetStreamingTextureMinResidentMaxDimension() : 1;

    {
        BE1::ScopedLock lock(requestMutex);

        if (!thread || stopping) {
            return false;
        }

        LoadRequest request;
        request.texture = std::move(texture);
        request.filename = filename;
        request.textureFlags = flags;
        request.streamingMinResidentMaxDimension = streamingMinResidentMaxDimension;
        request.isReload = isReload;
        request.serial = serial;
        request.priority = priority;
        request.sequence = nextRequestSequence++;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        requestCondition.Signal();
    }

    return true;
}

bool LoadingSystem::EnqueueTextureStreamingReplacement(SharedPtr<Texture> texture, const char *filename, int32_t firstMip, LoadPriority priority) {
    if (!texture || !filename || !filename[0]) {
        return false;
    }

    // 텍스쳐를 교체할 것이므로 이미 생성되어 있어야 한다.
    if (!texture->GetRHITexture()) {
        return false;
    }

    if (firstMip < 0 || firstMip >= texture->GetStreaming()->originalMipLevels) {
        return false;
    }

    {
        BE1::ScopedLock lock(requestMutex);

        if (!thread || stopping) {
            return false;
        }

        LoadRequest request;
        request.texture = std::move(texture);
        request.filename = filename;
        request.textureFlags = request.texture->GetFlags();
        request.isStreamingReplacement = true;
        request.originalMipLevels = request.texture->GetStreaming()->originalMipLevels;
        request.firstMip = firstMip;
        request.serial = request.texture->GetLoadSerial();
        request.priority = priority;
        request.sequence = nextRequestSequence++;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        requestCondition.Signal();
    }

    return true;
}

bool LoadingSystem::EnqueueMeshLoad(SharedPtr<Mesh> mesh, const char *filename, bool isReload, uint32_t serial, LoadPriority priority) {
    if (!mesh || !filename || !filename[0]) {
        return false;
    }

    {
        BE1::ScopedLock lock(requestMutex);

        if (!thread || stopping) {
            return false;
        }

        LoadRequest request;
        request.mesh = std::move(mesh);
        request.filename = filename;
        request.isReload = isReload;
        request.serial = serial;
        request.priority = priority;
        request.sequence = nextRequestSequence++;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        requestCondition.Signal();
    }

    return true;
}

bool LoadingSystem::EnqueueSkeletonLoad(SharedPtr<Skeleton> skeleton, const char *filename, bool isReload, uint32_t serial, LoadPriority priority) {
    if (!skeleton || !filename || !filename[0]) {
        return false;
    }

    {
        BE1::ScopedLock lock(requestMutex);

        if (!thread || stopping) {
            return false;
        }

        LoadRequest request;
        request.skeleton = std::move(skeleton);
        request.filename = filename;
        request.isReload = isReload;
        request.serial = serial;
        request.priority = priority;
        request.sequence = nextRequestSequence++;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        requestCondition.Signal();
    }

    return true;
}

bool LoadingSystem::EnqueueAnimLoad(SharedPtr<Anim> anim, const char *filename, bool isReload, uint32_t serial, LoadPriority priority) {
    if (!anim || !filename || !filename[0]) {
        return false;
    }

    {
        BE1::ScopedLock lock(requestMutex);

        if (!thread || stopping) {
            return false;
        }

        LoadRequest request;
        request.anim = std::move(anim);
        request.filename = filename;
        request.isReload = isReload;
        request.serial = serial;
        request.priority = priority;
        request.sequence = nextRequestSequence++;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        requestCondition.Signal();
    }

    return true;
}

bool LoadingSystem::BoostTextureLoad(const Texture *texture, uint32_t serial, LoadPriority priority) {
    if (!texture) {
        return false;
    }

    BE1::ScopedLock lock(requestMutex);
    bool boosted = false;

    for (LoadRequest &request : requests) {
        if (request.texture.get() != texture || request.serial != serial) {
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

bool LoadingSystem::BoostMeshLoad(const Mesh *mesh, uint32_t serial, LoadPriority priority) {
    if (!mesh) {
        return false;
    }

    BE1::ScopedLock lock(requestMutex);
    bool boosted = false;

    for (LoadRequest &request : requests) {
        if (request.mesh.get() != mesh || request.serial != serial) {
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

bool LoadingSystem::BoostSkeletonLoad(const Skeleton *skeleton, uint32_t serial, LoadPriority priority) {
    if (!skeleton) {
        return false;
    }

    BE1::ScopedLock lock(requestMutex);
    bool boosted = false;

    for (LoadRequest &request : requests) {
        if (request.skeleton.get() != skeleton || request.serial != serial) {
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

bool LoadingSystem::BoostAnimLoad(const Anim *anim, uint32_t serial, LoadPriority priority) {
    if (!anim) {
        return false;
    }

    BE1::ScopedLock lock(requestMutex);
    bool boosted = false;

    for (LoadRequest &request : requests) {
        if (request.anim.get() != anim || request.serial != serial) {
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

bool LoadingSystem::PopNextRequest(LoadRequest &outRequest) {
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

void LoadingSystem::Run() {
    while (1) {
        LoadRequest request;
        if (!PopNextRequest(request)) {
            break;
        }

        PROFILER_CPU_SCOPED_EVENT("LoadingSystem::Run");

        const int delayMsec = workerDelayMsec.load(std::memory_order_relaxed);
        if (delayMsec > 0) {
            BE1::PlatformProcess::Sleep(MILLI2SEC(delayMsec));
        }

        if (request.texture) {
            if (request.isStreamingReplacement) {
                ProcessTextureStreamingReplacementRequest(std::move(request));
            } else {
                ProcessTextureLoadRequest(std::move(request));
            }
        } else if (request.mesh) {
            ProcessMeshLoadRequest(std::move(request));
        } else if (request.skeleton) {
            ProcessSkeletonLoadRequest(std::move(request));
        } else if (request.anim) {
            ProcessAnimLoadRequest(std::move(request));
        }
    }
}

void LoadingSystem::ProcessTextureLoadRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessTextureLoadRequest");

    // 로딩 요청을 처리하려는 사이에 로딩이 취소되었거나 시리얼이 변경되었다면, 로딩을 하지 않는다.
    if (!request.texture->IsLoading() || request.serial != request.texture->GetLoadSerial()) {
        return;
    }

    LoadCompletion completion;
    completion.texture = std::move(request.texture);
    completion.isReload = request.isReload;
    completion.serial = request.serial;

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    BE1::Image::MipRange mipRange;
    bool useMipRange = false;

    BE_LOG("Loading texture '%s'...\n", request.filename.c_str());

    // Texture::Flag::Streaming 이 켜져있으면 스트리밍 가능한 파일의 partial mip loading 을 시도한다.
    if (BE1::HasFlag(request.textureFlags, Texture::Flag::Streaming)) {
        // 이미지 파일에서 r_textureStreamingMinResidentMaxDimension 이하의 크기까지의 mip 레벨만 initial loading
        if (image->LoadMipRangeForMaxDimension(request.filename.c_str(), request.streamingMinResidentMaxDimension, &mipRange)) {
            useMipRange = mipRange.mipCount < mipRange.originalMipLevels;
        }
    }

    if (image->IsEmpty()) {
        // 이미지 파일 full loading
        image->Load(request.filename.c_str());
    }

    if (image->IsEmpty()) {
        BE_WARNLOG("LoadingSystem::ProcessTextureLoadRequest: couldn't load texture \"%s\"\n", request.filename.c_str());
        PushCompletion(std::move(completion));
        return;
    }

    if (BE1::HasFlag(request.textureFlags, Texture::Flag::LinearColorSpace)) {
        image->SetGammaSpace(BE1::Image::GammaSpace::Linear);
    }

    int textureDepth = useMipRange ? mipRange.originalDepth : image->GetDepth();

    RHI::TextureType textureType;
    if (image->Is3D() || textureDepth > 1) {
        textureType = RHI::TextureType::Texture3D;
    } else if (image->IsCubeMap()) {
        textureType = RHI::TextureType::TextureCube;
    } else if (image->GetArraySize() > 1) {
        textureType = RHI::TextureType::Texture2DArray;
    } else {
        textureType = RHI::TextureType::Texture2D;
    }

    if (useMipRange) {
        completion.mipRange = mipRange;
        completion.success = Texture::PrepareDirect(textureType, std::move(image), request.textureFlags | Texture::Flag::LoadedFromFile, completion.textureData);
    } else {
        // .btex 파일은 압축이나 밉맵 생성없이 있는 원본 그대로의 데이터를 사용한다.
        const bool preserveSourceData = request.filename.CheckExtension(".btex");

        completion.success = Texture::Prepare(textureType, std::move(image), request.textureFlags | Texture::Flag::LoadedFromFile, completion.textureData, preserveSourceData);
    }

    if (completion.textureData.rhiTexture) {
        RHI::renderer->SetName(completion.textureData.rhiTexture, request.filename);
    }

    // 완료 큐에 집어넣는다.
    PushCompletion(std::move(completion));
}

void LoadingSystem::ProcessTextureStreamingReplacementRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessTextureStreamingReplacementRequest");

    // 로딩 요청을 처리하려는 사이에 시리얼이 변경되었다면, 로딩을 하지 않는다.
    if (request.serial != request.texture->GetLoadSerial()) {
        return;
    }

    LoadCompletion completion;
    completion.texture = std::move(request.texture);
    completion.serial = request.serial;

    // 기존 텍스쳐의 교체 요청을 받았다고 표시
    completion.isStreamingReplacement = true;

    int32_t mipCount = request.originalMipLevels - request.firstMip;

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    BE1::Image::MipRange mipRange;

    BE_LOG("Loading texture '%s' [%i, %i]...\n", request.filename.c_str(), request.firstMip, request.firstMip + mipCount - 1);

    image->LoadMipRange(request.filename.c_str(), request.firstMip, mipCount, &mipRange);
    
    if (image->IsEmpty()) {
        BE_WARNLOG("LoadingSystem::ProcessTextureStreamingReplacementRequest: couldn't load texture \"%s\"\n", request.filename.c_str());
        PushCompletion(std::move(completion));
        return;
    }

    if (BE1::HasFlag(request.textureFlags, Texture::Flag::LinearColorSpace)) {
        image->SetGammaSpace(BE1::Image::GammaSpace::Linear);
    }

    int textureDepth = mipRange.originalDepth;

    RHI::TextureType textureType;
    if (image->Is3D() || textureDepth > 1) {
        textureType = RHI::TextureType::Texture3D;
    } else if (image->IsCubeMap()) {
        textureType = RHI::TextureType::TextureCube;
    } else if (image->GetArraySize() > 1) {
        textureType = RHI::TextureType::Texture2DArray;
    } else {
        textureType = RHI::TextureType::Texture2D;
    }

    completion.mipRange = mipRange;
    completion.success = Texture::PrepareDirect(textureType, std::move(image), request.textureFlags | Texture::Flag::LoadedFromFile, completion.textureData);

    PushCompletion(std::move(completion));
}

void LoadingSystem::ProcessMeshLoadRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessMeshLoadRequest");

    // 로딩 요청을 처리하려는 사이에 로딩이 취소되었거나 시리얼이 변경되었다면, 로딩을 하지 않는다.
    if (!request.mesh->IsLoading() || request.serial != request.mesh->GetLoadSerial()) {
        return;
    }

    LoadCompletion completion;
    completion.mesh = std::move(request.mesh);
    completion.isReload = request.isReload;
    completion.serial = request.serial;
    completion.success = Mesh::Prepare(request.filename, completion.meshData);

    PushCompletion(std::move(completion));
}

void LoadingSystem::ProcessSkeletonLoadRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessSkeletonLoadRequest");

    // 로딩 요청을 처리하려는 사이에 로딩이 취소되었거나 시리얼이 변경되었다면, 로딩을 하지 않는다.
    if (!request.skeleton->IsLoading() || request.serial != request.skeleton->GetLoadSerial()) {
        return;
    }

    LoadCompletion completion;
    completion.skeleton = std::move(request.skeleton);
    completion.isReload = request.isReload;
    completion.serial = request.serial;
    completion.success = Skeleton::Prepare(request.filename, completion.skeletonData);

    PushCompletion(std::move(completion));
}

void LoadingSystem::ProcessAnimLoadRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessAnimLoadRequest");

    // 로딩 요청을 처리하려는 사이에 로딩이 취소되었거나 시리얼이 변경되었다면, 로딩을 하지 않는다.
    if (!request.anim->IsLoading() || request.serial != request.anim->GetLoadSerial()) {
        return;
    }

    LoadCompletion completion;
    completion.anim = std::move(request.anim);
    completion.isReload = request.isReload;
    completion.serial = request.serial;
    completion.success = Anim::Prepare(request.filename, completion.animData);

    PushCompletion(std::move(completion));
}

void LoadingSystem::PushCompletion(LoadCompletion &&completion) {
    BE1::ScopedLock lock(completionMutex);
    completions.Append(std::move(completion));
}

void LoadingSystem::ProcessLoadCompletions() {
    assert(BE1::Engine::IsInMainThread());

    if (loading_delayMsec.IsModified()) {
        workerDelayMsec.store(loading_delayMsec.GetInteger(), std::memory_order_relaxed);
        loading_delayMsec.ClearModified();
    }

    assert(processingCompletions.IsEmpty());

    {
        BE1::ScopedLock lock(completionMutex);
        processingCompletions.Swap(completions);
    }

    for (LoadCompletion &completion : processingCompletions) {
        // 리로드는 리소스의 내부 데이터를 새로 로딩한 데이터로 교체하므로,
        // 렌더 완료 후 적용하도록 ProcessPendingReloads() 로 미룬다.
        if (completion.isReload) {
            reloadCompletions.Append(std::move(completion));
            continue;
        }

        ProcessLoadCompletion(completion);
        completion = {};
    }

    // 원소를 모두 이동하거나 정리했으므로 배열 용량만 재사용한다.
    processingCompletions.SetCount(0, false);
}

void LoadingSystem::ProcessPendingReloads() {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessPendingReloads");

    BE1::Array<LoadCompletion> batch;
    if (!CollectPendingReloads(batch)) {
        return;
    }

    // 렌더 프런트엔드 프레임 작성 중에는 기존 리소스를 교체하면 안 된다.
    assert(RenderContext::activeContext == nullptr);

    // 모든 렌더 스레드가 자원 사용을 마치기를 기다린다.
    renderSystem->WaitAllRenderCompleted();

    ApplyPendingReloads(batch);
}

bool LoadingSystem::CollectPendingReloads(BE1::Array<LoadCompletion> &batch) {
    assert(BE1::Engine::IsInMainThread());
    assert(batch.IsEmpty());

    if (reloadCompletions.IsEmpty()) {
        return false;
    }

    batch.Swap(reloadCompletions);

    // 렌더 완료 대기가 필요한지 판단하기 전에 무효화되었거나 로딩에 실패한 결과를 폐기한다.
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
        batch.Swap(reloadCompletions);
        return false;
    }

    return true;
}

void LoadingSystem::ApplyPendingReloads(BE1::Array<LoadCompletion> &batch) {
    assert(BE1::Engine::IsInMainThread());
    assert(RenderContext::activeContext == nullptr);

    for (LoadCompletion &completion : batch) {
        // 적용 직전에 ProcessLoadCompletion 에서 시리얼을 다시 확인한다.
        ProcessLoadCompletion(completion);
        completion = {};
    }

    // 원소의 참조와 준비 데이터는 정리했고, 배열 용량은 다음 리로드에서 재사용한다.
    batch.SetCount(0, false);

    if (reloadCompletions.IsEmpty()) {
        reloadCompletions.Swap(batch);
    }
}

bool LoadingSystem::TryDiscardInvalidCompletion(LoadCompletion &completion) {
    if (completion.texture) {
        const SharedPtr<Texture> &texture = completion.texture;

        // 로딩 요청 이후 시리얼이 변경된 경우, 새로 만든 텍스쳐를 폐기한다.
        if (completion.serial != texture->GetLoadSerial()) {
            return true;
        }

#ifdef USE_MIPMAP_STREAMING
        // 교체 요청을 받았을 경우
        if (completion.isStreamingReplacement) {
            // 교체할 텍스쳐를 로딩하지 못했을 경우
            if (!completion.success || !completion.textureData.rhiTexture) {
                BE_ERRLOG("Texture streaming replacement failed: texture '%s', requested first mip %i\n", texture->GetHashName(), texture->GetStreaming()->requestedFirstMip);

                // 스트리밍 중단
                textureManager.UnregisterStreamingTexture(texture.get());
                return true;
            }

            return false;
        }
#endif

        // 로딩이 실패한 경우
        if (!completion.success) {
            texture->SetState(completion.isReload ? Texture::State::Ready : Texture::State::Failed);
            return true;
        }

        return false;
    }

    if (completion.mesh) {
        // 로딩 요청 이후 시리얼이 변경된 경우, 새로 만든 메시를 폐기한다.
        if (completion.serial != completion.mesh->GetLoadSerial()) {
            return true;
        }

        // 로딩이 실패한 경우
        if (!completion.success) {
            completion.mesh->SetState(completion.isReload ? Mesh::State::Ready : Mesh::State::Failed);
            return true;
        }

        return false;
    }

    if (completion.skeleton) {
        // 로딩 요청 이후 시리얼이 변경된 경우, 새로 만든 스켈레톤을 폐기한다.
        if (completion.serial != completion.skeleton->GetLoadSerial()) {
            return true;
        }

        // 로딩이 실패한 경우
        if (!completion.success) {
            completion.skeleton->SetState(completion.isReload ? Skeleton::State::Ready : Skeleton::State::Failed);
            return true;
        }

        return false;
    }

    if (completion.anim) {
        // 로딩 요청 이후 시리얼이 변경된 경우, 새로 로딩한 애니메이션 데이터를 폐기한다.
        if (completion.serial != completion.anim->GetLoadSerial()) {
            return true;
        }

        // 로딩이 실패한 경우
        if (!completion.success) {
            completion.anim->SetState(completion.isReload ? Anim::State::Ready : Anim::State::Failed);
            return true;
        }

        return false;
    }

    return true;
}

void LoadingSystem::ProcessLoadCompletion(LoadCompletion &completion) {
    // 적용 전에 시리얼과 로딩 결과를 다시 확인한다.
    if (TryDiscardInvalidCompletion(completion)) {
        return;
    }

    if (completion.texture) {
        ProcessTextureLoadCompletion(completion);
    } else if (completion.mesh) {
        ProcessMeshLoadCompletion(completion);
    } else if (completion.skeleton) {
        ProcessSkeletonLoadCompletion(completion);
    } else if (completion.anim) {
        ProcessAnimLoadCompletion(completion);
    }
}

void LoadingSystem::ProcessTextureLoadCompletion(LoadCompletion &completion) {
    const SharedPtr<Texture> &texture = completion.texture;

#ifdef USE_MIPMAP_STREAMING
    // 교체 요청을 받았을 경우
    if (completion.isStreamingReplacement) {
        // 교체할 RHI::Texture 포인터를 담아 놓는다.
        texture->GetStreaming()->pendingReplacementTexture = completion.textureData.rhiTexture;
        completion.textureData.rhiTexture = nullptr;
        return;
    }
#endif

    // 준비된 텍스쳐로 세팅한다.
    texture->ApplyPrepared(completion.textureData);

#ifdef USE_MIPMAP_STREAMING
    // 스트리밍 초기 데이터인 경우
    if (completion.mipRange.mipCount > 0) {
        // 스트리밍 텍스쳐로 등록한다.
        textureManager.RegisterStreamingTexture(texture, completion.mipRange);
    }
#endif

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    texture->SetState(Texture::State::Ready);
}

void LoadingSystem::ProcessMeshLoadCompletion(LoadCompletion &completion) {
    const SharedPtr<Mesh> &mesh = completion.mesh;

    // 준비된 메시로 세팅한다.
    mesh->ApplyPrepared(completion.meshData);

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    mesh->SetState(Mesh::State::Ready);
}

void LoadingSystem::ProcessSkeletonLoadCompletion(LoadCompletion &completion) {
    const SharedPtr<Skeleton> &skeleton = completion.skeleton;

    // 준비된 스켈레톤으로 세팅한다.
    skeleton->ApplyPrepared(completion.skeletonData);

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    skeleton->SetState(Skeleton::State::Ready);
}

void LoadingSystem::ProcessAnimLoadCompletion(LoadCompletion &completion) {
    const SharedPtr<Anim> &anim = completion.anim;

    // 준비된 Anim 으로 세팅한다.
    anim->ApplyPrepared(completion.animData);

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    anim->SetState(Anim::State::Ready);
}

template <typename Resource>
static void CancelResourceLoad(Resource *resource, uint32_t serial) {
    if (serial != resource->GetLoadSerial()) {
        return;
    }

    using State = typename Resource::State;
    const State state = resource->GetState();

    // 취소한 최초 로딩은 실패로 끝내고, 리로드는 기존 데이터를 계속 사용할 수 있게 한다.
    if (state == State::Loading) {
        resource->SetState(State::Failed);
    } else if (state == State::Reloading) {
        resource->SetState(State::Ready);
    }
}

template <typename PendingLoad>
void LoadingSystem::CancelLoad(const PendingLoad &load) {
    if (load.texture) {
        if (load.isStreamingReplacement) {
            if (load.serial != load.texture->GetLoadSerial()) {
                return;
            }

            // 이미 TextureManager 로 넘긴 교체 텍스쳐와 그에 대응하는 mip 정보는 유지한다.
            Texture::Streaming &streaming = *load.texture->GetStreaming();
            if (!streaming.pendingReplacementTexture) {
                streaming.requestedFirstMip = -1;
            }
        } else {
            CancelResourceLoad(load.texture.get(), load.serial);
        }
    } else if (load.mesh) {
        CancelResourceLoad(load.mesh.get(), load.serial);
    } else if (load.skeleton) {
        CancelResourceLoad(load.skeleton.get(), load.serial);
    } else if (load.anim) {
        CancelResourceLoad(load.anim.get(), load.serial);
    }
}

unsigned int LoadingSystem::ThreadProc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName("LoadingSystem::ThreadProc");

    BE1::SIMD::SetDenormalFlushMode(true);

    static_cast<LoadingSystem *>(param)->Run();

    return 0;
}
