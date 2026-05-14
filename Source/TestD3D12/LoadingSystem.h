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

class LoadingSystem {
public:
    void                        Init();
    void                        Shutdown();

    void                        Start();
    void                        Stop();

    bool                        EnqueueTextureLoad(uint32_t serial, Texture *texture, const char *filename, Texture::Flag flags);
    bool                        EnqueueTextureMipRangeLoad(Texture *texture, const char *filename, int firstMip, int mipCount);
    bool                        EnqueueMeshLoad(uint32_t serial, Mesh *mesh, const char *filename);

                                /// Applies completed load requests on the main thread.
    void                        ProcessLoadCompletions();

private:
    struct LoadRequest {
        Texture *               texture = nullptr;
        Mesh *                  mesh = nullptr;
        BE1::Str                filename;
        Texture::Flag           textureFlags = Texture::Flag::None;
        std::shared_ptr<BE1::Image> image;
        bool                    partialMipLoading = false;
        int                     firstMip = -1;
        int                     mipCount = -1;
        uint32_t                serial = 0;
    };

    struct LoadCompletion {
        Texture *               texture = nullptr;
        Mesh *                  mesh = nullptr;
        std::shared_ptr<BE1::Image> image;
        BE1::Image::MipRange    mipRange;
        Texture::Prepared       texturePrepared;
        //Mesh::Prepared        meshPrepared;
        uint32_t                serial = 0;
        bool                    streaming = false;
        bool                    success = false;
    };

    void                        Run();
    bool                        PopRequest(LoadRequest &outRequest);
    bool                        ProcessTextureLoadRequest(const LoadRequest &request);
    bool                        ProcessTextureMipRangeLoadRequest(const LoadRequest &request);
    bool                        ProcessMeshLoadRequest(const LoadRequest &request);

    void                        PushCompletion(LoadCompletion &&completion);
    void                        DiscardCompletion(LoadCompletion &completion);
    bool                        ProcessTextureLoadCompletion(LoadCompletion &completion);
    bool                        ProcessMeshLoadCompletion(LoadCompletion &completion);

    static unsigned int         ThreadProc(void *param);

    BE1::PlatformThread *       thread = nullptr;
    BE1::PlatformMutex *        requestMutex = nullptr;
    BE1::PlatformMutex *        completionMutex = nullptr;
    BE1::PlatformCondition *    requestCondition = nullptr;

    BE1::Array<LoadRequest>     requests;
    BE1::Array<LoadCompletion>  completions;

    bool                        stopping = false;
};

extern LoadingSystem            loadingSystem;
