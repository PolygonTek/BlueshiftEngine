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

#include "RHI.h"
#include "ShaderPermutationKey.h"

class ShaderTemplate;
class LoadingSystem;

struct ShaderPipelineDesc {
    static uint64_t                 GenerateInputLayoutHash(const RHI::InputLayout *inputLayout);

    uint64_t                        GeneratePipelineDescHash() const;
    uint64_t                        GenerateGraphicsPipelineHash(ShaderPermutationKey permutationKey) const;

    const RHI::InputLayout *        inputLayout = nullptr;
    RHI::RenderDest                 renderDest;
    RHI::RasterizerStateType        rasterizerStateType = RHI::RasterizerStateType::SolidFrontSided;
    RHI::DepthStencilStateType      depthStencilStateType = RHI::DepthStencilStateType::Default;
    RHI::BlendStateType             blendStateType = RHI::BlendStateType::Disabled;
    RHI::PrimitiveTopology          primitiveTopology = RHI::PrimitiveTopology::TriangleList;
    uint32_t                        sampleMask = 0xffffffff;
};

/*
---------------------------------------------------------------------------------------------------

    ShaderPipelineState

---------------------------------------------------------------------------------------------------
*/

class ShaderPipelineState : public std::enable_shared_from_this<ShaderPipelineState> {
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

    ShaderPipelineState(Type type, ShaderPermutationKey permutationKey, const ShaderPipelineDesc &graphicsDesc);
    ShaderPipelineState(Type type, ShaderPermutationKey permutationKey);
    ~ShaderPipelineState() { Purge(); }

    Type                            GetType() const { return type; }
    State                           GetState() const { return state.load(); }
    void                            SetState(State newState) { state.store(newState); }
    bool                            IsReady() const { State s = GetState(); return s == State::Ready || s == State::Rebuilding; }
    bool                            IsBuilding() const { State s = GetState(); return s == State::Building || s == State::Rebuilding; }

    uint32_t                        GetBuildSerial() const { return buildSerial.load(); }
    uint32_t                        BumpBuildSerial() { return buildSerial.fetch_add(1) + 1; }

    ShaderPermutationKey            GetPermutationKey() const { return permutationKey; }
    const ShaderPipelineDesc &      GetGraphicsDesc() const { assert(type == Type::Graphics); return graphicsDesc; }
    RHI::PipelineState *            GetPSO() const { return pso; }
    void                            SetPSO(RHI::PipelineState *newPso);

    void                            Reset();

    void                            BuildAsync(const std::shared_ptr<ShaderTemplate> &shaderTemplate);
    void                            EnsureBuildAsync(const std::shared_ptr<ShaderTemplate> &shaderTemplate);

private:
    void                            Purge();
    bool                            EnqueueBuild(const std::shared_ptr<ShaderTemplate> &shaderTemplate);

    const Type                      type = Type::Graphics;  // 생성자에서 한번 정해지면 변하지 않는다.
    const ShaderPermutationKey      permutationKey;         // 생성자에서 한번 정해지면 변하지 않는다.
    ShaderPipelineDesc              graphicsDesc;           // 생성자에서 한번 정해지면 변하지 않는다.
    RHI::InputLayout                inputLayout;            // 생성자에서 한번 정해지면 변하지 않는다.
    std::atomic<State>              state = State::Empty;
    std::atomic<uint32_t>           buildSerial = 0;
    RHI::PipelineState *            pso = nullptr;
};

BE_INLINE ShaderPipelineState::ShaderPipelineState(Type type, ShaderPermutationKey permutationKey, const ShaderPipelineDesc &graphicsPipelineDesc) :
    type(type), permutationKey(permutationKey), graphicsDesc(graphicsPipelineDesc) {
    assert(type == Type::Graphics);

    if (graphicsPipelineDesc.inputLayout) {
        inputLayout = *graphicsPipelineDesc.inputLayout;
        graphicsDesc.inputLayout = &inputLayout;
    }
}

BE_INLINE ShaderPipelineState::ShaderPipelineState(Type type, ShaderPermutationKey permutationKey) :
    type(type), permutationKey(permutationKey) {
    assert(type == Type::Compute);
}
