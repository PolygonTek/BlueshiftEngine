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

class LoadingSystem {
public:
    void                            Init();
    void                            Shutdown();

    void                            Start();
    void                            Stop();

    bool                            EnqueueTextureLoad(std::shared_ptr<Texture> texture, const char *filename, Texture::Flag flags, bool isReload, uint32_t serial);
    bool                            EnqueueTextureStreamingReplacement(std::shared_ptr<Texture> texture, const char *filename, int firstMip);
    bool                            EnqueueMeshLoad(std::shared_ptr<Mesh> mesh, const char *filename, bool isReload, uint32_t serial);
    bool                            EnqueueSkeletonLoad(std::shared_ptr<Skeleton> skeleton, const char *filename, bool isReload, uint32_t serial);
    bool                            EnqueueAnimLoad(std::shared_ptr<Anim> anim, const char *filename, bool isReload, uint32_t serial);

                                    /// Applies completed load requests on the main thread.
    void                            ProcessLoadCompletions();

private:
    struct LoadRequest {
        std::shared_ptr<Texture>    texture;
        std::shared_ptr<Mesh>       mesh;
        std::shared_ptr<Skeleton>   skeleton;
        std::shared_ptr<Anim>       anim;
        BE1::Str                    filename;
        Texture::Flag               textureFlags = Texture::Flag::None;
        std::shared_ptr<BE1::Image> image;
        int                         originalMipLevels = 0;
        int                         firstMip = -1;
        uint32_t                    serial = 0;
        bool                        isReload = false;
        bool                        isStreamingReplacement = false;
    };

    struct LoadCompletion {
        std::shared_ptr<Texture>    texture;
        std::shared_ptr<Mesh>       mesh;
        std::shared_ptr<Skeleton>   skeleton;
        std::shared_ptr<Anim>       anim;
        Texture::InternalData       textureData;
        BE1::Image::MipRange        mipRange;
        Mesh::InternalData          meshData;
        Skeleton::InternalData      skeletonData;
        Anim::InternalData          animData;
        uint32_t                    serial = 0;
        bool                        isReload = false;
        bool                        isStreamingReplacement = false;
        bool                        success = false;
    };

    void                            Run();
    bool                            ProcessTextureLoadRequest(const LoadRequest &request);
    bool                            ProcessTextureStreamingReplacementRequest(const LoadRequest &request);
    bool                            ProcessMeshLoadRequest(const LoadRequest &request);
    bool                            ProcessSkeletonLoadRequest(const LoadRequest &request);
    bool                            ProcessAnimLoadRequest(const LoadRequest &request);

    void                            PushCompletion(LoadCompletion &&completion);
    void                            DiscardCompletion(LoadCompletion &completion);
    bool                            ProcessTextureLoadCompletion(LoadCompletion &completion);
    bool                            ProcessMeshLoadCompletion(LoadCompletion &completion);
    bool                            ProcessSkeletonLoadCompletion(LoadCompletion &completion);
    bool                            ProcessAnimLoadCompletion(LoadCompletion &completion);

    static unsigned int             ThreadProc(void *param);

    BE1::PlatformThread *           thread = nullptr;
    BE1::PlatformMutex *            requestMutex = nullptr;
    BE1::PlatformMutex *            completionMutex = nullptr;
    BE1::PlatformCondition *        requestCondition = nullptr;

    BE1::Array<LoadRequest>         requests;
    BE1::Array<LoadCompletion>      completions;

    bool                            stopping = false;
};

extern LoadingSystem                loadingSystem;
