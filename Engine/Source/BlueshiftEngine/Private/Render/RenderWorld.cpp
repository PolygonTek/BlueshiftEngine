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
#include "Core/Heap.h"
#include "Platform/PlatformTime.h"

BE_NAMESPACE_BEGIN

static const int MaxViewDrawSurfs = 0x4000;

RenderWorld::RenderWorld() {
    viewCount = 0;

    textMesh.SetCoordFrame(GuiMesh::CoordFrame3D);

    skyboxMaterial = materialManager.defaultSkyboxMaterial;

    debugLineColor.Set(0, 0, 0, 0);
    debugFillColor.Set(0, 0, 0, 0);
}

RenderWorld::~RenderWorld() {
    ClearScene();
}

void RenderWorld::ClearScene() {
    entityDbvt.Purge();
    lightDbvt.Purge();
    staticMeshDbvt.Purge();

    for (int i = 0; i < sceneEntities.Count(); i++) {
        SAFE_DELETE(sceneEntities[i]);
    }
    for (int i = 0; i < sceneLights.Count(); i++) {
        SAFE_DELETE(sceneLights[i]);
    }
}

const SceneEntity *RenderWorld::GetEntity(int entityHandle) const {
    if (entityHandle < 0 || entityHandle >= sceneEntities.Count()) {
        BE_WARNLOG(L"RenderWorld::GetEntity: handle %i > %i\n", entityHandle, sceneEntities.Count() - 1);
        return nullptr;
    }

    SceneEntity *sceneEntity = sceneEntities[entityHandle];
    if (!sceneEntity) {
        BE_WARNLOG(L"RenderWorld::GetEntity: handle %i is nullptr\n", entityHandle);
        return nullptr;
    }

    return sceneEntity;
}

int RenderWorld::AddEntity(const SceneEntity::Parms *parms) {
    int entityHandle = sceneEntities.FindNull();
    if (entityHandle == -1) {
        entityHandle = sceneEntities.Append(nullptr);
    }

    UpdateEntity(entityHandle, parms);

    return entityHandle;
}

void RenderWorld::UpdateEntity(int entityHandle, const SceneEntity::Parms *parms) {
    while (entityHandle >= sceneEntities.Count()) {
        sceneEntities.Append(nullptr);
    }

    SceneEntity *sceneEntity = sceneEntities[entityHandle];
    if (!sceneEntity) {
        sceneEntity = new SceneEntity;
        sceneEntities[entityHandle] = sceneEntity;

        sceneEntity->index = entityHandle;
        sceneEntity->Update(parms);

        // Add proxy in the DBVT for the sceneEntities
        sceneEntity->proxy = (DbvtProxy *)Mem_ClearedAlloc(sizeof(DbvtProxy));
        sceneEntity->proxy->sceneEntity = sceneEntity;
        sceneEntity->proxy->aabb.SetFromTransformedAABB(parms->aabb * parms->scale, parms->origin, parms->axis);
        sceneEntity->proxy->id = entityDbvt.CreateProxy(sceneEntity->proxy->aabb, MeterToUnit(0.5f), sceneEntity->proxy);

        // If this entity is a static mesh, add proxy for each sub meshes in the DBVT for the static meshes
        if (parms->mesh && !parms->joints) {
            sceneEntity->numMeshSurfProxies = parms->mesh->NumSurfaces();
            sceneEntity->meshSurfProxies = (DbvtProxy *)Mem_ClearedAlloc(parms->mesh->NumSurfaces() * sizeof(DbvtProxy));

            for (int surfaceIndex = 0; surfaceIndex < parms->mesh->NumSurfaces(); surfaceIndex++) {
                const MeshSurf *meshSurf = parms->mesh->GetSurface(surfaceIndex);

                DbvtProxy *meshSurfProxy = &sceneEntity->meshSurfProxies[surfaceIndex];
                meshSurfProxy->sceneEntity = sceneEntity;
                meshSurfProxy->mesh = parms->mesh;
                meshSurfProxy->meshSurfIndex = surfaceIndex;
                meshSurfProxy->aabb.SetFromTransformedAABB(meshSurf->subMesh->GetAABB() * parms->scale, parms->origin, parms->axis);
                meshSurfProxy->id = staticMeshDbvt.CreateProxy(sceneEntity->meshSurfProxies[surfaceIndex].aabb, MeterToUnit(0.0f), &sceneEntity->meshSurfProxies[surfaceIndex]);
            }
        }
    } else {
        bool originMatch    = (parms->origin == sceneEntity->parms.origin);
        bool axisMatch      = (parms->axis == sceneEntity->parms.axis);
        bool scaleMatch     = (parms->scale == sceneEntity->parms.scale);
        bool aabbMatch      = (parms->aabb == sceneEntity->parms.aabb);
        bool meshMatch      = (parms->mesh == sceneEntity->parms.mesh);
        bool proxyMoved     = !originMatch || !axisMatch || !scaleMatch || !aabbMatch;

        if (proxyMoved || !meshMatch) {
            if (proxyMoved) {
                sceneEntity->proxy->aabb.SetFromTransformedAABB(parms->aabb * parms->scale, parms->origin, parms->axis);
                entityDbvt.MoveProxy(sceneEntity->proxy->id, sceneEntity->proxy->aabb, MeterToUnit(0.5f), parms->origin - sceneEntity->parms.origin);
            }

            // If this entity is a static mesh
            if (sceneEntity->parms.mesh && !sceneEntity->parms.joints) {
                // mesh surface count changed so we recreate static proxy
                if (parms->mesh->NumSurfaces() != sceneEntity->numMeshSurfProxies) {
                    Mem_Free(sceneEntity->meshSurfProxies);

                    sceneEntity->numMeshSurfProxies = parms->mesh->NumSurfaces();
                    sceneEntity->meshSurfProxies = (DbvtProxy *)Mem_ClearedAlloc(sceneEntity->numMeshSurfProxies * sizeof(DbvtProxy));

                    for (int surfaceIndex = 0; surfaceIndex < parms->mesh->NumSurfaces(); surfaceIndex++) {
                        const MeshSurf *meshSurf = parms->mesh->GetSurface(surfaceIndex);

                        staticMeshDbvt.DestroyProxy(sceneEntity->meshSurfProxies[surfaceIndex].id);

                        DbvtProxy *meshSurfProxy = &sceneEntity->meshSurfProxies[surfaceIndex];
                        meshSurfProxy->sceneEntity = sceneEntity;
                        meshSurfProxy->mesh = parms->mesh;
                        meshSurfProxy->meshSurfIndex = surfaceIndex;
                        meshSurfProxy->aabb.SetFromTransformedAABB(meshSurf->subMesh->GetAABB() * parms->scale, parms->origin, parms->axis);
                        meshSurfProxy->id = staticMeshDbvt.CreateProxy(sceneEntity->meshSurfProxies[surfaceIndex].aabb, MeterToUnit(0.0f), &sceneEntity->meshSurfProxies[surfaceIndex]);
                    }
                } else {
                    for (int surfaceIndex = 0; surfaceIndex < parms->mesh->NumSurfaces(); surfaceIndex++) {
                        sceneEntity->meshSurfProxies[surfaceIndex].aabb.SetFromTransformedAABB(parms->mesh->GetSurface(surfaceIndex)->subMesh->GetAABB() * parms->scale, parms->origin, parms->axis);
                        staticMeshDbvt.MoveProxy(sceneEntity->meshSurfProxies[surfaceIndex].id, sceneEntity->meshSurfProxies[surfaceIndex].aabb, MeterToUnit(0.5f), parms->origin - sceneEntity->parms.origin);
                    }
                }
            }
        }

        sceneEntity->Update(parms);
    }
}

