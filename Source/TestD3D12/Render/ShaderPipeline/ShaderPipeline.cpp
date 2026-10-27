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
#include "ShaderPipeline.h"
#include "ShaderCompileData.h"
#include "ShaderTemplate.h"
#include "LoadingSystem.h"
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"

bool ShaderPipeline::CreateStageShader(const ShaderCompileData &compileData, RHI::ShaderStage shaderStage, RHI::Shader *&outShader) {
    assert(shaderStage < RHI::ShaderStage::Count);

    const ShaderStageSource &stageSource = compileData.stageSources[to_int(shaderStage)];
    const BE1::StrArray &compileDefines = compileData.compileDefines;

    outShader = RHI::renderer->CreateShaderFromFile(stageSource.shaderModel, shaderStage, stageSource.filename, stageSource.entryPoint, compileDefines.Count() > 0 ? &compileDefines : nullptr);
    if (!outShader) {
        BE_FATALERROR("ShaderPipeline::CreateStageShader: failed to compile shader '%s'\n", stageSource.filename.c_str());
        return false;
    }
    return true;
}

bool ShaderPipeline::CreateGraphicsShaders(const ShaderCompileData &compileData, const GraphicsPipelineDesc &pipelineDesc, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    if (compileData.stageSources[to_int(RHI::ShaderStage::Vertex)].filename.IsEmpty()) {
        BE_WARNLOG("ShaderPipeline::CreateGraphicsShaders: no vertex shader stage\n");
        return false;
    }

    // Hull/Domain 셰이더는 함께 지정해야 한다.
    const bool hasHullShader = !compileData.stageSources[to_int(RHI::ShaderStage::Hull)].filename.IsEmpty();
    const bool hasDomainShader = !compileData.stageSources[to_int(RHI::ShaderStage::Domain)].filename.IsEmpty();
    if (hasHullShader != hasDomainShader) {
        BE_WARNLOG("ShaderPipeline::CreateGraphicsShaders: hull and domain shader stages must be specified together\n");
        return false;
    }

    // Hull/Domain 셰이더 쌍의 존재 여부와 PatchList 사용 여부가 일치해야 한다.
    const bool usesPatchTopology = pipelineDesc.primitiveTopology == RHI::PrimitiveTopology::PatchList;
    if (hasHullShader != usesPatchTopology) {
        BE_WARNLOG("ShaderPipeline::CreateGraphicsShaders: patch topology and hull/domain shader stages must be used together\n");
        return false;
    }

    static constexpr RHI::ShaderStage graphicsShaderStages[] = {
        RHI::ShaderStage::Vertex,
        RHI::ShaderStage::Hull,
        RHI::ShaderStage::Domain,
        RHI::ShaderStage::Geometry,
        RHI::ShaderStage::Fragment
    };

    for (RHI::ShaderStage shaderStage : graphicsShaderStages) {
        if (shaderStage == RHI::ShaderStage::Fragment && !pipelineDesc.fragmentShaderEnabled) {
            continue;
        }

        const ShaderStageSource &stageSource = compileData.stageSources[to_int(shaderStage)];
        if (stageSource.filename.IsEmpty()) {
            continue;
        }

        RHI::Shader *&stageShader = stageShaders[to_int(shaderStage)];
        if (!CreateStageShader(compileData, shaderStage, stageShader)) {
            return false;
        }
    }

    return true;
}

bool ShaderPipeline::CreateComputeShader(const ShaderCompileData &compileData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    const ShaderStageSource &stageSource = compileData.stageSources[to_int(RHI::ShaderStage::Compute)];
    if (stageSource.filename.IsEmpty()) {
        BE_WARNLOG("ShaderPipeline::CreateComputeShader: no compute shader stage\n");
        return false;
    }

    RHI::Shader *&stageShader = stageShaders[to_int(RHI::ShaderStage::Compute)];
    return CreateStageShader(compileData, RHI::ShaderStage::Compute, stageShader);
}

