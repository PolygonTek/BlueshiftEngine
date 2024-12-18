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
#include "D3D12Renderer/D3D12CommandList.h"
#include "D3D12VisObject.h"
#include "D3D12TriangleMesh.h"
#include "D3D12CubeMesh.h"

void D3D12VisObject::Draw(D3D12CommandList *commandList, D3D12VisObject *visObject) {
    PIX_SCOPED_EVENT(commandList->graphicsCommandList, 6, "D3D12VisObject::Draw");

    switch (visObject->state.meshType) {
    case MeshType::TriangleMesh:
        DrawTriangleMesh(commandList, visObject);
        break;
    case MeshType::CubeMesh:
        DrawCubeMesh(commandList, visObject);
        break;
    }
}

void D3D12VisObject::DrawInstanced(D3D12CommandList *commandList, D3D12VisObject *visObjects, int instanceCount) {
    PIX_SCOPED_EVENT(commandList->graphicsCommandList, 7, "D3D12VisObject::DrawInstanced");

    switch (visObjects[0].state.meshType) {
    case MeshType::TriangleMesh:
        D3D12VisObject::DrawTriangleMeshInstanced(commandList, visObjects, instanceCount);
        break;
    case MeshType::CubeMesh:
        D3D12VisObject::DrawCubeMeshInstanced(commandList, visObjects, instanceCount);
        break;
    }
}

void D3D12VisObject::DrawTriangleMesh(D3D12CommandList *commandList, D3D12VisObject *visObject) {
    D3D12TriangleMesh *triangleMesh = static_cast<D3D12TriangleMesh *>(visObject->state.mesh.get());
    triangleMesh->DrawMesh(commandList, visObject->state.offset);
}

void D3D12VisObject::DrawTriangleMeshInstanced(D3D12CommandList *commandList, D3D12VisObject *visObjects, int instanceCount) {
    Vec2 *instanceData = (Vec2 *)_alloca32(instanceCount * sizeof(visObjects[0].state.offset));

    for (int i = 0; i < instanceCount; i++) {
        instanceData[i] = visObjects[i].state.offset;
    }
    D3D12TriangleMesh *triangleMesh = static_cast<D3D12TriangleMesh *>(visObjects[0].state.mesh.get());
    triangleMesh->DrawMeshInstanced(commandList, instanceData, instanceCount);
}

void D3D12VisObject::DrawCubeMesh(D3D12CommandList *commandList, D3D12VisObject *visObject) {
    D3D12CubeMesh *cubeMesh = static_cast<D3D12CubeMesh *>(visObject->state.mesh.get());
    cubeMesh->DrawMesh(commandList, visObject->state.worldMatrix);
}

void D3D12VisObject::DrawCubeMeshInstanced(D3D12CommandList *commandList, D3D12VisObject *visObjects, int instanceCount) {
    Mat3x4 *instanceData = (Mat3x4 *)_alloca32(instanceCount * sizeof(visObjects[0].state.worldMatrix));

    for (int i = 0; i < instanceCount; i++) {
        instanceData[i] = visObjects[i].state.worldMatrix;
    }
    D3D12CubeMesh *cubeMesh = static_cast<D3D12CubeMesh *>(visObjects[0].state.mesh.get());
    cubeMesh->DrawMeshInstanced(commandList, instanceData, instanceCount);
}
