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
}

int RenderWorld::AddRenderObject(const RenderObject::State &def) {
    assert(BE1::Engine::IsInMainThread());

    int index = renderObjects.FindNull();
    if (index == -1) {
        index = renderObjects.Append(nullptr);
    }

    UpdateRenderObject(index, def);
    return index;
}

void RenderWorld::UpdateRenderObject(int index, const RenderObject::State &def) {
    assert(BE1::Engine::IsInMainThread());

    while (index >= renderObjects.Count()) {
        renderObjects.Append(nullptr);
    }

    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        renderObject = new RenderObject;
        renderObject->index = index;
        renderObjects[index] = renderObject;
    }

    renderObject->Update(def);
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

    visCamera->state = renderCamera->state;
    visCamera->viewProjMatrix = renderCamera->viewProjMatrix;

    DrawCamera(visCamera, frameData);
}

void RenderWorld::DrawCamera(VisCamera *visCamera, RenderFrameData *frameData) {
    visCamera->visObjectStartIndex = frameData->NumVisObjects();

    // 렌더 스레드가 다음 렌더링에 사용할 VisObject 들을 준비한다.
    // TODO: 현재 카메라에 기반해서 SceneGraph 나 Frustum culling 등으로 렌더링에 사용할 오브젝트들을 등록한다.
    for (int i = 0; i < renderObjects.Count(); ++i) {
        VisObject *visObject = frameData->AllocVisObject();
        if (!visObject) {
            continue;
        }
        visObject->GetState() = renderObjects[i]->GetState();
    }

    visCamera->visObjectEndIndex = frameData->NumVisObjects() - 1;

    // TODO 2: 렌더링에 사용할 라이트들도 추려낸다.
    // TODO 3: 렌더링할 Surface 리스트를 작성한다.
    // TODO 4: Surface 들을 소팅한다.
    // TODO 5: 이후에는 Surface 단위로 그려야 한다.

    // 렌더링 커맨드에 visCamera 를 기록한다.
    frameData->CmdDrawCamera(visCamera);
}
