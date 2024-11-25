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
#include "D3D12RenderObject.h"
#include "D3D12TriangleMesh.h"
#include "D3D12CubeMesh.h"
#include "D3D12Renderer.h"

void D3D12RenderObject::Update(const State &stateDef) {
    state = stateDef;
}

void D3D12RenderObject::Draw(int threadIndex, int drawIndex, D3D12CommandList* commandList) {
    switch (state.meshType) {
    case D3D12MeshType::TriangleMesh:
        DrawTriangleMesh(threadIndex, drawIndex, commandList);
        break;
    case D3D12MeshType::CubeMesh:
        DrawCubeMesh(threadIndex, drawIndex, commandList);
        break;
    }
}

void D3D12RenderObject::DrawTriangleMesh(int threadIndex, int drawIndex, D3D12CommandList* commandList) {
    D3D12TriangleMesh *triangleMesh = static_cast<D3D12TriangleMesh *>(state.mesh);
    triangleMesh->DrawMesh(threadIndex, drawIndex, commandList, state.offset);
}

void D3D12RenderObject::DrawCubeMesh(int threadIndex, int drawIndex, D3D12CommandList *commandList) {
    D3D12CubeMesh *cubeMesh = static_cast<D3D12CubeMesh *>(state.mesh);
    cubeMesh->DrawMesh(threadIndex, drawIndex, commandList, state.worldMatrix);
}
