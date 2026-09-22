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

#ifdef ENABLE_SHADOW_ATLAS_ALLOCATOR_TESTS

#include "ShadowAtlasAllocatorTests.h"
#include "ShadowAtlasAllocator.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

namespace {

[[noreturn]] void FailTest(const char *expression, const char *file, int line) {
    char message[1024];
    std::snprintf(message, sizeof(message), "%s:%d: %s\n", file, line, expression);
    OutputDebugStringA(message);
    std::fputs(message, stderr);
    std::exit(1);
}

#define CHECK(expression) do { if (!(expression)) { FailTest(#expression, __FILE__, __LINE__); } } while (false)

using Allocator = ShadowAtlasAllocator;
using Tile = Allocator::Tile;
static constexpr uint32_t MinResolution = 64;

// 할당기의 트리/목록 구현과 독립적으로 최소 타일 격자에서 겹침과 해제 영역을 검증한다.
class Occupancy {
public:
    explicit Occupancy(uint32_t size) : side(size / MinResolution), cells(side * side, false) {}

    void Set(const Tile &tile, bool occupied) {
        CHECK(tile.resolution >= MinResolution);
        CHECK((tile.resolution & (tile.resolution - 1)) == 0);
        CHECK(tile.x % tile.resolution == 0 && tile.y % tile.resolution == 0);
        CHECK(tile.x + tile.resolution <= side * MinResolution);
        CHECK(tile.y + tile.resolution <= side * MinResolution);
        for (uint32_t y = tile.y / MinResolution; y < (tile.y + tile.resolution) / MinResolution; ++y) {
            for (uint32_t x = tile.x / MinResolution; x < (tile.x + tile.resolution) / MinResolution; ++x) {
                const uint32_t index = y * side + x;
                CHECK(cells[index] != occupied);
                cells[index] = occupied;
            }
        }
    }

private:
    uint32_t side;
    std::vector<bool> cells;
};

static void TestCapacityAndCoalescing() {
    for (uint32_t size = 1024; size <= 8192; size *= 2) {
        for (uint32_t maxResolution = MinResolution; maxResolution <= 1024; maxResolution *= 2) {
            Allocator allocator;
            CHECK(allocator.Init(size, maxResolution, MinResolution));
            Occupancy occupancy(size);
            std::vector<Tile> tiles((size / MinResolution) * (size / MinResolution));
            CHECK(allocator.Allocate(MinResolution, static_cast<uint32_t>(tiles.size()), tiles.data()));
            CHECK(allocator.GetAllocatedArea() == uint64_t(size) * size);
            for (const Tile &tile : tiles) {
                CHECK(allocator.IsAllocated(tile.handle));
                occupancy.Set(tile, true);
            }
            Tile extra;
            CHECK(!allocator.Allocate(maxResolution, 1, &extra));
            CHECK(!extra.handle.IsValid());

            std::mt19937 random(1234);
            // 비순차 해제에서도 모든 부모가 병합되어 최대 크기 블록으로 회수되는지 확인한다.
            for (size_t count = tiles.size(); count > 0; --count) {
                const size_t index = random() % count;
                occupancy.Set(tiles[index], false);
                CHECK(allocator.Free(tiles[index].handle));
                CHECK(!allocator.Free(tiles[index].handle));
                tiles[index] = tiles[count - 1];
            }
            CHECK(allocator.GetAllocatedArea() == 0);
            CHECK(allocator.GetAllocatedTileCount() == 0);

            const uint32_t rootCount = (size / maxResolution) * (size / maxResolution);
            CHECK(allocator.Allocate(maxResolution, rootCount, tiles.data()));
            for (uint32_t i = 0; i < rootCount; ++i) {
                CHECK(tiles[i].resolution == maxResolution);
                occupancy.Set(tiles[i], true);
            }
        }
    }
}

static void TestPointFallbackAndRollback() {
    Allocator allocator;
    CHECK(allocator.Init(1024, 1024, MinResolution));
    Occupancy occupancy(1024);
    std::array<Tile, 256> cells;
    CHECK(allocator.Allocate(MinResolution, static_cast<uint32_t>(cells.size()), cells.data()));

    // 16개의 256 영역 중 11개에 64 타일을 하나씩 남긴다. 면적은 충분하지만
    // 256 타일은 5개뿐이어서 6개 묶음 할당 도중 rollback 이 필요하다.
    uint32_t blockers = 0;
    for (const Tile &tile : cells) {
        if (tile.x % 256 == 0 && tile.y % 256 == 0 && blockers < 11) {
            occupancy.Set(tile, true);
            ++blockers;
        } else {
            CHECK(allocator.Free(tile.handle));
        }
    }
    CHECK(blockers == 11);
    std::array<Tile, 6> faces;
    CHECK(allocator.Allocate(256, static_cast<uint32_t>(faces.size()), faces.data()));
    CHECK(allocator.GetAllocatedTileCount() == blockers + faces.size());
    CHECK(allocator.GetAllocatedArea() == uint64_t(blockers) * 64 * 64 + 6 * 128 * 128);
    for (const Tile &face : faces) {
        CHECK(face.resolution == 128);
        occupancy.Set(face, true);
    }
    for (const Tile &face : faces) {
        occupancy.Set(face, false);
        CHECK(allocator.Free(face.handle));
    }

    allocator.Reset();
    CHECK(allocator.Allocate(1024, 6, faces.data()));
    for (const Tile &face : faces) {
        CHECK(face.resolution == 256);
    }

    allocator.Reset();
    CHECK(allocator.Allocate(64, 251, cells.data()));
    const uint64_t oldArea = allocator.GetAllocatedArea();
    CHECK(!allocator.Allocate(1024, 6, faces.data()));
    CHECK(allocator.GetAllocatedTileCount() == 251);
    CHECK(allocator.GetAllocatedArea() == oldArea);
    for (const Tile &face : faces) {
        CHECK(!face.handle.IsValid() && face.resolution == 0);
    }
    for (uint32_t i = 0; i < 251; ++i) {
        CHECK(allocator.IsAllocated(cells[i].handle));
    }
    CHECK(allocator.Allocate(64, 4, cells.data() + 251));
    CHECK(allocator.Allocate(1024, 1, faces.data()));
    CHECK(faces[0].resolution == MinResolution);
}

static void TestHandlesAndReconfiguration() {
    Allocator allocator;
    Tile tile;
    CHECK(!allocator.Allocate(64, 1, &tile));
    CHECK(!allocator.Free({}));
    CHECK(allocator.Init(1024, 1024, MinResolution));
    CHECK(allocator.Allocate(1024, 1, &tile));
    Allocator::Handle old = tile.handle;
    CHECK(allocator.Free(old));
    CHECK(allocator.Allocate(1024, 1, &tile));
    CHECK(tile.handle.nodeIndex == old.nodeIndex);
    CHECK(!allocator.Free(old));

    old = tile.handle;
    CHECK(!allocator.Init(1000, 1024, MinResolution));
    CHECK(!allocator.Init(1024, 2048, MinResolution));
    CHECK(!allocator.Init(1024, 32, MinResolution));
    CHECK(!allocator.Init(1024, 1024, 0));
    CHECK(!allocator.Init(1u << 31, 1, 1));
    CHECK(allocator.IsAllocated(old));
    CHECK(allocator.GetSize() == 1024);
    CHECK(!allocator.Allocate(0, 1, &tile));
    CHECK(!allocator.Allocate(63, 1, &tile));
    CHECK(!allocator.Allocate(96, 1, &tile));
    CHECK(!allocator.Allocate(2048, 1, &tile));
    CHECK(!allocator.Allocate(64, 0, &tile));
    CHECK(!allocator.Allocate(64, 1, nullptr));

    allocator.Reset();
    CHECK(allocator.Allocate(1024, 1, &tile));
    CHECK(!allocator.IsAllocated(old));
    old = tile.handle;
    CHECK(allocator.Init(2048, 512, MinResolution));
    CHECK(allocator.GetSize() == 2048);
    CHECK(allocator.GetMaxTileResolution() == 512);
    CHECK(allocator.Allocate(512, 1, &tile));
    CHECK(!allocator.IsAllocated(old));
    old = tile.handle;
    allocator.Shutdown();
    CHECK(!allocator.IsAllocated(old));
    CHECK(allocator.GetSize() == 0);
    CHECK(allocator.Init(2048, 512, MinResolution));
    CHECK(allocator.Allocate(512, 1, &tile));
    CHECK(!allocator.Free(old));
    old = tile.handle;
    old.nodeIndex = Allocator::InvalidNode - 1;
    CHECK(!allocator.Free(old));
}

static void TestRandomizedOccupancy() {
    Allocator allocator;
    CHECK(allocator.Init(2048, 1024, MinResolution));
    Occupancy occupancy(2048);
    std::vector<Tile> live;
    std::mt19937 random(0x53484144);
    uint64_t area = 0;
    for (uint32_t iteration = 0; iteration < 40000; ++iteration) {
        if (!live.empty() && random() % 2 == 0) {
            const size_t index = random() % live.size();
            const Tile tile = live[index];
            occupancy.Set(tile, false);
            CHECK(allocator.Free(tile.handle));
            CHECK(!allocator.IsAllocated(tile.handle));
            area -= uint64_t(tile.resolution) * tile.resolution;
            live[index] = live.back();
            live.pop_back();
        } else {
            std::array<Tile, 6> tiles;
            const uint32_t count = random() % 2 == 0 ? 1 : 6;
            const uint32_t requested = MinResolution << (random() % 5);
            if (allocator.Allocate(requested, count, tiles.data())) {
                for (uint32_t i = 0; i < count; ++i) {
                    CHECK(tiles[i].resolution == tiles[0].resolution && tiles[i].resolution <= requested);
                    occupancy.Set(tiles[i], true);
                    area += uint64_t(tiles[i].resolution) * tiles[i].resolution;
                    live.push_back(tiles[i]);
                }
            } else {
                for (uint32_t i = 0; i < count; ++i) {
                    CHECK(!tiles[i].handle.IsValid());
                }
            }
        }
        CHECK(allocator.GetAllocatedArea() == area);
        CHECK(allocator.GetAllocatedTileCount() == live.size());
        for (const Tile &tile : live) {
            CHECK(allocator.IsAllocated(tile.handle));
        }
    }
    for (const Tile &tile : live) {
        CHECK(allocator.Free(tile.handle));
    }
    std::array<Tile, 4> roots;
    CHECK(allocator.Allocate(1024, 4, roots.data()));
    for (const Tile &root : roots) {
        CHECK(root.resolution == 1024);
    }
}

} // namespace

#undef CHECK

int RunShadowAtlasAllocatorTests() {
#ifdef _MSC_VER
    // 자동 테스트에서 assertion 실패가 대화상자를 띄우며 멈추지 않도록 한다.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    TestCapacityAndCoalescing();
    TestPointFallbackAndRollback();
    TestHandlesAndReconfiguration();
    TestRandomizedOccupancy();
    const char *message = "ShadowAtlasAllocator: capacity, coalescing, group rollback, handles and 40000 randomized operations passed.\n";
    OutputDebugStringA(message);
    std::fputs(message, stdout);
    return 0;
}

#endif // ENABLE_SHADOW_ATLAS_ALLOCATOR_TESTS
