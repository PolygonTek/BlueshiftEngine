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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/JointPose.h"
#include "RBackEnd.h"

BE_NAMESPACE_BEGIN

void Batch::DrawPrimitives() const {
    rhi.BindBuffer(RHI::BufferType::Index, indexBuffer);

    if (numIndirectCommands > 0) {
        rhi.MultiDrawElementsIndirect(RHI::Topology::TriangleList, sizeof(TriIndex), 0, numIndirectCommands, sizeof(RHI::DrawElementsIndirectCommand));
    } else if (numInstances > 0) {
        rhi.DrawElementsInstanced(RHI::Topology::TriangleList, startIndex, r_singleTriangle.GetBool() ? 3 : numIndexes, sizeof(TriIndex), 0, numInstances);
    } else {
        rhi.DrawElements(RHI::Topology::TriangleList, startIndex, r_singleTriangle.GetBool() ? 3 : numIndexes, sizeof(TriIndex), 0);
    }

    int instanceCount = Max(numInstances, 1);

    if (flushType == Flush::Shadow) {
        backEnd.ctx->renderCounter.shadowDrawCalls++;
        backEnd.ctx->renderCounter.shadowDrawIndexes += numIndexes * instanceCount;
        backEnd.ctx->renderCounter.shadowDrawVerts += numVerts * instanceCount;
    }

    backEnd.ctx->renderCounter.drawCalls++;
    backEnd.ctx->renderCounter.drawIndexes += numIndexes * instanceCount;
    backEnd.ctx->renderCounter.drawVerts += numVerts * instanceCount;
}

void Batch::SetShaderProperties(const Shader *shader, const StrHashMap<Shader::Property> &shaderProperties) const {
    const auto &propertyInfoHashMap = shader->GetPropertyInfoHashMap();

    // Iterate over all shader property specs
    for (int i = 0; i < propertyInfoHashMap.Count(); i++) {
        const auto *entry = propertyInfoHashMap.GetByIndex(i);
        const auto &key = entry->first;
        const auto &propInfo = entry->second;

        // Skip if it is a shader define
        if (propInfo.GetFlags() & PropertyInfo::Flag::ShaderDefine) {
            continue;
        }

        // Skip if not exist in shader properties
        const auto *propEntry = shaderProperties.Get(key);
        if (!propEntry) {
            continue;
        }

        const Shader::Property &prop = propEntry->second;

        switch (propInfo.GetType()) {
        case Variant::Type::Int:
            shader->SetConstant1i(key, prop.data.As<int>());
            break;
        case Variant::Type::Point:
            shader->SetConstant2i(key, prop.data.As<Point>());
            break;
        case Variant::Type::Rect:
            shader->SetConstant4i(key, prop.data.As<Rect>());
            break;
        case Variant::Type::Float:
            shader->SetConstant1f(key, prop.data.As<float>());
            break;
        case Variant::Type::Vec2:
            shader->SetConstant2f(key, prop.data.As<Vec2>());
            break;
        case Variant::Type::Vec3:
            shader->SetConstant3f(key, prop.data.As<Vec3>());
            break;
        case Variant::Type::Vec4:
            shader->SetConstant4f(key, prop.data.As<Vec4>());
            break;
        case Variant::Type::Color3:
            shader->SetConstant3f(key, rhi.IsSRGBWriteEnabled() ? prop.data.As<Color3>().SRGBToLinear() : prop.data.As<Color3>());
            break;
        case Variant::Type::Color4:
            shader->SetConstant4f(key, rhi.IsSRGBWriteEnabled() ? prop.data.As<Color4>().SRGBToLinear() : prop.data.As<Color4>());
            break;
        case Variant::Type::Mat2:
            shader->SetConstant2x2f(key, true, prop.data.As<Mat2>());
            break;
        case Variant::Type::Mat3:
            shader->SetConstant3x3f(key, true, prop.data.As<Mat3>());
            break;
        case Variant::Type::Mat4:
            shader->SetConstant4x4f(key, true, prop.data.As<Mat4>());
            break;
        case Variant::Type::Guid: // 
            shader->SetTexture(key, prop.texture);
            break;
        default:
            assert(0);
            break;
        }
    }
}

const Texture *Batch::TextureFromShaderProperties(const Material::ShaderPass *mtrlPass, const Str &textureName) const {
    const auto *entry = mtrlPass->shader->GetPropertyInfoHashMap().Get(textureName);
    if (!entry) {
        return nullptr;
    }

    const auto &propInfo = entry->second;
    if ((propInfo.GetFlags() & PropertyInfo::Flag::ShaderDefine) || (propInfo.GetType() != Variant::Type::Guid)) {
        return nullptr;
    }

    const Shader::Property &prop = mtrlPass->shaderProperties.Get(textureName)->second;
    return prop.texture;
}

