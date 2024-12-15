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
#include "D3D12Renderer.h"
#include "D3D12CommandList.h"
#include "D3D12RootDescriptorPool.h"
#include "D3D12TriangleMesh.h"

struct TriangleVertex {
    Vec3        position;
    uint32_t    color;
    Vec2        texCoord;
};

struct TriangleConstantData {
    Vec4        offset;
};

struct TriangleInstancedConstantData {
    Vec4        offset[1024];
};

std::shared_ptr<D3D12TriangleMesh> D3D12TriangleMesh::CreateMesh() {
    std::shared_ptr triangleMesh = std::make_shared<D3D12TriangleMesh>();
    triangleMesh->InitMesh();
    return triangleMesh;
}

void D3D12TriangleMesh::DestroyMesh(std::shared_ptr<D3D12TriangleMesh>& triangleMesh) {
    if (triangleMesh) {
        triangleMesh->FreeMesh();
        triangleMesh.reset();
    }
}

void D3D12TriangleMesh::InitMesh() {
    // 삼각형의 버텍스/인덱스 버퍼 내용을 작성
    // NOTE: UV 좌표의 V 는 아래쪽으로 증가함을 주의한다. 나중에 통합 렌더러를 작성한다면, shader code 에서 하는게 좋을 듯..
    ALIGN_AS32 const TriangleVertex verts[] = {
        { { 0.0f, 0.5f, 0.0f }, Color4::red.ToUInt32(), { 0.5f, 0.0f }},
        { { 0.5f, -0.5f, 0.0f }, Color4::green.ToUInt32(), { 1.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.0f }, Color4::blue.ToUInt32(), { 0.0f, 1.0f } },
    };

    ALIGN_AS32 const uint16_t indexes[] = {
        0, 1, 2
    };

    vertexBuffer = renderer.CreateVertexBuffer(RHIRenderer::BufferType::Static, sizeof(verts[0]), COUNT_OF(verts), (void *)verts);
    indexBuffer = renderer.CreateIndexBuffer(RHIRenderer::BufferType::Static, sizeof(indexes[0]), COUNT_OF(indexes), (void *)indexes);
    texture = renderer.CreateTextureFromFile(RHIRenderer::TextureType::Texture2D, "Data/EngineTextures/checker.dds");

    InitPipelineState();
}

void D3D12TriangleMesh::FreeMesh() {
    renderer.MarkForDelete(texture);
    renderer.MarkForDelete(vertexBuffer);
    renderer.MarkForDelete(indexBuffer);

    renderer.DestroyPSO(singlePSO);
    renderer.DestroyPSO(instancingPSO);
}

void D3D12TriangleMesh::InitPipelineState() {
    RHIRenderer::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, 0, 0, RHIRenderer::InputLayoutElement::Format::Float3 },
        { "COLOR", 0, 12, 0, RHIRenderer::InputLayoutElement::Format::UByte4N },
        { "TEXCOORD", 0, 16, 0, RHIRenderer::InputLayoutElement::Format::Float2 },
    };

    RHIRenderer::RenderPass renderPass;
    renderPass.renderTargetCount = 1;
    renderPass.renderTargetFormats[0] = Image::Format::RGBA_8_8_8_8;
    renderPass.depthStencilFormat = Image::Format::DepthStencil_24_8;

    RHIRenderer::PipelineStateDesc psoDesc;
    psoDesc.vs = static_cast<RHIRenderer::Shader *>(renderer.CreateShaderFromFile(RHIRenderer::ShaderStage::Vertex, "Source/TestD3D12/Shaders/Triangle.hlsl", "VSMain"));
    psoDesc.ps = static_cast<RHIRenderer::Shader *>(renderer.CreateShaderFromFile(RHIRenderer::ShaderStage::Fragment, "Source/TestD3D12/Shaders/Triangle.hlsl", "PSMain"));
    psoDesc.rasterizerState = renderer.GetRasterizerState(RHIRenderer::RasterizerStateType::SolidFrontSided);
    psoDesc.depthStencilState = renderer.GetDepthStencilState(RHIRenderer::DepthStencilStateType::Default);
    psoDesc.blendState = renderer.GetBlendState(RHIRenderer::BlendStateType::Opaque);
    psoDesc.inputLayout = &inputLayout;
    psoDesc.primitiveTopology = RHIRenderer::PrimitiveTopology::TriangleList;
    psoDesc.renderPass = &renderPass;
    singlePSO = renderer.CreatePSO(&psoDesc);

    SAFE_DELETE(psoDesc.vs);
    SAFE_DELETE(psoDesc.ps);

    psoDesc.vs = static_cast<RHIRenderer::Shader *>(renderer.CreateShaderFromFile(RHIRenderer::ShaderStage::Vertex, "Source/TestD3D12/Shaders/TriangleInstancing.hlsl", "VSMain"));
    psoDesc.ps = static_cast<RHIRenderer::Shader *>(renderer.CreateShaderFromFile(RHIRenderer::ShaderStage::Fragment, "Source/TestD3D12/Shaders/TriangleInstancing.hlsl", "PSMain"));
    psoDesc.rasterizerState = renderer.GetRasterizerState(RHIRenderer::RasterizerStateType::SolidFrontSided);
    psoDesc.depthStencilState = renderer.GetDepthStencilState(RHIRenderer::DepthStencilStateType::Default);
    psoDesc.blendState = renderer.GetBlendState(RHIRenderer::BlendStateType::Opaque);
    psoDesc.inputLayout = &inputLayout;
    psoDesc.primitiveTopology = RHIRenderer::PrimitiveTopology::TriangleList;
    psoDesc.renderPass = &renderPass;
    instancingPSO = renderer.CreatePSO(&psoDesc);

    SAFE_DELETE(psoDesc.vs);
    SAFE_DELETE(psoDesc.ps);
}

