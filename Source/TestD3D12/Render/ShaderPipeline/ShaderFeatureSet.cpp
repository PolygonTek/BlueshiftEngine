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
#include "ShaderFeatureSet.h"

bool ShaderFeatureSet::HasFeature(ShaderFeature feature) const {
    switch (feature) {
    case ShaderFeature::Perforated:
        return !!(flags & Perforated);
    case ShaderFeature::GpuSkinningWeight1:
        return !!(flags & GpuSkinningWeight1);
    case ShaderFeature::GpuSkinningWeight4:
        return !!(flags & GpuSkinningWeight4);
    case ShaderFeature::GpuSkinningWeight8:
        return !!(flags & GpuSkinningWeight8);
    case ShaderFeature::OutputNormal:
        return !!(flags & OutputNormal);
    case ShaderFeature::OutputVelocity:
        return !!(flags & OutputVelocity);
    case ShaderFeature::OutputRoughness:
        return !!(flags & OutputRoughness);
    case ShaderFeature::TransparentSSRPrepass:
        return !!(flags & TransparentSSRPrepass);
    case ShaderFeature::DirectLighting:
        return !!(flags & DirectLighting);
    case ShaderFeature::IndirectLighting:
        return !!(flags & IndirectLighting);
    default:
        return false;
    }
}

void ShaderFeatureSet::EnableFeature(ShaderFeature feature) {
    switch (feature) {
    case ShaderFeature::Perforated:
        flags |= Perforated;
        break;
    case ShaderFeature::GpuSkinningWeight1:
        flags = (flags & ~GpuSkinningWeightMask) | GpuSkinningWeight1;
        break;
    case ShaderFeature::GpuSkinningWeight4:
        flags = (flags & ~GpuSkinningWeightMask) | GpuSkinningWeight4;
        break;
    case ShaderFeature::GpuSkinningWeight8:
        flags = (flags & ~GpuSkinningWeightMask) | GpuSkinningWeight8;
        break;
    case ShaderFeature::OutputNormal:
        flags |= OutputNormal;
        break;
    case ShaderFeature::OutputVelocity:
        flags |= OutputVelocity;
        break;
    case ShaderFeature::OutputRoughness:
        flags |= OutputRoughness;
        break;
    case ShaderFeature::TransparentSSRPrepass:
        flags |= TransparentSSRPrepass;
        break;
    case ShaderFeature::DirectLighting:
        flags |= DirectLighting;
        break;
    case ShaderFeature::IndirectLighting:
        flags |= IndirectLighting;
        break;
    default:
        break;
    }
}
