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
#include "RHI.h"
#include "RenderWorld.h"
#include "RenderContext.h"
#include "RenderCamera.h"
#include "RenderInternal.h"

RenderWorld::RenderWorld() {
    renderObjects.Reserve(16384);
    renderObjects.SetGranularity(4096);
}

void RenderWorld::ClearScene() {
#ifdef USE_DBVT
    objectDbvt.Clear();
    staticMeshDbvt.Clear();
#endif
    
    for (RenderObject *renderObject : renderObjects) {
        SAFE_DELETE(renderObject);
    }
}

RenderObject *RenderWorld::GetRenderObject(int index) const {
    if (index < 0 || index >= renderObjects.Count()) {
        BE_WARNLOG("RenderWorld::GetRenderObject: index %i > %i\n", index, renderObjects.Count() - 1);
        return nullptr;
    }

    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        BE_WARNLOG("RenderWorld::GetRenderObject: index %i is nullptr\n", index);
        return nullptr;
    }

    return renderObject;
}

int RenderWorld::AddRenderObject(const RenderObjectDesc &desc) {
    assert(BE1::Engine::IsInMainThread());

    int index = renderObjects.FindNull();
    if (index == -1) {
        index = renderObjects.Append(nullptr);
    }

    UpdateRenderObject(index, desc);
    return index;
}

void RenderWorld::UpdateRenderObject(int index, const RenderObjectDesc &desc) {
    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        renderObject = new RenderObject;
        renderObject->index = index;
        renderObjects[index] = renderObject;
        renderObject->Update(desc);

#ifdef USE_DBVT
        // Add proxy node in the DBVT for the renderObjects
        renderObject->proxy = (DbvtProxy *)Mem_Alloc(sizeof(DbvtProxy));
        renderObject->proxy->renderObject = renderObject;
        renderObject->proxy->staticMesh = nullptr;
        renderObject->proxy->worldAABB = renderObject->GetWorldAABB();
        renderObject->proxy->staticMeshSurfIndex = -1;
        renderObject->proxy->id = objectDbvt.CreateProxy(renderObject->proxy->worldAABB, BE1::MeterToUnit(0.0f), renderObject->proxy);

        if (desc.mesh && desc.mesh->IsStaticMesh()) {
            renderObject->numMeshSurfProxies = desc.mesh->NumSurfaces();
            renderObject->meshSurfProxies = (DbvtProxy *)Mem_Alloc(desc.mesh->NumSurfaces() * sizeof(DbvtProxy));

            for (int surfaceIndex = 0; surfaceIndex < desc.mesh->NumSurfaces(); ++surfaceIndex) {
                const Mesh::Surface *meshSurf = desc.mesh->GetSurface(surfaceIndex);

                DbvtProxy *meshSurfProxy = &renderObject->meshSurfProxies[surfaceIndex];
                meshSurfProxy->renderObject = renderObject;
                meshSurfProxy->staticMesh = desc.mesh;
                meshSurfProxy->worldAABB.SetFromTransformedAABBFast(meshSurf->subMesh->GetAABB(), desc.worldMatrix);
                meshSurfProxy->staticMeshSurfIndex = surfaceIndex;
                meshSurfProxy->id = staticMeshDbvt.CreateProxy(renderObject->meshSurfProxies[surfaceIndex].worldAABB, BE1::MeterToUnit(0.0f), &renderObject->meshSurfProxies[surfaceIndex]);
            }
        }
#endif
    } else {
#ifdef USE_DBVT
        const bool worldMatrixMatch = (desc.worldMatrix == renderObject->desc.worldMatrix);
        const bool aabbMatch = (desc.aabb == renderObject->desc.aabb);
        const bool meshMatch = (desc.mesh == renderObject->desc.mesh);
        const bool proxyMoved = !worldMatrixMatch || !aabbMatch;

        BE1::Vec3 displacementVector;
        if (proxyMoved) {
            displacementVector = desc.worldMatrix.ToTranslationVec3() - renderObject->desc.worldMatrix.ToTranslationVec3();

            renderObject->proxy->worldAABB.SetFromTransformedAABBFast(desc.aabb, desc.worldMatrix);
            objectDbvt.MoveProxy(renderObject->proxy->id, renderObject->proxy->worldAABB, BE1::MeterToUnit(0.5f), displacementVector);
        }

        if (proxyMoved || !meshMatch) {
            // If this object is a static mesh
            if (renderObject->desc.mesh && renderObject->desc.mesh->IsStaticMesh()) {
                // mesh surface count changed so we recreate static proxies
                if (desc.mesh->NumSurfaces() != renderObject->numMeshSurfProxies) {
                    Mem_Free(renderObject->meshSurfProxies);

                    renderObject->numMeshSurfProxies = desc.mesh->NumSurfaces();
                    renderObject->meshSurfProxies = (DbvtProxy *)Mem_ClearedAlloc(renderObject->numMeshSurfProxies * sizeof(DbvtProxy));

                    for (int surfaceIndex = 0; surfaceIndex < desc.mesh->NumSurfaces(); surfaceIndex++) {
                        const Mesh::Surface *meshSurf = desc.mesh->GetSurface(surfaceIndex);

                        staticMeshDbvt.DestroyProxy(renderObject->meshSurfProxies[surfaceIndex].id);

                        DbvtProxy *meshSurfProxy = &renderObject->meshSurfProxies[surfaceIndex];
                        meshSurfProxy->renderObject = renderObject;
                        meshSurfProxy->staticMesh = desc.mesh;
                        meshSurfProxy->worldAABB.SetFromTransformedAABBFast(meshSurf->subMesh->GetAABB(), desc.worldMatrix);
                        meshSurfProxy->staticMeshSurfIndex = surfaceIndex;
                        meshSurfProxy->id = staticMeshDbvt.CreateProxy(renderObject->meshSurfProxies[surfaceIndex].worldAABB, BE1::MeterToUnit(0.0f), &renderObject->meshSurfProxies[surfaceIndex]);
                    }
                } else {
                    if (proxyMoved) {
                        for (int surfaceIndex = 0; surfaceIndex < desc.mesh->NumSurfaces(); surfaceIndex++) {
                            renderObject->meshSurfProxies[surfaceIndex].worldAABB.SetFromTransformedAABBFast(desc.mesh->GetSurface(surfaceIndex)->subMesh->GetAABB(), desc.worldMatrix);
                            staticMeshDbvt.MoveProxy(renderObject->meshSurfProxies[surfaceIndex].id, renderObject->meshSurfProxies[surfaceIndex].worldAABB, BE1::MeterToUnit(0.5f), displacementVector);
                        }
                    }
                }
            }
        }
#endif
        renderObject->Update(desc);
    }
}

