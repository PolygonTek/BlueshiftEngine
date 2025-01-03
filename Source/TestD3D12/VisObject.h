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

#include "RenderObject.h"

class D3D12CommandList;

class VisObject {
public:
    RenderObject::State &      GetState() { return state; }

    static void                     Draw(D3D12CommandList *commandList, VisObject *visObject);
    static void                     DrawInstanced(D3D12CommandList *commandList, VisObject *visObjects, int instanceCount);

private:
    static void                     DrawTriangleMesh(D3D12CommandList *commandList, VisObject *visObject);
    static void                     DrawTriangleMeshInstanced(D3D12CommandList *commandList, VisObject *visObjects, int instanceCount);

    static void                     DrawCubeMesh(D3D12CommandList *commandList, VisObject *visObject);
    static void                     DrawCubeMeshInstanced(D3D12CommandList *commandList, VisObject *visObjects, int instanceCount);

    RenderObject::State        state;
};
