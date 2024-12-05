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
#include "D3D12CubeMesh.h"
#include "D3D12App.h"

struct CubeVertex {
    Vec3        position;
    uint32_t    color;
    Vec2        texCoord;
};

struct CubeConstantData {
    Mat4        viewProjMatrix;
    Mat3x4      worldMatrix;
};

struct CubeInstancedConstantData {
    Mat4        viewProjMatrix;
    Mat3x4      worldMatrix[1024];
};

std::shared_ptr<D3D12CubeMesh> D3D12CubeMesh::CreateMesh() {
    std::shared_ptr cubeMesh = std::make_shared<D3D12CubeMesh>();
    cubeMesh->InitMesh();
    return cubeMesh;
}

void D3D12CubeMesh::DestroyMesh(std::shared_ptr<D3D12CubeMesh> &cubeMesh) {
    if (cubeMesh) {
        cubeMesh->FreeMesh();
        cubeMesh.reset();
    }
}

void D3D12CubeMesh::InitMesh() {
    // NOTE: UV 좌표의 V 는 아래쪽으로 증가함을 주의한다. 나중에 통합 렌더러를 작성한다면, shader code 에서 하는게 좋을 듯..
    ALIGN_AS32 const CubeVertex verts[] = {
        { { -1.0f, -1.0f, -1.0f }, 0xffffffff, { 0.0f, 1.0f } },
        { { -1.0f,  1.0f, -1.0f }, 0xffffffff, { 1.0f, 1.0f } },
        { {  1.0f,  1.0f, -1.0f }, 0xffffffff, { 1.0f, 0.0f } },
        { {  1.0f, -1.0f, -1.0f }, 0xffffffff, { 0.0f, 0.0f } },

        { {  1.0f, -1.0f,  1.0f }, 0xffffffff, { 0.0f, 1.0f } },
        { {  1.0f,  1.0f,  1.0f }, 0xffffffff, { 1.0f, 1.0f } },
        { { -1.0f,  1.0f,  1.0f }, 0xffffffff, { 1.0f, 0.0f } },
        { { -1.0f, -1.0f,  1.0f }, 0xffffffff, { 0.0f, 0.0f } },

        { {  1.0f, -1.0f, -1.0f }, 0xffffffff, { 0.0f, 1.0f } },
        { {  1.0f,  1.0f, -1.0f }, 0xffffffff, { 1.0f, 1.0f } },
        { {  1.0f,  1.0f,  1.0f }, 0xffffffff, { 1.0f, 0.0f } },
        { {  1.0f, -1.0f,  1.0f }, 0xffffffff, { 0.0f, 0.0f } },

        { {  1.0f,  1.0f, -1.0f }, 0xffffffff, { 0.0f, 1.0f } },
        { { -1.0f,  1.0f, -1.0f }, 0xffffffff, { 1.0f, 1.0f } },
        { { -1.0f,  1.0f,  1.0f }, 0xffffffff, { 1.0f, 0.0f } },
        { {  1.0f,  1.0f,  1.0f }, 0xffffffff, { 0.0f, 0.0f } },

        { { -1.0f,  1.0f, -1.0f }, 0xffffffff, { 0.0f, 1.0f } },
        { { -1.0f, -1.0f, -1.0f }, 0xffffffff, { 1.0f, 1.0f } },
        { { -1.0f, -1.0f,  1.0f }, 0xffffffff, { 1.0f, 0.0f } },
        { { -1.0f,  1.0f,  1.0f }, 0xffffffff, { 0.0f, 0.0f } },

        { { -1.0f, -1.0f, -1.0f }, 0xffffffff, { 0.0f, 1.0f } },
        { {  1.0f, -1.0f, -1.0f }, 0xffffffff, { 1.0f, 1.0f } },
        { {  1.0f, -1.0f,  1.0f }, 0xffffffff, { 1.0f, 0.0f } },
        { { -1.0f, -1.0f,  1.0f }, 0xffffffff, { 0.0f, 0.0f } },
    };

    ALIGN_AS32 const uint16_t indexes[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    vertexBuffer = renderer.CreateVertexBuffer(RHIRenderer::BufferType::Static, sizeof(verts[0]), COUNT_OF(verts), (void *)verts);
    indexBuffer = renderer.CreateIndexBuffer(RHIRenderer::BufferType::Static, sizeof(indexes[0]), COUNT_OF(indexes), (void *)indexes);
    texture = renderer.CreateTextureFromFile(RHIRenderer::TextureType::Texture2D, "Data/EngineTextures/checker.dds");

    InitRootSignature();

    InitPipelineState();
}

void D3D12CubeMesh::FreeMesh() {
    renderer.MarkForDelete(texture);
    renderer.MarkForDelete(vertexBuffer);
    renderer.MarkForDelete(indexBuffer);

    SAFE_RELEASE(rootSignature);
    SAFE_RELEASE(singlePSO);
    SAFE_RELEASE(instancingPSO);
}

void D3D12CubeMesh::InitRootSignature() {
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

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
    rootSignatureDesc.Desc_1_0.NumParameters = COUNT_OF(rootParameters);
    rootSignatureDesc.Desc_1_0.pParameters = rootParameters;
    rootSignatureDesc.Desc_1_0.NumStaticSamplers = 1;
    rootSignatureDesc.Desc_1_0.pStaticSamplers = &samplerDesc;
    rootSignatureDesc.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    // TODO: 필요한 루트 시그니쳐를 캐싱하는 방식으로 접근하자.
    if (SUCCEEDED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signatureBlob, &errorBlob))) {
        renderer.device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    }

    SAFE_RELEASE(signatureBlob);
    SAFE_RELEASE(errorBlob);
}

