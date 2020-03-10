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
#include "Platform/PlatformTime.h"
#include "RBackEnd.h"
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

static const int HOM_CULL_TEXTURE_WIDTH = 4096;
static const int HOM_CULL_TEXTURE_HEIGHT = 1;

RenderBackEnd   backEnd;

static void RB_InitStencilStates() {
    backEnd.stencilStates[RenderBackEnd::PreDefinedStencilState::VolumeIntersectionZPass] =
        rhi.CreateStencilState(~0, ~0,
        RHI::StencilFunc::Always, RHI::StencilOp::Keep, RHI::StencilOp::Keep, RHI::StencilOp::DecrWrap,
        RHI::StencilFunc::Always, RHI::StencilOp::Keep, RHI::StencilOp::Keep, RHI::StencilOp::IncrWrap);

    backEnd.stencilStates[RenderBackEnd::PreDefinedStencilState::VolumeIntersectionZFail] =
        rhi.CreateStencilState(~0, ~0, 
        RHI::StencilFunc::Always, RHI::StencilOp::Keep, RHI::StencilOp::IncrWrap, RHI::StencilOp::Keep,
        RHI::StencilFunc::Always, RHI::StencilOp::Keep, RHI::StencilOp::DecrWrap, RHI::StencilOp::Keep);

    backEnd.stencilStates[RenderBackEnd::PreDefinedStencilState::VolumeIntersectionInsideZFail] =
        rhi.CreateStencilState(~0, ~0, 
        RHI::StencilFunc::Always, RHI::StencilOp::Keep, RHI::StencilOp::IncrWrap, RHI::StencilOp::Keep,
        RHI::StencilFunc::Never, RHI::StencilOp::Keep, RHI::StencilOp::Keep, RHI::StencilOp::Keep);

    backEnd.stencilStates[RenderBackEnd::PreDefinedStencilState::VolumeIntersectionTest] =
        rhi.CreateStencilState(~0, 0, 
        RHI::StencilFunc::Equal, RHI::StencilOp::Keep, RHI::StencilOp::Keep, RHI::StencilOp::Keep,
        RHI::StencilFunc::Never, RHI::StencilOp::Keep, RHI::StencilOp::Keep, RHI::StencilOp::Keep);
}

static void RB_FreeStencilStates() {
    for (int i = 0; i < RenderBackEnd::PreDefinedStencilState::Count; i++) {
        rhi.DestroyStencilState(backEnd.stencilStates[i]);
    }
}

static void RB_InitLightQueries() {
    /*for (int i = 0; i < MAX_LIGHTS; i++) {
        backEnd.lightQueries[i].queryHandle = rhi.CreateQuery(RHI::QueryType::Occlusion);
        backEnd.lightQueries[i].light = nullptr;
        backEnd.lightQueries[i].frameCount = 0;
        backEnd.lightQueries[i].resultSamples = 0;
    }*/
}

static void RB_FreeLightQueries() {
    /*for (int i = 0; i < MAX_LIGHTS; i++) {
        rhi.DestroyQuery(backEnd.lightQueries[i].queryHandle);
    }*/
}

void RB_Init() {
    backEnd.batch.Init();

    RB_InitStencilStates();
    
    RB_InitLightQueries();

    PP_Init();

    memset(backEnd.csmUpdate, 0, sizeof(backEnd.csmUpdate));

    backEnd.dfgSumGgxTexture = textureManager.AllocTexture("_dfgSumGgx");
    backEnd.dfgSumGgxTexture->Load("Data/EngineTextures/DFGSumGGX.dds",
        Texture::Flag::Clamp | Texture::Flag::Nearest | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality);

    if (r_HOM.GetBool()) {
        // TODO: create one for each context
        backEnd.homCullingOutputTexture = textureManager.AllocTexture("_homCullingOutput");
        backEnd.homCullingOutputTexture->CreateEmpty(RHI::TextureType::Texture2D, HOM_CULL_TEXTURE_WIDTH, HOM_CULL_TEXTURE_HEIGHT, 1, 1, 1,
            Image::Format::RGBA_8_8_8_8, Texture::Flag::Clamp | Texture::Flag::Nearest | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality);
        backEnd.homCullingOutputRT = RenderTarget::Create(backEnd.homCullingOutputTexture, nullptr, 0);
    }

    backEnd.initialized = true;
}

void RB_Shutdown() {
    backEnd.initialized = false;

    PP_Free();

    RB_FreeLightQueries();

    RB_FreeStencilStates();

    backEnd.batch.Shutdown();
}

static const void *RB_ExecuteBeginContext(const void *data) {
    BeginContextRenderCommand *cmd = (BeginContextRenderCommand *)data;	

    backEnd.ctx = cmd->renderContext;

    return (const void *)(cmd + 1);
}

