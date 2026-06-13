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
#include "RenderWorld.h"
#include "RenderContext.h"
#include "RenderCamera.h"
#include "RenderInternal.h"

static constexpr uint32_t MaxSkinningJointsPerTask = 1024;

static const Material *GetVisObjectMaterial(const VisObject *visObject, int materialIndex) {
    if (materialIndex >= 0 && materialIndex < visObject->numMaterials) {
        return visObject->materials[materialIndex];
    }
    return materialManager.defaultLitMaterial.get();
}

static ShaderFeatureSet BuildBaseFeatureSet(const Material *material, const SubMesh *subMesh) {
    ShaderFeatureSet featureSet;

    if (material->GetRenderingMode() == Material::RenderingMode::AlphaCutoff) {
        featureSet.EnableFeature(ShaderFeature::Perforated);
    }
    int maxVertexWeights = subMesh->MaxVertexWeights();
    if (maxVertexWeights == 8) {
        featureSet.EnableFeature(ShaderFeature::GpuSkinningWeight8);
    } else if (maxVertexWeights == 4) {
        featureSet.EnableFeature(ShaderFeature::GpuSkinningWeight4);
    } else if (maxVertexWeights == 1) {
        featureSet.EnableFeature(ShaderFeature::GpuSkinningWeight1);
    }

    return featureSet;
}

void RenderWorld::AddVisSurf(RenderFrameData *frameData, VisCamera *visCamera, VisLight *visLight, VisObject *visObject, const Material *material, SubMesh *subMesh, VisSurfFlag flags) {
    if (visCamera->numVisSurfs + 1 > visCamera->maxVisSurfs) {
        BE_WARNLOG("RenderWorld::AddVisSurf: not enough available visible surfaces\n");
        return;
    }

    assert(material);

    VisSurf *drawSurf = (VisSurf *)frameData->MemAlloc(sizeof(VisSurf));
    drawSurf->index = visCamera->numVisSurfs;
    drawSurf->space = visObject;
    drawSurf->subMesh = subMesh;
    drawSurf->material = material;
    drawSurf->baseFeatureSet = BuildBaseFeatureSet(material, subMesh);
    drawSurf->shaderMaterialIndex = frameData->RegisterShaderMaterial(material);
    drawSurf->flags = flags;

    visCamera->visSurfs[visCamera->numVisSurfs++] = drawSurf;
}

void RenderWorld::SetSkyboxMaterial(SharedPtr<Material> skyboxMaterial) {
    this->skyboxMaterial = skyboxMaterial;
}

void RenderWorld::RenderScene(const RenderCamera *renderCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::RenderScene");

    assert(BE1::Engine::IsInMainThread());

    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrontendFrameData();

    // RenderCamera 정보를 기반으로 프레임 데이터에 필요한 VisCamera 를 구성한다.
    VisCamera *visCamera = frameData->AllocVisCamera();
    if (!visCamera) {
        return;
    }

    visCamera->maxVisSurfs = VisCamera::MaxVisSurfs;
    visCamera->visSurfs = (const VisSurf **)frameData->MemAlloc(visCamera->maxVisSurfs * sizeof(VisSurf *));
    visCamera->numRenderPasses = 0;
    visCamera->renderPasses = (RenderPass *)frameData->MemAlloc(VisCamera::MaxRenderPasses * sizeof(RenderPass));
    visCamera->renderRect = renderCamera->desc.renderRect;
    visCamera->clearMethod = renderCamera->desc.clearMethod;
    visCamera->clearColorMode = renderCamera->desc.clearColorMode;
    visCamera->clearColor = renderCamera->desc.clearColor;
    visCamera->viewMatrix = renderCamera->viewMatrix;
    visCamera->viewProjMatrix = renderCamera->viewProjMatrix;
    visCamera->worldAABB.Clear();
    visCamera->is2D = false;

    DrawCamera(frameData, renderCamera, visCamera);
}

