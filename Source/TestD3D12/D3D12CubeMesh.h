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

class D3D12Texture;
class D3D12VertexBuffer;
class D3D12IndexBuffer;

class D3D12CubeMesh : public D3D12Mesh {
public:
    virtual void                    InitMesh() override;
    virtual void                    FreeMesh() override;

    void                            DrawMesh(int threadIndex, D3D12CommandList *commandList, const Mat3x4 &worldMatrix);
    void                            DrawMeshInstanced(int threadIndex, D3D12CommandList *commandList, const Mat3x4 *instanceData, int instanceCount);

    void                            InitRootSignature();
    void                            InitPipelineState();

    D3D12Texture *                  texture = nullptr;
    D3D12VertexBuffer *             vertexBuffer = nullptr;
    D3D12IndexBuffer *              indexBuffer = nullptr;
 
    ID3D12RootSignature *           rootSignature = nullptr;
    ID3D12PipelineState *           pipelineState = nullptr;
    ID3D12PipelineState *           pipelineStateInstancing = nullptr;
};
