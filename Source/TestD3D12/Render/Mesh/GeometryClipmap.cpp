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
#include "GeometryClipmap.h"
#include "Math/AABB.h"
#include "Math/OBB.h"
#include "Math/Frustum.h"
#include <cassert>
#include <cmath>

struct ClipmapPatch {
    int32_t     x;
    int32_t     y;
    uint32_t    shape;
    uint32_t    level;
    float       spacing;
    float       morphStart;
    float       morphScale;
    uint32_t    packedBounds;   ///< outer edge mask (L/R/B/T), width << 8, height << 16
};

static void GetShapeSize(uint32_t patchCellCount, uint32_t shape, uint32_t &width, uint32_t &height) {
    assert(shape < GeometryClipmap::ShapeCount);
    width = shape == 1 ? 1 : (shape == 2 ? patchCellCount - 1 : patchCellCount);
    height = shape == 3 ? 1 : (shape == 4 ? patchCellCount - 1 : patchCellCount);
}

static uint32_t GetIndexCount(uint32_t patchCellCount, uint32_t shape) {
    uint32_t width, height;
    GetShapeSize(patchCellCount, shape, width, height);
    return width * height * 6;
}

static void WriteIndices(uint32_t patchCellCount, uint32_t shape, uint16_t *indices) {
    uint32_t width, height;
    GetShapeSize(patchCellCount, shape, width, height);
    const uint32_t stride = patchCellCount + 1;
    static_assert((GeometryClipmap::MaxPatchCellCount + 1) * (GeometryClipmap::MaxPatchCellCount + 1) <= 65536);

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const uint16_t bottomLeft = static_cast<uint16_t>(y * stride + x);
            const uint16_t bottomRight = bottomLeft + 1;
            const uint16_t topLeft = static_cast<uint16_t>(bottomLeft + stride);
            const uint16_t topRight = topLeft + 1;

            *indices++ = bottomLeft;
            *indices++ = bottomRight;
            *indices++ = topLeft;
            *indices++ = topLeft;
            *indices++ = bottomRight;
            *indices++ = topRight;
        }
    }
}

// callback 은 patch 를 즉시 소비한다. 매 view 마다 heap allocation/cache mutation 이 필요 없다.
template <typename EmitPatch>
static bool BuildPatches(const GeometryClipmap::Settings &settings, float cameraX, float cameraY, EmitPatch &&emitPatch) {
    if (!settings.IsValid() || !std::isfinite(cameraX) || !std::isfinite(cameraY)) {
        return false;
    }

    // int 좌표 변환의 overflow 와 HLSL float 정수 정밀도 손실을 방지한다.
    // 이 범위 밖의 world 는 engine 차원의 floating origin 이 필요하다.
    constexpr double maxGridCoordinate = 8388608.0 - 4 * GeometryClipmap::MaxPatchCellCount;
    if (std::abs(static_cast<double>(cameraX) / settings.baseSpacing) > maxGridCoordinate ||
        std::abs(static_cast<double>(cameraY) / settings.baseSpacing) > maxGridCoordinate) {
        return false;
    }

    const int32_t cells = static_cast<int32_t>(settings.patchCellCount);

    // parent 한 칸(2 cells)에서 가중치가 0 -> 1 로 바뀌면 코너 삼각형이 일찍 납작해질 수 있다.
    const float morphCells = std::fmin(static_cast<float>(cells - 6), std::fmax(4.0f, settings.morphWidth * static_cast<float>(cells)));
    int32_t previousMinX = 0, previousMinY = 0;

    for (uint32_t level = 0; level < settings.levelCount; ++level) {
        const float spacing = settings.baseSpacing * static_cast<float>(1u << level);
        // parent 격자에 정렬한다. 음수에서도 truncation 대신 floor 를 사용한다.
        const int32_t centerX = static_cast<int32_t>(std::floor(static_cast<double>(cameraX) / (2.0 * spacing))) * 2;
        const int32_t centerY = static_cast<int32_t>(std::floor(static_cast<double>(cameraY) / (2.0 * spacing))) * 2;
        const int32_t minX = centerX - 2 * cells;
        const int32_t minY = centerY - 2 * cells;
        const int32_t maxX = centerX + 2 * cells;
        const int32_t maxY = centerY + 2 * cells;

        // 스냅으로 추가/제거되는 2-cell 띠는 항상 완전히 parent 형상이다.
        // 폭의 상한으로 parent 의 inner 경계와 morph 시작 사이에도 여유를 남긴다.
        const float morphEnd = static_cast<float>(2 * cells - 2) * spacing;
        const float morphStart = morphEnd - morphCells * spacing;
        const float morphScale = level + 1 < settings.levelCount ? 1.0f / (morphCells * spacing) : 0.0f;

        auto EmitRectangle = [&](int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
            for (int32_t y = y0; y < y1; y += cells) {
                const int32_t height = y1 - y < cells ? y1 - y : cells;

                for (int32_t x = x0; x < x1; x += cells) {
                    const int32_t width = x1 - x < cells ? x1 - x : cells;
                    assert((width == cells && (height == cells || height == 1 || height == cells - 1)) || (height == cells && (width == 1 || width == cells - 1)));
                    const uint32_t shape = width == 1 ? 1 : (width == cells - 1 ? 2 : (height == 1 ? 3 : (height == cells - 1 ? 4 : 0)));
                    const uint32_t edges = level + 1 < settings.levelCount ? ((x == minX ? 1u : 0u) | (x + width == maxX ? 2u : 0u) | (y == minY ? 4u : 0u) | (y + height == maxY ? 8u : 0u)) : 0u;
                    const uint32_t packedBounds = edges | (static_cast<uint32_t>(width) << 8) | (static_cast<uint32_t>(height) << 16);
                    emitPatch(ClipmapPatch{x, y, shape, level, spacing, morphStart, morphScale, packedBounds});
                }
            }
        };

        if (level == 0) {
            EmitRectangle(minX, minY, maxX, maxY);
        } else {
            // finer level 의 경계는 현재 격자에 정확히 놓인다. 0/1-cell 상대 오프셋을
            // 네 직사각형으로 채워 hole/overlap 없이 ring 과 trim 을 함께 구성한다.
            const int32_t innerMinX = previousMinX / 2;
            const int32_t innerMinY = previousMinY / 2;
            const int32_t innerMaxX = innerMinX + 2 * cells;
            const int32_t innerMaxY = innerMinY + 2 * cells;
            EmitRectangle(minX, minY, innerMinX, maxY);
            EmitRectangle(innerMaxX, minY, maxX, maxY);
            EmitRectangle(innerMinX, minY, innerMaxX, innerMinY);
            EmitRectangle(innerMinX, innerMaxY, innerMaxX, maxY);
        }
        previousMinX = minX;
        previousMinY = minY;
    }
    return true;
}

