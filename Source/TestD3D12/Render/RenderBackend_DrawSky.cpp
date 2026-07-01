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
#include "RenderBackend.h"
#include "RenderInternal.h"
#include "Render/Texture/TextureManager.h"

static const Texture *GetReadyTexture(const SharedPtr<Texture> *texture) {
    if (!texture || !*texture || !(*texture)->IsReadyForRendering()) {
        return nullptr;
    }

    return texture->get();
}

static const Texture *GetReadyMaterialTexture(const Material *material, ShaderTemplate::BuiltInProperty builtInProperty) {
    if (!material) {
        return nullptr;
    }

    return GetReadyTexture(material->GetTextureProperty(builtInProperty));
}

static const Texture *GetReadyDefault2DTexture() {
    if (textureManager.defaultTexture && textureManager.defaultTexture->IsReadyForRendering()) {
        return textureManager.defaultTexture.get();
    }
    if (textureManager.whiteTexture && textureManager.whiteTexture->IsReadyForRendering()) {
        return textureManager.whiteTexture.get();
    }
    return nullptr;
}

static bool BindTexturePropertyOrDefault2D(RHI::CommandList *commandList, int shaderRegister, const Material *material, ShaderTemplate::BuiltInProperty builtInProperty) {
    const Texture *texture = GetReadyMaterialTexture(material, builtInProperty);
    if (!texture) {
        texture = GetReadyDefault2DTexture();
    }
    if (!texture) {
        return false;
    }
    if (texture->GetType() != RHI::TextureType::Texture2D) {
        BE_WARNLOG("Skybox texture '%s' is not Texture2D\n", texture->GetHashName());
        return false;
    }

    RHI::renderer->SetTexture(commandList, shaderRegister, false, texture->GetRHITexture());
    return true;
}

static bool BindSkyboxTextures(RHI::CommandList *commandList, const Material *material) {
    if (!material || !material->IsSkySurface()) {
        return true;
    }

    // SkyboxCubemap
    if (material->FindPropertyIndex(ShaderTemplate::BuiltInProperty::SkyCubeMap) >= 0) {
        const Texture *texture = GetReadyMaterialTexture(material, ShaderTemplate::BuiltInProperty::SkyCubeMap);
        if (!texture) {
            return false;
        }
        if (texture->GetType() != RHI::TextureType::TextureCube) {
            BE_WARNLOG("Skybox cubemap texture '%s' is not TextureCube\n", texture->GetHashName());
            return false;
        }

        RHI::renderer->SetTexture(commandList, 1, false, texture->GetRHITexture());
        return true;
    }

    // SkyboxSixSided
    if (material->FindPropertyIndex(ShaderTemplate::BuiltInProperty::SkyBackMap) >= 0) {
        return BindTexturePropertyOrDefault2D(commandList, 1, material, ShaderTemplate::BuiltInProperty::SkyBackMap) &&
            BindTexturePropertyOrDefault2D(commandList, 2, material, ShaderTemplate::BuiltInProperty::SkyFrontMap) &&
            BindTexturePropertyOrDefault2D(commandList, 3, material, ShaderTemplate::BuiltInProperty::SkyRightMap) &&
            BindTexturePropertyOrDefault2D(commandList, 4, material, ShaderTemplate::BuiltInProperty::SkyLeftMap) &&
            BindTexturePropertyOrDefault2D(commandList, 5, material, ShaderTemplate::BuiltInProperty::SkyTopMap) &&
            BindTexturePropertyOrDefault2D(commandList, 6, material, ShaderTemplate::BuiltInProperty::SkyDownMap);
    }

    return true;
}

void RenderBackend::DrawSkySurfaces(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSkySurfaces");

    assert(numDrawSurfs > 0);

    RHI::PipelineState *currentPSO = nullptr;
    const SubMesh *currentSubMesh = nullptr;
    VertexInputLayoutId currentInputLayoutId = VertexInputLayoutId::Count;

    for (uint32_t drawSurfIndex = 0; drawSurfIndex < numDrawSurfs; ++drawSurfIndex) {
        const PassDrawSurf *drawSurf = drawSurfs[drawSurfIndex];
        if (!drawSurf->pso) {
            continue;
        }

        if (currentPSO != drawSurf->pso) {
            currentPSO = drawSurf->pso;

            RHI::renderer->SetPSO(mainCommandList, currentPSO);
            RHI::renderer->SetConstantBuffer(mainCommandList, 0, frameConstantBuffer);
            RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
        }

        if (currentSubMesh != drawSurf->visSurf->subMesh || currentInputLayoutId != drawSurf->inputLayoutId) {
            currentSubMesh = drawSurf->visSurf->subMesh;
            currentInputLayoutId = drawSurf->inputLayoutId;

            RHI::renderer->SetVertexBuffer(mainCommandList, 0, currentSubMesh->vertexBuffer);
            RHI::renderer->SetIndexBuffer(mainCommandList, currentSubMesh->indexBuffer);
        }

        if (!BindSkyboxTextures(mainCommandList, drawSurf->visSurf->material)) {
            continue;
        }

        DrawSurfaceInstances(frameData->GetThreadData(0), mainCommandList, &drawSurfs[drawSurfIndex], 1, statDrawCount, statDrawTriangleCount, statInstanceTableAllocCount, statInstanceTableUploadBytes);
    }
}
