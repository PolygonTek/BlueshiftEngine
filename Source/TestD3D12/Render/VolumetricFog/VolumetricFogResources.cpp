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
#include "VolumetricFogResources.h"

VolumetricFogResources::~VolumetricFogResources() {
    Destroy();
}

static RHI::Texture *CreateVolumeTexture(uint32_t width, uint32_t height, uint32_t depth, const char *debugName) {
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Init3DFromMemory(width, height, depth, 1, BE1::Image::Format::R16G16B16A16_FLOAT, BE1::Image::GammaSpace::Linear, nullptr);

    RHI::Texture *texture = RHI::renderer->CreateTexture(RHI::TextureType::Texture3D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!texture) {
        BE_WARNLOG("VolumetricFogResources::CreateVolumeTexture: failed to create %s\n", debugName);
        return nullptr;
    }

    RHI::renderer->SetName(texture, debugName);
    return texture;
}

bool VolumetricFogResources::Ensure(uint32_t width, uint32_t height, uint32_t slices, bool needed) {
    if (!needed || width == 0 || height == 0 || slices == 0) {
        Destroy();
        return false;
    }

    if (!sourceTextures[0] || !sourceTextures[1] || !integratedTexture || sourceTextures[0]->GetWidth() != width || sourceTextures[0]->GetHeight() != height || sourceTextures[0]->GetDepth() != slices) {
        Destroy();
        sourceTextures[0] = CreateVolumeTexture(width, height, slices, "VolumetricFog::SourceExtinction0");
        sourceTextures[1] = CreateVolumeTexture(width, height, slices, "VolumetricFog::SourceExtinction1");
        integratedTexture = CreateVolumeTexture(width, height, slices + 1, "VolumetricFog::Integrated");

        if (!sourceTextures[0] || !sourceTextures[1] || !integratedTexture) {
            Destroy();
            return false;
        }

        ++generationNumber;
    }

    return true;
}

void VolumetricFogResources::Destroy() {
    for (RHI::Texture *&texture : sourceTextures) {
        if (texture) {
            RHI::renderer->DestroyTexture(texture);
            texture = nullptr;
        }
    }

    if (integratedTexture) {
        RHI::renderer->DestroyTexture(integratedTexture);
        integratedTexture = nullptr;
    }

    sourceTextureIndex = 0;
}
