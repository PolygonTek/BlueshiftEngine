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

#include "Precompiled.h"
#include "RHI/RHI.h"

class ShaderPipeline;
class ShaderTemplate;

/*
---------------------------------------------------------------------------------------------------

    Image-Based Lighting Generator

    * Generate diffuse IBL
        - Lambert diffuse irradiance cubemap
        - 조회 입력: surface normal N
        - 조회 출력: diffuse irradiance E(N) 에 Lambert BRDF 의 1 / PI 를 곱한 값

    * Generate Phong specular IBL
        - Phong specular LD cubemap
        - 조회 입력: reflection direction S 와 specular power 에 대응하는 mip level
        - 조회 출력: Phong-prefiltered radiance

    * Generate split-sum GGX specular IBL
        - 1st term: GGX LD cubemap
            - 조회 입력: reflection direction R 과 roughness 에 대응하는 mip level
            - 조회 출력: GGX-prefiltered radiance
        - 2nd term: GGX DFG LUT texture
            - 조회 입력: NdotV 와 roughness
            - 조회 출력: Fresnel scale 과 bias (A, B)

---------------------------------------------------------------------------------------------------
*/

class IBLGenerator {
public:
    void                            Init();
    void                            Shutdown();

                                    /// radianceCubeMap 의 mip 0 을 cosine-weighted midpoint quadrature 로 적분하여 Lambert diffuse irradiance cubemap 을 생성한다.
                                    /// outputUAV 는 mip 0 의 6개 face 를 포함해야 하며, 출력에는 Lambert BRDF 의 1/PI 가 포함된다.
    bool                            GenerateDiffuseIrradiance(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV = RHI::ViewID::PrimaryID);

                                    /// radianceCubeMap 을 3-band SH 의 9개 계수로 투영하고 Lambert convolution 하여 diffuse irradiance cubemap 을 생성한다.
                                    /// 출력에는 Lambert BRDF 의 1/PI 가 포함된다.
    bool                            GenerateDiffuseIrradianceSH(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputUAV = RHI::ViewID::PrimaryID);

                                    /// GGX split-sum 과 별개인 Phong specular LD cubemap 의 전체 mip chain 을 생성한다.
                                    /// specular power 는 mip 에 따라 낮아진다.
    bool                            GenerateLDSumPhongSpecular(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, int32_t maxSpecularPower, RHI::Texture *outputCubeMap, const RHI::ViewID *outputMipUAVs);

                                    /// outputCubeMap 의 특정 mip 에 해당하는 Phong specular LD 를 생성한다.
    bool                            GenerateLDSumPhongSpecularMip(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, int32_t maxSpecularPower, RHI::Texture *outputCubeMap, RHI::ViewID outputMipUAV, uint32_t mipLevel);

                                    /// split-sum GGX 의 1st term 인 LD cubemap 의 전체 mip chain 을 생성한다.
                                    /// radianceCubeMap 을 GGX 로 prefilter 한다.
                                    /// outputMipUAVs 는 출력 mip 수만큼 있어야 하며, 각 UAV 는 해당 mip 의 6개 face 를 포함해야 한다.
    bool                            GenerateLDSumGGX(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, const RHI::ViewID *outputMipUAVs);

                                    /// mip 0 은 radianceCubeMap 을 방향 기반으로 resampling 하고, 나머지 mip 은 GGX 적분을 수행한다.
                                    /// radianceCubeMap 은 mipmap filtered sampling 을 위해 전체 mip chain 을 포함해야 한다.
    bool                            GenerateLDSumGGXMip(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputMipUAV, uint32_t mipLevel);

                                    /// split-sum GGX 의 2nd term 인 DFG scale (A) 과 bias (B) 를 R16G16_FLOAT 2D LUT 로 생성한다.
                                    /// outputUAV 는 mip 0 의 단일 slice UAV 여야 한다.
    bool                            GenerateDFGSumGGX(RHI::CommandList *commandList, RHI::Texture *outputTexture, RHI::ViewID outputUAV = RHI::ViewID::PrimaryID);

                                    /// 자체 command list 로 DFG LUT 를 생성하고 GPU 완료를 기다린 뒤 CPU image 로 읽어온다.
    bool                            GenerateDFGSumGGXToImage(RHI::FrameThreadData *frameThreadData, int32_t size, BE1::Image &outImage);

                                    /// DFG LUT 를 생성하여 filename 의 확장자를 .dds 로 설정해 저장한다.
    bool                            GenerateDFGSumGGXToFile(RHI::FrameThreadData *frameThreadData, int32_t size, const char *filename);

private:
    bool                            DispatchCopyLDMip0(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputMipUAV, const RHI::TextureViewInfo &outputViewInfo);
    bool                            DispatchIntegrateLDMip(RHI::CommandList *commandList, const RHI::Texture *radianceCubeMap, RHI::Texture *outputCubeMap, RHI::ViewID outputMipUAV, const RHI::TextureViewInfo &outputViewInfo, uint32_t mipLevel);

    SharedPtr<ShaderTemplate>       genDiffuseIrradianceShaderTemplate;
    SharedPtr<ShaderTemplate>       projectCubemapSHShaderTemplate;
    SharedPtr<ShaderTemplate>       genDiffuseIrradianceSHShaderTemplate;
    SharedPtr<ShaderTemplate>       genLDSumPhongSpecularShaderTemplate;
    SharedPtr<ShaderTemplate>       genLDSumGGXShaderTemplate;
    SharedPtr<ShaderTemplate>       copyCubemapShaderTemplate;
    SharedPtr<ShaderTemplate>       genDFGSumGGXShaderTemplate;

    SharedPtr<ShaderPipeline>       genDiffuseIrradiancePipeline;
    SharedPtr<ShaderPipeline>       projectCubemapSHPipeline;
    SharedPtr<ShaderPipeline>       genDiffuseIrradianceSHPipeline;
    SharedPtr<ShaderPipeline>       genLDSumPhongSpecularPipeline;
    SharedPtr<ShaderPipeline>       genLDSumGGXPipeline;
    SharedPtr<ShaderPipeline>       copyCubemapPipeline;
    SharedPtr<ShaderPipeline>       genDFGSumGGXPipeline;

                                    // SH projection 의 face별 9 개 계수를 저장하는 재사용 scratch buffer.
    RHI::Buffer *                   shCoefficientBuffer = nullptr;
    RHI::ViewID                     shCoefficientSRV = RHI::ViewID::InvalidID;
    RHI::ViewID                     shCoefficientUAV = RHI::ViewID::InvalidID;
    RHI::GPUResourceState           shCoefficientBufferState = RHI::GPUResourceState::Undefined;
};
