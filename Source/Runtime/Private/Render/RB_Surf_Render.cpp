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

BE_NAMESPACE_BEGIN

void RBSurf::DrawPrimitives() const {
    rhi.BindBuffer(RHI::IndexBuffer, ibHandle);
    
    if (numInstances > 1) {
        rhi.DrawElementsInstanced(RHI::TrianglesPrim, startIndex, numIndexes, sizeof(TriIndex), 0, numInstances);
    } else {
        rhi.DrawElements(RHI::TrianglesPrim, startIndex, numIndexes, sizeof(TriIndex), 0);
    }

    if (flushType == ShadowFlush) {
        backEnd.ctx->renderCounter.shadowDrawCalls++;
        backEnd.ctx->renderCounter.shadowDrawIndexes += numIndexes;
        backEnd.ctx->renderCounter.shadowDrawVerts += numVerts;
    } else {
        backEnd.ctx->renderCounter.drawCalls++;
        backEnd.ctx->renderCounter.drawIndexes += numIndexes;
        backEnd.ctx->renderCounter.drawVerts += numVerts;
    }
}

void RBSurf::SetShaderProperties(const Shader *shader, const StrHashMap<Shader::Property> &shaderProperties) const {
    const auto &propertyInfoHashMap = shader->GetPropertyInfoHashMap();

    // Iterate over all shader property specs
    for (int i = 0; i < propertyInfoHashMap.Count(); i++) {
        const auto *entry = propertyInfoHashMap.GetByIndex(i);
        const auto &key = entry->first;
        const auto &propInfo = entry->second;

        // Skip if it is a shader define
        if (propInfo.GetFlags() & PropertyInfo::ShaderDefineFlag) {
            continue;
        }

        // Skip if not exist in shader properties
        const auto *propEntry = shaderProperties.Get(key);
        if (!propEntry) {
            continue;
        }

        const Shader::Property &prop = propEntry->second;

        switch (propInfo.GetType()) {
        case Variant::IntType:
            shader->SetConstant1i(key, prop.data.As<int>());
            break;
        case Variant::PointType:
            shader->SetConstant2i(key, prop.data.As<Point>());
            break;
        case Variant::RectType:
            shader->SetConstant4i(key, prop.data.As<Rect>());
            break;
        case Variant::FloatType:
            shader->SetConstant1f(key, prop.data.As<float>());
            break;
        case Variant::Vec2Type:
            shader->SetConstant2f(key, prop.data.As<Vec2>());
            break;
        case Variant::Vec3Type:
            shader->SetConstant3f(key, prop.data.As<Vec3>());
            break;
        case Variant::Vec4Type:
            shader->SetConstant4f(key, prop.data.As<Vec4>());
            break;
        case Variant::Color3Type:
            shader->SetConstant3f(key, prop.data.As<Color3>());
            break;
        case Variant::Color4Type:
            shader->SetConstant4f(key, prop.data.As<Color4>());
            break;
        case Variant::Mat2Type:
            shader->SetConstant2x2f(key, true, prop.data.As<Mat2>());
            break;
        case Variant::Mat3Type:
            shader->SetConstant3x3f(key, true, prop.data.As<Mat3>());
            break;
        case Variant::Mat4Type:
            shader->SetConstant4x4f(key, true, prop.data.As<Mat4>());
            break;
        case Variant::GuidType: // 
            shader->SetTexture(key, prop.texture);
            break;
        default:
            assert(0);
            break;
        }
    }

    //shader->SetConstant1f("currentRenderWidthRatio", (float)GL_GetVidWidth() / g_rsd.screenWidth);
    //shader->SetConstant1f("currentRenderHeightRatio", (float)GL_GetVidHeight() /g_rsd.screenHeight);
}

const Texture *RBSurf::TextureFromShaderProperties(const Material::ShaderPass *mtrlPass, const Str &textureName) const {
    const auto *entry = mtrlPass->shader->GetPropertyInfoHashMap().Get(textureName);
    if (!entry) {
        return nullptr;
    }

    const auto &propInfo = entry->second;
    if ((propInfo.GetFlags() & PropertyInfo::ShaderDefineFlag) || (propInfo.GetType() != Variant::GuidType)) {
        return nullptr;
    }

    const Shader::Property &prop = mtrlPass->shaderProperties.Get(textureName)->second;
    return prop.texture;
}

