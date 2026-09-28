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
#include "ShadowAtlasAllocator.h"

/*
---------------------------------------------------------------------------------------------------

    Shadow Atlas

    RenderContext 소유.
    프런트엔드는 할당 상태를 변경하고, 백엔드에는 확정된 타일과 리소스를 복사해 전달한다.

---------------------------------------------------------------------------------------------------
*/

class ShadowAtlas {
public:
    using Handle = ShadowAtlasAllocator::Handle;
    using Tile = ShadowAtlasAllocator::Tile;

    ShadowAtlas() = default;
    ~ShadowAtlas() { Destroy(); }
    ShadowAtlas(const ShadowAtlas &) = delete;
    ShadowAtlas &operator=(const ShadowAtlas &) = delete;

    bool                            Ensure(uint32_t atlasSize, uint32_t maxTileResolution, uint32_t minTileResolution);
    void                            Destroy();

    bool                            IsAllocated(Handle handle) const { return allocator.IsAllocated(handle); }
    void                            ResetAllocations() { allocator.Reset(); }

    bool                            Allocate(uint32_t requestedResolution, uint32_t tileCount, Tile *outTiles) { return allocator.Allocate(requestedResolution, tileCount, outTiles); }
    bool                            Free(Handle handle) { return allocator.Free(handle); }

    uint32_t                        GetSize() const { return allocator.GetSize(); }
    uint32_t                        GetMaxTileResolution() const { return allocator.GetMaxTileResolution(); }
    uint32_t                        GetAllocedTileCount() const { return allocator.GetAllocedTileCount(); }
    uint64_t                        GetAllocedArea() const { return allocator.GetAllocedArea(); }

    RHI::Texture *                  GetShadowTexture() const { return shadowTexture; }
    RHI::ViewID                     GetShadowDSV() const { return shadowDSV; }
    RHI::ViewID                     GetShadowSRV() const { return shadowSRV; }

private:
    ShadowAtlasAllocator            allocator;
    RHI::Texture *                  shadowTexture = nullptr;
    RHI::ViewID                     shadowDSV = RHI::ViewID::InvalidID;
    RHI::ViewID                     shadowSRV = RHI::ViewID::InvalidID;
};
