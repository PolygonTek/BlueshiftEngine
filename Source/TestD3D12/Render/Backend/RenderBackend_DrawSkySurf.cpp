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
#include "Render/RenderInternal.h"
#include "Render/RenderFrame.h"
#include "Render/Texture/TextureManager.h"

static RHI::Texture *GetReadyMaterialRHITexture(const Material *material, ShaderTemplate::StandardProperty standardProperty) {
    if (!material) {
        return nullptr;
    }

    const SharedPtr<Texture> *texture = material->GetPropertyTexture(standardProperty);
    return texture && *texture ? (*texture)->GetReadyRHITexture() : nullptr;
}

static bool BindTextureProperty(RHI::CommandList *commandList, uint32_t shaderRegister, const Material *material, ShaderTemplate::StandardProperty standardProperty) {
    RHI::Texture *texture = GetReadyMaterialRHITexture(material, standardProperty);
    if (!texture) {
        texture = textureManager.missingTexture->GetRHITexture();
    }

    if (texture->type != RHI::TextureType::Texture2D) {
        BE_WARNLOG("Skybox texture is not Texture2D\n");
        return false;
    }

    RHI::renderer->SetTexture(commandList, shaderRegister, false, texture);
    return true;
}

static bool BindSkyboxTextures(RHI::CommandList *commandList, const Material *material) {
    if (!material || !material->IsSkySurface()) {
        return true;
    }

    // SkyboxCubemap
    if (material->FindPropertyIndex(ShaderTemplate::StandardProperty::SkyCubeMap) >= 0) {
        RHI::Texture *texture = GetReadyMaterialRHITexture(material, ShaderTemplate::StandardProperty::SkyCubeMap);
        if (!texture) {
            return false;
        }

        if (texture->type != RHI::TextureType::TextureCube) {
            BE_WARNLOG("Skybox cubemap texture is not TextureCube\n");
            return false;
        }

        RHI::renderer->SetTexture(commandList, 1, false, texture);
        return true;
    }

    // SkyboxSixSided
    if (material->FindPropertyIndex(ShaderTemplate::StandardProperty::SkyBackMap) >= 0) {
        return BindTextureProperty(commandList, 1, material, ShaderTemplate::StandardProperty::SkyBackMap) &&
            BindTextureProperty(commandList, 2, material, ShaderTemplate::StandardProperty::SkyFrontMap) &&
            BindTextureProperty(commandList, 3, material, ShaderTemplate::StandardProperty::SkyRightMap) &&
            BindTextureProperty(commandList, 4, material, ShaderTemplate::StandardProperty::SkyLeftMap) &&
            BindTextureProperty(commandList, 5, material, ShaderTemplate::StandardProperty::SkyTopMap) &&
            BindTextureProperty(commandList, 6, material, ShaderTemplate::StandardProperty::SkyDownMap);
    }

    return true;
}

void RenderBackend::DrawSkySurfaces(const DrawSurf2 **drawSurfs, uint32_t numDrawSurfs) {
    PROFILER_CPU_SCOPED_EVENT("RenderBackend::DrawSkySurfaces");

    assert(numDrawSurfs > 0);

    RHI::PipelineState *currentPSO = nullptr;
    const SubMesh *currentSubMesh = nullptr;
    VertexInputLayoutId currentInputLayoutId = VertexInputLayoutId::Count;

    uint32_t drawCount = 0;
    uint64_t drawTriangleCount = 0;
    uint32_t instanceTableAllocCount = 0;
    uint32_t instanceTableUploadBytes = 0;

    for (uint32_t drawSurfIndex = 0; drawSurfIndex < numDrawSurfs; ++drawSurfIndex) {
        const DrawSurf2 *drawSurf = drawSurfs[drawSurfIndex];
        if (!drawSurf->pso) {
            continue;
        }

        if (currentPSO != drawSurf->pso) {
            currentPSO = drawSurf->pso;

            RHI::renderer->SetPSO(mainCommandList, currentPSO);
            RHI::renderer->SetConstantBuffer(mainCommandList, 0, frameConstantBuffer);
            RHI::renderer->SetConstantBuffer(mainCommandList, 1, cameraConstantBuffer);
        }

        if (currentSubMesh != drawSurf->frameSurf->subMesh || currentInputLayoutId != drawSurf->inputLayoutId) {
            currentSubMesh = drawSurf->frameSurf->subMesh;
            currentInputLayoutId = drawSurf->inputLayoutId;

            RHI::renderer->SetVertexBuffer(mainCommandList, 0, currentSubMesh->vertexBuffer);
            RHI::renderer->SetIndexBuffer(mainCommandList, drawSurf->frameSurf->indexBuffer);
        }

        if (!BindSkyboxTextures(mainCommandList, drawSurf->frameSurf->material)) {
            continue;
        }

        SurfaceInstanceBatch batch;
        if (PrepareSurfaceInstanceBatch(frame->GetThreadData(0), &drawSurfs[drawSurfIndex], drawSurfIndex, 1, batch, instanceTableAllocCount, instanceTableUploadBytes)) {
            DrawSurfaceInstanceBatch(mainCommandList, &drawSurfs[drawSurfIndex], batch, drawCount, drawTriangleCount);
        }
    }

    RecordDrawStats(drawCount, drawTriangleCount, instanceTableAllocCount, instanceTableUploadBytes);
}
