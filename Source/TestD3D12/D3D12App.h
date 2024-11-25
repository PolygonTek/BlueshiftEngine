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

#pragma once

#include "D3D12Common.h"
#include "D3D12ConstantBuffer.h"

class D3D12CommandList;
class D3D12GameObject;
class D3D12TriangleMesh;
class D3D12CubeMesh;

class D3D12App {
public:
    void                            Init(HWND windowHandle);
    void                            Shutdown();

    void                            RunFrame(int elapsedMsec);
    void                            Draw(int elapsedMsec);

    int                             GetElapsedMsec() const { return elapsedMsec; }

    void                            SetViewMatrix(const Mat3 &viewAxis, const Vec3 &viewOrigin, float *rowMajor4x4ViewMatrix) const;

    void                            UpdateCamera();

    void                            ClearGameObjects();

    void                            InitGameObjects();
    void                            InitTriangles();
    void                            InitCubes();

    void                            UpdateGameObjects();
    void                            UpdateTriangles();
    void                            UpdateCubes();

    void                            DrawMeshes(D3D12CommandList *commandList);
    void                            DrawTriangles(D3D12CommandList *commandList);
    void                            DrawCubes(D3D12CommandList *commandList);

    Array<D3D12GameObject *>        gameObjects;

    D3D12TriangleMesh *             triangleMesh = nullptr;
    D3D12CubeMesh *                 cubeMesh = nullptr;

    Mat4                            viewProjMatrix;

    int                             elapsedMsec = 0;
};

extern D3D12App                     app;
