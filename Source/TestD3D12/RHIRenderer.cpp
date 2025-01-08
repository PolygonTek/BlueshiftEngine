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

BE1::Image::Format::Enum RHIRenderer::GetMainRTColorFormat() const {
    return BE1::Image::Format::RGBA_8_8_8_8;
}

BE1::Image::Format::Enum RHIRenderer::GetMainRTDepthFormat() const {
    return BE1::Image::Format::Depth_32F;
}

uint32_t RHIRenderer::GetMainRTSampleCount() const {
    return 1;
}

void RHIRenderer::SetupStates() {
    RHI::RasterizerState *rs = &rasterizerStates[to_int(RHI::RasterizerStateType::SolidFrontSided)];
    rs->fillMode = RHI::FillMode::Solid;
    rs->cullMode = RHI::CullMode::Back;

    rs = &rasterizerStates[to_int(RHI::RasterizerStateType::SolidBackSided)];
    rs->fillMode = RHI::FillMode::Solid;
    rs->cullMode = RHI::CullMode::Front;

    rs = &rasterizerStates[to_int(RHI::RasterizerStateType::Wire)];
    rs->fillMode = RHI::FillMode::Wire;
    rs->cullMode = RHI::CullMode::None;

    rs = &rasterizerStates[to_int(RHI::RasterizerStateType::WireSmooth)];
    rs->fillMode = RHI::FillMode::Wire;
    rs->cullMode = RHI::CullMode::None;
    rs->smoothLineEnabled = true;

    RHI::DepthStencilState *dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::Default)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::All;
    dss->depthFunc = RHI::ComparisonFunc::LEqual;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::Never)];
    dss->depthTestEnabled = false;
    dss->depthWriteMask = RHI::DepthWriteMask::Zero;
    dss->depthFunc = RHI::ComparisonFunc::Never;

    RHI::BlendState *bs = &blendStates[to_int(RHI::BlendStateType::AlphaBlend)];
    bs->renderTargets[0].blendEnabled = true;
    bs->renderTargets[0].srcFactorColor = RHI::Blend::SrcAlpha;
    bs->renderTargets[0].destFactorColor = RHI::Blend::InvSrcAlpha;
    bs->renderTargets[0].blendOpColor = RHI::BlendOp::Add;
    bs->renderTargets[0].srcFactorAlpha = RHI::Blend::One;
    bs->renderTargets[0].destFactorAlpha = RHI::Blend::Zero;
    bs->renderTargets[0].blendOpAlpha = RHI::BlendOp::Add;

    bs = &blendStates[to_int(RHI::BlendStateType::Add)];
    bs->renderTargets[0].blendEnabled = true;
    bs->renderTargets[0].srcFactorColor = RHI::Blend::One;
    bs->renderTargets[0].destFactorColor = RHI::Blend::One;
    bs->renderTargets[0].blendOpColor = RHI::BlendOp::Add;
    bs->renderTargets[0].srcFactorAlpha = RHI::Blend::One;
    bs->renderTargets[0].destFactorAlpha = RHI::Blend::Zero;
    bs->renderTargets[0].blendOpAlpha = RHI::BlendOp::Add;
}

RHI::GPUBarrier RHIRenderer::MakeMemoryBarrier(const RHI::GPUResource *resource) {
    RHI::GPUBarrier barrier;
    barrier.type = RHI::GPUBarrier::Type::Memory;
    barrier.memoryBarrier.resource = resource;
    return barrier;
}

RHI::GPUBarrier RHIRenderer::MakeBufferBarrier(const RHI::Buffer *buffer, RHI::GPUResourceState stateBefore, RHI::GPUResourceState stateAfter) {
    RHI::GPUBarrier barrier;
    barrier.type = RHI::GPUBarrier::Type::Buffer;
    barrier.bufferBarrier.buffer = buffer;
    barrier.bufferBarrier.stateBefore = stateBefore;
    barrier.bufferBarrier.stateAfter = stateAfter;
    return barrier;
}

RHI::GPUBarrier RHIRenderer::MakeImageBarrier(const RHI::Texture *texture, RHI::GPUResourceState stateBefore, RHI::GPUResourceState stateAfter, int slice, int mipLevel) {
    RHI::GPUBarrier barrier;
    barrier.type = RHI::GPUBarrier::Type::Image;
    barrier.imageBarrier.texture = texture;
    barrier.imageBarrier.stateBefore = stateBefore;
    barrier.imageBarrier.stateAfter = stateAfter;
    barrier.imageBarrier.slice = slice;
    barrier.imageBarrier.mipLevel = mipLevel;
    return barrier;
}

RHI::GPUBarrier RHIRenderer::MakeAliasingBarrier(const RHI::GPUResource *resourceBefore, const RHI::GPUResource *resourceAfter) {
    RHI::GPUBarrier barrier;
    barrier.type = RHI::GPUBarrier::Type::Aliasing;
    barrier.aliasingBarrier.resourceBefore = resourceBefore;
    barrier.aliasingBarrier.resourceAfter = resourceAfter;
    return barrier;
}

RHI::Texture *RHIRenderer::CreateTextureFromFile(RHI::TextureType textureType, RHI::ResourceFlag flags, const char *filename, bool useCompression, bool useNormalMap) {
    BE1::Image *image = BE1::Image::NewImageFromFile(filename);
    if (!image) {
        return nullptr;
    }

    BE1::Image::Format::Enum dstFormat;
    AdjustTextureFormat(useCompression, useNormalMap, image->GetFormat(), &dstFormat);

    RHI::Texture *texture = CreateTexture(textureType, flags, image, dstFormat, true);
    delete image;

    return texture;
}

void RHIRenderer::AdjustTextureFormat(bool useCompression, bool useNormalMap, BE1::Image::Format::Enum inFormat, BE1::Image::Format::Enum *outFormat) {
    if (BE1::Image::IsDepthFormat(inFormat) || BE1::Image::IsDepthStencilFormat(inFormat)) {
        *outFormat = inFormat;
        return;
    }

    if (BE1::Image::IsCompressed(inFormat)) {
        if (IsSupportedImageFormat(inFormat)) {
            *outFormat = inFormat;
            return;
        }

        inFormat = ToUncompressedImageFormat(inFormat);
    }

    *outFormat = useCompression ? ToCompressedImageFormat(inFormat, useNormalMap) : ToUncompressedImageFormat(inFormat);
}
