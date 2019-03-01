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
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

static constexpr int MaxViewDrawSurfs = 0x4000;

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

RenderObject *RenderWorld::GetRenderObject(int handle) const {
    if (handle < 0 || handle >= renderObjects.Count()) {
        BE_WARNLOG("RenderWorld::GetRenderObject: handle %i > %i\n", handle, renderObjects.Count() - 1);
        return nullptr;
    }

    RenderObject *renderObject = renderObjects[handle];
    if (!renderObject) {
        BE_WARNLOG("RenderWorld::GetRenderObject: handle %i is nullptr\n", handle);
        return nullptr;
    }

    return renderObject;
}

int RenderWorld::AddRenderObject(const RenderObject::State *def) {
    int handle = renderObjects.FindNull();
    if (handle == -1) {
        handle = renderObjects.Append(nullptr);
    }

    UpdateRenderObject(handle, def);

    return handle;
}

void RenderWorld::UpdateRenderObject(int handle, const RenderObject::State *def) {
    while (handle >= renderObjects.Count()) {
        renderObjects.Append(nullptr);
    }

    RenderObject *renderObject = renderObjects[handle];
    if (!renderObject) {
        renderObject = new RenderObject(handle);
        renderObjects[handle] = renderObject;

        renderObject->Update(def);

        // Add proxy in the DBVT for the renderObjects
        renderObject->proxy = (DbvtProxy *)Mem_ClearedAlloc(sizeof(DbvtProxy));
        renderObject->proxy->renderObject = renderObject;
        renderObject->proxy->worldAABB.SetFromTransformedAABB(def->localAABB * def->scale, def->origin, def->axis);
        renderObject->proxy->id = objectDbvt.CreateProxy(renderObject->proxy->worldAABB, MeterToUnit(0.5f), renderObject->proxy);

        // If this object is a static mesh, add proxy for each sub meshes in the DBVT for the static meshes
        if (def->mesh && !def->joints) {
            renderObject->numMeshSurfProxies = def->mesh->NumSurfaces();
            renderObject->meshSurfProxies = (DbvtProxy *)Mem_ClearedAlloc(def->mesh->NumSurfaces() * sizeof(DbvtProxy));

            for (int surfaceIndex = 0; surfaceIndex < def->mesh->NumSurfaces(); surfaceIndex++) {
                const MeshSurf *meshSurf = def->mesh->GetSurface(surfaceIndex);

                DbvtProxy *meshSurfProxy = &renderObject->meshSurfProxies[surfaceIndex];
                meshSurfProxy->renderObject = renderObject;
                meshSurfProxy->mesh = def->mesh;
                meshSurfProxy->meshSurfIndex = surfaceIndex;
                meshSurfProxy->worldAABB.SetFromTransformedAABB(meshSurf->subMesh->GetAABB() * def->scale, def->origin, def->axis);
                meshSurfProxy->id = staticMeshDbvt.CreateProxy(renderObject->meshSurfProxies[surfaceIndex].worldAABB, MeterToUnit(0.0f), &renderObject->meshSurfProxies[surfaceIndex]);
            }
        }
    } else {
        bool originMatch    = (def->origin == renderObject->state.origin);
        bool axisMatch      = (def->axis == renderObject->state.axis);
        bool scaleMatch     = (def->scale == renderObject->state.scale);
        bool aabbMatch      = (def->localAABB == renderObject->state.localAABB);
        bool meshMatch      = (def->mesh == renderObject->state.mesh);
        bool proxyMoved     = !originMatch || !axisMatch || !scaleMatch || !aabbMatch;

        if (proxyMoved || !meshMatch) {
            if (proxyMoved) {
                renderObject->proxy->worldAABB.SetFromTransformedAABB(def->localAABB * def->scale, def->origin, def->axis);
                objectDbvt.MoveProxy(renderObject->proxy->id, renderObject->proxy->worldAABB, MeterToUnit(0.5f), def->origin - renderObject->state.origin);
            }

            // If this object is a static mesh
            if (renderObject->state.mesh && !renderObject->state.joints) {
                // mesh surface count changed so we recreate static proxy
                if (def->mesh->NumSurfaces() != renderObject->numMeshSurfProxies) {
                    Mem_Free(renderObject->meshSurfProxies);

                    renderObject->numMeshSurfProxies = def->mesh->NumSurfaces();
                    renderObject->meshSurfProxies = (DbvtProxy *)Mem_ClearedAlloc(renderObject->numMeshSurfProxies * sizeof(DbvtProxy));

                    for (int surfaceIndex = 0; surfaceIndex < def->mesh->NumSurfaces(); surfaceIndex++) {
                        const MeshSurf *meshSurf = def->mesh->GetSurface(surfaceIndex);

                        staticMeshDbvt.DestroyProxy(renderObject->meshSurfProxies[surfaceIndex].id);

                        DbvtProxy *meshSurfProxy = &renderObject->meshSurfProxies[surfaceIndex];
                        meshSurfProxy->renderObject = renderObject;
                        meshSurfProxy->mesh = def->mesh;
                        meshSurfProxy->meshSurfIndex = surfaceIndex;
                        meshSurfProxy->worldAABB.SetFromTransformedAABB(meshSurf->subMesh->GetAABB() * def->scale, def->origin, def->axis);
                        meshSurfProxy->id = staticMeshDbvt.CreateProxy(renderObject->meshSurfProxies[surfaceIndex].worldAABB, MeterToUnit(0.0f), &renderObject->meshSurfProxies[surfaceIndex]);
                    }
                } else {
                    for (int surfaceIndex = 0; surfaceIndex < def->mesh->NumSurfaces(); surfaceIndex++) {
                        renderObject->meshSurfProxies[surfaceIndex].worldAABB.SetFromTransformedAABB(def->mesh->GetSurface(surfaceIndex)->subMesh->GetAABB() * def->scale, def->origin, def->axis);
                        staticMeshDbvt.MoveProxy(renderObject->meshSurfProxies[surfaceIndex].id, renderObject->meshSurfProxies[surfaceIndex].worldAABB, MeterToUnit(0.5f), def->origin - renderObject->state.origin);
                    }
                }
            }
        }

        renderObject->Update(def);
    }
}

