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
    objectDbvt.Purge();
    lightDbvt.Purge();
    staticMeshDbvt.Purge();

    for (int i = 0; i < renderObjects.Count(); i++) {
        SAFE_DELETE(renderObjects[i]);
    }
    for (int i = 0; i < renderLights.Count(); i++) {
        SAFE_DELETE(renderLights[i]);
    }
}

const RenderObject *RenderWorld::GetRenderObject(int handle) const {
    if (handle < 0 || handle >= renderObjects.Count()) {
        BE_WARNLOG(L"RenderWorld::GetRenderObject: handle %i > %i\n", handle, renderObjects.Count() - 1);
        return nullptr;
    }

    RenderObject *renderObject = renderObjects[handle];
    if (!renderObject) {
        BE_WARNLOG(L"RenderWorld::GetRenderObject: handle %i is nullptr\n", handle);
        return nullptr;
    }

    return renderObject;
}

int RenderWorld::AddRenderObject(const RenderObject::State *objectDef) {
    int handle = renderObjects.FindNull();
    if (handle == -1) {
        handle = renderObjects.Append(nullptr);
    }

    UpdateRenderObject(handle, objectDef);

    return handle;
}

void RenderWorld::UpdateRenderObject(int handle, const RenderObject::State *objectDef) {
    while (handle >= renderObjects.Count()) {
        renderObjects.Append(nullptr);
    }

    RenderObject *renderObject = renderObjects[handle];
    if (!renderObject) {
        renderObject = new RenderObject;
        renderObjects[handle] = renderObject;

        renderObject->index = handle;
        renderObject->Update(objectDef);

        // Add proxy in the DBVT for the renderObjects
        renderObject->proxy = (DbvtProxy *)Mem_ClearedAlloc(sizeof(DbvtProxy));
        renderObject->proxy->renderObject = renderObject;
        renderObject->proxy->worldAABB.SetFromTransformedAABB(objectDef->localAABB * objectDef->scale, objectDef->origin, objectDef->axis);
        renderObject->proxy->id = objectDbvt.CreateProxy(renderObject->proxy->worldAABB, MeterToUnit(0.5f), renderObject->proxy);

        // If this object is a static mesh, add proxy for each sub meshes in the DBVT for the static meshes
        if (objectDef->mesh && !objectDef->joints) {
            renderObject->numMeshSurfProxies = objectDef->mesh->NumSurfaces();
            renderObject->meshSurfProxies = (DbvtProxy *)Mem_ClearedAlloc(objectDef->mesh->NumSurfaces() * sizeof(DbvtProxy));

            for (int surfaceIndex = 0; surfaceIndex < objectDef->mesh->NumSurfaces(); surfaceIndex++) {
                const MeshSurf *meshSurf = objectDef->mesh->GetSurface(surfaceIndex);

                DbvtProxy *meshSurfProxy = &renderObject->meshSurfProxies[surfaceIndex];
                meshSurfProxy->renderObject = renderObject;
                meshSurfProxy->mesh = objectDef->mesh;
                meshSurfProxy->meshSurfIndex = surfaceIndex;
                meshSurfProxy->worldAABB.SetFromTransformedAABB(meshSurf->subMesh->GetAABB() * objectDef->scale, objectDef->origin, objectDef->axis);
                meshSurfProxy->id = staticMeshDbvt.CreateProxy(renderObject->meshSurfProxies[surfaceIndex].worldAABB, MeterToUnit(0.0f), &renderObject->meshSurfProxies[surfaceIndex]);
            }
        }
    } else {
        bool originMatch    = (objectDef->origin == renderObject->state.origin);
        bool axisMatch      = (objectDef->axis == renderObject->state.axis);
        bool scaleMatch     = (objectDef->scale == renderObject->state.scale);
        bool aabbMatch      = (objectDef->localAABB == renderObject->state.localAABB);
        bool meshMatch      = (objectDef->mesh == renderObject->state.mesh);
        bool proxyMoved     = !originMatch || !axisMatch || !scaleMatch || !aabbMatch;

        if (proxyMoved || !meshMatch) {
            if (proxyMoved) {
                renderObject->proxy->worldAABB.SetFromTransformedAABB(objectDef->localAABB * objectDef->scale, objectDef->origin, objectDef->axis);
                objectDbvt.MoveProxy(renderObject->proxy->id, renderObject->proxy->worldAABB, MeterToUnit(0.5f), objectDef->origin - renderObject->state.origin);
            }

            // If this object is a static mesh
            if (renderObject->state.mesh && !renderObject->state.joints) {
                // mesh surface count changed so we recreate static proxy
                if (objectDef->mesh->NumSurfaces() != renderObject->numMeshSurfProxies) {
                    Mem_Free(renderObject->meshSurfProxies);

                    renderObject->numMeshSurfProxies = objectDef->mesh->NumSurfaces();
                    renderObject->meshSurfProxies = (DbvtProxy *)Mem_ClearedAlloc(renderObject->numMeshSurfProxies * sizeof(DbvtProxy));

                    for (int surfaceIndex = 0; surfaceIndex < objectDef->mesh->NumSurfaces(); surfaceIndex++) {
                        const MeshSurf *meshSurf = objectDef->mesh->GetSurface(surfaceIndex);

                        staticMeshDbvt.DestroyProxy(renderObject->meshSurfProxies[surfaceIndex].id);

                        DbvtProxy *meshSurfProxy = &renderObject->meshSurfProxies[surfaceIndex];
                        meshSurfProxy->renderObject = renderObject;
                        meshSurfProxy->mesh = objectDef->mesh;
                        meshSurfProxy->meshSurfIndex = surfaceIndex;
                        meshSurfProxy->worldAABB.SetFromTransformedAABB(meshSurf->subMesh->GetAABB() * objectDef->scale, objectDef->origin, objectDef->axis);
                        meshSurfProxy->id = staticMeshDbvt.CreateProxy(renderObject->meshSurfProxies[surfaceIndex].worldAABB, MeterToUnit(0.0f), &renderObject->meshSurfProxies[surfaceIndex]);
                    }
                } else {
                    for (int surfaceIndex = 0; surfaceIndex < objectDef->mesh->NumSurfaces(); surfaceIndex++) {
                        renderObject->meshSurfProxies[surfaceIndex].worldAABB.SetFromTransformedAABB(objectDef->mesh->GetSurface(surfaceIndex)->subMesh->GetAABB() * objectDef->scale, objectDef->origin, objectDef->axis);
                        staticMeshDbvt.MoveProxy(renderObject->meshSurfProxies[surfaceIndex].id, renderObject->meshSurfProxies[surfaceIndex].worldAABB, MeterToUnit(0.5f), objectDef->origin - renderObject->state.origin);
                    }
                }
            }
        }

        renderObject->Update(objectDef);
    }
}

