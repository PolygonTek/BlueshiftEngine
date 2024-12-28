// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "RHIRenderer.h"

void RHIRenderer::Init(HWND hwnd) {
    SetupStates();

    initialized = true;
}

void RHIRenderer::Shutdown() {
    initialized = false;
}

void RHIRenderer::SetupStates() {
    RasterizerState *rs = &rasterizerStates[RasterizerStateType::SolidFrontSided];
    rs->fillMode = FillMode::Solid;
    rs->cullMode = CullMode::Back;

    rs = &rasterizerStates[RasterizerStateType::SolidBackSided];
    rs->fillMode = FillMode::Solid;
    rs->cullMode = CullMode::Front;

    rs = &rasterizerStates[RasterizerStateType::Wire];
    rs->fillMode = FillMode::Wire;
    rs->cullMode = CullMode::None;

    rs = &rasterizerStates[RasterizerStateType::WireSmooth];
    rs->fillMode = FillMode::Wire;
    rs->cullMode = CullMode::None;
    rs->smoothLineEnabled = true;

    DepthStencilState *dss = &depthStencilStates[DepthStencilStateType::Default];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = DepthWriteMask::All;
    dss->depthFunc = ComparisonFunc::LEqual;

    BlendState *bs = &blendStates[BlendStateType::AlphaBlend];
    bs->renderTargets[0].blendEnabled = true;
    bs->renderTargets[0].srcFactorColor = Blend::SrcAlpha;
    bs->renderTargets[0].destFactorColor = Blend::InvSrcAlpha;
    bs->renderTargets[0].blendOpColor = BlendOp::Add;
    bs->renderTargets[0].srcFactorAlpha = Blend::One;
    bs->renderTargets[0].destFactorAlpha = Blend::Zero;
    bs->renderTargets[0].blendOpAlpha = BlendOp::Add;

    bs = &blendStates[BlendStateType::Add];
    bs->renderTargets[0].blendEnabled = true;
    bs->renderTargets[0].srcFactorColor = Blend::One;
    bs->renderTargets[0].destFactorColor = Blend::One;
    bs->renderTargets[0].blendOpColor = BlendOp::Add;
    bs->renderTargets[0].srcFactorAlpha = Blend::One;
    bs->renderTargets[0].destFactorAlpha = Blend::Zero;
    bs->renderTargets[0].blendOpAlpha = BlendOp::Add;
}

RHIRenderer::GPUBarrier RHIRenderer::MakeMemoryBarrier(const GPUResource *resource) {
    GPUBarrier barrier;
    barrier.type = GPUBarrier::Type::Memory;
    barrier.memoryBarrier.resource = resource;
    return barrier;
}

RHIRenderer::GPUBarrier RHIRenderer::MakeBufferBarrier(const RHIRenderer::Buffer *buffer, GPUResourceState::Enum stateBefore, GPUResourceState::Enum stateAfter) {
    GPUBarrier barrier;
    barrier.type = GPUBarrier::Type::Buffer;
    barrier.bufferBarrier.buffer = buffer;
    barrier.bufferBarrier.stateBefore = stateBefore;
    barrier.bufferBarrier.stateAfter = stateAfter;
    return barrier;
}

RHIRenderer::GPUBarrier RHIRenderer::MakeImageBarrier(const Texture *texture, GPUResourceState::Enum stateBefore, GPUResourceState::Enum stateAfter, int slice, int mipLevel) {
    GPUBarrier barrier;
    barrier.type = GPUBarrier::Type::Image;
    barrier.imageBarrier.texture = texture;
    barrier.imageBarrier.stateBefore = stateBefore;
    barrier.imageBarrier.stateAfter = stateAfter;
    barrier.imageBarrier.slice = slice;
    barrier.imageBarrier.mipLevel = mipLevel;
    return barrier;
}

RHIRenderer::GPUBarrier RHIRenderer::MakeAliasingBarrier(const GPUResource *resourceBefore, const GPUResource *resourceAfter) {
    GPUBarrier barrier;
    barrier.type = GPUBarrier::Type::Aliasing;
    barrier.aliasingBarrier.resourceBefore = resourceBefore;
    barrier.aliasingBarrier.resourceAfter = resourceAfter;
    return barrier;
}
