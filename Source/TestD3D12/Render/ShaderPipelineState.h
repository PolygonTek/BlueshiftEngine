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

struct ShaderPermutationKey {
    ShaderPermutationKey() = default;
    explicit ShaderPermutationKey(uint64_t hash) : hash(hash) {}

    bool                            operator==(const ShaderPermutationKey &rhs) const { return hash == rhs.hash; }
    bool                            operator!=(const ShaderPermutationKey &rhs) const { return hash != rhs.hash; }

    static ShaderPermutationKey     FromDefines(std::initializer_list<const char *> defines);
    static ShaderPermutationKey     FromDefines(const BE1::StrArray &defines);

    uint64_t                        hash = 0;
};

struct ShaderPipelineDesc {
    ShaderPermutationKey            permutationKey;
    const RHI::InputLayout *        inputLayout = nullptr;
    RHI::RenderDest                 renderDest;
    RHI::RasterizerStateType        rasterizerStateType = RHI::RasterizerStateType::SolidFrontSided;
    RHI::DepthStencilStateType      depthStencilStateType = RHI::DepthStencilStateType::Default;
    RHI::BlendStateType             blendStateType = RHI::BlendStateType::Opaque;
    RHI::PrimitiveTopology          primitiveTopology = RHI::PrimitiveTopology::TriangleList;
    uint32_t                        sampleMask = 0xffffffff;
};

/*
-------------------------------------------------------------------------------

    ShaderPipelineState

-------------------------------------------------------------------------------
*/

class ShaderPipelineState : public std::enable_shared_from_this<ShaderPipelineState> {
    friend class ShaderTemplate;

public:
    enum class Type : uint8_t {
        Graphics,
        Compute
    };

    enum class State : uint8_t {
        Empty,
        Building,
        Ready,
        Failed
    };

    ~ShaderPipelineState() { Purge(); }

    Type                            GetType() const { return type; }
    State                           GetState() const { return state.load(); }
    bool                            IsReady() const { return GetState() == State::Ready; }

    uint32_t                        GetBuildSerial() const { return buildSerial.load(); }
    uint32_t                        BumpBuildSerial() { return buildSerial.fetch_add(1) + 1; }

    uint64_t                        GetHash() const { return hash; }
    ShaderPermutationKey            GetPermutationKey() const { return permutationKey; }
    const ShaderPipelineDesc &      GetGraphicsDesc() const { assert(type == Type::Graphics); return graphicsDesc; }
    RHI::PipelineState *            GetPSO() const { return pso; }

    void                            RequestBuild(const std::shared_ptr<ShaderTemplate> &shaderTemplate);
    void                            ResetAndRequestBuild(const std::shared_ptr<ShaderTemplate> &shaderTemplate);

                                    // 실질적인 shader compile & PSO 생성 로직이다. 로딩 스레드에서 호출된다.
    bool                            Build(ShaderTemplate &shaderTemplate, uint32_t requestedBuildSerial);

private:
    ShaderPipelineState(Type type, uint64_t hash, const ShaderPipelineDesc &graphicsDesc);
    ShaderPipelineState(Type type, uint64_t hash, ShaderPermutationKey permutationKey);

    void                            Purge();
    void                            Reset();

    Type                            type = Type::Graphics;
    std::atomic<State>              state = State::Empty;
    std::atomic<uint32_t>           buildSerial = 0;
    uint64_t                        hash = 0;
    ShaderPermutationKey            permutationKey;
    ShaderPipelineDesc              graphicsDesc;
    RHI::InputLayout                inputLayout;
    RHI::PipelineState *            pso = nullptr;
};

BE_INLINE ShaderPipelineState::ShaderPipelineState(Type type, uint64_t hash, const ShaderPipelineDesc &graphicsPipelineDesc) :
    type(type), hash(hash), permutationKey(graphicsPipelineDesc.permutationKey), graphicsDesc(graphicsPipelineDesc) {
    assert(type == Type::Graphics);

    if (graphicsPipelineDesc.inputLayout) {
        inputLayout = *graphicsPipelineDesc.inputLayout;
        graphicsDesc.inputLayout = &inputLayout;
    }
}

BE_INLINE ShaderPipelineState::ShaderPipelineState(Type type, uint64_t hash, ShaderPermutationKey permutationKey) :
    type(type), hash(hash), permutationKey(permutationKey) {
    assert(type == Type::Compute);
}
