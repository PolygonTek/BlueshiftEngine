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

    bool                        EnqueueTextureLoad(Texture *texture, const char *filename, Texture::Flag flags, uint32_t serial);
    bool                        EnqueueMeshLoad(Mesh *mesh, const char *filename, uint32_t serial);

                                /// Applies completed load requests on the main thread.
    void                        ProcessCompletions();

private:
    struct LoadRequest {
        Texture *               texture = nullptr;
        Mesh *                  mesh = nullptr;
        BE1::Str                filename;
        Texture::Flag           textureFlags = Texture::Flag::None;
        uint32_t                serial = 0;
    };

    struct LoadCompletion {
        Texture *               texture = nullptr;
        Mesh *                  mesh = nullptr;
        Texture::Prepared       texturePrepared;
        //Mesh::Prepared        meshPrepared;
        uint32_t                serial = 0;
        bool                    success = false;
    };

    void                        Run();
    bool                        PopRequest(LoadRequest &outRequest);
    void                        PushCompletion(LoadCompletion &&completion);
    void                        DiscardCompletion(LoadCompletion &completion);
    void                        DiscardCompletions(BE1::Array<LoadCompletion> &completions);
    bool                        ProcessTextureCompletion(LoadCompletion &completion);
    bool                        ProcessMeshCompletion(LoadCompletion &completion);

    static unsigned int         ThreadProc(void *param);

    BE1::PlatformThread *       thread = nullptr;
    BE1::PlatformMutex *        requestMutex = nullptr;
    BE1::PlatformMutex *        completionMutex = nullptr;
    BE1::PlatformCondition *    requestCondition = nullptr;

    BE1::Array<LoadRequest> requests;
    BE1::Array<LoadCompletion> completions;

    bool                        stopping = false;
};

extern LoadingSystem            loadingSystem;
