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
#include "RBackEnd.h"

BE_NAMESPACE_BEGIN

// litVisAABB 와 viewFrustum 을 이용해서 light OBB 의 near, far 를 구한다.
static bool RB_ComputeNearFar(const Vec3 &lightOrigin, const OBB &lightOBB, const AABB &litVisAABB, const Frustum &viewFrustum, float *dNear, float *dFar) {
    float dmin1, dmax1;
    float dmin2, dmax2;

    Vec3 lightDir = lightOBB.Axis()[0];
    float lightFar = lightOBB.Extents()[0] * 2.0f;

    litVisAABB.ProjectOnAxis(lightDir, dmin1, dmax1);
    viewFrustum.ProjectOnAxis(lightDir, dmin2, dmax2);

    *dNear = dmin1 - lightDir.Dot(lightOrigin);
    
    *dFar = Max(dmax1, dmax2) - lightDir.Dot(lightOrigin);
    *dFar = Min(*dFar, lightFar);

    if (*dFar <= 0.0f || *dNear >= *dFar) {
        return false;
    }

    return true;
}

// litVisAABB 와 viewFrustum 을 이용해서 light frustum 의 near 와 far 를 구한다.
static bool RB_ComputeNearFar(const Frustum &lightFrustum, const AABB &litVisAABB, const Frustum &viewFrustum, float *dNear, float *dFar) {
    float dmin1, dmax1;
    float dmin2, dmax2;
    
    Vec3 lightDir = lightFrustum.GetAxis()[0];

    litVisAABB.ProjectOnAxis(lightDir, dmin1, dmax1);
    viewFrustum.ProjectOnAxis(lightDir, dmin2, dmax2);

    *dNear = Max(lightFrustum.GetNearDistance(), dmin1 - lightDir.Dot(lightFrustum.GetOrigin()) - 4.0f);
    *dFar = Min(lightFrustum.GetFarDistance(), Max(dmax1, dmax2) - lightDir.Dot(lightFrustum.GetOrigin()));

    if (*dFar <= lightFrustum.GetNearDistance() || *dNear >= *dFar) {
        return false;
    }

    return true;
}

static void RB_AlignProjectionBounds(float &xmin, float &xmax, float &ymin, float &ymax, const Vec2 &size, float alignSize) {
    // [-1, +1] to [0, 1]
    xmin = (xmin + 1.0f) * 0.5f;
    ymin = (ymin + 1.0f) * 0.5f;
    xmax = (xmax + 1.0f) * 0.5f;
    ymax = (ymax + 1.0f) * 0.5f;

    float invAlignSize = 1.0f / alignSize;

    xmin = Math::Floor(xmin * size.x * invAlignSize);
    ymin = Math::Floor(ymin * size.y * invAlignSize);

    xmax = xmin + r_shadowMapSize.GetFloat();
    ymax = ymin + r_shadowMapSize.GetFloat();

    float ax = alignSize / size.x;
    float ay = alignSize / size.y;

    xmin = xmin * ax;
    ymin = ymin * ay;
    xmax = xmax * ax;
    ymax = ymax * ay;
    
    //xmax = Min(xmax, 1.0f);
    //ymax = Min(ymax, 1.0f);

    // [0, 1] to [-1, +1]
    xmin = xmin * 2.0f - 1.0f;
    ymin = ymin * 2.0f - 1.0f;
    xmax = xmax * 2.0f - 1.0f;
    ymax = ymax * 2.0f - 1.0f;
}

