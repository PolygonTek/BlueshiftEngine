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
#include "RenderWorld.h"

#define TRIANGLE_OR_CUBE    0

static constexpr int        TriangleCount = 1024;

static constexpr int        CubeDimensionX = 64;
static constexpr int        CubeDimensionY = 64;
static constexpr int        CubeCount = CubeDimensionX * CubeDimensionY;
static constexpr float      CubeSpacing = 2.82842712f;

App                         app;

void App::Init() {
    renderWorld = AllocRenderWorld();

    InitGameObjects();
}

void App::Shutdown() {
    RHI::renderer->Finish(RHI::CommandQueueType::Graphics);

    ClearGameObjects();

    FreeRenderWorld(renderWorld);
}

void App::RunFrame(int frameMsec) {
    PROFILER_CPU_SCOPED_EVENT("App::RunFrame", 0);

    elapsedMsec += frameMsec;

    UpdateGameObjects();

    RHI::renderer->FreePendingResources();

    BE1::cmdSystem.ExecuteCommandBuffer();
}

void App::ClearGameObjects() {
    for (GameObject *gameObject : gameObjects) {
        gameObject->renderObjectDef.mesh.reset();

        renderWorld->RemoveRenderObject(gameObject->renderObjectHandle);
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
    PROFILER_CPU_SCOPED_EVENT("App::UpdateGameObjects", 1);

#if TRIANGLE_OR_CUBE == 1
    UpdateTriangles();
#else
    UpdateCubes();
#endif
}

RenderContext *App::CreateRenderContext(HWND hwnd) {
    RenderContext *renderContext = new RenderContext;
#ifdef USE_RENDER_THREAD
    bool useRenderThread = true;
#else
    bool useRenderThread = false;
#endif
    renderContext->Init(hwnd, useRenderThread);
    return renderContext;
}

void App::DestroyRenderContext(RenderContext *renderContext) {
    renderContext->Shutdown();
    delete renderContext;
}

RenderWorld *App::AllocRenderWorld() {
    RenderWorld *renderWorld = new RenderWorld;
    return renderWorld;
}

void App::FreeRenderWorld(RenderWorld *renderWorld) {
    if (renderWorld) {
        delete renderWorld;
    }
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

        gameObject->renderObjectHandle = renderWorld->AddRenderObject(gameObject->renderObjectDef);
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

        gameObject->renderObjectHandle = renderWorld->AddRenderObject(gameObject->renderObjectDef);
    }
}

void App::UpdateTriangles() {
    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    for (int i = 0; i < TriangleCount; ++i) {
        float t = elapsedSeconds + i * 0.1f;

        GameObject *gameObject = gameObjects[i];

        gameObject->renderObjectDef.offset.x = 0.5f * BE1::Math::Cos(t);
        gameObject->renderObjectDef.offset.y = 0.5f * BE1::Math::Sin(t * 3);

        renderWorld->UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDef);
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

            GameObject *gameObject = gameObjects[index];
            gameObject->renderObjectDef.worldMatrix.SetTranslationRotation(BE1::Vec3(0, startX + CubeSpacing * x, startY + CubeSpacing * y), BE1::Mat3::FromRotationZYX(t * 1.0f, 0, t * 0.25f), false);

            renderWorld->UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDef);
        }
    }
}