void RenderWorld::RemoveEntity(int entityHandle) {
    if (entityHandle < 0 || entityHandle >= sceneEntities.Count()) {
        BE_WARNLOG(L"RenderWorld::RemoveEntity: handle %i > %i\n", entityHandle, sceneEntities.Count() - 1);
        return;
    }

    SceneEntity *sceneEntity = sceneEntities[entityHandle];
    if (!sceneEntity) {
        BE_WARNLOG(L"RenderWorld::RemoveEntity: handle %i is nullptr\n", entityHandle);
        return;
    }

    entityDbvt.DestroyProxy(sceneEntity->proxy->id);
    for (int i = 0; i < sceneEntity->numMeshSurfProxies; i++) {
        staticMeshDbvt.DestroyProxy(sceneEntity->meshSurfProxies[i].id);
    }

    delete sceneEntities[entityHandle];
    sceneEntities[entityHandle] = nullptr;
}

const SceneLight *RenderWorld::GetLight(int lightHandle) const {
    if (lightHandle < 0 || lightHandle >= sceneLights.Count()) {
        BE_WARNLOG(L"RenderWorld::GetLight: handle %i > %i\n", lightHandle, sceneLights.Count() - 1);
        return nullptr;
    }

    SceneLight *sceneLight = sceneLights[lightHandle];
    if (!sceneLight) {
        BE_WARNLOG(L"RenderWorld::GetLight: handle %i is nullptr\n", lightHandle);
        return nullptr;
    }

    return sceneLight;
}

int RenderWorld::AddLight(const SceneLight::Parms *parms) {
    int lightHandle = sceneLights.FindNull();
    if (lightHandle == -1) {
        lightHandle = sceneLights.Append(nullptr);
    }

    UpdateLight(lightHandle, parms);

    return lightHandle;
}

void RenderWorld::UpdateLight(int lightHandle, const SceneLight::Parms *parms) {
    while (lightHandle >= sceneLights.Count()) {
        sceneLights.Append(nullptr);
    }

    SceneLight *sceneLight = sceneLights[lightHandle];
    if (!sceneLight) {
        sceneLight = new SceneLight;
        sceneLights[lightHandle] = sceneLight;

        sceneLight->index = lightHandle;
        sceneLight->Update(parms);

        sceneLight->proxy = (DbvtProxy *)Mem_ClearedAlloc(sizeof(DbvtProxy));
        sceneLight->proxy->sceneLight = sceneLight;
        sceneLight->proxy->aabb = sceneLight->GetAABB();
        sceneLight->proxy->id = lightDbvt.CreateProxy(sceneLight->proxy->aabb, MeterToUnit(0.0f), sceneLight->proxy);
    } else {
        bool originMatch    = (parms->origin == sceneLight->parms.origin);
        bool axisMatch      = (parms->axis == sceneLight->parms.axis);
        bool valueMatch     = (parms->value == sceneLight->parms.value);

        if (!originMatch || !axisMatch || !valueMatch) {
            sceneLight->proxy->aabb = sceneLight->proxy->sceneLight->GetAABB();
            lightDbvt.MoveProxy(sceneLight->proxy->id, sceneLight->proxy->aabb, MeterToUnit(0.5f), parms->origin - sceneLight->parms.origin);
        }

        sceneLight->Update(parms);
    }
}