void RBSurf::SetMatrixConstants(const Shader *shader) const {
    if (shader->builtInConstantLocations[Shader::ModelViewMatrixConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ModelViewMatrixConst], true, backEnd.modelViewMatrix);
    }
    
    if (shader->builtInConstantLocations[Shader::ProjectionMatrixConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ProjectionMatrixConst], true, backEnd.projMatrix);
    }

    if (shader->builtInConstantLocations[Shader::ModelViewProjectionMatrixConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ModelViewProjectionMatrixConst], true, backEnd.modelViewProjMatrix);
    }

    if (shader->builtInConstantLocations[Shader::ModelViewMatrixTransposeConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ModelViewMatrixTransposeConst], false, backEnd.modelViewMatrix);
    }

    if (shader->builtInConstantLocations[Shader::ProjectionMatrixTransposeConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ProjectionMatrixTransposeConst], false, backEnd.projMatrix);
    }

    if (shader->builtInConstantLocations[Shader::ModelViewProjectionMatrixTransposeConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ModelViewProjectionMatrixTransposeConst], false, backEnd.modelViewProjMatrix);
    }
}

void RBSurf::SetVertexColorConstants(const Shader *shader, const Material::VertexColorMode &vertexColor) const {
    Vec4 vertexColorScale;
    Vec4 vertexColorAdd;

    if (vertexColor == Material::ModulateVertexColor) {
        vertexColorScale.Set(1.0f, 1.0f, 1.0f, 1.0f);
        vertexColorAdd.Set(0.0f, 0.0f, 0.0f, 0.0f);
    } else if (vertexColor == Material::InverseModulateVertexColor) {
        vertexColorScale.Set(-1.0f, -1.0f, -1.0f, 1.0f);
        vertexColorAdd.Set(1.0f, 1.0f, 1.0f, 0.0f);
    } else {
        vertexColorScale.Set(0.0f, 0.0f, 0.0f, 0.0f);
        vertexColorAdd.Set(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::VertexColorScaleConst], vertexColorScale);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::VertexColorAddConst], vertexColorAdd);
}

void RBSurf::SetSkinningConstants(const Shader *shader, const SkinningJointCache *cache) const {
    if (renderGlobal.skinningMethod == Mesh::CpuSkinning) {
        return;
    }

    if (renderGlobal.skinningMethod == Mesh::VertexShaderSkinning) {
        shader->SetConstantArray4f("joints", cache->numJoints * 3, cache->skinningJoints[0].Ptr());
    } else if (renderGlobal.skinningMethod == Mesh::VtfSkinning) {
        const Texture *jointsMapTexture = cache->bufferCache.texture;
        shader->SetTexture("jointsMap", jointsMapTexture);

        if (renderGlobal.vtUpdateMethod == Mesh::TboUpdate) {
            shader->SetConstant1i("tcBase", cache->bufferCache.tcBase[0]);
        } else {
            shader->SetConstant2f("invJointsMapSize", Vec2(1.0f / jointsMapTexture->GetWidth(), 1.0f / jointsMapTexture->GetHeight()));
            shader->SetConstant2f("tcBase", Vec2(cache->bufferCache.tcBase[0], cache->bufferCache.tcBase[1]));
        }

        if (r_usePostProcessing.GetBool() && (r_motionBlur.GetInteger() & 2)) {
            shader->SetConstant1i("jointIndexOffsetCurr", cache->jointIndexOffsetCurr);
            shader->SetConstant1i("jointIndexOffsetPrev", cache->jointIndexOffsetPrev);
        }
    }
}

