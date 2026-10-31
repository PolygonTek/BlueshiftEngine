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
#include "SSRSceneColorHistoryResources.h"

SSRSceneColorHistoryResources::~SSRSceneColorHistoryResources() {
    Destroy();
}

bool SSRSceneColorHistoryResources::Ensure(uint32_t renderWidth, uint32_t renderHeight, RHI::Format radianceFormat, bool fullResolutionDepth, bool needed) {
    if (!needed || renderWidth == 0 || renderHeight == 0) {
        Destroy();
        return false;
    }

    // 이전 프레임의 full-resolution radiance history 를 논리적 mip 0 으로 직접 사용하므로 전용 pyramid 는 논리적 mip 1 인 반해상도부터 시작한다.
    const uint32_t pyramidWidth = BE1::Max((renderWidth + 1u) >> 1, 1u);
    const uint32_t pyramidHeight = BE1::Max((renderHeight + 1u) >> 1, 1u);

    const bool validTextures = radianceTexture && radiancePyramidTexture && depthExtremaTexture &&
        radianceTexture->GetWidth() == renderWidth && radianceTexture->GetHeight() == renderHeight && radianceTexture->GetFormat() == radianceFormat &&
        radiancePyramidTexture->GetWidth() == pyramidWidth && radiancePyramidTexture->GetHeight() == pyramidHeight &&
        depthExtremaTexture->GetWidth() == pyramidWidth && depthExtremaTexture->GetHeight() == pyramidHeight;
    if (!validTextures) {
        Destroy();
        if (!CreateTextures(renderWidth, renderHeight, radianceFormat)) {
            return false;
        }
    }

    // Full-resolution temporal validation needs the individual receiver depth, not a 2x2 extremum.
    // Keep the existing color/Hi-Z history allocation when only the trace resolution changes.
    if (fullResolutionDepth && !fullResolutionDepthTexture) {
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        image->Init2DFromMemory(renderWidth, renderHeight, 1, BE1::Image::Format::R32_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
        fullResolutionDepthTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess,
            image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!fullResolutionDepthTexture) {
            BE_WARNLOG("SSRSceneColorHistoryResources::Ensure: failed to create full-resolution depth history texture\n");
            Destroy();
            return false;
        }
        RHI::renderer->SetName(fullResolutionDepthTexture, "SSRSceneColorHistoryResources::fullResolutionDepthTexture");
        valid = false;
    } else if (!fullResolutionDepth && fullResolutionDepthTexture) {
        RHI::renderer->DestroyTexture(fullResolutionDepthTexture);
        fullResolutionDepthTexture = nullptr;
    }

    return true;
}

