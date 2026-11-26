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

#include "Render/Texture/Texture.h"
#include "Render/Mesh/Mesh.h"
#include "Render/Animation/Skeleton.h"
#include "Render/Animation/Anim.h"

/*
---------------------------------------------------------------------------------------------------

    Loading System

---------------------------------------------------------------------------------------------------
*/

class LoadingSystem {
    friend class RenderSystem;

public:
    enum class LoadPriority : int8_t {
        Background                  = 0,
        Normal                      = 10,
        High                        = 20,
        Critical                    = 30
    };

    LoadingSystem() = default;
    LoadingSystem(const LoadingSystem &) = delete;
    LoadingSystem &operator=(const LoadingSystem &) = delete;

    void                            Init();
    void                            Shutdown();

    static bool                     IsAsyncAllowed();

    bool                            EnqueueTextureLoad(SharedPtr<Texture> texture, const char *filename, Texture::Flag flags, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueTextureStreamingReplacement(SharedPtr<Texture> texture, const char *filename, int32_t firstMip, LoadPriority priority = LoadPriority::Background);
    bool                            EnqueueMeshLoad(SharedPtr<Mesh> mesh, const char *filename, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueSkeletonLoad(SharedPtr<Skeleton> skeleton, const char *filename, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueAnimLoad(SharedPtr<Anim> anim, const char *filename, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);

    bool                            BoostTextureLoad(const Texture *texture, uint32_t serial, LoadPriority priority);
    bool                            BoostMeshLoad(const Mesh *mesh, uint32_t serial, LoadPriority priority);
    bool                            BoostSkeletonLoad(const Skeleton *skeleton, uint32_t serial, LoadPriority priority);
    bool                            BoostAnimLoad(const Anim *anim, uint32_t serial, LoadPriority priority);

                                    /// 완료된 로딩 요청의 결과를 메인 스레드에서 적용한다.
    void                            ProcessLoadCompletions();

                                    /// 동기 리로드 대기를 위해 렌더 완료 대기와 결과 적용을 함께 수행한다.
    void                            ProcessPendingReloads();

private:
    struct LoadRequest {
        SharedPtr<Texture>          texture;
        SharedPtr<Mesh>             mesh;
        SharedPtr<Skeleton>         skeleton;
        SharedPtr<Anim>             anim;
        BE1::Str                    filename;
        Texture::Flag               textureFlags = Texture::Flag::None;
        uint32_t                    streamingMinResidentMaxDimension = 1;
        int32_t                     originalMipLevels = 0;
        int32_t                     firstMip = -1;
        uint32_t                    serial = 0;
        uint64_t                    sequence = 0;                       // 큐에 들어온 순서
        LoadPriority                priority = LoadPriority::Normal;
        bool                        isReload = false;
        bool                        isStreamingReplacement = false;
    };

    struct LoadCompletion {
        SharedPtr<Texture>          texture;
        SharedPtr<Mesh>             mesh;
        SharedPtr<Skeleton>         skeleton;
        SharedPtr<Anim>             anim;
        Texture::InternalData       textureData;
        Mesh::InternalData          meshData;
        Skeleton::InternalData      skeletonData;
        Anim::InternalData          animData;
        BE1::Image::MipRange        mipRange;
        uint32_t                    serial = 0;
        bool                        isReload = false;
        bool                        isStreamingReplacement = false;
        bool                        success = false;
    };

    bool                            PopNextRequest(LoadRequest &outRequest);
    void                            Run();
    void                            ProcessTextureLoadRequest(LoadRequest &&request);
    void                            ProcessTextureStreamingReplacementRequest(LoadRequest &&request);
    void                            ProcessMeshLoadRequest(LoadRequest &&request);
    void                            ProcessSkeletonLoadRequest(LoadRequest &&request);
    void                            ProcessAnimLoadRequest(LoadRequest &&request);
    void                            PushCompletion(LoadCompletion &&completion);

    bool                            CollectPendingReloads(BE1::Array<LoadCompletion> &batch);
    void                            ApplyPendingReloads(BE1::Array<LoadCompletion> &batch);

    bool                            TryDiscardInvalidCompletion(LoadCompletion &completion);
    void                            ProcessLoadCompletion(LoadCompletion &completion);
    void                            ProcessTextureLoadCompletion(LoadCompletion &completion);
    void                            ProcessMeshLoadCompletion(LoadCompletion &completion);
    void                            ProcessSkeletonLoadCompletion(LoadCompletion &completion);
    void                            ProcessAnimLoadCompletion(LoadCompletion &completion);

    template <typename PendingLoad>
    static void                     CancelLoad(const PendingLoad &load);

    static bool                     IsHigherPriority(const LoadRequest &lhs, const LoadRequest &rhs);
    static unsigned int             ThreadProc(void *param);

    BE1::PlatformThread *           thread = nullptr;
    BE1::Mutex                      requestMutex;
    BE1::Mutex                      completionMutex;
    BE1::Condition                  requestCondition;

    BE1::Array<LoadRequest>         requests;
    BE1::Array<LoadCompletion>      completions;
    BE1::Array<LoadCompletion>      processingCompletions;
    BE1::Array<LoadCompletion>      reloadCompletions;
    uint64_t                        nextRequestSequence = 0;
    std::atomic<int>                workerDelayMsec { 0 };

    bool                            stopping = false;
};

BE_INLINE bool LoadingSystem::IsHigherPriority(const LoadRequest &lhs, const LoadRequest &rhs) {
    if (lhs.priority != rhs.priority) {
        return lhs.priority > rhs.priority;
    }
    return lhs.sequence < rhs.sequence;
}

extern LoadingSystem                loadingSystem;