bool GeometryClipmap::Settings::IsValid() const {
    return std::isfinite(baseSpacing) && baseSpacing > 0.0f && std::isfinite(1.0f / baseSpacing) &&
        patchCellCount >= MinPatchCellCount && patchCellCount <= MaxPatchCellCount &&
        (patchCellCount & (patchCellCount - 1)) == 0 &&
        levelCount > 0 && levelCount <= MaxLevelCount &&
        std::isfinite(morphWidth) && morphWidth >= 0.05f && morphWidth <= 0.75f &&
        std::isfinite(baseSpacing * static_cast<float>(4u * patchCellCount * (1u << (levelCount - 1))));
}

uint32_t GeometryClipmap::GetResolutionIndex(uint32_t patchCellCount) {
    uint32_t index = 0;
    while (patchCellCount > MinPatchCellCount) {
        patchCellCount >>= 1;
        ++index;
    }
    return index;
}

bool GeometryClipmap::Init() {
    if (indexBuffer) {
        return true;
    }

    uint32_t totalIndexCount = 0;
    for (uint32_t resolution = 0; resolution < ResolutionCount; ++resolution) {
        const uint32_t cells = MinPatchCellCount << resolution;
        for (uint32_t shape = 0; shape < ShapeCount; ++shape) {
            IndexRange &range = indexRanges[resolution][shape];
            range.firstIndex = totalIndexCount;
            range.indexCount = GetIndexCount(cells, shape);
            totalIndexCount += range.indexCount;
        }
    }

    BE1::Array<uint16_t> indices;
    indices.SetCount(totalIndexCount);
    for (uint32_t resolution = 0; resolution < ResolutionCount; ++resolution) {
        for (uint32_t shape = 0; shape < ShapeCount; ++shape) {
            WriteIndices(MinPatchCellCount << resolution,
                shape, indices.Ptr() + indexRanges[resolution][shape].firstIndex);
        }
    }

    indexBuffer = RHI::renderer->CreateIndexBuffer(RHI::BufferUsage::Default, sizeof(uint16_t), totalIndexCount, indices.Ptr());
    if (!indexBuffer) {
        BE_WARNLOG("GeometryClipmap::Init: failed to create clipmap index buffer\n");
        return false;
    }
    RHI::renderer->SetName(indexBuffer, "GeometryClipmap::indexBuffer");
    return true;
}

void GeometryClipmap::Shutdown() {
    if (indexBuffer) {
        RHI::renderer->DestroyIndexBuffer(indexBuffer);
        indexBuffer = nullptr;
    }
}