void RenderWorld::RemoveRenderObject(int handle) {
    if (handle < 0 || handle >= renderObjects.Count()) {
        BE_WARNLOG("RenderWorld::RemoveRenderObject: handle %i > %i\n", handle, renderObjects.Count() - 1);
        return;
    }

    RenderObject *renderObject = renderObjects[handle];
    if (!renderObject) {
        BE_WARNLOG("RenderWorld::RemoveRenderObject: handle %i is nullptr\n", handle);
        return;
    }

    objectDbvt.DestroyProxy(renderObject->proxy->id);
    for (int i = 0; i < renderObject->numMeshSurfProxies; i++) {
        staticMeshDbvt.DestroyProxy(renderObject->meshSurfProxies[i].id);
    }

    delete renderObjects[handle];
    renderObjects[handle] = nullptr;
}

RenderLight *RenderWorld::GetRenderLight(int handle) const {
    if (handle < 0 || handle >= renderLights.Count()) {
        BE_WARNLOG("RenderWorld::GetRenderLight: handle %i > %i\n", handle, renderLights.Count() - 1);
        return nullptr;
    }

    RenderLight *renderLight = renderLights[handle];
    if (!renderLight) {
        BE_WARNLOG("RenderWorld::GetRenderLight: handle %i is nullptr\n", handle);
        return nullptr;
    }

    return renderLight;
}

int RenderWorld::AddRenderLight(const RenderLight::State *def) {
    int handle = renderLights.FindNull();
    if (handle == -1) {
        handle = renderLights.Append(nullptr);
    }

    UpdateRenderLight(handle, def);

    return handle;
}

void RenderWorld::UpdateRenderLight(int handle, const RenderLight::State *def) {
    while (handle >= renderLights.Count()) {
        renderLights.Append(nullptr);
    }

    RenderLight *renderLight = renderLights[handle];
    if (!renderLight) {
        renderLight = new RenderLight(handle);
        renderLights[handle] = renderLight;

        renderLight->Update(def);

        renderLight->proxy = (DbvtProxy *)Mem_ClearedAlloc(sizeof(DbvtProxy));
        renderLight->proxy->renderLight = renderLight;
        renderLight->proxy->worldAABB = renderLight->GetWorldAABB();
        renderLight->proxy->id = lightDbvt.CreateProxy(renderLight->proxy->worldAABB, MeterToUnit(0.0f), renderLight->proxy);
    } else {
        bool originMatch    = (def->origin == renderLight->state.origin);
        bool axisMatch      = (def->axis == renderLight->state.axis);
        bool valueMatch     = (def->size == renderLight->state.size);

        if (!originMatch || !axisMatch || !valueMatch) {
            renderLight->proxy->worldAABB = renderLight->proxy->renderLight->GetWorldAABB();
            lightDbvt.MoveProxy(renderLight->proxy->id, renderLight->proxy->worldAABB, MeterToUnit(0.5f), def->origin - renderLight->state.origin);
        }

        renderLight->Update(def);
    }
}

