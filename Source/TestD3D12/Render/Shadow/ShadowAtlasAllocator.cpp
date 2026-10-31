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

#include "Precompiled.h"
#include "ShadowAtlasAllocator.h"
#include <algorithm>
#include <cassert>
#include <limits>
#include <new>

bool ShadowAtlasAllocator::Init(uint32_t size, uint32_t maxResolution, uint32_t minResolution) {
    if (size > static_cast<uint32_t>(INT_MAX) || minResolution > maxResolution || maxResolution > size ||
        !BE1::IsPowerOfTwo(static_cast<int>(size)) || !BE1::IsPowerOfTwo(static_cast<int>(maxResolution)) ||
        !BE1::IsPowerOfTwo(static_cast<int>(minResolution))) {
        return false;
    }

    Level newLevels[MaxLevelCount] = {};
    uint32_t newLevelCount = 0;
    uint64_t newNodeCount = 0;
    uint64_t levelNodeCount = uint64_t(size / maxResolution) * (size / maxResolution);

    for (uint32_t resolution = maxResolution; ; resolution >>= 1) {
        if (newLevelCount == MaxLevelCount || newNodeCount + levelNodeCount > InvalidNode) {
            return false;
        }

        Level &level = newLevels[newLevelCount++];
        level.firstNode = static_cast<uint32_t>(newNodeCount);
        level.nodeCount = static_cast<uint32_t>(levelNodeCount);
        level.resolution = resolution;
        newNodeCount += levelNodeCount;

        if (resolution == minResolution) {
            break;
        }
        levelNodeCount *= 4;
    }

    if (newNodeCount > (std::numeric_limits<size_t>::max)() / sizeof(Node)) {
        return false;
    }

    std::unique_ptr<Node[]> newNodes(new (std::nothrow) Node[static_cast<size_t>(newNodeCount)]);
    if (!newNodes) {
        return false;
    }

    nodes = std::move(newNodes);
    std::copy_n(newLevels, newLevelCount, levels);
    levelCount = newLevelCount;
    nodeCount = static_cast<uint32_t>(newNodeCount);
    atlasSize = size;
    rootsPerRow = size / maxResolution;
    Reset();
    return true;
}

void ShadowAtlasAllocator::Shutdown() {
    nodes.reset();
    levelCount = 0;
    nodeCount = 0;
    atlasSize = 0;
    rootsPerRow = 0;
    allocatedTileCount = 0;
    allocatedArea = 0;
}

void ShadowAtlasAllocator::Reset() {
    allocatedTileCount = 0;
    allocatedArea = 0;

    if (!nodes) {
        return;
    }

    std::fill_n(nodes.get(), nodeCount, Node{0, InvalidNode, InvalidNode, NodeState::Inactive});
    for (uint32_t levelIndex = 0; levelIndex < levelCount; ++levelIndex) {
        levels[levelIndex].freeHead = InvalidNode;
    }

    for (uint32_t root = levels[0].nodeCount; root > 0; --root) {
        AddFreeNode(0, root - 1);
    }
}

void ShadowAtlasAllocator::AddFreeNode(uint32_t levelIndex, uint32_t nodeIndex) {
    Level &level = levels[levelIndex];
    Node &node = nodes[nodeIndex];
    node.state = NodeState::Free;
    node.previous = InvalidNode;
    node.next = level.freeHead;
    if (node.next != InvalidNode) {
        nodes[node.next].previous = nodeIndex;
    }
    level.freeHead = nodeIndex;
}

void ShadowAtlasAllocator::RemoveFreeNode(uint32_t levelIndex, uint32_t nodeIndex) {
    Node &node = nodes[nodeIndex];
    assert(node.state == NodeState::Free);
    if (node.previous != InvalidNode) {
        nodes[node.previous].next = node.next;
    } else {
        levels[levelIndex].freeHead = node.next;
    }

    if (node.next != InvalidNode) {
        nodes[node.next].previous = node.previous;
    }
    node.state = NodeState::Inactive;
    node.previous = InvalidNode;
    node.next = InvalidNode;
}