RHI::PipelineState *ShaderPipeline::CreateGraphicsPSO(const ShaderCompileData &compileData, const GraphicsPipelineDesc &pipelineDesc, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    if (!CreateGraphicsShaders(compileData, pipelineDesc, stageShaders)) {
        return nullptr;
    }

    RHI::PipelineStateDesc psoDesc;
    psoDesc.vs = stageShaders[to_int(RHI::ShaderStage::Vertex)];
    psoDesc.ps = pipelineDesc.fragmentShaderEnabled ? stageShaders[to_int(RHI::ShaderStage::Fragment)] : nullptr;
    psoDesc.ds = stageShaders[to_int(RHI::ShaderStage::Domain)];
    psoDesc.hs = stageShaders[to_int(RHI::ShaderStage::Hull)];
    psoDesc.gs = stageShaders[to_int(RHI::ShaderStage::Geometry)];
    psoDesc.inputLayout = pipelineDesc.inputLayout;
    psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(pipelineDesc.rasterizerStateType);

    RHI::RasterizerState rasterizerState;
    if (pipelineDesc.depthBiasOverride || pipelineDesc.depthClipEnabled != psoDesc.rasterizerState->depthClipEnabled) {
        rasterizerState = *psoDesc.rasterizerState;

        if (pipelineDesc.depthClipEnabled != psoDesc.rasterizerState->depthClipEnabled) {
            rasterizerState.depthClipEnabled = pipelineDesc.depthClipEnabled;
        }
        if (pipelineDesc.depthBiasOverride) {
            rasterizerState.constantDepthBias = pipelineDesc.constantDepthBias;
            rasterizerState.depthBiasClamp = pipelineDesc.depthBiasClamp;
            rasterizerState.slopeScaledDepthBias = pipelineDesc.slopeScaledDepthBias;
        }
        psoDesc.rasterizerState = &rasterizerState;
    }

    psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(pipelineDesc.depthStencilStateType);
    psoDesc.blendState = RHI::renderer->GetBlendState(pipelineDesc.blendStateType);
    psoDesc.renderDest = &pipelineDesc.renderDest;
    psoDesc.primitiveTopology = pipelineDesc.primitiveTopology;
    psoDesc.sampleMask = pipelineDesc.sampleMask;

    return RHI::renderer->CreateGraphicsPSO(&psoDesc);
}

RHI::PipelineState *ShaderPipeline::CreateComputePSO(const ShaderCompileData &compileData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    if (!CreateComputeShader(compileData, stageShaders)) {
        return nullptr;
    }

    RHI::Shader *computeShader = stageShaders[to_int(RHI::ShaderStage::Compute)];
    return RHI::renderer->CreateComputePSO(computeShader);
}

void ShaderPipeline::Purge() {
    if (pso) {
        RHI::renderer->DestroyPSO(pso);
        pso = nullptr;
    }
}

void ShaderPipeline::Reset() {
    BumpBuildSerial();
    state = State::Empty;

    Purge();
}

void ShaderPipeline::ReplacePSO(RHI::PipelineState *newPso) {
    if (pso) {
        RHI::renderer->DestroyPSO(pso);
    }
    pso = newPso;
}

RHI::PipelineState *ShaderPipeline::CreatePSO(const ShaderCompileData &compileData) const {
    RHI::PipelineState *pso = nullptr;

    RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)] = {};

    switch (GetType()) {
    case Type::Graphics:
        pso = CreateGraphicsPSO(compileData, graphicsPipelineDesc, stageShaders);
        break;
    case Type::Compute:
        pso = CreateComputePSO(compileData, stageShaders);
        break;
    default:
        assert(0);
        break;
    }

    for (int stageIndex = 0; stageIndex < to_int(RHI::ShaderStage::Count); ++stageIndex) {
        if (stageShaders[stageIndex]) {
            RHI::renderer->DestroyShader(stageShaders[stageIndex], true);
            stageShaders[stageIndex] = nullptr;
        }
    }
    return pso;
}

