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

BE_NAMESPACE_BEGIN

// litVisAABB 와 viewFrustum 을 이용해서 light OBB 의 near, far 를 구한다
static bool RB_ComputeNearFar(const Vec3 &lightOrigin, const OBB &lightOBB, const AABB &litVisAABB, const Frustum &viewFrustum, float *dNear, float *dFar) {
    float dmin1, dmax1;
    float dmin2, dmax2;

    Vec3 lightDir = lightOBB.Axis()[0];
    float lightFar = lightOBB.Extents()[0] * 2.0f;

    litVisAABB.AxisProjection(lightDir, dmin1, dmax1);
    viewFrustum.AxisProjection(lightDir, dmin2, dmax2);

    *dNear = dmin1 - lightDir.Dot(lightOrigin);
    
    *dFar = Max(dmax1, dmax2) - lightDir.Dot(lightOrigin);
    *dFar = Min(*dFar, lightFar);

    if (*dFar <= 0.0f || *dNear >= *dFar) {
        return false;
    }

    return true;
}

// litVisAABB 와 viewFrustum 을 이용해서 light frustum 의 near 와 far 를 구한다
static bool RB_ComputeNearFar(const Frustum &lightFrustum, const AABB &litVisAABB, const Frustum &viewFrustum, float *dNear, float *dFar) {
    float dmin1, dmax1;
    float dmin2, dmax2;
    
    Vec3 lightDir = lightFrustum.GetAxis()[0];

    litVisAABB.AxisProjection(lightDir, dmin1, dmax1);
    viewFrustum.AxisProjection(lightDir, dmin2, dmax2);

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

    xmin = Math::Floor(xmin * size.x / alignSize);
    ymin = Math::Floor(ymin * size.y / alignSize);

    xmax = xmin + r_shadowMapSize.GetFloat();
    ymax = ymin + r_shadowMapSize.GetFloat();

    xmin = xmin * alignSize / size.x;
    ymin = ymin * alignSize / size.y;
    xmax = xmax * alignSize / size.x;
    ymax = ymax * alignSize / size.y;
    
    //xmax = Min(xmax, 1.0f);
    //ymax = Min(ymax, 1.0f);

    // [0, 1] to [-1, +1]
    xmin = xmin * 2.0f - 1.0f;
    ymin = ymin * 2.0f - 1.0f;
    xmax = xmax * 2.0f - 1.0f;
    ymax = ymax * 2.0f - 1.0f;
}

static bool RB_ComputeShadowCropMatrix(const OBB &lightOBB, const OBB &shadowCasterOBB, const Frustum &viewFrustum, Mat4 &shadowCropMatrix) {
    // crop bounds 를 만든다
    AABB casterCropBounds, viewCropBounds;
    lightOBB.ProjectionBounds(viewFrustum, viewCropBounds);
    lightOBB.ProjectionBounds(shadowCasterOBB, casterCropBounds);

    // 두개의 crop bounds 의 교집합
    AABB cropBounds;
    cropBounds[0][LeftAxis] = (casterCropBounds[0][LeftAxis] > viewCropBounds[0][LeftAxis]) ? casterCropBounds[0][LeftAxis] : viewCropBounds[0][LeftAxis];
    cropBounds[1][LeftAxis] = (casterCropBounds[1][LeftAxis] < viewCropBounds[1][LeftAxis]) ? casterCropBounds[1][LeftAxis] : viewCropBounds[1][LeftAxis];
    cropBounds[0][UpAxis] = (casterCropBounds[0][UpAxis] > viewCropBounds[0][UpAxis]) ? casterCropBounds[0][UpAxis] : viewCropBounds[0][UpAxis];		
    cropBounds[1][UpAxis] = (casterCropBounds[1][UpAxis] < viewCropBounds[1][UpAxis]) ? casterCropBounds[1][UpAxis] : viewCropBounds[1][UpAxis];

    if (cropBounds[0][LeftAxis] > cropBounds[1][LeftAxis] || cropBounds[0][UpAxis] > cropBounds[1][UpAxis]) {
        return false;
    }

    float xmin = -cropBounds[1][LeftAxis];
    float xmax = -cropBounds[0][LeftAxis];
    float ymin =  cropBounds[0][UpAxis];
    float ymax =  cropBounds[1][UpAxis];

    R_Set2DCropMatrix(xmin, xmax, ymin, ymax, shadowCropMatrix);

    return true;
}

static bool RB_ComputeShadowCropMatrix(const OBB &lightOBB, const Sphere &viewSphere, Mat4 &shadowCropMatrix) {
    // Calculate projection bounds [-1 ~ +1] of viewSphere in lightOBB space
    AABB cropBounds;
    lightOBB.ProjectionBounds(viewSphere, cropBounds);

    float xmin = -cropBounds[1][LeftAxis];
    float xmax = -cropBounds[0][LeftAxis];
    float ymin =  cropBounds[0][UpAxis];
    float ymax =  cropBounds[1][UpAxis];

    if (r_shadowMapCropAlign.GetBool()) {
        float lengthPerTexel = viewSphere.Radius() * 2.0f / r_shadowMapSize.GetFloat();

        const Vec3 &e = lightOBB.Extents();
        RB_AlignProjectionBounds(xmin, xmax, ymin, ymax, Vec2(e.y, e.z) * 2.0f, lengthPerTexel);
    }

    R_Set2DCropMatrix(xmin, xmax, ymin, ymax, shadowCropMatrix);
    
    return true;
}