void Batch::SetMatrixConstants(const Shader *shader) const {
    if (shader->builtInConstantIndices[Shader::BuiltInConstant::ModelViewMatrix] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ModelViewMatrix], true, backEnd.modelViewMatrix);
    }

    if (shader->builtInConstantIndices[Shader::BuiltInConstant::ViewMatrix] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ViewMatrix], true, backEnd.viewMatrix);
    }

    if (shader->builtInConstantIndices[Shader::BuiltInConstant::ProjectionMatrix] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ProjectionMatrix], true, backEnd.projMatrix);
    }

    if (shader->builtInConstantIndices[Shader::BuiltInConstant::ViewProjectionMatrix] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ViewProjectionMatrix], true, backEnd.viewProjMatrix);
    }

    if (shader->builtInConstantIndices[Shader::BuiltInConstant::ModelViewProjectionMatrix] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ModelViewProjectionMatrix], true, backEnd.modelViewProjMatrix);
    }

    if (shader->builtInConstantIndices[Shader::BuiltInConstant::ModelViewMatrixTranspose] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ModelViewMatrixTranspose], false, backEnd.modelViewMatrix);
    }

    if (shader->builtInConstantIndices[Shader::BuiltInConstant::ProjectionMatrixTranspose] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ProjectionMatrixTranspose], false, backEnd.projMatrix);
    }

    if (shader->builtInConstantIndices[Shader::BuiltInConstant::ViewMatrixTranspose] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ViewMatrixTranspose], false, backEnd.viewMatrix);
    }

    if (shader->builtInConstantIndices[Shader::BuiltInConstant::ViewProjectionMatrixTranspose] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ViewProjectionMatrixTranspose], false, backEnd.viewProjMatrix);
    }

    if (shader->builtInConstantIndices[Shader::BuiltInConstant::ModelViewProjectionMatrixTranspose] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ModelViewProjectionMatrixTranspose], false, backEnd.modelViewProjMatrix);
    }
}

void Batch::SetVertexColorConstants(const Shader *shader, const Material::VertexColorMode::Enum &vertexColor) const {
    Vec4 vertexColorScale;
    Vec4 vertexColorAdd;

    if (vertexColor == Material::VertexColorMode::Modulate) {
        vertexColorScale = Vec4::one;
        vertexColorAdd = Vec4::zero;
    } else if (vertexColor == Material::VertexColorMode::InverseModulate) {
        vertexColorScale = -Vec4::one;
        vertexColorAdd = Vec4::one;
    } else {
        vertexColorScale = Vec4::zero;
        vertexColorAdd = Vec4::one;
    }
    
    shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::VertexColorScale], vertexColorScale);
    shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::VertexColorAdd], vertexColorAdd);
}

void Batch::SetSkinningConstants(const Shader *shader, const SkinningJointCache *cache) const {
    if (renderGlobal.skinningMethod == SkinningJointCache::SkinningMethod::Cpu) {
        return;
    }

    if (renderGlobal.skinningMethod == SkinningJointCache::SkinningMethod::VertexShader) {
        shader->SetConstantArray4f(shader->builtInConstantIndices[Shader::BuiltInConstant::Joints], cache->numJoints * 3, cache->skinningJoints[0].Ptr());
    } else if (renderGlobal.skinningMethod == SkinningJointCache::SkinningMethod::VertexTextureFetch) {
        const Texture *jointsMapTexture = cache->bufferCache.texture;

        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::JointsMap], jointsMapTexture);

        if (renderGlobal.vertexTextureMethod == BufferCacheManager::VertexTextureMethod::Tbo) {
            if (numInstances == 0) {
                shader->SetConstant1i(shader->builtInConstantIndices[Shader::BuiltInConstant::SkinningBaseTc], cache->bufferCache.tcBase[0]);
            }
        } else {
            shader->SetConstant2f(shader->builtInConstantIndices[Shader::BuiltInConstant::InvJointsMapSize], Vec2(1.0f / jointsMapTexture->GetWidth(), 1.0f / jointsMapTexture->GetHeight()));

            if (numInstances == 0) {
                shader->SetConstant2f(shader->builtInConstantIndices[Shader::BuiltInConstant::SkinningBaseTc], Vec2(cache->bufferCache.tcBase[0], cache->bufferCache.tcBase[1]));
            }
        }

        if (r_usePostProcessing.GetBool() && (r_motionBlur.GetInteger() & 2)) {
            shader->SetConstant2i(shader->builtInConstantIndices[Shader::BuiltInConstant::JointIndexOffset], cache->jointIndexOffset);
        }
    }
}