void RBSurf::RenderColor(const Color4 &color) const {
    Shader *shader = ShaderManager::constantColorShader;

    if (subMesh->useGpuSkinning) {
        if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
            shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    if (subMesh->useGpuSkinning) {
        const Mesh *mesh = surfSpace->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    shader->SetConstant4f("color", color);

    DrawPrimitives();
}

void RBSurf::RenderSelection(const Material::ShaderPass *mtrlPass, const Vec3 &vec3_id) const {
    Shader *shader = ShaderManager::selectionIdShader;
    
    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff && shader->GetPerforatedVersion()) {
        shader = shader->GetPerforatedVersion();
    }

    if (subMesh->useGpuSkinning) {
        if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
            shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    if (subMesh->useGpuSkinning) {
        const Mesh *mesh = surfSpace->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        shader->SetConstant1f("perforatedAlpha", mtrlPass->cutoffAlpha);

        Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
        Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

        Color4 color;
        if (mtrlPass->useOwnerColor) {
            color = Color4(&surfSpace->def->parms.materialParms[SceneEntity::RedParm]);
        } else {
            color = mtrlPass->constantColor;
        }

        shader->SetConstant4f("constantColor", color);

        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "albedoMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], baseTexture);
    }

    shader->SetConstant3f("id", vec3_id);

    DrawPrimitives();
}

void RBSurf::RenderDepth(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = ShaderManager::depthShader;

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff && shader->GetPerforatedVersion()) {
        shader = shader->GetPerforatedVersion();
    }

    if (subMesh->useGpuSkinning) {
        if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
            shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    if (subMesh->useGpuSkinning) {
        const Mesh *mesh = surfSpace->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        shader->SetConstant1f("perforatedAlpha", mtrlPass->cutoffAlpha);

        Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
        Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

        Color4 color;
        if (mtrlPass->useOwnerColor) {
            color = Color4(&surfSpace->def->parms.materialParms[SceneEntity::RedParm]);
        } else {
            color = mtrlPass->constantColor;
        }

        shader->SetConstant4f("constantColor", color);

        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "albedoMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], baseTexture);
    }

    DrawPrimitives();
}

void RBSurf::RenderVelocity(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = ShaderManager::objectMotionBlurShader;

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff && shader->GetPerforatedVersion()) {
        shader = shader->GetPerforatedVersion();
    }

    if (subMesh->useGpuSkinning) {
        if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
            shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    Mat4 prevModelViewMatrix = backEnd.view->def->viewMatrix * surfSpace->def->motionBlurModelMatrix[1];
    //shader->SetConstantMatrix4fv("prevModelViewMatrix", 1, true, prevModelViewMatrix);

    Mat4 prevModelViewProjMatrix = backEnd.view->def->projMatrix * prevModelViewMatrix;
    shader->SetConstant4x4f("prevModelViewProjectionMatrix", true, prevModelViewProjMatrix);

    shader->SetConstant1f("shutterSpeed", r_motionBlur_ShutterSpeed.GetFloat() / backEnd.ctx->frameTime);
    //shader->SetConstant1f("motionBlurID", (float)surfSpace->id);

    shader->SetTexture("depthMap", backEnd.ctx->screenDepthTexture);

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        shader->SetConstant1f("perforatedAlpha", mtrlPass->cutoffAlpha);

        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "albedoMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], baseTexture);
        
        Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
        Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);
    }

    if (subMesh->useGpuSkinning) {
        const Mesh *mesh = surfSpace->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    DrawPrimitives();
}

void RBSurf::RenderGeneric(const Material::ShaderPass *mtrlPass) const {
    Shader *shader;

    if (mtrlPass->shader) {
        shader = mtrlPass->shader;

        if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff && shader->GetPerforatedVersion()) {
            shader = shader->GetPerforatedVersion();
        }

        if (subMesh->useGpuSkinning) {
            if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
                shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
            }
        }

        shader->Bind();
        SetShaderProperties(shader, mtrlPass->shaderProperties);
    } else {
        shader = ShaderManager::standardDefaultShader;

        if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff && shader->GetPerforatedVersion()) {
            shader = shader->GetPerforatedVersion();
        }

        if (subMesh->useGpuSkinning) {
            if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
                shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
            }
        }

        shader->Bind();
        shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], mtrlPass->texture);
    }
    
    SetMatrixConstants(shader);

    if (subMesh->useGpuSkinning) {
        const Mesh *mesh = surfSpace->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    Vec3 localViewOrigin = surfSpace->def->parms.axis.TransposedMulVec(backEnd.view->def->parms.origin - surfSpace->def->parms.origin) / surfSpace->def->parms.scale;
    //Vec3 localViewOrigin = (backEnd.view->def->parms.origin - surfSpace->def->parms.origin) * surfSpace->def->parms.axis;
    shader->SetConstant3f(shader->builtInConstantLocations[Shader::LocalViewOriginConst], localViewOrigin);

    const Mat4 &worldMatrix = surfSpace->def->GetModelMatrix();
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixSConst], worldMatrix[0]);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixTConst], worldMatrix[1]);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixRConst], worldMatrix[2]);

    Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
    Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

    SetVertexColorConstants(shader, mtrlPass->vertexColorMode);

    Color4 color;
    if (mtrlPass->useOwnerColor) {
        color = Color4(&surfSpace->def->parms.materialParms[SceneEntity::RedParm]);
    } else {
        color = mtrlPass->constantColor;
    }

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::ConstantColorConst], color);
    shader->SetConstant1f("ambientScale", 1.0f);

    DrawPrimitives();
}

