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
#include "ShadowResolutionHistory.h"
#include "Render/RenderFrameSettings.h"
#include <algorithm>
#include <cassert>

void ShadowResolutionHistory::BeginView() {
    if (++viewIndex == 0) {
        Reset();
        ++viewIndex;
    }
}

void ShadowResolutionHistory::Reset() {
    entries.SetCount(0, false);
    viewIndex = 0;
}

uint32_t ShadowResolutionHistory::Update(uint32_t lightIndex, uint64_t lightId, uint32_t maxResolution, float scaledProjectedSize) {
    assert(viewIndex != 0 && lightId != 0);

    if (lightIndex >= static_cast<uint32_t>(entries.Count())) {
        const int oldCount = entries.Count();
        const int newCount = static_cast<int>(lightIndex) + 1;

        if (newCount > entries.Capacity()) {
            // 연속된 light index 추가 시 매번 재할당하지 않는다. 새 메모리의 항목은 기본 생성자로 초기화된다.
            entries.Reserve(std::max(newCount, entries.Capacity() * 2));
            entries.SetCount(newCount, false);
        } else {
            entries.SetCount(newCount, false);

            // 재사용하는 entiry 를 초기화한다.
            std::fill(entries.Ptr() + oldCount, entries.Ptr() + newCount, Entry{});
        }
    }

    Entry &entry = entries[lightIndex];
    // light index 가 재사용되거나 이전 뷰에서 shadow 후보가 아니었으면 새 요청으로 시작한다.
    const bool valid = entry.lightId == lightId && (entry.viewIndex == viewIndex || entry.viewIndex + 1 == viewIndex);
    entry.resolution = SelectResolution(valid ? entry.resolution : 0, maxResolution, scaledProjectedSize);
    entry.lightId = lightId;
    entry.viewIndex = viewIndex;
    return entry.resolution;
}

uint32_t ShadowResolutionHistory::SelectResolution(uint32_t previousResolution, uint32_t maxResolution, float scaledProjectedSize) {
    constexpr uint32_t minResolution = RenderFrameSettings::ShadowAtlasMinTileResolution;
    assert(maxResolution >= minResolution && (maxResolution & (maxResolution - 1)) == 0);

    // 초기 해상도 선택과 히스테리시스 판정에 같은 배율을 적용하며, 소수 부분도 유지한다.
    if (previousResolution == 0) {
        uint32_t resolution = minResolution;
        while (resolution < maxResolution && resolution < scaledProjectedSize) {
            resolution *= 2;
        }
        return resolution;
    }

    uint32_t resolution = std::clamp(previousResolution, minResolution, maxResolution);

    // 필요 크기가 현재 해상도에서 112.5% (+12.5%) 를 초과하면 두 배로 늘린다.
    while (resolution < maxResolution && scaledProjectedSize > resolution + resolution / 8) {
        resolution *= 2;
    }
    // 필요 크기가 현재 해상도의 절반에서 87.5% (-12.5%) 이하면 절반으로 줄인다.
    while (resolution > minResolution && scaledProjectedSize <= resolution / 2 - resolution / 16) {
        resolution /= 2;
    }
    return resolution;
}