void Batch::SetEntityConstants(const Material::ShaderPass *mtrlPass, const Shader *shader) const {
    if (subMesh->useGpuSkinning) {
        SetSkinningConstants(shader, surfSpace->def->GetState().mesh->skinningJointCache);
    }

    if (numIndirectCommands > 0) {
        rhi.BindBuffer(RHI::BufferType::DrawIndirect, indirectBuffer);
        rhi.BufferDiscardWrite(indirectBuffer, numIndirectCommands * sizeof(indirectCommands[0]), indirectCommands);
    } else if (numInstances > 0) {
        int bufferOffset = backEnd.instanceBufferCache->offset + instanceStartIndex * rhi.HWLimit().uniformBufferOffsetAlignment;
        int bufferSize = (instanceEndIndex - instanceStartIndex + 1) * rhi.HWLimit().uniformBufferOffsetAlignment;

        // 0-indexed buffer for instance buffer
        rhi.BindIndexedBufferRange(RHI::BufferType::Uniform, 0, backEnd.instanceBufferCache->buffer, bufferOffset, bufferSize);
        shader->SetConstantBuffer(shader->builtInConstantIndices[Shader::BuiltInConstant::InstanceDataBuffer], 0);

        shader->SetConstantArray1i(shader->builtInConstantIndices[Shader::BuiltInConstant::InstanceIndexes], numInstances, instanceLocalIndexes);
    } else {
        if (shader->builtInConstantIndices[Shader::BuiltInConstant::LocalToWorldMatrix] >= 0) {
            const Mat3x4 &localToWorldMatrix = surfSpace->def->GetWorldMatrix();
            shader->SetConstant4x3f(shader->builtInConstantIndices[Shader::BuiltInConstant::LocalToWorldMatrix], true, localToWorldMatrix);
        }

        if (shader->builtInConstantIndices[Shader::BuiltInConstant::WorldToLocalMatrix] >= 0) {
            const Mat3x4 worldToLocalMatrix = surfSpace->def->GetWorldMatrixInverse();
            shader->SetConstant4x3f(shader->builtInConstantIndices[Shader::BuiltInConstant::WorldToLocalMatrix], true, worldToLocalMatrix);
        }

        if (shader->builtInConstantIndices[Shader::BuiltInConstant::ConstantColor] >= 0) {
            const Color4 &color = mtrlPass->useOwnerColor ? reinterpret_cast<const Color4 &>(surfSpace->def->GetState().materialParms[RenderObject::MaterialParm::Red]) : mtrlPass->constantColor;
            shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ConstantColor], color);
        }
    }
}

void Batch::SetProbeConstants(const Shader *shader) const {
    if (surfSpace->envProbeInfo[0].envProbe) {
        const EnvProbe *probe0 = surfSpace->envProbeInfo[0].envProbe;

        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::Probe0DiffuseCubeMap], probe0->GetDiffuseProbeTexture());
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::Probe0SpecularCubeMap], probe0->GetSpecularProbeTexture());

        shader->SetConstant1f(shader->builtInConstantIndices[Shader::BuiltInConstant::Probe0SpecularCubeMapMaxMipLevel], probe0->GetSpecularProbeTextureMaxMipLevel());

        if (r_probeBoxProjection.GetBool()) {
            // .w holds boolean value for box projection 
            shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::Probe0Position], Vec4(probe0->GetOrigin(), probe0->UseBoxProjection() ? 1.0f : 0.0f));
            shader->SetConstant3f(shader->builtInConstantIndices[Shader::BuiltInConstant::Probe0Mins], surfSpace->envProbeInfo[0].proxyAABB[0]);
            shader->SetConstant3f(shader->builtInConstantIndices[Shader::BuiltInConstant::Probe0Maxs], surfSpace->envProbeInfo[0].proxyAABB[1]);
        }
    }

    if (r_probeBlending.GetBool()) {
        shader->SetConstant1f(shader->builtInConstantIndices[Shader::BuiltInConstant::ProbeLerp], surfSpace->envProbeInfo[0].weight);

        if (surfSpace->envProbeInfo[1].envProbe) {
            const EnvProbe *probe1 = surfSpace->envProbeInfo[1].envProbe;

            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::Probe1DiffuseCubeMap], probe1->GetDiffuseProbeTexture());
            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::Probe1SpecularCubeMap], probe1->GetSpecularProbeTexture());

            shader->SetConstant1f(shader->builtInConstantIndices[Shader::BuiltInConstant::Probe1SpecularCubeMapMaxMipLevel], probe1->GetSpecularProbeTextureMaxMipLevel());

            if (r_probeBoxProjection.GetBool()) {
                // .w holds boolean value for box projection 
                shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::Probe1Position], Vec4(probe1->GetOrigin(), probe1->UseBoxProjection() ? 1.0f : 0.0f));
                shader->SetConstant3f(shader->builtInConstantIndices[Shader::BuiltInConstant::Probe1Mins], surfSpace->envProbeInfo[1].proxyAABB[0]);
                shader->SetConstant3f(shader->builtInConstantIndices[Shader::BuiltInConstant::Probe1Maxs], surfSpace->envProbeInfo[1].proxyAABB[1]);
            }
        }
    }
}

