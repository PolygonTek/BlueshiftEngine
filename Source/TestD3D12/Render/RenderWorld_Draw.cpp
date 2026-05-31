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

void RenderWorld::AddDrawSurf(RenderFrameData *frameData, VisCamera *visCamera, VisLight *visLight, VisObject *visObject, const Texture *texture, SubMesh *subMesh, DrawSurf::Flag flags) {
    if (visCamera->numDrawSurfs + 1 > visCamera->maxDrawSurfs) {
        BE_WARNLOG("RenderWorld::AddDrawSurf: not enough available drawing surfaces\n");
        return;
    }

    assert(texture);

    DrawSurf *drawSurf = (DrawSurf *)frameData->MemAlloc(sizeof(DrawSurf));
    drawSurf->space = visObject;
    drawSurf->texture = texture;
    drawSurf->subMesh = subMesh;
    drawSurf->flags = flags;

    uint64_t visLightIndex = 0;
    uint64_t visObjectIndex = visObject->index;
    uint64_t textureIndex = texture->GetIndex() & 0xFFFF;
    uint64_t materialSort = 0;
    uint64_t subMeshIndex = subMesh->subMeshId & 0xFFFF;

    //---------------------------------------------------
    // 0xFFF0000000000000 (0~4095)  : visLight index
    // 0x000F000000000000 (0~15)    : material sort
    // 0x0000FFFF00000000 (0~65535) : subMesh index
    // 0x00000000FFFF0000 (0~65535) : texture index
    // 0x000000000000FFFF (0~65535) : visObject index
    //---------------------------------------------------
    drawSurf->sortKey = ((visLightIndex << 52) | (materialSort << 48) | (subMeshIndex << 32) | (textureIndex << 16) | visObjectIndex);

    visCamera->drawSurfs[visCamera->numDrawSurfs++] = drawSurf;
}

static void RadixSortDrawSurfs(const DrawSurf **drawSurfs, uint32_t numDrawSurfs, const DrawSurf **tempDrawSurfs) {
    constexpr uint32_t NumBuckets = 256;
    constexpr uint32_t NumPasses = 8;

    const DrawSurf **src = drawSurfs;
    const DrawSurf **dst = tempDrawSurfs;

    for (uint32_t pass = 0; pass < NumPasses; pass++) {
        const uint32_t shift = pass * 8;

        uint32_t counts[NumBuckets] = {};

        for (uint32_t i = 0; i < numDrawSurfs; i++) {
            const uint32_t bucket = (src[i]->sortKey >> shift) & 0xFF;
            counts[bucket]++;
        }

        uint32_t offsets[NumBuckets];
        uint32_t sum = 0;

        for (uint32_t i = 0; i < NumBuckets; i++) {
            offsets[i] = sum;
            sum += counts[i];
        }

        for (uint32_t i = 0; i < numDrawSurfs; i++) {
            const DrawSurf *drawSurf = src[i];
            const uint32_t bucket = (drawSurf->sortKey >> shift) & 0xFF;
            dst[offsets[bucket]++] = drawSurf;
        }

        const DrawSurf **swap = src;
        src = dst;
        dst = swap;
    }

    // 8 passes라 짝수 번 swap되므로 보통 src == drawSurfs 이지만,
    // pass 수가 바뀌어도 안전하게 유지.
    if (src != drawSurfs) {
        memcpy(drawSurfs, src, numDrawSurfs * sizeof(drawSurfs[0]));
    }
}

void RenderWorld::SortDrawSurfs(RenderFrameData *frameData, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::SortDrawSurfs");

    if (visCamera->numDrawSurfs < 128) {
        std::sort(visCamera->drawSurfs, visCamera->drawSurfs + visCamera->numDrawSurfs, [](const DrawSurf *lhs, const DrawSurf *rhs) {
            return lhs->sortKey < rhs->sortKey;
        });
        return;
    }

    const DrawSurf **tempDrawSurfs = (const DrawSurf **)frameData->MemAlloc(visCamera->numDrawSurfs * sizeof(tempDrawSurfs[0]));
    RadixSortDrawSurfs(visCamera->drawSurfs, visCamera->numDrawSurfs, tempDrawSurfs);
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

    visCamera->maxDrawSurfs = VisCamera::MaxDrawSurfs;
    visCamera->drawSurfs = (const DrawSurf **)frameData->MemAlloc(visCamera->maxDrawSurfs * sizeof(DrawSurf *));
    visCamera->renderRect = renderCamera->desc.renderRect;
    visCamera->clearMethod = RenderCameraClearMethod::Color;
    visCamera->clearColor = BE1::Color4(0, 0, 1, 1);
    visCamera->viewProjMatrix = renderCamera->viewProjMatrix;
    visCamera->worldAABB.Clear();
    visCamera->is2D = false;

    DrawCamera(frameData, renderCamera, visCamera);
}

