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
#include "ShaderPipelineBuildSystem.h"
#include "Render/RenderContext.h"

ShaderPipeline::InternalData &ShaderPipeline::InternalData::operator=(InternalData &&rhs) noexcept {
    if (this != &rhs) {
        Purge();
        Swap(rhs);
    }
    return *this;
}

void ShaderPipeline::InternalData::Purge() {
    if (pso) {
        // 캐시에서 공유되는 PSO 참조를 반환하고, 마지막 참조도 지연 해제한다.
        RHI::renderer->DestroyPSO(pso);
        pso = nullptr;
    }
}

void ShaderPipeline::InternalData::Swap(InternalData &rhs) {
    BE1::Swap(pso, rhs.pso);
}

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
    internalData.Purge();
}

void ShaderPipeline::Reset() {
    BumpBuildSerial();
    state = State::Empty;

    Purge();
}

bool ShaderPipeline::Prepare(const ShaderCompileData &compileData, InternalData &outInternalData) const {
    assert(&outInternalData != &internalData);

    outInternalData.Purge();

    RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)] = {};

    switch (GetType()) {
    case Type::Graphics:
        outInternalData.pso = CreateGraphicsPSO(compileData, graphicsPipelineDesc, stageShaders);
        break;
    case Type::Compute:
        outInternalData.pso = CreateComputePSO(compileData, stageShaders);
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
    return outInternalData.pso != nullptr;
}

void ShaderPipeline::ApplyPrepared(InternalData &newInternalData) {
    assert(newInternalData.pso);
    assert(&newInternalData != &internalData);

    // 캐시가 같은 PSO 를 반환해도 두 데이터는 각각 참조를 소유하므로, 기존 참조를 반환한 뒤 교체한다.
    Purge();

    internalData.Swap(newInternalData);
}

bool ShaderPipeline::Build(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!BuildAsync(shaderTemplate)) {
        return false;
    }

    return WaitReady();
}

bool ShaderPipeline::BuildAsync(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    assert(!internalData.pso);

    state = State::Building;

    // 템플릿이 유효하지 않아도 이전 비동기 빌드 결과는 무효화한다.
    uint32_t newSerial = BumpBuildSerial();

    if (!shaderTemplate) {
        state = State::Failed;
        return false;
    }

    ShaderCompileData compileData;
    if (!shaderTemplate->PrepareShaderCompileData(shaderPermutationKey, compileData)) {
        state = State::Failed;
        return false;
    }

    if (!shaderPipelineBuildSystem.EnqueueBuild(shared_from_this(), std::move(compileData), false, newSerial, ShaderPipelineBuildSystem::BuildPriority::High)) {
        state = State::Failed;
        return false;
    }

    return true;
}

bool ShaderPipeline::Rebuild(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    assert(BE1::Engine::IsInMainThread());
    assert(RenderContext::activeContext == nullptr);

    if (!RebuildAsync(shaderTemplate)) {
        return false;
    }

    return WaitBuildComplete();
}

bool ShaderPipeline::RebuildAsync(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!shaderTemplate) {
        return false;
    }

    if (!IsReady()) {
        BE_WARNLOG("ShaderPipeline::RebuildAsync: shader pipeline for shader template '%s' permutation 0x%llx is not ready\n", shaderTemplate->GetHashName(), shaderPermutationKey.hash);
        return false;
    }

    assert(internalData.pso);

    uint32_t newSerial = BumpBuildSerial();
    state = State::Rebuilding;

    ShaderCompileData compileData;
    if (!shaderTemplate->PrepareShaderCompileData(shaderPermutationKey, compileData)) {
        state = State::Ready;
        return false;
    }

    if (!shaderPipelineBuildSystem.EnqueueBuild(shared_from_this(), std::move(compileData), true, newSerial, ShaderPipelineBuildSystem::BuildPriority::High)) {
        state = State::Ready;
        return false;
    }

    return true;
}

bool ShaderPipeline::WaitReady() {
    assert(BE1::Engine::IsInMainThread());

    State currentState = GetState();
    if (currentState == State::Building) {
        shaderPipelineBuildSystem.BoostBuild(this, GetBuildSerial(), ShaderPipelineBuildSystem::BuildPriority::Critical);
    }

    while (currentState == State::Building) {
        shaderPipelineBuildSystem.ProcessBuildCompletions();
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
        shaderPipelineBuildSystem.BoostBuild(this, GetBuildSerial(), ShaderPipelineBuildSystem::BuildPriority::Critical);
    }

    while (currentState == State::Building || currentState == State::Rebuilding) {
        shaderPipelineBuildSystem.ProcessBuildCompletions();
        shaderPipelineBuildSystem.ProcessPendingRebuilds();
        currentState = GetState();

        if (currentState == State::Building || currentState == State::Rebuilding) {
            BE1::PlatformThread::Yield();
        }
    }

    return currentState == State::Ready;
}
