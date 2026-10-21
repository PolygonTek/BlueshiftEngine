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

static constexpr int RenderSSRSceneColorHistoryMaxLevelCount = 16;

class SSRSceneColorHistoryResources {
public:
    SSRSceneColorHistoryResources() = default;
    ~SSRSceneColorHistoryResources();
    SSRSceneColorHistoryResources(const SSRSceneColorHistoryResources &) = delete;
    SSRSceneColorHistoryResources &operator=(const SSRSceneColorHistoryResources &) = delete;

    bool                        Ensure(uint32_t renderWidth, uint32_t renderHeight, RHI::Format radianceFormat, bool fullResolutionDepth, bool needed);
    void                        Destroy();
    void                        Invalidate() { valid = false; }
    void                        SetValid(bool value) { valid = value; }

    RHI::Texture *              GetRadianceTexture() const { return radianceTexture; }
    RHI::Texture *              GetRadiancePyramidTexture() const { return radiancePyramidTexture; }
    RHI::Texture *              GetDepthExtremaTexture() const { return depthExtremaTexture; }
    RHI::Texture *              GetFullResolutionDepthTexture() const { return fullResolutionDepthTexture; }
    const RHI::ViewID *         GetMipSRVs() const { return mipSRVs; }
    const RHI::ViewID *         GetMipUAVs() const { return mipUAVs; }
    const uint32_t *            GetMipWidths() const { return mipWidths; }
    const uint32_t *            GetMipHeights() const { return mipHeights; }
    uint32_t                    GetMipCount() const { return mipCount; }
    bool                        IsValid() const { return valid; }

private:
    bool                        CreateTextures(uint32_t width, uint32_t height, RHI::Format radianceFormat);

    RHI::Texture *              radianceTexture = nullptr;
    RHI::Texture *              radiancePyramidTexture = nullptr;
    RHI::Texture *              depthExtremaTexture = nullptr;
    RHI::Texture *              fullResolutionDepthTexture = nullptr;
    RHI::ViewID                 mipSRVs[RenderSSRSceneColorHistoryMaxLevelCount] = {};
    RHI::ViewID                 mipUAVs[RenderSSRSceneColorHistoryMaxLevelCount] = {};
    uint32_t                    mipWidths[RenderSSRSceneColorHistoryMaxLevelCount] = {};
    uint32_t                    mipHeights[RenderSSRSceneColorHistoryMaxLevelCount] = {};
    uint32_t                    mipCount = 0;
    bool                        valid = false;
};
