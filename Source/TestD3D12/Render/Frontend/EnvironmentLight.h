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

class ProbeBakeSystem;
class RenderWorld;
class Texture;

class EnvironmentLight {
    friend class ProbeBakeSystem;
    friend class RenderWorld;

public:
    struct Desc {
        BE1::Mat3                   axis = BE1::Mat3::identity;         ///< Environment orientation in world space
        BE1::Color3                 linearColor = BE1::Color3::white;   ///< Linear RGB environment tint in [0, 1]
        float                       intensity = 1.0f;
        SharedPtr<Texture>          radianceCubeMap;                    ///< Optional source radiance cubemap. When null, the RenderWorld sky is captured.
    };

    EnvironmentLight() = default;
    ~EnvironmentLight() = default;

    const Desc &                    GetDesc() const { return desc; }

    const SharedPtr<Texture> &      GetDiffuseIrradianceCubeMap() const { return diffuseIrradianceCubeMap; }
    const SharedPtr<Texture> &      GetSpecularPrefilteredCubeMap() const { return specularPrefilteredCubeMap; }

    bool                            NeedsRegeneration() const { return needsRegeneration; }

private:
    void                            Reset();
    void                            Update(const Desc &desc);
    void                            SetBakedCubeMaps(const SharedPtr<Texture> &diffuseIrradianceCubeMap, const SharedPtr<Texture> &specularPrefilteredCubeMap);
    void                            ApplyBakeResult(bool succeeded, const SharedPtr<Texture> &diffuseCubeMap, const SharedPtr<Texture> &specularCubeMap);
    void                            MarkRegenerationNeeded() { needsRegeneration = true; }

    Desc                            desc;
    SharedPtr<Texture>              diffuseIrradianceCubeMap;
    SharedPtr<Texture>              specularPrefilteredCubeMap;
    bool                            needsRegeneration = true;
};
