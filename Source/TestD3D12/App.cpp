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
#include "App.h"
#include "GameObject.h"
#include "VisObject.h"
#include "TriangleMesh.h"
#include "CubeMesh.h"
#include "D3D12Renderer/D3D12Renderer.h"

#define TRIANGLE_OR_CUBE    0

static constexpr int        TriangleCount = 1024;

static constexpr int        CubeDimensionX = 64;
static constexpr int        CubeDimensionY = 64;
static constexpr int        CubeCount = CubeDimensionX * CubeDimensionY;
static constexpr float      CubeSpacing = 2.82842712f;

App                         app;

void App::Init() {
    InitGameObjects();

    renderObjects.Reserve(16384);

#ifdef USE_D3D12_MEMALLOC
    renderer->PrintMemoryAllocatorStats();
#endif
}

void App::Shutdown() {
    renderer->Finish(RHI::CommandQueueType::Graphics);

    ClearGameObjects();
}

void App::RunFrame(int frameMsec) {
    PIX_CPU_SCOPED_EVENT(2, "App::RunFrame");

    elapsedMsec += frameMsec;

    UpdateGameObjects();

    renderer->FreePendingResources();

    BE1::cmdSystem.ExecuteCommandBuffer();
}

void App::SetViewMatrix(const BE1::Mat3 &viewAxis, const BE1::Vec3 &viewOrigin, float *rowMajor4x4ViewMatrix) const {
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

void App::ClearGameObjects() {
    for (GameObject *gameObject : gameObjects) {
        gameObject->renderObjectDef.mesh.reset();

        RemoveRenderObject(gameObject->renderObjectHandle);
    }

    TriangleMesh::DestroyMesh(triangleMesh);
    CubeMesh::DestroyMesh(cubeMesh);

    gameObjects.DeleteContents(true);
}

void App::InitGameObjects() {
#if TRIANGLE_OR_CUBE == 1
    InitTriangles();
#else
    InitCubes();
#endif
}

void App::UpdateGameObjects() {
    PIX_CPU_SCOPED_EVENT(1, "App::UpdateGameObjects");

#if TRIANGLE_OR_CUBE == 1
    UpdateTriangles();
#else
    UpdateCubes();
#endif
}

int App::AddRenderObject(const RenderObject::State &def) {
    assert(BE1::Engine::IsInMainThread());

    int index = renderObjects.FindNull();
    if (index == -1) {
        index = renderObjects.Append(nullptr);
    }

    UpdateRenderObject(index, def);
    return index;
}

void App::UpdateRenderObject(int index, const RenderObject::State &def) {
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

void App::RemoveRenderObject(int index) {
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

void App::RenderScene(RenderContext *renderContext/*, const RenderCamera *camera*/) {
    assert(BE1::Engine::IsInMainThread());

    float w = renderContext->GetWidth();
    float h = renderContext->GetHeight();
    float aspectRatio = w / h;

    BE1::Mat4 projMatrix;
    projMatrix.SetPerspectiveRH(45, aspectRatio, 1, 1000, false);

    BE1::Mat4 viewMatrix;
    SetViewMatrix(BE1::Mat3(-1, 0, 0, 0, -1, 0, 0, 0, 1), BE1::Vec3(220, 0, 0), viewMatrix);

#ifdef USE_RENDER_THREAD
    renderContext->WaitRenderCompleted();

    // 렌더 스레드에서 다음 렌더링에 사용할 VisObject 들을 준비한다.
    // 
    // TODO: 보이는 오브젝트 수를 계산한다.
    int numVisObjects = renderObjects.Count();

    RenderFrameData *writeFrameData = renderContext->GetCurrentFrameData();

    VisCamera *visCamera = writeFrameData->AllocVisCamera();
    visCamera->viewProjMatrix = projMatrix * viewMatrix;
    
    // TODO: RenderScene 을 여러번 호출할 수 있어야함
    VisObject *visObjects = writeFrameData->AllocVisObjects(numVisObjects);

    // TODO 1: 현재 카메라에 기반해 SceneGraph 나 Frustum culling 등으로 렌더링에 사용할 렌더 오브젝트들을 추려낸다. 추려낸 렌더 오브젝트들의 변수는 복사 or (레퍼런스 카운트를 이용한) 공유를 해서 가지고 있어야 한다.
    for (int i = 0; i < numVisObjects; ++i) {
        visObjects[i].GetState() = renderObjects[i]->GetState();
    }

    // TODO 2: 렌더링에 사용할 라이트들도 추려낸다.
    // TODO 3: 렌더링할 Surface 리스트를 작성한다.
    // TODO 4: Surface 들을 소팅한다.
    // TODO 5: 이후에는 Surface 단위로 그려야 한다.

    renderContext->MarkUpdateCompleted();
#else
    renderContext->BeginFrame();

    int numVisObjects = renderObjects.Count();

    RenderFrameData *writeFrameData = renderContext->GetCurrentFrameData();

    VisCamera *visCamera = writeFrameData->AllocVisCamera();
    visCamera->viewProjMatrix = projMatrix * viewMatrix;

    VisObject *visObjects = writeFrameData->AllocVisObjects(numVisObjects);

    for (int i = 0; i < numVisObjects; ++i) {
        visObjects[i].GetState() = renderObjects[i]->GetState();
    }

    renderContext->RenderFrame();
    renderContext->EndFrame();
#endif
}

RenderContext *App::CreateRenderContext(HWND hwnd) {
    RenderContext *renderContext = new RenderContext;
    renderContext->Init(hwnd);
    return renderContext;
}

void App::DestroyRenderContext(RenderContext *renderContext) {
    renderContext->Shutdown();
    delete renderContext;
}

void App::InitTriangles() {
    triangleMesh = TriangleMesh::CreateMesh();

    gameObjects.Reserve(TriangleCount);

    for (int i = 0; i < TriangleCount; ++i) {
        GameObject *gameObject = new GameObject;
        gameObjects.Append(gameObject);

        gameObject->renderObjectDef.meshType = MeshType::TriangleMesh;
        gameObject->renderObjectDef.mesh = triangleMesh;
        gameObject->renderObjectDef.offset.Set(0, 0);

        gameObject->renderObjectHandle = AddRenderObject(gameObject->renderObjectDef);
    }
}

void App::InitCubes() {
    cubeMesh = CubeMesh::CreateMesh();

    gameObjects.Reserve(CubeCount);

    for (int i = 0; i < CubeCount; ++i) {
        GameObject *gameObject = new GameObject;
        gameObjects.Append(gameObject);

        gameObject->renderObjectDef.meshType = MeshType::CubeMesh;
        gameObject->renderObjectDef.mesh = cubeMesh;
        gameObject->renderObjectDef.worldMatrix.SetIdentity();

        gameObject->renderObjectHandle = AddRenderObject(gameObject->renderObjectDef);
    }
}

void App::UpdateTriangles() {
    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    for (int i = 0; i < TriangleCount; ++i) {
        float t = elapsedSeconds + i * 0.1f;

        GameObject *gameObject = gameObjects[i];

        gameObject->renderObjectDef.offset.x = 0.5f * BE1::Math::Cos(t);
        gameObject->renderObjectDef.offset.y = 0.5f * BE1::Math::Sin(t * 3);

        UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDef);
    }
}

void App::UpdateCubes() {
    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    constexpr float startX = -CubeSpacing * (CubeDimensionX - 1) * 0.5f;
    constexpr float startY = -CubeSpacing * (CubeDimensionY - 1) * 0.5f;

    for (int y = 0; y < CubeDimensionY; ++y) {
        for (int x = 0; x < CubeDimensionX; ++x) {
            int index = CubeDimensionX * y + x;

            float t = elapsedSeconds + index * 0.1f;

            GameObject* gameObject = gameObjects[index];
            gameObject->renderObjectDef.worldMatrix.SetTranslationRotation(BE1::Vec3(0, startX + CubeSpacing * x, startY + CubeSpacing * y), BE1::Mat3::FromRotationZYX(t * 1.0f, 0, t * 0.25f), false);

            UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDef);
        }
    }
}