static bool RB_ComputeShadowCropMatrix(const Frustum &lightFrustum, const OBB &shadowCasterOBB, const Frustum &viewFrustum, Mat4 &shadowCropMatrix) {
    // crop bounds 를 만든다
    AABB casterCropBounds, viewCropBounds;
    lightFrustum.ProjectionBounds(viewFrustum, viewCropBounds);
    lightFrustum.ProjectionBounds(shadowCasterOBB, casterCropBounds);		

    // 두개의 crop bounds 의 교집합
    AABB cropBounds;
    cropBounds[0][LeftAxis] = (casterCropBounds[0][LeftAxis] > viewCropBounds[0][LeftAxis]) ? casterCropBounds[0][LeftAxis] : viewCropBounds[0][LeftAxis];
    cropBounds[1][LeftAxis] = (casterCropBounds[1][LeftAxis] < viewCropBounds[1][LeftAxis]) ? casterCropBounds[1][LeftAxis] : viewCropBounds[1][LeftAxis];
    cropBounds[0][UpAxis] = (casterCropBounds[0][UpAxis] > viewCropBounds[0][UpAxis]) ? casterCropBounds[0][UpAxis] : viewCropBounds[0][UpAxis];		
    cropBounds[1][UpAxis] = (casterCropBounds[1][UpAxis] < viewCropBounds[1][UpAxis]) ? casterCropBounds[1][UpAxis] : viewCropBounds[1][UpAxis];

    if (cropBounds[0][1] > cropBounds[1][1] || cropBounds[0][2] > cropBounds[1][2]) {
        return false;
    }

    float xmin = -cropBounds[1][LeftAxis];
    float xmax = -cropBounds[0][LeftAxis];
    float ymin =  cropBounds[0][UpAxis];
    float ymax =  cropBounds[1][UpAxis];

    R_Set2DCropMatrix(xmin, xmax, ymin, ymax, shadowCropMatrix);
    //BE_LOG(L"%f %f, %f %f\n", xmin, xmax, ymin, ymax);

    return true;
}

static bool RB_ComputeShadowCropMatrix(const Frustum &lightFrustum, const Frustum &viewFrustum, Mat4 &shadowCropMatrix) {
    AABB cropBounds;
    lightFrustum.ProjectionBounds(viewFrustum, cropBounds);

    float xmin = -cropBounds[1][LeftAxis];
    float xmax = -cropBounds[0][LeftAxis];
    float ymin =  cropBounds[0][UpAxis];
    float ymax =  cropBounds[1][UpAxis];

    R_Set2DCropMatrix(xmin, xmax, ymin, ymax, shadowCropMatrix);
    //BE_LOG(L"%f %f, %f %f\n", xmin, xmax, ymin, ymax);

    return true;
}

static void RB_LitPass(const viewLight_t *viewLight, bool skipSelfShadow, bool skipNoSelfShadow) {
    int                 prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                prevDepthHack = false;

    for (drawSurfNode_t *litSurfNode = viewLight->litSurfs; litSurfNode; litSurfNode = litSurfNode->next) {
        const DrawSurf *surf = litSurfNode->drawSurf;

        if (!(surf->flags & DrawSurf::AmbientVisible)) {
            continue;
        }

        if (surf->sortKey != prevSortkey) {
            if (surf->material->GetSort() != Material::OpaqueSort && 
                surf->material->GetSort() != Material::AlphaTestSort && 
                surf->material->GetSort() != Material::AdditiveLightingSort) {
                continue;
            }

            if (surf->material != prevMaterial || surf->space != prevSpace) {
                if (prevMaterial) {
                    backEnd.rbsurf.Flush();
                }

                backEnd.rbsurf.Begin(RBSurf::LitFlush, surf->material, surf->materialRegisters, surf->space, viewLight);

                prevMaterial = surf->material;
            }

            if (surf->space != prevSpace) {
                prevSpace = surf->space;

                backEnd.modelViewMatrix = surf->space->modelViewMatrix;
                backEnd.modelViewProjMatrix = surf->space->modelViewProjMatrix;

                bool depthHack = surf->space->def->parms.depthHack;
                
                if (prevDepthHack != depthHack) {
                    if (depthHack) {
                        rhi.SetDepthRange(0.0f, 0.1f);
                    } else {
                        rhi.SetDepthRange(0.0f, 1.0f);
                    }

                    prevDepthHack = depthHack;
                }
            }

            prevSortkey = surf->sortKey;
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh);
    }

    if (prevMaterial) {
        backEnd.rbsurf.Flush();
    }	

    // restore depthHack
    if (prevDepthHack) {
        rhi.SetDepthRange(0.0f, 1.0f);
    }	
}