void RenderWorld::DrawCamera(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawCamera");

#ifdef USE_DBVT
    UpdatePendingObjectProxies();
    UpdateSleepingObjectProxies();
#endif

    // objectDbvt 에서 카메라 frustum 으로 쿼리해서 보이는 RenderObject 들을 찾아낸다.
    // 보이는 RenderObject 들로 프레임 데이터에 VisObject 들을 등록한다.
    FindVisObjects(frameData, renderCamera, visCamera);

    // 보이는 VisObject 에서 mesh surface 들을 visCamera->visSurfs 에 추가한다.
    AddMeshes(frameData, renderCamera, visCamera);

    // 보이는 visObjects 중에 raw mesh 를 visCamera->visSurfs 에 추가한다.
    //AddRawMeshes(frameData, renderCamera, visCamera);

    // 보이는 visObjects 중에 text mesh 를 visCamera->visSurfs 에 추가한다.
    AddTextMeshes(frameData, renderCamera, visCamera);

    // Add drawing surface of skybox.
    AddSkyBoxMeshes(frameData, renderCamera, visCamera);

    // 렌더 패스를 구성한다.
    renderPassBuilder.BuildMain(frameData, visCamera);

    // 렌더링 커맨드에 visCamera 를 기록한다.
    frameData->CmdDrawCamera(visCamera);

    viewCount++;
}
bool RenderWorld::IsVisObjectRegistered(const RenderObject *renderObject) const {
    // 현재 프레임에 이미 등록된 VisObject 인지 체크
    return renderObject->viewCount == viewCount && renderObject->visObject;
}

VisObject *RenderWorld::RegisterVisObject(RenderFrameData *frameData, const RenderObject *renderObject) {
    if (IsVisObjectRegistered(renderObject)) {
        return nullptr;
    }

    VisObject *visObject = frameData->AllocVisObject();
    if (!visObject) {
        return nullptr;
    }
    renderObject->visObject = visObject;
    renderObject->viewCount = viewCount;

    if (renderObject->desc.meshInstance) {
        visObject->meshInstance = frameData->RetainMeshInstance(renderObject->desc.meshInstance);
    }
    assert(renderObject->desc.materials.Count() <= COUNT_OF(visObject->materials));
    visObject->numMaterials = renderObject->desc.materials.Count();
    for (int i = 0; i < visObject->numMaterials; ++i) {
        visObject->materials[i] = frameData->RetainMaterial(renderObject->desc.materials[i]);
    }
    if (renderObject->desc.numVerts > 0 && renderObject->desc.numIndexes > 0) {
        visObject->numVerts = renderObject->desc.numVerts;
        visObject->verts = renderObject->desc.verts;
        visObject->numIndexes = renderObject->desc.numIndexes;
        visObject->indexes = renderObject->desc.indexes;
    }
    if (renderObject->desc.text) {
        RenderObject::TextDesc &textDesc = *(renderObject->desc.text);
        if (textDesc.font) {
            visObject->font = frameData->RetainFont(textDesc.font);
        }
        if (!textDesc.text.IsEmpty()) {
            int size = textDesc.text.Length() + 1;
            char *textMemPtr = (char *)frameData->MemAlloc(size);
            BE1::Str::Copynz(textMemPtr, textDesc.text.c_str(), size);
            visObject->text = textMemPtr;
            visObject->textRect = textDesc.textRect;
            visObject->textShadowOffset = textDesc.textShadowOffset;
            visObject->textShadowColor = textDesc.textShadowColor;
            visObject->textParams = textDesc.textParams;
            visObject->textScale = textDesc.textScale;
            visObject->textLineSpacing = textDesc.textLineSpacing;
        }
    }

    return visObject;
}

void RenderWorld::FindVisObjects(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindVisObjects");

    const uint32_t startVisibleObjectCount = frameData->NumVisObjects();

    visCamera->worldAABB.Clear();

#ifdef USE_DBVT
    // Frustum 에 교차된 RenderObject 들을 추려내서 프레임 데이터에 VisObject 로 등록한다.
    // 쿼리 콜백 함수의 리턴값이 true 라면, 다음 쿼리를 진행한다.
    auto addVisibleObjects = [this, frameData, renderCamera, visCamera](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)objectDbvt.GetUserData(proxyId);
        const RenderObject *renderObject = proxy->renderObject;
        if (!renderObject) {
            return true;
        }

        VisObject *visObject = RegisterVisObject(frameData, renderObject);
        if (!visObject) {
            return true;
        }

        visObject->worldMatrix = renderObject->GetWorldMatrix();
        visObject->cameraVisible = true;

        visCamera->worldAABB.AddAABB(proxy->worldAABB);
        return true;
    };

    objectDbvt.QueryFrustum(renderCamera->frustum, addVisibleObjects);

    for (int objectIndex : dynamicObjectIndices) {
        const RenderObject *renderObject = renderObjects[objectIndex];

        if (renderCamera->frustum.CullAABB(renderObject->worldAABB)) {
            continue;
        }

        VisObject *visObject = RegisterVisObject(frameData, renderObject);
        if (!visObject) {
            continue;
        }

        visObject->worldMatrix = renderObject->GetWorldMatrix();
        visObject->cameraVisible = true;

        visCamera->worldAABB.AddAABB(renderObject->worldAABB);
    }
