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
#include "ScreenSpaceAOResources.h"
#include "Render/RenderFrameSettings.h"

ScreenSpaceAOResources::~ScreenSpaceAOResources() {
    Destroy();
}

void ScreenSpaceAOResources::Ensure(uint32_t renderWidth, uint32_t renderHeight, const RenderFrameSettings &settings) {
    if (renderWidth == 0 || renderHeight == 0) {
        Destroy();
        return;
    }

    if (settings.aoMode != RenderAOMode::GTAO) {
        Destroy();
        return;
    }

    if (textures.outputTexture && (textures.outputTexture->GetWidth() != renderWidth || textures.outputTexture->GetHeight() != renderHeight)) {
        Destroy();
    }

    if (!textures.outputTexture) {
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        // L8 uses R8_UNORM storage and an (R, R, R, 1) SRV swizzle for the existing fullscreen blit.
        image->Init2DFromMemory(renderWidth, renderHeight, 1, BE1::Image::Format::L8, BE1::Image::GammaSpace::Linear, nullptr);
        textures.outputTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!textures.outputTexture) {
            BE_WARNLOG("ScreenSpaceAOResources::Ensure: failed to create AO texture\n");
            Destroy();
            return;
        }
        RHI::renderer->SetName(textures.outputTexture, "ScreenSpaceAOResources::outputTexture");
    }

    const bool useHalfResolution = settings.aoHalfResolution;
    const uint32_t aoWidth = useHalfResolution ? BE1::DivCeil(renderWidth, 2u) : renderWidth;
    const uint32_t aoHeight = useHalfResolution ? BE1::DivCeil(renderHeight, 2u) : renderHeight;
    const bool needTemp = useHalfResolution || settings.gtaoDenoisePasses > 1;

    // 해상도 옵션이 바뀌면 크기가 달라지는 중간 텍스처만 다시 만든다.
    if (textures.rawTexture && (textures.rawTexture->GetWidth() != aoWidth || textures.rawTexture->GetHeight() != aoHeight)) {
        RHI::renderer->DestroyTexture(textures.rawTexture);
        textures.rawTexture = nullptr;
    }

    if (textures.halfDepthTexture && (!useHalfResolution || textures.halfDepthTexture->GetWidth() != aoWidth || textures.halfDepthTexture->GetHeight() != aoHeight)) {
        RHI::renderer->DestroyTexture(textures.halfDepthTexture);
        textures.halfDepthTexture = nullptr;
    }

    if (textures.tempTexture && (!needTemp || textures.tempTexture->GetWidth() != aoWidth || textures.tempTexture->GetHeight() != aoHeight)) {
        DestroyFilterResources();
    }

    // Keep intermediate visibility in FP16 to avoid repeated R8 quantization.
    if (!textures.rawTexture) {
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        image->Init2DFromMemory(aoWidth, aoHeight, 1, BE1::Image::Format::R16_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
        textures.rawTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (textures.rawTexture) {
            RHI::renderer->SetName(textures.rawTexture, "ScreenSpaceAOResources::rawTexture");
        }
    }

    if (useHalfResolution && !textures.halfDepthTexture) {
        // AO 를 평가한 표면의 device depth 를 정확히 보존한다.
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        image->Init2DFromMemory(aoWidth, aoHeight, 1, BE1::Image::Format::R32_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
        textures.halfDepthTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (textures.halfDepthTexture) {
            RHI::renderer->SetName(textures.halfDepthTexture, "ScreenSpaceAOResources::halfDepthTexture");
        }
    }

    if (needTemp && textures.rawTexture && !textures.tempTexture) {
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        image->Init2DFromMemory(aoWidth, aoHeight, 1, BE1::Image::Format::R16_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
        textures.tempTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (textures.tempTexture) {
            RHI::renderer->SetName(textures.tempTexture, "ScreenSpaceAOResources::tempTexture");
        }
    }

    const BE1::Image::Format gtaoPrefilterDepthFormat = RHI::renderer->ToUAVImageFormat(settings.gtao32BitDepth ? BE1::Image::Format::R32_FLOAT : BE1::Image::Format::R16_FLOAT);
    RHI::Format gtaoPrefilterDepthRHIFormat = RHI::Format::Unknown;
    if (!RHI::Renderer::ImageFormatToRHIFormat(gtaoPrefilterDepthFormat, false, &gtaoPrefilterDepthRHIFormat)) {
        BE_WARNLOG("ScreenSpaceAOResources::Ensure: unsupported GTAO prefiltered depth format\n");
        DestroyGTAOResources();
        return;
    }

    if ((textures.gtaoPrefilterDepthTexture && (textures.gtaoPrefilterDepthTexture->GetWidth() != aoWidth || textures.gtaoPrefilterDepthTexture->GetHeight() != aoHeight ||
        textures.gtaoPrefilterDepthTexture->GetFormat() != gtaoPrefilterDepthRHIFormat)) ||
        (textures.gtaoPackedEdgesTexture && (textures.gtaoPackedEdgesTexture->GetWidth() != aoWidth || textures.gtaoPackedEdgesTexture->GetHeight() != aoHeight))) {
        DestroyGTAOResources();
    }

    if (!textures.gtaoPrefilterDepthTexture && !CreateGTAOPrefilterDepthTexture(aoWidth, aoHeight, gtaoPrefilterDepthFormat)) {
        return;
    }

    if (!textures.gtaoPackedEdgesTexture) {
        const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R8);
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        image->Init2DFromMemory(aoWidth, aoHeight, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);
        textures.gtaoPackedEdgesTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!textures.gtaoPackedEdgesTexture) {
            BE_WARNLOG("ScreenSpaceAOResources::Ensure: failed to create GTAO packed edges texture\n");
            DestroyGTAOResources();
            return;
        }
        RHI::renderer->SetName(textures.gtaoPackedEdgesTexture, "ScreenSpaceAOResources::gtaoPackedEdgesTexture");
    }
}

