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

class Texture;

/*
---------------------------------------------------------------------------------------------------

    Probe Bake Job

    입력 radiance cubemap 을 필터링해서 probe cubemap 을 생성한다.

---------------------------------------------------------------------------------------------------
*/

class ProbeBakeJob {
public:
    enum class OutputFlag : uint8_t {
        None                        = 0,
        DiffuseIrradiance           = BIT(0),
        SpecularPrefiltered         = BIT(1),
        All                         = DiffuseIrradiance | SpecularPrefiltered
    };

    friend constexpr std::true_type EnableBitmaskEnum(OutputFlag) noexcept { return {}; }

    struct Desc {
        SharedPtr<Texture>          radianceCubeMap;                    ///< 필수 입력 radiance cubemap
        OutputFlag                  outputFlags = OutputFlag::None;
        bool                        useHDR = true;                      ///< 결과 texture 의 HDR 여부
        int32_t                     diffuseIrradianceCubemapSize = 16;  ///< 결과 diffuse irradiance cubemap size
        int32_t                     specularPrefilteredSize = 256;      ///< 결과 specular prefiltered cubemap size
    };

    ProbeBakeJob() = default;
    explicit ProbeBakeJob(Desc &&desc) : desc(std::move(desc)) {}

    bool                            Execute(RHI::CommandList *commandList);

                                    // 입력 radiance cubemap 을 리턴
    const SharedPtr<Texture> &      GetRadianceCubeMap() const { return desc.radianceCubeMap; }

                                    // 생성된 diffuse irradiance cubemap 을 리턴
    const SharedPtr<Texture> &      GetDiffuseIrradianceCubeMap() const { return diffuseIrradianceCubeMap; }

                                    // 생성된 specular prefiltered cubemap 을 리턴
    const SharedPtr<Texture> &      GetSpecularPrefilteredCubeMap() const { return specularPrefilteredCubeMap; }

private:
    Desc                            desc;
    SharedPtr<Texture>              diffuseIrradianceCubeMap;
    SharedPtr<Texture>              specularPrefilteredCubeMap;
};
