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

static constexpr int RenderHiZMaxLevelCount = 16;
static constexpr int RenderHiZCullMaxInstanceCount = 2048;
static constexpr int RenderHiZCullMaxVisibleInstanceCount = 0x40000;
static constexpr int RenderHiZCullMaxBatchCount = RenderHiZCullMaxVisibleInstanceCount;

class HiZResources {
public:
    HiZResources() = default;
    ~HiZResources();
    HiZResources(const HiZResources &) = delete;
    HiZResources &operator=(const HiZResources &) = delete;

    void                        Ensure(uint32_t renderWidth, uint32_t renderHeight, bool needHiZTexture, bool needHiZCullResources, bool needHiZDebugPreview);
    void                        Destroy();

    RHI::Texture *              GetTexture() const { return texture; }
    const RHI::ViewID *         GetMipSRVs() const { return mipSRVs; }
    const RHI::ViewID *         GetMipUAVs() const { return mipUAVs; }
    const uint32_t *            GetMipWidths() const { return mipWidths; }
    const uint32_t *            GetMipHeights() const { return mipHeights; }
    uint32_t                    GetMipCount() const { return mipCount; }
    RHI::Texture *              GetDebugPreviewTexture() const { return debugPreviewTexture; }
    RHI::Buffer *               GetVisibleInstanceBuffer() const { return visibleInstanceBuffer; }
    RHI::Buffer *               GetIndirectArgsBuffer() const { return indirectArgsBuffer; }

    bool                        IsHistoryValid() const { return historyValid; }
    void                        SetHistoryValid(bool valid) { historyValid = valid; }

private:
    bool                        CreateTexture(uint32_t width, uint32_t height);
    void                        DestroyTexture();
    bool                        EnsureDebugPreviewTexture(uint32_t width, uint32_t height);
    bool                        CreateDebugPreviewTexture(uint32_t width, uint32_t height);
    void                        DestroyDebugPreviewTexture();
    bool                        CreateCullResources();
    void                        DestroyCullResources();

    RHI::Texture *              texture = nullptr;
    RHI::ViewID                 mipSRVs[RenderHiZMaxLevelCount] = {};
    RHI::ViewID                 mipUAVs[RenderHiZMaxLevelCount] = {};
    uint32_t                    mipWidths[RenderHiZMaxLevelCount] = {};
    uint32_t                    mipHeights[RenderHiZMaxLevelCount] = {};
    uint32_t                    mipCount = 0;
    RHI::Texture *              debugPreviewTexture = nullptr;
    RHI::Buffer *               visibleInstanceBuffer = nullptr;
    RHI::Buffer *               indirectArgsBuffer = nullptr;
    bool                        historyValid = false;
};