#else
    // 등록된 모든 RenderObject 들을 개별 Culling 해서 프레임 데이터에 VisObject 로 등록한다.
    for (const RenderObject *renderObject : renderObjects) {
        if (!renderObject) {
            continue;
        }

        if (renderCamera->frustum.CullAABB(renderObject->worldAABB)) {
            continue;
        }

        VisObject *visObject = RegisterVisObject(frameData, renderObject);
        if (!visObject) {
            continue;
        }

        visObject->worldMatrix = renderObject->GetWorldMatrix();
        visObject->cameraVisible = true;

        visCamera->worldAABB.AddAABB(renderObject->worldAABB);
    }
#endif

    frameData->AddObjectStats(static_cast<uint32_t>(renderObjects.Count()), frameData->NumVisObjects() - startVisibleObjectCount);
}

void RenderWorld::UpdateSkinningJointCaches(RenderFrameData *frameData, VisObject **visObjects, uint32_t numVisObjects, uint32_t numSkinningJoints) {
    if (numVisObjects == 0 || numSkinningJoints == 0) {
        return;
    }

#ifdef USE_RENDER_TASK
    uint32_t numTasks = (numSkinningJoints + MaxSkinningJointsPerTask - 1) / MaxSkinningJointsPerTask;
    numTasks = BE1::Min(BE1::Engine::taskManager->NumThreads(), numTasks);

    if (numTasks > 1) {
        UpdateSkinningJointCachesWithTask(frameData, visObjects, numVisObjects, numSkinningJoints, numTasks);
    } else {
        UpdateSkinningJointCachesWithoutTask(frameData, visObjects, numVisObjects);
    }
#else
    UpdateSkinningJointCachesWithoutTask(frameData, visObjects, numVisObjects);
#endif
}

void RenderWorld::UpdateSkinningJointCachesWithoutTask(RenderFrameData *frameData, VisObject **visObjects, uint32_t numVisObjects) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesWithoutTask");

    RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);

    for (uint32_t visObjectIndex = 0; visObjectIndex < numVisObjects; ++visObjectIndex) {
        VisObject *visObject = visObjects[visObjectIndex];
        visObject->meshInstance->UpdateSkinningJointCache(frameThreadData, visObject->skeleton);
    }
}

void RenderWorld::UpdateSkinningJointCachesByTask(RenderWorld::SkinningJointCacheTaskDesc *taskDesc) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesByTask");

    RHI::FrameThreadData *frameThreadData = taskDesc->frameData->GetThreadData(taskDesc->threadIndex);

    for (uint32_t visObjectIndex = 0; visObjectIndex < taskDesc->numVisObjects; ++visObjectIndex) {
        VisObject *visObject = taskDesc->visObjects[visObjectIndex];
        visObject->meshInstance->UpdateSkinningJointCache(frameThreadData, visObject->skeleton);
    }
}

void RenderWorld::UpdateSkinningJointCachesByTaskFunction(void *data) {
    RenderWorld::SkinningJointCacheTaskDesc *taskDesc = reinterpret_cast<RenderWorld::SkinningJointCacheTaskDesc *>(data);
    taskDesc->renderWorld->UpdateSkinningJointCachesByTask(taskDesc);
}