void RBSurf::RenderAmbient(const Material::ShaderPass *mtrlPass, float ambientScale) const {
    Shader *shader = ShaderManager::standardDefaultShader;

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff && shader->GetPerforatedVersion()) {
        shader = shader->GetPerforatedVersion();
    }

    if (subMesh->useGpuSkinning) {
        if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
            shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    if (subMesh->useGpuSkinning) {
        const Mesh *mesh = surfSpace->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "albedoMap") : mtrlPass->texture;
    shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], baseTexture);

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        shader->SetConstant1f("perforatedAlpha", mtrlPass->cutoffAlpha);
    }

    Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
    Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

    SetVertexColorConstants(shader, mtrlPass->vertexColorMode);

    Color4 color;
    if (mtrlPass->useOwnerColor) {
        color = Color4(&surfSpace->def->parms.materialParms[SceneEntity::RedParm]);
    } else {
        color = mtrlPass->constantColor;
    }

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::ConstantColorConst], color);
    shader->SetConstant1f("ambientScale", ambientScale);

    DrawPrimitives();
}

void RBSurf::RenderAmbientLit(const Material::ShaderPass *mtrlPass, float ambientScale) const {
    Shader *shader = shader = mtrlPass->shader;

    if (shader && shader->GetAmbientLitVersion()) {
        shader = shader->GetAmbientLitVersion();
    } else {
        shader = ShaderManager::standardDefaultAmbientLitShader;
    }

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff && shader->GetPerforatedVersion()) {
        shader = shader->GetPerforatedVersion();
    }

    if (subMesh->useGpuSkinning) {
        if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
            shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        }
    }

    shader->Bind();

    if (mtrlPass->shader) {
        if (mtrlPass->shader->GetAmbientLitVersion()) {
            SetShaderProperties(shader, mtrlPass->shaderProperties);
        } else {
            const Texture *baseTexture = TextureFromShaderProperties(mtrlPass, "albedoMap");
            shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], baseTexture);
        }
    } else {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], mtrlPass->texture);
    }

    SetMatrixConstants(shader);

    if (subMesh->useGpuSkinning) {
        const Mesh *mesh = surfSpace->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    // TODO:
    shader->SetTexture("envCubeMap", backEnd.envCubeTexture);
    shader->SetTexture("integrationLUTMap", backEnd.integrationLUTTexture);
    shader->SetTexture("irradianceEnvCubeMap0", backEnd.irradianceEnvCubeTexture);
    shader->SetTexture("irradianceEnvCubeMap1", backEnd.irradianceEnvCubeTexture);
    shader->SetTexture("prefilteredEnvCubeMap0", backEnd.prefilteredEnvCubeTexture);
    shader->SetTexture("prefilteredEnvCubeMap1", backEnd.prefilteredEnvCubeTexture);
    shader->SetConstant1f("ambientLerp", 0.0f);

    // view vector: world -> to mesh coordinates
    Vec3 localViewOrigin = surfSpace->def->parms.axis.TransposedMulVec(backEnd.view->def->parms.origin - surfSpace->def->parms.origin) / surfSpace->def->parms.scale;
    //Vec3 localViewOrigin = (backEnd.view->def->parms.origin - surfSpace->def->parms.origin) * surfSpace->def->parms.axis;
    shader->SetConstant3f(shader->builtInConstantLocations[Shader::LocalViewOriginConst], localViewOrigin);

    const Mat4 &worldMatrix = surfSpace->def->GetModelMatrix();
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixSConst], worldMatrix[0]);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixTConst], worldMatrix[1]);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixRConst], worldMatrix[2]);

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        shader->SetConstant1f("perforatedAlpha", mtrlPass->cutoffAlpha);
    }

    Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
    Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

    SetVertexColorConstants(shader, mtrlPass->vertexColorMode);

    Color4 color;
    if (mtrlPass->useOwnerColor) {
        color = Color4(&surfSpace->def->parms.materialParms[SceneEntity::RedParm]);
    } else {
        color = mtrlPass->constantColor;
    }

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::ConstantColorConst], color);
    shader->SetConstant1f("ambientScale", ambientScale);

    DrawPrimitives();
}