void RenderWorld::RemoveRenderObject(int index) {
    assert(BE1::Engine::IsInMainThread());

    if (!renderObjects.IsValidIndex(index)) {
        BE_WARNLOG("RenderWorld::RemoveRenderObject: invalid index %i\n", index);
        return;
    }

    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        BE_WARNLOG("RenderWorld::RemoveRenderObject: index %i is nullptr\n", index);
        return;
    }

#ifdef USE_DBVT
    objectDbvt.DestroyProxy(renderObject->proxy->id);

    for (int i = 0; i < renderObject->numMeshSurfProxies; i++) {
        staticMeshDbvt.DestroyProxy(renderObject->meshSurfProxies[i].id);
    }
#endif

    delete renderObjects[index];
    renderObjects[index] = nullptr;
}

void RenderWorld::AddDrawSurf(RenderFrameData *frameData, VisCamera *visCamera, VisLight *visLight, VisObject *visObject, const Texture *texture, SubMesh *subMesh, DrawSurf::Flag flags) {
    if (visCamera->numDrawSurfs + 1 > visCamera->maxDrawSurfs) {
        BE_WARNLOG("RenderWorld::AddDrawSurf: not enough available drawing surfaces\n");
        return;
    }

    DrawSurf *drawSurf = (DrawSurf *)frameData->ClearedMemAlloc(sizeof(DrawSurf));
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

static int BE_CDECL _CompareDrawSurf(const void *elem1, const void *elem2) {
    const uint64_t sortKey1 = (*(DrawSurf **)elem1)->sortKey;
    const uint64_t sortKey2 = (*(DrawSurf **)elem2)->sortKey;

    if (sortKey1 < sortKey2) {
        return -1;
    }
    if (sortKey1 > sortKey2) {
        return 1;
    }
    return 0;
}

void RenderWorld::SortDrawSurfs(VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::SortDrawSurfs", 1);

    // FIXME: Use radix sort to boost up speed.
    qsort(visCamera->drawSurfs, visCamera->numDrawSurfs, sizeof(DrawSurf *), _CompareDrawSurf);
}

void RenderWorld::RenderScene(const RenderCamera *renderCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::RenderScene", 1);

    assert(BE1::Engine::IsInMainThread());

    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrameData();

    // 렌더링 백엔드가 사용할 내부 카메라 정보를 세팅한다.
    VisCamera *visCamera = frameData->AllocVisCamera();
    if (!visCamera) {
        return;
    }

    visCamera->clearMethod = RenderCameraClearMethod::Color;
    visCamera->clearColor = BE1::Color4(0, 0, 1, 1);
    visCamera->maxDrawSurfs = VisCamera::MaxDrawSurfs;
    visCamera->drawSurfs = (const DrawSurf **)frameData->MemAlloc(visCamera->maxDrawSurfs * sizeof(DrawSurf *));
    visCamera->renderRect = renderCamera->desc.renderRect;
    visCamera->viewProjMatrix = renderCamera->viewProjMatrix;
    visCamera->worldAABB.Clear();

    // RenderCamera 정보를 기반으로 프레임 데이터에 필요한 VisCamera 를 구성하여, 렌더링 커맨드에 DrawCamera 명령을 기록한다.
    DrawCamera(frameData, renderCamera, visCamera);
}

void RenderWorld::DrawCamera(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawCamera", 1);

    viewCount++;

    // 카메라에 VisObject 의 시작 인덱스 기록
    visCamera->visObjectStartIndex = frameData->NumVisObjects();

    // objectDbvt 에서 카메라 frustum 으로 쿼리해서 보이는 RenderObject 들을 찾아낸다.
    // 보이는 RenderObject 들로 프레임 데이터에 VisObject 를 등록한다.
    FindVisObjects(frameData, renderCamera, visCamera);

    // 카메라에 VisObject 의 끝 인덱스 기록
    visCamera->visObjectEndIndex = frameData->NumVisObjects() - 1;

    // staticMeshDbvt 에서 카메라 frustum 으로 쿼리해서 보이는 static mesh 들을 찾아낸다.
    // 보이는 static mesh surface 들을 visCamera->drawSurfs 에 추가한다.
    AddStaticMeshes(frameData, renderCamera, visCamera);

    // 모든 visCamera->drawSurfs 를 소팅한다.
    SortDrawSurfs(visCamera);

    // 렌더링 커맨드에 visCamera 를 기록한다.
    frameData->CmdDrawCamera(visCamera);
}

void RenderWorld::RenderGUI() {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::RenderGUI", 2);

    assert(BE1::Engine::IsInMainThread());

    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrameData();

    // guiMesh 를 기반으로 GUI VisCamera 를 구성하여, 렌더링 커맨드에 DrawCamera 명령을 기록한다.
    DrawGUICamera(frameData, RenderContext::activeContext->GetWidth(), RenderContext::activeContext->GetHeight());
}

void RenderWorld::DrawGUICamera(RenderFrameData *frameData, uint32_t screenWidth, uint32_t screenHeight) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawGUICamera", 1);

    VisCamera *visCamera = frameData->AllocVisCamera();
    visCamera->renderRect = BE1::Rect(0, 0, screenWidth, screenHeight);
    visCamera->clearMethod = RenderCameraClearMethod::NoClear;
    //visCamera->time = BE1::PlatformTime::Milliseconds();
    //visCamera->orthogonal = true;
    //visCamera->zNear = 0.0f;
    //visCamera->zFar = 1.0f;
    visCamera->viewProjMatrix.SetOrthoRH(0, screenWidth, screenHeight, 0, -1.0, 1.0, true);
    visCamera->worldAABB.Clear();
    visCamera->is2D = true;

    frameData->CmdDrawCamera(visCamera);
}

