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
#include "Math/Vector3.h"
#include "Shaders/GeometryClipmapInterop.h"

namespace BE1 {
    class Frustum;
    class OBB;
}

/*
---------------------------------------------------------------------------------------------------

    Geometry Clipmap

    XY plane 의 geometry 만 소유한다. 높이/변위 source 와 material 은 소비자가 제공한다.

---------------------------------------------------------------------------------------------------
*/

class GeometryClipmap {
public:
    static constexpr uint32_t   MinPatchCellCount = 16;
    static constexpr uint32_t   MaxPatchCellCount = 128;
    static constexpr uint32_t   MaxLevelCount = 12;
    static constexpr uint32_t   ShapeCount = 5;
    static constexpr uint32_t   MaxPatchCount = 16 + 18 * (MaxLevelCount - 1);

    struct Settings {
        float                   baseSpacing = 1.0f;
        uint32_t                patchCellCount = 64;    ///< 한 level 의 한 변은 4 * patchCellCount cells 이다.
        uint32_t                levelCount = 8;
        float                   morphWidth = 0.5f;      ///< patchCellCount 에 대한 전환 폭의 비율 (4 .. patchCellCount-6 cells)

        bool                    IsValid() const;
    };

    struct View {
        BE1::Vec3               origin = BE1::Vec3::origin;
        const BE1::Frustum *    frustum = nullptr;
        const BE1::OBB *        box = nullptr;
        float                   minHeight = 0.0f;
        float                   maxHeight = 0.0f;
        float                   maxHorizontalDisplacement = 0.0f;
        bool                    frustumCulling = true;
        bool                    infiniteZFar = false;
    };

    struct DrawRange {
        uint32_t                firstIndex;
        uint32_t                indexCount;
        uint32_t                instanceOffset;
        uint32_t                instanceCount;
    };

    struct Batch {
        GeometryClipmapPatch    instances[MaxPatchCount];
        DrawRange               draws[ShapeCount];
        uint32_t                instanceCount;
        uint64_t                triangleCount;
    };

                                GeometryClipmap() = default;
                                GeometryClipmap(const GeometryClipmap &) = delete;
    GeometryClipmap &           operator=(const GeometryClipmap &) = delete;

    bool                        Init();
    void                        Shutdown();
    bool                        IsReady() const { return indexBuffer != nullptr; }
    RHI::IndexBuffer *          GetIndexBuffer() const { return indexBuffer; }
    bool                        BuildBatch(const Settings &settings, const View &view, Batch &batch) const;

    static uint32_t             GetResolutionIndex(uint32_t patchCellCount);

private:
    static constexpr uint32_t   ResolutionCount = 4;

    struct IndexRange {
        uint32_t                firstIndex = 0;
        uint32_t                indexCount = 0;
    };

    RHI::IndexBuffer *          indexBuffer = nullptr;
    IndexRange                  indexRanges[ResolutionCount][ShapeCount] = {};
};