void RBSurf::RenderAmbient_DirectLit(const Material::ShaderPass *mtrlPass, float ambientScale) const {
    Shader *shader = shader = mtrlPass->shader;
    
    if (shader && shader->GetDirectLitVersion()) {
        shader = shader->GetDirectLitVersion();
    } else {
        shader = ShaderManager::standardDefaultDirectLitShader;
    }

    bool useShadowMap = (r_shadows.GetInteger() == 0) || (!surfLight->def->parms.castShadows || !surfSpace->def->parms.receiveShadows) ? false : true;
    if (useShadowMap) {
        if (surfLight->def->parms.type == SceneLight::PointLight) {
            shader = shader->GetPointShadowVersion();
        } else if (surfLight->def->parms.type == SceneLight::SpotLight) {
            shader = shader->GetSpotShadowVersion();
        } else if (surfLight->def->parms.type == SceneLight::DirectionalLight) {
            shader = shader->GetParallelShadowVersion();
        }
    }

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff && shader->GetPerforatedVersion()) {
        shader = shader->GetPerforatedVersion();
    }

    if (subMesh->useGpuSkinning) {
        if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
            shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        }
    }

    shader->Bind();
    
    if (mtrlPass->shader) {
        if (mtrlPass->shader->GetDirectLitVersion()) {
            SetShaderProperties(shader, mtrlPass->shaderProperties);
        } else {
            const Texture *baseTexture = TextureFromShaderProperties(mtrlPass, "albedoMap");
            shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], baseTexture);
        }
    } else {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], mtrlPass->texture);
    }

    SetMatrixConstants(shader);

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        shader->SetConstant1f("perforatedAlpha", mtrlPass->cutoffAlpha);
    }

    shader->SetConstant1f("ambientScale", ambientScale);

    SetupLightingShader(mtrlPass, shader, useShadowMap);

    DrawPrimitives();
}

void RBSurf::RenderAmbientLit_DirectLit(const Material::ShaderPass *mtrlPass, float ambientScale) const {
    Shader *shader = shader = mtrlPass->shader;

    if (shader && shader->GetAmbientLitDirectLitVersion()) {
        shader = shader->GetAmbientLitDirectLitVersion();
    } else {
        shader = ShaderManager::standardDefaultAmbientLitDirectLitShader;
    }

    bool useShadowMap = (r_shadows.GetInteger() == 0) || (!surfLight->def->parms.castShadows || !surfSpace->def->parms.receiveShadows) ? false : true;
    if (useShadowMap) {
        if (surfLight->def->parms.type == SceneLight::PointLight) {
            shader = shader->GetPointShadowVersion();
        } else if (surfLight->def->parms.type == SceneLight::SpotLight) {
            shader = shader->GetSpotShadowVersion();
        } else if (surfLight->def->parms.type == SceneLight::DirectionalLight) {
            shader = shader->GetParallelShadowVersion();
        }
    }

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff && shader->GetPerforatedVersion()) {
        shader = shader->GetPerforatedVersion();
    }

    if (subMesh->useGpuSkinning) {
        if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
            shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        }
    }

    shader->Bind();

    if (mtrlPass->shader) {
        if (mtrlPass->shader->GetAmbientLitDirectLitVersion()) {
            SetShaderProperties(shader, mtrlPass->shaderProperties);
        } else {
            const Texture *baseTexture = TextureFromShaderProperties(mtrlPass, "albedoMap");
            shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], baseTexture);
        }
    } else {
        shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], mtrlPass->texture);
    }

    SetMatrixConstants(shader);

    if (mtrlPass->renderingMode == Material::RenderingMode::AlphaCutoff) {
        shader->SetConstant1f("perforatedAlpha", mtrlPass->cutoffAlpha);
    }

    shader->SetConstant1f("ambientScale", ambientScale);

    // TODO:
    shader->SetTexture("envCubeMap", backEnd.envCubeTexture);
    shader->SetTexture("integrationLUTMap", backEnd.integrationLUTTexture);
    shader->SetTexture("irradianceEnvCubeMap0", backEnd.irradianceEnvCubeTexture);
    shader->SetTexture("irradianceEnvCubeMap1", backEnd.irradianceEnvCubeTexture);
    shader->SetTexture("prefilteredEnvCubeMap0", backEnd.prefilteredEnvCubeTexture);
    shader->SetTexture("prefilteredEnvCubeMap1", backEnd.prefilteredEnvCubeTexture);
    shader->SetConstant1f("ambientLerp", 0.0f);

    SetupLightingShader(mtrlPass, shader, useShadowMap);

    DrawPrimitives();
}

