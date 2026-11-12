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

#include "RHI/RHI.h"

class VolumetricFogResources {
public:
    ~VolumetricFogResources();
    VolumetricFogResources() = default;
    VolumetricFogResources(const VolumetricFogResources &) = delete;
    VolumetricFogResources &operator=(const VolumetricFogResources &) = delete;

    bool                        Ensure(uint32_t width, uint32_t height, uint32_t slices, bool needed);
    void                        Destroy();

    RHI::Texture *              GetSourceTexture() const { return sourceTextures[sourceTextureIndex]; }
    RHI::Texture *              GetHistoryTexture() const { return sourceTextures[sourceTextureIndex ^ 1u]; }
    RHI::Texture *              GetIntegratedTexture() const { return integratedTexture; }
    uint64_t                    GetGenerationNumber() const { return generationNumber; }

    void                        SwapSourceTextures() { assert(sourceTextures[1]); sourceTextureIndex ^= 1u; }

private:
    RHI::Texture *              sourceTextures[2] = {};
    RHI::Texture *              integratedTexture = nullptr;
    uint32_t                    sourceTextureIndex = 0;
    uint64_t                    generationNumber = 0;
};
