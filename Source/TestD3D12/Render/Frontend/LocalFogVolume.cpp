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
#include "LocalFogVolume.h"

bool LocalFogVolume::Desc::IsValid() const {
    assert(boundaryFade >= 0.0f && boundaryFade <= 1.0f);
    assert(heightFogFalloff >= 0.0f && heightFogFalloff <= 200.0f);

    if (!(medium.baseExtinction >= 0.0f && medium.baseExtinction <= 1000.0f) ||
        !(medium.scatteringAlbedo.x >= 0.0f && medium.scatteringAlbedo.x <= 1.0f) ||
        !(medium.scatteringAlbedo.y >= 0.0f && medium.scatteringAlbedo.y <= 1.0f) ||
        !(medium.scatteringAlbedo.z >= 0.0f && medium.scatteringAlbedo.z <= 1.0f) ||
        !(medium.anisotropy >= -0.9f && medium.anisotropy <= 0.9f)) {
        return false;
    }

    switch (shape) {
    case Shape::Sphere:
        return radius > 0.0f;
    case Shape::Box:
        return halfExtents.x > 0.0f && halfExtents.y > 0.0f && halfExtents.z > 0.0f;
    default:
        return false;
    }
}

bool LocalFogVolume::Desc::IsActive() const {
    return enabled && (medium.baseExtinction > 0.0f || medium.baseEmission.x > 0.0f || medium.baseEmission.y > 0.0f || medium.baseEmission.z > 0.0f);
}

bool LocalFogVolume::Update(const Desc &desc) {
    const bool boundsChanged = worldAABB.IsCleared() || this->desc.shape != desc.shape || this->desc.origin != desc.origin ||
        (desc.shape == Shape::Sphere ? this->desc.radius != desc.radius : (this->desc.halfExtents != desc.halfExtents || this->desc.axis != desc.axis));

    if (boundsChanged) {
        const BE1::AABB bounds = desc.shape == Shape::Sphere ?
            BE1::Sphere(desc.origin, desc.radius).ToAABB() : BE1::OBB(desc.origin, desc.halfExtents, desc.axis).ToAABB();

        // 유한한 입력끼리의 연산도 오버플로할 수 있다.
        if (!bounds[0].IsFinite() || !bounds[1].IsFinite()) {
            return false;
        }
        worldAABB = bounds;
    }

    if (this->desc.maskAddressMode != desc.maskAddressMode) {
        maskScrollOffset[0] = 0.0f;
        maskScrollOffset[1] = 0.0f;
        maskScrollOffset[2] = 0.0f;
    }

    this->desc = desc;
    return true;
}

void LocalFogVolume::AdvanceMaskScroll(float frameTime, uint64_t frameIndex) {
    assert(frameIndex > 0);

    if (maskScrollFrameIndex == frameIndex) {
        return;
    }
    maskScrollFrameIndex = frameIndex;

    if (desc.maskAddressMode != MaskAddressMode::Repeat || !desc.maskTexture || frameTime <= 0.0f) {
        return;
    }

    for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
        const float speed = desc.maskScrollSpeed[axisIndex];
        if (speed == 0.0f) {
            continue;
        }

        const float scrollOffset = maskScrollOffset[axisIndex] + speed * frameTime;
        const float wrappedOffset = scrollOffset - BE1::Math::Floor(scrollOffset);
        maskScrollOffset[axisIndex] = wrappedOffset < 1.0f ? wrappedOffset : 0.0f;
    }
}