bool ShaderPipeline::Build(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    assert(!pso);

    BumpBuildSerial(); // 진행 중인 async completion 무효화

    if (!shaderTemplate) {
        state = State::Failed;
        return false;
    }

    ShaderCompileData compileData;
    if (!shaderTemplate->PrepareShaderCompileData(shaderPermutationKey, compileData)) {
        state = State::Failed;
        return false;
    }

    RHI::PipelineState *preparedPSO = CreatePSO(compileData);
    if (!preparedPSO) {
        state = State::Failed;
        return false;
    }

    ReplacePSO(preparedPSO);
    state = State::Ready;
    return true;
}

void ShaderPipeline::BuildAsync(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    assert(!pso);

    if (!shaderTemplate) {
        return;
    }

    state = State::Building;

    if (!EnqueueBuild(shaderTemplate)) {
        state = State::Failed;
    }
}

bool ShaderPipeline::Rebuild(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!shaderTemplate) {
        return false;
    }

    if (GetState() != State::Ready) {
        BE_WARNLOG("ShaderPipeline::Rebuild: shader pipeline for shader template '%s' permutation 0x%llx is not ready\n", shaderTemplate->GetHashName(), shaderPermutationKey.hash);
        return false;
    }

    assert(pso);
    assert(BE1::Engine::IsInMainThread());
    assert(RenderContext::activeContext == nullptr);

    ShaderCompileData compileData;
    if (!shaderTemplate->PrepareShaderCompileData(shaderPermutationKey, compileData)) {
        return false;
    }

    RHI::PipelineState *preparedPSO = CreatePSO(compileData);
    if (!preparedPSO) {
        return false;
    }

    BumpBuildSerial(); // 진행 중인 async completion 무효화
    state = State::Rebuilding;

    renderSystem->WaitAllRenderCompleted();

    ReplacePSO(preparedPSO);
    state = State::Ready;
    return true;
}

void ShaderPipeline::RebuildAsync(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!shaderTemplate) {
        return;
    }

    if (!IsReady()) {
        BE_WARNLOG("ShaderPipeline::RebuildAsync: shader pipeline for shader template '%s' permutation 0x%llx is not ready\n", shaderTemplate->GetHashName(), shaderPermutationKey.hash);
        return;
    }

    assert(pso);

    state = State::Rebuilding;

    if (!EnqueueBuild(shaderTemplate)) {
        state = State::Ready;
    }
}

bool ShaderPipeline::EnqueueBuild(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!shaderTemplate) {
        return false;
    }

    uint32_t newSerial = BumpBuildSerial();

    ShaderCompileData compileData;
    if (!shaderTemplate->PrepareShaderCompileData(shaderPermutationKey, compileData)) {
        return false;
    }

    if (!loadingSystem.EnqueueShaderPipelineBuild(shared_from_this(), std::move(compileData), newSerial, LoadingSystem::LoadPriority::High)) {
        return false;
    }
    return true;
}

bool ShaderPipeline::WaitReady() {
    assert(BE1::Engine::IsInMainThread());

    State currentState = GetState();
    if (currentState == State::Building) {
        loadingSystem.BoostShaderPipelineBuild(this, GetBuildSerial(), LoadingSystem::LoadPriority::Critical);
    }

    while (currentState == State::Building) {
        loadingSystem.ProcessLoadCompletions();
        currentState = GetState();

        if (currentState == State::Building) {
            BE1::PlatformThread::Yield();
        }
    }

    return currentState == State::Ready || currentState == State::Rebuilding;
}

bool ShaderPipeline::WaitBuildComplete() {
    assert(BE1::Engine::IsInMainThread());

    State currentState = GetState();
    if (currentState == State::Building || currentState == State::Rebuilding) {
        loadingSystem.BoostShaderPipelineBuild(this, GetBuildSerial(), LoadingSystem::LoadPriority::Critical);
    }

    while (currentState == State::Building || currentState == State::Rebuilding) {
        loadingSystem.ProcessLoadCompletions();
        loadingSystem.ProcessPendingReloads();
        currentState = GetState();

        if (currentState == State::Building || currentState == State::Rebuilding) {
            BE1::PlatformThread::Yield();
        }
    }

    return currentState == State::Ready;
}