void RenderWorld::RemoveLight(int lightHandle) {
    if (lightHandle < 0 || lightHandle >= sceneLights.Count()) {
        BE_WARNLOG(L"RenderWorld::RemoveLight: handle %i > %i\n", lightHandle, sceneLights.Count() - 1);
        return;
    }

    SceneLight *sceneLight = sceneLights[lightHandle];
    if (!sceneLight) {
        BE_WARNLOG(L"RenderWorld::RemoveLight: handle %i is nullptr\n", lightHandle);
        return;
    }

    lightDbvt.DestroyProxy(sceneLight->proxy->id);

    delete sceneLights[lightHandle];
    sceneLights[lightHandle] = nullptr;
}

void RenderWorld::SetSkyboxMaterial(Material *skyboxMaterial) {
    this->skyboxMaterial = skyboxMaterial;
}

void RenderWorld::FinishMapLoading() {
//#ifndef _DEBUG
//    int startTime = PlatformTime::Milliseconds();
//
//    entityDbvt.RebuildBottomUp();
//    staticMeshDbvt.RebuildBottomUp();
//    lightDbvt.RebuildBottomUp();
//
//    int elapsedTime = PlatformTime::Milliseconds() - startTime;
//    BE_LOG(L"%i msec to build dynamic AABB tree\n", elapsedTime);
//#endif
}

void RenderWorld::RenderScene(const SceneView *view) {
    if (view->parms.renderRect.w <= 0.0f || view->parms.renderRect.h <= 0.0f) {
        return;
    }

    if (view->parms.layerMask == 0) {
        return;
    }

    // Set current render view
    currentView = (view_t *)frameData.ClearedAlloc(sizeof(*currentView));
    currentView->def = view;
    currentView->maxDrawSurfs = MaxViewDrawSurfs;
    currentView->numDrawSurfs = 0;
    currentView->drawSurfs = (DrawSurf **)frameData.Alloc(currentView->maxDrawSurfs * sizeof(DrawSurf *));

    RenderView(currentView);
}

void RenderWorld::EmitGuiFullScreen(GuiMesh &guiMesh) {
    if (guiMesh.NumSurfaces() == 0) {
        return;
    }

    viewCount++;

    // GUI view def
    SceneView::Parms viewParms;
    viewParms.renderRect    = Rect(0, 0, renderSystem.currentContext->GetDeviceWidth(), renderSystem.currentContext->GetDeviceHeight());
    viewParms.time          = PlatformTime::Milliseconds();
    viewParms.orthogonal    = true;
    viewParms.zNear         = 0.0f;
    viewParms.zFar          = 1.0f;
    viewParms.sizeX         = renderSystem.currentContext->GetDeviceWidth() * 0.5f;
    viewParms.sizeY         = renderSystem.currentContext->GetDeviceHeight() * 0.5f;
    viewParms.axis          = Mat3::identity;
    viewParms.origin        = Vec3::origin;

    static SceneView sceneView;
    new (&sceneView) SceneView();
    sceneView.Update(&viewParms);

    // GUI entity def
    SceneEntity::Parms entityParms;
    memset(&entityParms, 0, sizeof(entityParms));
    entityParms.scale       = Vec3::one;
    entityParms.axis        = Mat3::identity;
    entityParms.materialParms[SceneEntity::RedParm] = 1.0f;
    entityParms.materialParms[SceneEntity::GreenParm] = 1.0f;
    entityParms.materialParms[SceneEntity::BlueParm] = 1.0f;
    entityParms.materialParms[SceneEntity::AlphaParm] = 1.0f;
    entityParms.materialParms[SceneEntity::TimeScaleParm] = 1.0f;
    
    static SceneEntity sceneEntity;
    new (&sceneEntity) SceneEntity();
    sceneEntity.Update(&entityParms);

    // GUI view
    view_t *guiView         = (view_t *)frameData.ClearedAlloc(sizeof(*guiView));
    guiView->def            = &sceneView;
    guiView->is2D           = true;
    guiView->maxDrawSurfs   = guiMesh.NumSurfaces();
    guiView->drawSurfs      = (DrawSurf **)frameData.Alloc(guiView->maxDrawSurfs * sizeof(DrawSurf *));

    // GUI view entity
    Mat4 projMatrix;
    projMatrix.SetOrtho(0, renderSystem.currentContext->GetDeviceWidth(), renderSystem.currentContext->GetDeviceHeight(), 0, -1.0, 1.0);

    viewEntity_t *viewEntity = RegisterViewEntity(guiView, &sceneEntity);
    viewEntity->modelViewMatrix.SetIdentity();
    viewEntity->modelViewMatrix.Scale(renderSystem.currentContext->GetUpscaleFactorX(), renderSystem.currentContext->GetUpscaleFactorY(), 1.0f);
    viewEntity->modelViewProjMatrix = projMatrix * viewEntity->modelViewMatrix;

    guiMesh.CacheIndexes();
    
    for (int surfaceIndex = 0; surfaceIndex < guiMesh.NumSurfaces(); surfaceIndex++) {
        const GuiMeshSurf *guiSurf = guiMesh.Surface(surfaceIndex);
        if (!guiSurf->numIndexes) {
            break;
        }

        SubMesh *subMesh        = (SubMesh *)frameData.ClearedAlloc(sizeof(SubMesh));
        subMesh->alloced        = false;
        subMesh->type           = Mesh::DynamicMesh;
        subMesh->numIndexes     = guiSurf->numIndexes;
        subMesh->numVerts       = guiSurf->numVerts;
#if 1
        subMesh->vertexCache    = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
        *(subMesh->vertexCache) = guiSurf->vertexCache;

        subMesh->indexCache     = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
        *(subMesh->indexCache)  = guiSurf->indexCache;
#else
        subMesh->vertexCache    = &guiSurf->vertexCache;
        subMesh->indexCache     = &guiSurf->indexCache;
#endif
        AddDrawSurf(guiView, viewEntity, guiSurf->material, subMesh, 0);
    }

    renderSystem.CmdDrawView(guiView);
}

