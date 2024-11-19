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
#include "D3D12TriangleMesh.h"
#include "D3D12CubeMesh.h"

D3D12App        app;

void D3D12App::Init(HWND hwnd) {
#ifdef _DEBUG
    bool enableDebugLayer = true;
    bool withGpuValidation = true;
#else
    bool enableDebugLayer = false;
    bool withGpuValidation = false;
#endif

    renderer.Init(hwnd, enableDebugLayer, withGpuValidation);

    triangleMesh = new D3D12TriangleMesh;
    triangleMesh->InitMesh();

    cubeMesh = new D3D12CubeMesh;
    cubeMesh->InitMesh();

#ifdef USE_D3D12_MEMALLOC
    renderer.PrintMemoryAllocatorStats();
#endif
}

void D3D12App::Shutdown() {
    renderer.Finish();

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

void D3D12App::Draw(int frameMsec) {
    renderer.BeginRender();

    //DrawTriangles();
    DrawCubes();

    renderer.EndRender();
    renderer.Present();
}

void D3D12App::DrawTriangles() {
    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    for (int i = 0; i < 1000; ++i) {
        float t = elapsedSeconds + i * 0.1f;

        triangleMesh->offset.x = 0.5f * Math::Cos(t);
        triangleMesh->offset.y = 0.5f * Math::Sin(t * 3);

        triangleMesh->DrawMesh();
    }
}

void D3D12App::DrawCubes() {
    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    Mat4 projMatrix;
    float aspectRatio = renderer.viewport.Width / renderer.viewport.Height;
    projMatrix.SetPerspectiveRH(45, aspectRatio, 1, 1000, false);

    Mat4 viewMatrix;
    SetViewMatrix(Mat3(-1, 0, 0, 0, -1, 0, 0, 0, 1), Vec3(100, 0, 0), viewMatrix);

    viewProjMatrix = projMatrix * viewMatrix;

    constexpr int dimensionX = 32;
    constexpr int dimensionY = 32;
    constexpr float spacing = 2.5f;
    constexpr float startX = -spacing * (dimensionX - 1) * 0.5f;
    constexpr float startY = -spacing * (dimensionY - 1) * 0.5f;

    int numDrawCalls = 0;

    for (int y = 0; y < dimensionY; ++y) {
        for (int x = 0; x < dimensionX; ++x) {
            float t = elapsedSeconds + (dimensionX * y + x) * 0.1f;

            cubeMesh->worldMatrix.SetTRS(Vec3(0, startX + spacing * x, startY + spacing * y), Mat3::FromRotationZYX(t, 0, t), Vec3::one);

            cubeMesh->DrawMesh();
            numDrawCalls++;
        }

        if (numDrawCalls >= 300) {
            numDrawCalls = 0;
            renderer.FlushCommandList();
        }
    }
}

void D3D12App::RunFrame(int frameMsec) {
    elapsedMsec += frameMsec;

    cmdSystem.ExecuteCommandBuffer();
}
