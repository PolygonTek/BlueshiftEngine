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
#include "RenderInternal.h"
#include "CubeMesh.h"

void VisObject::Draw(RHI::CommandList *commandList, const VisCamera *visCamera, const VisObject *visObject) {
    switch (visObject->decl.meshType) {
    case MeshType::CubeMesh:
        DrawCubeMesh(commandList, visCamera, visObject);
        break;
    }
}

void VisObject::DrawInstanced(RHI::CommandList *commandList, const VisCamera *visCamera, const VisObject *visObjects, int instanceCount) {
    switch (visObjects[0].decl.meshType) {
    case MeshType::CubeMesh:
        VisObject::DrawCubeMeshInstanced(commandList, visCamera, visObjects, instanceCount);
        break;
    }
}

void VisObject::DrawCubeMesh(RHI::CommandList *commandList, const VisCamera *visCamera, const VisObject *visObject) {
    CubeMesh *cubeMesh = static_cast<CubeMesh *>(visObject->decl.mesh.get());
    cubeMesh->DrawMesh(commandList, visCamera, visObject->decl.worldMatrix);
}

void VisObject::DrawCubeMeshInstanced(RHI::CommandList *commandList, const VisCamera *visCamera, const VisObject *visObjects, int instanceCount) {
    BE1::Mat3x4 *instanceData = (BE1::Mat3x4 *)_alloca32(instanceCount * sizeof(visObjects[0].decl.worldMatrix));

    for (int i = 0; i < instanceCount; i++) {
        instanceData[i] = visObjects[i].decl.worldMatrix;
    }
    CubeMesh *cubeMesh = static_cast<CubeMesh *>(visObjects[0].decl.mesh.get());
    cubeMesh->DrawMeshInstanced(commandList, visCamera, instanceData, instanceCount);
}
