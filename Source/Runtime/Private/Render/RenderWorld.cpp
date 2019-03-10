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

    distantEnvProbe = nullptr;
}

RenderWorld::~RenderWorld() {
    ClearScene();
}

void RenderWorld::ClearScene() {
    objectDbvt.Purge();
    lightDbvt.Purge();
    staticMeshDbvt.Purge();
    probeDbvt.Purge();

    for (int i = 0; i < renderObjects.Count(); i++) {
        SAFE_DELETE(renderObjects[i]);
    }
    for (int i = 0; i < renderLights.Count(); i++) {
        SAFE_DELETE(renderLights[i]);
    }
    for (int i = 0; i < envProbes.Count(); i++) {
        SAFE_DELETE(envProbes[i]);
    }

    distantEnvProbe = nullptr;
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
        renderObject = new RenderObject(this, handle);
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
        renderLight = new RenderLight(this, handle);
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

EnvProbe *RenderWorld::GetEnvProbe(int handle) const {
    if (handle < 0 || handle >= envProbes.Count()) {
        BE_WARNLOG("RenderWorld::GetEnvProbe: handle %i > %i\n", handle, envProbes.Count() - 1);
        return nullptr;
    }

    EnvProbe *envProbe = envProbes[handle];
    if (!envProbe) {
        BE_WARNLOG("RenderWorld::GetEnvProbe: handle %i is nullptr\n", handle);
        return nullptr;
    }

    return envProbe;
}

int RenderWorld::AddEnvProbe(const EnvProbe::State *def) {
    int handle = envProbes.FindNull();
    if (handle == -1) {
        handle = envProbes.Append(nullptr);
    }

    UpdateEnvProbe(handle, def);

    return handle;
}

void RenderWorld::UpdateEnvProbe(int handle, const EnvProbe::State *def) {
    while (handle >= envProbes.Count()) {
        envProbes.Append(nullptr);
    }

    EnvProbe *envProbe = envProbes[handle];
    if (!envProbe) {
        envProbe = new EnvProbe(handle);
        envProbes[handle] = envProbe;

        envProbe->Update(def);

        envProbe->proxy = (DbvtProxy *)Mem_ClearedAlloc(sizeof(DbvtProxy));
        envProbe->proxy->envProbe = envProbe;
        envProbe->proxy->worldAABB = envProbe->GetInfluenceAABB();
        envProbe->proxy->id = probeDbvt.CreateProxy(envProbe->proxy->worldAABB, MeterToUnit(0.0f), envProbe->proxy);
    } else {
        bool originMatch = (def->origin == envProbe->state.origin);
        bool boxExtentMatch = (def->boxExtent == envProbe->state.boxExtent);

        if (!originMatch || !boxExtentMatch) {
            envProbe->proxy->worldAABB = envProbe->proxy->envProbe->GetInfluenceAABB();
            probeDbvt.MoveProxy(envProbe->proxy->id, envProbe->proxy->worldAABB, MeterToUnit(0.5f), def->origin - envProbe->state.origin);
        }

        envProbe->Update(def);
    }
}

void RenderWorld::RemoveEnvProbe(int handle) {
    if (handle < 0 || handle >= envProbes.Count()) {
        BE_WARNLOG("RenderWorld::RemoveEnvProbe: handle %i > %i\n", handle, envProbes.Count() - 1);
        return;
    }

    EnvProbe *envProbe = envProbes[handle];
    if (!envProbe) {
        BE_WARNLOG("RenderWorld::RemoveEnvProbe: handle %i is nullptr\n", handle);
        return;
    }

    probeDbvt.DestroyProxy(envProbe->proxy->id);

    // Cancel environment probe in refreshing
    for (int i = 0; i < renderSystem.envProbeJobs.Count(); ) {
        EnvProbeJob *job = &renderSystem.envProbeJobs[i];

        if (job->GetRenderWorld() == this && job->GetEnvProbe() == envProbe) {
            renderSystem.envProbeJobs.RemoveIndex(i);
        } else {
            i++;
        }
    }

    delete envProbes[handle];
    envProbes[handle] = nullptr;
}

void RenderWorld::AddDistantEnvProbe() {
    if (distantEnvProbe) {
        BE_WARNLOG("Couldn't add distant environment probe twice.\n");
        return;
    }

    EnvProbe::State def;
    def.timeSlicing = EnvProbe::TimeSlicing::NoTimeSlicing;
    def.layerMask = 0;
    def.guid = Guid::CreateGuid();

    int handle = envProbes.FindNull();
    if (handle == -1) {
        handle = envProbes.Append(nullptr);
    }

    distantEnvProbe = new EnvProbe(handle);
    envProbes[handle] = distantEnvProbe;
    distantEnvProbe->Update(&def);
}

void RenderWorld::RemoveDistantEnvProbe() {
    if (!distantEnvProbe) {
        return;
    }

    delete envProbes[0];
    envProbes[0] = nullptr;

    distantEnvProbe = nullptr;
}

static float CalculateEnvProbeLerpValue(const AABB &objectAABB,
    float probe0IntersectVolume, float probe0Importance, const AABB &probe0InfluenceAABB,
    float probe1IntersectVolume, float probe1Importance, const AABB &probe1InfluenceAABB) {
    float objectVolume = Max(objectAABB.Volume(), 0.00001f);

    if (probe1IntersectVolume > 0.0f) {
        if (probe0Importance > probe1Importance) {
            return probe0IntersectVolume / objectVolume;
        }
        if (probe1Importance > probe0Importance) {
            return 1.0f - probe1IntersectVolume / objectVolume;
        }
        if (probe1InfluenceAABB.IsContainAABB(probe0InfluenceAABB)) {
            return probe0IntersectVolume / objectVolume;
        }
        if (probe0InfluenceAABB.IsContainAABB(probe1InfluenceAABB)) {
            return 1.0f - probe1IntersectVolume / objectVolume;
        }
        return probe0IntersectVolume / (probe0IntersectVolume + probe1IntersectVolume);
    }
    return 1.0f;
}

void RenderWorld::GetClosestProbes(const AABB &sourceAABB, EnvProbeBlending blending, Array<EnvProbeBlendInfo> &outProbes) const {
    auto addProbe = [this, &sourceAABB, &outProbes](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)probeDbvt.GetUserData(proxyId);

        AABB intersectAABB = sourceAABB.Intersect(proxy->envProbe->GetInfluenceAABB());
        if (intersectAABB.IsCleared()) {
            return true;
        }

        EnvProbeBlendInfo info;
        info.envProbe = proxy->envProbe;
        info.weight = Max(intersectAABB.Volume(), 0.00001f);
        outProbes.Append(info);
        return true;
    };

    probeDbvt.Query(sourceAABB, addProbe);

    if (outProbes.Count() == 0) {
        return;
    }

    int actualCount;

    if (outProbes.Count() >= 2) {
        outProbes.Sort([&sourceAABB](const EnvProbeBlendInfo &a, const EnvProbeBlendInfo &b) -> bool {
            const int importanceA = a.envProbe->GetImportance();
            const int importanceB = b.envProbe->GetImportance();

            // Compare importances.
            if (importanceA != importanceB) {
                return importanceA > importanceB;
            }
            // Compare intersection volumes.
            if (a.weight != b.weight) {
                return a.weight > b.weight;
            }
            Vec3 centerA = a.envProbe->GetBoxCenter();
            Vec3 centerB = b.envProbe->GetBoxCenter();
            Vec3 centerObject = sourceAABB.Center();

            return centerA.DistanceSqr(centerObject) < centerB.DistanceSqr(centerObject);
        });

        if (blending == EnvProbeBlending::Simple) {
            outProbes[0].weight = 1.0f;

            for (int i = 1; i < outProbes.Count(); i++) {
                outProbes[i].weight = 0.0f;
            }

            actualCount = 1;
        } else {
            float value = CalculateEnvProbeLerpValue(sourceAABB,
                outProbes[0].weight, outProbes[0].envProbe->GetImportance(), outProbes[0].envProbe->GetInfluenceAABB(),
                outProbes[1].weight, outProbes[1].envProbe->GetImportance(), outProbes[1].envProbe->GetInfluenceAABB());

            outProbes[0].weight = value;
            outProbes[1].weight = 1.0f - value;

            actualCount = 2;
        }
    } else {
        outProbes[0].weight = 1.0;

        actualCount = 1;
    }

    for (int i = 0; i < actualCount; i++) {
        outProbes[i].proxyAABB = outProbes[i].envProbe->GetProxyAABB();

        // Adjust probe's AABB for better box projection
        if (outProbes[i].envProbe->UseBoxProjection()) {
            outProbes[i].proxyAABB += sourceAABB;
        }
    }
}

