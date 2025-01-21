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
#endif
    
    for (RenderObject *renderObject : renderObjects) {
        SAFE_DELETE(renderObject);
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

int RenderWorld::AddRenderObject(const RenderObject::Decl &def) {
    assert(BE1::Engine::IsInMainThread());

    int index = renderObjects.FindNull();
    if (index == -1) {
        index = renderObjects.Append(nullptr);
    }

    UpdateRenderObject(index, def);
    return index;
}

void RenderWorld::UpdateRenderObject(int index, const RenderObject::Decl &def) {
    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        renderObject = new RenderObject;
        renderObject->index = index;
        renderObjects[index] = renderObject;

        renderObject->Update(def);

#ifdef USE_DBVT
        renderObject->proxy = (DbvtProxy *)Mem_Alloc(sizeof(DbvtProxy));
        renderObject->proxy->renderObject = renderObject;
        renderObject->proxy->worldAABB = renderObject->GetWorldAABB();
        renderObject->proxy->id = objectDbvt.CreateProxy(renderObject->proxy->worldAABB, BE1::MeterToUnit(0.0f), renderObject->proxy);
#endif
    } else {
#ifdef USE_DBVT
        const bool worldMatrixMatch = (def.worldMatrix == renderObject->decl.worldMatrix);
        const bool aabbMatch = (def.aabb == renderObject->decl.aabb);
        const bool proxyMoved = !worldMatrixMatch || !aabbMatch;

        BE1::Vec3 displacementVector;
        if (proxyMoved) {
            displacementVector = def.worldMatrix.ToTranslationVec3() - renderObject->decl.worldMatrix.ToTranslationVec3();

            renderObject->proxy->worldAABB.SetFromTransformedAABBFast(def.aabb, def.worldMatrix);
            objectDbvt.MoveProxy(renderObject->proxy->id, renderObject->proxy->worldAABB, BE1::MeterToUnit(0.5f), displacementVector);
        }
#endif
        renderObject->Update(def);
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
#endif

    delete renderObjects[index];
    renderObjects[index] = nullptr;
}

void RenderWorld::RenderScene(RenderContext *renderContext, const RenderCamera *renderCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::RenderScene", 1);

    assert(BE1::Engine::IsInMainThread());

    RenderFrameData *frameData = renderContext->GetCurrentFrameData();

    // 렌더링 백엔드가 사용할 내부 카메라 정보를 세팅한다.
    VisCamera *visCamera = frameData->AllocVisCamera();
    if (!visCamera) {
        return;
    }

    visCamera->decl = renderCamera->decl;
    visCamera->viewProjMatrix = renderCamera->viewProjMatrix;

    DrawCamera(renderCamera, visCamera, frameData);
}

void RenderWorld::DrawCamera(const RenderCamera *renderCamera, VisCamera *visCamera, RenderFrameData *frameData) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::DrawCamera", 2);

    visCamera->visObjectStartIndex = frameData->NumVisObjects();

    FindVisObjects(renderCamera, visCamera, frameData);

    visCamera->visObjectEndIndex = frameData->NumVisObjects() - 1;

    // TODO 2: 렌더링에 사용할 라이트들도 추려낸다.
    // TODO 3: 렌더링할 Surface 리스트를 작성한다.
    // TODO 4: Surface 들을 소팅한다.
    // TODO 5: 이후에는 Surface 단위로 그려야 한다.

    // 렌더링 커맨드에 visCamera 를 기록한다.
    frameData->CmdDrawCamera(visCamera);
}

// 렌더 스레드가 다음 렌더링에 사용할 VisObject 들을 준비한다.
void RenderWorld::FindVisObjects(const RenderCamera *renderCamera, VisCamera *visCamera, RenderFrameData *frameData) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::FindVisObjects", 3);

    visCamera->worldAABB.Clear();
#ifdef USE_DBVT
    // Frustum 에 교차된 render objects 들을 추려낸다.
    // 콜백함수에서 true 를 리턴하면 다음 쿼리를 진행한다.
    auto addVisibleObjects = [this, renderCamera, visCamera, frameData](int32_t proxyId) -> bool {
        const DbvtProxy *proxy = (const DbvtProxy *)objectDbvt.GetUserData(proxyId);
        const RenderObject *renderObject = proxy->renderObject;
        if (!renderObject) {
            return true;
        }

        VisObject *visObject = frameData->AllocVisObject();
        if (!visObject) {
            return true;
        }

        visCamera->worldAABB.AddAABB(proxy->worldAABB);

        visObject->GetDecl() = renderObject->GetDecl();
        visObject->modelViewMatrix = renderCamera->viewMatrix * renderObject->GetWorldMatrix();
        visObject->modelViewProjMatrix = renderCamera->viewProjMatrix * renderObject->GetWorldMatrix();
        visObject->ambientVisible = true;
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

        visCamera->worldAABB.AddAABB(renderObject->worldAABB);

        visObject->GetDecl() = renderObject->GetDecl();
        visObject->modelViewMatrix = renderCamera->viewMatrix * renderObject->GetWorldMatrix();
        visObject->modelViewProjMatrix = renderCamera->viewProjMatrix * renderObject->GetWorldMatrix();
        visObject->ambientVisible = true;
    }
#endif
}