void RenderWorld::RemoveRenderObject(int handle) {
    if (handle < 0 || handle >= renderObjects.Count()) {
        BE_WARNLOG(L"RenderWorld::RemoveRenderObject: handle %i > %i\n", handle, renderObjects.Count() - 1);
        return;
    }

    RenderObject *renderObject = renderObjects[handle];
    if (!renderObject) {
        BE_WARNLOG(L"RenderWorld::RemoveRenderObject: handle %i is nullptr\n", handle);
        return;
    }

    objectDbvt.DestroyProxy(renderObject->proxy->id);
    for (int i = 0; i < renderObject->numMeshSurfProxies; i++) {
        staticMeshDbvt.DestroyProxy(renderObject->meshSurfProxies[i].id);
    }

    delete renderObjects[handle];
    renderObjects[handle] = nullptr;
}

const RenderLight *RenderWorld::GetRenderLight(int handle) const {
    if (handle < 0 || handle >= renderLights.Count()) {
        BE_WARNLOG(L"RenderWorld::GetRenderLight: handle %i > %i\n", handle, renderLights.Count() - 1);
        return nullptr;
    }

    RenderLight *renderLight = renderLights[handle];
    if (!renderLight) {
        BE_WARNLOG(L"RenderWorld::GetRenderLight: handle %i is nullptr\n", handle);
        return nullptr;
    }

    return renderLight;
}

int RenderWorld::AddRenderLight(const RenderLight::State *lightDef) {
    int handle = renderLights.FindNull();
    if (handle == -1) {
        handle = renderLights.Append(nullptr);
    }

    UpdateRenderLight(handle, lightDef);

    return handle;
}