void RB_SetupLight(VisLight *visLight) {
    const RenderLight *renderLight = visLight->def;

    const Material *lightMaterial = renderLight->GetState().material;

    /*float *outputValues = (float *)frameData.Alloc(lightMaterial->GetExprChunk()->NumRegisters() * sizeof(float));
    float localParms[MAX_EXPR_LOCALPARMS];
    localParms[0] = backEnd.ctx->elapsedTime;
    memcpy(&localParms[1], renderLight->parms.materialParms, sizeof(renderLight->parms.materialParms));
    lightMaterial->GetExprChunk()->Evaluate(localParms, outputValues);*/

    visLight->materialRegisters = nullptr;//outputValues;

    const Material::ShaderPass *lightPass = lightMaterial->GetPass();

    if (lightPass->useOwnerColor) {
        visLight->lightColor = Color4(&renderLight->GetState().materialParms[RenderObject::MaterialParm::Red]);
    } else {
        visLight->lightColor = Color4(lightPass->constantColor);
    }

    if (rhi.IsSRGBWriteEnabled()) {
        // Linearize light color
        visLight->lightColor.ToColor3() = visLight->lightColor.ToColor3().SRGBToLinear();
    }

    // Build light texture transform matrix
    ALIGN_AS32 float lightTexMatrix[2][4];
    lightTexMatrix[0][0] = lightPass->tcScale[0];
    lightTexMatrix[0][1] = 0.0f;
    lightTexMatrix[0][2] = 0.0f;
    lightTexMatrix[0][3] = lightPass->tcTranslation[0];

    lightTexMatrix[1][0] = 0.0f;
    lightTexMatrix[1][1] = lightPass->tcScale[1];
    lightTexMatrix[1][2] = 0.0f;
    lightTexMatrix[1][3] = lightPass->tcTranslation[1];

    const Mat4 &viewProjScaleBiasMat = visLight->def->GetViewProjScaleBiasMatrix();

    visLight->viewProjTexMatrix[0][0] = lightTexMatrix[0][0] * viewProjScaleBiasMat[0][0] + /*lightTexMatrix[0][1] * viewProjScaleBiasMat[1][0] + */lightTexMatrix[0][3] * viewProjScaleBiasMat[3][0];
    visLight->viewProjTexMatrix[0][1] = lightTexMatrix[0][0] * viewProjScaleBiasMat[0][1] + /*lightTexMatrix[0][1] * viewProjScaleBiasMat[1][1] + */lightTexMatrix[0][3] * viewProjScaleBiasMat[3][1];
    visLight->viewProjTexMatrix[0][2] = lightTexMatrix[0][0] * viewProjScaleBiasMat[0][2] + /*lightTexMatrix[0][1] * viewProjScaleBiasMat[1][2] + */lightTexMatrix[0][3] * viewProjScaleBiasMat[3][2];
    visLight->viewProjTexMatrix[0][3] = lightTexMatrix[0][0] * viewProjScaleBiasMat[0][3] + /*lightTexMatrix[0][1] * viewProjScaleBiasMat[1][3] + */lightTexMatrix[0][3] * viewProjScaleBiasMat[3][3];

    visLight->viewProjTexMatrix[1][0] = /*lightTexMatrix[1][0] * viewProjScaleBiasMat[0][0] + */lightTexMatrix[1][1] * viewProjScaleBiasMat[1][0] + lightTexMatrix[1][3] * viewProjScaleBiasMat[3][0];
    visLight->viewProjTexMatrix[1][1] = /*lightTexMatrix[1][0] * viewProjScaleBiasMat[0][1] + */lightTexMatrix[1][1] * viewProjScaleBiasMat[1][1] + lightTexMatrix[1][3] * viewProjScaleBiasMat[3][1];
    visLight->viewProjTexMatrix[1][2] = /*lightTexMatrix[1][0] * viewProjScaleBiasMat[0][2] + */lightTexMatrix[1][1] * viewProjScaleBiasMat[1][2] + lightTexMatrix[1][3] * viewProjScaleBiasMat[3][2];
    visLight->viewProjTexMatrix[1][3] = /*lightTexMatrix[1][0] * viewProjScaleBiasMat[0][3] + */lightTexMatrix[1][1] * viewProjScaleBiasMat[1][3] + lightTexMatrix[1][3] * viewProjScaleBiasMat[3][3];

    visLight->viewProjTexMatrix[2][0] = viewProjScaleBiasMat[2][0];
    visLight->viewProjTexMatrix[2][1] = viewProjScaleBiasMat[2][1];
    visLight->viewProjTexMatrix[2][2] = viewProjScaleBiasMat[2][2];
    visLight->viewProjTexMatrix[2][3] = viewProjScaleBiasMat[2][3];

    visLight->viewProjTexMatrix[3][0] = viewProjScaleBiasMat[3][0];
    visLight->viewProjTexMatrix[3][1] = viewProjScaleBiasMat[3][1];
    visLight->viewProjTexMatrix[3][2] = viewProjScaleBiasMat[3][2];
    visLight->viewProjTexMatrix[3][3] = viewProjScaleBiasMat[3][3];
}

void RB_DrawLightVolume(const RenderLight *light) {
    switch (light->GetType()) {
    case RenderLight::Type::Directional:
        RB_DrawOBB(light->GetWorldOBB());
        break;
    case RenderLight::Type::Point:
        if (light->IsRadiusUniform()) {
            RB_DrawSphere(Sphere(light->GetOrigin(), light->GetRadius()[0]), 16, 16);
        } else {
            // FIXME: ellipsoid 그리기로 바꾸자
            RB_DrawOBB(light->GetWorldOBB());
        }
        break;
    case RenderLight::Type::Spot:
        RB_DrawFrustum(light->GetWorldFrustum());
        break;
    default:
        break;
    }
}

static void RB_DrawStencilLightVolume(const VisLight *light, bool insideLightVolume) {
    rhi.SetStateBits(RHI::DF_LEqual);

    if (insideLightVolume) {
        rhi.SetCullFace(RHI::CullType::None);
        rhi.SetStencilState(backEnd.stencilStates[RenderBackEnd::PreDefinedStencilState::VolumeIntersectionInsideZFail], 0);
    } else {
        rhi.SetCullFace(RHI::CullType::None);
        rhi.SetStencilState(backEnd.stencilStates[RenderBackEnd::PreDefinedStencilState::VolumeIntersectionZPass], 0);
    }

    RB_DrawLightVolume(light->def);

    rhi.SetStencilState(RHI::NullStencilState, 0);
}