void RenderWorld::ClearDebugPrimitives(int time) {
    RB_ClearDebugPrimitives(time);
}

void RenderWorld::DebugLine(const Vec3 &start, const Vec3 &end, float lineWidth, bool depthTest, int lifeTime) {
    if (lineWidth > 0 && debugLineColor[3] > 0) {
        Vec3 *v = RB_ReserveDebugPrimsVerts(RHI::LinesPrim, 2, debugLineColor, lineWidth, false, depthTest, lifeTime);
        if (v) {
            v[0] = start;
            v[1] = end;
        }
    }
}

void RenderWorld::DebugTriangle(const Vec3 &a, const Vec3 &b, const Vec3 &c, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    if (debugFillColor[3] > 0) {
        Vec3 *v = RB_ReserveDebugPrimsVerts(RHI::TrianglesPrim, 3, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (v) {
            v[0] = a;
            v[1] = b;
            v[2] = c;
        }
    }

    if (lineWidth > 0 && debugLineColor[3] > 0) {
        DebugLine(a, b, lineWidth, depthTest, lifeTime);
        DebugLine(b, c, lineWidth, depthTest, lifeTime);
        DebugLine(c, a, lineWidth, depthTest, lifeTime);
    }
}

void RenderWorld::DebugQuad(const Vec3 &origin, const Vec3 &right, const Vec3 &up, float size, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 sr = right * size;
    Vec3 su = up * size;

    Vec3 v[4];
    v[0] = origin + sr - su;
    v[1] = origin - sr - su;
    v[2] = origin - sr + su;
    v[3] = origin + sr + su;

    if (debugFillColor[3] > 0) {
        Vec3 *fv = RB_ReserveDebugPrimsVerts(RHI::TriangleFanPrim, 4, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fv) {
            fv[0] = v[0];
            fv[1] = v[1];
            fv[2] = v[2];
            fv[3] = v[3];
        }
    }

    if (lineWidth > 0 && debugLineColor[3] > 0) {
        DebugLine(v[0], v[1], lineWidth, depthTest, lifeTime);
        DebugLine(v[1], v[2], lineWidth, depthTest, lifeTime);
        DebugLine(v[2], v[3], lineWidth, depthTest, lifeTime);
        DebugLine(v[3], v[0], lineWidth, depthTest, lifeTime);
    }
}

void RenderWorld::DebugCircle(const Vec3 &origin, const Vec3 &dir, const float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    const int numSteps = 48;

    Vec3 left, up;
    dir.OrthogonalBasis(left, up);
    left *= radius;
    up *= radius;

    if (debugFillColor[3] > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleFanPrim, numSteps + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = origin;

            for (int i = 0; i <= numSteps; i++) {
                float a = Math::TwoPi * i / numSteps;
                *fvptr++ = origin + Math::Cos16(a) * left + Math::Sin16(a) * up;
            }
        }
    }

    if (lineWidth > 0 && debugLineColor[3] > 0) {
        Vec3 lastPoint = origin + left;
        Vec3 point;

        for (int i = 1; i <= numSteps; i++) {
            float a = Math::TwoPi * i / numSteps;
            point = origin + Math::Cos16(a) * left + Math::Sin16(a) * up;
            DebugLine(lastPoint, point, lineWidth, depthTest, lifeTime);
            lastPoint = point;
        }
    }
}

void RenderWorld::DebugArc(const Vec3 &origin, const Vec3 &right, const Vec3 &up, const float radius, float angle1, float angle2, bool drawSector, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {	
    float deltaAngle = angle2 - angle1;
    if (deltaAngle == 0.0f) {
        return;
    }

    int numSteps = Min((int)Math::Ceil((Math::Fabs(deltaAngle / 7.5f))), 96);
    float theta1 = DEG2RAD(angle1);
    float theta2 = DEG2RAD(angle2);
    float delta = theta2 - theta1;

    Vec3 rx = radius * right;
    Vec3 ry = radius * up;

    if (drawSector && debugFillColor[3] > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleFanPrim, numSteps + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = origin;

            for (int i = 0; i <= numSteps; i++) {
                float a = theta1 + delta * i / numSteps;
                *fvptr++ = origin + Math::Cos16(a) * rx + Math::Sin16(a) * ry;
            }
        }
    }

    if (lineWidth > 0 && debugLineColor[3] > 0) {
        Vec3 lastPoint = origin + Math::Cos16(theta1) * rx + Math::Sin16(theta1) * ry;
        Vec3 point;

        if (drawSector) {
            DebugLine(origin, lastPoint, lineWidth, depthTest, lifeTime);
        }

        for (int i = 1; i <= numSteps; i++) {
            float a = theta1 + delta * i / numSteps;
            point = origin + Math::Cos16(a) * rx + Math::Sin16(a) * ry;
            DebugLine(lastPoint, point, lineWidth, depthTest, lifeTime);
            lastPoint = point;
        }

        if (drawSector) {
            DebugLine(lastPoint, origin, lineWidth, depthTest, lifeTime);
        }
    }
}

void RenderWorld::DebugEllipse(const Vec3 &origin, const Vec3 &right, const Vec3 &up, const float radius1, const float radius2, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    const int numSteps = 64;

    Vec3 rx = right * radius1;
    Vec3 ry = up * radius2;

    if (debugFillColor[3] > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleFanPrim, numSteps + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = origin;

            for (int i = 0; i <= numSteps; i++) {
                float a = Math::TwoPi * i / numSteps;
                *fvptr++ = origin + Math::Cos16(a) * rx + Math::Sin16(a) * ry;
            }
        }
    }

    if (lineWidth > 0 && debugLineColor[3] > 0) {
        Vec3 lastPoint = origin + rx;
        Vec3 point;

        for (int i = 1; i <= numSteps; i++) {
            float a = Math::TwoPi * i / numSteps;
            point = origin + Math::Cos16(a) * rx + Math::Sin16(a) * ry;
            DebugLine(lastPoint, point, lineWidth, depthTest, lifeTime);
            lastPoint = point;
        }
    }
}