static bool RB_ShadowCubeMapFacePass(const viewLight_t *viewLight, const Mat4 &lightViewMatrix, const Frustum &lightFrustum, const Frustum &viewFrustum, bool forceClear, int cubeMapFace) {
    int                 prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const viewEntity_t *skipEntity = nullptr;
    const viewEntity_t *entity2 = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                firstDraw = true;
    
    for (drawSurfNode_t *shadowCasterSurfNode = viewLight->shadowCasterSurfs; shadowCasterSurfNode; shadowCasterSurfNode = shadowCasterSurfNode->next) {	
        const DrawSurf *surf = shadowCasterSurfNode->drawSurf;

        if (surf->sortKey != prevSortkey) {
            if (surf->space == skipEntity) {
                continue;
            }
            
            if (surf->material->GetFlags() & Material::NoShadow) {
                continue;
            }

            if (!(surf->material->GetCoverage() & (Material::OpaqueCoverage | Material::PerforatedCoverage)) && 
                !(surf->material->GetFlags() & Material::ForceShadow)) {
                continue;
            }

            if (surf->material != prevMaterial || surf->space != prevSpace) {
                if (!prevMaterial) {
                    backEnd.rbsurf.Begin(RBSurf::ShadowFlush, surf->material, surf->materialRegisters, surf->space, viewLight);
                } else {
                    if (surf->space != prevSpace ||
                        (prevMaterial->GetCoverage() & (Material::OpaqueCoverage | Material::PerforatedCoverage)) == Material::PerforatedCoverage || 
                        (surf->material->GetCoverage() & (Material::OpaqueCoverage | Material::PerforatedCoverage)) == Material::PerforatedCoverage) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::ShadowFlush, surf->material, surf->materialRegisters, surf->space, viewLight);
                    }
                }

                prevMaterial = surf->material;
            }

            if (surf->space != prevSpace) {
                prevSpace = surf->space;

                if (!surf->space->def->parms.castShadows) {
                    continue;
                }
                
                OBB obb(surf->space->def->GetAABB(), surf->space->def->parms.origin, surf->space->def->parms.axis);
                if (lightFrustum.CullOBB(obb)) {
                    skipEntity = surf->space;
                    continue;
                }

                skipEntity = nullptr;
            }

            prevSortkey = surf->sortKey;
        }
        
        if (!surf->space->def->parms.joints) {
            OBB obb(surf->subMesh->GetAABB() * surf->space->def->parms.scale, surf->space->def->parms.origin, surf->space->def->parms.axis);
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
                rhi.Clear(RHI::DepthBit, Color4::black, 1.0f, 0);
            } else {
                backEnd.ctx->vscmCleared[cubeMapFace] = false;
            }
        }

        if (surf->space != entity2) {
            entity2 = surf->space;

            backEnd.modelViewMatrix = lightViewMatrix * surf->space->def->GetModelMatrix();
            backEnd.modelViewProjMatrix = backEnd.projMatrix * backEnd.modelViewMatrix;
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh);
    }

    if (!firstDraw) {
        backEnd.rbsurf.Flush();

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
        rhi.Clear(RHI::DepthBit, Color4::black, 1.0f, 0);

        backEnd.ctx->vscmRT->End();

        backEnd.ctx->vscmCleared[cubeMapFace] = true;
    }

    return !firstDraw;
}

static int RB_ShadowCubeMapPass(const viewLight_t *viewLight, const Frustum &viewFrustum) {
    float zNear = r_shadowCubeMapZNear.GetFloat();
    float zFar = viewLight->def->GetMajorRadius();
    float zRangeInv = 1.0f / (zFar - zNear);

    // Zeye 에서 depth 값을 구하기 위한 projection 행렬의 33, 43 성분을 다시 W(-Zeye) 로 나눈값
    backEnd.shadowProjectionDepth[0] = -zFar * zNear * zRangeInv;
    backEnd.shadowProjectionDepth[1] = zFar * zRangeInv;

    float fov = RAD2DEG(backEnd.ctx->vscmBiasedFov);
    R_SetPerspectiveProjectionMatrix(fov, fov, zNear, zFar, false, backEnd.shadowProjectionMatrix);

    Frustum lightFrustum;
    lightFrustum.SetOrigin(viewLight->def->parms.origin);
    float size = zFar * Math::Tan(Math::OneFourthPi);
    lightFrustum.SetSize(zNear, zFar, size, size);

    int shadowMapDraw = 0;	

    Mat3 axis;

    Rect prevScissorRect = rhi.GetScissor();
    Mat4 prevProjMatrix = backEnd.projMatrix;
    backEnd.projMatrix = backEnd.shadowProjectionMatrix;

    for (int i = RHI::PositiveX; i <= RHI::NegativeZ; i++) {
        R_CubeMapFaceToAxis((RHI::CubeMapFace)i, axis);

        lightFrustum.SetAxis(axis);

        if (viewFrustum.CullFrustum(lightFrustum)) {
            continue;
        }

        Mat4 lightViewMatrix;
        R_SetViewMatrix(axis, viewLight->def->parms.origin, lightViewMatrix);

        backEnd.shadowMapOffsetFactor = viewLight->def->parms.shadowOffsetFactor;
        backEnd.shadowMapOffsetUnits = viewLight->def->parms.shadowOffsetUnits;

        rhi.SetDepthBias(backEnd.shadowMapOffsetFactor, backEnd.shadowMapOffsetUnits);

        if (RB_ShadowCubeMapFacePass(viewLight, lightViewMatrix, lightFrustum, viewFrustum, true, i)) {
            shadowMapDraw++;
        }

        rhi.SetDepthBias(0.0f, 0.0f);
    }

    backEnd.projMatrix = prevProjMatrix;

    rhi.SetScissor(prevScissorRect);
    rhi.SetViewport(backEnd.renderRect);

    return shadowMapDraw;
}

