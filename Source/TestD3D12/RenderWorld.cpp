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
#include "VisObject.h"

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

static void SetViewMatrix(const BE1::Mat3 &viewAxis, const BE1::Vec3 &viewOrigin, float *rowMajor4x4ViewMatrix) {
    // left axis
    rowMajor4x4ViewMatrix[0] = -viewAxis[1].x;
    rowMajor4x4ViewMatrix[1] = -viewAxis[1].y;
    rowMajor4x4ViewMatrix[2] = -viewAxis[1].z;
    rowMajor4x4ViewMatrix[3] = -viewAxis[1].Dot(-viewOrigin);

    // up axis
    rowMajor4x4ViewMatrix[4] = viewAxis[2].x;
    rowMajor4x4ViewMatrix[5] = viewAxis[2].y;
    rowMajor4x4ViewMatrix[6] = viewAxis[2].z;
    rowMajor4x4ViewMatrix[7] = viewAxis[2].Dot(-viewOrigin);

    // forward axis
    rowMajor4x4ViewMatrix[8] = -viewAxis[0].x;
    rowMajor4x4ViewMatrix[9] = -viewAxis[0].y;
    rowMajor4x4ViewMatrix[10] = -viewAxis[0].z;
    rowMajor4x4ViewMatrix[11] = -viewAxis[0].Dot(-viewOrigin);

    rowMajor4x4ViewMatrix[12] = 0.0f;
    rowMajor4x4ViewMatrix[13] = 0.0f;
    rowMajor4x4ViewMatrix[14] = 0.0f;
    rowMajor4x4ViewMatrix[15] = 1.0f;
}

void RenderWorld::RenderScene(RenderContext *renderContext/*, const RenderCamera *renderCamera*/) {
    PROFILER_CPU_SCOPED_EVENT("RenderWorld::RenderScene", 1);

    assert(BE1::Engine::IsInMainThread());

    float w = renderContext->GetWidth();
    float h = renderContext->GetHeight();
    float aspectRatio = w / h;

    BE1::Mat4 projMatrix;
    projMatrix.SetPerspectiveRH(45, aspectRatio, 1, 1000, false);

    BE1::Mat4 viewMatrix;
    SetViewMatrix(BE1::Mat3(-1, 0, 0, 0, -1, 0, 0, 0, 1), BE1::Vec3(220, 0, 0), viewMatrix);

    if (renderContext->IsUsingRenderThread()) {
        renderContext->WaitRenderCompleted();
    }

    // NOTE: 반드시 WaitRenderCompleted 이후에 호출해야 함
    RenderFrameData *currentFrameData = renderContext->GetCurrentFrameData();

    // 렌더링 백엔드가 사용할 내부 카메라 정보를 세팅한다.
    VisCamera *visCamera = currentFrameData->AllocVisCamera();
    visCamera->viewProjMatrix = projMatrix * viewMatrix;

    // 렌더 스레드가 다음 렌더링에 사용할 VisObject 들을 준비한다.
    //
    // TODO: 보이는 오브젝트 수를 계산한다.
    int numVisObjects = renderObjects.Count();

    // 렌더링 백엔드가 사용할 내부 렌더 오브젝트 정보를 세팅한다.
    VisObject *visObjects = currentFrameData->AllocVisObjects(numVisObjects);

    // TODO: 현재 카메라에 기반해 SceneGraph 나 Frustum culling 등으로 렌더링에 사용할 렌더 오브젝트들을 추려낸다. 추려낸 렌더 오브젝트들의 변수는 복사 or (레퍼런스 카운트를 이용한) 공유를 해서 가지고 있어야 한다.
    for (int i = 0; i < numVisObjects; ++i) {
        visObjects[i].GetState() = renderObjects[i]->GetState();
    }

    // TODO 2: 렌더링에 사용할 라이트들도 추려낸다.
    // TODO 3: 렌더링할 Surface 리스트를 작성한다.
    // TODO 4: Surface 들을 소팅한다.
    // TODO 5: 이후에는 Surface 단위로 그려야 한다.

    if (renderContext->IsUsingRenderThread()) {
        renderContext->MarkUpdateCompleted();
    }
}
