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
#include "Render/Material/Material.h"
#include "Render/ShaderPipeline/ShaderPipeline.h"

class ShaderPipeline;

/*
---------------------------------------------------------------------------------------------------

    Loading System

---------------------------------------------------------------------------------------------------
*/

class LoadingSystem {
public:
    enum class LoadPriority : int8_t {
        Background                  = 0,
        Normal                      = 10,
        High                        = 20,
        Critical                    = 30
    };

    void                            Init();
    void                            Shutdown();

    void                            Start();
    void                            Stop();

    bool                            EnqueueTextureLoad(SharedPtr<Texture> texture, const char *filename, Texture::Flag flags, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueTextureStreamingReplacement(SharedPtr<Texture> texture, const char *filename, int32_t firstMip, LoadPriority priority = LoadPriority::Background);
    bool                            EnqueueMeshLoad(SharedPtr<Mesh> mesh, const char *filename, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueSkeletonLoad(SharedPtr<Skeleton> skeleton, const char *filename, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueAnimLoad(SharedPtr<Anim> anim, const char *filename, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueShaderPipelineBuild(SharedPtr<ShaderPipeline> shaderPipeline, ShaderCompileData &&compileData, uint32_t serial, LoadPriority priority = LoadPriority::High);

    void                            QueueMaterialReload(SharedPtr<Material> material, Material::InternalData &newInternalData);
    void                            QueueShaderTemplateReload(SharedPtr<ShaderTemplate> shaderTemplate, ShaderTemplate::InternalData &newInternalData);

    bool                            BoostTextureLoad(const Texture *texture, uint32_t serial, LoadPriority priority);
    bool                            BoostMeshLoad(const Mesh *mesh, uint32_t serial, LoadPriority priority);
    bool                            BoostSkeletonLoad(const Skeleton *skeleton, uint32_t serial, LoadPriority priority);
    bool                            BoostAnimLoad(const Anim *anim, uint32_t serial, LoadPriority priority);
    bool                            BoostShaderPipelineBuild(const ShaderPipeline *shaderPipeline, uint32_t serial, LoadPriority priority);

                                    /// Applies completed load requests on the main thread.
    void                            ProcessLoadCompletions();

                                    /// Processes pending reload completions after all rendering has completed.
    void                            ProcessPendingReloads();

    static bool                     IsAsyncAllowed();

private:
    struct LoadRequest {
        SharedPtr<Texture>          texture;
        SharedPtr<Mesh>             mesh;
        SharedPtr<Skeleton>         skeleton;
        SharedPtr<Anim>             anim;
        SharedPtr<ShaderPipeline>   shaderPipeline;
        ShaderCompileData           shaderCompileData;
        BE1::Str                    filename;
        Texture::Flag               textureFlags = Texture::Flag::None;
        SharedPtr<BE1::Image>       image;
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
        SharedPtr<ShaderPipeline>   shaderPipeline;
        Texture::InternalData       textureData;
        Mesh::InternalData          meshData;
        Skeleton::InternalData      skeletonData;
        Anim::InternalData          animData;
        RHI::PipelineState *        pso = nullptr;
        BE1::Image::MipRange        mipRange;
        uint32_t                    serial = 0;
        bool                        isReload = false;
        bool                        isStreamingReplacement = false;
        bool                        success = false;
    };

    struct PendingMaterialReload {
        SharedPtr<Material>         material;
        Material::InternalData      internalData;
    };

    struct PendingShaderTemplateReload {
        SharedPtr<ShaderTemplate>   shaderTemplate;
        ShaderTemplate::InternalData internalData;
        BE1::Array<ShaderPropertyDesc> oldProperties;
    };

    void                            UpdateWorkerSettings();

    void                            Run();
    bool                            PopNextRequest(LoadRequest &outRequest);
    bool                            ProcessTextureLoadRequest(LoadRequest &&request);
    bool                            ProcessTextureStreamingReplacementRequest(LoadRequest &&request);
    bool                            ProcessMeshLoadRequest(LoadRequest &&request);
    bool                            ProcessSkeletonLoadRequest(LoadRequest &&request);
    bool                            ProcessAnimLoadRequest(LoadRequest &&request);
    bool                            ProcessShaderPipelineBuildRequest(LoadRequest &&request);

    void                            PushCompletion(LoadCompletion &&completion);
    void                            DiscardCompletion(LoadCompletion &completion);
    bool                            ProcessTextureLoadCompletion(LoadCompletion &completion);
    bool                            ProcessMeshLoadCompletion(LoadCompletion &completion);
    bool                            ProcessSkeletonLoadCompletion(LoadCompletion &completion);
    bool                            ProcessAnimLoadCompletion(LoadCompletion &completion);
    bool                            ProcessShaderPipelineBuildCompletion(LoadCompletion &completion);
    void                            ProcessLoadCompletion(LoadCompletion &completion);

    void                            ProcessMaterialReloads(BE1::Array<PendingMaterialReload> &pendingReloads);
    void                            ProcessShaderTemplateReloads(BE1::Array<PendingShaderTemplateReload> &pendingReloads);

    bool                            TryDiscardInvalidReloadCompletion(LoadCompletion &completion);

    static bool                     IsLiveResourceReplacement(const LoadCompletion &completion);
    static bool                     IsHigherPriority(const LoadRequest &lhs, const LoadRequest &rhs);
    static unsigned int             ThreadProc(void *param);

    BE1::PlatformThread *           thread = nullptr;
    BE1::Mutex                      requestMutex;
    BE1::Mutex                      completionMutex;
    BE1::Condition                  requestCondition;

    BE1::Array<LoadRequest>         requests;
    BE1::Array<LoadCompletion>      completions;
    BE1::Array<LoadCompletion>      reloadCompletions;
    BE1::Array<PendingMaterialReload> pendingMaterialReloads;
    BE1::Array<PendingShaderTemplateReload> pendingShaderTemplateReloads;
    uint64_t                        nextRequestSequence = 0;
    std::atomic<int>                workerDelayMsec { 0 };

    bool                            stopping = false;
};

extern LoadingSystem                loadingSystem;
