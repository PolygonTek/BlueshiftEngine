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

struct ShaderPermutationKey;

struct GraphicsPipelineDesc {
    uint64_t                        GeneratePipelineDescHash() const;
    uint64_t                        GenerateGraphicsPipelineHash(ShaderPermutationKey permutationKey) const;

    static uint64_t                 GenerateInputLayoutHash(const RHI::InputLayout *inputLayout);

    const RHI::InputLayout *        inputLayout = nullptr;
    RHI::RenderDest                 renderDest;
    RHI::RasterizerStateType        rasterizerStateType = RHI::RasterizerStateType::SolidFrontSided;
    RHI::DepthStencilStateType      depthStencilStateType = RHI::DepthStencilStateType::LEqualWrite;
    RHI::BlendStateType             blendStateType = RHI::BlendStateType::Disabled;
    RHI::PrimitiveTopology          primitiveTopology = RHI::PrimitiveTopology::TriangleList;
    uint32_t                        sampleMask = 0xffffffff;
    bool                            fragmentShaderEnabled = true;
    bool                            depthClipEnabled = true;
    bool                            depthBiasOverride = false;
    float                           constantDepthBias = 0;
    float                           depthBiasClamp = 0;
    float                           slopeScaledDepthBias = 0;
};
