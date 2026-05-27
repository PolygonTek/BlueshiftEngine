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

LoadingSystem      loadingSystem;

void LoadingSystem::Init() {
    requestMutex = BE1::PlatformMutex::Create();
    completionMutex = BE1::PlatformMutex::Create();
    requestCondition = BE1::PlatformCondition::Create();

    Start();
}

void LoadingSystem::Shutdown() {
    Stop();

    BE1::PlatformCondition::Destroy(requestCondition);
    BE1::PlatformMutex::Destroy(completionMutex);
    BE1::PlatformMutex::Destroy(requestMutex);
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
        BE1::PlatformCondition::Broadcast(requestCondition);
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
}

bool LoadingSystem::EnqueueTextureLoad(std::shared_ptr<Texture> texture, const char *filename, Texture::Flag flags, bool isReload, uint32_t serial) {
    if (!texture || !filename || !filename[0]) {
        return false;
    }

    {
        BE1::ScopedLock lock(requestMutex);

        if (stopping) {
            return false;
        }

        LoadRequest request;
        request.texture = texture;
        request.filename = filename;
        request.textureFlags = flags;
        request.isReload = isReload;
        request.serial = serial;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        BE1::PlatformCondition::Signal(requestCondition);
    }

    return true;
}

bool LoadingSystem::EnqueueTextureStreamingReplacement(std::shared_ptr<Texture> texture, const char *filename, int firstMip) {
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
        request.texture = texture;
        request.filename = filename;
        request.textureFlags = texture->GetFlags();
        request.isStreamingReplacement = true;
        request.originalMipLevels = texture->GetStreaming()->originalMipLevels;
        request.firstMip = firstMip;
        request.serial = texture->GetLoadSerial();
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        BE1::PlatformCondition::Signal(requestCondition);
    }

    return true;
}

bool LoadingSystem::EnqueueMeshLoad(std::shared_ptr<Mesh> mesh, const char *filename, bool isReload, uint32_t serial) {
    if (!mesh || !filename || !filename[0]) {
        return false;
    }

    {
        BE1::ScopedLock lock(requestMutex);

        if (stopping) {
            return false;
        }

        LoadRequest request;
        request.mesh = mesh;
        request.filename = filename;
        request.isReload = isReload;
        request.serial = serial;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        BE1::PlatformCondition::Signal(requestCondition);
    }

    return true;
}

bool LoadingSystem::EnqueueSkeletonLoad(std::shared_ptr<Skeleton> skeleton, const char *filename, bool isReload, uint32_t serial) {
    if (!skeleton || !filename || !filename[0]) {
        return false;
    }

    {
        BE1::ScopedLock lock(requestMutex);

        if (stopping) {
            return false;
        }

        LoadRequest request;
        request.skeleton = skeleton;
        request.filename = filename;
        request.isReload = isReload;
        request.serial = serial;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        BE1::PlatformCondition::Signal(requestCondition);
    }

    return true;
}

bool LoadingSystem::EnqueueAnimLoad(std::shared_ptr<Anim> anim, const char *filename, bool isReload, uint32_t serial) {
    if (!anim || !filename || !filename[0]) {
        return false;
    }

    {
        BE1::ScopedLock lock(requestMutex);

        if (stopping) {
            return false;
        }

        LoadRequest request;
        request.anim = anim;
        request.filename = filename;
        request.isReload = isReload;
        request.serial = serial;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        BE1::PlatformCondition::Signal(requestCondition);
    }

    return true;
}

void LoadingSystem::Run() {
    while (1) {
        PROFILER_CPU_SCOPED_EVENT("LoadingSystem::Run");

        BE1::Array<LoadRequest> localRequests;
        {
            BE1::ScopedLock lock(requestMutex);

            BE1::PlatformCondition::Wait(requestCondition, requestMutex, [this] {
                return requests.Count() > 0 || stopping;
            });

            if (stopping) {
                break;
            }

            localRequests.Swap(requests);
        }

        for (LoadRequest &request : localRequests) {
            if (request.texture) {
                if (request.isStreamingReplacement) {
                    if (!ProcessTextureStreamingReplacementRequest(request)) {
                        continue;
                    }
                } else {
                    if (!ProcessTextureLoadRequest(request)) {
                        continue;
                    }
                }
            } else if (request.mesh) {
                if (!ProcessMeshLoadRequest(request)) {
                    continue;
                }
            } else if (request.skeleton) {
                if (!ProcessSkeletonLoadRequest(request)) {
                    continue;
                }
            } else if (request.anim) {
                if (!ProcessAnimLoadRequest(request)) {
                    continue;
                }
            }
        }
    }
}

