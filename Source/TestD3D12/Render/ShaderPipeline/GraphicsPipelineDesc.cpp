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
#include "GraphicsPipelineDesc.h"
#include "ShaderPermutationKey.h"

uint64_t GraphicsPipelineDesc::GenerateInputLayoutHash(const RHI::InputLayout *inputLayout) {
    if (!inputLayout) {
        return 0;
    }

    uint64_t hash = 0;
    for (const RHI::InputLayoutElement &element : inputLayout->elements) {
        hash = hash_combine(hash, element);
    }
    return hash;
}

uint64_t GraphicsPipelineDesc::GeneratePipelineDescHash() const {
    struct PipelineDescHashData {
        uint64_t    inputLayoutHash;
        uint64_t    renderDestHash;
        uint8_t     rasterizerStateType;
        uint8_t     depthStencilStateType;
        uint8_t     blendStateType;
        uint8_t     primitiveTopology;
        uint32_t    sampleMask;
        uint8_t     fragmentShaderEnabled;
        uint8_t     depthClipEnabled;
        uint8_t     depthBiasOverride;
        float       constantDepthBias;
        float       depthBiasClamp;
        float       slopeScaledDepthBias;
    } hashData;

    // CityHash64 hashes the entire object representation, including padding.
    memset(&hashData, 0, sizeof(hashData));

    hashData.inputLayoutHash = GenerateInputLayoutHash(inputLayout);
    hashData.renderDestHash = renderDest.GetHash();
    hashData.rasterizerStateType = static_cast<uint8_t>(rasterizerStateType);
    hashData.depthStencilStateType = static_cast<uint8_t>(depthStencilStateType);
    hashData.blendStateType = static_cast<uint8_t>(blendStateType);
    hashData.primitiveTopology = static_cast<uint8_t>(primitiveTopology);
    hashData.sampleMask = sampleMask;
    hashData.fragmentShaderEnabled = fragmentShaderEnabled;
    hashData.depthClipEnabled = depthClipEnabled;
    hashData.depthBiasOverride = depthBiasOverride;

    if (depthBiasOverride) {
        hashData.constantDepthBias = constantDepthBias;
        hashData.depthBiasClamp = depthBiasClamp;
        hashData.slopeScaledDepthBias = slopeScaledDepthBias;
    }

    return BE1::CityHash64(reinterpret_cast<const char *>(&hashData), sizeof(hashData));
}

uint64_t GraphicsPipelineDesc::GenerateGraphicsPipelineHash(ShaderPermutationKey permutationKey) const {
    uint64_t hash = permutationKey.hash;
    hash = hash_combine(hash, GeneratePipelineDescHash());
    return hash;
}
