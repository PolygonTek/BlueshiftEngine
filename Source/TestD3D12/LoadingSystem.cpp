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

bool LoadingSystem::EnqueueTextureLoad(uint32_t serial, Texture *texture, const char *filename, Texture::Flag flags) {
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
        request.serial = serial;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        BE1::PlatformCondition::Signal(requestCondition);
    }

    return true;
}

bool LoadingSystem::EnqueueTextureMipRangeLoad(Texture *texture, const char *filename, int firstMip, int mipCount) {
    if (!texture || !filename || !filename[0]) {
        return false;
    }
    if (!texture->GetRHITexture()) {
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
        request.partialMipLoading = true;
        request.firstMip = firstMip;
        request.mipCount = mipCount;
        request.serial = texture->GetLoadSerial();
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        BE1::PlatformCondition::Signal(requestCondition);
    }

    return true;
}

bool LoadingSystem::EnqueueMeshLoad(uint32_t serial, Mesh *mesh, const char *filename) {
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
        request.serial = serial;
        requests.Append(std::move(request));

        // 로딩 요청 큐에 집어넣는다.
        BE1::PlatformCondition::Signal(requestCondition);
    }

    return true;
}

void LoadingSystem::Run() {
    while (1) {
        LoadRequest request;
        if (!PopRequest(request)) {
            break;
        }

        // 로딩 요청을 처리한다.
        if (request.texture) {
            if (!request.partialMipLoading) {
                if (!ProcessTextureLoadRequest(request)) {
                    continue;
                }
            } else {
                if (!ProcessTextureMipRangeLoadRequest(request)) {
                    continue;
                }
            }
        } else if (request.mesh) {
            if (!ProcessMeshLoadRequest(request)) {
                continue;
            }
        }
    }
}

bool LoadingSystem::PopRequest(LoadRequest &outRequest) {
    BE1::ScopedLock lock(requestMutex);

    BE1::PlatformCondition::Wait(requestCondition, requestMutex, [this] {
        return requests.Count() > 0 || stopping;
        });

    if (stopping) {
        return false;
    }

    outRequest = std::move(requests[0]);
    requests.RemoveIndex(0); //

    return true;
}

bool LoadingSystem::ProcessTextureLoadRequest(const LoadRequest &request) {
    LoadCompletion completion;
    completion.texture = request.texture;
    completion.serial = request.serial;

    BE_LOG("Loading texture '%s'...\n", request.filename.c_str());

    std::shared_ptr<BE1::Image> image = std::make_shared<BE1::Image>();
    BE1::Image::MipRange mipRange;

    // Texture::Flag::Streaming 이 켜져있고 DDS 파일이면 스트리밍 로딩을 시도한다.
    bool useInitialMipRange = BE1::HasFlag(request.textureFlags, Texture::Flag::Streaming) && request.filename.CheckExtension(".dds");
    if (!useInitialMipRange) {
        // 이미지 파일 full loading
        image->Load(request.filename.c_str());
    } else {
        // 이미지 파일에서 256x256 이하의 크기까지의 mip 레벨만 initial loading
        if (image->LoadDDSMipRangeForMaxDimension(request.filename.c_str(), 256, &mipRange)) {
            // 이미지 자체가 256x256 이하라면 스트리밍을 그만둔다.
            useInitialMipRange = mipRange.mipCount == mipRange.originalMipLevels ? false : true;
        }
    }

    if (image->IsEmpty()) {
        BE_WARNLOG("LoadingSystem::ProcessTextureLoadRequest: couldn't load texture \"%s\"\n", request.filename.c_str());
        PushCompletion(std::move(completion));
        return false;
    }

    int textureDepth = useInitialMipRange ? mipRange.originalDepth : image->GetDepth();

    RHI::TextureType textureType;
    if (textureDepth > 1) {
        textureType = RHI::TextureType::Texture3D;
    } else if (image->IsCubeMap()) {
        textureType = RHI::TextureType::TextureCube;
    } else {
        textureType = RHI::TextureType::Texture2D;
    }

    if (!useInitialMipRange) {
        completion.success = Texture::PrepareNew(textureType, image, request.textureFlags | Texture::Flag::LoadedFromFile, completion.texturePrepared);
    } else {
        completion.image = image;
        completion.mipRange = mipRange;
        completion.success = Texture::PrepareMipRangeNew(textureType, image, mipRange, request.textureFlags | Texture::Flag::LoadedFromFile, completion.texturePrepared);
    }

    // 완료 큐에 집어넣는다.
    PushCompletion(std::move(completion));
    return true;
}

bool LoadingSystem::ProcessTextureMipRangeLoadRequest(const LoadRequest &request) {
    LoadCompletion completion;
    completion.texture = request.texture;
    completion.serial = request.serial;
    completion.streaming = true;

    BE_LOG("Loading texture '%s' mip [%i, %i]...\n", request.filename.c_str(), request.firstMip, request.firstMip + request.mipCount);

    BE1::Image::MipRange mipRange;
    std::shared_ptr<BE1::Image> image = std::make_shared<BE1::Image>();
    image->LoadDDSMipRange(request.filename.c_str(), request.firstMip, request.mipCount, &mipRange);

    if (image->IsEmpty()) {
        BE_WARNLOG("LoadingSystem::ProcessTextureMipRangeLoadRequest: couldn't load texture \"%s\"\n", request.filename.c_str());
        PushCompletion(std::move(completion));
        return false;
    }

    completion.success = true;
    completion.image = image;
    completion.mipRange = mipRange;

    // 완료 큐에 집어넣는다.
    PushCompletion(std::move(completion));
    return true;
}

