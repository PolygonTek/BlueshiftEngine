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

static constexpr int RenderSSRHiZMaxLevelCount = 16;

class SSRHiZResources {
public:
    SSRHiZResources() = default;
    ~SSRHiZResources();
    SSRHiZResources(const SSRHiZResources &) = delete;
    SSRHiZResources &operator=(const SSRHiZResources &) = delete;

    bool                        Ensure(uint32_t renderWidth, uint32_t renderHeight, bool needed);
    void                        Destroy();

    RHI::Texture *              GetTexture() const { return texture; }
    const RHI::ViewID *         GetMipSRVs() const { return mipSRVs; }
    const RHI::ViewID *         GetMipUAVs() const { return mipUAVs; }
    const uint32_t *            GetMipWidths() const { return mipWidths; }
    const uint32_t *            GetMipHeights() const { return mipHeights; }
    uint32_t                    GetMipCount() const { return mipCount; }
    uint32_t                    GetValidWidth() const { return validWidth; }
    uint32_t                    GetValidHeight() const { return validHeight; }

private:
    bool                        CreateTexture(uint32_t width, uint32_t height);

    RHI::Texture *              texture = nullptr;
    RHI::ViewID                 mipSRVs[RenderSSRHiZMaxLevelCount] = {};
    RHI::ViewID                 mipUAVs[RenderSSRHiZMaxLevelCount] = {};
    uint32_t                    mipWidths[RenderSSRHiZMaxLevelCount] = {};
    uint32_t                    mipHeights[RenderSSRHiZMaxLevelCount] = {};
    uint32_t                    mipCount = 0;
    uint32_t                    validWidth = 0;
    uint32_t                    validHeight = 0;
};