static void RB_ShadowCubeMapAndLitPass(const viewLight_t *viewLight) {
    Rect prevScissorRect;

    backEnd.ctx->renderCounter.numShadowMapDraw += RB_ShadowCubeMapPass(viewLight, backEnd.view->def->frustum);

    if (r_useLightScissors.GetBool()) {
        prevScissorRect = rhi.GetScissor();
        rhi.SetScissor(viewLight->scissorRect);
    }

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(backEnd.depthMin, backEnd.depthMax);
    }

    RB_LitPass(viewLight, false, false);

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(0.0f, 1.0f);
    }

    if (r_useLightScissors.GetBool()) {
        rhi.SetScissor(prevScissorRect);
    }
}

// TODO: cascade 별로 컬링해야함
static bool RB_ShadowMapPass(const viewLight_t *viewLight, const Frustum &viewFrustum, int cascadeIndex, bool forceClear) {
    int                 prevSortkey = -1;
    const viewEntity_t *prevSpace = nullptr;
    const viewEntity_t *skipEntity = nullptr;
    const viewEntity_t *entity2 = nullptr;
    const Material *    prevMaterial = nullptr;
    bool                firstDraw = true;
    Rect                prevScissorRect;
    Mat4                prevProjMatrix;

    if (r_CSM_pancaking.GetBool()) {
        rhi.SetDepthClamp(true);
    }

    rhi.SetDepthBias(backEnd.shadowMapOffsetFactor, backEnd.shadowMapOffsetUnits);
    
    for (drawSurfNode_t *shadowCasterSurfNode = viewLight->shadowCasterSurfs; shadowCasterSurfNode; shadowCasterSurfNode = shadowCasterSurfNode->next) {	
        const DrawSurf *surf = shadowCasterSurfNode->drawSurf;

        if (surf->sortKey != prevSortkey) {
            if (surf->space == skipEntity) {
                continue;
            }

            if (surf->material->GetFlags() & Material::NoShadow) {
                continue;
            }

            if (!(surf->material->GetCoverage() & (Material::OpaqueCoverage | Material::PerforatedCoverage)) && 
                !(surf->material->GetFlags() & Material::ForceShadow)) {
                continue;
            }

            if (surf->material != prevMaterial || surf->space != prevSpace) {
                if (!prevMaterial) {
                    backEnd.rbsurf.Begin(RBSurf::ShadowFlush, surf->material, surf->materialRegisters, surf->space, viewLight);
                } else {
                    if (surf->space != prevSpace ||
                        (prevMaterial->GetCoverage() & Material::PerforatedCoverage) || (surf->material->GetCoverage() & Material::PerforatedCoverage) ||
                        (surf->material->GetCullType() != prevMaterial->GetCullType())) {
                        backEnd.rbsurf.Flush();
                        backEnd.rbsurf.Begin(RBSurf::ShadowFlush, surf->material, surf->materialRegisters, surf->space, viewLight);
                    }
                }

                prevMaterial = surf->material;
            }

            if (surf->space != prevSpace) {
                prevSpace = surf->space;

                if (!surf->space->def->parms.castShadows) {
                    skipEntity = surf->space;
                    continue;
                }

                skipEntity = nullptr;
            }

            prevSortkey = surf->sortKey;
        }

        if (firstDraw) {
            firstDraw = false;

            backEnd.ctx->shadowMapRT->Begin(0, cascadeIndex);

            rhi.SetViewport(Rect(0, 0, backEnd.ctx->shadowMapRT->GetWidth(), backEnd.ctx->shadowMapRT->GetHeight()));
            
            prevScissorRect = rhi.GetScissor();
            rhi.SetScissor(Rect::empty);
            rhi.SetStateBits(RHI::DepthWrite);
            rhi.Clear(RHI::DepthBit, Color4::black, 1.0f, 0);

            prevProjMatrix = backEnd.projMatrix;
            backEnd.projMatrix = backEnd.shadowProjectionMatrix;
        }

        if (surf->space != entity2) {
            entity2 = surf->space;

            backEnd.modelViewMatrix = viewLight->def->viewMatrix * surf->space->def->GetModelMatrix();
            backEnd.modelViewProjMatrix = backEnd.projMatrix * backEnd.modelViewMatrix;
        }

        backEnd.rbsurf.DrawSubMesh(surf->subMesh);
    }

    if (!firstDraw) {
        backEnd.rbsurf.Flush();

        backEnd.ctx->shadowMapRT->End();

        backEnd.projMatrix = prevProjMatrix;
        rhi.SetScissor(prevScissorRect);
        rhi.SetViewport(backEnd.renderRect);
    } else if (forceClear) {
        firstDraw = false;

        backEnd.ctx->shadowMapRT->Begin(0, cascadeIndex);

        rhi.SetViewport(Rect(0, 0, backEnd.ctx->shadowMapRT->GetWidth(), backEnd.ctx->shadowMapRT->GetHeight()));
        prevScissorRect = rhi.GetScissor();
        rhi.SetScissor(Rect::empty);
        
        rhi.SetStateBits(RHI::DepthWrite);
        rhi.Clear(RHI::DepthBit, Color4::black, 1.0f, 0);

        backEnd.ctx->shadowMapRT->End();

        backEnd.projMatrix = prevProjMatrix;
        rhi.SetScissor(prevScissorRect);
        rhi.SetViewport(backEnd.renderRect);
    }

    rhi.SetDepthBias(0.0f, 0.0f);

    if (r_CSM_pancaking.GetBool()) {
        rhi.SetDepthClamp(false);
    }

    return !firstDraw;
}