void RenderWorld::DrawCamera(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawCamera");

#ifdef USE_DBVT
    UpdatePendingStaticMeshSurfProxies();
#endif

    // objectDbvt 에서 카메라 frustum 으로 쿼리해서 보이는 RenderObject 들을 찾아낸다.
    // 보이는 RenderObject 들로 프레임 데이터에 VisObject 들을 등록한다.
    FindVisObjects(frameData, renderCamera, visCamera);

    // staticMeshDbvt 에서 카메라 frustum 으로 쿼리해서 보이는 static mesh 들을 찾아낸다.
    // 보이는 static mesh surface 들을 visCamera->drawSurfs 에 추가한다.
    // 보이는 skinned mesh surface 들을 visCamera->drawSurfs 에 추가한다.
    AddMeshes(frameData, renderCamera, visCamera);

    // 보이는 visObjects 중에 text mesh 를 visCamera->drawSurfs 에 추가한다.
    AddTextMeshes(frameData, renderCamera, visCamera);

    // Add drawing surface of skybox.
    AddSkyBoxMeshes(frameData, renderCamera, visCamera);

    // 모든 visCamera->drawSurfs 를 소팅한다.
    SortDrawSurfs(frameData, visCamera);

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
    visObject->numTextures = renderObject->desc.textures.Count();
    for (int i = 0; i < visObject->numTextures; ++i) {
        visObject->textures[i] = frameData->RetainTexture(renderObject->desc.textures[i]);
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
        visObject->ambientVisible = true;

        visCamera->worldAABB.AddAABB(proxy->worldAABB);
        return true;
    };

    objectDbvt.QueryFrustum(renderCamera->frustum, addVisibleObjects);
#else
    // 등록된 모든 RenderObject 들을 개별 Culling 해서 프레임 데이터에 VisObject 로 등록한다.
    for (const RenderObject *renderObject : renderObjects) {
        if (renderCamera->frustum.CullAABB(renderObject->worldAABB)) {
            continue;
        }

        VisObject *visObject = RegisterVisObject(frameData, renderObject);
        if (!visObject) {
            continue;
        }

        visObject->worldMatrix = renderObject->GetWorldMatrix();
        //visObject->modelViewMatrix = renderCamera->viewMatrix * visObject->worldMatrix;
        visObject->modelViewProjMatrix = renderCamera->viewProjMatrix * visObject->worldMatrix;
        visObject->ambientVisible = true;

        visCamera->worldAABB.AddAABB(renderObject->worldAABB);
    }
#endif
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
        if (!visObject->ambientVisible) {
            continue;
        }
        if (!visObject->meshInstance) {
            continue;
        }
        if (!visObject->skeleton) {
            continue;
        }

        skinnedVisObjects[numSkinnedVisObjects++] = visObject;
        numSkinningJoints += visObject->meshInstance->NumSkinningMatrices();
    }

    UpdateSkinningJointCaches(frameData, skinnedVisObjects, numSkinnedVisObjects, numSkinningJoints);
}

void RenderWorld::AddMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddMeshes");

#ifdef USE_DBVT
    // Called for each static mesh surfaces intersecting with camera frustum.
    // Returns true if it wants to proceed next query.
    auto addStaticMeshSurfs = [this, frameData, visCamera](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)staticMeshDbvt.GetUserData(proxyId);
        const Mesh::Surface *meshSurf = proxy->staticMeshInstance->GetSurface(proxy->staticMeshSurfIndex);
        if (!meshSurf) {
            return true;
        }

        // 프록시의 RenderObject 가 VisObject 로 등록되었는지 체크
        if (!IsVisObjectRegistered(proxy->renderObject)) {
            return true;
        }

        VisObject *visObject = proxy->renderObject->visObject;
        if (!visObject->ambientVisible) {
            //return true;
        }

        // FIXME: 일단 무조건 인스턴싱을 켠다. 나중에 texture 가 아닌 material 로 변경하면 거기에 옵션을 넣자
        DrawSurf::Flag flags = DrawSurf::Flag::Visible | DrawSurf::Flag::UseInstancing;

        AddDrawSurf(frameData, visCamera, nullptr, visObject, visObject->textures[meshSurf->materialIndex], meshSurf->subMesh, flags);

        return true;
    };

    staticMeshDbvt.QueryFrustum(renderCamera->frustum, addStaticMeshSurfs);