void RenderWorld::DebugHemisphere(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 raxis, p, lastp, last0;
    int num = 360 / 15;

    Vec3 *lastArray = (Vec3 *)_alloca16(num * sizeof(Vec3));

    if (debugFillColor[3] > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleStripPrim, (num + 1) * 2 * (num / 4), debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            lastArray[0] = origin + axis[2] * radius;
            for (int n = 1; n < num; n++) {
                lastArray[n] = lastArray[0];
            }

            for (int i = 15; i <= 90; i += 15) {
                float cr = Math::Cos16(DEG2RAD(i)) * radius;
                float sr = Math::Sin16(DEG2RAD(i)) * radius;

                last0 = lastArray[0];

                int j = 15;
                for (int n = 0; j <= 360; j += 15, n++) {
                    raxis = axis[0] * Math::Cos16(DEG2RAD(j)) + axis[1] * Math::Sin16(DEG2RAD(j));
                    p = origin + cr * axis[2] + sr * raxis;

                    *fvptr++ = lastArray[n];
                    *fvptr++ = p;

                    lastArray[n] = p;
                }

                *fvptr++ = last0;
                *fvptr++ = lastArray[0];
            }
        }
    }

    if (lineWidth > 0 && debugLineColor[3] > 0) {
        lastArray[0] = origin + axis[2] * radius;
        for (int n = 1; n < num; n++) {
            lastArray[n] = lastArray[0];
        }

        for (int i = 15; i <= 90; i += 15) {
            float cr = Math::Cos16(DEG2RAD(i)) * radius;
            float sr = Math::Sin16(DEG2RAD(i)) * radius;

            lastp = origin + cr * axis[2] + sr * axis[0];

            int j = 15;
            for (int n = 0; j <= 360; j += 15, n++) {
                raxis = axis[0] * Math::Cos16(DEG2RAD(j)) + axis[1] * Math::Sin16(DEG2RAD(j));
                p = origin + cr * axis[2] + sr * raxis;

                DebugLine(lastp, p, lineWidth, depthTest, lifeTime);
                DebugLine(lastp, lastArray[n], lineWidth, depthTest, lifeTime);

                lastArray[n] = lastp;
                lastp = p;
            }
        }
    }
}

void RenderWorld::DebugHemisphereSimple(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth, bool depthTest, int lifeTime) {
    DebugCircle(origin, axis[2], radius, lineWidth, false, depthTest, lifeTime);
    DebugArc(origin, axis[0], axis[2], radius, 0, 180, false, lineWidth, false, depthTest, lifeTime);
    DebugArc(origin, axis[1], axis[2], radius, 0, 180, false, lineWidth, false, depthTest, lifeTime);
}

void RenderWorld::DebugSphere(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 raxis, p, lastp, last0;
    int num = 360 / 15;

    Vec3 *lastArray = (Vec3 *)_alloca16(num * sizeof(Vec3));

    if (debugFillColor[3] > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleStripPrim, (num + 1) * 2 * (num / 2), debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            lastArray[0] = origin + axis[2] * radius;
            for (int n = 1; n < num; n++) {
                lastArray[n] = lastArray[0];
            }

            for (int i = 15; i <= 180; i += 15) {
                float cr = Math::Cos16(DEG2RAD(i)) * radius;
                float sr = Math::Sin16(DEG2RAD(i)) * radius;

                last0 = lastArray[0];

                int j = 15;
                for (int n = 0; j <= 360; j += 15, n++) {
                    raxis = axis[0] * Math::Cos16(DEG2RAD(j)) + axis[1] * Math::Sin16(DEG2RAD(j));
                    p = origin + cr * axis[2] + sr * raxis;

                    *fvptr++ = lastArray[n];
                    *fvptr++ = p;

                    lastArray[n] = p;
                }

                *fvptr++ = last0;
                *fvptr++ = lastArray[0];
            }
        }
    }

    if (lineWidth > 0 && debugLineColor[3] > 0) {
        lastArray[0] = origin + axis[2] * radius;
        for (int n = 1; n < num; n++) {
            lastArray[n] = lastArray[0];
        }

        for (int i = 15; i <= 180; i += 15) {
            float cr = Math::Cos16(DEG2RAD(i)) * radius;
            float sr = Math::Sin16(DEG2RAD(i)) * radius;

            lastp = origin + cr * axis[2] + sr * axis[0];

            int j = 15;
            for (int n = 0; j <= 360; j += 15, n++) {
                raxis = axis[0] * Math::Cos16(DEG2RAD(j)) + axis[1] * Math::Sin16(DEG2RAD(j));
                p = origin + cr * axis[2] + sr * raxis;

                DebugLine(lastp, p, lineWidth, depthTest, lifeTime);
                DebugLine(lastp, lastArray[n], lineWidth, depthTest, lifeTime);

                lastArray[n] = lastp;
                lastp = p;
            }
        }
    }
}

void RenderWorld::DebugSphereSimple(const Vec3 &origin, const Mat3 &axis, float radius, float lineWidth, bool depthTest, int lifeTime) {
    DebugCircle(origin, axis[0], radius, lineWidth, false, depthTest, lifeTime);
    DebugCircle(origin, axis[1], radius, lineWidth, false, depthTest, lifeTime);
    DebugCircle(origin, axis[2], radius, lineWidth, false, depthTest, lifeTime);
}