bool ShadowAtlasAllocator::AllocateAtLevel(uint32_t targetLevel, Tile &tile) {
    uint32_t levelIndex = targetLevel;
    while (levels[levelIndex].freeHead == InvalidNode) {
        if (levelIndex == 0) {
            return false;
        }
        --levelIndex;
    }

    uint32_t nodeIndex = levels[levelIndex].freeHead;
    RemoveFreeNode(levelIndex, nodeIndex);

    while (levelIndex < targetLevel) {
        nodes[nodeIndex].state = NodeState::Split;
        const uint32_t firstChild = levels[levelIndex + 1].firstNode + (nodeIndex - levels[levelIndex].firstNode) * 4;
        ++levelIndex;
        for (uint32_t child = 4; child > 0; --child) {
            AddFreeNode(levelIndex, firstChild + child - 1);
        }
        nodeIndex = firstChild;
        RemoveFreeNode(levelIndex, nodeIndex);
    }

    Node &node = nodes[nodeIndex];
    node.state = NodeState::Allocated;
    if (++nextGeneration == 0) {
        ++nextGeneration;
    }

    node.generation = nextGeneration;
    tile.handle = {node.generation, nodeIndex};
    tile.resolution = levels[targetLevel].resolution;

    // 각 level 은 root 순서와 자식 사분면 순서로 배치된다. 노드마다 좌표를 저장하지 않는다.
    const uint32_t localIndex = nodeIndex - levels[targetLevel].firstNode;
    const uint32_t root = localIndex >> (targetLevel * 2);
    tile.x = (root % rootsPerRow) * levels[0].resolution;
    tile.y = (root / rootsPerRow) * levels[0].resolution;

    for (uint32_t depth = 1; depth <= targetLevel; ++depth) {
        const uint32_t quadrant = (localIndex >> ((targetLevel - depth) * 2)) & 3;
        tile.x += (quadrant & 1) * levels[depth].resolution;
        tile.y += (quadrant >> 1) * levels[depth].resolution;
    }

    ++allocatedTileCount;
    allocatedArea += uint64_t(tile.resolution) * tile.resolution;
    return true;
}

bool ShadowAtlasAllocator::Allocate(uint32_t requestedResolution, uint32_t tileCount, Tile *tiles) {
    if (!tiles || tileCount == 0) {
        return false;
    }

    std::fill_n(tiles, tileCount, Tile{});
    if (!nodes || requestedResolution < GetMinTileResolution() || requestedResolution > GetMaxTileResolution() ||
        !BE1::IsPowerOfTwo(static_cast<int>(requestedResolution))) {
        return false;
    }

    uint32_t firstLevel = 0;
    while (levels[firstLevel].resolution != requestedResolution) {
        ++firstLevel;
    }

    for (uint32_t levelIndex = firstLevel; levelIndex < levelCount; ++levelIndex) {
        const uint64_t resolution = levels[levelIndex].resolution;
        const uint64_t freeArea = uint64_t(atlasSize) * atlasSize - allocatedArea;
        if (tileCount > freeArea / (resolution * resolution)) {
            continue;
        }

        uint32_t allocated = 0;
        while (allocated < tileCount && AllocateAtLevel(levelIndex, tiles[allocated])) {
            ++allocated;
        }

        if (allocated == tileCount) {
            return true;
        }

        // Point 의 일부 면만 남지 않도록 이 시도에서 확보한 타일을 모두 반환한다.
        while (allocated > 0) {
            --allocated;
            const bool freed = Free(tiles[allocated].handle);
            assert(freed);
            (void)freed;
            tiles[allocated] = Tile{};
        }
    }
    return false;
}

bool ShadowAtlasAllocator::IsAllocated(Handle handle) const {
    return handle.IsValid() && handle.nodeIndex < nodeCount && nodes[handle.nodeIndex].state == NodeState::Allocated && nodes[handle.nodeIndex].generation == handle.generation;
}

bool ShadowAtlasAllocator::Free(Handle handle) {
    if (!IsAllocated(handle)) {
        return false;
    }

    uint32_t levelIndex = 0;
    while (levelIndex + 1 < levelCount && handle.nodeIndex >= levels[levelIndex + 1].firstNode) {
        ++levelIndex;
    }

    --allocatedTileCount;
    allocatedArea -= uint64_t(levels[levelIndex].resolution) * levels[levelIndex].resolution;

    uint32_t nodeIndex = handle.nodeIndex;
    while (levelIndex > 0) {
        nodes[nodeIndex].state = NodeState::Free;

        const uint32_t parentOffset = (nodeIndex - levels[levelIndex].firstNode) / 4;
        const uint32_t firstChild = levels[levelIndex].firstNode + parentOffset * 4;
        bool allFree = true;
        for (uint32_t child = 0; child < 4; ++child) {
            allFree &= nodes[firstChild + child].state == NodeState::Free;
        }

        if (!allFree) {
            break;
        }

        for (uint32_t child = 0; child < 4; ++child) {
            const uint32_t childIndex = firstChild + child;
            if (childIndex != nodeIndex) {
                RemoveFreeNode(levelIndex, childIndex);
            }
            nodes[childIndex].state = NodeState::Inactive;
        }
        nodeIndex = levels[--levelIndex].firstNode + parentOffset;
        assert(nodes[nodeIndex].state == NodeState::Split);
    }

    AddFreeNode(levelIndex, nodeIndex);
    return true;
}