void RenderWorld::FindVisObjects(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindVisObjects", 4);

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

        // 보이는 RenderObject 로 VisObject 를 등록
        VisObject *visObject = frameData->AllocVisObject();
        if (!visObject) {
            return true;
        }

        renderObject->viewCount = viewCount;
        renderObject->visObject = visObject;

        visObject->mesh = renderObject->desc.mesh;
        visObject->mesh->AddRefCount();
        visObject->textures = renderObject->desc.textures;
        for (Texture *texture : visObject->textures) {
            texture->AddRefCount();
        }
        visObject->worldMatrix = renderObject->GetWorldMatrix();
        visObject->modelViewMatrix = renderCamera->viewMatrix * renderObject->GetWorldMatrix();
        visObject->modelViewProjMatrix = renderCamera->viewProjMatrix * renderObject->GetWorldMatrix();
        visObject->ambientVisible = true;

        visCamera->worldAABB.AddAABB(proxy->worldAABB);
        return true;
    };

    objectDbvt.QueryFrustum(renderCamera->frustum, addVisibleObjects);
#else
    for (int i = 0; i < renderObjects.Count(); ++i) {
        const RenderObject *renderObject = renderObjects[i];

        if (renderCamera->frustum.CullAABB(renderObject->worldAABB)) {
            continue;
        }

        VisObject *visObject = frameData->AllocVisObject();
        if (!visObject) {
            continue;
        }

        renderObject->viewCount = viewCount;
        renderObject->visObject = visObject;

        visObject->mesh = renderObject->desc.mesh;
        visObject->mesh->AddRefCount();
        visObject->textures = renderObject->desc.textures;
        for (Texture *texture : visObject->textures) {
            texture->AddRefCount();
        }
        visObject->modelViewMatrix = renderCamera->viewMatrix * renderObject->GetWorldMatrix();
        visObject->modelViewProjMatrix = renderCamera->viewProjMatrix * renderObject->GetWorldMatrix();
        visObject->ambientVisible = true;

        visCamera->worldAABB.AddAABB(renderObject->worldAABB);
    }
