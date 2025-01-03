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
#include "VisObject.h"
#include "TriangleMesh.h"
#include "CubeMesh.h"

void VisObject::Draw(D3D12CommandList *commandList, VisObject *visObject) {
    PIX_SCOPED_EVENT(commandList->GetGraphicsCommandList(), 6, "D3D12VisObject::Draw");

    switch (visObject->state.meshType) {
    case MeshType::TriangleMesh:
        DrawTriangleMesh(commandList, visObject);
        break;
    case MeshType::CubeMesh:
        DrawCubeMesh(commandList, visObject);
        break;
    }
}

void VisObject::DrawInstanced(D3D12CommandList *commandList, VisObject *visObjects, int instanceCount) {
    PIX_SCOPED_EVENT(commandList->GetGraphicsCommandList(), 7, "D3D12VisObject::DrawInstanced");

    switch (visObjects[0].state.meshType) {
    case MeshType::TriangleMesh:
        VisObject::DrawTriangleMeshInstanced(commandList, visObjects, instanceCount);
        break;
    case MeshType::CubeMesh:
        VisObject::DrawCubeMeshInstanced(commandList, visObjects, instanceCount);
        break;
    }
}

void VisObject::DrawTriangleMesh(D3D12CommandList *commandList, VisObject *visObject) {
    TriangleMesh *triangleMesh = static_cast<TriangleMesh *>(visObject->state.mesh.get());
    triangleMesh->DrawMesh(commandList, visObject->state.offset);
}

void VisObject::DrawTriangleMeshInstanced(D3D12CommandList *commandList, VisObject *visObjects, int instanceCount) {
    BE1::Vec2 *instanceData = (BE1::Vec2 *)_alloca32(instanceCount * sizeof(visObjects[0].state.offset));

    for (int i = 0; i < instanceCount; i++) {
        instanceData[i] = visObjects[i].state.offset;
    }
    TriangleMesh *triangleMesh = static_cast<TriangleMesh *>(visObjects[0].state.mesh.get());
    triangleMesh->DrawMeshInstanced(commandList, instanceData, instanceCount);
}

void VisObject::DrawCubeMesh(D3D12CommandList *commandList, VisObject *visObject) {
    CubeMesh *cubeMesh = static_cast<CubeMesh *>(visObject->state.mesh.get());
    cubeMesh->DrawMesh(commandList, visObject->state.worldMatrix);
}

void VisObject::DrawCubeMeshInstanced(D3D12CommandList *commandList, VisObject *visObjects, int instanceCount) {
    BE1::Mat3x4 *instanceData = (BE1::Mat3x4 *)_alloca32(instanceCount * sizeof(visObjects[0].state.worldMatrix));

    for (int i = 0; i < instanceCount; i++) {
        instanceData[i] = visObjects[i].state.worldMatrix;
    }
    CubeMesh *cubeMesh = static_cast<CubeMesh *>(visObjects[0].state.mesh.get());
    cubeMesh->DrawMeshInstanced(commandList, instanceData, instanceCount);
}
