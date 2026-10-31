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

#include <cstdint>
#include <memory>

/*
---------------------------------------------------------------------------------------------------

    Shadow Atlas Allocator

    쿼드트리 기반 CPU 전용 shadow atlas 타일 할당기.

    - 최대 타일 크기의 root 를 필요에 따라 4 분할하고, 반납 시 형제 4개가 모두 비워지면 부모로 합쳐친다.
    - 기존 타일을 재배치하지 않으므로 할당이 유지되는 동안 좌표가 변하지 않는다.
    - 타일 묶음 전체를 같은 해상도로 확보한다. Spot 은 1개, Point 는 6개를 요청한다.
    - 실패하면 부분 할당을 모두 반환하고, 해상도를 절반씩 낮춰 최소 해상도까지 재시도한다.
    - Init 에서 노드를 미리 확보하고 level 별 free list 로 관리하며, 할당/해제 중 힙 할당은 없다.
    - 프런트엔드 스레드에서만 변경하며, 요청 해상도 선정과 광원별 타일 재사용/회수는 호출 측에서 담당한다.

---------------------------------------------------------------------------------------------------
*/

class ShadowAtlasAllocator {
public:
    static constexpr uint32_t       InvalidNode = ~uint32_t(0);

    // 핸들은 발급한 allocator 에서만 사용한다. Reset/Init/Shutdown 과 재할당 후에는 무효가 된다.
    struct Handle {
        uint64_t                    generation = 0;
        uint32_t                    nodeIndex = InvalidNode;

        bool                        IsValid() const { return generation != 0 && nodeIndex != InvalidNode; }
    };

    struct Tile {
        Handle                      handle;
        uint32_t                    x = 0;
        uint32_t                    y = 0;
        uint32_t                    resolution = 0; // Border 를 포함한 물리적 타일 크기.
    };

    ShadowAtlasAllocator() = default;
    ShadowAtlasAllocator(const ShadowAtlasAllocator &) = delete;
    ShadowAtlasAllocator &operator=(const ShadowAtlasAllocator &) = delete;

                                    // 크기는 모두 2의 거듭제곱. 실패하면 기존 할당 상태를 보존한다.
    bool                            Init(uint32_t atlasSize, uint32_t maxTileResolution, uint32_t minTileResolution);
    void                            Shutdown();
    void                            Reset();

                                    // 같은 해상도의 타일 묶음을 확보하며, 실패하면 절반 해상도로 재시도한다.
    bool                            Allocate(uint32_t requestedResolution, uint32_t tileCount, Tile *tiles);
    bool                            Free(Handle handle);
    bool                            IsAllocated(Handle handle) const;

    uint32_t                        GetSize() const { return atlasSize; }
    uint32_t                        GetMaxTileResolution() const { return levelCount ? levels[0].resolution : 0; }
    uint32_t                        GetMinTileResolution() const { return levelCount ? levels[levelCount - 1].resolution : 0; }
    uint32_t                        GetAllocedTileCount() const { return allocatedTileCount; }
    uint64_t                        GetAllocedArea() const { return allocatedArea; }

private:
    static constexpr uint32_t       MaxLevelCount = 16;

    enum class NodeState : uint8_t {
        Inactive,
        Free,
        Split,
        Allocated
    };

    struct Node {
        uint64_t                    generation;
        uint32_t                    previous;
        uint32_t                    next;
        NodeState                   state;
    };

    struct Level {
        uint32_t                    firstNode = 0;
        uint32_t                    nodeCount = 0;
        uint32_t                    resolution = 0;
        uint32_t                    freeHead = InvalidNode;
    };

    bool                            AllocateAtLevel(uint32_t levelIndex, Tile &tile);
    void                            AddFreeNode(uint32_t levelIndex, uint32_t nodeIndex);
    void                            RemoveFreeNode(uint32_t levelIndex, uint32_t nodeIndex);

    std::unique_ptr<Node[]>         nodes;
    Level                           levels[MaxLevelCount] = {};
    uint32_t                        levelCount = 0;
    uint32_t                        nodeCount = 0;
    uint32_t                        atlasSize = 0;
    uint32_t                        rootsPerRow = 0;
    uint32_t                        allocatedTileCount = 0;
    uint64_t                        allocatedArea = 0;
    uint64_t                        nextGeneration = 0; // 재초기화해도 되돌리지 않아 이전 핸들의 재사용을 막는다.
};