static bool RB_OrthogonalShadowMapPass(const viewLight_t *viewLight, const Frustum &viewFrustum) {
    backEnd.shadowViewProjectionScaleBiasMatrix[0].SetZero();

    backEnd.shadowMapOffsetFactor = viewLight->def->parms.shadowOffsetFactor;
    backEnd.shadowMapOffsetUnits = viewLight->def->parms.shadowOffsetUnits;

    float dNear, dFar;
    if (!RB_ComputeNearFar(viewLight->def->parms.origin, viewLight->def->obb, viewLight->shadowCasterAABB, viewFrustum, &dNear, &dFar)) {
        return false;
    }

    R_SetOrthogonalProjectionMatrix(viewLight->def->obb.Extents()[1], viewLight->def->obb.Extents()[2], dNear, dFar, backEnd.shadowProjectionMatrix);

    if (r_optimizedShadowProjection.GetInteger() == 2) {
        Mat4 shadowCropMatrix;
        OBB lightOBB = viewLight->def->obb;

        lightOBB.SetCenter(viewLight->def->parms.origin + viewLight->def->parms.axis[0] * (dFar + dNear) * 0.5f);

        Vec3 extents = lightOBB.Extents();
        lightOBB.SetExtents(Vec3((dFar - dNear) * 0.5f, extents.y, extents.z));

        if (!RB_ComputeShadowCropMatrix(lightOBB, OBB(viewLight->shadowCasterAABB), viewFrustum, shadowCropMatrix)) {
            return false;
        }

        // crop matrix 를 곱해서 effective 'zoomed in' shadow view-projection matrix 를 만든다
        backEnd.shadowProjectionMatrix = shadowCropMatrix * backEnd.shadowProjectionMatrix;
    }

    backEnd.shadowMapFilterSize[0] = r_shadowMapFilterSize.GetFloat();

    static const Mat4 textureScaleBiasMatrix(Vec4(0.5, 0, 0, 0.5), Vec4(0, 0.5, 0, 0.5), Vec4(0, 0, 0.5, 0.5), Vec4(0.0, 0.0, 0.0, 1));
    backEnd.shadowViewProjectionScaleBiasMatrix[0] = textureScaleBiasMatrix * backEnd.shadowProjectionMatrix * viewLight->def->viewMatrix;

    return RB_ShadowMapPass(viewLight, viewFrustum, 0, false);
}

static void RB_OrthogonalShadowMapAndLitPass(const viewLight_t *viewLight) {
    if (RB_OrthogonalShadowMapPass(viewLight, backEnd.view->def->frustum)) {
        backEnd.ctx->renderCounter.numShadowMapDraw++;
    }

    Rect prevScissorRect;
    if (r_useLightScissors.GetBool()) {
        prevScissorRect = rhi.GetScissor();
        rhi.SetScissor(viewLight->scissorRect);
    }

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(backEnd.depthMin, backEnd.depthMax);
    }

    RB_LitPass(viewLight, false, false);

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(0.0f, 1.0f);
    }

    if (r_useLightScissors.GetBool()) {
        rhi.SetScissor(prevScissorRect);
    }
}

static bool RB_ProjectedShadowMapPass(const viewLight_t *viewLight, const Frustum &viewFrustum) {
    backEnd.shadowViewProjectionScaleBiasMatrix[0].SetZero();

    backEnd.shadowMapOffsetFactor = viewLight->def->parms.shadowOffsetFactor;
    backEnd.shadowMapOffsetUnits = viewLight->def->parms.shadowOffsetUnits;

    float dNear, dFar;
    if (!RB_ComputeNearFar(viewLight->def->frustum, viewLight->shadowCasterAABB, viewFrustum, &dNear, &dFar)) {
        return false;
    }

    float xFov = RAD2DEG(Math::ATan(viewLight->def->frustum.GetLeft(), dFar)) * 2.0f;
    float yFov = RAD2DEG(Math::ATan(viewLight->def->frustum.GetUp(), dFar)) * 2.0f;

    R_SetPerspectiveProjectionMatrix(xFov, yFov, dNear, dFar, false, backEnd.shadowProjectionMatrix);

    /*if (r_optimizedShadowProjection.GetInteger() > 0) {
        Mat4 shadowCropMatrix;
        Frustum lightFrustum = viewLight->def->frustum;

        lightFrustum.MoveNearDistance(dNear);
        lightFrustum.MoveFarDistance(dFar);

        if (r_optimizedShadowProjection.GetInteger() == 2) {
            if (!RB_ComputeShadowCropMatrix(lightFrustum, OBB(viewLight->shadowCasterAABB), viewFrustum, shadowCropMatrix)) {
                return false;
            }
        } else {
            RB_ComputeShadowCropMatrix(lightFrustum, viewFrustum, shadowCropMatrix);
        }

        // crop matrix 를 곱해서 effective 'zoomed in' shadow view-projection matrix 를 만든다
        backEnd.shadowProjectionMatrix = shadowCropMatrix * backEnd.shadowProjectionMatrix;
    }*/

    backEnd.shadowMapFilterSize[0] = r_shadowMapFilterSize.GetFloat();

    static const Mat4 textureScaleBiasMatrix(Vec4(0.5, 0, 0, 0.5), Vec4(0, 0.5, 0, 0.5), Vec4(0, 0, 0.5, 0.5), Vec4(0.0, 0.0, 0.0, 1));
    backEnd.shadowViewProjectionScaleBiasMatrix[0] = textureScaleBiasMatrix * backEnd.shadowProjectionMatrix * viewLight->def->viewMatrix;

    return RB_ShadowMapPass(viewLight, viewFrustum, 0, false);
}