static bool RB_ComputeShadowCropMatrix(const OBB &lightOBB, const OBB &shadowCasterOBB, const Frustum &viewFrustum, Mat4 &shadowCropMatrix) {
    // Calculates crop bounds of view frustum in light OBB space.
    AABB viewCropBounds;
    if (!lightOBB.ProjectionBounds(viewFrustum, viewCropBounds)) {
        return false;
    }

    // Calculates crop bounds of shadow caster OBB in light OBB space.
    AABB casterCropBounds;
    if (!lightOBB.ProjectionBounds(shadowCasterOBB, casterCropBounds)) {
        return false;
    }

    // Intersects crop bounds.
    AABB cropBounds;
    cropBounds[0][AxisIndex::Left] = (casterCropBounds[0][AxisIndex::Left] > viewCropBounds[0][AxisIndex::Left]) ? casterCropBounds[0][AxisIndex::Left] : viewCropBounds[0][AxisIndex::Left];
    cropBounds[1][AxisIndex::Left] = (casterCropBounds[1][AxisIndex::Left] < viewCropBounds[1][AxisIndex::Left]) ? casterCropBounds[1][AxisIndex::Left] : viewCropBounds[1][AxisIndex::Left];
    cropBounds[0][AxisIndex::Up] = (casterCropBounds[0][AxisIndex::Up] > viewCropBounds[0][AxisIndex::Up]) ? casterCropBounds[0][AxisIndex::Up] : viewCropBounds[0][AxisIndex::Up];
    cropBounds[1][AxisIndex::Up] = (casterCropBounds[1][AxisIndex::Up] < viewCropBounds[1][AxisIndex::Up]) ? casterCropBounds[1][AxisIndex::Up] : viewCropBounds[1][AxisIndex::Up];

    // Returns false if there is no intersection.
    if (cropBounds[0][AxisIndex::Left] > cropBounds[1][AxisIndex::Left] || cropBounds[0][AxisIndex::Up] > cropBounds[1][AxisIndex::Up]) {
        return false;
    }

    float xmin = -cropBounds[1][AxisIndex::Left];
    float xmax = -cropBounds[0][AxisIndex::Left];
    float ymin =  cropBounds[0][AxisIndex::Up];
    float ymax =  cropBounds[1][AxisIndex::Up];

    R_Set2DCropMatrix(xmin, xmax, ymin, ymax, shadowCropMatrix);
    return true;
}

static bool RB_ComputeShadowCropMatrix(const OBB &lightOBB, const Sphere &viewSphere, Mat4 &shadowCropMatrix) {
    // Calculate crop bounds [-1, 1] of view sphere in light OBB space.
    AABB cropBounds;
    if (!lightOBB.ProjectionBounds(viewSphere, cropBounds)) {
        return false;
    }

    float xmin = -cropBounds[1][AxisIndex::Left];
    float xmax = -cropBounds[0][AxisIndex::Left];
    float ymin =  cropBounds[0][AxisIndex::Up];
    float ymax =  cropBounds[1][AxisIndex::Up];

    if (r_shadowMapCropAlign.GetBool()) {
        float lengthPerTexel = viewSphere.Radius() * 2.0f / r_shadowMapSize.GetFloat();

        const Vec3 &e = lightOBB.Extents();
        RB_AlignProjectionBounds(xmin, xmax, ymin, ymax, Vec2(e.y, e.z) * 2.0f, lengthPerTexel);
    }

    R_Set2DCropMatrix(xmin, xmax, ymin, ymax, shadowCropMatrix);
    return true;
}

static bool RB_ComputeShadowCropMatrix(const Frustum &lightFrustum, const OBB &shadowCasterOBB, const Frustum &viewFrustum, Mat4 &shadowCropMatrix) {
    // Make crop bounds.
    AABB casterCropBounds, viewCropBounds;
    lightFrustum.ProjectionBounds(viewFrustum, viewCropBounds);
    lightFrustum.ProjectionBounds(shadowCasterOBB, casterCropBounds);

    // Intersection of two crop bounds.
    AABB cropBounds;
    cropBounds[0][AxisIndex::Left] = (casterCropBounds[0][AxisIndex::Left] > viewCropBounds[0][AxisIndex::Left]) ? casterCropBounds[0][AxisIndex::Left] : viewCropBounds[0][AxisIndex::Left];
    cropBounds[1][AxisIndex::Left] = (casterCropBounds[1][AxisIndex::Left] < viewCropBounds[1][AxisIndex::Left]) ? casterCropBounds[1][AxisIndex::Left] : viewCropBounds[1][AxisIndex::Left];
    cropBounds[0][AxisIndex::Up] = (casterCropBounds[0][AxisIndex::Up] > viewCropBounds[0][AxisIndex::Up]) ? casterCropBounds[0][AxisIndex::Up] : viewCropBounds[0][AxisIndex::Up];
    cropBounds[1][AxisIndex::Up] = (casterCropBounds[1][AxisIndex::Up] < viewCropBounds[1][AxisIndex::Up]) ? casterCropBounds[1][AxisIndex::Up] : viewCropBounds[1][AxisIndex::Up];

    if (cropBounds[0][1] > cropBounds[1][1] || cropBounds[0][2] > cropBounds[1][2]) {
        return false;
    }

    float xmin = -cropBounds[1][AxisIndex::Left];
    float xmax = -cropBounds[0][AxisIndex::Left];
    float ymin =  cropBounds[0][AxisIndex::Up];
    float ymax =  cropBounds[1][AxisIndex::Up];

    R_Set2DCropMatrix(xmin, xmax, ymin, ymax, shadowCropMatrix);
    //BE_LOG("%f %f, %f %f\n", xmin, xmax, ymin, ymax);

    return true;
}

