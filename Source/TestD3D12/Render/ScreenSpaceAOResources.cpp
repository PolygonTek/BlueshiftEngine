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

    if (textureSet.outputTexture && (textureSet.outputTexture->GetWidth() != renderWidth || textureSet.outputTexture->GetHeight() != renderHeight)) {
        Destroy();
    }

    if (!textureSet.outputTexture) {
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        // L8 uses R8_UNORM storage and an (R, R, R, 1) SRV swizzle for the existing fullscreen blit.
        image->Init2DFromMemory(renderWidth, renderHeight, 1, BE1::Image::Format::L8, BE1::Image::GammaSpace::Linear, nullptr);
        textureSet.outputTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!textureSet.outputTexture) {
            BE_WARNLOG("ScreenSpaceAOResources::Ensure: failed to create AO texture\n");
            Destroy();
            return;
        }
        RHI::renderer->SetName(textureSet.outputTexture, "ScreenSpaceAOResources::outputTexture");
    }

    const bool useHalfResolution = settings.aoHalfResolution;
    const uint32_t aoWidth = useHalfResolution ? BE1::DivCeil(renderWidth, 2u) : renderWidth;
    const uint32_t aoHeight = useHalfResolution ? BE1::DivCeil(renderHeight, 2u) : renderHeight;
    const bool needTemp = useHalfResolution || settings.gtaoDenoisePasses > 1;

    // 해상도 옵션이 바뀌면 크기가 달라지는 중간 텍스처만 다시 만든다.
    if (textureSet.rawTexture && (textureSet.rawTexture->GetWidth() != aoWidth || textureSet.rawTexture->GetHeight() != aoHeight)) {
        RHI::renderer->DestroyTexture(textureSet.rawTexture);
        textureSet.rawTexture = nullptr;
    }

    if (textureSet.halfDepthTexture && (!useHalfResolution || textureSet.halfDepthTexture->GetWidth() != aoWidth || textureSet.halfDepthTexture->GetHeight() != aoHeight)) {
        RHI::renderer->DestroyTexture(textureSet.halfDepthTexture);
        textureSet.halfDepthTexture = nullptr;
    }

    if (textureSet.tempTexture && (!needTemp || textureSet.tempTexture->GetWidth() != aoWidth || textureSet.tempTexture->GetHeight() != aoHeight)) {
        DestroyFilterResources();
    }

    // Keep intermediate visibility in FP16 to avoid repeated R8 quantization.
    if (!textureSet.rawTexture) {
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        image->Init2DFromMemory(aoWidth, aoHeight, 1, BE1::Image::Format::R16_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
        textureSet.rawTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (textureSet.rawTexture) {
            RHI::renderer->SetName(textureSet.rawTexture, "ScreenSpaceAOResources::rawTexture");
        }
    }

    if (useHalfResolution && !textureSet.halfDepthTexture) {
        // AO 를 평가한 표면의 device depth 를 정확히 보존한다.
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        image->Init2DFromMemory(aoWidth, aoHeight, 1, BE1::Image::Format::R32_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
        textureSet.halfDepthTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (textureSet.halfDepthTexture) {
            RHI::renderer->SetName(textureSet.halfDepthTexture, "ScreenSpaceAOResources::halfDepthTexture");
        }
    }

    if (needTemp && textureSet.rawTexture && !textureSet.tempTexture) {
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        image->Init2DFromMemory(aoWidth, aoHeight, 1, BE1::Image::Format::R16_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);
        textureSet.tempTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (textureSet.tempTexture) {
            RHI::renderer->SetName(textureSet.tempTexture, "ScreenSpaceAOResources::tempTexture");
        }
    }

    const BE1::Image::Format gtaoPrefilterDepthFormat = RHI::renderer->ToUAVImageFormat(settings.gtao32BitDepth ? BE1::Image::Format::R32_FLOAT : BE1::Image::Format::R16_FLOAT);
    RHI::Format gtaoPrefilterDepthRHIFormat = RHI::Format::Unknown;
    if (!RHI::Renderer::ImageFormatToRHIFormat(gtaoPrefilterDepthFormat, false, &gtaoPrefilterDepthRHIFormat)) {
        BE_WARNLOG("ScreenSpaceAOResources::Ensure: unsupported GTAO prefiltered depth format\n");
        DestroyGTAOResources();
        return;
    }

    if ((textureSet.gtaoPrefilterDepthTexture && (textureSet.gtaoPrefilterDepthTexture->GetWidth() != aoWidth || textureSet.gtaoPrefilterDepthTexture->GetHeight() != aoHeight ||
        textureSet.gtaoPrefilterDepthTexture->GetFormat() != gtaoPrefilterDepthRHIFormat)) ||
        (textureSet.gtaoPackedEdgesTexture && (textureSet.gtaoPackedEdgesTexture->GetWidth() != aoWidth || textureSet.gtaoPackedEdgesTexture->GetHeight() != aoHeight))) {
        DestroyGTAOResources();
    }

    if (!textureSet.gtaoPrefilterDepthTexture && !CreateGTAOPrefilterDepthTexture(aoWidth, aoHeight, gtaoPrefilterDepthFormat)) {
        return;
    }

    if (!textureSet.gtaoPackedEdgesTexture) {
        const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R8);
        SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
        image->Init2DFromMemory(aoWidth, aoHeight, 1, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);
        textureSet.gtaoPackedEdgesTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!textureSet.gtaoPackedEdgesTexture) {
            BE_WARNLOG("ScreenSpaceAOResources::Ensure: failed to create GTAO packed edges texture\n");
            DestroyGTAOResources();
            return;
        }
        RHI::renderer->SetName(textureSet.gtaoPackedEdgesTexture, "ScreenSpaceAOResources::gtaoPackedEdgesTexture");
    }
}