// NOTE: ambient pass 이후에 실행되므로 화면에 깊이값은 채워져있다
static void RB_MarkOcclusionVisibleLights(int numLights, VisLight **lights) {
/*	bool    insideLightVolume;
    Rect    prevScissorRect;
    int     numQueryWait = 0;
    int     numQueryResult = 0;
    int     numVisLights = 0;

    if (r_useLightScissors.GetBool()) {
        prevScissorRect = rhi.GetScissor();
    }

    for (int i = 0; i < numLights; i++) {
        VisLight *light = lights[i];
        if (light->def->GetState().flags & RenderLight::PrimaryLight) {
            continue;
        }

        LightQuery *lightQuery = &backEnd.lightQueries[light->index];
            
        // light 가 query 중이었다면..
        if (lightQuery->light) {
            // 아직 query result 를 사용할 수 없고, query wait frame 한도를 넘지 않았다면..
            if (!rhi.QueryResultAvailable(lightQuery->queryHandle) && renderConfig.frameCount - lightQuery->frameCount < r_queryWaitFrames.GetInteger()) {
                // 이전 result sample 로 visibility 를 판단한다
                if (lightQuery->resultSamples >= 10) {
                    light->occlusionVisible = true;
                    numVisLights++;
                } else {
                    //BE_LOG("%i\n", lightQuery->resultSamples);
                }

                numQueryWait++;
                continue;
            }

            // query result 를 가져온다. unavailable 일 경우 blocking 상태가 된다.
            lightQuery->resultSamples = rhi.QueryResult(lightQuery->queryHandle);
            numQueryResult++;
        } else {
            lightQuery->light = light;
            lightQuery->resultSamples = 10; // visible threshold 값
        }

        if (lightQuery->resultSamples >= 10) {
            light->occlusionVisible = true;
            numVisLights++;
        }
        
        lightQuery->frameCount = renderConfig.frameCount;

        rhi.Clear(ClearStencil, Vec4::zero, 0.0f, 0);

        if (r_useLightScissors.GetBool()) {
            rhi.SetScissor(light->scissorRect);
        }

        // 카메라가 light volume 안에 있는지 체크
        if (light->def->GetState().type == LT_OMNI) {
            if (light->def->IsRadiusUniform()) {
                float radius = light->def->GetRadius().x;
                insideLightVolume = light->def->GetState().origin.DistanceSqr(backEnd.camera->origin) < radius * radius ? true : false;
            } else {
                Vec3 p = (backEnd.camera->origin - light->def->GetState().origin) * light->def->GetState().axis;
                p /= light->def->GetState().value;
                insideLightVolume = p.LengthSqr() <= 1.0f ? true : false;
            }
        } else if (light->def->GetState().type == LT_PROJECTED) {
            insideLightVolume = !light->def->frustum.CullPoint(backEnd.camera->origin);
        } else if (light->def->GetState().type == LT_DIRECTIONAL) {
            insideLightVolume = light->def->obb.IsContainPoint(backEnd.camera->origin);
        } else {
            assert(0);
        }
        
        RB_DrawStencilLightVolume(light, insideLightVolume);

        rhi.SetStateBits(0);
        rhi.SetCullFace(BackCull);
        rhi.SetStencilState(backEnd.stencilStates[VolumeIntersectionTest], 1);
                
        rhi.BeginQuery(lightQuery->queryHandle);
        RB_DrawLightVolume(light->def);
        rhi.EndQuery(lightQuery->queryHandle);
    }

    rhi.SetStencilState(NullStencilState, 0);

    if (r_useLightScissors.GetBool()) {
        rhi.SetScissor(prevScissorRect);
    }

    if (r_showLights.GetInteger() > 0) {
        BE_LOG("lights: %i/%i (visible/total), queries: %i/%i (result/wait)\n", numVisLights, numLights, numQueryResult, numQueryWait);
    }*/
}

static void RB_RenderOcclusionMap(int numDrawSurfs, DrawSurf **drawSurfs) {
    Rect prevViewportRect = rhi.GetViewport();

    backEnd.ctx->homRT->Begin();
    backEnd.ctx->homRT->Clear(Color4::red, 1.0f, 0);
    rhi.SetViewport(Rect(0, 0, backEnd.ctx->homRT->GetWidth(), backEnd.ctx->homRT->GetHeight()));

    RB_OccluderPass(numDrawSurfs, drawSurfs);

    backEnd.ctx->homRT->End();
    rhi.SetViewport(prevViewportRect);
}

static void RB_GenerateOcclusionMapHierarchy() {
    uint32_t startTime = PlatformTime::Milliseconds();

    Rect prevViewportRect = rhi.GetViewport();

    int w = backEnd.ctx->homRT->GetWidth();
    int h = backEnd.ctx->homRT->GetHeight();

    float size = Max(w, h);
    int numLevels = Math::Log(2.0f, size);
    
    const Shader *shader = ShaderManager::generateHomShader;

    shader->Bind();

    Vec2 texelSize;

    for (int i = 1; i < numLevels; i++) {
        int lastMipLevel = i - 1;
        
        texelSize.Set(1.0f / w, 1.0f / h);

        w >>= 1;
        h >>= 1;
        w = w == 0 ? 1 : w;
        h = h == 0 ? 1 : h;

        backEnd.ctx->homTexture->Bind();
        rhi.SetTextureLevel(lastMipLevel, lastMipLevel);

        backEnd.ctx->homRT->Begin(i);

        rhi.SetViewport(Rect(0, 0, w, h));
        rhi.SetStateBits(RHI::DepthWrite | RHI::DF_Always);
        rhi.SetCullFace(RHI::CullType::None);

        shader->SetTexture("lastMip", backEnd.ctx->homTexture);
        shader->SetConstant2f("texelSize", texelSize);

        RB_DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);

        backEnd.ctx->homRT->End();
    }

    backEnd.ctx->homTexture->Bind();
    rhi.SetTextureLevel(0, numLevels);

    rhi.SetViewport(prevViewportRect);

    backEnd.ctx->GetRenderCounter().homGenMsec = PlatformTime::Milliseconds() - startTime;
}

