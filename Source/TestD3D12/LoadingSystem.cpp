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
#include "LoadingSystem.h"
#include "Core/CVars.h"
#include "Platform/PlatformProcess.h"
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"
#include "Render/Texture/TextureManager.h"

LoadingSystem      loadingSystem;

static CVAR_MINMAX(loading_delayMsec, "0", BE1::CVar::Flag::Integer, "artificial delay per loading request in milliseconds", 0, 5000);
static CVAR(loading_forceSynchronous, "0", BE1::CVar::Flag::Bool, "force asynchronous load and shader pipeline build requests to run synchronously");

void LoadingSystem::Init() {
    UpdateWorkerSettings();
    Start();
}

void LoadingSystem::Shutdown() {
    Stop();
}

bool LoadingSystem::IsAsyncAllowed() {
    assert(BE1::Engine::IsInMainThread());

    return !loading_forceSynchronous.GetBool();
}

void LoadingSystem::Start() {
    if (thread) {
        return;
    }
    thread = BE1::PlatformThread::Start(LoadingSystem::ThreadProc, this);
}

void LoadingSystem::Stop() {
    if (!thread) {
        return;
    }

    {
        BE1::ScopedLock lock(requestMutex);
        stopping = true;
        requestCondition.Broadcast();
    }

    BE1::PlatformThread::Join(thread);
    thread = nullptr;

    requests.Clear();
    stopping = false;

    {
        BE1::ScopedLock lock(completionMutex);
        for (LoadCompletion &completion : completions) {
            DiscardCompletion(completion);
        }
        completions.Clear();
    }

    // reloadCompletions 은 메인스레드에서 쌓이기 때문에 락이 필요없음
    for (LoadCompletion &completion : reloadCompletions) {
        DiscardCompletion(completion);
    }
    reloadCompletions.Clear();

    pendingMaterialReloads.Clear();
    pendingShaderTemplateReloads.Clear();
}