void RBSurf::RenderBase(const Material::ShaderPass *mtrlPass, float ambientScale) const {
    if (r_ambientLit.GetBool()) {
        if (surfLight) {
            RenderAmbientLit_DirectLit(mtrlPass, ambientScale);
        } else {
            RenderAmbientLit(mtrlPass, ambientScale);
        }
    } else {
        if (surfLight) {
            RenderAmbient_DirectLit(mtrlPass, ambientScale);
        } else {
            RenderAmbient(mtrlPass, ambientScale);
        }
    }
}

void RBSurf::SetupLightingShader(const Material::ShaderPass *mtrlPass, const Shader *shader, bool useShadowMap) const {
    // Set local to world matrix
    const Mat4 &worldMatrix = surfSpace->def->GetModelMatrix();
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixSConst], worldMatrix[0]);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixTConst], worldMatrix[1]);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixRConst], worldMatrix[2]);

    // world coordinates -> entity's local coordinates
    Vec3 localViewOrigin = surfSpace->def->parms.axis.TransposedMulVec(backEnd.view->def->parms.origin - surfSpace->def->parms.origin) / surfSpace->def->parms.scale;
    Vec4 localLightOrigin;
    Vec3 lightInvRadius;

    if (surfLight->def->parms.type == SceneLight::DirectionalLight) {
        localLightOrigin = Vec4(surfSpace->def->parms.axis.TransposedMulVec(-surfLight->def->parms.axis[0]), 1.0f);
        lightInvRadius.SetFromScalar(0);
    } else {
        localLightOrigin = Vec4(surfSpace->def->parms.axis.TransposedMulVec(surfLight->def->parms.origin - surfSpace->def->parms.origin) / surfSpace->def->parms.scale, 0.0f);
        lightInvRadius = 1.0f / surfLight->def->GetRadius();
    }

    Mat3 localLightAxis = surfSpace->def->parms.axis.TransposedMul(surfLight->def->parms.axis);
    localLightAxis[0] *= surfSpace->def->parms.scale;
    localLightAxis[1] *= surfSpace->def->parms.scale;
    localLightAxis[2] *= surfSpace->def->parms.scale;

    shader->SetConstant3f(shader->builtInConstantLocations[Shader::LocalViewOriginConst], localViewOrigin);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::LocalLightOriginConst], localLightOrigin);
    shader->SetConstant3x3f(shader->builtInConstantLocations[Shader::LocalLightAxisConst], false, localLightAxis);

    Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
    Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

    SetVertexColorConstants(shader, mtrlPass->vertexColorMode);

    Color4 color;
    if (mtrlPass->useOwnerColor) {
        color = Color4(&surfSpace->def->parms.materialParms[SceneEntity::RedParm]);
    } else {
        color = mtrlPass->constantColor;
    }

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::ConstantColorConst], color);

    if (subMesh->useGpuSkinning) {
        const Mesh *mesh = surfSpace->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }
        
    shader->SetConstant3f("lightInvRadius", lightInvRadius);
    shader->SetConstant1f("lightFallOffExponent", surfLight->def->parms.fallOffExponent);
    shader->SetConstant1i("removeBackProjection", surfLight->def->parms.type == SceneLight::SpotLight ? 1 : 0);
        
    if (useShadowMap) {
        if (surfLight->def->parms.type == SceneLight::PointLight) {
            shader->SetConstant2f("shadowProjectionDepth", backEnd.shadowProjectionDepth);
            shader->SetConstant1f("vscmBiasedScale", backEnd.ctx->vscmBiasedScale);
            shader->SetTexture("cubicNormalCubeMap", textureManager.cubicNormalCubeMapTexture);
            shader->SetTexture("indirectionCubeMap", backEnd.ctx->indirectionCubeMapTexture);
            shader->SetTexture("shadowMap", backEnd.ctx->vscmRT->DepthStencilTexture());
        } else if (surfLight->def->parms.type == SceneLight::SpotLight) {
            shader->SetConstant4x4f("shadowProjMatrix", true, backEnd.shadowViewProjectionScaleBiasMatrix[0]);
            shader->SetTexture("shadowArrayMap", backEnd.ctx->shadowMapRT->DepthStencilTexture());
        } else if (surfLight->def->parms.type == SceneLight::DirectionalLight) {
            shader->SetConstantArray4x4f("shadowCascadeProjMatrix", true, r_CSM_count.GetInteger(), backEnd.shadowViewProjectionScaleBiasMatrix);

            if (r_CSM_selectionMethod.GetInteger() == 0) {
                // z-based selection shader needs shadowSplitFar value
                float sFar[4];
                for (int cascadeIndex = 0; cascadeIndex < r_CSM_count.GetInteger(); cascadeIndex++) {
                    float dFar = backEnd.csmDistances[cascadeIndex + 1];
                    sFar[cascadeIndex] = (backEnd.projMatrix[2][2] * -dFar + backEnd.projMatrix[2][3]) / dFar;
                    sFar[cascadeIndex] = sFar[cascadeIndex] * 0.5f + 0.5f;
                }
                shader->SetConstant4f("shadowSplitFar", sFar);
            }
            shader->SetConstant1f("cascadeBlendSize", r_CSM_blendSize.GetFloat());
            shader->SetConstantArray1f("shadowMapFilterSize", r_CSM_count.GetInteger(), backEnd.shadowMapFilterSize);
            shader->SetTexture("shadowArrayMap", backEnd.ctx->shadowMapRT->DepthStencilTexture());
        }

        if (r_shadowMapQuality.GetInteger() == 3) {
            shader->SetTexture("randomRotMatMap", textureManager.randomRotMatTexture);
        }

        Vec2 shadowMapTexelSize;

        if (surfLight->def->parms.type == SceneLight::PointLight) {
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
            shader->SetTexture("shadowArrayMap", backEnd.ctx->shadowMapRT->DepthStencilTexture());
        }*/
    }

    const Material *lightMaterial = surfLight->def->GetMaterial();

    shader->SetTexture("lightProjectionMap", lightMaterial->GetPass()->texture);
    shader->SetConstant4x4f("lightTextureMatrix", true, surfLight->viewProjTexMatrix);

    Color4 lightColor = surfLight->lightColor * surfLight->def->parms.intensity * r_lightScale.GetFloat();
    shader->SetConstant4f("lightColor", lightColor);

    //bool useLightCube = lightStage->textureStage.texture->GetType() == TextureCubeMap ? true : false;
    //shader->SetConstant1i("useLightCube", useLightCube);
    