static bool RB_ComputeShadowCropMatrix(const Frustum &lightFrustum, const Frustum &viewFrustum, Mat4 &shadowCropMatrix) {
    AABB cropBounds;
    lightFrustum.ProjectionBounds(viewFrustum, cropBounds);

    float xmin = -cropBounds[1][AxisIndex::Left];
    float xmax = -cropBounds[0][AxisIndex::Left];
    float ymin =  cropBounds[0][AxisIndex::Up];
    float ymax =  cropBounds[1][AxisIndex::Up];

    R_Set2DCropMatrix(xmin, xmax, ymin, ymax, shadowCropMatrix);
    //BE_LOG("%f %f, %f %f\n", xmin, xmax, ymin, ymax);

    return true;
}

static bool RB_ShadowCubeMapFacePass(const VisLight *visLight, const Mat4 &lightViewMatrix, const Frustum &lightFrustum, const Frustum &viewFrustum, bool forceClear, int cubeMapFace) {
    const VisObject *   prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const VisObject *   skipObject = nullptr;
    const VisObject *   entity2 = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                firstDraw = true;

    backEnd.batch.SetCurrentLight(visLight);
    
    for (int i = 0; i < visLight->numDrawSurfs; i++) {
        const DrawSurf *drawSurf = backEnd.drawSurfs[visLight->firstDrawSurf + i];

        if (!(drawSurf->flags & DrawSurf::Flag::ShadowVisible)) {
            continue;
        }

        if (drawSurf->space == skipObject) {
            continue;
        }

        if (!(drawSurf->material->GetSort() == Material::Sort::Opaque || drawSurf->material->GetSort() == Material::Sort::AlphaTest) &&
            !(drawSurf->material->GetFlags() & Material::Flag::ForceShadow)) {
            continue;
        }

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentSubMesh = prevSubMesh ? !drawSurf->subMesh->IsShared(prevSubMesh) : true;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;
        bool isDifferentInstance = !(drawSurf->flags & DrawSurf::Flag::UseInstancing) || isDifferentMaterial || isDifferentSubMesh || !prevSpace || prevSpace->def->GetState().flags != drawSurf->space->def->GetState().flags || prevSpace->def->GetState().layer != drawSurf->space->def->GetState().layer ? true : false;

        if (isDifferentObject || isDifferentSubMesh || isDifferentMaterial) {
            if (prevMaterial && isDifferentInstance) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(Batch::Flush::Shadow, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);

            prevSubMesh = drawSurf->subMesh;
            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                prevSpace = drawSurf->space;

                if (!(drawSurf->space->def->GetState().flags & RenderObject::Flag::CastShadows)) {
                    continue;
                }

                OBB obb(drawSurf->space->def->GetWorldOBB());
                if (lightFrustum.CullOBB(obb)) {
                    skipObject = drawSurf->space;
                    continue;
                }

                skipObject = nullptr;
            }
        }

        if (!drawSurf->space->def->GetState().joints) {
            OBB obb(drawSurf->subMesh->GetAABB(), drawSurf->space->def->GetWorldMatrix());
            if (lightFrustum.CullOBB(obb)) {
                continue;
            }
        }

        if (firstDraw) {
            firstDraw = false;

            backEnd.ctx->vscmRT->Begin();

            int vscmFaceWidth = backEnd.ctx->vscmRT->GetWidth() / 3;
            int vscmFaceHeight = backEnd.ctx->vscmRT->GetHeight() / 2;

            Rect faceRect;
            faceRect.x = vscmFaceWidth * (cubeMapFace >> 1);
            faceRect.y = vscmFaceHeight * (cubeMapFace & 1);
            faceRect.w = vscmFaceWidth;
            faceRect.h = vscmFaceHeight;

            rhi.SetViewport(faceRect);
            rhi.SetScissor(faceRect);

            if (!backEnd.ctx->vscmCleared[cubeMapFace]) {
                rhi.SetStateBits(RHI::DepthWrite);
                rhi.Clear(RHI::ClearBit::Depth, Color4::black, 1.0f, 0);
            } else {
                backEnd.ctx->vscmCleared[cubeMapFace] = false;
            }
        }

        if (drawSurf->space != entity2) {
            if (!(drawSurf->flags & DrawSurf::Flag::UseInstancing)) {
                backEnd.modelViewMatrix = lightViewMatrix * drawSurf->space->def->GetWorldMatrix();
                backEnd.modelViewProjMatrix = backEnd.projMatrix * backEnd.modelViewMatrix;
            } else {
                backEnd.batch.AddInstance(drawSurf);
            }

            entity2 = drawSurf->space;
        }

        backEnd.batch.DrawSubMesh(drawSurf->subMesh);
    }

    if (!firstDraw) {
        backEnd.batch.Flush();

        backEnd.ctx->vscmRT->End();
    } else if (forceClear && !backEnd.ctx->vscmCleared[cubeMapFace]) {
        firstDraw = false;

        backEnd.ctx->vscmRT->Begin();

        int vscmFaceWidth = backEnd.ctx->vscmRT->GetWidth() / 3;
        int vscmFaceHeight = backEnd.ctx->vscmRT->GetHeight() / 2;

        Rect faceRect;
        faceRect.x = vscmFaceWidth * (cubeMapFace >> 1);
        faceRect.y = vscmFaceHeight * (cubeMapFace & 1);
        faceRect.w = vscmFaceWidth;
        faceRect.h = vscmFaceHeight;

        rhi.SetViewport(faceRect);
        rhi.SetScissor(faceRect);
                
        rhi.SetStateBits(RHI::DepthWrite);
        rhi.Clear(RHI::ClearBit::Depth, Color4::black, 1.0f, 0);

        backEnd.ctx->vscmRT->End();

        backEnd.ctx->vscmCleared[cubeMapFace] = true;
    }

    return !firstDraw;
}

