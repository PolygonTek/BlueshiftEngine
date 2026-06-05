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
