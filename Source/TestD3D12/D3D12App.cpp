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

#ifdef USE_D3D12_MEMALLOC
    renderer.PrintMemoryAllocatorStats();
#endif
}

void D3D12App::Shutdown() {
    renderer.Finish();

    triangleMesh->FreeMesh();
    SAFE_DELETE(triangleMesh);

    renderer.Shutdown();
}

void D3D12App::Draw(int frameMsec) {
    renderer.BeginRender();

    float elapsedSeconds = MILLI2SEC(elapsedMsec);

    for (int i = 0; i < 1000; ++i) {
        triangleMesh->DrawMesh(elapsedSeconds + i * 0.1f);
    }

    renderer.EndRender();
    renderer.Present();
}

void D3D12App::RunFrame(int frameMsec) {
    elapsedMsec += frameMsec;

    cmdSystem.ExecuteCommandBuffer();
}