static void RB_ShadowCubeMapPass(const VisLight *visLight, const Frustum &viewFrustum) {
    float zNear = r_shadowCubeMapZNear.GetFloat();
    float zFar = visLight->def->GetMajorRadius();
    float zRangeInv = 1.0f / (zFar - zNear);

    // Zeye 에서 depth 값을 구하기 위한 projection 행렬의 33, 43 성분을 다시 W(-Zeye) 로 나눈값
    backEnd.shadowProjectionDepth[0] = -zFar * zNear * zRangeInv;
    backEnd.shadowProjectionDepth[1] = zFar * zRangeInv;

    float fov = RAD2DEG(backEnd.ctx->vscmBiasedFov);
    R_SetPerspectiveProjectionMatrix(fov, fov, zNear, zFar, false, backEnd.shadowProjectionMatrix);

    Frustum lightFrustum;
    lightFrustum.SetOrigin(visLight->def->GetState().origin);
    float size = zFar * Math::Tan(Math::OneFourthPi);
    lightFrustum.SetSize(zNear, zFar, size, size);

    int shadowMapDraw = 0;

    Mat3 axis;

    Rect prevScissorRect = rhi.GetScissor();
    ALIGN_AS32 Mat4 prevProjMatrix = backEnd.projMatrix;
    ALIGN_AS32 Mat4 prevViewProjMatrix = backEnd.viewProjMatrix;
    backEnd.projMatrix = backEnd.shadowProjectionMatrix;

    for (int faceIndex = RHI::CubeMapFace::PositiveX; faceIndex <= RHI::CubeMapFace::NegativeZ; faceIndex++) {
        R_EnvCubeMapFaceToOpenGLAxis((RHI::CubeMapFace::Enum)faceIndex, axis);

        lightFrustum.SetAxis(axis);

        if (viewFrustum.CullFrustum(lightFrustum)) {
            continue;
        }

        ALIGN_AS32 Mat4 lightViewMatrix;
        R_SetViewMatrix(axis, visLight->def->GetState().origin, lightViewMatrix);

        backEnd.viewProjMatrix = backEnd.shadowProjectionMatrix * lightViewMatrix;

        backEnd.shadowMapOffsetFactor = visLight->def->GetState().shadowOffsetFactor;
        backEnd.shadowMapOffsetUnits = visLight->def->GetState().shadowOffsetUnits;

        rhi.SetDepthBias(backEnd.shadowMapOffsetFactor, backEnd.shadowMapOffsetUnits);

        if (RB_ShadowCubeMapFacePass(visLight, lightViewMatrix, lightFrustum, viewFrustum, true, faceIndex)) {
            shadowMapDraw++;
        }

        rhi.SetDepthBias(0.0f, 0.0f);
    }

    backEnd.projMatrix = prevProjMatrix;
    backEnd.viewProjMatrix = prevViewProjMatrix;

    rhi.SetScissor(prevScissorRect);
    rhi.SetViewport(backEnd.renderRect);

    backEnd.ctx->GetRenderCounter().numShadowMapDraw += shadowMapDraw;
}

