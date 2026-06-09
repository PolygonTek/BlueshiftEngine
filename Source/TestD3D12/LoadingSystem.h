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

#include "Render/Texture.h"
#include "Render/Mesh.h"
#include "Render/Skeleton.h"
#include "Render/Anim.h"
#include "Render/ShaderTemplate.h"

class ShaderPipelineState;

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

    bool                            EnqueueTextureLoad(std::shared_ptr<Texture> texture, const char *filename, Texture::Flag flags, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueTextureStreamingReplacement(std::shared_ptr<Texture> texture, const char *filename, int firstMip, LoadPriority priority = LoadPriority::Background);
    bool                            EnqueueMeshLoad(std::shared_ptr<Mesh> mesh, const char *filename, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueSkeletonLoad(std::shared_ptr<Skeleton> skeleton, const char *filename, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueAnimLoad(std::shared_ptr<Anim> anim, const char *filename, bool isReload, uint32_t serial, LoadPriority priority = LoadPriority::Normal);
    bool                            EnqueueShaderPipelineBuild(std::shared_ptr<ShaderPipelineState> pipelineState, const ShaderTemplate::PipelineBuildData &buildData, uint32_t serial, LoadPriority priority = LoadPriority::High);

    bool                            BoostTextureLoad(const Texture *texture, uint32_t serial, LoadPriority priority);
    bool                            BoostMeshLoad(const Mesh *mesh, uint32_t serial, LoadPriority priority);
    bool                            BoostSkeletonLoad(const Skeleton *skeleton, uint32_t serial, LoadPriority priority);
    bool                            BoostAnimLoad(const Anim *anim, uint32_t serial, LoadPriority priority);
    bool                            BoostShaderPipelineBuild(const ShaderPipelineState *pipelineState, uint32_t serial, LoadPriority priority);

                                    /// Applies completed load requests on the main thread.
    void                            ProcessLoadCompletions();

private:
    struct LoadRequest {
        std::shared_ptr<Texture>    texture;
        std::shared_ptr<Mesh>       mesh;
        std::shared_ptr<Skeleton>   skeleton;
        std::shared_ptr<Anim>       anim;
        std::shared_ptr<ShaderPipelineState> pipelineState;
        ShaderTemplate::PipelineBuildData shaderPipelineBuildData;
        BE1::Str                    filename;
        Texture::Flag               textureFlags = Texture::Flag::None;
        std::shared_ptr<BE1::Image> image;
        int                         originalMipLevels = 0;
        int                         firstMip = -1;
        uint32_t                    serial = 0;
        uint64_t                    sequence = 0;
        LoadPriority                priority = LoadPriority::Normal;
        bool                        isReload = false;
        bool                        isStreamingReplacement = false;
    };

    struct LoadCompletion {
        std::shared_ptr<Texture>    texture;
        std::shared_ptr<Mesh>       mesh;
        std::shared_ptr<Skeleton>   skeleton;
        std::shared_ptr<Anim>       anim;
        std::shared_ptr<ShaderPipelineState> pipelineState;
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

    void                            Run();
    bool                            PopNextRequest(LoadRequest &outRequest);
    bool                            ProcessTextureLoadRequest(const LoadRequest &request);
    bool                            ProcessTextureStreamingReplacementRequest(const LoadRequest &request);
    bool                            ProcessMeshLoadRequest(const LoadRequest &request);
    bool                            ProcessSkeletonLoadRequest(const LoadRequest &request);
    bool                            ProcessAnimLoadRequest(const LoadRequest &request);
    bool                            ProcessShaderPipelineBuildRequest(const LoadRequest &request);

    void                            PushCompletion(LoadCompletion &&completion);
    void                            DiscardCompletion(LoadCompletion &completion);
    bool                            ProcessTextureLoadCompletion(LoadCompletion &completion);
    bool                            ProcessMeshLoadCompletion(LoadCompletion &completion);
    bool                            ProcessSkeletonLoadCompletion(LoadCompletion &completion);
    bool                            ProcessAnimLoadCompletion(LoadCompletion &completion);
    bool                            ProcessShaderPipelineBuildCompletion(LoadCompletion &completion);

    static bool                     IsHigherPriority(const LoadRequest &lhs, const LoadRequest &rhs);
    static unsigned int             ThreadProc(void *param);

    BE1::PlatformThread *           thread = nullptr;
    BE1::PlatformMutex *            requestMutex = nullptr;
    BE1::PlatformMutex *            completionMutex = nullptr;
    BE1::PlatformCondition *        requestCondition = nullptr;

    BE1::Array<LoadRequest>         requests;
    BE1::Array<LoadCompletion>      completions;
    uint64_t                        nextRequestSequence = 0;

    bool                            stopping = false;
};

extern LoadingSystem                loadingSystem;