bool GeometryClipmap::BuildBatch(const Settings &settings, const View &view, Batch &batch) const {
    batch.instanceCount = 0;
    batch.triangleCount = 0;

    for (DrawRange &draw : batch.draws) {
        draw = {};
    }

    const bool hasFrustum = view.frustum != nullptr;
    if (!IsReady() || !settings.IsValid() || (view.frustumCulling && hasFrustum == (view.box != nullptr)) ||
        !std::isfinite(view.origin.z) || !std::isfinite(view.minHeight) || !std::isfinite(view.maxHeight) ||
        view.minHeight > view.maxHeight || !std::isfinite(view.maxHorizontalDisplacement) || view.maxHorizontalDisplacement < 0.0f) {
        return false;
    }

    BE1::Frustum expandedFrustum;
    const BE1::Frustum *frustum = view.frustum;
    if (view.frustumCulling && hasFrustum && view.infiniteZFar) {
        expandedFrustum = *frustum;
        // Infinite-far 투영의 유한 CPU far plane 을 전체 clipmap 과 변위 범위까지 확장한다.
        const float spacing = settings.baseSpacing * static_cast<float>(1u << (settings.levelCount - 1));
        const float maxXY = static_cast<float>(2 * settings.patchCellCount + 3) * spacing + view.maxHorizontalDisplacement;
        const float maxZ = BE1::Max(BE1::Math::Abs(view.minHeight - view.origin.z), BE1::Math::Abs(view.maxHeight - view.origin.z));
        const float requiredFar = BE1::Math::Sqrt(2.0f * maxXY * maxXY + maxZ * maxZ);
        if (requiredFar > expandedFrustum.GetFarDistance()) {
            expandedFrustum.MoveFarDistance(requiredFar);
        }
        frustum = &expandedFrustum;
    }

    // 먼저 한 번만 cull 하고, shape 별 draw 를 위해 살아남은 작은 instance 목록을 compact 한다.
    GeometryClipmapPatch visiblePatches[MaxPatchCount];
    uint32_t visibleShapes[MaxPatchCount];

    const bool built = BuildPatches(settings, view.origin.x, view.origin.y, [&](const ClipmapPatch &patch) {
        if (view.frustumCulling) {
            uint32_t width, height;
            GetShapeSize(settings.patchCellCount, patch.shape, width, height);
            // morph 는 각 축의 음의 방향으로 최대 한 cell 이동한다. 소비자의 최대 변위도 포함한다.
            const float displacement = view.maxHorizontalDisplacement;
            const BE1::AABB bounds(
                BE1::Vec3(static_cast<float>(patch.x - 1) * patch.spacing - displacement,
                    static_cast<float>(patch.y - 1) * patch.spacing - displacement, view.minHeight),
                BE1::Vec3(static_cast<float>(patch.x + static_cast<int32_t>(width)) * patch.spacing + displacement,
                    static_cast<float>(patch.y + static_cast<int32_t>(height)) * patch.spacing + displacement, view.maxHeight));
            if (hasFrustum ? frustum->CullAABB(bounds) : !view.box->IsIntersectAABB(bounds)) {
                return;
            }
        }
        assert(batch.instanceCount < MaxPatchCount);
        GeometryClipmapPatch &instance = visiblePatches[batch.instanceCount];
        instance.gridOrigin = int2(patch.x, patch.y);
        instance.gridSpacing = patch.spacing;
        instance.level = patch.level;
        instance.morphStart = patch.morphStart;
        instance.morphScale = patch.morphScale;
        instance.vertexResolution = settings.patchCellCount + 1;
        instance.packedBounds = patch.packedBounds;
        visibleShapes[batch.instanceCount++] = patch.shape;
        ++batch.draws[patch.shape].instanceCount;
    });

    if (!built) {
        return false;
    }

    uint32_t instanceOffset = 0;
    uint32_t writeOffsets[ShapeCount];
    const uint32_t resolution = GetResolutionIndex(settings.patchCellCount);
    for (uint32_t shape = 0; shape < ShapeCount; ++shape) {
        DrawRange &draw = batch.draws[shape];
        const IndexRange &range = indexRanges[resolution][shape];
        draw.firstIndex = range.firstIndex;
        draw.indexCount = range.indexCount;
        draw.instanceOffset = instanceOffset;
        writeOffsets[shape] = instanceOffset;
        instanceOffset += draw.instanceCount;
        batch.triangleCount += static_cast<uint64_t>(draw.instanceCount) * (draw.indexCount / 3);
    }

    for (uint32_t index = 0; index < batch.instanceCount; ++index) {
        batch.instances[writeOffsets[visibleShapes[index]]++] = visiblePatches[index];
    }
    return true;
}