void Batch::SetMaterialConstants(const Material::ShaderPass *mtrlPass, const Shader *shader) const {
    if (shader->builtInConstantIndices[Shader::BuiltInConstant::TextureMatrixS] >= 0) {
        Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
        Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

        shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::TextureMatrixS], textureMatrixS);
        shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::TextureMatrixT], textureMatrixT);
    }

    if (shader->builtInConstantIndices[Shader::BuiltInConstant::PerforatedAlpha] >= 0) {
        shader->SetConstant1f(shader->builtInConstantIndices[Shader::BuiltInConstant::PerforatedAlpha], mtrlPass->cutoffAlpha);
    }

    SetVertexColorConstants(shader, mtrlPass->vertexColorMode);
}

void Batch::RenderColor(const Material::ShaderPass *mtrlPass, const Color4 &color) const {
    Shader *shader = ShaderManager::constantColorShader;

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    if (numInstances > 0) {
        if (shader->GetGPUInstancingVersion()) {
            shader = shader->GetGPUInstancingVersion();
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    SetEntityConstants(mtrlPass, shader);

    shader->SetConstant4f("color", color);

    DrawPrimitives();
}

void Batch::RenderSelection(const Material::ShaderPass *mtrlPass, const Vec3 &idInVec3) const {
    Shader *shader = ShaderManager::selectionIdShader;

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        if (shader->GetPerforatedVersion()) {
            shader = shader->GetPerforatedVersion();
        }
    }

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    SetEntityConstants(mtrlPass, shader);

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "albedoMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], baseTexture);

        Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
        Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

        shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::TextureMatrixS], textureMatrixS);
        shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::TextureMatrixT], textureMatrixT);

        shader->SetConstant1f(shader->builtInConstantIndices[Shader::BuiltInConstant::PerforatedAlpha], mtrlPass->cutoffAlpha);
    }

    shader->SetConstant3f("id", idInVec3);

    DrawPrimitives();
}

void Batch::RenderDepth(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = ShaderManager::depthShader;

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        if (shader->GetPerforatedVersion()) {
            shader = shader->GetPerforatedVersion();
        }
    }

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    if (numInstances > 0) {
        if (shader->GetGPUInstancingVersion()) {
            shader = shader->GetGPUInstancingVersion();
        }
    }
    
    shader->Bind();

    SetMatrixConstants(shader);

    SetEntityConstants(mtrlPass, shader);

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "albedoMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], baseTexture);

        SetMaterialConstants(mtrlPass, shader);
    }

    DrawPrimitives();
}

void Batch::RenderDepthNormal(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = ShaderManager::depthNormalShader;

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        if (shader->GetPerforatedVersion()) {
            shader = shader->GetPerforatedVersion();
        }
    }

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    if (numInstances > 0) {
        if (shader->GetGPUInstancingVersion()) {
            shader = shader->GetGPUInstancingVersion();
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    SetEntityConstants(mtrlPass, shader);

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "albedoMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], baseTexture);

        SetMaterialConstants(mtrlPass, shader);
    }

    DrawPrimitives();
}

void Batch::RenderVelocity(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = ShaderManager::objectMotionBlurShader;

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        if (shader->GetPerforatedVersion()) {
            shader = shader->GetPerforatedVersion();
        }
    }

    if (subMesh->useGpuSkinning) {
        if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
            shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    Mat4 prevModelViewMatrix = backEnd.camera->def->GetViewMatrix() * surfSpace->def->GetPrevWorldMatrix();
    //shader->SetConstantMatrix4fv("prevModelViewMatrix", 1, true, prevModelViewMatrix);

    Mat4 prevModelViewProjMatrix = backEnd.camera->def->GetProjMatrix() * prevModelViewMatrix;
    shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::PrevModelViewProjectionMatrix], true, prevModelViewProjMatrix);

    shader->SetConstant1f("shutterSpeed", r_motionBlur_ShutterSpeed.GetFloat() / backEnd.ctx->frameTime);
    //shader->SetConstant1f("motionBlurID", (float)surfSpace->id);

    shader->SetTexture("depthMap", backEnd.ctx->screenDepthTexture);

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "albedoMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], baseTexture);
        
        Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
        Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

        shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::TextureMatrixS], textureMatrixS);
        shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::TextureMatrixT], textureMatrixT);

        shader->SetConstant1f(shader->builtInConstantIndices[Shader::BuiltInConstant::PerforatedAlpha], mtrlPass->cutoffAlpha);
    }

    if (subMesh->useGpuSkinning) {
        SetSkinningConstants(shader, surfSpace->def->GetState().mesh->skinningJointCache);
    }

    DrawPrimitives();
}