void D3D12CubeMesh::InitPipelineState() {
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    singlePSO = renderer.CreatePSO(rootSignature, "Source/TestD3D12/Shaders/Cube.hlsl", { inputElementDescs, COUNT_OF(inputElementDescs) });
    instancingPSO = renderer.CreatePSO(rootSignature, "Source/TestD3D12/Shaders/CubeInstancing.hlsl", { inputElementDescs, COUNT_OF(inputElementDescs) });
}

void D3D12CubeMesh::DrawMesh(int threadIndex, D3D12CommandList* commandList, const Mat3x4& worldMatrix) {
    D3D12RootDescriptorPool* rootDescriptorPool = renderer.currentFrameData->GetThreadData(threadIndex).rootDescriptorPool;

    // 상수 버퍼 공간을 할당한다.
    D3D12_CPU_DESCRIPTOR_HANDLE cbvDescriptorHandle = {0};
    void *writePtr = renderer.currentFrameData->AllocConstant(threadIndex, sizeof(CubeConstantData), &cbvDescriptorHandle);
    if (!writePtr) {
        return;
    }

    CubeConstantData *constantDataPtr = (reinterpret_cast<CubeConstantData*>(writePtr));
    constantDataPtr->viewProjMatrix = app.viewProjMatrix;
    constantDataPtr->worldMatrix = worldMatrix;

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

    // 루트 시그니쳐를 세팅한다.
    commandList->SetGraphicsRootSignature(rootSignature);

    // 위에서 할당한 루트 디스크립터 테이블을 세팅한다.
    commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(0, gpuRootDescriptorHandle);

    //gpuRootDescriptorHandle.Offset(1, rootDescriptorPool->descriptorHandleSize * 2);
    //commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(1, gpuRootDescriptorHandle);

    commandList->SetVertexBuffer(0, vertexBuffer);
    commandList->SetIndexBuffer(indexBuffer);

    commandList->SetPipelineState(singlePSO);
    commandList->SetPrimitiveTopology(RHIRenderer::PrimitiveTopology::TriangleList);

    commandList->graphicsCommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}

void D3D12CubeMesh::DrawMeshInstanced(int threadIndex, D3D12CommandList *commandList, const Mat3x4 *instanceData, int instanceCount) {
    D3D12RootDescriptorPool *rootDescriptorPool = renderer.currentFrameData->GetThreadData(threadIndex).rootDescriptorPool;

    // 상수 버퍼 공간을 할당한다.
    D3D12_CPU_DESCRIPTOR_HANDLE cbvDescriptorHandle = { 0 };
    void *writePtr = renderer.currentFrameData->AllocConstant(threadIndex, sizeof(CubeInstancedConstantData), &cbvDescriptorHandle);
    if (!writePtr) {
        return;
    }

    CubeInstancedConstantData *constantPtr = (reinterpret_cast<CubeInstancedConstantData *>(writePtr));
    constantPtr->viewProjMatrix = app.viewProjMatrix;

    for (int i = 0; i < instanceCount; ++i) {
        constantPtr->worldMatrix[i] = instanceData[i];
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

    // 루트 시그니쳐를 세팅한다.
    commandList->SetGraphicsRootSignature(rootSignature);

    // 위에서 할당한 루트 디스크립터 테이블을 세팅한다.
    commandList->graphicsCommandList->SetGraphicsRootDescriptorTable(0, gpuRootDescriptorHandle);

    commandList->SetVertexBuffer(0, vertexBuffer);
    commandList->SetIndexBuffer(indexBuffer);

    commandList->SetPipelineState(instancingPSO);
    commandList->SetPrimitiveTopology(RHIRenderer::PrimitiveTopology::TriangleList);

    commandList->graphicsCommandList->DrawIndexedInstanced(36, instanceCount, 0, 0, 0);
}