void D3D12TriangleMesh::DrawMesh(int threadIndex, D3D12CommandList *commandList, const Vec2& offset) {
    D3D12RootDescriptorPool* rootDescriptorPool = renderer.currentFrameData->GetThreadData(threadIndex).rootDescriptorPool;

    // 상수 버퍼 공간을 할당한다.
    D3D12_CPU_DESCRIPTOR_HANDLE cbvDescriptorHandle = {0};
    void *writePtr = renderer.currentFrameData->AllocConstant(threadIndex, sizeof(TriangleConstantData), &cbvDescriptorHandle);
    if (!writePtr) {
        return;
    }

    TriangleConstantData *constantDataPtr = (reinterpret_cast<TriangleConstantData*>(writePtr));
    constantDataPtr->offset.x = offset.x;
    constantDataPtr->offset.y = offset.y;

    // 2개의 디스크립터를 갖는 루트 디스크립터 테이블을 할당한다. 여기서 디스크립터 테이블은 연속된 디스크립터 핸들을 말한다.
    D3D12_CPU_DESCRIPTOR_HANDLE cpuRootDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuRootDescriptorHandle;
    if (!rootDescriptorPool->AllocRange(2, &cpuRootDescriptorHandle, &gpuRootDescriptorHandle)) {
        return;
    }

    // 루트 디스크립터 테이블에 SRV 디스크립터 카피 - 0번
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvDest(cpuRootDescriptorHandle, 0, rootDescriptorPool->descriptorHandleSize);
    renderer.device->CopyDescriptorsSimple(1, srvDest, static_cast<D3D12Texture *>(texture)->descriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 루트 디스크립터 테이블에 CBV 디스크립터 카피 - 1번
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(cpuRootDescriptorHandle, 1, rootDescriptorPool->descriptorHandleSize);
    renderer.device->CopyDescriptorsSimple(1, cbvDest, cbvDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { rootDescriptorPool->descriptorHeap };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    commandList->SetPipelineState(singlePSO);
    commandList->SetPrimitiveTopology(RHIRenderer::PrimitiveTopology::TriangleList);
 
    // 위에서 할당한 루트 디스크립터 테이블을 세팅한다.
    commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(0, gpuRootDescriptorHandle);

    //gpuRootDescriptorHandle.Offset(1, rootDescriptorPool->descriptorHandleSize * 2);
    //commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(1, gpuRootDescriptorHandle);

    commandList->SetVertexBuffer(0, vertexBuffer);
    commandList->SetIndexBuffer(indexBuffer);

    commandList->graphicsCommandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
}

void D3D12TriangleMesh::DrawMeshInstanced(int threadIndex, D3D12CommandList* commandList, const Vec2* instanceData, int instanceCount) {
    D3D12RootDescriptorPool *rootDescriptorPool = renderer.currentFrameData->GetThreadData(threadIndex).rootDescriptorPool;

    // 상수 버퍼 공간을 할당한다.
    D3D12_CPU_DESCRIPTOR_HANDLE cbvDescriptorHandle = { 0 };
    void *writePtr = renderer.currentFrameData->AllocConstant(threadIndex, sizeof(TriangleInstancedConstantData), &cbvDescriptorHandle);
    if (!writePtr) {
        return;
    }

    TriangleInstancedConstantData *constantDataPtr = (reinterpret_cast<TriangleInstancedConstantData *>(writePtr));
    for (int i = 0; i < instanceCount; ++i) {
        constantDataPtr->offset[i] = Vec4(instanceData[i], Vec2::zero);
    }

    // 2개의 디스크립터를 갖는 루트 디스크립터 테이블을 할당한다. 여기서 디스크립터 테이블은 연속된 디스크립터 핸들을 말한다.
    D3D12_CPU_DESCRIPTOR_HANDLE cpuRootDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuRootDescriptorHandle;
    if (!rootDescriptorPool->AllocRange(2, &cpuRootDescriptorHandle, &gpuRootDescriptorHandle)) {
        return;
    }

    // 루트 디스크립터 테이블에 SRV 디스크립터 카피 - 0번
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvDest(cpuRootDescriptorHandle, 0, rootDescriptorPool->descriptorHandleSize);
    renderer.device->CopyDescriptorsSimple(1, srvDest, static_cast<D3D12Texture *>(texture)->descriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 루트 디스크립터 테이블에 CBV 디스크립터 카피 - 1번
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(cpuRootDescriptorHandle, 1, rootDescriptorPool->descriptorHandleSize);
    renderer.device->CopyDescriptorsSimple(1, cbvDest, cbvDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { rootDescriptorPool->descriptorHeap };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    commandList->SetPipelineState(instancingPSO);
    commandList->SetPrimitiveTopology(RHIRenderer::PrimitiveTopology::TriangleList);

    // 위에서 할당한 루트 디스크립터 테이블을 세팅한다.
    commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(0, gpuRootDescriptorHandle);

    //gpuRootDescriptorHandle.Offset(1, rootDescriptorPool->descriptorHandleSize * 2);
    //commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(1, gpuRootDescriptorHandle);

    commandList->SetVertexBuffer(0, vertexBuffer);
    commandList->SetIndexBuffer(indexBuffer);

    commandList->graphicsCommandList->DrawIndexedInstanced(3, instanceCount, 0, 0, 0);
}