static void RB_QueryOccludeeAABBs(int numAmbientOccludees, const AABB *occludeeAABB) {
    uint32_t startTime = PlatformTime::Milliseconds();

    // alloc ambient occludee buffer in a AABB form
    struct Occludee { Vec2 position; Vec3 center; Vec3 extents; };
    Occludee *occludeeBuffer = (Occludee *)_alloca16(numAmbientOccludees * sizeof(Occludee));
    Occludee *occludeePtr = occludeeBuffer;
    int x = 0;
    int y = 0;

    for (int i = 0; i < numAmbientOccludees; i++) {
        occludeePtr->position.x = (float)x / backEnd.homCullingOutputRT->GetWidth();
        occludeePtr->position.y = (float)y / backEnd.homCullingOutputRT->GetHeight();
        occludeePtr->position = occludeePtr->position * 2 - Vec2(1);
        occludeePtr->center = occludeeAABB[i].Center();
        occludeePtr->extents = occludeeAABB[i].Extents();
        occludeePtr++;

        x++;

        if (x >= backEnd.homCullingOutputRT->GetWidth()) {
            x = 0;
            y++;
        }
    }

    Rect prevViewportRect = rhi.GetViewport();

    // Query HOM culling for each occludees
    backEnd.homCullingOutputRT->Begin();

    const Shader *shader = ShaderManager::queryHomShader;

    shader->Bind();
    shader->SetTexture("homap", backEnd.ctx->homTexture);
    shader->SetConstant4x4f("viewProjectionMatrix", true, backEnd.camera->def->GetViewProjMatrix());
    shader->SetConstant2f("viewportSize", Vec2(backEnd.ctx->homRT->GetWidth(), backEnd.ctx->homRT->GetHeight()));
    int size = Max(backEnd.ctx->homRT->GetWidth(), backEnd.ctx->homRT->GetHeight());
    int maxLevel = Math::Log(2, size) - 1;
    shader->SetConstant1i("maxLevel", maxLevel);

    rhi.SetViewport(Rect(0, 0, backEnd.homCullingOutputRT->GetWidth(), backEnd.homCullingOutputRT->GetHeight()));
    rhi.SetStateBits(RHI::ColorWrite | RHI::AlphaWrite);
    rhi.SetCullFace(RHI::CullType::None);

    rhi.BindBuffer(RHI::BufferType::Vertex, bufferCacheManager.streamVertexBuffer);
    rhi.BufferDiscardWrite(bufferCacheManager.streamVertexBuffer, numAmbientOccludees * sizeof(occludeeBuffer[0]), occludeeBuffer);

    rhi.SetVertexFormat(vertexFormats[VertexFormat::Type::Occludee].vertexFormatHandle);
    rhi.SetStreamSource(0, bufferCacheManager.streamVertexBuffer, 0, sizeof(occludeeBuffer[0]));
    rhi.DrawArrays(RHI::Topology::PointList, 0, numAmbientOccludees);

    backEnd.homCullingOutputRT->Discard(true, true, 0);
    backEnd.homCullingOutputRT->End();

    rhi.SetViewport(prevViewportRect);

    backEnd.ctx->GetRenderCounter().homQueryMsec = PlatformTime::Milliseconds() - startTime;
}

static void RB_MarkOccludeeVisibility(int numAmbientOccludees, const int *occludeeSurfIndexes, int numDrawSurfs, DrawSurf **drawSurfs) {
    uint32_t startTime = PlatformTime::Milliseconds();

    // write back for visibility information to each surf
    int size = backEnd.homCullingOutputTexture->MemRequired(false);
    byte *visibilityBuffer = (byte *)_alloca(size);
    backEnd.homCullingOutputTexture->Bind();
    backEnd.homCullingOutputTexture->GetTexels2D(0, Image::Format::RGBA_8_8_8_8, visibilityBuffer);
    byte *visibilityPtr = visibilityBuffer;

    for (int i = 0; i < numAmbientOccludees; i++) {
        int index = occludeeSurfIndexes[i];
        DrawSurf *surf = drawSurfs[index];

        if (visibilityPtr[2] == 0) {
            const VisObject *space = surf->space;

            surf->flags &= ~DrawSurf::Flag::Visible;

            if (space->def->GetState().joints) {
                int sameEntityIndex = index + 1;
                while (sameEntityIndex < numDrawSurfs) {
                    surf = drawSurfs[sameEntityIndex];
                    if (surf->space != space) {
                        break;
                    }

                    surf->flags &= ~DrawSurf::Flag::Visible;
                    sameEntityIndex++;
                }
            }
        }
        visibilityPtr += 4;
    }

    backEnd.ctx->GetRenderCounter().homCullMsec = PlatformTime::Milliseconds() - startTime;
}

static void RB_TestOccludeeBounds(int numDrawSurfs, DrawSurf **drawSurfs) {
    const VisObject *prevSpace = nullptr;

    // count ambient occludees for culling
    AABB *occludeeAABB = (AABB *)_alloca(numDrawSurfs * sizeof(AABB));
    int *occludeeSurfIndexes = (int *)_alloca(numDrawSurfs * sizeof(int));
    int numAmbientOccludees = 0;

    Plane nearPlane = backEnd.camera->def->GetFrustumPlane(0);
    nearPlane.Normalize();
    nearPlane.TranslateSelf(backEnd.camera->def->GetState().origin);
    
    for (int i = 0; i < numDrawSurfs; i++) {
        const DrawSurf *surf = drawSurfs[i];

        const VisObject *space = surf->space;

        if (space->def->GetState().joints) {
            if (space == prevSpace) {
                continue;
            }
            occludeeAABB[numAmbientOccludees] = space->def->GetWorldAABB();
        } else {
            occludeeAABB[numAmbientOccludees].SetFromTransformedAABBFast(surf->subMesh->GetAABB(), space->def->GetWorldMatrix());
        }
        
        //BE_LOG("%.2f %.2f %.2f %.2f\n", nearPlane.a, nearPlane.b, nearPlane.c, nearPlane.d);
        if (occludeeAABB[numAmbientOccludees].PlaneSide(nearPlane) != Plane::Side::Back) {
            continue;
        }
        
        occludeeSurfIndexes[numAmbientOccludees] = i;
        numAmbientOccludees++;

        prevSpace = space;
    }

    if (numAmbientOccludees == 0) {
        return;
    }

    RB_QueryOccludeeAABBs(numAmbientOccludees, occludeeAABB);

    RB_MarkOccludeeVisibility(numAmbientOccludees, occludeeSurfIndexes, numDrawSurfs, drawSurfs);
}