void RenderWorld::RemoveRenderLight(int handle) {
    if (handle < 0 || handle >= renderLights.Count()) {
        BE_WARNLOG("RenderWorld::RemoveRenderLight: handle %i > %i\n", handle, renderLights.Count() - 1);
        return;
    }

    RenderLight *renderLight = renderLights[handle];
    if (!renderLight) {
        BE_WARNLOG("RenderWorld::RemoveRenderLight: handle %i is nullptr\n", handle);
        return;
    }

    lightDbvt.DestroyProxy(renderLight->proxy->id);

    delete renderLights[handle];
    renderLights[handle] = nullptr;
}

ReflectionProbe *RenderWorld::GetReflectionProbe(int handle) const {
    if (handle < 0 || handle >= reflectionProbes.Count()) {
        BE_WARNLOG("RenderWorld::GetReflectionProbe: handle %i > %i\n", handle, reflectionProbes.Count() - 1);
        return nullptr;
    }

    ReflectionProbe *reflectionProbe = reflectionProbes[handle];
    if (!reflectionProbe) {
        BE_WARNLOG("RenderWorld::GetReflectionProbe: handle %i is nullptr\n", handle);
        return nullptr;
    }

    return reflectionProbe;
}

int RenderWorld::AddReflectionProbe(const ReflectionProbe::State *def) {
    int handle = reflectionProbes.FindNull();
    if (handle == -1) {
        handle = reflectionProbes.Append(nullptr);
    }

    UpdateReflectionProbe(handle, def);

    return handle;
}

void RenderWorld::UpdateReflectionProbe(int handle, const ReflectionProbe::State *def) {
    while (handle >= reflectionProbes.Count()) {
        reflectionProbes.Append(nullptr);
    }

    ReflectionProbe *reflectionProbe = reflectionProbes[handle];
    if (!reflectionProbe) {
        reflectionProbe = new ReflectionProbe(handle);
        reflectionProbes[handle] = reflectionProbe;

        reflectionProbe->Update(def);

        reflectionProbe->proxy = (DbvtProxy *)Mem_ClearedAlloc(sizeof(DbvtProxy));
        reflectionProbe->proxy->reflectionProbe = reflectionProbe;
        reflectionProbe->proxy->worldAABB = reflectionProbe->GetWorldAABB();
        reflectionProbe->proxy->id = probeDbvt.CreateProxy(reflectionProbe->proxy->worldAABB, MeterToUnit(0.0f), reflectionProbe->proxy);
    } else {
        bool originMatch = (def->origin == reflectionProbe->state.origin);
        bool boxSizeMatch = (def->boxSize == reflectionProbe->state.boxSize);

        if (!originMatch || !boxSizeMatch) {
            reflectionProbe->proxy->worldAABB = reflectionProbe->proxy->reflectionProbe->GetWorldAABB();
            probeDbvt.MoveProxy(reflectionProbe->proxy->id, reflectionProbe->proxy->worldAABB, MeterToUnit(0.5f), def->origin - reflectionProbe->state.origin);
        }

        reflectionProbe->Update(def);
    }
}

void RenderWorld::RemoveReflectionProbe(int handle) {
    if (handle < 0 || handle >= reflectionProbes.Count()) {
        BE_WARNLOG("RenderWorld::RemoveReflectionProbe: handle %i > %i\n", handle, reflectionProbes.Count() - 1);
        return;
    }

    ReflectionProbe *reflectionProbe = reflectionProbes[handle];
    if (!reflectionProbe) {
        BE_WARNLOG("RenderWorld::RemoveReflectionProbe: handle %i is nullptr\n", handle);
        return;
    }

    probeDbvt.DestroyProxy(reflectionProbe->proxy->id);

    delete reflectionProbes[handle];
    reflectionProbes[handle] = nullptr;
}

void RenderWorld::ScheduleToRefreshReflectionProbe(int handle) {
    ReflectionProbe *probe = GetReflectionProbe(handle);

    RenderContext *renderContext = renderSystem.GetMainRenderContext();
    //probe->ForceToRefresh(this);
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
//    BE_LOG("%i msec to build dynamic AABB tree\n", elapsedTime);
//#endif
}