void RenderWorld::UpdateSkinningJointCachesWithTask(RenderFrameData *frameData, VisObject **visObjects, uint32_t numVisObjects, uint32_t numSkinningJoints, uint32_t numTasks) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::UpdateSkinningJointCachesWithTask");

    uint32_t targetJointsPerTask = (numSkinningJoints + numTasks - 1) / numTasks;
    uint32_t startIndex = 0;

    skinningJointCacheTaskDescs.Reserve(numTasks);
    skinningJointCacheTaskDescs.SetCount(0, false);

    while (startIndex < numVisObjects && skinningJointCacheTaskDescs.Count() + 1 < (int)numTasks) {
        uint32_t endIndex = startIndex;
        uint32_t numTaskJoints = 0;

        do {
            VisObject *visObject = visObjects[endIndex];
            numTaskJoints += BE1::Max(1, visObject->meshInstance->NumSkinningMatrices());
            endIndex++;
        } while (endIndex < numVisObjects && numTaskJoints < targetJointsPerTask);

        SkinningJointCacheTaskDesc &taskDesc = skinningJointCacheTaskDescs.Alloc();
        taskDesc.threadIndex = skinningJointCacheTaskDescs.Count() - 1;
        taskDesc.renderWorld = this;
        taskDesc.frameData = frameData;
        taskDesc.visObjects = &visObjects[startIndex];
        taskDesc.numVisObjects = endIndex - startIndex;

        startIndex = endIndex;
    }

    if (startIndex < numVisObjects) {
        SkinningJointCacheTaskDesc &taskDesc = skinningJointCacheTaskDescs.Alloc();
        taskDesc.threadIndex = skinningJointCacheTaskDescs.Count() - 1;
        taskDesc.renderWorld = this;
        taskDesc.frameData = frameData;
        taskDesc.visObjects = &visObjects[startIndex];
        taskDesc.numVisObjects = numVisObjects - startIndex;
    }

    for (int taskIndex = 0; taskIndex < skinningJointCacheTaskDescs.Count(); ++taskIndex) {
        BE1::Engine::taskManager->AddTask(RenderWorld::UpdateSkinningJointCachesByTaskFunction, &skinningJointCacheTaskDescs[taskIndex], renderWorldGroupId, false);
    }

    BE1::Engine::taskManager->WaitFinish(renderWorldGroupId, true);
}

void RenderWorld::UpdateVisibleSkinningJointCaches(RenderFrameData *frameData) {
    const uint32_t numVisObjects = frameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

    uint32_t numSkinnedVisObjects = 0;
    uint32_t numSkinningJoints = 0;
    VisObject **skinnedVisObjects = (VisObject **)_alloca(numVisObjects * sizeof(skinnedVisObjects[0]));

    for (uint32_t visObjectIndex = 0; visObjectIndex < numVisObjects; ++visObjectIndex) {
        VisObject *visObject = frameData->GetVisObjects() + visObjectIndex;
        if (!visObject->cameraVisible) {
            continue;
        }
        if (!visObject->skeleton) {
            continue;
        }
        if (!visObject->meshInstance) {
            continue;
        }
        if (!visObject->meshInstance->EnsureCurrent()) {
            continue;
        }

        skinnedVisObjects[numSkinnedVisObjects++] = visObject;
        numSkinningJoints += visObject->meshInstance->NumSkinningMatrices();
    }

    UpdateSkinningJointCaches(frameData, skinnedVisObjects, numSkinnedVisObjects, numSkinningJoints);
}

void RenderWorld::AddMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddMeshes");

    UpdateVisibleSkinningJointCaches(frameData);

    const int numVisObjects = frameData->NumVisObjects();
    for (int i = 0; i < numVisObjects; ++i) {
        VisObject *visObject = frameData->GetVisObjects() + i;
        if (!visObject->cameraVisible) {
            continue;
        }
        if (!visObject->meshInstance) {
            continue;
        }
        if (!visObject->meshInstance->EnsureCurrent()) {
            continue;
        }

        VisSurfFlag flags = VisSurfFlag::CameraVisible;
        const int numSurfaces = visObject->meshInstance->NumSurfaces();
        const bool cullSurfaces = numSurfaces > 1;

        for (int meshSurfIndex = 0; meshSurfIndex < numSurfaces; ++meshSurfIndex) {
            const Mesh::Surface *meshSurf = visObject->meshInstance->GetSurface(meshSurfIndex);
            if (!meshSurf || !meshSurf->subMesh) {
                continue;
            }

            if (cullSurfaces) {
                const BE1::AABB &localAABB = meshSurf->subMesh->GetAABB();
                if (!localAABB.IsCleared()) {
                    BE1::AABB worldAABB;
                    worldAABB.SetFromTransformedAABBFast(localAABB, visObject->worldMatrix);

                    if (renderCamera->frustum.CullAABB(worldAABB)) {
                        continue;
                    }
                }
            }
            AddVisSurf(frameData, visCamera, nullptr, visObject, GetVisObjectMaterial(visObject, meshSurf->materialIndex), meshSurf->subMesh, flags);
        }
    }
}