static void RB_HiOcclusionPass(int numDrawSurfs, DrawSurf **drawSurfs) {
    if (!r_HOM.GetBool()) {
        return;
    }

    if (backEnd.camera->def->GetState().clearMethod != RenderCamera::ClearMethod::Color || 
        backEnd.camera->def->GetState().clearMethod != RenderCamera::ClearMethod::Skybox) {
        return;
    }

    // Render occluder to HiZ occlusion buffer
    RB_RenderOcclusionMap(backEnd.numAmbientSurfs, backEnd.drawSurfs);

    // Generate depth hierarchy
    RB_GenerateOcclusionMapHierarchy();

    // Test all the ambient occludee's AABB using HiZ occlusion buffer
    RB_TestOccludeeBounds(backEnd.numAmbientSurfs, backEnd.drawSurfs);
}

static void RB_ClearView() {
    BE_SCOPE_PROFILE_CPU("RB_ClearView");
    BE_SCOPE_PROFILE_GPU("RB_ClearView");

    int clearBits = 0;

    if (backEnd.camera->def->GetState().clearMethod == RenderCamera::ClearMethod::DepthOnly || 
        backEnd.camera->def->GetState().clearMethod == RenderCamera::ClearMethod::Skybox) {
        clearBits = RHI::ClearBit::Depth | RHI::ClearBit::Stencil;

        rhi.SetStateBits(rhi.GetStateBits() | RHI::DepthWrite);
        rhi.Clear(clearBits, Color4::black, 1.0f, 0);
    } else if (backEnd.camera->def->GetState().clearMethod == RenderCamera::ClearMethod::Color) {
        clearBits = RHI::ClearBit::Depth | RHI::ClearBit::Stencil | RHI::ClearBit::Color;
        Color4 clearColor = backEnd.camera->def->GetState().clearColor;

        rhi.SetStateBits(rhi.GetStateBits() | RHI::DepthWrite | RHI::ColorWrite | RHI::AlphaWrite);
        rhi.Clear(clearBits, clearColor, 1.0f, 0);
    }
}

static void RB_RenderView() {
    BE_SCOPE_PROFILE_CPU("RB_RenderView");
    BE_SCOPE_PROFILE_GPU("RB_RenderView");

    if (backEnd.camera->def->GetState().flags & RenderCamera::Flag::TexturedMode) {
        // Render pass for HiZ occlusion culling.
        RB_HiOcclusionPass(backEnd.numAmbientSurfs, backEnd.drawSurfs);

        // Render opaque/perforated surface to depth buffer fast for early-z culling ([depth] + unlit + [normal])
        RB_DepthPrePass(backEnd.numAmbientSurfs, backEnd.drawSurfs);

        // Render all solid (non-translucent) geometry ([depth] + base + [primary lit])
        RB_ForwardBasePass(backEnd.numAmbientSurfs, backEnd.drawSurfs);

        // Render all shadow and light interaction.
        RB_ForwardAdditivePass(backEnd.visLights);

        // Render any stage with blend surfaces.
        RB_BlendPass(backEnd.numAmbientSurfs, backEnd.drawSurfs);

        // Render to velocity map.
        RB_VelocityMapPass(backEnd.numAmbientSurfs, backEnd.drawSurfs);

        // Render no lighting interaction surfaces.
        RB_FinalPass(backEnd.numAmbientSurfs, backEnd.drawSurfs);

        // Render wireframe for option.
        RB_DrawTris(backEnd.numAmbientSurfs, backEnd.drawSurfs, false);
    }

    if (backEnd.camera->def->GetState().flags & RenderCamera::Flag::WireFrameMode) {
        if (!(backEnd.camera->def->GetState().flags & RenderCamera::Flag::TexturedMode)) {
            RB_BackgroundPass(backEnd.numAmbientSurfs, backEnd.drawSurfs);
        }

        // Render wireframes.
        RB_DrawTris(backEnd.numAmbientSurfs, backEnd.drawSurfs, true);
    }

    // Render debug surfaces.
    if (!(backEnd.camera->def->GetState().flags & RenderCamera::Flag::SkipDebugDraw)) {
        RB_DebugToolsPass(backEnd.numAmbientSurfs, backEnd.drawSurfs);
    }
}

static void RB_DrawDebugShadowMap() {
    rhi.SetStateBits(RHI::ColorWrite);

    if (r_showShadows.GetInteger() == 1) {
        float w = 100.0f;
        float h = 100.0f;

        backEnd.ctx->AdjustFrom640x480(nullptr, nullptr, &w, &h);

        float space = 1;
        float x = space;
        float y = space;

        const Texture *shadowTexture = backEnd.ctx->shadowMapRT->DepthStencilTexture();

        shadowTexture->Bind();
        rhi.SetTextureShadowFunc(false);

        const Shader *shader = ShaderManager::drawArrayTextureShader;

        shader->Bind();
        shader->SetTexture("tex0", shadowTexture);

        for (int i = 0; i < r_CSM_count.GetInteger(); i++) {
            RB_DrawScreenRectSlice(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, i);
            x += w + space;
        }

        rhi.SetTextureShadowFunc(true);

        x = space;
        y += h + space;
    } else if (r_showShadows.GetInteger() == 2) {
        float w = 200.0f;
        float h = 200.0f;

        backEnd.ctx->AdjustFrom640x480(nullptr, nullptr, &w, &h);

        float space = 1;
        float x = space;
        float y = space;

        const Texture *shadowTexture = backEnd.ctx->vscmRT->DepthStencilTexture();

        shadowTexture->Bind();
        rhi.SetTextureShadowFunc(false);

        const Shader *shader = ShaderManager::postPassThruShader;

        shader->Bind();
        shader->SetTexture("tex0", shadowTexture);

        RB_DrawScreenRect(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);

        rhi.SetTextureShadowFunc(true);

        x += w + space;

        if (x + w > backEnd.ctx->GetDeviceWidth()) {
            x = space;
            y += h + space;
        }
    }
}

