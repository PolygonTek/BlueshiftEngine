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
#include "ShaderPipelineState.h"
#include "ShaderTemplate.h"
#include "LoadingSystem.h"

uint64_t ShaderPipelineDesc::GenerateInputLayoutHash(const RHI::InputLayout *inputLayout) {
    if (!inputLayout) {
        return 0;
    }

    uint64_t hash = 0;
    for (const RHI::InputLayoutElement &element : inputLayout->elements) {
        hash = hash_combine(hash, element);
    }
    return hash;
}

uint64_t ShaderPipelineDesc::GeneratePipelineDescHash() const {
    struct PipelineDescHashData {
        uint64_t    inputLayoutHash = 0;
        uint64_t    renderDestHash = 0;
        uint8_t     rasterizerStateType = 0;
        uint8_t     depthStencilStateType = 0;
        uint8_t     blendStateType = 0;
        uint8_t     primitiveTopology = 0;
        uint32_t    sampleMask = 0;
    } hashData = {};

    hashData.inputLayoutHash = GenerateInputLayoutHash(inputLayout);
    hashData.renderDestHash = renderDest.GetHash();
    hashData.rasterizerStateType = static_cast<uint8_t>(rasterizerStateType);
    hashData.depthStencilStateType = static_cast<uint8_t>(depthStencilStateType);
    hashData.blendStateType = static_cast<uint8_t>(blendStateType);
    hashData.primitiveTopology = static_cast<uint8_t>(primitiveTopology);
    hashData.sampleMask = sampleMask;

    return BE1::CityHash64(reinterpret_cast<const char *>(&hashData), sizeof(hashData));
}

uint64_t ShaderPipelineDesc::GenerateGraphicsPipelineHash(ShaderPermutationKey permutationKey) const {
    uint64_t hash = permutationKey.hash;
    hash = hash_combine(hash, GeneratePipelineDescHash());
    return hash;
}

void ShaderPipelineState::Purge() {
    if (pso) {
        RHI::renderer->DestroyPSO(pso);
        pso = nullptr;
    }
}

void ShaderPipelineState::Reset() {
    BumpBuildSerial();
    state = State::Empty;

    Purge();
}

void ShaderPipelineState::SetPSO(RHI::PipelineState *newPso) {
    if (pso) {
        RHI::renderer->DestroyPSO(pso);
    }
    pso = newPso;
}

void ShaderPipelineState::BuildAsync(const std::shared_ptr<ShaderTemplate> &shaderTemplate) {
    state = pso ? State::Rebuilding : State::Building;

    if (!EnqueueBuild(shaderTemplate)) {
        state = pso ? State::Ready : State::Failed;
    }
}

void ShaderPipelineState::EnsureBuildAsync(const std::shared_ptr<ShaderTemplate> &shaderTemplate) {
    // state == State::Empty 일 때만 BuildAsync() 를 실행한다.
    State expected = State::Empty;
    if (!state.compare_exchange_strong(expected, State::Building)) {
        return;
    }

    if (!EnqueueBuild(shaderTemplate)) {
        state = State::Failed;
    }
}

bool ShaderPipelineState::EnqueueBuild(const std::shared_ptr<ShaderTemplate> &shaderTemplate) {
    uint32_t newSerial = BumpBuildSerial();

    ShaderTemplate::PipelineBuildData buildData;
    if (!shaderTemplate->PreparePipelineBuildData(permutationKey, buildData)) {
        return false;
    }

    if (!loadingSystem.EnqueueShaderPipelineBuild(shared_from_this(), buildData, newSerial, LoadingSystem::LoadPriority::High)) {
        return false;
    }
    return true;
}