void ScreenSpaceAOResources::Destroy() {
    DestroyGTAOResources();
    DestroyFilterResources();

    if (textureSet.rawTexture) {
        RHI::renderer->DestroyTexture(textureSet.rawTexture);
        textureSet.rawTexture = nullptr;
    }

    if (textureSet.halfDepthTexture) {
        RHI::renderer->DestroyTexture(textureSet.halfDepthTexture);
        textureSet.halfDepthTexture = nullptr;
    }

    if (textureSet.outputTexture) {
        RHI::renderer->DestroyTexture(textureSet.outputTexture);
        textureSet.outputTexture = nullptr;
    }
}

void ScreenSpaceAOResources::DestroyFilterResources() {
    if (textureSet.tempTexture) {
        RHI::renderer->DestroyTexture(textureSet.tempTexture);
        textureSet.tempTexture = nullptr;
    }
}

bool ScreenSpaceAOResources::CreateGTAOPrefilterDepthTexture(uint32_t width, uint32_t height, BE1::Image::Format imageFormat) {
    const uint32_t maxMipCount = BE1::Image::MaxMipLevels(width, height, 1);
    const uint32_t mipCount = BE1::Min(maxMipCount, static_cast<uint32_t>(RenderGTAODepthMipMaxLevelCount));

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init2DFromMemory(width, height, mipCount, imageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    textureSet.gtaoPrefilterDepthTexture = RHI::renderer->CreateTexture2D(RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!textureSet.gtaoPrefilterDepthTexture) {
        BE_WARNLOG("ScreenSpaceAOResources::CreateGTAOPrefilterDepthTexture: failed to create texture\n");
        DestroyGTAOResources();
        return false;
    }
    RHI::renderer->SetName(textureSet.gtaoPrefilterDepthTexture, "ScreenSpaceAOResources::gtaoPrefilterDepthTexture");

    for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel) {
        textureSet.gtaoDepthMipSRVs[mipLevel] = RHI::renderer->CreateView(textureSet.gtaoPrefilterDepthTexture, RHI::ViewType::SRV, RHI::Format::Auto, 0, textureSet.gtaoPrefilterDepthTexture->GetArraySize(), mipLevel, 1);
        textureSet.gtaoDepthMipUAVs[mipLevel] = RHI::renderer->CreateView(textureSet.gtaoPrefilterDepthTexture, RHI::ViewType::UAV, RHI::Format::Auto, 0, textureSet.gtaoPrefilterDepthTexture->GetArraySize(), mipLevel, 1);
        if (!textureSet.gtaoDepthMipSRVs[mipLevel].IsValid() || !textureSet.gtaoDepthMipUAVs[mipLevel].IsValid()) {
            BE_WARNLOG("ScreenSpaceAOResources::CreateGTAOPrefilterDepthTexture: failed to create mip %i views\n", mipLevel);
            DestroyGTAOResources();
            return false;
        }
    }

    textureSet.gtaoDepthMipCount = mipCount;
    return true;
}

void ScreenSpaceAOResources::DestroyGTAOResources() {
    if (textureSet.gtaoPrefilterDepthTexture) {
        RHI::renderer->DestroyTexture(textureSet.gtaoPrefilterDepthTexture);
        textureSet.gtaoPrefilterDepthTexture = nullptr;
    }

    if (textureSet.gtaoPackedEdgesTexture) {
        RHI::renderer->DestroyTexture(textureSet.gtaoPackedEdgesTexture);
        textureSet.gtaoPackedEdgesTexture = nullptr;
    }

    for (uint32_t mipLevel = 0; mipLevel < RenderGTAODepthMipMaxLevelCount; ++mipLevel) {
        textureSet.gtaoDepthMipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        textureSet.gtaoDepthMipUAVs[mipLevel] = RHI::ViewID::InvalidID;
    }
    textureSet.gtaoDepthMipCount = 0;
}