bool LoadingSystem::ProcessMeshLoadRequest(const LoadRequest &request) {
    return false;
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
        }
    }
}

bool LoadingSystem::ProcessTextureLoadCompletion(LoadCompletion &completion) {
    Texture *texture = completion.texture;

    // 파괴 대기 중인 경우
    if (texture->GetState() == Texture::State::DestroyPending) {
        DiscardCompletion(completion);
        textureManager.DestroyTexture(texture);
        return false;
    }

    // 로딩 요청 이후 시리얼이 변경된 경우 : 다시 로딩 요청했거나 요청 처리 전에 Purge or DestroyTexture 가 호출된 상황
    if (completion.serial != texture->GetLoadSerial()) {
        DiscardCompletion(completion);
        return false;
    }

    // 스트리밍인 경우
    if (completion.streaming) {
        // 로딩이 실패한 경우
        if (!completion.success) {
            textureManager.UnregisterStreamingTexture(texture);
            return true;
        }

        // 부분 데이터 업로드
        if (!RHI::renderer->UploadTextureMipRange(texture->GetRHITexture(), completion.image, completion.mipRange.firstMip)) {
            textureManager.UnregisterStreamingTexture(texture);
            return true;
        }

        // 로딩한 mipRange 로 residentFirstMip 과 residentMipCount 를 업데이트 한다.
        uint32_t residentFirstMip;
        uint32_t residentMipCount;
        if (!textureManager.CompleteStreamingTextureRequest(texture, completion.mipRange, &residentFirstMip, &residentMipCount)) {
            textureManager.UnregisterStreamingTexture(texture);
            return true;
        }

        // resident SRV 를 업데이트 한다.
        if (!RHI::renderer->UpdateTextureViewMipRange(texture->GetRHITexture(), RHI::ViewID::PrimaryID, residentFirstMip, residentMipCount)) {
            textureManager.UnregisterStreamingTexture(texture);
            return true;
        }
        return true;
    }

    // 로딩이 실패한 경우
    if (!completion.success) {
        texture->SetState(Texture::State::Failed);
        return false;
    }

    texture->ApplyPrepared(completion.texturePrepared);

    // 스트리밍 초기 데이터인 경우 업로드
    if (completion.mipRange.mipCount > 0) {
        if (!RHI::renderer->UploadTextureMipRange(texture->GetRHITexture(), completion.image, completion.mipRange.firstMip)) {
            texture->SetState(Texture::State::Failed);
            return false;
        }

        // mip range 로 SRV 를 업데이트 한다.
        if (!RHI::renderer->UpdateTextureViewMipRange(texture->GetRHITexture(), RHI::ViewID::PrimaryID, completion.mipRange.firstMip, completion.mipRange.mipCount)) {
            texture->SetState(Texture::State::Failed);
            return false;
        }

        // 텍스쳐 스트리밍 시스템에 등록한다.
        textureManager.RegisterStreamingTexture(texture, completion.mipRange);
    }

    // 로딩이 성공했으므로 Ready 상태로 바꿔준다.
    texture->SetState(Texture::State::Ready);

    return true;
}

bool LoadingSystem::ProcessMeshLoadCompletion(LoadCompletion &completion) {
    /*Mesh *mesh = completion.mesh;

    // 파괴 대기 중인 경우
    if (mesh->GetState() == Mesh::State::DestroyPending) {
        DiscardCompletion(completion);
        meshManager.DestroyMesh(mesh);
        return false;
    }

    // 로딩 요청 이후 시리얼이 변경된 경우 : 다시 로딩 요청했거나 요청 처리 전에 Purge or DestroyTexture 가 호출된 상황
    if (completion.serial != mesh->GetLoadSerial()) {
        DiscardCompletion(completion);
        return false;
    }

    // 로딩이 실패한 경우
    if (!completion.success) {
        mesh->SetState(Mesh::State::Failed);
        return false;
    }

    // 로딩이 성공한 경우 Ready 상태로 바꿔준다.
    mesh->ApplyPrepared(completion.meshPrepared);
    mesh->SetState(Mesh::State::Ready);
    completion.meshPrepared.mesh = nullptr;

    return true;*/
    return false;
}

void LoadingSystem::PushCompletion(LoadCompletion &&completion) {
    BE1::ScopedLock lock(completionMutex);
    completions.Append(std::move(completion));
}

void LoadingSystem::DiscardCompletion(LoadCompletion &completion) {
    if (completion.texturePrepared.texture) {
        RHI::renderer->DestroyTexture(completion.texturePrepared.texture);
        completion.texturePrepared.texture = nullptr;
    }/*else if (completion.meshPrepared.mesh) {
        RHI::renderer->DestroyMesh(completion.meshPrepared.mesh);
        completion.meshPrepared.mesh = nullptr;
    }*/
}

unsigned int LoadingSystem::ThreadProc(void *param) {
    BE1::PlatformThread::SetCurrentThreadName("LoadingSystem::ThreadProc");

    BE1::SIMD::SetDenormalFlushMode(true);

    LoadingSystem *loadingSystem = reinterpret_cast<LoadingSystem *>(param);
    loadingSystem->Run();

    return 0;
}