void Batch::RenderGeneric(const Material::ShaderPass *mtrlPass) const {
    Shader *shader;

    if (mtrlPass->shader) {
        shader = mtrlPass->shader;

        if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
            if (shader->GetPerforatedVersion()) {
                shader = shader->GetPerforatedVersion();
            }
        }

        if (subMesh->useGpuSkinning) {
            Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
            if (skinningShader) {
                shader = skinningShader;
            }
        }

        if (numInstances > 0) {
            if (shader->GetGPUInstancingVersion()) {
                shader = shader->GetGPUInstancingVersion();
            }
        }

        shader->Bind();

        SetShaderProperties(shader, mtrlPass->shaderProperties);
    } else {
        shader = ShaderManager::standardDefaultShader;
                
        if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
            if (shader->GetPerforatedVersion()) {
                shader = shader->GetPerforatedVersion();
            }
        }

        if (subMesh->useGpuSkinning) {
            Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
            if (skinningShader) {
                shader = skinningShader;
            }
        }

        if (numInstances > 0) {
            if (shader->GetGPUInstancingVersion()) {
                shader = shader->GetGPUInstancingVersion();
            }
        }

        shader->Bind();
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], mtrlPass->texture);
    }

    shader->SetConstant1f("ambientScale", 1.0f);

    SetMatrixConstants(shader);

    SetEntityConstants(mtrlPass, shader);

    SetMaterialConstants(mtrlPass, shader);

    shader->SetConstant3f(shader->builtInConstantIndices[Shader::BuiltInConstant::ViewOrigin], backEnd.camera->def->GetState().origin);

    DrawPrimitives();
}

void Batch::RenderAmbient(const Material::ShaderPass *mtrlPass, float ambientScale) const {
    Shader *shader = ShaderManager::standardDefaultShader;

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        if (shader->GetPerforatedVersion()) {
            shader = shader->GetPerforatedVersion();
        }
    }

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    if (numInstances > 0) {
        if (shader->GetGPUInstancingVersion()) {
            shader = shader->GetGPUInstancingVersion();
        }
    }

    shader->Bind();

    const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "albedoMap") : mtrlPass->texture;
    shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], baseTexture);

    shader->SetConstant1f("ambientScale", ambientScale);

    SetMatrixConstants(shader);

    SetEntityConstants(mtrlPass, shader);

    SetMaterialConstants(mtrlPass, shader);

    DrawPrimitives();
}

void Batch::RenderIndirectLit(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = shader = mtrlPass->shader;

    if (shader && shader->GetIndirectLitVersion()) {
        shader = shader->GetIndirectLitVersion();
    } else {
        shader = ShaderManager::standardDefaultIndirectLitShader;
    }

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        if (shader->GetPerforatedVersion()) {
            shader = shader->GetPerforatedVersion();
        }
    }

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    if (numInstances > 0) {
        if (shader->GetGPUInstancingVersion()) {
            shader = shader->GetGPUInstancingVersion();
        }
    }

    shader->Bind();

    if (mtrlPass->shader) {
        if (mtrlPass->shader->GetIndirectLitVersion()) {
            SetShaderProperties(shader, mtrlPass->shaderProperties);

            SetProbeConstants(shader);
        } else {
            const Texture *baseTexture = TextureFromShaderProperties(mtrlPass, "albedoMap");
            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], baseTexture);
        }
    } else {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], mtrlPass->texture);
    }

    shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::PrefilteredDfgMap], backEnd.dfgSumGgxTexture);

    SetMatrixConstants(shader);

    SetEntityConstants(mtrlPass, shader);

    SetMaterialConstants(mtrlPass, shader);

    shader->SetConstant3f(shader->builtInConstantIndices[Shader::BuiltInConstant::ViewOrigin], backEnd.camera->def->GetState().origin);

    DrawPrimitives();
}

static Shader *GetShadowShader(Shader *shader, RenderLight::Type::Enum lightType) {
    if (lightType == RenderLight::Type::Point) {
        return shader->GetPointShadowVersion();
    }
    if (lightType == RenderLight::Type::Spot) {
        return shader->GetSpotShadowVersion();
    }
    if (lightType == RenderLight::Type::Directional) {
        return shader->GetParallelShadowVersion();
    }
    return shader;
}

