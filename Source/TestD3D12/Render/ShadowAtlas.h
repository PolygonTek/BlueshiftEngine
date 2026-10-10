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
#include "Containers/HashIndex.h"
#include "RHI/RHI.h"
#include "ShadowAtlasAllocator.h"
#include <atomic>

/*
---------------------------------------------------------------------------------------------------

    Shadow Atlas

    RenderCamera 소유.
    프런트엔드는 할당 상태를 변경하고, 백엔드에는 확정된 타일과 리소스를 복사해 전달한다.

---------------------------------------------------------------------------------------------------
*/

class ShadowAtlas {
public:
    using Tile = ShadowAtlasAllocator::Tile;

    // 광원 그림자의 depth 렌더링 결과를 프런트엔드와 백엔드가 공유한다.
    class RenderState {
    public:
        bool                        HasValidDepth() const { return hasValidDepth.load(std::memory_order_acquire); }

                                    // 백엔드에서 depth 기록의 성공 여부를 저장한다.
        void                        SetDepthValid(bool valid) { hasValidDepth.store(valid, std::memory_order_release); }

    private:
        std::atomic<bool>           hasValidDepth { false };
    };

    // 광원 그림자의 타일 할당 정보와 갱신 상태
    struct Entry {
        uint64_t                    lightId = 0;
        uint64_t                    shadowRevision = 0;         // 마지막으로 갱신을 요청한 라이트의 그림자 설정 변경 번호.
        uint32_t                    casterLayerMask = 0;        // 마지막으로 갱신을 요청한 caster 레이어 마스크.
        uint32_t                    borderTexels = 0;           // 마지막으로 갱신을 요청한 border. 스킵 프레임에도 같은 값을 사용한다.
        BE1::Mat4                   spotViewProjMatrix = BE1::Mat4::identity; // 마지막으로 갱신을 요청한 spot 라이트의 view-projection 행렬. 스킵 프레임에도 같은 값을 사용한다.
        BE1::Vec3                   pointOrigin = BE1::Vec3::zero;  // 마지막으로 갱신을 요청한 point 라이트의 origin. 스킵 프레임에도 같은 값을 사용한다.
        BE1::Vec2                   pointDepthScaleBias = BE1::Vec2::zero; // 마지막으로 갱신을 요청한 point 라이트의 depth scale bias. 스킵 프레임에도 같은 값을 사용한다.
        uint32_t                    requestedResolution = 0;
        uint32_t                    numTiles = 0;               // Spot 은 1개, Point 는 6개
        Tile                        tiles[6];                   // 모든 타일의 해상도는 동일하다.
        SharedPtr<RenderState>      renderState;                // Backend 와 공유하며 Entry 이동/해제와 독립적으로 유지된다.
        bool                        keep = false;               // 다음 FreeUnusedEntries 에서 entry 를 유지할지 여부.
    };

    ShadowAtlas() = default;
    ~ShadowAtlas() { Destroy(); }
    ShadowAtlas(const ShadowAtlas &) = delete;
    ShadowAtlas &operator=(const ShadowAtlas &) = delete;

    bool                            Ensure(uint32_t atlasSize, uint32_t maxTileResolution, uint32_t minTileResolution);
    void                            Destroy();

    void                            ResetAllocations();

                                    // 엔트리를 찾는다. 반환한 포인터는 항목 추가/해제 또는 atlas 재초기화 전까지만 유효하다.
    Entry *                         FindEntry(uint64_t lightId);
    const Entry *                   FindEntry(uint64_t lightId) const;

                                    // 현재 요청과 일치하는 기존 엔트리를 유지 대상으로 표시한다.
    void                            KeepEntry(uint64_t lightId, uint32_t requestedResolution, uint32_t tileCount);

                                    // 표시되지 않은 엔트리를 반환하고, 유지한 엔트리의 표시도 다음 호출을 위해 지운다.
    void                            FreeUnusedEntries();

                                    // 새 라이트 그림자의 타일 묶음을 할당한다.
    Entry *                         Alloc(uint64_t lightId, uint32_t requestedResolution, uint32_t tileCount);
    bool                            Free(uint64_t lightId);

    uint32_t                        GetSize() const { return allocator.GetSize(); }
    uint32_t                        GetMaxTileResolution() const { return allocator.GetMaxTileResolution(); }
    uint32_t                        GetAllocedTileCount() const { return allocator.GetAllocedTileCount(); }
    uint64_t                        GetAllocedArea() const { return allocator.GetAllocedArea(); }

    RHI::Texture *                  GetShadowTexture() const { return shadowTexture; }
    RHI::ViewID                     GetShadowDSV() const { return shadowDSV; }
    RHI::ViewID                     GetShadowSRV() const { return shadowSRV; }

private:
    void                            ClearEntries();
    int                             FindEntryIndex(uint64_t lightId) const;
    void                            RemoveEntry(int index);

    ShadowAtlasAllocator            allocator;
    BE1::Array<Entry>               entries;
    BE1::HashIndex                  entryHash { 1024, 128 };
    RHI::Texture *                  shadowTexture = nullptr;
    RHI::ViewID                     shadowDSV = RHI::ViewID::InvalidID;
    RHI::ViewID                     shadowSRV = RHI::ViewID::InvalidID;
};
