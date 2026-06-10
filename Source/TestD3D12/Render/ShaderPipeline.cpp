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

void ShaderPipelineBuildData::Clear() {
    for (ShaderStageSource &stageSource : stageSources) {
        stageSource = ShaderStageSource();
    }

    compileDefines.Clear();
}

static bool PrepareStageShader(const ShaderPipelineBuildData &buildData, RHI::ShaderStage shaderStage, const BE1::StrArray &compileDefines, RHI::Shader *&outShader) {
    assert(shaderStage < RHI::ShaderStage::Count);

    const ShaderStageSource &stageSource = buildData.stageSources[to_int(shaderStage)];
    if (stageSource.shaderStage == RHI::ShaderStage::Count) {
        return false;
    }

    outShader = RHI::renderer->CreateShaderFromFile(stageSource.shaderModel, stageSource.shaderStage, stageSource.filename, stageSource.entryPoint, compileDefines.Count() > 0 ? &compileDefines : nullptr);
    if (!outShader) {
        BE_WARNLOG("ShaderPipeline::PrepareStageShader: failed to compile shader '%s'\n", stageSource.filename.c_str());
        return false;
    }
    return true;
}

static bool PreparePipelineGraphicsShaders(const ShaderPipelineBuildData &buildData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    static constexpr RHI::ShaderStage GraphicsShaderStages[] = {
        RHI::ShaderStage::Vertex,
        RHI::ShaderStage::Hull,
        RHI::ShaderStage::Domain,
        RHI::ShaderStage::Geometry,
        RHI::ShaderStage::Fragment
    };

    bool hasGraphicsStage = false;

    for (RHI::ShaderStage shaderStage : GraphicsShaderStages) {
        const ShaderStageSource &stageSource = buildData.stageSources[to_int(shaderStage)];
        if (stageSource.shaderStage == RHI::ShaderStage::Count) {
            continue;
        }

        hasGraphicsStage = true;

        RHI::Shader *&stageShader = stageShaders[to_int(shaderStage)];
        if (!stageShader) {
            if (!PrepareStageShader(buildData, shaderStage, buildData.compileDefines, stageShader)) {
                return false;
            }
        }
    }

    if (!hasGraphicsStage) {
        BE_WARNLOG("ShaderPipeline::PreparePipelineGraphicsShaders: no graphics shader stages\n");
        return false;
    }

    return true;
}

static bool PreparePipelineComputeShader(const ShaderPipelineBuildData &buildData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    const ShaderStageSource &stageSource = buildData.stageSources[to_int(RHI::ShaderStage::Compute)];
    if (stageSource.shaderStage == RHI::ShaderStage::Count) {
        return false;
    }

    RHI::Shader *&stageShader = stageShaders[to_int(RHI::ShaderStage::Compute)];
    if (!stageShader) {
        if (!PrepareStageShader(buildData, RHI::ShaderStage::Compute, buildData.compileDefines, stageShader)) {
            BE_WARNLOG("ShaderPipeline::PreparePipelineComputeShader: no compute shader stage\n");
            return false;
        }
    }

    return true;
}

static RHI::PipelineState *CreateGraphicsPSO(const ShaderPipelineBuildData &buildData, const ShaderPipelineDesc &pipelineDesc, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    if (!PreparePipelineGraphicsShaders(buildData, stageShaders)) {
        return nullptr;
    }

    RHI::PipelineStateDesc psoDesc;
    psoDesc.vs = stageShaders[to_int(RHI::ShaderStage::Vertex)];
    psoDesc.ps = stageShaders[to_int(RHI::ShaderStage::Fragment)];
    psoDesc.ds = stageShaders[to_int(RHI::ShaderStage::Domain)];
    psoDesc.hs = stageShaders[to_int(RHI::ShaderStage::Hull)];
    psoDesc.gs = stageShaders[to_int(RHI::ShaderStage::Geometry)];
    psoDesc.inputLayout = pipelineDesc.inputLayout;
    psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(pipelineDesc.rasterizerStateType);
    psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(pipelineDesc.depthStencilStateType);
    psoDesc.blendState = RHI::renderer->GetBlendState(pipelineDesc.blendStateType);
    psoDesc.renderDest = &pipelineDesc.renderDest;
    psoDesc.primitiveTopology = pipelineDesc.primitiveTopology;
    psoDesc.sampleMask = pipelineDesc.sampleMask;

    return RHI::renderer->CreateGraphicsPSO(&psoDesc);
}