static void RB_ProjectedShadowMapAndLitPass(const viewLight_t *viewLight) {
    if (RB_ProjectedShadowMapPass(viewLight, backEnd.view->def->frustum)) {
        backEnd.ctx->renderCounter.numShadowMapDraw++;
    }

    Rect prevScissorRect;
    if (r_useLightScissors.GetBool()) {
        prevScissorRect = rhi.GetScissor();
        rhi.SetScissor(viewLight->scissorRect);
    }

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(backEnd.depthMin, backEnd.depthMax);
    }

    RB_LitPass(viewLight, false, false);

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(0.0f, 1.0f);
    }

    if (r_useLightScissors.GetBool()) {
        rhi.SetScissor(prevScissorRect);
    }
}

static bool RB_CascadedShadowMapPass(const viewLight_t *viewLight, const Frustum &splitViewFrustum, int cascadeIndex, bool forceClear) {
    // split 된 viewFrustum 일 수 있기 때문에 컬링 가능
    if (splitViewFrustum.CullAABB(viewLight->litAABB)) {
        return false;
    }

    backEnd.shadowViewProjectionScaleBiasMatrix[cascadeIndex].SetZero();

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
    if (!RB_ComputeNearFar(viewLight->def->parms.origin, viewLight->def->obb, viewLight->shadowCasterAABB, splitViewFrustum, &dNear, &dFar)) {
        return false;
    }

    R_SetOrthogonalProjectionMatrix(viewLight->def->obb.Extents()[1], viewLight->def->obb.Extents()[2], dNear, dFar, backEnd.shadowProjectionMatrix);

    if (r_optimizedShadowProjection.GetInteger() > 0) {
        Mat4 shadowCropMatrix;
        OBB lightOBB = viewLight->def->obb;

        lightOBB.SetCenter(viewLight->def->parms.origin + viewLight->def->parms.axis[0] * (dFar + dNear) * 0.5f);

        Vec3 extents = lightOBB.Extents();
        lightOBB.SetExtents(Vec3((dFar - dNear) * 0.5f, extents.y, extents.z));

        if (r_optimizedShadowProjection.GetInteger() == 2) {
            if (!RB_ComputeShadowCropMatrix(lightOBB, OBB(viewLight->shadowCasterAABB), splitViewFrustum, shadowCropMatrix)) {
                return false;
            }
        } else {
            Sphere viewSphere = splitViewFrustum.ToMinimumSphere();
            float viewSize = viewSphere.Radius() * 2;
            float texelsPerCenti = r_shadowMapSize.GetFloat() / UnitToCenti(viewSize);
            backEnd.shadowMapFilterSize[cascadeIndex] = Max(r_shadowMapFilterSize.GetFloat() * texelsPerCenti, 1.0f);
                
            RB_ComputeShadowCropMatrix(lightOBB, viewSphere, shadowCropMatrix);
        }

        // crop matrix 를 곱해서 effective 'zoomed in' shadow view-projection matrix 를 만든다
        backEnd.shadowProjectionMatrix = shadowCropMatrix * backEnd.shadowProjectionMatrix;
    }

    static const Mat4 textureScaleBiasMatrix(Vec4(0.5, 0, 0, 0.5), Vec4(0, 0.5, 0, 0.5), Vec4(0, 0, 0.5, 0.5), Vec4(0.0, 0.0, 0.0, 1));
    backEnd.shadowViewProjectionScaleBiasMatrix[cascadeIndex] = textureScaleBiasMatrix * backEnd.shadowProjectionMatrix * viewLight->def->viewMatrix;

    return RB_ShadowMapPass(viewLight, splitViewFrustum, cascadeIndex, forceClear);
}