void RenderWorld::AddTextMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddTextMeshes");

    const int numVisObjects = frameData->NumVisObjects();
    for (int i = 0; i < numVisObjects; ++i) {
        VisObject *visObject = frameData->GetVisObjects() + i;
        if (!visObject->cameraVisible) {
            continue;
        }
        if (!visObject->text) {
            continue;
        }

        textMesh.Clear();

        if (visObject->textRect.IsEmpty()) {
            textMesh.DrawText3D(frameData->GetThreadData(0), (RenderObject::TextDrawMode)visObject->textParams.bits.textDrawMode,
                (RenderObject::TextAnchor)visObject->textParams.bits.textAnchor, (RenderObject::TextHorzAlignment)visObject->textParams.bits.textHorzAlignment,
                visObject->textLineSpacing, visObject->textScale, BE1::Color4::white.ToUInt32(), visObject->textShadowColor, visObject->textShadowOffset, visObject->font, visObject->text);
        } else {
            textMesh.DrawText3D(frameData->GetThreadData(0), (RenderObject::TextDrawMode)visObject->textParams.bits.textDrawMode, visObject->textRect,
                (RenderObject::TextHorzAlignment)visObject->textParams.bits.textHorzAlignment, (RenderObject::TextVertAlignment)visObject->textParams.bits.textVertAlignment,
                (RenderObject::TextHorzOverflow)visObject->textParams.bits.textHorzOverflow, (RenderObject::TextVertOverflow)visObject->textParams.bits.textVertOverflow,
                visObject->textLineSpacing, visObject->textScale, BE1::Color4::white.ToUInt32(), visObject->textShadowColor, visObject->textShadowOffset, visObject->font, visObject->text);
        }

        textMesh.CacheIndexes(frameData->GetThreadData(0));

        const int numSurfaces = textMesh.NumSurfaces();
        for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
            const GuiMesh::Surface *guiSurf = textMesh.GetSurface(surfaceIndex);
            if (!guiSurf->numIndexes) {
                break;
            }

            SubMesh *subMesh = (SubMesh *)frameData->MemAlloc(sizeof(SubMesh));
            new (subMesh) SubMesh();

            subMesh->numIndexes = guiSurf->numIndexes;
            subMesh->numVerts = guiSurf->numVerts;
            subMesh->vertexBuffer = guiSurf->vertexBuffer;
            subMesh->indexBuffer = guiSurf->indexBuffer;

            AddVisSurf(frameData, visCamera, nullptr, visObject, guiSurf->material, subMesh, VisSurfFlag::None);
        }
    }
}

void RenderWorld::AddRawMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddRawMeshes");

    const int numVisObjects = frameData->NumVisObjects();
    for (int i = 0; i < numVisObjects; ++i) {
        VisObject *visObject = frameData->GetVisObjects() + i;
        if (!visObject->cameraVisible) {
            continue;
        }
        if (visObject->numVerts <= 0 || visObject->numIndexes <= 0) {
            continue;
        }

        RHI::FrameThreadData *frameThreadData = frameData->GetThreadData(0);
        RHI::VertexBuffer *vertexBuffer = frameThreadData->AllocVertex(sizeof(visObject->verts[0]), visObject->numVerts, visObject->verts);
        RHI::IndexBuffer *indexBuffer = frameThreadData->AllocIndex(sizeof(visObject->indexes[0]), visObject->numIndexes, visObject->indexes);
        if (!vertexBuffer || !indexBuffer) {
            continue;
        }

        // Copy this SubMesh to the temporary frame data to use in the backend.
        SubMesh *subMesh = (SubMesh *)frameData->MemAlloc(sizeof(SubMesh));
        new (subMesh) SubMesh();

        subMesh->numVerts = visObject->numVerts;
        subMesh->numIndexes = visObject->numIndexes;
        subMesh->vertexBuffer = vertexBuffer;
        subMesh->indexBuffer = indexBuffer;

        AddVisSurf(frameData, visCamera, nullptr, visObject, GetVisObjectMaterial(visObject, 0), subMesh, VisSurfFlag::None);
    }
}