void RenderWorld::UpdateRenderLight(int handle, const RenderLight::State *lightDef) {
    while (handle >= renderLights.Count()) {
        renderLights.Append(nullptr);
    }

    RenderLight *renderLight = renderLights[handle];
    if (!renderLight) {
        renderLight = new RenderLight;
        renderLights[handle] = renderLight;

        renderLight->index = handle;
        renderLight->Update(lightDef);

        renderLight->proxy = (DbvtProxy *)Mem_ClearedAlloc(sizeof(DbvtProxy));
        renderLight->proxy->renderLight = renderLight;
        renderLight->proxy->worldAABB = renderLight->GetWorldAABB();
        renderLight->proxy->id = lightDbvt.CreateProxy(renderLight->proxy->worldAABB, MeterToUnit(0.0f), renderLight->proxy);
    } else {
        bool originMatch    = (lightDef->origin == renderLight->state.origin);
        bool axisMatch      = (lightDef->axis == renderLight->state.axis);
        bool valueMatch     = (lightDef->size == renderLight->state.size);

        if (!originMatch || !axisMatch || !valueMatch) {
            renderLight->proxy->worldAABB = renderLight->proxy->renderLight->GetWorldAABB();
            lightDbvt.MoveProxy(renderLight->proxy->id, renderLight->proxy->worldAABB, MeterToUnit(0.5f), lightDef->origin - renderLight->state.origin);
        }

        renderLight->Update(lightDef);
    }
}

void RenderWorld::RemoveRenderLight(int handle) {
    if (handle < 0 || handle >= renderLights.Count()) {
        BE_WARNLOG(L"RenderWorld::RemoveRenderLight: handle %i > %i\n", handle, renderLights.Count() - 1);
        return;
    }

    RenderLight *renderLight = renderLights[handle];
    if (!renderLight) {
        BE_WARNLOG(L"RenderWorld::RemoveRenderLight: handle %i is nullptr\n", handle);
        return;
    }

    lightDbvt.DestroyProxy(renderLight->proxy->id);

    delete renderLights[handle];
    renderLights[handle] = nullptr;
}

void RenderWorld::SetSkyboxMaterial(Material *skyboxMaterial) {
    this->skyboxMaterial = skyboxMaterial;
}

void RenderWorld::FinishMapLoading() {
//#ifndef _DEBUG
//    int startTime = PlatformTime::Milliseconds();
//
//    objectDbvt.RebuildBottomUp();
//    staticMeshDbvt.RebuildBottomUp();
//    lightDbvt.RebuildBottomUp();
//
//    int elapsedTime = PlatformTime::Milliseconds() - startTime;
//    BE_LOG(L"%i msec to build dynamic AABB tree\n", elapsedTime);
//#endif
}

void RenderWorld::RenderScene(const RenderView *renderView) {
    if (renderView->state.renderRect.w <= 0.0f || renderView->state.renderRect.h <= 0.0f) {
        return;
    }

    if (renderView->state.layerMask == 0) {
        return;
    }

    // Create current visible view in frame data
    currentView = (VisibleView *)frameData.ClearedAlloc(sizeof(*currentView));
    currentView->def = renderView;
    currentView->maxDrawSurfs = MaxViewDrawSurfs;
    currentView->drawSurfs = (DrawSurf **)frameData.Alloc(currentView->maxDrawSurfs * sizeof(DrawSurf *));
    currentView->instanceBufferCache = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));

    new (&currentView->visObjects) LinkList<VisibleObject>();
    new (&currentView->visLights) LinkList<VisibleLight>();

    RenderCamera(currentView);
}