bool LoadingSystem::EnqueueTextureLoad(SharedPtr<Texture> texture, const char *filename, Texture::Flag flags, bool isReload, uint32_t serial, LoadPriority priority) {
    if (!texture || !filename || !filename[0]) {
        return false;
    }

    const uint32_t streamingMinResidentMaxDimension = BE1::HasFlag(flags, Texture::Flag::Streaming) ? textureManager.GetStreamingTextureMinResidentMaxDimension() : 1;

    {
        BE1::ScopedLock lock(requestMutex);

        if (stopping) {
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

        if (stopping) {
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

        if (stopping) {
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

        if (stopping) {
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

        if (stopping) {
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

bool LoadingSystem::EnqueueShaderPipelineBuild(SharedPtr<ShaderPipeline> shaderPipeline, ShaderCompileData &&compileData, uint32_t serial, LoadPriority priority) {
    if (!shaderPipeline) {
        return false;
    }

    {
        BE1::ScopedLock lock(requestMutex);

        if (stopping) {
            return false;
        }

        LoadRequest request;
        request.shaderPipeline = std::move(shaderPipeline);
        request.shaderCompileData = std::move(compileData);
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

        if ((int)request.priority < (int)priority) {
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

        if ((int)request.priority < (int)priority) {
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

        if ((int)request.priority < (int)priority) {
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

        if ((int)request.priority < (int)priority) {
            request.priority = priority;
            boosted = true;
        }
    }

    if (boosted) {
        requestCondition.Signal();
    }
    return boosted;
}

bool LoadingSystem::BoostShaderPipelineBuild(const ShaderPipeline *shaderPipeline, uint32_t serial, LoadPriority priority) {
    if (!shaderPipeline) {
        return false;
    }

    BE1::ScopedLock lock(requestMutex);
    bool boosted = false;

    for (LoadRequest &request : requests) {
        if (request.shaderPipeline.get() != shaderPipeline || request.serial != serial) {
            continue;
        }

        if ((int)request.priority < (int)priority) {
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
        return requests.Count() > 0 || stopping;
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

void LoadingSystem::UpdateWorkerSettings() {
    assert(BE1::Engine::IsInMainThread());

    workerDelayMsec.store(loading_delayMsec.GetInteger(), std::memory_order_relaxed);
}

void LoadingSystem::Run() {
    while (1) {
        PROFILER_CPU_SCOPED_EVENT("LoadingSystem::Run");

        LoadRequest request;
        if (!PopNextRequest(request)) {
            break;
        }

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
        } else if (request.shaderPipeline) {
            ProcessShaderPipelineBuildRequest(std::move(request));
        }
    }
}

bool LoadingSystem::ProcessTextureLoadRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessTextureLoadRequest");

    // 로딩 요청을 처리하려는 사이에 로딩이 취소되었거나 시리얼이 변경되었다면, 로딩을 하지 않는다.
    if (!request.texture->IsLoading() || request.serial != request.texture->GetLoadSerial()) {
        return false;
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
        return false;
    }

    if (BE1::HasFlag(request.textureFlags, Texture::Flag::LinearColorSpace)) {
        image->SetGammaSpace(BE1::Image::GammaSpace::Linear);
    }

    int textureDepth = useMipRange ? mipRange.originalDepth : image->GetDepth();

    RHI::TextureType textureType;
    if (textureDepth > 1) {
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
        completion.success = Texture::Prepare(textureType, std::move(image), request.textureFlags | Texture::Flag::LoadedFromFile, completion.textureData);
    }

    if (completion.textureData.rhiTexture) {
        RHI::renderer->SetName(completion.textureData.rhiTexture, request.filename);
    }

    // 완료 큐에 집어넣는다.
    PushCompletion(std::move(completion));
    return true;
}

bool LoadingSystem::ProcessTextureStreamingReplacementRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessTextureStreamingReplacementRequest");

    // 로딩 요청을 처리하려는 사이에 시리얼이 변경되었다면, 로딩을 하지 않는다.
    if (request.serial != request.texture->GetLoadSerial()) {
        return false;
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
        return false;
    }

    if (BE1::HasFlag(request.textureFlags, Texture::Flag::LinearColorSpace)) {
        image->SetGammaSpace(BE1::Image::GammaSpace::Linear);
    }

    int textureDepth = mipRange.originalDepth;

    RHI::TextureType textureType;
    if (textureDepth > 1) {
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
    return true;
}

bool LoadingSystem::ProcessMeshLoadRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessMeshLoadRequest");

    // 로딩 요청을 처리하려는 사이에 로딩이 취소되었거나 시리얼이 변경되었다면, 로딩을 하지 않는다.
    if (!request.mesh->IsLoading() || request.serial != request.mesh->GetLoadSerial()) {
        return false;
    }

    LoadCompletion completion;
    completion.mesh = std::move(request.mesh);
    completion.isReload = request.isReload;
    completion.serial = request.serial;
    completion.success = Mesh::Prepare(request.filename, completion.meshData);

    PushCompletion(std::move(completion));
    return true;
}

bool LoadingSystem::ProcessSkeletonLoadRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessSkeletonLoadRequest");

    // 로딩 요청을 처리하려는 사이에 로딩이 취소되었거나 시리얼이 변경되었다면, 로딩을 하지 않는다.
    if (!request.skeleton->IsLoading() || request.serial != request.skeleton->GetLoadSerial()) {
        return false;
    }

    LoadCompletion completion;
    completion.skeleton = std::move(request.skeleton);
    completion.isReload = request.isReload;
    completion.serial = request.serial;
    completion.success = Skeleton::Prepare(request.filename, completion.skeletonData);

    PushCompletion(std::move(completion));
    return true;
}

bool LoadingSystem::ProcessAnimLoadRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessAnimLoadRequest");

    // 로딩 요청을 처리하려는 사이에 로딩이 취소되었거나 시리얼이 변경되었다면, 로딩을 하지 않는다.
    if (!request.anim->IsLoading() || request.serial != request.anim->GetLoadSerial()) {
        return false;
    }

    LoadCompletion completion;
    completion.anim = std::move(request.anim);
    completion.isReload = request.isReload;
    completion.serial = request.serial;
    completion.success = Anim::Prepare(request.filename, completion.animData);

    PushCompletion(std::move(completion));
    return true;
}

bool LoadingSystem::ProcessShaderPipelineBuildRequest(LoadRequest &&request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessShaderPipelineBuildRequest");

    // 빌딩 요청을 처리하려는 사이에 빌딩이 취소되었거나 시리얼이 변경되었다면, 빌딩을 하지 않는다.
    if (!request.shaderPipeline->IsBuilding() || request.serial != request.shaderPipeline->GetBuildSerial()) {
        return false;
    }

    LoadCompletion completion;
    completion.shaderPipeline = std::move(request.shaderPipeline);
    completion.serial = request.serial;
    completion.pso = completion.shaderPipeline->CreatePSO(request.shaderCompileData);
    completion.success = completion.pso != nullptr;

    PushCompletion(std::move(completion));
    return true;
}

void LoadingSystem::QueueMaterialReload(SharedPtr<Material> material, Material::InternalData &newInternalData) {
    assert(BE1::Engine::IsInMainThread());

    if (!material) {
        return;
    }

    for (PendingMaterialReload &pendingReload : pendingMaterialReloads) {
        if (pendingReload.material.get() == material.get()) {
            pendingReload.internalData.Swap(newInternalData);
            return;
        }
    }

    PendingMaterialReload &pendingReload = pendingMaterialReloads.Alloc();
    pendingReload.material = std::move(material);
    pendingReload.internalData.Swap(newInternalData);
}

void LoadingSystem::QueueShaderTemplateReload(SharedPtr<ShaderTemplate> shaderTemplate, ShaderTemplate::InternalData &newInternalData) {
    assert(BE1::Engine::IsInMainThread());

    if (!shaderTemplate) {
        return;
    }

    for (PendingShaderTemplateReload &pendingReload : pendingShaderTemplateReloads) {
        if (pendingReload.shaderTemplate.get() == shaderTemplate.get()) {
            pendingReload.internalData.Swap(newInternalData);
            return;
        }
    }

    PendingShaderTemplateReload &pendingReload = pendingShaderTemplateReloads.Alloc();
    pendingReload.shaderTemplate = std::move(shaderTemplate);
    pendingReload.internalData.Swap(newInternalData);
    pendingReload.oldProperties = pendingReload.shaderTemplate->GetProperties();
}

void LoadingSystem::ProcessLoadCompletions() {
    assert(BE1::Engine::IsInMainThread());

    UpdateWorkerSettings();

    BE1::Array<LoadCompletion> localCompletions;
    {
        BE1::ScopedLock lock(completionMutex);
        localCompletions.Swap(completions);
    }

    for (LoadCompletion &completion : localCompletions) {
        // 리로딩/리빌딩 처리는 여기서 완료 처리를 하지 않고, ProcessPendingReloads() 로 미룬다.
        if (IsLiveResourceReplacement(completion)) {
            reloadCompletions.Append(std::move(completion));
            continue;
        }

        ProcessLoadCompletion(completion);
    }
}

void LoadingSystem::ProcessPendingReloads() {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessPendingReloads");

    assert(BE1::Engine::IsInMainThread());

    if (reloadCompletions.IsEmpty() && pendingMaterialReloads.IsEmpty() && pendingShaderTemplateReloads.IsEmpty()) {
        return;
    }

    BE1::Array<LoadCompletion> localCompletions;
    localCompletions.Swap(reloadCompletions);

    BE1::Array<PendingMaterialReload> localMaterialReloads;
    localMaterialReloads.Swap(pendingMaterialReloads);

    BE1::Array<PendingShaderTemplateReload> localShaderTemplateReloads;
    localShaderTemplateReloads.Swap(pendingShaderTemplateReloads);

    // 리로드 전에 폐기 대상인 completion 은 제거한다.
    for (int i = 0; i < localCompletions.Count();) {
        if (TryDiscardInvalidReloadCompletion(localCompletions[i])) {
            localCompletions.RemoveIndexFast(i);
        } else {
            i++;
        }
    }

    if (localCompletions.IsEmpty() && localMaterialReloads.IsEmpty() && localShaderTemplateReloads.IsEmpty()) {
        return;
    }

    // 렌더 프런트엔드 프레임 작성 중에는 기존 리소스를 교체하면 안 된다.
    assert(RenderContext::activeContext == nullptr);

    // 모든 렌더 스레드가 자원 사용을 마치기를 기다린다.
    renderSystem->WaitAllRenderCompleted();

    for (LoadCompletion &completion : localCompletions) {
        ProcessLoadCompletion(completion);
    }

    ProcessMaterialReloads(localMaterialReloads);
    ProcessShaderTemplateReloads(localShaderTemplateReloads);
}

void LoadingSystem::ProcessMaterialReloads(BE1::Array<PendingMaterialReload> &pendingReloads) {
    for (PendingMaterialReload &pendingReload : pendingReloads) {
        pendingReload.material->ApplyPrepared(pendingReload.internalData);
    }
}

void LoadingSystem::ProcessShaderTemplateReloads(BE1::Array<PendingShaderTemplateReload> &pendingReloads) {
    // 부모 ShaderTemplate 이 리로드되어도 이를 상속한 자식 ShaderTemplate 은 자동으로 리로드하지 않는다.
    for (PendingShaderTemplateReload &pendingReload : pendingReloads) {
        pendingReload.shaderTemplate->ApplyPrepared(pendingReload.internalData);

        // 외부 material shader template reload 대응용 알림이다.
        // Reload 중 Purge 로 pipeline cache 는 이미 비워졌으므로 material 은 필요할 때 GetGraphicsPipeline 를 통해 새 pipeline 을 다시 얻는다.
        ShaderTemplate::reloaded(pendingReload.shaderTemplate.get(), pendingReload.oldProperties);
    }
}

bool LoadingSystem::TryDiscardInvalidReloadCompletion(LoadCompletion &completion) {
    if (completion.texture) {
        if (completion.serial != completion.texture->GetLoadSerial()) {
            DiscardCompletion(completion);
            return true;
        }

        if (!completion.success) {
            DiscardCompletion(completion);
            completion.texture->SetState(completion.isReload ? Texture::State::Ready : Texture::State::Failed);
            return true;
        }

        return false;
    }

    if (completion.mesh) {
        if (completion.serial != completion.mesh->GetLoadSerial()) {
            DiscardCompletion(completion);
            return true;
        }

        if (!completion.success) {
            DiscardCompletion(completion);
            completion.mesh->SetState(completion.isReload ? Mesh::State::Ready : Mesh::State::Failed);
            return true;
        }

        return false;
    }

    if (completion.skeleton) {
        if (completion.serial != completion.skeleton->GetLoadSerial()) {
            DiscardCompletion(completion);
            return true;
        }

        if (!completion.success) {
            DiscardCompletion(completion);
            completion.skeleton->SetState(completion.isReload ? Skeleton::State::Ready : Skeleton::State::Failed);
            return true;
        }

        return false;
    }

    if (completion.anim) {
        if (completion.serial != completion.anim->GetLoadSerial()) {
            DiscardCompletion(completion);
            return true;
        }

        if (!completion.success) {
            DiscardCompletion(completion);
            completion.anim->SetState(completion.isReload ? Anim::State::Ready : Anim::State::Failed);
            return true;
        }

        return false;
    }

    if (completion.shaderPipeline) {
        if (completion.serial != completion.shaderPipeline->GetBuildSerial()) {
            DiscardCompletion(completion);
            return true;
        }

        if (!completion.success || !completion.pso) {
            DiscardCompletion(completion);
            completion.shaderPipeline->SetState(completion.shaderPipeline->GetPSO() ? ShaderPipeline::State::Ready : ShaderPipeline::State::Failed);
            return true;
        }

        return false;
    }

    return true;
}

void LoadingSystem::ProcessLoadCompletion(LoadCompletion &completion) {
    if (completion.texture) {
        ProcessTextureLoadCompletion(completion);
    } else if (completion.mesh) {
        ProcessMeshLoadCompletion(completion);
    } else if (completion.skeleton) {
        ProcessSkeletonLoadCompletion(completion);
    } else if (completion.anim) {
        ProcessAnimLoadCompletion(completion);
    } else if (completion.shaderPipeline) {
        ProcessShaderPipelineBuildCompletion(completion);
    }
}

bool LoadingSystem::ProcessTextureLoadCompletion(LoadCompletion &completion) {
    const SharedPtr<Texture> &texture = completion.texture;

    // 로딩 요청 이후 시리얼이 변경된 경우, 새로 만든 텍스쳐를 폐기한다.
    if (completion.serial != texture->GetLoadSerial()) {
        DiscardCompletion(completion);
        return false;
    }

#ifdef USE_MIPMAP_STREAMING
    // 교체 요청을 받았을 경우
    if (completion.isStreamingReplacement) {
        // 교체할 텍스쳐를 로딩하지 못했을 경우
        if (!completion.success || !completion.textureData.rhiTexture) {
            BE_ERRLOG("Texture streaming replacement failed: texture '%s', requested first mip %i\n", texture->GetHashName(), texture->GetStreaming()->requestedFirstMip);

            DiscardCompletion(completion);

            // 스트리밍 중단
            textureManager.UnregisterStreamingTexture(texture.get());
            return true;
        }

        // 교체할 RHI::Texture 포인터를 담아 놓는다.
        texture->GetStreaming()->pendingReplacementTexture = completion.textureData.rhiTexture;
        completion.textureData.rhiTexture = nullptr;
        return true;
    }
#endif

    // 로딩이 실패한 경우
    if (!completion.success) {
        DiscardCompletion(completion);
        texture->SetState(completion.isReload ? Texture::State::Ready : Texture::State::Failed);
        return false;
    }

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

    return true;
}

bool LoadingSystem::ProcessMeshLoadCompletion(LoadCompletion &completion) {
    const SharedPtr<Mesh> &mesh = completion.mesh;

    // 로딩 요청 이후 시리얼이 변경된 경우, 새로 만든 메시를 폐기한다.
    if (completion.serial != mesh->GetLoadSerial()) {
        DiscardCompletion(completion);
        return false;
    }

    // 로딩이 실패한 경우
    if (!completion.success) {
        DiscardCompletion(completion);
        mesh->SetState(completion.isReload ? Mesh::State::Ready : Mesh::State::Failed);
        return false;
    }

    // 준비된 메시로 세팅한다.
    mesh->ApplyPrepared(completion.meshData);

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    mesh->SetState(Mesh::State::Ready);
    return true;
}

bool LoadingSystem::ProcessSkeletonLoadCompletion(LoadCompletion &completion) {
    const SharedPtr<Skeleton> &skeleton = completion.skeleton;

    // 로딩 요청 이후 시리얼이 변경된 경우, 새로 만든 스켈레톤을 폐기한다.
    if (completion.serial != skeleton->GetLoadSerial()) {
        DiscardCompletion(completion);
        return false;
    }

    // 로딩이 실패한 경우
    if (!completion.success) {
        DiscardCompletion(completion);
        skeleton->SetState(completion.isReload ? Skeleton::State::Ready : Skeleton::State::Failed);
        return false;
    }

    // 준비된 스켈레톤으로 세팅한다.
    skeleton->ApplyPrepared(completion.skeletonData);

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    skeleton->SetState(Skeleton::State::Ready);
    return true;
}

bool LoadingSystem::ProcessAnimLoadCompletion(LoadCompletion &completion) {
    const SharedPtr<Anim> &anim = completion.anim;

    // 로딩 요청 이후 시리얼이 변경된 경우, 새로 만든 스켈레톤을 폐기한다.
    if (completion.serial != anim->GetLoadSerial()) {
        DiscardCompletion(completion);
        return false;
    }

    // 로딩이 실패한 경우
    if (!completion.success) {
        DiscardCompletion(completion);
        anim->SetState(completion.isReload ? Anim::State::Ready : Anim::State::Failed);
        return false;
    }

    // 준비된 Anim 으로 세팅한다.
    anim->ApplyPrepared(completion.animData);

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    anim->SetState(Anim::State::Ready);
    return true;
}

bool LoadingSystem::ProcessShaderPipelineBuildCompletion(LoadCompletion &completion) {
    const SharedPtr<ShaderPipeline> &shaderPipeline = completion.shaderPipeline;

    // 빌드 요청 이후 시리얼이 변경된 경우, 새로 만든 PipelineState 를 폐기한다.
    if (completion.serial != shaderPipeline->GetBuildSerial()) {
        DiscardCompletion(completion);
        return false;
    }

    // 빌드가 실패한 경우
    if (!completion.success || !completion.pso) {
        DiscardCompletion(completion);
        shaderPipeline->SetState(shaderPipeline->GetPSO() ? ShaderPipeline::State::Ready : ShaderPipeline::State::Failed);
        return false;
    }

    // 준비된 PSO 로 교체한다.
    shaderPipeline->ReplacePSO(completion.pso);
    completion.pso = nullptr;

    // 빌드가 성공했으므로 Ready 상태로 바꿔준다.
    shaderPipeline->SetState(ShaderPipeline::State::Ready);
    return true;
}

bool LoadingSystem::IsLiveResourceReplacement(const LoadCompletion &completion) {
    if (completion.isStreamingReplacement) {
        return false;
    }

    if (completion.isReload) {
        return true;
    }

    return completion.shaderPipeline && completion.shaderPipeline->GetState() == ShaderPipeline::State::Rebuilding;
}

bool LoadingSystem::IsHigherPriority(const LoadRequest &lhs, const LoadRequest &rhs) {
    if ((int)lhs.priority != (int)rhs.priority) {
        return (int)lhs.priority > (int)rhs.priority;
    }
    return lhs.sequence < rhs.sequence;
}

void LoadingSystem::PushCompletion(LoadCompletion &&completion) {
    BE1::ScopedLock lock(completionMutex);
    completions.Append(std::move(completion));
}

void LoadingSystem::DiscardCompletion(LoadCompletion &completion) {
    if (completion.texture) {
        completion.textureData.Purge();
    } else if (completion.mesh) {
        completion.meshData.Purge();
    } else if (completion.skeleton) {
        completion.skeletonData.Purge();
    } else if (completion.anim) {
        completion.animData.Purge();
    } else if (completion.shaderPipeline) {
        if (completion.pso) {
            RHI::renderer->DestroyPSO(completion.pso, true);
            completion.pso = nullptr;
        }
    }
}

unsigned int LoadingSystem::ThreadProc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName("LoadingSystem::ThreadProc");

    BE1::SIMD::SetDenormalFlushMode(true);

    LoadingSystem *loadingSystem = reinterpret_cast<LoadingSystem *>(param);
    loadingSystem->Run();

    return 0;
}