void Batch::RenderAmbient_DirectLit(const Material::ShaderPass *mtrlPass, float ambientScale) const {
    Shader *shader = shader = mtrlPass->shader;
    
    if (shader && shader->GetDirectLitVersion()) {
        shader = shader->GetDirectLitVersion();
    } else {
        shader = ShaderManager::standardDefaultDirectLitShader;
    }

    bool useShadowMap = false;
    if (r_shadows.GetInteger()) {
        if ((surfLight->def->GetState().flags & RenderLight::Flag::CastShadows) && (surfSpace->def->GetState().flags & RenderObject::Flag::ReceiveShadows)) {
            shader = GetShadowShader(shader, surfLight->def->GetState().type);
            useShadowMap = true;
        }
    }

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        if (shader->GetPerforatedVersion()) {
            shader = shader->GetPerforatedVersion();
        }
    }

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    if (numInstances > 0) {
        if (shader->GetGPUInstancingVersion()) {
            shader = shader->GetGPUInstancingVersion();
        }
    }

    shader->Bind();
    
    if (mtrlPass->shader) {
        if (mtrlPass->shader->GetDirectLitVersion()) {
            SetShaderProperties(shader, mtrlPass->shaderProperties);
        } else {
            const Texture *baseTexture = TextureFromShaderProperties(mtrlPass, "albedoMap");
            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], baseTexture);
        }
    } else {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], mtrlPass->texture);
    }

    // Requires pre-filtered DFG LUT for energy compensation even in direct lighting pass.
    if (r_specularEnergyCompensation.GetBool()) {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::PrefilteredDfgMap], backEnd.dfgSumGgxTexture);
    }

    SetMatrixConstants(shader);

    SetEntityConstants(mtrlPass, shader);

    SetMaterialConstants(mtrlPass, shader);

    SetupLightingShader(mtrlPass, shader, useShadowMap);

    DrawPrimitives();
}

void Batch::RenderIndirectLit_DirectLit(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = shader = mtrlPass->shader;

    if (shader && shader->GetIndirectLitDirectLitVersion()) {
        shader = shader->GetIndirectLitDirectLitVersion();
    } else {
        shader = ShaderManager::standardDefaultIndirectLitDirectLitShader;
    }

    bool useShadowMap = false;
    if (r_shadows.GetInteger()) {
        if ((surfLight->def->GetState().flags & RenderLight::Flag::CastShadows) && (surfSpace->def->GetState().flags & RenderObject::Flag::ReceiveShadows)) {
            shader = GetShadowShader(shader, surfLight->def->GetState().type);
            useShadowMap = true;
        }
    }

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        if (shader->GetPerforatedVersion()) {
            shader = shader->GetPerforatedVersion();
        }
    }

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    if (numInstances > 0) {
        if (shader->GetGPUInstancingVersion()) {
            shader = shader->GetGPUInstancingVersion();
        }
    }

    shader->Bind();

    shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::PrefilteredDfgMap], backEnd.dfgSumGgxTexture);

    if (mtrlPass->shader) {
        if (mtrlPass->shader->GetIndirectLitDirectLitVersion()) {
            SetShaderProperties(shader, mtrlPass->shaderProperties);

            SetProbeConstants(shader);
        } else {
            const Texture *baseTexture = TextureFromShaderProperties(mtrlPass, "albedoMap");
            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], baseTexture);
        }
    } else {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], mtrlPass->texture);
    }

    SetMatrixConstants(shader);

    SetEntityConstants(mtrlPass, shader);

    SetMaterialConstants(mtrlPass, shader);

    SetupLightingShader(mtrlPass, shader, useShadowMap);

    DrawPrimitives();
}

void Batch::RenderBase(const Material::ShaderPass *mtrlPass, float ambientScale) const {
    if (r_indirectLit.GetBool() && surfSpace->envProbeInfo[0].envProbe) {
        if (surfLight) {
            RenderIndirectLit_DirectLit(mtrlPass);
        } else {
            RenderIndirectLit(mtrlPass);
        }
    } else {
        if (surfLight) {
            RenderAmbient_DirectLit(mtrlPass, ambientScale);
        } else {
            RenderAmbient(mtrlPass, ambientScale);
        }
    }
}