void RenderWorld::DebugAABB(const AABB &aabb, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    if (aabb.IsCleared()) {
        return;
    }

    Vec3 v[8];
    aabb.ToPoints(v);

    /*for (int i = 0; i < 8; i++) {
        v[i][0] = origin[0] + aabb[(i ^ (i >> 1)) & 1][0];
        v[i][1] = origin[1] + aabb[(i >> 1) & 1][1];
        v[i][2] = origin[2] + aabb[(i >> 2) & 1][2];
    }*/

    if (debugFillColor[3] > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleStripPrim, 14, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = v[7];
            *fvptr++ = v[4];
            *fvptr++ = v[6];
            *fvptr++ = v[5];

            *fvptr++ = v[1];
            *fvptr++ = v[4];
            *fvptr++ = v[0];
            *fvptr++ = v[7];

            *fvptr++ = v[3];
            *fvptr++ = v[6];
            *fvptr++ = v[2];
            *fvptr++ = v[1];

            *fvptr++ = v[3];
            *fvptr++ = v[0];
        }
    }

    if (lineWidth > 0 && debugLineColor[3] > 0) {
        for (int i = 0; i < 4; i++) {
            DebugLine(v[i], v[(i + 1) & 3], lineWidth, depthTest, lifeTime);
            DebugLine(v[4 + i], v[4 + ((i + 1) & 3)], lineWidth, depthTest, lifeTime);
            DebugLine(v[i], v[4 + i], lineWidth, depthTest, lifeTime);
        }
    }
}

void RenderWorld::DebugOBB(const OBB &obb, float lineWidth, bool depthTest, bool twoSided, int lifeTime) {
    Vec3 v[8];

    obb.ToPoints(v);

    if (debugFillColor[3] > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleStripPrim, 14, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = v[7];
            *fvptr++ = v[4];
            *fvptr++ = v[6];
            *fvptr++ = v[5];

            *fvptr++ = v[1];
            *fvptr++ = v[4];
            *fvptr++ = v[0];
            *fvptr++ = v[7];

            *fvptr++ = v[3];
            *fvptr++ = v[6];
            *fvptr++ = v[2];
            *fvptr++ = v[1];

            *fvptr++ = v[3];
            *fvptr++ = v[0];
        }
    }

    if (lineWidth > 0 && debugLineColor[3] > 0) {
        for (int i = 0; i < 4; i++) {
            DebugLine(v[i], v[(i + 1) & 3], lineWidth, depthTest, lifeTime);
            DebugLine(v[4 + i], v[4 + ((i + 1) & 3)], lineWidth, depthTest, lifeTime);
            DebugLine(v[i], v[4 + i], lineWidth, depthTest, lifeTime);
        }
    }
}

