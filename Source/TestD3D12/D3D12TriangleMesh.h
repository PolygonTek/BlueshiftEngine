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

#include "D3D12Mesh.h"

class D3D12TriangleMesh : public D3D12Mesh {
public:
    static std::shared_ptr<D3D12TriangleMesh> CreateMesh();
    static void                     DestroyMesh(std::shared_ptr<D3D12TriangleMesh> &triangleMesh);

    virtual void                    InitMesh() override;
    virtual void                    FreeMesh() override;

    void                            InitPipelineState();

    void                            DrawMesh(RHIRenderer::CommandList *commandList, const Vec2 &offset);
    void                            DrawMeshInstanced(RHIRenderer::CommandList *commandList, const Vec2 *instanceData, int instanceCount);

    RHIRenderer::Texture *          texture = nullptr;
    RHIRenderer::VertexBuffer *     vertexBuffer = nullptr;
    RHIRenderer::IndexBuffer *      indexBuffer = nullptr; 
    RHIRenderer::PipelineState *    singlePSO = nullptr;
    RHIRenderer::PipelineState *    instancingPSO = nullptr;
};