void Batch::SetupLightingShader(const Material::ShaderPass *mtrlPass, const Shader *shader, bool useShadowMap) const {
    Vec4 lightVec;

    if (surfLight->def->GetState().type == RenderLight::Type::Directional) {
        lightVec = Vec4(-surfLight->def->GetState().axis[0], 0);
    } else {
        lightVec = Vec4(surfLight->def->GetState().origin, 1);
    }
    shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::LightVec], lightVec);

    shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::LightTextureMatrix], true, surfLight->viewProjTexMatrix);
    shader->SetConstant4x3f(shader->builtInConstantIndices[Shader::BuiltInConstant::LightFallOffMatrix], true, surfLight->def->GetFallOffMatrix());
    shader->SetConstant1f(shader->builtInConstantIndices[Shader::BuiltInConstant::LightFallOffExponent], surfLight->def->GetState().fallOffExponent);

    shader->SetConstant3f(shader->builtInConstantIndices[Shader::BuiltInConstant::ViewOrigin], backEnd.camera->def->GetState().origin);

    if (useShadowMap) {
        if (surfLight->def->GetState().type == RenderLight::Type::Point) {
            shader->SetConstant2f("shadowProjectionDepth", backEnd.shadowProjectionDepth);
            shader->SetConstant1f("vscmBiasedScale", backEnd.ctx->vscmBiasedScale);

            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::CubicNormalCubeMap], textureManager.cubicNormalCubeMapTexture);
            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::IndirectionCubeMap], backEnd.ctx->indirectionCubeMapTexture);
            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::ShadowMap], backEnd.ctx->vscmRT->DepthStencilTexture());
        } else if (surfLight->def->GetState().type == RenderLight::Type::Spot) {
            shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ShadowProjMatrix], true, backEnd.shadowViewProjectionScaleBiasMatrix[0]);
            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::ShadowArrayMap], backEnd.ctx->shadowMapRT->DepthStencilTexture());
        } else if (surfLight->def->GetState().type == RenderLight::Type::Directional) {
            shader->SetConstantArray4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ShadowCascadeProjMatrix], true, r_CSM_count.GetInteger(), backEnd.shadowViewProjectionScaleBiasMatrix);

            if (r_CSM_selectionMethod.GetInteger() == 0) {
                shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ShadowSplitFar], backEnd.csmFar);
            }
            shader->SetConstantArray1f("shadowMapFilterSize", r_CSM_count.GetInteger(), backEnd.shadowMapFilterSize);
            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::ShadowArrayMap], backEnd.ctx->shadowMapRT->DepthStencilTexture());
        }

        if (r_shadowMapQuality.GetInteger() == 3) {
            shader->SetTexture("randomRotMatMap", textureManager.randomRotMatTexture);
        }

        Vec2 shadowMapTexelSize;

        if (surfLight->def->GetState().type == RenderLight::Type::Point) {
            shadowMapTexelSize.x = 1.0f / backEnd.ctx->vscmRT->GetWidth();
            shadowMapTexelSize.y = 1.0f / backEnd.ctx->vscmRT->GetHeight();
        } else {
            shadowMapTexelSize.x = 1.0f / backEnd.ctx->shadowMapRT->GetWidth();
            shadowMapTexelSize.y = 1.0f / backEnd.ctx->shadowMapRT->GetHeight();
        }

        shader->SetConstant2f("shadowMapTexelSize", shadowMapTexelSize);
    } else {
        /*
        // WARNING: for the nvidia's stupid dynamic branching... 
        if (r_shadowMapQuality.GetInteger() == 3) {
            shader->SetTexture("randomRotMatMap", textureManager.randomRotMatTexture);
        }

        // WARNING: for the nvidia's stupid dynamic branching... 
        if (r_shadows.GetInteger() == 1) {
            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::ShadowArrayMap], backEnd.ctx->shadowMapRT->DepthStencilTexture());
        }*/
    }

    const Material *lightMaterial = surfLight->def->GetMaterial();

    shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::LightProjectionMap], lightMaterial->GetPass()->texture);

    Color4 lightColor = surfLight->lightColor * surfLight->def->GetState().intensity * r_lightScale.GetFloat();
    shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::LightColor], lightColor);

    /*bool useLightCube = lightStage->textureStage.texture->GetType() == TextureCubeMap ? true : false;
    shader->SetConstant1i("useLightCube", useLightCube);
    
    if (useLightCube) {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::LightCubeMap], lightStage->textureStage.texture);
    } else {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::LightCubeMap], textureManager.m_defaultCubeMapTexture);
    }*/
}

void Batch::RenderLightInteraction(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = mtrlPass->shader;

    if (shader && shader->GetDirectLitVersion()) {
        shader = shader->GetDirectLitVersion();
    } else {
        shader = ShaderManager::standardDefaultDirectLitShader;
    }

    bool useShadowMap = false;
    if (r_shadows.GetInteger()) {
        if ((surfLight->def->GetState().flags & RenderLight::Flag::CastShadows) && (surfSpace->def->GetState().flags & RenderObject::Flag::ReceiveShadows)) {
            shader = GetShadowShader(shader, surfLight->def->GetState().type);
            useShadowMap = true;
        }
    }

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    if (numInstances > 0) {
        if (shader->GetGPUInstancingVersion()) {
            shader = shader->GetGPUInstancingVersion();
        }
    }

    shader->Bind();

    shader->SetConstant1f("ambientScale", 0);

    if (mtrlPass->shader) {
        if (mtrlPass->shader->GetDirectLitVersion()) {
            SetShaderProperties(shader, mtrlPass->shaderProperties);
        } else {
            const Texture *baseTexture = TextureFromShaderProperties(mtrlPass, "albedoMap");
            shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], baseTexture);
        }
    } else {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], mtrlPass->texture);
    }

    // Requires pre-filtered DFG LUT for energy compensation even in direct lighting pass.
    if (r_specularEnergyCompensation.GetBool()) {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::PrefilteredDfgMap], backEnd.dfgSumGgxTexture);
    }

    SetMatrixConstants(shader);

    SetEntityConstants(mtrlPass, shader);

    SetMaterialConstants(mtrlPass, shader);

    SetupLightingShader(mtrlPass, shader, useShadowMap);
   
    DrawPrimitives();
}

