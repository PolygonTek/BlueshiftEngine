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

#include <stdint.h>

enum class ShaderFeature : uint8_t {
    Perforated,
    GpuSkinningWeight1,
    GpuSkinningWeight4,
    GpuSkinningWeight8,
    OutputNormal,
    OutputVelocity,
    DirectLighting,
    IndirectLighting,
    Count
};

struct ShaderFeatureSet {
    enum Flag : uint32_t {
        Perforated                  = 1u << 0,
        GpuSkinningWeight1          = 1u << 1,
        GpuSkinningWeight4          = 1u << 2,
        GpuSkinningWeight8          = 1u << 3,
        OutputNormal                = 1u << 4,
        OutputVelocity              = 1u << 5,
        DirectLighting              = 1u << 6,
        IndirectLighting            = 1u << 7,
        GpuSkinningWeightMask       = GpuSkinningWeight1 | GpuSkinningWeight4 | GpuSkinningWeight8
    };

    bool                            IsEmpty() const { return flags == 0; }
    bool                            HasFeature(ShaderFeature feature) const;
    void                            EnableFeature(ShaderFeature feature);

    bool                            operator==(const ShaderFeatureSet &rhs) const { return flags == rhs.flags; }
    bool                            operator!=(const ShaderFeatureSet &rhs) const { return flags != rhs.flags; }

    uint32_t                        flags = 0;
};

BE_INLINE ShaderFeatureSet operator|(const ShaderFeatureSet &lhs, const ShaderFeatureSet &rhs) {
    const uint32_t lhsSkinning = lhs.flags & ShaderFeatureSet::GpuSkinningWeightMask;
    const uint32_t rhsSkinning = rhs.flags & ShaderFeatureSet::GpuSkinningWeightMask;
    assert(lhsSkinning == 0 || rhsSkinning == 0 || lhsSkinning == rhsSkinning);

    ShaderFeatureSet result;
    result.flags = lhs.flags | rhs.flags;
    return result;
}

BE_INLINE ShaderFeatureSet &operator|=(ShaderFeatureSet &lhs, const ShaderFeatureSet &rhs) {
    lhs = lhs | rhs;
    return lhs;
}
