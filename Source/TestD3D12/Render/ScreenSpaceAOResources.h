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

struct RenderFrameSettings;

static constexpr int RenderGTAODepthMipMaxLevelCount = 5;

struct ScreenSpaceAOTextures {
    RHI::Texture *                  outputTexture = nullptr;
    RHI::Texture *                  rawTexture = nullptr;
    RHI::Texture *                  tempTexture = nullptr;
    RHI::Texture *                  halfDepthTexture = nullptr;
    RHI::Texture *                  gtaoPrefilterDepthTexture = nullptr;
    RHI::Texture *                  gtaoPackedEdgesTexture = nullptr;
    RHI::ViewID                     gtaoDepthMipSRVs[RenderGTAODepthMipMaxLevelCount] = {};
    RHI::ViewID                     gtaoDepthMipUAVs[RenderGTAODepthMipMaxLevelCount] = {};
    uint32_t                        gtaoDepthMipCount = 0;
};

class ScreenSpaceAOResources {
public:
    ScreenSpaceAOResources() = default;
    ~ScreenSpaceAOResources();
    ScreenSpaceAOResources(const ScreenSpaceAOResources &) = delete;
    ScreenSpaceAOResources &operator=(const ScreenSpaceAOResources &) = delete;

    void                            Ensure(uint32_t renderWidth, uint32_t renderHeight, const RenderFrameSettings &settings);
    void                            Destroy();

    const ScreenSpaceAOTextures &   GetTextures() const { return textures; }

private:
    void                            DestroyFilterResources();
    void                            DestroyGTAOResources();
    bool                            CreateGTAOPrefilterDepthTexture(uint32_t width, uint32_t height, BE1::Image::Format imageFormat);

    ScreenSpaceAOTextures           textures;
};
