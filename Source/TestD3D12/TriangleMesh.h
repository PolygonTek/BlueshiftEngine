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

#include "RHIRenderer.h"
#include "Mesh.h"

class TriangleMesh : public Mesh {
public:
    static std::shared_ptr<TriangleMesh> CreateMesh();
    static void                     DestroyMesh(std::shared_ptr<TriangleMesh> &triangleMesh);

    virtual void                    InitMesh() override;
    virtual void                    FreeMesh() override;

    void                            InitPipelineState();

    void                            DrawMesh(RHI::CommandList *commandList, const BE1::Vec2 &offset);
    void                            DrawMeshInstanced(RHI::CommandList *commandList, const BE1::Vec2 *instanceData, int instanceCount);

    RHI::Texture *                  texture = nullptr;
    RHI::VertexBuffer *             vertexBuffer = nullptr;
    RHI::IndexBuffer *              indexBuffer = nullptr; 
    RHI::PipelineState *            singlePSO = nullptr;
    RHI::PipelineState *            instancingPSO = nullptr;
};
