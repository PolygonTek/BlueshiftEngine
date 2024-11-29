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
#include "D3D12CommandListPool.h"
#include "D3D12RenderObject.h"
#include "D3D12TriangleMesh.h"
#include "D3D12CubeMesh.h"
#include "D3D12Renderer.h"

void D3D12RenderObject::Update(const State &stateDef) {
    state = stateDef;
}

void D3D12RenderObject::Draw(int threadIndex, D3D12CommandList* commandList) {
    PIX_SCOPED_EVENT(commandList->graphicsCommandList, 8, "D3D12RenderObject::Draw");

    switch (state.meshType) {
    case D3D12MeshType::TriangleMesh:
        DrawTriangleMesh(threadIndex, commandList);
        break;
    case D3D12MeshType::CubeMesh:
        DrawCubeMesh(threadIndex, commandList);
        break;
    }
}

void D3D12RenderObject::DrawInstanced(int threadIndex, D3D12CommandList *commandList, D3D12RenderObject **renderObjectPtrs, int instanceCount) {
    PIX_SCOPED_EVENT(commandList->graphicsCommandList, 8, "D3D12RenderObject::DrawInstanced");

    switch (renderObjectPtrs[0]->state.meshType) {
    case D3D12MeshType::TriangleMesh:
        D3D12RenderObject::DrawTriangleMeshInstanced(threadIndex, commandList, renderObjectPtrs, instanceCount);
        break;
    case D3D12MeshType::CubeMesh:
        D3D12RenderObject::DrawCubeMeshInstanced(threadIndex, commandList, renderObjectPtrs, instanceCount);
        break;
    }
}

void D3D12RenderObject::DrawTriangleMesh(int threadIndex, D3D12CommandList* commandList) {
    D3D12TriangleMesh *triangleMesh = static_cast<D3D12TriangleMesh *>(state.mesh);
    triangleMesh->DrawMesh(threadIndex, commandList, state.offset);
}

void D3D12RenderObject::DrawTriangleMeshInstanced(int threadIndex, D3D12CommandList *commandList, D3D12RenderObject **renderObjectPtrs, int instanceCount) {
    Vec2* instanceData = (Vec2 *)_alloca32(instanceCount * sizeof(renderObjectPtrs[0]->state.offset));

    for (int i = 0; i < instanceCount; i++) {
        instanceData[i] = renderObjectPtrs[i]->state.offset;
    }
    D3D12TriangleMesh *triangleMesh = static_cast<D3D12TriangleMesh *>(renderObjectPtrs[0]->state.mesh);
    triangleMesh->DrawMeshInstanced(threadIndex, commandList, instanceData, instanceCount);
}

void D3D12RenderObject::DrawCubeMesh(int threadIndex, D3D12CommandList *commandList) {
    D3D12CubeMesh *cubeMesh = static_cast<D3D12CubeMesh *>(state.mesh);
    cubeMesh->DrawMesh(threadIndex, commandList, state.worldMatrix);
}

void D3D12RenderObject::DrawCubeMeshInstanced(int threadIndex, D3D12CommandList *commandList, D3D12RenderObject **renderObjectPtrs, int instanceCount) {
    Mat3x4 *instanceData = (Mat3x4 *)_alloca32(instanceCount * sizeof(renderObjectPtrs[0]->state.worldMatrix));

    for (int i = 0; i < instanceCount; i++) {
        instanceData[i] = renderObjectPtrs[i]->state.worldMatrix;
    }
    D3D12CubeMesh *cubeMesh = static_cast<D3D12CubeMesh *>(renderObjectPtrs[0]->state.mesh);
    cubeMesh->DrawMeshInstanced(threadIndex, commandList, instanceData, instanceCount);
}
