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
#include "VisObject.h"
#include "TriangleMesh.h"
#include "CubeMesh.h"
#include "RenderContext.h"

void VisObject::Draw(const RenderContext *renderContext, RHI::CommandList *commandList, const VisObject *visObject) {
    switch (visObject->state.meshType) {
    case MeshType::TriangleMesh:
        DrawTriangleMesh(renderContext, commandList, visObject);
        break;
    case MeshType::CubeMesh:
        DrawCubeMesh(renderContext, commandList, visObject);
        break;
    }
}

void VisObject::DrawInstanced(const RenderContext *renderContext, RHI::CommandList *commandList, const VisObject *visObjects, int instanceCount) {
    switch (visObjects[0].state.meshType) {
    case MeshType::TriangleMesh:
        VisObject::DrawTriangleMeshInstanced(renderContext, commandList, visObjects, instanceCount);
        break;
    case MeshType::CubeMesh:
        VisObject::DrawCubeMeshInstanced(renderContext, commandList, visObjects, instanceCount);
        break;
    }
}

void VisObject::DrawTriangleMesh(const RenderContext *renderContext, RHI::CommandList *commandList, const VisObject *visObject) {
    TriangleMesh *triangleMesh = static_cast<TriangleMesh *>(visObject->state.mesh.get());
    triangleMesh->DrawMesh(renderContext, commandList, visObject->state.offset);
}

void VisObject::DrawTriangleMeshInstanced(const RenderContext *renderContext, RHI::CommandList *commandList, const VisObject *visObjects, int instanceCount) {
    BE1::Vec2 *instanceData = (BE1::Vec2 *)_alloca32(instanceCount * sizeof(visObjects[0].state.offset));

    for (int i = 0; i < instanceCount; i++) {
        instanceData[i] = visObjects[i].state.offset;
    }
    TriangleMesh *triangleMesh = static_cast<TriangleMesh *>(visObjects[0].state.mesh.get());
    triangleMesh->DrawMeshInstanced(renderContext, commandList, instanceData, instanceCount);
}

void VisObject::DrawCubeMesh(const RenderContext *renderContext, RHI::CommandList *commandList, const VisObject *visObject) {
    CubeMesh *cubeMesh = static_cast<CubeMesh *>(visObject->state.mesh.get());
    cubeMesh->DrawMesh(renderContext, commandList, visObject->state.worldMatrix);
}

void VisObject::DrawCubeMeshInstanced(const RenderContext *renderContext, RHI::CommandList *commandList, const VisObject *visObjects, int instanceCount) {
    BE1::Mat3x4 *instanceData = (BE1::Mat3x4 *)_alloca32(instanceCount * sizeof(visObjects[0].state.worldMatrix));

    for (int i = 0; i < instanceCount; i++) {
        instanceData[i] = visObjects[i].state.worldMatrix;
    }
    CubeMesh *cubeMesh = static_cast<CubeMesh *>(visObjects[0].state.mesh.get());
    cubeMesh->DrawMeshInstanced(renderContext, commandList, instanceData, instanceCount);
}