bool SSRSceneColorHistoryResources::CreateTextures(uint32_t width, uint32_t height, RHI::Format radianceFormat) {
    BE1::Image::Format radianceImageFormat = BE1::Image::Format::Unknown;
    if (!RHI::Renderer::RHIFormatToImageFormat(radianceFormat, &radianceImageFormat, nullptr)) {
        BE_WARNLOG("SSRSceneColorHistoryResources::CreateTextures: failed to convert radiance format\n");
        return false;
    }

    SharedPtr<BE1::Image> radianceImage = MakeShared<BE1::Image>();
    radianceImage->Init2DFromMemory(width, height, 1, radianceImageFormat, BE1::Image::GammaSpace::Linear, nullptr);
    radianceTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource, radianceImage, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!radianceTexture) {
        BE_WARNLOG("SSRSceneColorHistoryResources::CreateTextures: failed to create radiance history texture\n");
        Destroy();
        return false;
    }
    RHI::renderer->SetName(radianceTexture, "SSRSceneColorHistoryResources::radianceTexture");

    const uint32_t pyramidWidth = BE1::Max((width + 1u) >> 1, 1u);
    const uint32_t pyramidHeight = BE1::Max((height + 1u) >> 1, 1u);
    uint32_t levelCount = 0;
    uint32_t mipWidth = pyramidWidth;
    uint32_t mipHeight = pyramidHeight;

    while (levelCount < RenderSSRSceneColorHistoryMaxLevelCount) {
        mipWidths[levelCount] = mipWidth;
        mipHeights[levelCount] = mipHeight;
        levelCount++;

        if (mipWidth == 1 && mipHeight == 1) {
            break;
        }

        mipWidth = mipWidth > 1 ? mipWidth >> 1 : 1;
        mipHeight = mipHeight > 1 ? mipHeight >> 1 : 1;
    }

    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R16G16B16A16_FLOAT);
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(pyramidWidth, pyramidHeight, levelCount, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    radiancePyramidTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!radiancePyramidTexture) {
        BE_WARNLOG("SSRSceneColorHistoryResources::CreateTextures: failed to create radiance pyramid texture\n");
        Destroy();
        return false;
    }

    RHI::renderer->SetName(radiancePyramidTexture, "SSRSceneColorHistoryResources::radiancePyramidTexture");

    // SSR Hi-Z mip 0 과 같은 2x2 depth block 의 min/max 범위를 보존해 이전 프레임 hit 의 가시성을 검증한다.
    const BE1::Image::Format depthExtremaImageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R32G32_FLOAT);
    SharedPtr<BE1::Image> depthExtremaImage = MakeShared<BE1::Image>();
    depthExtremaImage->Init2DFromMemory(pyramidWidth, pyramidHeight, 1, depthExtremaImageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    depthExtremaTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource, depthExtremaImage, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!depthExtremaTexture) {
        BE_WARNLOG("SSRSceneColorHistoryResources::CreateTextures: failed to create depth extrema history texture\n");
        Destroy();
        return false;
    }
    RHI::renderer->SetName(depthExtremaTexture, "SSRSceneColorHistoryResources::depthExtremaTexture");

    for (uint32_t mipLevel = 0; mipLevel < levelCount; ++mipLevel) {
        mipSRVs[mipLevel] = RHI::renderer->CreateView(radiancePyramidTexture, RHI::ViewType::SRV, RHI::Format::Auto, 0, radiancePyramidTexture->GetArraySize(), mipLevel, 1);
        if (!mipSRVs[mipLevel].IsValid()) {
            BE_WARNLOG("SSRSceneColorHistoryResources::CreateTextures: failed to create mip %i SRV\n", mipLevel);
            Destroy();
            return false;
        }

        mipUAVs[mipLevel] = RHI::renderer->CreateView(radiancePyramidTexture, RHI::ViewType::UAV, RHI::Format::Auto, 0, radiancePyramidTexture->GetArraySize(), mipLevel, 1);
        if (!mipUAVs[mipLevel].IsValid()) {
            BE_WARNLOG("SSRSceneColorHistoryResources::CreateTextures: failed to create mip %i UAV\n", mipLevel);
            Destroy();
            return false;
        }
    }

    mipCount = levelCount;
    return true;
}

void SSRSceneColorHistoryResources::Destroy() {
    if (radianceTexture) {
        RHI::renderer->DestroyTexture(radianceTexture);
        radianceTexture = nullptr;
    }
    if (radiancePyramidTexture) {
        RHI::renderer->DestroyTexture(radiancePyramidTexture);
        radiancePyramidTexture = nullptr;
    }
    if (depthExtremaTexture) {
        RHI::renderer->DestroyTexture(depthExtremaTexture);
        depthExtremaTexture = nullptr;
    }

    if (fullResolutionDepthTexture) {
        RHI::renderer->DestroyTexture(fullResolutionDepthTexture);
        fullResolutionDepthTexture = nullptr;
    }

    for (uint32_t mipLevel = 0; mipLevel < RenderSSRSceneColorHistoryMaxLevelCount; ++mipLevel) {
        mipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        mipUAVs[mipLevel] = RHI::ViewID::InvalidID;
        mipWidths[mipLevel] = 0;
        mipHeights[mipLevel] = 0;
    }
    mipCount = 0;
    valid = false;
}