void RenderWorld::EmitGuiFullScreen(GuiMesh &guiMesh) {
    if (guiMesh.NumSurfaces() == 0) {
        return;
    }

    viewCount++;

    // GUI view def
    RenderView::State renderViewDef;
    renderViewDef.renderRect    = Rect(0, 0, renderSystem.currentContext->GetDeviceWidth(), renderSystem.currentContext->GetDeviceHeight());
    renderViewDef.time          = PlatformTime::Milliseconds();
    renderViewDef.orthogonal    = true;
    renderViewDef.zNear         = 0.0f;
    renderViewDef.zFar          = 1.0f;
    renderViewDef.sizeX         = renderSystem.currentContext->GetDeviceWidth() * 0.5f;
    renderViewDef.sizeY         = renderSystem.currentContext->GetDeviceHeight() * 0.5f;
    renderViewDef.axis          = Mat3::identity;
    renderViewDef.origin        = Vec3::origin;

    static RenderView renderView;
    new (&renderView) RenderView();
    renderView.Update(&renderViewDef);

    // GUI object def
    RenderObject::State objectDef;
    memset(&objectDef, 0, sizeof(objectDef));
    objectDef.scale = Vec3::one;
    objectDef.axis = Mat3::identity;
    objectDef.materialParms[RenderObject::RedParm] = 1.0f;
    objectDef.materialParms[RenderObject::GreenParm] = 1.0f;
    objectDef.materialParms[RenderObject::BlueParm] = 1.0f;
    objectDef.materialParms[RenderObject::AlphaParm] = 1.0f;
    objectDef.materialParms[RenderObject::TimeScaleParm] = 1.0f;
    
    static RenderObject renderObject;
    new (&renderObject) RenderObject();
    renderObject.Update(&objectDef);

    // GUI view
    VisibleView *guiView    = (VisibleView *)frameData.ClearedAlloc(sizeof(*guiView));
    guiView->def            = &renderView;
    guiView->is2D           = true;
    guiView->maxDrawSurfs   = guiMesh.NumSurfaces();
    guiView->drawSurfs      = (DrawSurf **)frameData.Alloc(guiView->maxDrawSurfs * sizeof(DrawSurf *));

    new (&guiView->visObjects) LinkList<VisibleObject>();
    new (&guiView->visLights) LinkList<VisibleLight>();

    // GUI visible object
    Mat4 projMatrix;
    projMatrix.SetOrtho(0, renderSystem.currentContext->GetDeviceWidth(), renderSystem.currentContext->GetDeviceHeight(), 0, -1.0, 1.0);

    VisibleObject *visObject = RegisterVisibleObject(guiView, &renderObject);
    visObject->modelViewMatrix.SetIdentity();
    visObject->modelViewMatrix.Scale(renderSystem.currentContext->GetUpscaleFactorX(), renderSystem.currentContext->GetUpscaleFactorY(), 1.0f);
    visObject->modelViewProjMatrix = projMatrix * visObject->modelViewMatrix;

    guiMesh.CacheIndexes();
    
    for (int surfaceIndex = 0; surfaceIndex < guiMesh.NumSurfaces(); surfaceIndex++) {
        const GuiMeshSurf *guiSurf = guiMesh.Surface(surfaceIndex);
        if (!guiSurf->numIndexes) {
            break;
        }

        SubMesh *subMesh = (SubMesh *)frameData.ClearedAlloc(sizeof(SubMesh));
        new (subMesh) SubMesh();

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
        AddDrawSurf(guiView, nullptr, visObject, guiSurf->material, subMesh, 0);

        guiView->numAmbientSurfs++;
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

void RenderWorld::DebugJoints(const RenderObject *renderObject, bool showJointsNames, const Mat3 &viewAxis) {
    const Joint *joint = renderObject->state.mesh->GetJoints();
    const Mat3x4 *jointMat = renderObject->state.joints;

    for (int i = 0; i < renderObject->state.numJoints; i++, joint++, jointMat++) {
        Vec3 pos = renderObject->state.origin + renderObject->state.axis * (renderObject->state.scale * jointMat->ToTranslationVec3());
        Mat3 mat = jointMat->ToMat3();

        if (joint->parent) {
            int parentNum = (int)(joint->parent - renderObject->state.mesh->GetJoints());
            SetDebugColor(Color4::white, Color4::zero);
            DebugLine(pos, renderObject->state.origin + renderObject->state.axis * (renderObject->state.scale * renderObject->state.joints[parentNum].ToTranslationVec3()), 1);
        }

        SetDebugColor(Color4::red, Color4::zero);
        DebugLine(pos, pos + renderObject->state.axis * mat[0] * CentiToUnit(1.0f), 1);
        SetDebugColor(Color4::green, Color4::zero);
        DebugLine(pos, pos + renderObject->state.axis * mat[1] * CentiToUnit(1.0f), 1);
        SetDebugColor(Color4::blue, Color4::zero);
        DebugLine(pos, pos + renderObject->state.axis * mat[2] * CentiToUnit(1.0f), 1);

        if (showJointsNames) {
            SetDebugColor(Color4::white, Color4::zero);
            DebugText(joint->name.c_str(), pos, viewAxis, CentiToUnit(4.0f), 1, 0);
        }
    }
}

BE_NAMESPACE_END
