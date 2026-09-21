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
#include "ScreenSpaceDepthPyramidResources.h"

ScreenSpaceDepthPyramidResources::~ScreenSpaceDepthPyramidResources() {
    Destroy();
}

void ScreenSpaceDepthPyramidResources::Ensure(uint32_t renderWidth, uint32_t renderHeight, bool needed) {
    if (!needed || renderWidth == 0 || renderHeight == 0) {
        Destroy();
        return;
    }

    // newValidWidth, newValidHeight 는 원본 depth 를 2x2 축소한 mip 0 의 실제 데이터 영역이다.
    const uint32_t newValidWidth = BE1::Max((renderWidth + 1) >> 1, 1u);
    const uint32_t newValidHeight = BE1::Max((renderHeight + 1) >> 1, 1u);

    // pyramidWidth, pyramidHeight 는 valid 영역을 좌상단에 포함하도록 power-of-two 로 올린 텍스처 할당 크기다.
    const uint32_t pyramidWidth = BE1::Math::RoundUpPowerOfTwo(newValidWidth);
    const uint32_t pyramidHeight = BE1::Math::RoundUpPowerOfTwo(newValidHeight);

    if (texture &&
        texture->GetWidth() == pyramidWidth &&
        texture->GetHeight() == pyramidHeight) {
        validWidth = newValidWidth;
        validHeight = newValidHeight;
        return;
    }

    Destroy();

    validWidth = newValidWidth;
    validHeight = newValidHeight;

    CreateTexture(pyramidWidth, pyramidHeight);
}

bool ScreenSpaceDepthPyramidResources::CreateTexture(uint32_t width, uint32_t height) {
    uint32_t levelCount = 0;
    uint32_t mipWidth = width;
    uint32_t mipHeight = height;

    while (levelCount < RenderScreenSpaceDepthPyramidMaxLevelCount) {
        mipWidths[levelCount] = mipWidth;
        mipHeights[levelCount] = mipHeight;
        levelCount++;

        if (mipWidth == 1 && mipHeight == 1) {
            break;
        }

        mipWidth = mipWidth > 1 ? mipWidth >> 1 : 1;
        mipHeight = mipHeight > 1 ? mipHeight >> 1 : 1;
    }

    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R32G32_FLOAT);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, levelCount, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    texture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!texture) {
        BE_WARNLOG("ScreenSpaceDepthPyramidResources::CreateTexture: failed to create depth pyramid texture\n");
        Destroy();
        return false;
    }

    RHI::renderer->SetName(texture, "ScreenSpaceDepthPyramidResources::texture");

    for (uint32_t mipLevel = 0; mipLevel < levelCount; ++mipLevel) {
        mipSRVs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::SRV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
        if (!mipSRVs[mipLevel].IsValid()) {
            BE_WARNLOG("ScreenSpaceDepthPyramidResources::CreateTexture: failed to create mip %i SRV\n", mipLevel);
            Destroy();
            return false;
        }

        mipUAVs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::UAV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
        if (!mipUAVs[mipLevel].IsValid()) {
            BE_WARNLOG("ScreenSpaceDepthPyramidResources::CreateTexture: failed to create mip %i UAV\n", mipLevel);
            Destroy();
            return false;
        }
    }

    mipCount = levelCount;
    return true;
}

void ScreenSpaceDepthPyramidResources::Destroy() {
    if (texture) {
        RHI::renderer->DestroyTexture(texture);
        texture = nullptr;
    }

    for (uint32_t mipLevel = 0; mipLevel < RenderScreenSpaceDepthPyramidMaxLevelCount; ++mipLevel) {
        mipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        mipUAVs[mipLevel] = RHI::ViewID::InvalidID;
        mipWidths[mipLevel] = 0;
        mipHeights[mipLevel] = 0;
    }
    mipCount = 0;
    validWidth = 0;
    validHeight = 0;
}