bool LoadingSystem::ProcessTextureLoadRequest(const LoadRequest &request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessTextureLoadRequest");

    LoadCompletion completion;
    completion.texture = request.texture;
    completion.isReload = request.isReload;
    completion.serial = request.serial;

    std::shared_ptr<BE1::Image> image = std::make_shared<BE1::Image>();
    BE1::Image::MipRange mipRange;
    bool useMipRange = false;

    BE_LOG("Loading texture '%s'...\n", request.filename.c_str());

    // Texture::Flag::Streaming 이 켜져있고 DDS 파일이면 스트리밍 로딩을 시도한다.
    if (BE1::HasFlag(request.textureFlags, Texture::Flag::Streaming) && request.filename.CheckExtension(".dds")) {
        // 이미지 파일에서 MinResidentStreamingTextureMaxDimension 이하의 크기까지의 mip 레벨만 initial loading
        if (image->LoadDDSMipRangeForMaxDimension(request.filename.c_str(), TextureManager::MinResidentStreamingTextureMaxDimension, &mipRange)) {
            useMipRange = mipRange.mipCount < mipRange.originalMipLevels;
        }
    } else {
        // 이미지 파일 full loading
        image->Load(request.filename.c_str());
    }

    if (image->IsEmpty()) {
        BE_WARNLOG("LoadingSystem::ProcessTextureLoadRequest: couldn't load texture \"%s\"\n", request.filename.c_str());
        PushCompletion(std::move(completion));
        return false;
    }

    int textureDepth = useMipRange ? mipRange.originalDepth : image->GetDepth();

    RHI::TextureType textureType;
    if (textureDepth > 1) {
        textureType = RHI::TextureType::Texture3D;
    } else if (image->IsCubeMap()) {
        textureType = RHI::TextureType::TextureCube;
    } else {
        textureType = RHI::TextureType::Texture2D;
    }

    if (useMipRange) {
        completion.mipRange = mipRange;
        completion.success = Texture::PrepareDirect(textureType, image, request.textureFlags | Texture::Flag::LoadedFromFile, completion.textureData);
    } else {
        completion.success = Texture::Prepare(textureType, image, request.textureFlags | Texture::Flag::LoadedFromFile, completion.textureData);
    }

    if (completion.textureData.rhiTexture) {
        RHI::renderer->SetName(completion.textureData.rhiTexture, request.filename);
    }

    // 완료 큐에 집어넣는다.
    PushCompletion(std::move(completion));
    return true;
}

bool LoadingSystem::ProcessTextureStreamingReplacementRequest(const LoadRequest &request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessTextureStreamingReplacementRequest");

    LoadCompletion completion;
    completion.texture = request.texture;
    completion.serial = request.serial;

    // 기존 텍스쳐의 교체 요청을 받았다고 표시
    completion.isStreamingReplacement = true;

    uint32_t mipCount = request.originalMipLevels - request.firstMip;

    std::shared_ptr<BE1::Image> image = std::make_shared<BE1::Image>();
    BE1::Image::MipRange mipRange;

    BE_LOG("Loading texture '%s' [%i, %i]...\n", request.filename.c_str(), request.firstMip, request.firstMip + mipCount - 1);

    image->LoadDDSMipRange(request.filename.c_str(), request.firstMip, mipCount, &mipRange);
    
    if (image->IsEmpty()) {
        BE_WARNLOG("LoadingSystem::ProcessTextureStreamingReplacementRequest: couldn't load texture \"%s\"\n", request.filename.c_str());
        PushCompletion(std::move(completion));
        return false;
    }

    int textureDepth = mipRange.originalDepth;

    RHI::TextureType textureType;
    if (textureDepth > 1) {
        textureType = RHI::TextureType::Texture3D;
    } else if (image->IsCubeMap()) {
        textureType = RHI::TextureType::TextureCube;
    } else {
        textureType = RHI::TextureType::Texture2D;
    }

    completion.mipRange = mipRange;
    completion.success = Texture::PrepareDirect(textureType, image, request.textureFlags | Texture::Flag::LoadedFromFile, completion.textureData);

    // 완료 큐에 집어넣는다.
    PushCompletion(std::move(completion));
    return true;
}

bool LoadingSystem::ProcessMeshLoadRequest(const LoadRequest &request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessMeshLoadRequest");

    LoadCompletion completion;
    completion.mesh = request.mesh;
    completion.isReload = request.isReload;
    completion.serial = request.serial;
    completion.success = Mesh::Prepare(request.filename, completion.meshData);

    // 완료 큐에 집어넣는다.
    PushCompletion(std::move(completion));
    return true;
}

bool LoadingSystem::ProcessSkeletonLoadRequest(const LoadRequest &request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessSkeletonLoadRequest");

    LoadCompletion completion;
    completion.skeleton = request.skeleton;
    completion.isReload = request.isReload;
    completion.serial = request.serial;
    completion.success = Skeleton::Prepare(request.filename, completion.skeletonData);

    // 완료 큐에 집어넣는다.
    PushCompletion(std::move(completion));
    return true;
}

