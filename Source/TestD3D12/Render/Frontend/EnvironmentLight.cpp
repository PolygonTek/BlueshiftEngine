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
#include "EnvironmentLight.h"

void EnvironmentLight::Reset() {
    desc = Desc();
    diffuseIrradianceCubeMap.reset();
    specularPrefilteredCubeMap.reset();
    needsRegeneration = true;
}

void EnvironmentLight::Update(const Desc &src) {
    assert(src.linearColor.r >= 0.0f && src.linearColor.r <= 1.0f);
    assert(src.linearColor.g >= 0.0f && src.linearColor.g <= 1.0f);
    assert(src.linearColor.b >= 0.0f && src.linearColor.b <= 1.0f);
    assert(src.intensity >= 0.0f);

    if (desc.radianceCubeMap != src.radianceCubeMap) {
        MarkRegenerationNeeded();
    }

    desc = src;
}

void EnvironmentLight::SetBakedCubeMaps(const SharedPtr<Texture> &diffuseIrradianceCubeMap, const SharedPtr<Texture> &specularPrefilteredCubeMap) {
    this->diffuseIrradianceCubeMap = diffuseIrradianceCubeMap;
    this->specularPrefilteredCubeMap = specularPrefilteredCubeMap;
    needsRegeneration = !diffuseIrradianceCubeMap || !specularPrefilteredCubeMap;
}

void EnvironmentLight::ApplyBakeResult(bool succeeded, const SharedPtr<Texture> &diffuseCubeMap, const SharedPtr<Texture> &specularCubeMap) {
    if (succeeded && diffuseCubeMap && specularCubeMap) {
        SetBakedCubeMaps(diffuseCubeMap, specularCubeMap);
    } else {
        MarkRegenerationNeeded();
    }
}