void RB_DrawRenderTargetTexture() {
    float x, y, w, h;

    if (r_showRenderTarget_fullscreen.GetBool()) {
        x = 0.0f;
        y = 0.0f;

        w = backEnd.ctx->GetDeviceWidth();
        h = backEnd.ctx->GetDeviceHeight();
    } else {
        x = 10.0f;
        y = 10.0f;

        w = backEnd.ctx->GetDeviceWidth() / 4;
        h = backEnd.ctx->GetDeviceHeight() / 4;
    }

    rhi.SetStateBits(RHI::ColorWrite);

    int index = r_showRenderTarget.GetInteger() - 1;
    if (index < RenderTarget::rts.Count()) {
        const RenderTarget *rt = RenderTarget::rts[index];
        if (rt && rt->ColorTexture()) {
            const Shader *shader = ShaderManager::postPassThruShader;

            shader->Bind();
            shader->SetTexture("tex0", rt->ColorTexture());

            RB_DrawScreenRect(x, y, w, h, 0.0f, 1.0f, 1.0f, 0.0f);
        }
    }
}

void RB_DrawDebugHdrMap() {
    int space = 16;

    float x = space;
    float y = space;

    float w = 128.0f;
    float h = 128.0f;

    rhi.SetStateBits(RHI::ColorWrite);

    const Shader *shader = ShaderManager::postPassThruShader;

    shader->Bind();
    shader->SetTexture("tex0", backEnd.ctx->screenColorTexture);

    RB_DrawScreenRect(x, y, w, h, 0.0f, 1.0f, 1.0f, 0.0f);

    x += w + space;

    for (int i = 0; i < COUNT_OF(backEnd.ctx->hdrBloomTexture); i++) {
        shader->Bind();
        shader->SetTexture("tex0", backEnd.ctx->hdrBloomTexture[i]);

        RB_DrawScreenRect(x, y, w, h, 0.0f, 1.0f, 1.0f, 0.0f);

        x += w + space;
    }

    y += h + space;
    x = space;

    for (int i = 0; i < COUNT_OF(backEnd.ctx->hdrLumAverageTexture); i++) {
        if (!backEnd.ctx->hdrLumAverageTexture[i]) {
            break;
        }

        shader->Bind();
        shader->SetTexture("tex0", backEnd.ctx->hdrLumAverageTexture[i]);

        RB_DrawScreenRect(x, y, w, h, 0.0f, 1.0f, 1.0f, 0.0f);

        x += w + space;
    }

    y += h + space;
    x = space;

    w = 32;
    h = 32;

    for (int i = 0; i < COUNT_OF(backEnd.ctx->hdrLuminanceTexture); i++) {
        shader->Bind();
        shader->SetTexture("tex0", backEnd.ctx->hdrLuminanceTexture[i]);

        RB_DrawScreenRect(x, y, w, h, 0.0f, 1.0f, 1.0f, 0.0f);

        x += w + space;
    }
}

static void RB_DrawDebugHOMap() {
    int space = 10;

    float x = space;
    float y = space;

    float w = 100.0f;
    float h = 100.0f;

    float size = Max(backEnd.ctx->homRT->GetWidth(), backEnd.ctx->homRT->GetHeight());
    int numLevels = Math::Log(2, size);

    rhi.SetStateBits(RHI::ColorWrite);

    for (int i = 0; i < numLevels; i++) {
        backEnd.ctx->homRT->DepthStencilTexture()->Bind();
        rhi.SetTextureLevel(i, i);

        const Shader *shader = ShaderManager::postPassThruShader;

        shader->Bind();
        shader->SetTexture("tex0", backEnd.ctx->homRT->DepthStencilTexture());
        //shader->SetConstant2f("depthRange", Vec2(backEnd.camera->zNear, backEnd.camera->zFar));

        RB_DrawScreenRect(x, y, w, h, 0.0f, 1.0f, 1.0f, 0.0f);

        x += w + space;
    }

    backEnd.ctx->homRT->DepthStencilTexture()->Bind();
    rhi.SetTextureLevel(0, numLevels);
}

void RB_DrawDebugTextures() {
    int start = 0;
    int end = textureManager.textureHashMap.Count();
    
    float w = 40.0f;
    float h = 40.0f;
    
    backEnd.ctx->AdjustFrom640x480(nullptr, nullptr, &w, &h);
    
    int x = 0;
    int y = 0;
    
    rhi.SetStateBits(RHI::ColorWrite | RHI::BS_SrcAlpha | RHI::BD_OneMinusSrcAlpha);
    
    for (int i = start; i < end; i++) {
        const auto *entry = textureManager.textureHashMap.GetByIndex(i);
        Texture *texture = entry->second;
        
        if (!texture) {
            continue;
        }
        
        if (texture->GetType() == RHI::TextureType::Texture2D || texture->GetType() == RHI::TextureType::TextureRectangle) {
            const Shader *shader = ShaderManager::postPassThruShader;
            
            shader->Bind();
            shader->SetTexture("tex0", texture);
            
            if (texture->GetFlags() & Texture::Flag::Shadow) {
                texture->Bind();
                rhi.SetTextureShadowFunc(false);
            }
            
            if (texture->GetType() == RHI::TextureType::TextureRectangle) {
                RB_DrawScreenRect(x, y, w, h, 0.0f, 0.0f, texture->GetWidth(), texture->GetHeight());
            } else {
                RB_DrawScreenRect(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);
            }
            
            if (texture->GetFlags() & Texture::Flag::Shadow) {
                rhi.SetTextureShadowFunc(true);
            }
        }
        
        x += w;
        
        if ((int)(x + w) > backEnd.ctx->GetDeviceWidth()) {
            x = 0.0f;
            y += h;
        }
        
        if (y > backEnd.ctx->GetDeviceHeight()) {
            break;
        }
    }
}

