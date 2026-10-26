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

#include "Containers/Array.h"
#include "Containers/HashMap.h"
#include "Texture.h"

/*
---------------------------------------------------------------------------------------------------

    Texture Manager

---------------------------------------------------------------------------------------------------
*/

class TextureManager {
    friend class Texture;
    friend class LoadingSystem;

public:
    void                            Init();
    void                            Shutdown();

    void                            UpdateFrame();

    SharedPtr<Texture>              AllocTexture(const char *hashName);
    SharedPtr<Texture>              FindTexture(const char *hashName) const;
    SharedPtr<Texture>              GetTexture(const char *hashName, Texture::Flag flags = Texture::Flag::None, bool async = true);

    bool                            IsRegisteredStreamingTexture(const Texture *texture) const;
    void                            RegisterStreamingTexture(const SharedPtr<Texture> &texture, const BE1::Image::MipRange &mipRange);
    void                            UnregisterStreamingTexture(Texture *texture);
    uint32_t                        GetStreamingTextureCount() const { return streamingTextures.Count(); }
    uint64_t                        GetStreamingTextureMemoryUsageBytes() const { return streamingTextureMemoryUsageBytes; }
    uint64_t                        GetStreamingTextureMemoryBudgetBytes() const { return streamingTextureMemoryBudgetBytes; }
    uint32_t                        GetStreamingTextureMinResidentMaxDimension() const;
    void                            SetStreamingTextureMemoryBudgetBytes(uint64_t budget) { streamingTextureMemoryBudgetBytes = budget; }

    SharedPtr<Texture>              missingTexture;
    SharedPtr<Texture>              blackTexture;
    SharedPtr<Texture>              whiteTexture;
    SharedPtr<Texture>              white3DTexture;
    SharedPtr<Texture>              flatNormalTexture;

private:
    void                            CleanExpiredTextures();
    void                            CreateDefaultTextures();
    void                            DestroyDefaultTextures();
    void                            UpdateStreamingTextures();
    void                            ComputeStreamingTextureMemoryUsage();

    BE1::StrIHashMap<WeakPtr<Texture>> textureHashMap;
    int                             expiredTextureHashMapCursor = 0;
    uint32_t                        nextTextureIndex = 0;
    BE1::Array<WeakPtr<Texture>>    streamingTextures;
    uint64_t                        streamingTextureMemoryUsageBytes = 0;
    uint64_t                        streamingTextureMemoryBudgetBytes = 256 * (1024 * 1024);
};

extern TextureManager               textureManager;
