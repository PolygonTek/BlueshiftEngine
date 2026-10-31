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
#include "HiZResources.h"
#include "Shaders/ShaderInterop.h"

HiZResources::~HiZResources() {
    Destroy();
}

void HiZResources::Ensure(uint32_t renderWidth, uint32_t renderHeight, bool needHiZTexture, bool needHiZCullResources, bool needHiZDebugPreview) {
    assert(!needHiZCullResources || needHiZTexture);
    assert(!needHiZDebugPreview || needHiZTexture);

    if (!needHiZTexture || renderWidth == 0 || renderHeight == 0) {
        Destroy();
        return;
    }

    if (!texture || texture->GetWidth() != renderWidth || texture->GetHeight() != renderHeight) {
        DestroyDebugPreviewTexture();
        DestroyTexture();
        if (!CreateTexture(renderWidth, renderHeight)) {
            DestroyCullResources();
            return;
        }
    }

    if (needHiZCullResources) {
        if ((!visibleInstanceBuffer || !indirectArgsBuffer) && !CreateCullResources()) {
            DestroyCullResources();
        }
    } else {
        DestroyCullResources();
    }

    if (needHiZDebugPreview) {
        EnsureDebugPreviewTexture(renderWidth, renderHeight);
    } else {
        DestroyDebugPreviewTexture();
    }
}

bool HiZResources::CreateTexture(uint32_t width, uint32_t height) {
    uint32_t hiZLevelCount = 0;
    uint32_t w = width;
    uint32_t h = height;

    while (hiZLevelCount < RenderHiZMaxLevelCount) {
        mipWidths[hiZLevelCount] = w;
        mipHeights[hiZLevelCount] = h;
        hiZLevelCount++;

        if (w == 1 && h == 1) {
            break;
        }

        w = w > 1 ? w >> 1 : 1;
        h = h > 1 ? h >> 1 : 1;
    }

    const BE1::Image::Format hiZImageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R32_FLOAT);
    SharedPtr<BE1::Image> hiZImage = MakeShared<BE1::Image>();
    hiZImage->Init2DFromMemory(width, height, hiZLevelCount, hiZImageFormat, BE1::Image::GammaSpace::Linear, nullptr);

    auto createHiZMipViews = [](RHI::Texture *texture, RHI::ViewID *srvViewIDs, RHI::ViewID *uavViewIDs, uint32_t levelCount) -> bool {
        for (uint32_t mipLevel = 0; mipLevel < levelCount; ++mipLevel) {
            srvViewIDs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::SRV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
            if (!srvViewIDs[mipLevel].IsValid()) {
                BE_WARNLOG("HiZResources::CreateTexture: failed to create Hi-Z texture mip %i SRV\n", mipLevel);
                return false;
            }

            uavViewIDs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::UAV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
            if (!uavViewIDs[mipLevel].IsValid()) {
                BE_WARNLOG("HiZResources::CreateTexture: failed to create Hi-Z texture mip %i UAV\n", mipLevel);
                return false;
            }
        }

        return true;
    };

    texture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, hiZImage, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!texture) {
        BE_WARNLOG("HiZResources::CreateTexture: failed to create Hi-Z texture\n");
        DestroyTexture();
        return false;
    }

    RHI::renderer->SetName(texture, "HiZResources::texture");

    if (!createHiZMipViews(texture, mipSRVs, mipUAVs, hiZLevelCount)) {
        DestroyTexture();
        return false;
    }
    mipCount = hiZLevelCount;

    return true;
}

void HiZResources::DestroyTexture() {
    if (texture) {
        RHI::renderer->DestroyTexture(texture);
        texture = nullptr;
    }

    for (uint32_t mipLevel = 0; mipLevel < RenderHiZMaxLevelCount; ++mipLevel) {
        mipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        mipUAVs[mipLevel] = RHI::ViewID::InvalidID;
        mipWidths[mipLevel] = 0;
        mipHeights[mipLevel] = 0;
    }
    mipCount = 0;
    historyValid = false;
}

void HiZResources::Destroy() {
    DestroyDebugPreviewTexture();
    DestroyCullResources();
    DestroyTexture();
}