// TODO: To be culled by for each cascades.
static bool RB_ShadowMapPass(const VisLight *visLight, const Frustum &viewFrustum, int cascadeIndex, bool forceClear) {
    const VisObject *   prevSpace = nullptr;
    const SubMesh *     prevSubMesh = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                firstDraw = true;
    Rect                prevScissorRect;

    backEnd.batch.SetCurrentLight(visLight);

    if (r_CSM_pancaking.GetBool()) {
        rhi.SetDepthClamp(true);
    }

    rhi.SetDepthBias(backEnd.shadowMapOffsetFactor, backEnd.shadowMapOffsetUnits);

    ALIGN_AS32 Mat4 prevProjMatrix = backEnd.projMatrix;
    ALIGN_AS32 Mat4 prevViewProjMatrix = backEnd.viewProjMatrix;

    backEnd.projMatrix = backEnd.shadowProjectionMatrix;
    backEnd.viewProjMatrix = backEnd.shadowProjectionMatrix * visLight->def->GetViewMatrix();
    
    for (int i = 0; i < visLight->numDrawSurfs; i++) {
        const DrawSurf *drawSurf = backEnd.drawSurfs[visLight->firstDrawSurf + i];

        if (!(drawSurf->flags & DrawSurf::Flag::ShadowVisible)) {
            continue;
        }
        
        if (!(drawSurf->material->GetSort() == Material::Sort::Opaque || drawSurf->material->GetSort() == Material::Sort::AlphaTest) && 
            !(drawSurf->material->GetFlags() & Material::Flag::ForceShadow)) {
            continue;
        }

        if (firstDraw) {
            firstDraw = false;

            backEnd.ctx->shadowMapRT->Begin(0, cascadeIndex);

            rhi.SetViewport(Rect(0, 0, backEnd.ctx->shadowMapRT->GetWidth(), backEnd.ctx->shadowMapRT->GetHeight()));

            prevScissorRect = rhi.GetScissor();
            rhi.SetScissor(Rect::zero);
            rhi.SetStateBits(RHI::DepthWrite);
            rhi.Clear(RHI::ClearBit::Depth, Color4::black, 1.0f, 0);
        }

        bool isDifferentObject = drawSurf->space != prevSpace;
        bool isDifferentSubMesh = prevSubMesh ? !drawSurf->subMesh->IsShared(prevSubMesh) : true;
        bool isDifferentMaterial = drawSurf->material != prevMaterial;
        bool isDifferentInstance = !(drawSurf->flags & DrawSurf::Flag::UseInstancing) || isDifferentMaterial || isDifferentSubMesh || !prevSpace ||
            prevSpace->def->GetState().flags != drawSurf->space->def->GetState().flags || prevSpace->def->GetState().layer != drawSurf->space->def->GetState().layer ? true : false;

        if (isDifferentObject || isDifferentSubMesh || isDifferentMaterial) {
            if (prevMaterial && isDifferentInstance) {
                backEnd.batch.Flush();
            }

            backEnd.batch.Begin(Batch::Flush::Shadow, drawSurf->material, drawSurf->materialRegisters, drawSurf->space);

            prevSubMesh = drawSurf->subMesh;
            prevMaterial = drawSurf->material;

            if (isDifferentObject) {
                if (!(drawSurf->flags & DrawSurf::Flag::UseInstancing)) {
                    backEnd.modelViewMatrix = visLight->def->GetViewMatrix() * drawSurf->space->def->GetWorldMatrix();
                    backEnd.modelViewProjMatrix = backEnd.shadowProjectionMatrix * backEnd.modelViewMatrix;
                }

                prevSpace = drawSurf->space;
            }
        }

        if (drawSurf->flags & DrawSurf::Flag::UseInstancing) {
            backEnd.batch.AddInstance(drawSurf);
        }

        backEnd.batch.DrawSubMesh(drawSurf->subMesh);
    }

    if (!firstDraw) {
        backEnd.batch.Flush();

        backEnd.ctx->shadowMapRT->End();

        rhi.SetScissor(prevScissorRect);
        rhi.SetViewport(backEnd.renderRect);
    } else if (forceClear) {
        firstDraw = false;

        backEnd.ctx->shadowMapRT->Begin(0, cascadeIndex);

        rhi.SetViewport(Rect(0, 0, backEnd.ctx->shadowMapRT->GetWidth(), backEnd.ctx->shadowMapRT->GetHeight()));
        prevScissorRect = rhi.GetScissor();
        rhi.SetScissor(Rect::zero);
        
        rhi.SetStateBits(RHI::DepthWrite);
        rhi.Clear(RHI::ClearBit::Depth, Color4::black, 1.0f, 0);

        backEnd.ctx->shadowMapRT->End();

        rhi.SetScissor(prevScissorRect);
        rhi.SetViewport(backEnd.renderRect);
    }

    backEnd.projMatrix = prevProjMatrix;
    backEnd.viewProjMatrix = prevViewProjMatrix;

    rhi.SetDepthBias(0.0f, 0.0f);

    if (r_CSM_pancaking.GetBool()) {
        rhi.SetDepthClamp(false);
    }

    return !firstDraw;
}