static void RB_CascadedShadowMapAndLitPass(const viewLight_t *viewLight) {
    // 각 split 뷰 프러스텀에 대하여 shadow map 생성
    for (int i = 0; i < backEnd.csmCount; i++) {
        backEnd.csmUpdate[i] += backEnd.csmUpdateRatio[i];
        if (backEnd.csmUpdate[i] < 1.0f) {
            continue;
        }
        backEnd.csmUpdate[i] -= 1.0f;
        
        Frustum splitViewFrustum = backEnd.view->def->frustum;
        float dNear = backEnd.csmDistances[i];
        float dFar = backEnd.csmDistances[i + 1];

        if (r_CSM_blend.GetBool() && i > 0) {
            // FIXME
            dNear -= (backEnd.csmDistances[i] - backEnd.csmDistances[i - 1]);
        }

        splitViewFrustum.MoveNearDistance(dNear);
        splitViewFrustum.MoveFarDistance(dFar);

        if (RB_CascadedShadowMapPass(viewLight, splitViewFrustum, i, true)) {
            backEnd.ctx->renderCounter.numShadowMapDraw++;
        }
    }

    Rect prevScissorRect;
    if (r_useLightScissors.GetBool()) {
        prevScissorRect = rhi.GetScissor();
        rhi.SetScissor(viewLight->scissorRect);
    }

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(backEnd.depthMin, backEnd.depthMax);
    }

    RB_LitPass(viewLight, false, false);

    if (r_useDepthBoundTest.GetBool()) {
        rhi.SetDepthBounds(0.0f, 1.0f);
    }

    if (r_useLightScissors.GetBool()) {
        rhi.SetScissor(prevScissorRect);
    }
}

static void RB_ShadowMapAndLitPass(const viewLight_t *viewLight) {
    if (viewLight->def->parms.type == SceneLight::PointLight) {
        RB_ShadowCubeMapAndLitPass(viewLight);
    } else if (viewLight->def->parms.type == SceneLight::SpotLight) {
        backEnd.csmCount = 1;

        RB_ProjectedShadowMapAndLitPass(viewLight);
    } else if (viewLight->def->parms.type == SceneLight::DirectionalLight) {
        float dNear = backEnd.view->def->frustum.GetNearDistance();
        float dFar = backEnd.view->def->frustum.GetFarDistance();

        backEnd.csmCount = r_CSM_count.GetInteger();

        if (viewLight->def->parms.isMainLight && backEnd.csmCount > 1) {
            dFar = r_CSM_maxDistance.GetFloat();
            R_ComputeSplitDistances(dNear, dFar, r_CSM_splitLamda.GetFloat(), backEnd.csmCount, backEnd.csmDistances);
            
            for (int i = 0; i < backEnd.csmCount; i++) {
                if (backEnd.csmDistances[i + 1] <= r_CSM_nonCachedDistance.GetFloat()) {
                    backEnd.csmUpdateRatio[i] = 1.0f;
                } else {
                    backEnd.csmUpdateRatio[i] = r_CSM_updateRatio.GetFloat() * (1.0f - ((float)i / (float)backEnd.csmCount));
                    Clamp(backEnd.csmUpdateRatio[i], 0.1f, 1.0f);
                }
            }

            RB_CascadedShadowMapAndLitPass(viewLight);
        } else {
            RB_OrthogonalShadowMapAndLitPass(viewLight);
        }
    }
}