#else
    UpdateVisibleSkinningJointCaches(frameData);

    for (int i = 0; i < frameData->NumVisObjects(); ++i) {
        VisObject *visObject = frameData->GetVisObjects() + i;
        if (!visObject->ambientVisible) {
            continue;
        }
        if (!visObject->meshInstance) {
            continue;
        }

        DrawSurf::Flag flags = DrawSurf::Flag::Visible | DrawSurf::Flag::UseInstancing;

        for (int meshSurfIndex = 0; meshSurfIndex < visObject->meshInstance->NumSurfaces(); ++meshSurfIndex) {
            const Mesh::Surface *meshSurf = visObject->meshInstance->GetSurface(meshSurfIndex);

            /*BE1::AABB worldAABB;
            worldAABB.SetFromTransformedAABBFast(meshSurf->subMesh->GetAABB(), visObject->worldMatrix);

            if (renderCamera->frustum.CullAABB(worldAABB)) {
                continue;
            }*/

            AddDrawSurf(frameData, visCamera, nullptr, visObject, visObject->textures[meshSurf->materialIndex], meshSurf->subMesh, flags);
        }
    }
#endif
}

void RenderWorld::AddTextMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddTextMeshes");

    for (int i = 0; i < frameData->NumVisObjects(); ++i) {
        VisObject *visObject = frameData->GetVisObjects() + i;
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

        for (int surfaceIndex = 0; surfaceIndex < textMesh.NumSurfaces(); surfaceIndex++) {
            const GuiMesh::Surface *guiSurf = textMesh.GetSurface(surfaceIndex);
            if (!guiSurf->numIndexes) {
                break;
            }

            SubMesh *subMesh = (SubMesh *)frameData->ClearedMemAlloc(sizeof(SubMesh));
            new (subMesh) SubMesh();

            subMesh->numIndexes = guiSurf->numIndexes;
            subMesh->numVerts = guiSurf->numVerts;
            subMesh->vertexBuffer = guiSurf->vertexBuffer;
            subMesh->indexBuffer = guiSurf->indexBuffer;

            AddDrawSurf(frameData, visCamera, nullptr, visObject, guiSurf->texture, subMesh, DrawSurf::Flag::None);
        }
    }
}

void RenderWorld::AddSkyBoxMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    if (renderCamera->GetDesc().clearMethod != RenderCameraClearMethod::Skybox) {
        return;
    }
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
    visCamera->maxDrawSurfs = guiMesh.NumSurfaces();
    visCamera->drawSurfs = (const DrawSurf **)frameData->MemAlloc(visCamera->maxDrawSurfs * sizeof(DrawSurf *));
    visCamera->renderRect = BE1::Rect(0, 0, screenWidth, screenHeight);
    visCamera->clearMethod = RenderCameraClearMethod::NoClear;
    //visCamera->time = BE1::PlatformTime::Milliseconds();
    //visCamera->orthogonal = true;
    //visCamera->zNear = 0.0f;
    //visCamera->zFar = 1.0f;
    visCamera->viewProjMatrix.SetOrthoRH(0, screenWidth, screenHeight, 0, -1.0, 1.0, true); // Y 를 뒤집는다.
    visCamera->worldAABB.Clear();
    visCamera->is2D = true;

    VisObject *visObject = frameData->AllocVisObject();
    //visObject->modelViewMatrix.SetIdentity();
    visObject->modelViewProjMatrix = visCamera->viewProjMatrix;

    guiMesh.CacheIndexes(frameData->GetThreadData(0));

    for (int surfaceIndex = 0; surfaceIndex < guiMesh.NumSurfaces(); surfaceIndex++) {
        const GuiMesh::Surface *guiSurf = guiMesh.GetSurface(surfaceIndex);
        if (!guiSurf->numIndexes) {
            break;
        }

        SubMesh *subMesh = (SubMesh *)frameData->ClearedMemAlloc(sizeof(SubMesh));
        new (subMesh) SubMesh();

        subMesh->numIndexes = guiSurf->numIndexes;
        subMesh->numVerts = guiSurf->numVerts;
        subMesh->vertexBuffer = guiSurf->vertexBuffer;
        subMesh->indexBuffer = guiSurf->indexBuffer;

        AddDrawSurf(frameData, visCamera, nullptr, visObject, guiSurf->texture, subMesh, DrawSurf::Flag::None);
    }

    guiMesh.Clear();

    frameData->CmdDrawCamera(visCamera);

    viewCount++;
}