void RenderWorld::AddSkyBoxMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddSkyBoxMeshes");

    if (renderCamera->GetDesc().clearMethod != RenderCameraClearMethod::Skybox) {
        return;
    }
    if (!skyboxMaterial || !meshManager.defaultBoxMesh || !meshManager.defaultBoxMesh->IsReady()) {
        return;
    }

    const Mesh::Surface *meshSurf = meshManager.defaultBoxMesh->GetSurface(0);
    if (!meshSurf || !meshSurf->subMesh) {
        return;
    }

    VisObject *visObject = frameData->AllocVisObject();
    if (!visObject) {
        return;
    }

    visObject->worldMatrix.SetTRS(renderCamera->GetDesc().origin, BE1::Mat3::identity, BE1::Vec3(10, 10, 10));
    visObject->cameraVisible = true;
    Material *material = frameData->RetainMaterial(skyboxMaterial);

    AddVisSurf(frameData, visCamera, nullptr, visObject, material, meshSurf->subMesh, VisSurfFlag::CameraVisible | VisSurfFlag::SkipSelection);
}

void RenderWorld::RenderGUI(GuiMesh &guiMesh) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::RenderGUI");

    assert(BE1::Engine::IsInMainThread());

    if (guiMesh.NumSurfaces() == 0) {
        return;
    }

    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrontendFrameData();

    uint32_t screenWidth = RenderContext::activeContext->GetWidth();
    uint32_t screenHeight = RenderContext::activeContext->GetHeight();

    // guiMesh 를 기반으로 GUI VisCamera 를 구성하여, 렌더링 커맨드에 DrawCamera 명령을 기록한다.
    DrawGUICamera(frameData, screenWidth, screenHeight, guiMesh);
}

void RenderWorld::DrawGUICamera(RenderFrameData *frameData, uint32_t screenWidth, uint32_t screenHeight, GuiMesh &guiMesh) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawGUICamera");

    VisCamera *visCamera = frameData->AllocVisCamera();
    visCamera->maxVisSurfs = guiMesh.NumSurfaces();
    visCamera->visSurfs = (const VisSurf **)frameData->MemAlloc(visCamera->maxVisSurfs * sizeof(VisSurf *));
    visCamera->numRenderPasses = 0;
    visCamera->renderPasses = (RenderPass *)frameData->MemAlloc(VisCamera::MaxRenderPasses * sizeof(RenderPass));
    visCamera->renderRect = BE1::Rect(0, 0, screenWidth, screenHeight);
    visCamera->clearMethod = RenderCameraClearMethod::NoClear;
    //visCamera->time = BE1::PlatformTime::Milliseconds();
    //visCamera->orthogonal = true;
    //visCamera->zNear = 0.0f;
    //visCamera->zFar = 1.0f;
    visCamera->viewMatrix = BE1::Mat4::identity;
    visCamera->viewProjMatrix.SetOrthoRH(0, screenWidth, screenHeight, 0, -1.0, 1.0, true); // Y 를 뒤집는다.
    visCamera->worldAABB.Clear();
    visCamera->is2D = true;

    VisObject *visObject = frameData->AllocVisObject();
    visObject->worldMatrix.SetIdentity();

    guiMesh.CacheIndexes(frameData->GetThreadData(0));

    const int numSurfaces = guiMesh.NumSurfaces();
    for (int surfaceIndex = 0; surfaceIndex < numSurfaces; surfaceIndex++) {
        const GuiMesh::Surface *guiSurf = guiMesh.GetSurface(surfaceIndex);
        if (!guiSurf->numIndexes) {
            break;
        }

        SubMesh *subMesh = (SubMesh *)frameData->MemAlloc(sizeof(SubMesh));
        new (subMesh) SubMesh();

        subMesh->numIndexes = guiSurf->numIndexes;
        subMesh->numVerts = guiSurf->numVerts;
        subMesh->vertexBuffer = guiSurf->vertexBuffer;
        subMesh->indexBuffer = guiSurf->indexBuffer;

        AddVisSurf(frameData, visCamera, nullptr, visObject, guiSurf->material, subMesh, VisSurfFlag::None);
    }

    guiMesh.Clear();

    renderPassBuilder.BuildGUI(frameData, visCamera);

    frameData->CmdDrawCamera(visCamera);

    viewCount++;
}
