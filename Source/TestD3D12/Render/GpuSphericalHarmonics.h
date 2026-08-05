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

#include "RHI/RHI.h"

class ShaderPipeline;
class ShaderTemplate;

/*
---------------------------------------------------------------------------------------------------

    GPU Spherical Harmonics

    * 2/3-band real SH
        - 2-band SH 는 band 0~1 의 4개 basis function 을 사용한다.
        - 3-band SH 는 band 0~2 의 9개 basis function 을 사용한다.
        - 각 RGB 계수는 float4 structured buffer element 로 저장하며 xyz 에 RGB, w 에 0을 기록한다.

    * Cubemap projection
        - 입력: 방향별 RGB 값을 저장한 cubemap
        - 출력: cubemap 전체를 적분한 2-band 4개 또는 3-band 9개의 RGB SH 계수

    * Cubemap reconstruction
        - 입력: RGB SH 계수와 각 band 에 적용할 factor
        - 출력: factor 가 적용된 SH 를 방향별로 평가한 RGB cubemap

    * Cubemap filtering
        - cubemap projection 과 reconstruction 을 연속해서 수행한다.
        - band factor (1, 1): 2-band SH 근사 복원
        - band factor (1, 2/3): 2-band Lambert diffuse convolution
        - band factor (1, 1, 1): 3-band SH 근사 복원
        - band factor (1, 2/3, 1/4): Lambert diffuse convolution

---------------------------------------------------------------------------------------------------
*/

class GpuSphericalHarmonics {
public:
    static constexpr uint32_t       SH2CoefficientCount = 4; // 2-band 계수 개수
    static constexpr uint32_t       SH3CoefficientCount = 9; // 3-band 계수 개수
    static constexpr uint32_t       SHCoefficientStride = sizeof(float) * 4; // float4 버퍼 stride
    static constexpr uint32_t       SH2CoefficientBufferSize = SH2CoefficientCount * SHCoefficientStride;
    static constexpr uint32_t       SH3CoefficientBufferSize = SH3CoefficientCount * SHCoefficientStride;

    void                            Init();
    void                            Shutdown();

    void                            UpdatePipelineBuildState();

    bool                            ArePipelinesReady() const { return pipelinesReady; }
    bool                            HasPipelineBuildFailed() const { return pipelineBuildFailed; }

                                    /// cubemap 을 2-band SH 로 투영하여 outputCoefficients 의 4개 float4 element 에 기록한다.
                                    /// cubeMap 은 ShaderResourceCompute, outputCoefficients 는 UnorderedAccess 상태여야 하며 함수는 두 상태를 변경하지 않는다.
    bool                            ProjectCubemapSH2(RHI::CommandList *commandList, const RHI::Texture *cubeMap, RHI::Buffer *outputCoefficients, RHI::ViewID outputUAV, RHI::ViewID cubeMapSRV = RHI::ViewID::PrimaryID);

                                    /// cubemap 을 3-band SH 로 투영하여 outputCoefficients 의 9개 float4 element 에 기록한다.
                                    /// cubeMap 은 ShaderResourceCompute, outputCoefficients 는 UnorderedAccess 상태여야 하며 함수는 두 상태를 변경하지 않는다.
    bool                            ProjectCubemapSH3(RHI::CommandList *commandList, const RHI::Texture *cubeMap, RHI::Buffer *outputCoefficients, RHI::ViewID outputUAV, RHI::ViewID cubeMapSRV = RHI::ViewID::PrimaryID);

                                    /// 2-band SH 계수에 bandFactors 를 적용하여 outputCubeMap 으로 재구성한다.
                                    /// inputCoefficients 는 ShaderResourceCompute, outputCubeMap 은 UnorderedAccess 상태여야 하며 함수는 두 상태를 변경하지 않는다.
                                    /// bandFactors 의 xy 는 각각 band 0, 1 에 곱해진다.
    bool                            ReconstructCubemapSH2(RHI::CommandList *commandList, const RHI::Buffer *inputCoefficients, RHI::ViewID inputSRV, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec2 &bandFactors);

