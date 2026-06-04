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

void ShaderPermutationKey::NormalizePermutationDefines(BE1::StrArray &defines) {
    // Remove empty defines.
    for (int i = defines.Count() - 1; i >= 0; --i) {
        if (defines[i].IsEmpty()) {
            defines.RemoveIndexFast(i);
        }
    }

    // Keep permutation keys deterministic.
    defines.Sort();

    // Remove duplicated defines.
    for (int i = 1; i < defines.Count();) {
        if (defines[i - 1] == defines[i]) {
            defines.RemoveIndex(i);
            continue;
        }
        i++;
    }
}

ShaderPermutationKey ShaderPermutationKey::GeneratePermutationKey(const BE1::StrArray &defines) {
    if (defines.Count() == 0) {
        return ShaderPermutationKey();
    }

    uint64_t hash = 1469598103934665603ull;
    for (const BE1::Str &define : defines) {
        hash = hash_combine(hash, define.ToHash64());
    }

    return ShaderPermutationKey(hash);
}

ShaderPermutationKey ShaderPermutationKey::FromDefines(std::initializer_list<const char *> defines) {
    BE1::StrArray permutationDefines;
    for (const char *defineName : defines) {
        if (defineName && defineName[0]) {
            permutationDefines.Append(defineName);
        }
    }

    ShaderPermutationKey::NormalizePermutationDefines(permutationDefines);
    return ShaderPermutationKey::GeneratePermutationKey(permutationDefines);
}

ShaderPermutationKey ShaderPermutationKey::FromDefines(const BE1::StrArray &defines) {
    BE1::StrArray permutationDefines = defines;
    ShaderPermutationKey::NormalizePermutationDefines(permutationDefines);
    return ShaderPermutationKey::GeneratePermutationKey(permutationDefines);
}

void ShaderPipelineState::Purge() {
    if (pso) {
        RHI::renderer->DestroyPSO(pso);
        pso = nullptr;
    }

    state = State::Empty;
}

void ShaderPipelineState::Reset() {
    BumpBuildSerial();
    state = State::Empty;

    Purge();
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

    if (!loadingSystem.EnqueueShaderPipelineBuild(shaderTemplate, shared_from_this(), buildData, newSerial, LoadingSystem::LoadPriority::High)) {
        return false;
    }
    return true;
}