static void RB_OrthogonalShadowMapPass(const VisLight *visLight, const Frustum &viewFrustum) {
    backEnd.shadowViewProjectionScaleBiasMatrix[0].SetZero();

    backEnd.shadowMapOffsetFactor = visLight->def->GetState().shadowOffsetFactor;
    backEnd.shadowMapOffsetUnits = visLight->def->GetState().shadowOffsetUnits;

    float dNear, dFar;
    if (!RB_ComputeNearFar(visLight->def->GetState().origin, visLight->def->GetWorldOBB(), visLight->shadowCastersAABB, viewFrustum, &dNear, &dFar)) {
        return;
    }

    R_SetOrthogonalProjectionMatrix(visLight->def->GetWorldOBB().Extents()[1], visLight->def->GetWorldOBB().Extents()[2], dNear, dFar, backEnd.shadowProjectionMatrix);

    if (r_optimizedShadowProjection.GetInteger() == 2) {
        ALIGN_AS32 Mat4 shadowCropMatrix;
        ALIGN_AS32 OBB lightOBB = visLight->def->GetWorldOBB();

        lightOBB.SetCenter(visLight->def->GetState().origin + visLight->def->GetState().axis[0] * (dFar + dNear) * 0.5f);

        ALIGN_AS32 Vec3 extents = lightOBB.Extents();
        lightOBB.SetExtents(Vec3((dFar - dNear) * 0.5f, extents.y, extents.z));

        if (!RB_ComputeShadowCropMatrix(lightOBB, OBB(visLight->shadowCastersAABB), viewFrustum, shadowCropMatrix)) {
            return;
        }

        // Multiply the crop matrix to create an effective 'zoomed in' shadow view-projection matrix.
        backEnd.shadowProjectionMatrix = shadowCropMatrix * backEnd.shadowProjectionMatrix;
    }

    backEnd.shadowMapFilterSize[0] = r_shadowMapFilterSize.GetFloat();

    ALIGN_AS32 static const Mat4 textureScaleBiasMatrix(Vec4(0.5, 0, 0, 0.5), Vec4(0, 0.5, 0, 0.5), Vec4(0, 0, 0.5, 0.5), Vec4(0.0, 0.0, 0.0, 1));
    backEnd.shadowViewProjectionScaleBiasMatrix[0] = textureScaleBiasMatrix * backEnd.shadowProjectionMatrix * visLight->def->GetViewMatrix();

    if (RB_ShadowMapPass(visLight, viewFrustum, 0, false)) {
        backEnd.ctx->GetRenderCounter().numShadowMapDraw++;
    }
}

