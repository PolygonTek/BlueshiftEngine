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
#include "ShaderPermutationKey.h"

class ShaderTemplate;
class LoadingSystem;

struct GraphicsPipelineDesc {
    uint64_t                        GeneratePipelineDescHash() const;
    uint64_t                        GenerateGraphicsPipelineHash(ShaderPermutationKey permutationKey) const;

    static uint64_t                 GenerateInputLayoutHash(const RHI::InputLayout *inputLayout);

    const RHI::InputLayout *        inputLayout = nullptr;
    RHI::RenderDest                 renderDest;
    RHI::RasterizerStateType        rasterizerStateType = RHI::RasterizerStateType::SolidFrontSided;
    RHI::DepthStencilStateType      depthStencilStateType = RHI::DepthStencilStateType::Default;
    RHI::BlendStateType             blendStateType = RHI::BlendStateType::Disabled;
    RHI::PrimitiveTopology          primitiveTopology = RHI::PrimitiveTopology::TriangleList;
    uint32_t                        sampleMask = 0xffffffff;
};

struct ShaderStageSource {
    RHI::ShaderModel                shaderModel = RHI::ShaderModel::SM_6_0;
    BE1::Str                        filename;
    BE1::Str                        entryPoint;
};

struct ShaderCompileData {
    void                            Clear();

    ShaderStageSource               stageSources[to_int(RHI::ShaderStage::Count)] = {};
    BE1::StrArray                   compileDefines;
};

/*
---------------------------------------------------------------------------------------------------

    ShaderPipeline

    PSO (RHI::PipelineState) 를 담고 있는 파이프라인 객체
    shaderPipelineCache 에 의해 캐싱되고 재사용된다.

---------------------------------------------------------------------------------------------------
*/

class ShaderPipeline : public std::enable_shared_from_this<ShaderPipeline> {
    friend class ShaderPipelineCache;
    friend class LoadingSystem;

public:
    enum class Type : uint8_t {
        Graphics,
        Compute
    };

    enum class State : uint8_t {
        Empty,
        Building,
        Ready,
        Rebuilding,
        Failed
    };

    // Graphics pipeline ctor (type == Type::Graphics)
    ShaderPipeline(Type type, ShaderPermutationKey permutationKey, const GraphicsPipelineDesc &graphicsDesc);

    // Compute pipeline ctor (type == Type::Compute)
    ShaderPipeline(Type type, ShaderPermutationKey permutationKey);

    // dtor
    ~ShaderPipeline() { Purge(); }

    Type                            GetType() const { return type; }

    State                           GetState() const { return state.load(); }
    void                            SetState(State newState) { state.store(newState); }
    bool                            IsReady() const { State s = GetState(); return s == State::Ready || s == State::Rebuilding; }
    bool                            IsBuilding() const { State s = GetState(); return s == State::Building || s == State::Rebuilding; }

                                    // 빌드 요청 했던 시리얼 번호.
    uint32_t                        GetBuildSerial() const { return buildSerial.load(); }

                                    // 이미 진행 중인 빌딩 completion 무효화
    uint32_t                        BumpBuildSerial() { return buildSerial.fetch_add(1) + 1; }

    ShaderPermutationKey            GetShaderPermutationKey() const { return shaderPermutationKey; }
    const GraphicsPipelineDesc &    GetGraphicsPipelineDesc() const { assert(type == Type::Graphics); return graphicsPipelineDesc; }
    RHI::PipelineState *            GetPSO() const { return pso; }

    static RHI::PipelineState *     GetReadyPSO(const ShaderPipeline *shaderPipeline);
    static RHI::PipelineState *     GetReadyPSO(const SharedPtr<ShaderPipeline> &shaderPipeline);

    void                            Reset();

                                    // 최초 빌딩 중이면 완료될 때까지 기다리고, 리빌딩 중이면 기존 PSO 를 사용할 수 있으므로 Ready 로 간주한다.
    bool                            WaitReady();

                                    // 최초 빌딩과 리빌딩이 모두 완료될 때까지 기다린다.
    bool                            WaitLoadComplete();

private:
    void                            Purge();

    bool                            PreparePSO(const ShaderCompileData &compileData, RHI::PipelineState *&outPSO) const;
    void                            SetPSO(RHI::PipelineState *newPso);

    bool                            Build(const SharedPtr<ShaderTemplate> &shaderTemplate);
    void                            BuildAsync(const SharedPtr<ShaderTemplate> &shaderTemplate);
    bool                            EnqueueBuild(const SharedPtr<ShaderTemplate> &shaderTemplate);

    const Type                      type = Type::Graphics;  // 생성자에서 한번 정해지면 변하지 않는다.
    const ShaderPermutationKey      shaderPermutationKey;   // 생성자에서 한번 정해지면 변하지 않는다.
    GraphicsPipelineDesc            graphicsPipelineDesc;   // 생성자에서 한번 정해지면 변하지 않는다.
    RHI::InputLayout                inputLayout;            // 생성자에서 한번 정해지면 변하지 않는다.
    std::atomic<State>              state = State::Empty;
    std::atomic<uint32_t>           buildSerial = 0;
    RHI::PipelineState *            pso = nullptr;
};

BE_INLINE ShaderPipeline::ShaderPipeline(Type type, ShaderPermutationKey permutationKey, const GraphicsPipelineDesc &desc) :
    type(type), shaderPermutationKey(permutationKey), graphicsPipelineDesc(desc) {
    assert(type == Type::Graphics);

    if (desc.inputLayout) {
        inputLayout = *desc.inputLayout;
        graphicsPipelineDesc.inputLayout = &inputLayout;
    }
}

BE_INLINE ShaderPipeline::ShaderPipeline(Type type, ShaderPermutationKey permutationKey) :
    type(type), shaderPermutationKey(permutationKey) {
    assert(type == Type::Compute);
}

BE_INLINE RHI::PipelineState *ShaderPipeline::GetReadyPSO(const ShaderPipeline *shaderPipeline) {
    if (!shaderPipeline || !shaderPipeline->IsReady()) {
        return nullptr;
    }
    return shaderPipeline->GetPSO();
}

BE_INLINE RHI::PipelineState *ShaderPipeline::GetReadyPSO(const SharedPtr<ShaderPipeline> &shaderPipeline) {
    return GetReadyPSO(shaderPipeline.get());
}