void RB_AllShadowAndLitPass(viewLight_t *viewLights) {
    Rect prevScissorRect;

    backEnd.mainLight = nullptr;

    for (viewLight_t *viewLight = viewLights; viewLight; viewLight = viewLight->next) {
        const SceneLight *light = viewLight->def;

        if (light->parms.isMainLight && !backEnd.mainLight) {
            backEnd.mainLight = viewLight;

            AABB bounds;
            backEnd.view->def->frustum.AxisProjection(backEnd.mainLight->def->obb.Axis(), bounds);
            backEnd.mainLight->mainLightOBB = OBB(bounds, Vec3::origin, backEnd.mainLight->def->parms.axis);
        } else {
            if (r_useLightOcclusionQuery.GetBool() && !viewLight->occlusionVisible) {
                continue;
            }
        }

        const Material *lightMaterial = light->parms.material;

        /*float *outputValues = (float *)frameData.Alloc(lightMaterial->GetExprChunk()->NumRegisters() * sizeof(float));
        float localParms[MAX_EXPR_LOCALPARMS];
        localParms[0] = backEnd.ctx->elapsedTime;
        memcpy(&localParms[1], light->parms.materialParms, sizeof(light->parms.materialParms));
        lightMaterial->GetExprChunk()->Evaluate(localParms, outputValues);*/

        viewLight->materialRegisters = nullptr;//outputValues;

        const Material::Pass *lightPass = lightMaterial->GetPass();

        if (lightPass->useOwnerColor) {
            viewLight->lightColor = Color4(&light->parms.materialParms[SceneEntity::RedParm]);
        } else {
            viewLight->lightColor = Color4(lightPass->constantColor);
        }
        
        if (cvarSystem.GetCVarBool(L"gl_sRGB")) {
            // Linearize viewLight color
            viewLight->lightColor.ToColor3() = viewLight->lightColor.ToColor3().SRGBtoLinear();
        }

        if (r_useDepthBoundTest.GetBool()) {
            double lightDepthMin, lightDepthMax;
            if (!backEnd.view->def->GetDepthBoundsFromLight(light, &lightDepthMin, &lightDepthMax)) {
                continue;
            }

            double visSurfDepthMin, visSurfDepthMax;
            if (backEnd.view->def->GetDepthBoundsFromAABB(viewLight->litAABB, &visSurfDepthMin, &visSurfDepthMax)) {
                // FIXME:
                visSurfDepthMin = Max(visSurfDepthMin - 0.001, 0.0);
                visSurfDepthMax = Min(visSurfDepthMax + 0.001, 1.0);

                backEnd.depthMin = Max(lightDepthMin, visSurfDepthMin);
                backEnd.depthMax = Min(lightDepthMax, visSurfDepthMax);
            }

            //BE_LOG("%f %f\n", depthMin, depthMax);
        }

        // viewLight texture transform matrix
        const Mat4 &viewProjScaleBiasMat = viewLight->def->GetViewProjScaleBiasMatrix();

        float lightTexMat[2][4];
        lightTexMat[0][0] = lightPass->tcScale[0];
        lightTexMat[0][1] = 0.0f;
        lightTexMat[0][2] = 0.0f;
        lightTexMat[0][3] = lightPass->tcTranslation[0];

        lightTexMat[1][0] = 0.0f;
        lightTexMat[1][1] = lightPass->tcScale[1];
        lightTexMat[1][2] = 0.0f;
        lightTexMat[1][3] = lightPass->tcTranslation[1];

        viewLight->viewProjTexMatrix[0][0] = lightTexMat[0][0] * viewProjScaleBiasMat[0][0] + lightTexMat[0][1] * viewProjScaleBiasMat[1][0] + lightTexMat[0][3] * viewProjScaleBiasMat[3][0];
        viewLight->viewProjTexMatrix[0][1] = lightTexMat[0][0] * viewProjScaleBiasMat[0][1] + lightTexMat[0][1] * viewProjScaleBiasMat[1][1] + lightTexMat[0][3] * viewProjScaleBiasMat[3][1];
        viewLight->viewProjTexMatrix[0][2] = lightTexMat[0][0] * viewProjScaleBiasMat[0][2] + lightTexMat[0][1] * viewProjScaleBiasMat[1][2] + lightTexMat[0][3] * viewProjScaleBiasMat[3][2];
        viewLight->viewProjTexMatrix[0][3] = lightTexMat[0][0] * viewProjScaleBiasMat[0][3] + lightTexMat[0][1] * viewProjScaleBiasMat[1][3] + lightTexMat[0][3] * viewProjScaleBiasMat[3][3];

        viewLight->viewProjTexMatrix[1][0] = lightTexMat[1][0] * viewProjScaleBiasMat[0][0] + lightTexMat[1][1] * viewProjScaleBiasMat[1][0] + lightTexMat[1][3] * viewProjScaleBiasMat[3][0];
        viewLight->viewProjTexMatrix[1][1] = lightTexMat[1][0] * viewProjScaleBiasMat[0][1] + lightTexMat[1][1] * viewProjScaleBiasMat[1][1] + lightTexMat[1][3] * viewProjScaleBiasMat[3][1];
        viewLight->viewProjTexMatrix[1][2] = lightTexMat[1][0] * viewProjScaleBiasMat[0][2] + lightTexMat[1][1] * viewProjScaleBiasMat[1][2] + lightTexMat[1][3] * viewProjScaleBiasMat[3][2];
        viewLight->viewProjTexMatrix[1][3] = lightTexMat[1][0] * viewProjScaleBiasMat[0][3] + lightTexMat[1][1] * viewProjScaleBiasMat[1][3] + lightTexMat[1][3] * viewProjScaleBiasMat[3][3];

        viewLight->viewProjTexMatrix[2][0] = viewProjScaleBiasMat[2][0];
        viewLight->viewProjTexMatrix[2][1] = viewProjScaleBiasMat[2][1];
        viewLight->viewProjTexMatrix[2][2] = viewProjScaleBiasMat[2][2];
        viewLight->viewProjTexMatrix[2][3] = viewProjScaleBiasMat[2][3];

        viewLight->viewProjTexMatrix[3][0] = viewProjScaleBiasMat[3][0];
        viewLight->viewProjTexMatrix[3][1] = viewProjScaleBiasMat[3][1];
        viewLight->viewProjTexMatrix[3][2] = viewProjScaleBiasMat[3][2];
        viewLight->viewProjTexMatrix[3][3] = viewProjScaleBiasMat[3][3];

        if (viewLight->def->parms.castShadows && r_shadows.GetInteger() != 0 && !(backEnd.view->def->parms.flags & SceneView::NoShadows)) {
            RB_ShadowMapAndLitPass(viewLight);
        } else {
            if (r_useLightScissors.GetBool()) {
                prevScissorRect = rhi.GetScissor();
                rhi.SetScissor(viewLight->scissorRect);
            }

            if (r_useDepthBoundTest.GetBool()) {
                rhi.SetDepthBounds(backEnd.depthMin, backEnd.depthMax);
            }

            RB_LitPass(viewLight, false, false);

            if (r_useDepthBoundTest.GetBool()) {
                rhi.SetDepthBounds(0.0f, 1.0f);
            }

            if (r_useLightScissors.GetBool()) {
                rhi.SetScissor(prevScissorRect);
            }
        }
    }
}

BE_NAMESPACE_END