void ScreenSpaceAOResources::Destroy() {
    DestroyGTAOResources();
    DestroyFilterResources();

    if (textures.rawTexture) {
        RHI::renderer->DestroyTexture(textures.rawTexture);
        textures.rawTexture = nullptr;
    }

    if (textures.halfDepthTexture) {
        RHI::renderer->DestroyTexture(textures.halfDepthTexture);
        textures.halfDepthTexture = nullptr;
    }

    if (textures.outputTexture) {
        RHI::renderer->DestroyTexture(textures.outputTexture);
        textures.outputTexture = nullptr;
    }
}

void ScreenSpaceAOResources::DestroyFilterResources() {
    if (textures.tempTexture) {
        RHI::renderer->DestroyTexture(textures.tempTexture);
        textures.tempTexture = nullptr;
    }
}

bool ScreenSpaceAOResources::CreateGTAOPrefilterDepthTexture(uint32_t width, uint32_t height, BE1::Image::Format imageFormat) {
    const uint32_t maxMipCount = BE1::Image::MaxMipLevels(width, height, 1);
    const uint32_t mipCount = BE1::Min(maxMipCount, static_cast<uint32_t>(RenderGTAODepthMipMaxLevelCount));

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, mipCount, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    textures.gtaoPrefilterDepthTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!textures.gtaoPrefilterDepthTexture) {
        BE_WARNLOG("ScreenSpaceAOResources::CreateGTAOPrefilterDepthTexture: failed to create texture\n");
        DestroyGTAOResources();
        return false;
    }
    RHI::renderer->SetName(textures.gtaoPrefilterDepthTexture, "ScreenSpaceAOResources::gtaoPrefilterDepthTexture");

    for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        textures.gtaoDepthMipSRVs[mipLevel] = RHI::renderer->CreateView(textures.gtaoPrefilterDepthTexture, RHI::ViewType::SRV, RHI::Format::Auto, 0, textures.gtaoPrefilterDepthTexture->GetArraySize(), mipLevel, 1);
        textures.gtaoDepthMipUAVs[mipLevel] = RHI::renderer->CreateView(textures.gtaoPrefilterDepthTexture, RHI::ViewType::UAV, RHI::Format::Auto, 0, textures.gtaoPrefilterDepthTexture->GetArraySize(), mipLevel, 1);
        if (!textures.gtaoDepthMipSRVs[mipLevel].IsValid() || !textures.gtaoDepthMipUAVs[mipLevel].IsValid()) {
            BE_WARNLOG("ScreenSpaceAOResources::CreateGTAOPrefilterDepthTexture: failed to create mip %i views\n", mipLevel);
            DestroyGTAOResources();
            return false;
        }
    }

    textures.gtaoDepthMipCount = mipCount;
    return true;
}

void ScreenSpaceAOResources::DestroyGTAOResources() {
    if (textures.gtaoPrefilterDepthTexture) {
        RHI::renderer->DestroyTexture(textures.gtaoPrefilterDepthTexture);
        textures.gtaoPrefilterDepthTexture = nullptr;
    }

    if (textures.gtaoPackedEdgesTexture) {
        RHI::renderer->DestroyTexture(textures.gtaoPackedEdgesTexture);
        textures.gtaoPackedEdgesTexture = nullptr;
    }

    for (uint32_t mipLevel = 0; mipLevel < RenderGTAODepthMipMaxLevelCount; ++mipLevel) {
        textures.gtaoDepthMipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        textures.gtaoDepthMipUAVs[mipLevel] = RHI::ViewID::InvalidID;
    }
    textures.gtaoDepthMipCount = 0;
}
