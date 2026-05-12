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

    stopping = false;

    requests.Clear();

    {
        BE1::ScopedLock lock(completionMutex);
        DiscardCompletions(completions);
        completions.Clear();
    }
}

bool LoadingSystem::EnqueueTextureLoad(Texture *texture, const char *filename, Texture::Flag flags, uint32_t serial) {
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

bool LoadingSystem::EnqueueMeshLoad(Mesh *mesh, const char *filename, uint32_t serial) {
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

// 로딩 요청 처리가 완료된 것들은 최종적으로 메인 스레드에서 처리한다.
void LoadingSystem::ProcessCompletions() {
    assert(BE1::Engine::IsInMainThread());

    BE1::Array<LoadCompletion> localCompletions;
    {
        BE1::ScopedLock lock(completionMutex);
        localCompletions.Swap(completions);
    }

    for (LoadCompletion &completion : localCompletions) {
        if (completion.texture) {
            if (!ProcessTextureCompletion(completion)) {
                continue;
            }
        } else if (completion.mesh) {
            if (!ProcessMeshCompletion(completion)) {
                continue;
            }
        }
    }
}

bool LoadingSystem::ProcessTextureCompletion(LoadCompletion &completion) {
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

    // 로딩이 실패한 경우
    if (!completion.success) {
        texture->SetState(Texture::State::Failed);
        return false;
    }

    // 로딩이 성공한 경우 Ready 상태로 바꿔준다.
    texture->ApplyPrepared(completion.texturePrepared);
    texture->SetState(Texture::State::Ready);
    completion.texturePrepared.texture = nullptr;
    
    return true;
}

bool LoadingSystem::ProcessMeshCompletion(LoadCompletion &completion) {
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

void LoadingSystem::DiscardCompletions(BE1::Array<LoadCompletion> &completions) {
    for (LoadCompletion &completion : completions) {
        DiscardCompletion(completion);
    }
}

void LoadingSystem::Run() {
    while (1) {
        LoadRequest request;
        if (!PopRequest(request)) {
            break;
        }

        // 로딩 요청을 처리한다.
        if (request.texture) {
            LoadCompletion completion;
            completion.texture = request.texture;
            completion.serial = request.serial;

            BE_LOG("Loading texture '%s'...\n", request.filename.c_str());

            std::shared_ptr<BE1::Image> image = std::make_shared<BE1::Image>();
            image->Load(request.filename.c_str());

            if (image->IsEmpty()) {
                BE_WARNLOG("LoadingSystem::Run: couldn't load texture \"%s\"\n", request.filename.c_str());
                PushCompletion(std::move(completion));
                continue;
            }

            RHI::TextureType textureType;
            if (image->GetDepth() > 1) {
                textureType = RHI::TextureType::Texture3D;
            } else if (image->IsCubeMap()) {
                textureType = RHI::TextureType::TextureCube;
            } else {
                textureType = RHI::TextureType::Texture2D;
            }

            completion.success = Texture::Prepare(textureType, image, request.textureFlags | Texture::Flag::LoadedFromFile, completion.texturePrepared);

            // 완료 큐에 집어넣는다.
            PushCompletion(std::move(completion));
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