void RenderWorld::RenderScene(const RenderCamera *renderCamera) {
    BE_PROFILE_CPU_SCOPE("RenderWorld::RenderScene", Color3::orange);

    if (renderCamera->GetState().renderRect.w <= 0.0f || renderCamera->GetState().renderRect.h <= 0.0f) {
        return;
    }

    if (renderCamera->GetState().layerMask == 0) {
        return;
    }

    // Create current camera in frame data
    currentCamera = (VisCamera *)frameData.ClearedAlloc(sizeof(*currentCamera));
    currentCamera->def = renderCamera;
    currentCamera->maxDrawSurfs = MaxViewDrawSurfs; 
    currentCamera->drawSurfs = (DrawSurf **)frameData.Alloc(currentCamera->maxDrawSurfs * sizeof(DrawSurf *));
    currentCamera->instanceBufferCache = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));

    new (&currentCamera->visObjects) LinkList<VisObject>();
    new (&currentCamera->visLights) LinkList<VisLight>();

    DrawCamera(currentCamera);
}

void RenderWorld::DrawGUICamera(GuiMesh &guiMesh) {
    BE_PROFILE_CPU_SCOPE("RenderWorld::DrawGUICamera", Color3::yellow);

    if (guiMesh.NumSurfaces() == 0) {
        return;
    }

    viewCount++;

    // Create GUI camera
    RenderCamera::State cameraDef;
    cameraDef.renderRect = Rect(0, 0, renderSystem.currentContext->GetDeviceWidth(), renderSystem.currentContext->GetDeviceHeight());
    cameraDef.time = PlatformTime::Milliseconds();
    cameraDef.orthogonal = true;
    cameraDef.zNear = 0.0f;
    cameraDef.zFar = 1.0f;
    cameraDef.sizeX = renderSystem.currentContext->GetDeviceWidth() * 0.5f;
    cameraDef.sizeY = renderSystem.currentContext->GetDeviceHeight() * 0.5f;
    cameraDef.origin = Vec3::origin;
    cameraDef.axis = Mat3::identity;

    static RenderCamera renderCamera;
    new (&renderCamera) RenderCamera();
    renderCamera.Update(&cameraDef);

    // GUI object def
    RenderObject::State def;
    def.materialParms[RenderObject::RedParm] = 1.0f;
    def.materialParms[RenderObject::GreenParm] = 1.0f;
    def.materialParms[RenderObject::BlueParm] = 1.0f;
    def.materialParms[RenderObject::AlphaParm] = 1.0f;
    def.materialParms[RenderObject::TimeScaleParm] = 1.0f;

    static RenderObject renderObject(-1);
    new (&renderObject) RenderObject(-1);
    renderObject.Update(&def);

    // GUI camera
    VisCamera *guiCamera = (VisCamera *)frameData.ClearedAlloc(sizeof(*guiCamera));
    guiCamera->def = &renderCamera;
    guiCamera->is2D = true;
    guiCamera->maxDrawSurfs = guiMesh.NumSurfaces();
    guiCamera->drawSurfs = (DrawSurf **)frameData.Alloc(guiCamera->maxDrawSurfs * sizeof(DrawSurf *));

    new (&guiCamera->visObjects) LinkList<VisObject>();
    new (&guiCamera->visLights) LinkList<VisLight>();

    // GUI visible object
    Mat4 projMatrix;
    projMatrix.SetOrtho(0, renderSystem.currentContext->GetDeviceWidth(), renderSystem.currentContext->GetDeviceHeight(), 0, -1.0, 1.0);

    VisObject *visObject = RegisterVisObject(guiCamera, &renderObject);
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

        subMesh->type = Mesh::DynamicMesh;
        subMesh->numIndexes = guiSurf->numIndexes;
        subMesh->numVerts = guiSurf->numVerts;
#if 1
        subMesh->vertexCache = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
        *(subMesh->vertexCache) = guiSurf->vertexCache;

        subMesh->indexCache = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));
        *(subMesh->indexCache) = guiSurf->indexCache;
#else
        subMesh->vertexCache = &guiSurf->vertexCache;
        subMesh->indexCache = &guiSurf->indexCache;
#endif
        AddDrawSurf(guiCamera, nullptr, visObject, guiSurf->material, subMesh, 0);

        guiCamera->numAmbientSurfs++;
    }

    renderSystem.CmdDrawCamera(guiCamera);
}

BE_NAMESPACE_END
