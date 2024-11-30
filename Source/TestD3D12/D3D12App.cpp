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
#include "D3D12Renderer.h"
#include "D3D12App.h"
#include "D3D12GameObject.h"
#include "D3D12TriangleMesh.h"
#include "D3D12CubeMesh.h"

#define TRIANGLE_OR_CUBE    0

static constexpr int        TriangleCount = 1024;

static constexpr int        CubeDimensionX = 64;
static constexpr int        CubeDimensionY = 64;
static constexpr int        CubeCount = CubeDimensionX * CubeDimensionY;
static constexpr float      CubeSpacing = 2.82842712f;

D3D12App                    app;

void D3D12App::Init(HWND hwnd) {
    renderer.Init(hwnd);

    InitGameObjects();

#ifdef USE_D3D12_MEMALLOC
    renderer.PrintMemoryAllocatorStats();
#endif
}

void D3D12App::Shutdown() {
    renderer.Finish();

    ClearGameObjects();

    if (triangleMesh) {
        triangleMesh->FreeMesh();
        delete triangleMesh;
    }

    if (cubeMesh) {
        cubeMesh->FreeMesh();
        delete cubeMesh;
    }

    renderer.Shutdown();
}

void D3D12App::RunFrame(int frameMsec) {
    PIX_CPU_SCOPED_EVENT(7, "D3D12App::RunFrame");

    elapsedMsec += frameMsec;

    UpdateCamera();

    UpdateGameObjects();

    renderer.FlushRenderObjects();

#ifndef USE_RENDER_THREAD
    renderer.BeginFrame();
    renderer.DrawRenderObjects();
    renderer.EndFrame();
#endif

    cmdSystem.ExecuteCommandBuffer();
}

void D3D12App::SetViewMatrix(const Mat3 &viewAxis, const Vec3 &viewOrigin, float *rowMajor4x4ViewMatrix) const {
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

void D3D12App::UpdateCamera() {
    PIX_CPU_SCOPED_EVENT(8, "D3D12App::UpdateCamera");

    Mat4 projMatrix;
    float aspectRatio = renderer.viewport.Width / renderer.viewport.Height;
    projMatrix.SetPerspectiveRH(45, aspectRatio, 1, 1000, false);

    Mat4 viewMatrix;
    SetViewMatrix(Mat3(-1, 0, 0, 0, -1, 0, 0, 0, 1), Vec3(220, 0, 0), viewMatrix);

    viewProjMatrix = projMatrix * viewMatrix;
}

void D3D12App::ClearGameObjects() {
    for (int i = 0; i < gameObjects.Count(); ++i) {
        D3D12GameObject *gameObject = gameObjects[i];

        renderer.RemoveRenderObject(gameObject->renderObjectHandle);
    }

    gameObjects.DeleteContents(true);
}

void D3D12App::InitGameObjects() {
#if TRIANGLE_OR_CUBE == 1
    InitTriangles();
#else
    InitCubes();
#endif
}

void D3D12App::UpdateGameObjects() {
    PIX_CPU_SCOPED_EVENT(9, "D3D12App::UpdateGameObjects");

#if TRIANGLE_OR_CUBE == 1
    UpdateTriangles();
#else
    UpdateCubes();
#endif
}

void D3D12App::InitTriangles() {
    triangleMesh = new D3D12TriangleMesh;
    triangleMesh->InitMesh();

    gameObjects.Reserve(TriangleCount);

    for (int i = 0; i < TriangleCount; ++i) {
        D3D12GameObject *gameObject = new D3D12GameObject;
        gameObjects.Append(gameObject);

        gameObject->renderObjectDef.meshType = D3D12MeshType::TriangleMesh;
        gameObject->renderObjectDef.mesh = triangleMesh;
        gameObject->renderObjectDef.offset.Set(0, 0);

        gameObject->renderObjectHandle = renderer.AddRenderObject(gameObject->renderObjectDef);
    }
}

void D3D12App::InitCubes() {
    cubeMesh = new D3D12CubeMesh;
    cubeMesh->InitMesh();

    gameObjects.Reserve(CubeCount);

    for (int i = 0; i < CubeCount; ++i) {
        D3D12GameObject *gameObject = new D3D12GameObject;
        gameObjects.Append(gameObject);

        gameObject->renderObjectDef.meshType = D3D12MeshType::CubeMesh;
        gameObject->renderObjectDef.mesh = cubeMesh;
        gameObject->renderObjectDef.worldMatrix.SetIdentity();

        gameObject->renderObjectHandle = renderer.AddRenderObject(gameObject->renderObjectDef);
    }
}

void D3D12App::UpdateTriangles() {
    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    for (int i = 0; i < TriangleCount; ++i) {
        float t = elapsedSeconds + i * 0.1f;

        D3D12GameObject *gameObject = gameObjects[i];

        gameObject->renderObjectDef.offset.x = 0.5f * Math::Cos(t);
        gameObject->renderObjectDef.offset.y = 0.5f * Math::Sin(t * 3);

        renderer.UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDef);
    }
}

void D3D12App::UpdateCubes() {
    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    constexpr float startX = -CubeSpacing * (CubeDimensionX - 1) * 0.5f;
    constexpr float startY = -CubeSpacing * (CubeDimensionY - 1) * 0.5f;

    for (int y = 0; y < CubeDimensionY; ++y) {
        for (int x = 0; x < CubeDimensionX; ++x) {
            int index = CubeDimensionX * y + x;

            float t = elapsedSeconds + index * 0.1f;

            D3D12GameObject* gameObject = gameObjects[index];
            gameObject->renderObjectDef.worldMatrix.SetTranslationRotation(Vec3(0, startX + CubeSpacing * x, startY + CubeSpacing * y), Mat3::FromRotationZYX(t * 1.0f, 0, t * 0.25f), false);

            renderer.UpdateRenderObject(gameObject->renderObjectHandle, gameObject->renderObjectDef);
        }
    }
}
