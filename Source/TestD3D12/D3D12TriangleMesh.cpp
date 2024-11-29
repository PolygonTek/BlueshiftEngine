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
#include "D3D12CommandListPool.h"
#include "D3D12RootDescriptorPool.h"
#include "D3D12VertexBuffer.h"
#include "D3D12IndexBuffer.h"
#include "D3D12Texture.h"
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

    vertexBuffer = D3D12VertexBuffer::CreateVertexBuffer(D3D12VertexBuffer::Type::Static, sizeof(verts[0]), COUNT_OF(verts), (void *)verts);
    indexBuffer = D3D12IndexBuffer::CreateIndexBuffer(D3D12IndexBuffer::Type::Static, sizeof(indexes[0]), COUNT_OF(indexes), (void *)indexes);
    texture = D3D12Texture::CreateTexture(D3D12Texture::Type::Texture2D, "Data/EngineTextures/checker.dds");

    InitRootSignature();

    InitPipelineState();
}

void D3D12TriangleMesh::FreeMesh() {
    SAFE_DELETE(texture);
    SAFE_DELETE(vertexBuffer);
    SAFE_DELETE(indexBuffer);

    SAFE_RELEASE(rootSignature);
    SAFE_RELEASE(pipelineState);
    SAFE_RELEASE(pipelineStateInstancing);
}

void D3D12TriangleMesh::InitRootSignature() {
    // 디스크립터 레인지로 루트 디스크립터 테이블을 정의한다.
    // 디스크립터 레인지는 같은 타입의 디스크립터 여러개를 순차적으로 나타낸다.
    // 디스크립터 테이블 하나는 여러개의 디스크립터 레인지로 구성된다.
    D3D12_DESCRIPTOR_RANGE descriptorRanges[2] = {};

    // SRV (texture) 디스크립터
    descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[0].NumDescriptors = 1;
    descriptorRanges[0].BaseShaderRegister = 0; // t0 부터 시작
    descriptorRanges[0].RegisterSpace = 0;
    descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // CBV 디스크립터
    descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descriptorRanges[1].NumDescriptors = 1;
    descriptorRanges[1].BaseShaderRegister = 0; // b0 부터 시작
    descriptorRanges[1].RegisterSpace = 0;
    descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Root Parameter 하나 당 Descriptor Table 하나를 참조하게 된다.
    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = COUNT_OF(descriptorRanges);
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRanges;

    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // Trilinear 필터링
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplerDesc.MinLOD = -FLT_MAX;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.NumParameters = COUNT_OF(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &samplerDesc;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* pSignatureBlob = nullptr;
    ID3DBlob* pErrorBlob = nullptr;

    // TODO: 필요한 루트 시그니쳐를 캐싱하는 방식으로 접근하자.
    if (SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignatureBlob, &pErrorBlob))) {
        renderer.device->CreateRootSignature(0, pSignatureBlob->GetBufferPointer(), pSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    }

    SAFE_RELEASE(pSignatureBlob);
    SAFE_RELEASE(pErrorBlob);
}

void D3D12TriangleMesh::InitPipelineState() {
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    pipelineState = CreatePSO(rootSignature, "Source/TestD3D12/Shaders/Triangle.hlsl", { inputElementDescs, COUNT_OF(inputElementDescs) });
    pipelineStateInstancing = CreatePSO(rootSignature, "Source/TestD3D12/Shaders/TriangleInstancing.hlsl", { inputElementDescs, COUNT_OF(inputElementDescs) });
}

void D3D12TriangleMesh::DrawMesh(int threadIndex, D3D12CommandList *commandList, const Vec2& offset) {
    D3D12RootDescriptorPool* rootDescriptorPool = renderer.currentFrameData->threadData[threadIndex].rootDescriptorPool;

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
    renderer.device->CopyDescriptorsSimple(1, srvDest, texture->descriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 루트 디스크립터 테이블에 CBV 디스크립터 카피 - 1번
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(cpuRootDescriptorHandle, 1, rootDescriptorPool->descriptorHandleSize);
    renderer.device->CopyDescriptorsSimple(1, cbvDest, cbvDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { rootDescriptorPool->descriptorHeap };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // 루트 시그니쳐를 세팅한다.
    commandList->SetGraphicsRootSignature(rootSignature);
 
    // 위에서 할당한 루트 디스크립터 테이블을 세팅한다.
    commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(0, gpuRootDescriptorHandle);

    //gpuRootDescriptorHandle.Offset(1, rootDescriptorPool->descriptorHandleSize * 2);
    //commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(1, gpuRootDescriptorHandle);

    commandList->SetPipelineState(pipelineState);
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetVertexBuffers(0, 1, &vertexBuffer->vbv);
    commandList->SetIndexBuffer(&indexBuffer->ibv);

    commandList->graphicsCommandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
}

void D3D12TriangleMesh::DrawMeshInstanced(int threadIndex, D3D12CommandList* commandList, const Vec2* instanceData, int instanceCount) {
    D3D12RootDescriptorPool *rootDescriptorPool = renderer.currentFrameData->threadData[threadIndex].rootDescriptorPool;

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
    renderer.device->CopyDescriptorsSimple(1, srvDest, texture->descriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 루트 디스크립터 테이블에 CBV 디스크립터 카피 - 1번
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(cpuRootDescriptorHandle, 1, rootDescriptorPool->descriptorHandleSize);
    renderer.device->CopyDescriptorsSimple(1, cbvDest, cbvDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { rootDescriptorPool->descriptorHeap };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // 루트 시그니쳐를 세팅한다.
    commandList->SetGraphicsRootSignature(rootSignature);

    // 위에서 할당한 루트 디스크립터 테이블을 세팅한다.
    commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(0, gpuRootDescriptorHandle);

    //gpuRootDescriptorHandle.Offset(1, rootDescriptorPool->descriptorHandleSize * 2);
    //commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(1, gpuRootDescriptorHandle);

    commandList->SetPipelineState(pipelineStateInstancing);
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetVertexBuffers(0, 1, &vertexBuffer->vbv);
    commandList->SetIndexBuffer(&indexBuffer->ibv);

    commandList->graphicsCommandList->DrawIndexedInstanced(3, instanceCount, 0, 0, 0);
}