                                    /// 3-band SH 계수에 bandFactors 를 적용하여 outputCubeMap 으로 재구성한다.
                                    /// inputCoefficients 는 ShaderResourceCompute, outputCubeMap 은 UnorderedAccess 상태여야 하며 함수는 두 상태를 변경하지 않는다.
                                    /// bandFactors 의 xyz 는 각각 band 0, 1, 2 에 곱해진다.
    bool                            ReconstructCubemapSH3(RHI::CommandList *commandList, const RHI::Buffer *inputCoefficients, RHI::ViewID inputSRV, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec3 &bandFactors);

                                    /// inputCubeMap 을 2-band SH 로 투영하고 bandFactors 를 적용하여 outputCubeMap 으로 재구성한다.
                                    /// inputCubeMap 은 ShaderResourceCompute, outputCubeMap 은 UnorderedAccess 상태여야 하며 함수는 두 상태를 변경하지 않는다.
    bool                            FilterCubemapSH2(RHI::CommandList *commandList, const RHI::Texture *inputCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec2 &bandFactors, RHI::ViewID inputSRV = RHI::ViewID::PrimaryID);

                                    /// inputCubeMap 을 3-band SH 로 투영하고 bandFactors 를 적용하여 outputCubeMap 으로 재구성한다.
                                    /// inputCubeMap 은 ShaderResourceCompute, outputCubeMap 은 UnorderedAccess 상태여야 하며 함수는 두 상태를 변경하지 않는다.
    bool                            FilterCubemapSH3(RHI::CommandList *commandList, const RHI::Texture *inputCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec3 &bandFactors, RHI::ViewID inputSRV = RHI::ViewID::PrimaryID);

private:
    static constexpr uint32_t       FaceSH2CoefficientBufferSize = 6 * SH2CoefficientBufferSize;
    static constexpr uint32_t       FaceSH3CoefficientBufferSize = 6 * SH3CoefficientBufferSize;

    struct SHBandResources {
        uint32_t                    coefficientCount = 0;

        SharedPtr<ShaderTemplate>   projectShaderTemplate;
        SharedPtr<ShaderTemplate>   reduceShaderTemplate;
        SharedPtr<ShaderTemplate>   reconstructShaderTemplate;

        SharedPtr<ShaderPipeline>   projectPipeline;
        SharedPtr<ShaderPipeline>   reducePipeline;
        SharedPtr<ShaderPipeline>   reconstructPipeline;

        RHI::ViewID                 faceCoefficientSRV = RHI::ViewID::InvalidID;
        RHI::ViewID                 faceCoefficientUAV = RHI::ViewID::InvalidID;
        RHI::ViewID                 coefficientSRV = RHI::ViewID::InvalidID;
        RHI::ViewID                 coefficientUAV = RHI::ViewID::InvalidID;
    };

    bool                            ProjectCubemapSH(RHI::CommandList *commandList, const RHI::Texture *cubeMap, RHI::Buffer *outputCoefficients, RHI::ViewID outputUAV, RHI::ViewID cubeMapSRV, const SHBandResources &bandResources);
    bool                            ReconstructCubemapSH(RHI::CommandList *commandList, const RHI::Buffer *inputCoefficients, RHI::ViewID inputSRV, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec3 &bandFactors, const SHBandResources &bandResources);
    bool                            FilterCubemapSH(RHI::CommandList *commandList, const RHI::Texture *inputCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV, const BE1::Vec3 &bandFactors, RHI::ViewID inputSRV, const SHBandResources &bandResources);

    SHBandResources                 sh2Resources;
    SHBandResources                 sh3Resources;

                                    // face 별 투영 계수를 reduction pass 전까지 저장하는 3-band 크기의 재사용 scratch buffer.
    RHI::Buffer *                   faceSHCoefficientBuffer = nullptr;
    RHI::GPUResourceState           faceSHCoefficientBufferState = RHI::GPUResourceState::Undefined;

                                    // FilterCubemapSH2/SH3 의 projection 과 reconstruction 사이에서 계수를 저장하는 3-band 크기의 재사용 scratch buffer.
    RHI::Buffer *                   shCoefficientBuffer = nullptr;
    RHI::GPUResourceState           shCoefficientBufferState = RHI::GPUResourceState::Undefined;

    bool                            pipelinesReady = false;
    bool                            pipelineBuildFailed = false;
};