static void RB_ProjectedShadowMapPass(const VisLight *visLight, const Frustum &viewFrustum) {
    backEnd.shadowViewProjectionScaleBiasMatrix[0].SetZero();

    backEnd.shadowMapOffsetFactor = visLight->def->GetState().shadowOffsetFactor;
    backEnd.shadowMapOffsetUnits = visLight->def->GetState().shadowOffsetUnits;

    float dNear, dFar;
    if (!RB_ComputeNearFar(visLight->def->GetWorldFrustum(), visLight->shadowCastersAABB, viewFrustum, &dNear, &dFar)) {
        return;
    }

    float xFov = RAD2DEG(Math::ATan(visLight->def->GetWorldFrustum().GetLeft(), dFar)) * 2.0f;
    float yFov = RAD2DEG(Math::ATan(visLight->def->GetWorldFrustum().GetUp(), dFar)) * 2.0f;

    R_SetPerspectiveProjectionMatrix(xFov, yFov, dNear, dFar, false, backEnd.shadowProjectionMatrix);

    /*if (r_optimizedShadowProjection.GetInteger() > 0) {
        ALIGN_AS32 Mat4 shadowCropMatrix;
        Frustum lightFrustum = visLight->def->frustum;

        lightFrustum.MoveNearDistance(dNear);
        lightFrustum.MoveFarDistance(dFar);

        if (r_optimizedShadowProjection.GetInteger() == 2) {
            if (!RB_ComputeShadowCropMatrix(lightFrustum, OBB(visLight->shadowCastersAABB), viewFrustum, shadowCropMatrix)) {
                return;
            }
        } else {
            if (!RB_ComputeShadowCropMatrix(lightFrustum, viewFrustum, shadowCropMatrix)) {
                return false;
            }
        }

        // Multiply the crop matrix to create an effective 'zoomed in' shadow view-projection matrix.
        backEnd.shadowProjectionMatrix = shadowCropMatrix * backEnd.shadowProjectionMatrix;
    }*/

    backEnd.shadowMapFilterSize[0] = r_shadowMapFilterSize.GetFloat();

    static const Mat4 textureScaleBiasMatrix(Vec4(0.5, 0, 0, 0.5), Vec4(0, 0.5, 0, 0.5), Vec4(0, 0, 0.5, 0.5), Vec4(0.0, 0.0, 0.0, 1));
    backEnd.shadowViewProjectionScaleBiasMatrix[0] = textureScaleBiasMatrix * backEnd.shadowProjectionMatrix * visLight->def->GetViewMatrix();

    if (RB_ShadowMapPass(visLight, viewFrustum, 0, false)) {
        backEnd.ctx->GetRenderCounter().numShadowMapDraw++;
    }
}

static bool RB_SingleCascadedShadowMapPass(const VisLight *visLight, const Frustum &splitViewFrustum, int cascadeIndex, bool forceClear) {
    backEnd.shadowViewProjectionScaleBiasMatrix[cascadeIndex].SetZero();

    if (splitViewFrustum.CullAABB(visLight->litSurfsAABB)) {
        return false;
    }

    switch (cascadeIndex) {
    case 0:
        backEnd.shadowMapOffsetFactor = r_CSM_offsetFactor0.GetFloat();
        backEnd.shadowMapOffsetUnits = r_CSM_offsetUnits0.GetFloat();
        break;
    case 1:
        backEnd.shadowMapOffsetFactor = r_CSM_offsetFactor1.GetFloat();
        backEnd.shadowMapOffsetUnits = r_CSM_offsetUnits1.GetFloat();
        break;
    case 2:
        backEnd.shadowMapOffsetFactor = r_CSM_offsetFactor2.GetFloat();
        backEnd.shadowMapOffsetUnits = r_CSM_offsetUnits2.GetFloat();
        break;
    case 3:
    default:
        backEnd.shadowMapOffsetFactor = r_CSM_offsetFactor3.GetFloat();
        backEnd.shadowMapOffsetUnits = r_CSM_offsetUnits3.GetFloat();
        break;
    }

    float dNear, dFar;
    if (!RB_ComputeNearFar(visLight->def->GetState().origin, visLight->def->GetWorldOBB(), visLight->shadowCastersAABB, splitViewFrustum, &dNear, &dFar)) {
        return false;
    }

    R_SetOrthogonalProjectionMatrix(visLight->def->GetWorldOBB().Extents()[1], visLight->def->GetWorldOBB().Extents()[2], dNear, dFar, backEnd.shadowProjectionMatrix);

    if (r_optimizedShadowProjection.GetInteger() > 0) {
        ALIGN_AS32 Mat4 shadowCropMatrix;
        ALIGN_AS32 OBB lightOBB = visLight->def->GetWorldOBB();

        lightOBB.SetCenter(visLight->def->GetState().origin + visLight->def->GetState().axis[0] * (dFar + dNear) * 0.5f);

        ALIGN_AS32 Vec3 extents = lightOBB.Extents();
        lightOBB.SetExtents(Vec3((dFar - dNear) * 0.5f, extents.y, extents.z));

        if (r_optimizedShadowProjection.GetInteger() == 2) {
            if (!RB_ComputeShadowCropMatrix(lightOBB, OBB(visLight->shadowCastersAABB), splitViewFrustum, shadowCropMatrix)) {
                return false;
            }
        } else {
            Sphere viewSphere = splitViewFrustum.ToMinimumSphere();
            float viewSize = viewSphere.Radius() * 2;
            float texelsPerCenti = r_shadowMapSize.GetFloat() / UnitToCm(viewSize);
            backEnd.shadowMapFilterSize[cascadeIndex] = Max(r_shadowMapFilterSize.GetFloat() * texelsPerCenti, 1.0f);

            if (!RB_ComputeShadowCropMatrix(lightOBB, viewSphere, shadowCropMatrix)) {
                return false;
            }
        }

        // Multiply the crop matrix to create an effective 'zoomed in' shadow view-projection matrix.
        backEnd.shadowProjectionMatrix = shadowCropMatrix * backEnd.shadowProjectionMatrix;
    }

    static const Mat4 textureScaleBiasMatrix(Vec4(0.5, 0, 0, 0.5), Vec4(0, 0.5, 0, 0.5), Vec4(0, 0, 0.5, 0.5), Vec4(0.0, 0.0, 0.0, 1));
    backEnd.shadowViewProjectionScaleBiasMatrix[cascadeIndex] = textureScaleBiasMatrix * backEnd.shadowProjectionMatrix * visLight->def->GetViewMatrix();

    return RB_ShadowMapPass(visLight, splitViewFrustum, cascadeIndex, forceClear);
}