void RenderWorld::SetSkyboxMaterial(Material *skyboxMaterial) {
    this->skyboxMaterial = skyboxMaterial;

    renderSystem.ScheduleToRefreshEnvProbe(this, distantEnvProbe->index);
}

void RenderWorld::FinishMapLoading() {
//#ifndef _DEBUG
//    int startTime = PlatformTime::Milliseconds();
//
//    objectDbvt.RebuildBottomUp();
//    staticMeshDbvt.RebuildBottomUp();
//    lightDbvt.RebuildBottomUp();
//    probeDbvt.RebuildBottomUp();
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

    // Create current camera in frame data
    currentVisCamera = (VisCamera *)frameData.ClearedAlloc(sizeof(*currentVisCamera));
    currentVisCamera->def = renderCamera;
    currentVisCamera->maxDrawSurfs = MaxViewDrawSurfs; 
    currentVisCamera->drawSurfs = (DrawSurf **)frameData.Alloc(currentVisCamera->maxDrawSurfs * sizeof(DrawSurf *));
    currentVisCamera->instanceBufferCache = (BufferCache *)frameData.ClearedAlloc(sizeof(BufferCache));

    new (&currentVisCamera->visObjects) LinkList<VisObject>();
    new (&currentVisCamera->visLights) LinkList<VisLight>();

    DrawCamera(currentVisCamera);
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

    static RenderObject renderObject(this, -1);
    new (&renderObject) RenderObject(this, -1);
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