/*	if (useLightCube) {
        shader->SetTexture("lightCubeMap", lightStage->textureStage.texture);
    } else {
        //shader->SetTexture("lightCubeMap", textureManager.m_defaultCubeMapTexture);
    }*/
}

void RBSurf::RenderLightInteraction(const Material::ShaderPass *mtrlPass) const {    
    Shader *shader = mtrlPass->shader;

    if (shader && shader->GetDirectLitVersion()) {
        shader = shader->GetDirectLitVersion();
    } else {
        shader = ShaderManager::standardDefaultDirectLitShader;
    }

    bool useShadowMap = (r_shadows.GetInteger() == 0) || (!surfLight->def->parms.castShadows || !surfSpace->def->parms.receiveShadows) ? false : true;
    if (useShadowMap) {
        if (surfLight->def->parms.type == SceneLight::PointLight) {
            shader = shader->GetPointShadowVersion();
        } else if (surfLight->def->parms.type == SceneLight::SpotLight) {
            shader = shader->GetSpotShadowVersion();
        } else if (surfLight->def->parms.type == SceneLight::DirectionalLight) {
            shader = shader->GetParallelShadowVersion();
        }
    }

    if (subMesh->useGpuSkinning) {
        if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
            shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        }
    }

    shader->Bind();

    if (mtrlPass->shader) {
        if (mtrlPass->shader->GetDirectLitVersion()) {
            SetShaderProperties(shader, mtrlPass->shaderProperties);
        } else {
            const Texture *baseTexture = TextureFromShaderProperties(mtrlPass, "albedoMap");
            shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], baseTexture);
        }
    } else {        
        shader->SetTexture(shader->builtInSamplerUnits[Shader::AlbedoMapSampler], mtrlPass->texture);
    }

    SetMatrixConstants(shader);

    shader->SetConstant1f("ambientScale", 0);

    SetupLightingShader(mtrlPass, shader, useShadowMap);
   
    DrawPrimitives();
}

