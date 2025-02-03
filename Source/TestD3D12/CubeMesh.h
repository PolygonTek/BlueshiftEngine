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

#include "RHI.h"
#include "Mesh.h"
#include "Texture.h"

class RenderContext;
class VisCamera;

class CubeMesh : public Mesh {
public:
    static std::shared_ptr<CubeMesh> CreateMesh();
    static void                 DestroyMesh(std::shared_ptr<CubeMesh> &cubeMesh);

    virtual void                InitMesh() override;
    virtual void                FreeMesh() override;

    virtual BE1::AABB           GetAABB() const override;

    void                        InitPipelineState();

    void                        DrawMesh(RHI::CommandList *commandList, const VisCamera *visCamera, const BE1::Mat3x4 &worldMatrix);
    void                        DrawMeshInstanced(RHI::CommandList *commandList, const VisCamera *visCamera, const BE1::Mat3x4 *instanceData, int instanceCount);

    Texture *                   texture = nullptr;
    RHI::VertexBuffer *         vertexBuffer = nullptr;
    RHI::IndexBuffer *          indexBuffer = nullptr; 
    RHI::PipelineState *        singlePSO = nullptr;
    RHI::PipelineState *        instancingPSO = nullptr;
};
