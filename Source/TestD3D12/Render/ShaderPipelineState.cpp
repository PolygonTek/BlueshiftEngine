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

static void NormalizePermutationDefines(BE1::StrArray &defines) {
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

static ShaderPermutationKey GeneratePermutationKey(const BE1::StrArray &defines) {
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

    NormalizePermutationDefines(permutationDefines);
    return GeneratePermutationKey(permutationDefines);
}

ShaderPermutationKey ShaderPermutationKey::FromDefines(const BE1::StrArray &defines) {
    BE1::StrArray permutationDefines = defines;
    NormalizePermutationDefines(permutationDefines);
    return GeneratePermutationKey(permutationDefines);
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

    Purge();
}

void ShaderPipelineState::RequestBuild(const std::shared_ptr<ShaderTemplate> &shaderTemplate) {
    State expected = State::Empty;
    if (!state.compare_exchange_strong(expected, State::Building)) {
        return;
    }

    uint32_t requestedBuildSerial = BumpBuildSerial();

    if (!loadingSystem.EnqueueShaderPipelineBuild(shaderTemplate, shared_from_this(), requestedBuildSerial, LoadingSystem::LoadPriority::High)) {
        state = State::Failed;
        return;
    }
}

void ShaderPipelineState::ResetAndRequestBuild(const std::shared_ptr<ShaderTemplate> &shaderTemplate) {
    Reset();
    RequestBuild(shaderTemplate);
}

bool ShaderPipelineState::Build(ShaderTemplate &shaderTemplate, uint32_t requestedBuildSerial) {
    if (GetState() != State::Building || GetBuildSerial() != requestedBuildSerial) {
        return IsReady();
    }

    RHI::PipelineState *newPSO = nullptr;

    switch (type) {
    case Type::Graphics:
        newPSO = shaderTemplate.CreateGraphicsPSO(graphicsDesc);
        break;
    case Type::Compute:
        newPSO = shaderTemplate.CreateComputePSO(permutationKey);
        break;
    default:
        assert(0);
        break;
    }

    if (GetState() != State::Building || GetBuildSerial() != requestedBuildSerial) {
        if (newPSO) {
            RHI::renderer->DestroyPSO(newPSO, true);
        }
        return false;
    }

    if (pso) {
        RHI::renderer->DestroyPSO(pso);
    }

    pso = newPSO;
    state = pso ? State::Ready : State::Failed;

    return pso != nullptr;
}