void RenderWorld::DebugFrustum(const Frustum &frustum, const bool showFromOrigin, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 v[8];

    frustum.ToPoints(v);

    if (debugFillColor[3] > 0) {
        Vec3 *fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleStripPrim, 24, debugFillColor, 0, twoSided, depthTest, lifeTime);
        if (fvptr) {
            *fvptr++ = v[7];
            *fvptr++ = v[4];
            *fvptr++ = v[6];
            *fvptr++ = v[5];

            *fvptr++ = v[1];
            *fvptr++ = v[4];
            *fvptr++ = v[0];
            *fvptr++ = v[7];

            *fvptr++ = v[3];
            *fvptr++ = v[6];
            *fvptr++ = v[2];
            *fvptr++ = v[1];

            *fvptr++ = v[3];
            *fvptr++ = v[0];
        }
    }

    if (lineWidth > 0 && debugLineColor[3] > 0) {
        if (frustum.GetNearDistance() > 0.0f ) {
            for (int i = 0; i < 4; i++) {
                DebugLine(v[i], v[(i + 1) & 3], lineWidth, depthTest, lifeTime);
            }

            if (showFromOrigin) {
                for (int i = 0; i < 4; i++ ) {
                    DebugLine(frustum.GetOrigin(), v[i], lineWidth, depthTest, lifeTime);
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            DebugLine(v[4 + i], v[4 + ((i + 1) & 3)], lineWidth, depthTest, lifeTime);
            DebugLine(v[i], v[4 + i], lineWidth, depthTest, lifeTime);
        }
    }
}

// radius1 is the radius at the apex
// radius2 is the radius at the bottom
void RenderWorld::DebugCone(const Vec3 &origin, const Mat3 &axis, float height, float radius1, float radius2, bool drawCap, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 apex = origin + axis[2] * height;
    Vec3 lastp2 = origin + radius2 * axis[0];
    Vec3 p1, p2, d;

    Vec3 *fvptr = nullptr;

    if (radius1 == 0.0f) {
        if (debugFillColor[3] > 0) {
            fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleFanPrim, (360 / 15) + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
            if (fvptr) {
                *fvptr++ = apex;

                for (int i = 0; i <= 360; i += 15) {
                    d = Math::Cos16(DEG2RAD(i)) * axis[0] + Math::Sin16(DEG2RAD(i)) * axis[1];
                    *fvptr++ = origin + d * radius2;
                }

                if (drawCap) {
                    fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleFanPrim, (360 / 15) + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
                    *fvptr++ = origin;

                    for (int i = 0; i <= 360; i += 15) {
                        d = Math::Cos16(DEG2RAD(i)) * axis[0] - Math::Sin16(DEG2RAD(i)) * axis[1];
                        *fvptr++ = origin + d * radius2;
                    }
                }
            }
        }

        if (debugLineColor[3] > 0) {
            for (int i = 15; i <= 360; i += 15) {
                d = Math::Cos16(DEG2RAD(i)) * axis[0] + Math::Sin16(DEG2RAD(i)) * axis[1];
                p2 = origin + d * radius2;

                DebugLine(lastp2, p2, lineWidth, depthTest, lifeTime);
                DebugLine(p2, apex, lineWidth, depthTest, lifeTime);

                lastp2 = p2;
            }
        }
    } else {
        Vec3 lastp1 = apex + radius1 * axis[0];

        if (debugFillColor[3] > 0) {
            fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleStripPrim, (360 / 15) * 2 + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
            if (fvptr) {
                *fvptr++ = lastp1;
                *fvptr++ = lastp2;

                for (int i = 15; i <= 360; i += 15) {
                    d = Math::Cos16(DEG2RAD(i)) * axis[0] + Math::Sin16(DEG2RAD(i)) * axis[1];
                    *fvptr++ = apex + d * radius1;
                    *fvptr++ = origin + d * radius2;
                }

                if (drawCap) {
                    fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleFanPrim, (360 / 15) + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
                    *fvptr++ = apex;

                    for (int i = 0; i <= 360; i += 15) {
                        d = Math::Cos16(DEG2RAD(i)) * axis[0] - Math::Sin16(DEG2RAD(i)) * axis[1];
                        *fvptr++ = apex + d * radius1;
                    }

                    fvptr = RB_ReserveDebugPrimsVerts(RHI::TriangleFanPrim, (360 / 15) + 2, debugFillColor, 0, twoSided, depthTest, lifeTime);
                    *fvptr++ = origin;

                    for (int i = 0; i <= 360; i += 15) {
                        d = Math::Cos16(DEG2RAD(i)) * axis[0] - Math::Sin16(DEG2RAD(i)) * axis[1];
                        *fvptr++ = origin + d * radius2;
                    }
                }
            }
        }

        if (lineWidth > 0 && debugLineColor[3] > 0) {
            for (int i = 15; i <= 360; i += 15) {
                d = Math::Cos16(DEG2RAD(i)) * axis[0] + Math::Sin16(DEG2RAD(i)) * axis[1];
                p1 = apex + d * radius1;
                p2 = origin + d * radius2;

                DebugLine(lastp1, p1, lineWidth, depthTest, lifeTime);
                DebugLine(lastp2, p2, lineWidth, depthTest, lifeTime);
                DebugLine(p1, p2, lineWidth, depthTest, lifeTime);

                lastp1 = p1;
                lastp2 = p2;
            }
        }
    }
}

void RenderWorld::DebugCone(const Vec3 &apex, const Vec3 &bottom, float radius1, float radius2, bool drawCap, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {	
    Mat3 axis;
    axis[2] = apex - bottom;
    float height = axis[2].Normalize();
    axis[2].NormalVectors(axis[0], axis[1]);
    axis[1] = -axis[1];

    DebugCone(bottom, axis, height, radius1, radius2, drawCap, lineWidth, twoSided, depthTest, lifeTime);
}

void RenderWorld::DebugConeSimple(const Vec3 &origin, const Mat3 &axis, float height, float radius1, float radius2, bool drawCap, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 apex = origin + axis[2] * height;
    Vec3 vec[2][2];
    vec[0][0] = axis[0] * radius2;
    vec[0][1] = axis[1] * radius2;
    vec[1][0] = axis[0] * radius1;
    vec[1][1] = axis[1] * radius1;

    if (radius1 == 0.0f) {
        DebugCircle(apex, axis[2], radius1, lineWidth, twoSided, depthTest, lifeTime);
    }

    DebugCircle(origin, axis[2], radius2, lineWidth, twoSided, depthTest, lifeTime);

    DebugLine(origin + vec[0][0], apex + vec[1][0], lineWidth, depthTest, lifeTime);
    DebugLine(origin - vec[0][0], apex - vec[1][0], lineWidth, depthTest, lifeTime);
    DebugLine(origin + vec[0][1], apex + vec[1][1], lineWidth, depthTest, lifeTime);
    DebugLine(origin - vec[0][1], apex - vec[1][1], lineWidth, depthTest, lifeTime);
}

void RenderWorld::DebugCylinder(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {	
    Vec3 origin = center - axis[2] * height * 0.5f;

    DebugCone(origin, axis, height, radius, radius, true, lineWidth, twoSided, depthTest, lifeTime);
}

void RenderWorld::DebugCylinder(const Vec3 &top, const Vec3 &bottom, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {	
    DebugCone(top, bottom, radius, radius, true, lineWidth, twoSided, depthTest, lifeTime);
}

void RenderWorld::DebugCylinderSimple(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth, bool depthTest, int lifeTime) {
    Vec3 top = center + axis[2] * height * 0.5f;
    Vec3 bottom = center - axis[2] * height * 0.5f;

    DebugCircle(top, axis[2], radius, lineWidth, false, depthTest, lifeTime);
    DebugCircle(bottom, -axis[2], radius, lineWidth, false, depthTest, lifeTime);

    Vec3 vec[2];
    vec[0] = axis[0] * radius;
    vec[1] = axis[1] * radius;

    DebugLine(bottom + vec[0], top + vec[0], lineWidth, depthTest, lifeTime);
    DebugLine(bottom - vec[0], top - vec[0], lineWidth, depthTest, lifeTime);
    DebugLine(bottom + vec[1], top + vec[1], lineWidth, depthTest, lifeTime);
    DebugLine(bottom - vec[1], top - vec[1], lineWidth, depthTest, lifeTime);
}

void RenderWorld::DebugCapsule(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Vec3 a = center + axis[2] * height * 0.5f;
    Vec3 b = center - axis[2] * height * 0.5f;

    DebugCone(a, axis, height, radius, radius, false, lineWidth, twoSided, depthTest, lifeTime);

    DebugHemisphere(a, axis, radius, lineWidth, twoSided, depthTest, lifeTime);

    Mat3 axis2;
    axis2[0] = axis[0];
    axis2[1] = -axis[1];
    axis2[2] = -axis[2];
    DebugHemisphere(b, axis2, radius, lineWidth, twoSided, depthTest, lifeTime);
}

void RenderWorld::DebugCapsule(const Vec3 &a, const Vec3 &b, float radius, float lineWidth, bool twoSided, bool depthTest, int lifeTime) {
    Mat3 axis;
    axis[2] = a - b;
    axis[2].Normalize();
    axis[2].NormalVectors(axis[0], axis[1]);
    axis[1] = -axis[1];

    DebugCone(a, b, radius, radius, false, lineWidth, twoSided, depthTest, lifeTime);

    DebugHemisphere(a, axis, radius, lineWidth, twoSided, depthTest, lifeTime);

    axis[2] = -axis[2];
    axis[1] = -axis[1];
    DebugHemisphere(b, axis, radius, lineWidth, twoSided, depthTest, lifeTime);
}

void RenderWorld::DebugCapsuleSimple(const Vec3 &center, const Mat3 &axis, float height, float radius, float lineWidth, bool depthTest, int lifeTime) {
    Vec3 a = center + axis[2] * height * 0.5f;
    Vec3 b = center - axis[2] * height * 0.5f;

    Vec3 vec[2];
    vec[0] = axis[0] * radius;
    vec[1] = axis[1] * radius;

    DebugLine(a + vec[0], b + vec[0], lineWidth, depthTest, lifeTime);
    DebugLine(a - vec[0], b - vec[0], lineWidth, depthTest, lifeTime);
    DebugLine(a + vec[1], b + vec[1], lineWidth, depthTest, lifeTime);
    DebugLine(a - vec[1], b - vec[1], lineWidth, depthTest, lifeTime);

    DebugHemisphereSimple(a, axis, radius, lineWidth, depthTest, lifeTime);

    Mat3 axis2;
    axis2[0] = axis[0];
    axis2[1] = -axis[1];
    axis2[2] = -axis[2];
    DebugHemisphereSimple(b, axis2, radius, lineWidth, depthTest, lifeTime);
}

void RenderWorld::DebugArrow(const Vec3 &start, const Vec3 &end, float coneSize, float coneRadius, float lineWidth, bool depthTest, int lifeTime) {
    Vec3 dir = end - start;
    float length = dir.Normalize();
    coneSize = Min(coneSize, length);
    Vec3 coneBottom = end - dir * coneSize;

    Color4 _debugLineColor = debugLineColor;	
    Color4 _debugFillColor = debugFillColor;

    SetDebugColor(Color4::zero, _debugFillColor);
    DebugCone(end, coneBottom, 0, coneRadius, false, 0, true, depthTest, lifeTime);

    SetDebugColor(Color4::zero, Color4(0, 0, 0, 0.75f));
    DebugCircle(coneBottom - dir * 0.01f, -dir, coneRadius, 0, false, depthTest, lifeTime);

    if (lineWidth > 0 && _debugLineColor[3] > 0) {
        SetDebugColor(_debugLineColor, Color4::zero);
        DebugLine(start, coneBottom, lineWidth, depthTest, lifeTime);
    }

    SetDebugColor(_debugLineColor, _debugFillColor);
}

void RenderWorld::ClearDebugText(int time) {
    RB_ClearDebugText(time);
}

void RenderWorld::DebugText(const char *text, const Vec3 &origin, const Mat3 &viewAxis, float scale, float lineWidth, const int align, bool depthTest, int lifeTime) {
    RB_AddDebugText(text, origin, viewAxis, scale, lineWidth, debugLineColor, align, lifeTime, depthTest);
}

void RenderWorld::DebugJoints(const SceneEntity *ent, bool showJointsNames, const Mat3 &viewAxis) {
    const Joint *joint = ent->parms.mesh->GetJoints();
    const Mat3x4 *jointMat = ent->parms.joints;

    for (int i = 0; i < ent->parms.numJoints; i++, joint++, jointMat++) {
        Vec3 pos = ent->parms.origin + ent->parms.axis * (ent->parms.scale * jointMat->ToTranslationVec3());
        Mat3 mat = jointMat->ToMat3();

        if (joint->parent) {
            int parentNum = (int)(joint->parent - ent->parms.mesh->GetJoints());
            SetDebugColor(Color4::white, Color4::zero);
            DebugLine(pos, ent->parms.origin + ent->parms.axis * (ent->parms.scale * ent->parms.joints[parentNum].ToTranslationVec3()), 1);
        }

        SetDebugColor(Color4::red, Color4::zero);
        DebugLine(pos, pos + ent->parms.axis * mat[0] * CentiToUnit(1), 1);
        SetDebugColor(Color4::green, Color4::zero);
        DebugLine(pos, pos + ent->parms.axis * mat[1] * CentiToUnit(1), 1);
        SetDebugColor(Color4::blue, Color4::zero);
        DebugLine(pos, pos + ent->parms.axis * mat[2] * CentiToUnit(1), 1);

        if (showJointsNames) {
            SetDebugColor(Color4::white, Color4::zero);
            DebugText(joint->name.c_str(), pos, viewAxis, CentiToUnit(4), 1, 0);
        }
    }
}

BE_NAMESPACE_END