void RBSurf::RenderFogLightInteraction(const Material::ShaderPass *mtrlPass) const {	
    Shader *shader = ShaderManager::fogLightShader;

    if (subMesh->useGpuSkinning) {
        if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
            shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        }
    }

    shader->Bind();

    // light texture transform matrix
    Mat4 viewProjScaleBiasMat = surfLight->def->GetViewProjScaleBiasMatrix() * surfSpace->def->GetModelMatrix();	
    shader->SetConstant4x4f("lightTextureMatrix", true, viewProjScaleBiasMat);
    shader->SetConstant3f("fogColor", &surfLight->def->parms.materialParms[SceneEntity::RedParm]);

    Vec3 vec = surfLight->def->parms.origin - backEnd.view->def->parms.origin;
    bool fogEnter = vec.Dot(surfLight->def->parms.axis[0]) < 0.0f ? true : false;

    if (fogEnter) {
        shader->SetTexture("fogMap", textureManager.fogTexture);
        shader->SetTexture("fogEnterMap", textureManager.whiteTexture);
    } else {
        shader->SetTexture("fogMap", textureManager.whiteTexture);
        shader->SetTexture("fogEnterMap", textureManager.fogEnterTexture);
    }

    const Material *lightMaterial = surfLight->def->parms.material;
    shader->SetTexture("fogProjectionMap", lightMaterial->GetPass()->texture);

    DrawPrimitives();
}

void RBSurf::RenderBlendLightInteraction(const Material::ShaderPass *mtrlPass) const {
    Shader *shader = ShaderManager::blendLightShader;

    if (subMesh->useGpuSkinning) {
        if (shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex)) {
            shader = shader->GetGPUSkinningVersion(subMesh->gpuSkinningVersionIndex);
        }
    }

    Color3 blendColor(&surfLight->def->parms.materialParms[SceneEntity::RedParm]);

    if (cvarSystem.GetCVarBool(L"gl_sRGB")) {
        blendColor = blendColor.SRGBtoLinear();
    }

    shader->Bind();

    // light texture transform matrix
    Mat4 viewProjScaleBiasMat = surfLight->def->GetViewProjScaleBiasMatrix() * surfSpace->def->GetModelMatrix();
    shader->SetConstant4x4f("lightTextureMatrix", true, viewProjScaleBiasMat);
    shader->SetConstant3f("blendColor", blendColor);

    const Material *lightMaterial = surfLight->def->parms.material;
    shader->SetTexture("blendProjectionMap", lightMaterial->GetPass()->texture);	

    DrawPrimitives();
}

void RBSurf::RenderGui(const Material::ShaderPass *mtrlPass) const {
    const Shader *shader;

    if (mtrlPass->shader) {
        shader = mtrlPass->shader;
        shader->Bind();

        SetShaderProperties(shader, mtrlPass->shaderProperties);
    } else {
        shader = ShaderManager::simpleShader;
        shader->Bind();

        shader->SetTexture("albedoMap", mtrlPass->texture);
    }

    SetMatrixConstants(shader);

    Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
    Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

    SetVertexColorConstants(shader, Material::ModulateVertexColor);

    Color4 color;
    if (mtrlPass->useOwnerColor) {
        color = Color4(&surfSpace->def->parms.materialParms[SceneEntity::RedParm]);
    } else {
        color = mtrlPass->constantColor;
    }

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::ConstantColorConst], color);
    
    DrawPrimitives();
}

BE_NAMESPACE_END