// FIXME: Consider this view is sub camera
static void RB_DrawCamera3D() {
    BE_SCOPE_PROFILE_CPU("RB_DrawCamera3D");
    BE_SCOPE_PROFILE_GPU("RB_DrawCamera3D");

    if (backEnd.ctx->flags & RenderContext::Flag::UseSelectionBuffer) {
        backEnd.ctx->screenSelectionRT->Begin();

        float scaleX = (float)backEnd.ctx->screenSelectionRT->GetWidth() / backEnd.ctx->screenRT->GetWidth();
        float scaleY = (float)backEnd.ctx->screenSelectionRT->GetHeight() / backEnd.ctx->screenRT->GetHeight();

        Rect renderRect;
        renderRect.x = backEnd.renderRect.x * scaleX;
        renderRect.y = backEnd.renderRect.y * scaleY;
        renderRect.w = backEnd.renderRect.w * scaleX;
        renderRect.h = backEnd.renderRect.h * scaleY;

        rhi.SetViewport(renderRect);
        rhi.SetDepthRange(0, 1);

        rhi.SetScissor(Rect::zero);

        rhi.SetStateBits(RHI::DepthWrite | RHI::ColorWrite | RHI::AlphaWrite);
        rhi.Clear(RHI::ClearBit::Color | RHI::ClearBit::Depth, Color4::white, 1.0f, 0);

        RB_SelectionPass(backEnd.numAmbientSurfs, backEnd.drawSurfs);

        backEnd.ctx->screenSelectionRT->Discard(true, true, 0);
        backEnd.ctx->screenSelectionRT->End();
    }

    Rect upscaledRenderRect;
    upscaledRenderRect.x = Math::Round(backEnd.renderRect.x * backEnd.upscaleFactor.x);
    upscaledRenderRect.y = Math::Round(backEnd.renderRect.y * backEnd.upscaleFactor.y);
    upscaledRenderRect.w = Math::Round(backEnd.renderRect.w * backEnd.upscaleFactor.x);
    upscaledRenderRect.h = Math::Round(backEnd.renderRect.h * backEnd.upscaleFactor.y);

    if (!(backEnd.camera->def->GetState().flags & RenderCamera::Flag::SkipPostProcess) && r_usePostProcessing.GetBool()) {
        backEnd.ctx->screenRT->Begin();

        rhi.SetViewport(backEnd.renderRect);
        rhi.SetDepthRange(0, 1);

        rhi.SetScissor(Rect::zero);

        RB_ClearView();

        RB_RenderView();

        backEnd.ctx->screenRT->Discard(true, true, 0);
        backEnd.ctx->screenRT->End();

        rhi.SetViewport(upscaledRenderRect);
        rhi.SetScissor(upscaledRenderRect);

        RB_PostProcess();
    } else {
        rhi.SetViewport(upscaledRenderRect);
        rhi.SetDepthRange(0, 1);

        rhi.SetScissor(Rect::zero);

        RB_ClearView();

        RB_RenderView();
    }
}

static void RB_DrawCamera2D() {
    BE_SCOPE_PROFILE_CPU("RB_DrawCamera2D");
    BE_SCOPE_PROFILE_GPU("RB_DrawCamera2D");

    if (!backEnd.numDrawSurfs) {
        return;
    }
    
    rhi.SetViewport(backEnd.renderRect);
    rhi.SetDepthRange(0, 0);
    rhi.SetScissor(backEnd.renderRect);
    
    RB_GuiPass(backEnd.numDrawSurfs, backEnd.drawSurfs);

    rhi.SetScissor(Rect::zero);
}

static const void *RB_ExecuteDrawCamera(const void *data) {
    DrawCameraRenderCommand *cmd = (DrawCameraRenderCommand *)data;

    backEnd.camera              = &cmd->camera;
    backEnd.time                = MILLI2SEC(cmd->camera.def->GetState().time);
    backEnd.visObjects          = &cmd->camera.visObjects;
    backEnd.visLights           = &cmd->camera.visLights;
    backEnd.primaryLight        = cmd->camera.primaryLight;
    backEnd.numDrawSurfs        = cmd->camera.numDrawSurfs;
    backEnd.numAmbientSurfs     = cmd->camera.numAmbientSurfs;
    backEnd.drawSurfs           = cmd->camera.drawSurfs;
    backEnd.instanceBufferCache = cmd->camera.instanceBufferCache;
    backEnd.projMatrix          = cmd->camera.def->GetProjMatrix();
    backEnd.viewMatrix          = cmd->camera.def->GetViewMatrix();
    backEnd.viewProjMatrix      = cmd->camera.def->GetViewProjMatrix();
    backEnd.renderRect          = cmd->camera.def->GetState().renderRect;
    backEnd.upscaleFactor       = Vec2(backEnd.ctx->GetUpscaleFactorX(), backEnd.ctx->GetUpscaleFactorY());
    backEnd.useDepthPrePass     = r_useDepthPrePass.GetBool() &&
        (backEnd.camera->def->GetState().clearMethod == RenderCamera::ClearMethod::Color || backEnd.camera->def->GetState().clearMethod == RenderCamera::ClearMethod::Skybox);

    if (backEnd.camera->is2D) {
        RB_DrawCamera2D();
    } else {
        RB_DrawCamera3D();
    }

    backEnd.viewMatrixPrev = backEnd.camera->def->GetViewMatrix();

    return (const void *)(cmd + 1);
}

