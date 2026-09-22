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
#include "ShadowAtlas.h"
#include "RenderFrameSettings.h"

bool ShadowAtlas::Ensure(const RenderFrameSettings &settings) {
    if (settings.shadowAtlasMaxViews == 0) {
        Destroy();
        return true;
    }

    const uint32_t atlasSize = settings.shadowAtlasSize;
    const uint32_t minTileSize = RenderFrameSettings::ShadowAtlasMinTileResolution;
    const uint32_t maxTileSize = settings.shadowAtlasMaxTileResolution;

    if (shadowTexture && GetSize() == atlasSize) {
        if (GetMaxTileResolution() == maxTileSize) {
            return true;
        }
        // 타일의 최대 해상도만 바뀌었다면 texture 는 유지하고 allocator 의 구성과 핸들만 갱신한다.
        return allocator.Init(atlasSize, maxTileSize, minTileSize);
    }

    SharedPtr<BE1::Image> depthImage = MakeShared<BE1::Image>();
    depthImage->Init2DFromMemory(atlasSize, atlasSize, 1, BE1::Image::Format::D32_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);

    const RHI::TextureFlag flags = RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless;
    const RHI::ClearValue clearValue = RHI::ClearValue::DepthStencil(1.0f, 0);
    RHI::Texture *newTexture = RHI::renderer->CreateTexture2D(flags, depthImage, RHI::TextureMipInitMode::FromImage, clearValue, 1, RHI::GPUResourceState::ShaderResource);
    if (!newTexture) {
        return false;
    }
    RHI::renderer->SetName(newTexture, "ShadowAtlas::shadowTexture");

    const RHI::ViewID newDSV = RHI::renderer->CreateView(newTexture, RHI::ViewType::DSV, RHI::Format::D32_FLOAT, 0, 1, 0, 1);
    const RHI::ViewID newSRV = RHI::renderer->CreateView(newTexture, RHI::ViewType::SRV, RHI::Format::R32_FLOAT, 0, 1, 0, 1);
    if (!newDSV.IsValid() || !newSRV.IsValid() || !allocator.Init(atlasSize, maxTileSize, minTileSize)) {
        RHI::renderer->DestroyTexture(newTexture);
        return false;
    }

    if (shadowTexture) {
        RHI::renderer->DestroyTexture(shadowTexture);
    }

    shadowTexture = newTexture;
    shadowDSV = newDSV;
    shadowSRV = newSRV;

    return true;
}

void ShadowAtlas::Destroy() {
    if (shadowTexture) {
        RHI::renderer->DestroyTexture(shadowTexture);
        shadowTexture = nullptr;
    }
    shadowDSV = RHI::ViewID::InvalidID;
    shadowSRV = RHI::ViewID::InvalidID;
    allocator.Shutdown();
}