#endif
}

void RenderWorld::AddStaticMeshes(RenderFrameData *frameData, const RenderCamera *renderCamera, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::AddStaticMeshes", 5);

#ifdef USE_DBVT
    // Called for each static mesh surfaces intersecting with camera frustum.
    // Returns true if it want to proceed next query.
    auto addStaticMeshSurfs = [this, frameData, visCamera](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)staticMeshDbvt.GetUserData(proxyId);
        const Mesh::Surface *meshSurf = proxy->staticMesh->GetSurface(proxy->staticMeshSurfIndex);
        if (!meshSurf) {
            return true;
        }

        // 프록시의 RenderObject 가 VisObject 로 등록되었는지 체크
        if (proxy->renderObject->viewCount != viewCount) {
            return true;
        }

        // FIXME: 일단 무조건 인스턴싱을 켠다. 나중에 texture 가 아닌 material 로 변경하면 거기에 옵션을 넣자
        DrawSurf::Flag flags = DrawSurf::Flag::Visible | DrawSurf::Flag::UseInstancing;

        VisObject *visObject = proxy->renderObject->visObject;
        AddDrawSurf(frameData, visCamera, nullptr, visObject, visObject->textures[meshSurf->materialIndex], meshSurf->subMesh, flags);

        return true;
    };

    staticMeshDbvt.QueryFrustum(renderCamera->frustum, addStaticMeshSurfs);
#else
    for (int i = 0; i < frameData->NumVisObjects(); ++i) {
        VisObject *visObject = frameData->GetVisObjects() + i;

        for (int meshSurfIndex = 0; meshSurfIndex < visObject->mesh->NumSurfaces(); ++meshSurfIndex) {
            const Mesh::Surface *meshSurf = visObject->mesh->GetSurface(meshSurfIndex);

            BE1::AABB worldAABB;
            worldAABB.SetFromTransformedAABBFast(meshSurf->subMesh->GetAABB(), visObject->worldMatrix);

            if (renderCamera->frustum.CullAABB(worldAABB)) {
                continue;
            }

            DrawSurf::Flag flags = DrawSurf::Flag::Visible;

            AddDrawSurf(frameData, visCamera, nullptr, visObject, visObject->textures[meshSurf->materialIndex], meshSurf->subMesh, flags);
        }
    }
#endif
}

void RenderWorld::DrawPic(float x, float y, float w, float h, const Texture *texture) {
    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrameData();

    frameData->CmdDrawPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, texture, currentColor.ToUInt32());
}

void RenderWorld::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Texture *texture) {
    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrameData();

    frameData->CmdDrawPic(x, y, w, h, s1, t1, s2, t2, texture, currentColor.ToUInt32());
}

void RenderWorld::DrawBar(float x, float y, float w, float h) {
    RenderFrameData *frameData = RenderContext::activeContext->GetCurrentFrameData();

    frameData->CmdDrawPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, textureManager.whiteTexture, currentColor.ToUInt32());
}

void RenderWorld::DrawRect(float x, float y, float w, float h) {
    if (w > 1) {
        DrawBar(x, y, w, 1);
        if (h > 1) {
            DrawBar(x, y + h - 1, w, 1);
        }
    }

    if (h > 2) {
        DrawBar(x, y + 1, 1, h - 2);
        if (w > 2) {
            DrawBar(x + w - 1, y + 1, 1, h - 2);
        }
    }
}