bool LoadingSystem::ProcessAnimLoadRequest(const LoadRequest &request) {
    PROFILER_CPU_SCOPED_EVENT("LoadingSystem::ProcessAnimLoadRequest");

    LoadCompletion completion;
    completion.anim = request.anim;
    completion.isReload = request.isReload;
    completion.serial = request.serial;
    completion.success = Anim::Prepare(request.filename, completion.animData);

    // 완료 큐에 집어넣는다.
    PushCompletion(std::move(completion));
    return true;
}

// 로딩 요청 처리가 완료된 것들은 최종적으로 메인 스레드에서 처리한다.
void LoadingSystem::ProcessLoadCompletions() {
    assert(BE1::Engine::IsInMainThread());

    BE1::Array<LoadCompletion> localCompletions;
    {
        BE1::ScopedLock lock(completionMutex);
        localCompletions.Swap(completions);
    }

    for (LoadCompletion &completion : localCompletions) {
        if (completion.texture) {
            if (!ProcessTextureLoadCompletion(completion)) {
                continue;
            }
        } else if (completion.mesh) {
            if (!ProcessMeshLoadCompletion(completion)) {
                continue;
            }
        } else if (completion.skeleton) {
            if (!ProcessSkeletonLoadCompletion(completion)) {
                continue;
            }
        } else if (completion.anim) {
            if (!ProcessAnimLoadCompletion(completion)) {
                continue;
            }
        }
    }
}

bool LoadingSystem::ProcessTextureLoadCompletion(LoadCompletion &completion) {
    std::shared_ptr<Texture> texture = completion.texture;

    // 로딩 요청 이후 시리얼이 변경된 경우
    if (completion.serial != texture->GetLoadSerial()) {
        // 새로 만든 텍스쳐를 폐기한다.
        DiscardCompletion(completion);
        return false;
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

        // 교체할 RHI::Texture 포인터를 담아 놓는다.
        texture->GetStreaming()->pendingReplacementTexture = completion.textureData.rhiTexture;
        completion.textureData.rhiTexture = nullptr;
        return true;
    }
#endif

    // 로딩이 실패한 경우
    if (!completion.success) {
        texture->SetState(Texture::State::Failed);

        if (completion.isReload) {
            texture->Purge();
        }
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
    std::shared_ptr<Mesh> mesh = completion.mesh;

    // 로딩 요청 이후 시리얼이 변경된 경우
    if (completion.serial != mesh->GetLoadSerial()) {
        // 새로 만든 메시를 폐기한다.
        DiscardCompletion(completion);
        return false;
    }

    // 로딩이 실패한 경우
    if (!completion.success) {
        mesh->SetState(Mesh::State::Failed);

        if (completion.isReload) {
            mesh->Purge();
        }
        return false;
    }

    // 준비된 메시로 세팅한다.
    mesh->ApplyPrepared(completion.meshData);

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    mesh->SetState(Mesh::State::Ready);
    return true;
}

bool LoadingSystem::ProcessSkeletonLoadCompletion(LoadCompletion &completion) {
    std::shared_ptr<Skeleton> skeleton = completion.skeleton;

    // 로딩 요청 이후 시리얼이 변경된 경우
    if (completion.serial != skeleton->GetLoadSerial()) {
        // 새로 만든 스켈레톤을 폐기한다.
        DiscardCompletion(completion);
        return false;
    }

    // 로딩이 실패한 경우
    if (!completion.success) {
        skeleton->SetState(Skeleton::State::Failed);

        if (completion.isReload) {
            skeleton->Purge();
        }
        return false;
    }

    // 준비된 스켈레톤으로 세팅한다.
    skeleton->ApplyPrepared(completion.skeletonData);

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    skeleton->SetState(Skeleton::State::Ready);
    return true;
}

bool LoadingSystem::ProcessAnimLoadCompletion(LoadCompletion &completion) {
    std::shared_ptr<Anim> anim = completion.anim;

    // 로딩 요청 이후 시리얼이 변경된 경우
    if (completion.serial != anim->GetLoadSerial()) {
        // 새로 만든 스켈레톤을 폐기한다.
        DiscardCompletion(completion);
        return false;
    }

    // 로딩이 실패한 경우
    if (!completion.success) {
        if (completion.isReload) {
            anim->SetState(Anim::State::Ready);
        } else {
            anim->SetState(Anim::State::Failed);
        }
        return false;
    }

    // 준비된 Anim 으로 세팅한다.
    anim->ApplyPrepared(completion.animData);

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    anim->SetState(Anim::State::Ready);
    return true;
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
    }
}

unsigned int LoadingSystem::ThreadProc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName("LoadingSystem::ThreadProc");

    BE1::SIMD::SetDenormalFlushMode(true);

    LoadingSystem *loadingSystem = reinterpret_cast<LoadingSystem *>(param);
    loadingSystem->Run();

    return 0;
}
