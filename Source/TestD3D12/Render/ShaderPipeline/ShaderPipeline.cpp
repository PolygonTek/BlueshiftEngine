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
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"

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
        uint8_t     depthBiasOverride;
        float       constantDepthBias;
        float       depthBiasClamp;
        float       slopeScaledDepthBias;
    } hashData = {};

    // CityHash64 hashes the entire object representation, including padding.
    memset(&hashData, 0, sizeof(hashData));

    hashData.inputLayoutHash = GenerateInputLayoutHash(inputLayout);
    hashData.renderDestHash = renderDest.GetHash();
    hashData.rasterizerStateType = static_cast<uint8_t>(rasterizerStateType);
    hashData.depthStencilStateType = static_cast<uint8_t>(depthStencilStateType);
    hashData.blendStateType = static_cast<uint8_t>(blendStateType);
    hashData.primitiveTopology = static_cast<uint8_t>(primitiveTopology);
    hashData.sampleMask = sampleMask;
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

static bool PrepareStageShader(const ShaderCompileData &compileData, RHI::ShaderStage shaderStage, const BE1::StrArray &compileDefines, RHI::Shader *&outShader) {
    assert(shaderStage < RHI::ShaderStage::Count);

    const ShaderStageSource &stageSource = compileData.stageSources[to_int(shaderStage)];
    if (stageSource.filename.IsEmpty()) {
        return false;
    }

    outShader = RHI::renderer->CreateShaderFromFile(stageSource.shaderModel, shaderStage, stageSource.filename, stageSource.entryPoint, compileDefines.Count() > 0 ? &compileDefines : nullptr);
    if (!outShader) {
        BE_FATALERROR("ShaderPipeline::PrepareStageShader: failed to compile shader '%s'\n", stageSource.filename.c_str());
        return false;
    }
    return true;
}

static bool PreparePipelineGraphicsShaders(const ShaderCompileData &compileData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    static constexpr RHI::ShaderStage GraphicsShaderStages[] = {
        RHI::ShaderStage::Vertex,
        RHI::ShaderStage::Hull,
        RHI::ShaderStage::Domain,
        RHI::ShaderStage::Geometry,
        RHI::ShaderStage::Fragment
    };

    bool hasGraphicsStage = false;

    for (RHI::ShaderStage shaderStage : GraphicsShaderStages) {
        const ShaderStageSource &stageSource = compileData.stageSources[to_int(shaderStage)];
        if (stageSource.filename.IsEmpty()) {
            continue;
        }

        hasGraphicsStage = true;

        RHI::Shader *&stageShader = stageShaders[to_int(shaderStage)];
        if (!stageShader) {
            if (!PrepareStageShader(compileData, shaderStage, compileData.compileDefines, stageShader)) {
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

static bool PreparePipelineComputeShader(const ShaderCompileData &compileData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    const ShaderStageSource &stageSource = compileData.stageSources[to_int(RHI::ShaderStage::Compute)];
    if (stageSource.filename.IsEmpty()) {
        return false;
    }

    RHI::Shader *&stageShader = stageShaders[to_int(RHI::ShaderStage::Compute)];
    if (!stageShader) {
        if (!PrepareStageShader(compileData, RHI::ShaderStage::Compute, compileData.compileDefines, stageShader)) {
            BE_WARNLOG("ShaderPipeline::PreparePipelineComputeShader: no compute shader stage\n");
            return false;
        }
    }

    return true;
}

static RHI::PipelineState *CreateGraphicsPSO(const ShaderCompileData &compileData, const GraphicsPipelineDesc &pipelineDesc, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    if (!PreparePipelineGraphicsShaders(compileData, stageShaders)) {
        return nullptr;
    }

    RHI::PipelineStateDesc psoDesc;
    RHI::RasterizerState rasterizerState;
    psoDesc.vs = stageShaders[to_int(RHI::ShaderStage::Vertex)];
    psoDesc.ps = stageShaders[to_int(RHI::ShaderStage::Fragment)];
    psoDesc.ds = stageShaders[to_int(RHI::ShaderStage::Domain)];
    psoDesc.hs = stageShaders[to_int(RHI::ShaderStage::Hull)];
    psoDesc.gs = stageShaders[to_int(RHI::ShaderStage::Geometry)];
    psoDesc.inputLayout = pipelineDesc.inputLayout;
    psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(pipelineDesc.rasterizerStateType);
    if (pipelineDesc.depthBiasOverride) {
        rasterizerState = *psoDesc.rasterizerState;
        rasterizerState.constantDepthBias = pipelineDesc.constantDepthBias;
        rasterizerState.depthBiasClamp = pipelineDesc.depthBiasClamp;
        rasterizerState.slopeScaledDepthBias = pipelineDesc.slopeScaledDepthBias;
        psoDesc.rasterizerState = &rasterizerState;
    }
    psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(pipelineDesc.depthStencilStateType);
    psoDesc.blendState = RHI::renderer->GetBlendState(pipelineDesc.blendStateType);
    psoDesc.renderDest = &pipelineDesc.renderDest;
    psoDesc.primitiveTopology = pipelineDesc.primitiveTopology;
    psoDesc.sampleMask = pipelineDesc.sampleMask;

    return RHI::renderer->CreateGraphicsPSO(&psoDesc);
}

static RHI::PipelineState *CreateComputePSO(const ShaderCompileData &compileData, RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)]) {
    if (!PreparePipelineComputeShader(compileData, stageShaders)) {
        return nullptr;
    }

    RHI::Shader *computeShader = stageShaders[to_int(RHI::ShaderStage::Compute)];
    return RHI::renderer->CreateComputePSO(computeShader);
}

void ShaderCompileData::Clear() {
    for (ShaderStageSource &stageSource : stageSources) {
        stageSource = ShaderStageSource();
    }
    compileDefines.Clear();
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

bool ShaderPipeline::PreparePSO(const ShaderCompileData &compileData, RHI::PipelineState *&outPSO) const {
    outPSO = nullptr;

    RHI::Shader *stageShaders[to_int(RHI::ShaderStage::Count)] = {};

    switch (GetType()) {
    case Type::Graphics:
        outPSO = CreateGraphicsPSO(compileData, graphicsPipelineDesc, stageShaders);
        break;
    case Type::Compute:
        outPSO = CreateComputePSO(compileData, stageShaders);
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
    return outPSO != nullptr;
}

bool ShaderPipeline::Build(const SharedPtr<ShaderTemplate> &shaderTemplate) {
    assert(!pso);

    if (!shaderTemplate) {
        return false;
    }

    ShaderCompileData compileData;
    if (!shaderTemplate->PrepareShaderCompileData(shaderPermutationKey, compileData)) {
        return false;
    }

    RHI::PipelineState *preparedPSO = nullptr;
    if (!PreparePSO(compileData, preparedPSO)) {
        return false;
    }

    BumpBuildSerial(); // 진행 중인 async completion 무효화

    SetPSO(preparedPSO);
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

    RHI::PipelineState *preparedPSO = nullptr;
    if (!PreparePSO(compileData, preparedPSO)) {
        return false;
    }

    BumpBuildSerial(); // 진행 중인 async completion 무효화
    state = State::Rebuilding;

    renderSystem->WaitAllRenderCompleted();

    SetPSO(preparedPSO);
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

    if (!loadingSystem.EnqueueShaderPipelineBuild(shared_from_this(), compileData, newSerial, LoadingSystem::LoadPriority::High)) {
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

bool ShaderPipeline::WaitLoadComplete() {
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