static const void *RB_ExecuteScreenshot(const void *data) {
    ScreenShotRenderCommand *cmd = (ScreenShotRenderCommand *)data;

    Rect captureRect(cmd->x, cmd->y, cmd->width, cmd->height);
    captureRect.y = backEnd.ctx->GetDeviceHeight() - captureRect.Y2();

    if (captureRect.X2() > backEnd.ctx->GetDeviceWidth() || captureRect.Y2() > backEnd.ctx->GetDeviceHeight()) {
        BE_WARNLOG("larger than screen size: %i, %i, %i, %i\n", cmd->x, cmd->y, cmd->width, cmd->height);
    }
    
    Image screenImage;
    screenImage.Create2D(captureRect.w, captureRect.h, 1, Image::Format::BGR_8_8_8, Image::GammaSpace::sRGB, nullptr, 0);
    rhi.ReadPixels(captureRect.x, captureRect.y, captureRect.w, captureRect.h, Image::Format::BGR_8_8_8, screenImage.GetPixels());
    screenImage.FlipY();

    // Apply gamma ramp table
    if (r_gamma.GetFloat() != 1.0) {
        uint16_t ramp[768];
        rhi.GetGammaRamp(ramp);
        screenImage.ApplyGammaRampTableRGB888(ramp);
    }

    Str filename = cmd->filename;
    filename.DefaultFileExtension(".png");

    screenImage.Write(filename);
/*
    int sw = (cmd->width + 15) & ~15;
    int sh = (cmd->height + 15) & ~15;

    byte *temp = (byte *)Mem_Alloc16(sw * sh * 3);
    byte *temp2 = (byte *)Mem_Alloc16(sw * sh * 3);

    simdProcessor->Memset(temp2, 0, sw * sh * 3);

    Image_Scale(src, cmd->width, cmd->height, temp, sw, sh, Image::BGRA_8_8_8_8, Image::BicubicFilter);
    Image_Write("ycocgtest0.tga", temp, sw, sh, Image::BGRA_8_8_8_8);

    int YCoCgSize = 3 * sw * sh;
    short *YCoCg = (short *)Mem_Alloc16(YCoCgSize);

    int wblocks = sw >> 4;
    int hblocks = sh >> 4;
    
    for (int j = 0; j < hblocks; j++) {
        for (int i = 0; i < wblocks; i++) {
            simdProcessor->RGBToYCoCg(temp + 16 * 3 * (j * sw + i), sw * 3, YCoCg + 384 * (j * wblocks + i));
            simdProcessor->YCoCgToRGB(YCoCg + 384 * (j * wblocks + i), temp2 + 16 * 3 * (j * sw + i), sw * 3);
        }
    }
    
    Image_Write("ycocgtest1.tga", temp2, sw, sh, Image::BGRA_8_8_8_8);

    Mem_Free16(YCoCg);
    Mem_Free16(temp);
    Mem_Free16(temp2);*/

    return (const void *)(cmd + 1);
}

static const void *RB_ExecuteSwapBuffers(const void *data) {
    BE_SCOPE_PROFILE_CPU("RB_ExecuteSwapBuffers");
    BE_SCOPE_PROFILE_GPU("RB_ExecuteSwapBuffers");

    SwapBuffersRenderCommand *cmd = (SwapBuffersRenderCommand *)data;

    Rect deviceRect(0, 0, backEnd.ctx->GetDeviceWidth(), backEnd.ctx->GetDeviceHeight());

    rhi.SetViewport(deviceRect);
    rhi.SetDepthRange(0, 0);

    rhi.SetScissor(deviceRect);
    rhi.SetCullFace(RHI::CullType::None);

    if (r_showTextures.GetInteger() > 0) {
        RB_DrawDebugTextures();
    }

    if (r_showShadows.GetInteger() > 0 && r_shadows.GetInteger() == 1) {
        RB_DrawDebugShadowMap();
    }

    if (r_showRenderTarget.GetInteger() > 0) {
        RB_DrawRenderTargetTexture();
    }

    if (r_HDR.GetInteger() > 0 && r_HDR_debug.GetInteger() > 0) {
        RB_DrawDebugHdrMap();
    }

    if (r_HOM.GetBool() && r_HOM_debug.GetBool()) {
        RB_DrawDebugHOMap();
    }

    rhi.SetScissor(Rect::zero);

#ifdef ENABLE_IMGUI
    rhi.ImGuiRender();
#endif

    rhi.SwapBuffers();
    
    return (const void *)(cmd + 1);
}

void RB_Execute(const void *data) {
    BE_SCOPE_PROFILE_CPU("RB_Execute");
    BE_SCOPE_PROFILE_GPU("RB_Execute");

    uint32_t t1 = PlatformTime::Milliseconds();
    uint32_t t2;

    backEnd.batch.SetCurrentLight(nullptr);
    backEnd.batch.Begin(Batch::Flush::Final, nullptr, nullptr, nullptr);
    
    while (1) {
        int cmd = *(const int *)data;
        switch (cmd) {
        case RenderCommand::BeginContext:
            data = RB_ExecuteBeginContext(data);
            continue;
        case RenderCommand::DrawCamera:
            data = RB_ExecuteDrawCamera(data);
            continue;
        case RenderCommand::ScreenShot:
            data = RB_ExecuteScreenshot(data);
            continue;
        case RenderCommand::SwapBuffers:
            data = RB_ExecuteSwapBuffers(data);
            continue;
        case RenderCommand::End:
            t2 = PlatformTime::Milliseconds();
            backEnd.ctx->GetRenderCounter().backEndMsec = t2 - t1;
            return;
        }
    }
}

BE_NAMESPACE_END