static void RB_CascadedShadowMapPass(const VisLight *visLight) {
    float dNear = backEnd.camera->def->GetFrustum().GetNearDistance();
    float dFar = r_CSM_maxDistance.GetFloat();
    int csmCount = r_CSM_count.GetInteger();

    R_ComputeSplitDistances(dNear, dFar, r_CSM_splitLambda.GetFloat(), csmCount, backEnd.csmDistances);

    if (r_CSM_selectionMethod.GetInteger() == 0) {
        // Z-based selection shader needs shadowSplitFar value.
        for (int cascadeIndex = 0; cascadeIndex < r_CSM_count.GetInteger(); cascadeIndex++) {
            float dFar = backEnd.csmDistances[cascadeIndex + 1];

            backEnd.csmFar[cascadeIndex] = (backEnd.projMatrix[2][2] * -dFar + backEnd.projMatrix[2][3]) / dFar;
            backEnd.csmFar[cascadeIndex] = backEnd.csmFar[cascadeIndex] * 0.5f + 0.5f;
        }
    }

    // Render shadow maps for each split view frustum.
    for (int cascadeIndex = 0; cascadeIndex < csmCount; cascadeIndex++) {
        if (backEnd.csmDistances[cascadeIndex + 1] <= r_CSM_nonCachedDistance.GetFloat()) {
            backEnd.csmUpdateRatio[cascadeIndex] = 1.0f;
        } else {
            backEnd.csmUpdateRatio[cascadeIndex] = r_CSM_updateRatio.GetFloat() * (1.0f - ((float)cascadeIndex / (float)csmCount));
            Clamp(backEnd.csmUpdateRatio[cascadeIndex], 0.1f, 1.0f);
        }

        backEnd.csmUpdate[cascadeIndex] += backEnd.csmUpdateRatio[cascadeIndex];
        if (backEnd.csmUpdate[cascadeIndex] < 1.0f) {
            continue;
        }
        backEnd.csmUpdate[cascadeIndex] -= 1.0f;

        Frustum splitViewFrustum = backEnd.camera->def->GetFrustum();
        float dNear = backEnd.csmDistances[cascadeIndex];
        float dFar = backEnd.csmDistances[cascadeIndex + 1];

        if (r_CSM_blend.GetBool() && cascadeIndex > 0) {
            // FIXME
            dNear -= (backEnd.csmDistances[cascadeIndex] - backEnd.csmDistances[cascadeIndex - 1]);
        }

        splitViewFrustum.MoveNearDistance(dNear);
        splitViewFrustum.MoveFarDistance(dFar);

        if (RB_SingleCascadedShadowMapPass(visLight, splitViewFrustum, cascadeIndex, true)) {
            backEnd.ctx->GetRenderCounter().numShadowMapDraw++;
        }
    }
}

void RB_ShadowPass(const VisLight *visLight) {
    if (visLight->def->GetState().type == RenderLight::Type::Point) {
        RB_ShadowCubeMapPass(visLight, backEnd.camera->def->GetFrustum());
    } else if (visLight->def->GetState().type == RenderLight::Type::Spot) {
        RB_ProjectedShadowMapPass(visLight, backEnd.camera->def->GetFrustum());
    } else if (visLight->def->GetState().type == RenderLight::Type::Directional) {
        if ((visLight->def->GetState().flags & RenderLight::Flag::PrimaryLight) && r_CSM_count.GetInteger() > 1) {
            RB_CascadedShadowMapPass(visLight);
        } else {
            RB_OrthogonalShadowMapPass(visLight, backEnd.camera->def->GetFrustum());
        }
    }
}

BE_NAMESPACE_END