bool HiZResources::EnsureDebugPreviewTexture(uint32_t width, uint32_t height) {
    if (debugPreviewTexture && debugPreviewTexture->GetWidth() == width && debugPreviewTexture->GetHeight() == height) {
        return true;
    }

    DestroyDebugPreviewTexture();
    return CreateDebugPreviewTexture(width, height);
}

bool HiZResources::CreateDebugPreviewTexture(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return false;
    }

    BE1::Image::Format debugPreviewImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(RHI::Format::R8G8B8A8_UNORM, &debugPreviewImageFormat, nullptr);

    SharedPtr<BE1::Image> debugPreviewImage = MakeShared<BE1::Image>();
    debugPreviewImage->Init2DFromMemory(width, height, 1, debugPreviewImageFormat, BE1::Image::GammaSpace::Linear, nullptr);
    debugPreviewTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, debugPreviewImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!debugPreviewTexture) {
        BE_WARNLOG("HiZResources::CreateDebugPreviewTexture: failed to create Hi-Z debug preview texture\n");
        return false;
    }

    RHI::renderer->SetName(debugPreviewTexture, "HiZResources::debugPreviewTexture");
    return true;
}

void HiZResources::DestroyDebugPreviewTexture() {
    if (debugPreviewTexture) {
        RHI::renderer->DestroyTexture(debugPreviewTexture);
        debugPreviewTexture = nullptr;
    }
}

bool HiZResources::CreateCullResources() {
    DestroyCullResources();

    const uint64_t visibleInstanceBufferSize = uint64_t(RenderHiZCullMaxVisibleInstanceCount) * sizeof(uint32_t);
    visibleInstanceBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, visibleInstanceBufferSize, nullptr);
    if (!visibleInstanceBuffer) {
        BE_WARNLOG("HiZResources::CreateCullResources: failed to create visible instance buffer\n");
        DestroyCullResources();
        return false;
    }

    RHI::renderer->SetName(visibleInstanceBuffer, "HiZResources::visibleInstanceBuffer");

    RHI::ViewID visibleInstanceSRV = RHI::renderer->CreateView(visibleInstanceBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(uint32_t), 0, visibleInstanceBufferSize);
    if (!visibleInstanceSRV.IsValid()) {
        BE_WARNLOG("HiZResources::CreateCullResources: failed to create visible instance SRV\n");
        DestroyCullResources();
        return false;
    }

    RHI::ViewID visibleInstanceUAV = RHI::renderer->CreateView(visibleInstanceBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, sizeof(uint32_t), 0, visibleInstanceBufferSize);
    if (!visibleInstanceUAV.IsValid()) {
        BE_WARNLOG("HiZResources::CreateCullResources: failed to create visible instance UAV\n");
        DestroyCullResources();
        return false;
    }

    const uint64_t indirectArgsBufferSize = uint64_t(RenderHiZCullMaxBatchCount) * HiZCullIndirectArgSize;
    indirectArgsBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::UnorderedAccess | RHI::BufferFlag::IndirectBuffer, indirectArgsBufferSize, nullptr);
    if (!indirectArgsBuffer) {
        BE_WARNLOG("HiZResources::CreateCullResources: failed to create indirect args buffer\n");
        DestroyCullResources();
        return false;
    }

    RHI::renderer->SetName(indirectArgsBuffer, "HiZResources::indirectArgsBuffer");

    RHI::ViewID indirectArgsUAV = RHI::renderer->CreateView(indirectArgsBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, 0, 0, indirectArgsBufferSize);
    if (!indirectArgsUAV.IsValid()) {
        BE_WARNLOG("HiZResources::CreateCullResources: failed to create indirect args UAV\n");
        DestroyCullResources();
        return false;
    }

    return true;
}

void HiZResources::DestroyCullResources() {
    if (visibleInstanceBuffer) {
        RHI::renderer->DestroyBuffer(visibleInstanceBuffer);
        visibleInstanceBuffer = nullptr;
    }
    if (indirectArgsBuffer) {
        RHI::renderer->DestroyBuffer(indirectArgsBuffer);
        indirectArgsBuffer = nullptr;
    }
}