void Batch::RenderFogLightInteraction(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = ShaderManager::fogLightShader;

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    if (numInstances > 0) {
        if (shader->GetGPUInstancingVersion()) {
            shader = shader->GetGPUInstancingVersion();
        }
    }

    shader->Bind();

    // light texture transform matrix
    Mat4 viewProjScaleBiasMat = surfLight->def->GetViewProjScaleBiasMatrix() * surfSpace->def->GetWorldMatrix();
    shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::LightTextureMatrix], true, viewProjScaleBiasMat);
    shader->SetConstant3f("fogColor", &surfLight->def->GetState().materialParms[RenderObject::MaterialParm::Red]);

    Vec3 vec = surfLight->def->GetState().origin - backEnd.camera->def->GetState().origin;
    bool fogEnter = vec.Dot(surfLight->def->GetState().axis[0]) < 0.0f ? true : false;

    if (fogEnter) {
        shader->SetTexture("fogMap", textureManager.fogTexture);
        shader->SetTexture("fogEnterMap", textureManager.whiteTexture);
    } else {
        shader->SetTexture("fogMap", textureManager.whiteTexture);
        shader->SetTexture("fogEnterMap", textureManager.fogEnterTexture);
    }

    const Material *lightMaterial = surfLight->def->GetState().material;
    shader->SetTexture("fogProjectionMap", lightMaterial->GetPass()->texture);

    DrawPrimitives();
}

void Batch::RenderBlendLightInteraction(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = ShaderManager::blendLightShader;

    if (subMesh->useGpuSkinning) {
        Shader *skinningShader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        if (skinningShader) {
            shader = skinningShader;
        }
    }

    if (numInstances > 0) {
        if (shader->GetGPUInstancingVersion()) {
            shader = shader->GetGPUInstancingVersion();
        }
    }

    Color3 blendColor(&surfLight->def->GetState().materialParms[RenderObject::MaterialParm::Red]);

    if (rhi.IsSRGBWriteEnabled()) {
        blendColor = blendColor.SRGBToLinear();
    }

    shader->Bind();

    // light texture transform matrix
    Mat4 viewProjScaleBiasMat = surfLight->def->GetViewProjScaleBiasMatrix() * surfSpace->def->GetWorldMatrix();
    shader->SetConstant4x4f(shader->builtInConstantIndices[Shader::BuiltInConstant::LightTextureMatrix], true, viewProjScaleBiasMat);
    shader->SetConstant3f("blendColor", blendColor);

    const Material *lightMaterial = surfLight->def->GetState().material;
    shader->SetTexture("blendProjectionMap", lightMaterial->GetPass()->texture);

    DrawPrimitives();
}

void Batch::RenderGui(const Material::ShaderPass *mtrlPass) const {
    const Shader *shader;

    if (mtrlPass->shader) {
        shader = mtrlPass->shader;
        shader->Bind();

        SetShaderProperties(shader, mtrlPass->shaderProperties);
    } else {
        shader = ShaderManager::unlitShader;
        shader->Bind();

        shader->SetTexture(shader->builtInSamplerUnits[Shader::BuiltInSampler::AlbedoMap], mtrlPass->texture);
        shader->SetConstant1f(shader->builtInConstantIndices[Shader::BuiltInConstant::Intensity], 1.0f);
    }

    SetMatrixConstants(shader);

    Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
    Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

    shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::TextureMatrixS], textureMatrixS);
    shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::TextureMatrixT], textureMatrixT);

    Color4 color;
    if (mtrlPass->useOwnerColor) {
        color = Color4(&surfSpace->def->GetState().materialParms[RenderObject::MaterialParm::Red]);
    } else {
        color = mtrlPass->constantColor;
    }

    shader->SetConstant4f(shader->builtInConstantIndices[Shader::BuiltInConstant::ConstantColor], color);

    SetVertexColorConstants(shader, Material::VertexColorMode::Modulate);

    DrawPrimitives();
}

BE_NAMESPACE_END