static RHI::PipelineState *CreateComputePSO(const ShaderPipelineBuildData &buildData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    if (!PreparePipelineComputeShader(buildData, stageShaders)) {
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

void ShaderPipeline::SetPSO(RHI::PipelineState *newPso) {
    if (pso) {
        RHI::renderer->DestroyPSO(pso);
    }
    pso = newPso;
}

bool ShaderPipeline::PreparePSO(const ShaderPipelineBuildData &buildData, RHI::PipelineState *&preparedPSO) const {
    preparedPSO = nullptr;

    RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)] = {};

    switch (GetType()) {
    case Type::Graphics:
        preparedPSO = CreateGraphicsPSO(buildData, GetGraphicsDesc(), stageShaders);
        break;
    case Type::Compute:
        preparedPSO = CreateComputePSO(buildData, stageShaders);
        break;
    default:
        assert(0);
        break;
    }

    for (int i = 0; i < to_int(RHI::ShaderStage::Count); ++i) {
        if (stageShaders[i]) {
            RHI::renderer->DestroyShader(stageShaders[i], true);
            stageShaders[i] = nullptr;
        }
    }
    return preparedPSO != nullptr;
}

bool ShaderPipeline::Build(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!shaderTemplate) {
        return false;
    }

    ShaderPipelineBuildData buildData;
    if (!shaderTemplate->PreparePipelineBuildData(permutationKey, buildData)) {
        return false;
    }

    RHI::PipelineState *preparedPSO = nullptr;
    if (!PreparePSO(buildData, preparedPSO)) {
        return false;
    }

    BumpBuildSerial(); // 진행 중인 async completion 무효화

    SetPSO(preparedPSO);
    state = State::Ready;
    return true;
}

void ShaderPipeline::BuildAsync(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!shaderTemplate) {
        return;
    }

    state = pso ? State::Rebuilding : State::Building;

    if (!EnqueueBuild(shaderTemplate)) {
        state = pso ? State::Ready : State::Failed;
    }
}

void ShaderPipeline::EnsureBuildAsync(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!shaderTemplate) {
        return;
    }

    // state == State::Empty 일 때만 BuildAsync() 를 실행한다.
    State expected = State::Empty;
    if (!state.compare_exchange_strong(expected, State::Building)) {
        return;
    }

    if (!EnqueueBuild(shaderTemplate)) {
        state = State::Failed;
    }
}

bool ShaderPipeline::EnqueueBuild(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    if (!shaderTemplate) {
        return false;
    }

    uint32_t newSerial = BumpBuildSerial();

    ShaderPipelineBuildData buildData;
    if (!shaderTemplate->PreparePipelineBuildData(permutationKey, buildData)) {
        return false;
    }

    if (!loadingSystem.EnqueueShaderPipelineBuild(shared_from_this(), buildData, newSerial, LoadingSystem::LoadPriority::High)) {
        return false;
    }
    return true;
}

bool ShaderPipeline::WaitReady() {
    assert(BE1::Engine::IsInMainThread());

    if (GetState() == State::Building) {
        loadingSystem.BoostShaderPipelineBuild(this, GetBuildSerial(), LoadingSystem::LoadPriority::Critical);
    }

    while (GetState() == State::Building) {
        loadingSystem.ProcessLoadCompletions();

        BE1::PlatformThread::Yield();
    }

    return IsReady();
}

bool ShaderPipeline::WaitLoadComplete() {
    assert(BE1::Engine::IsInMainThread());

    if (IsBuilding()) {
        loadingSystem.BoostShaderPipelineBuild(this, GetBuildSerial(), LoadingSystem::LoadPriority::Critical);
    }

    while (IsBuilding()) {
        loadingSystem.ProcessLoadCompletions();

        BE1::PlatformThread::Yield();
    }

    return GetState() == State::Ready;
}
