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
#include "D3D12DescriptorPool.h"
#include "D3D12VertexBuffer.h"
#include "D3D12IndexBuffer.h"
#include "D3D12Texture.h"
#include "D3D12TriangleMesh.h"

struct TriangleVertex {
    Vec3        position;
    uint32_t    color;
    Vec2        texCoord;
};

struct TriangleConstants {
    Vec4        offset;
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
    texture = D3D12Texture::CreateTexture(D3D12TextureType::Texture2D, "Data/EngineTextures/checker.dds");

    InitRootSignature();

    InitPipelineState();
}

void D3D12TriangleMesh::FreeMesh() {
    SAFE_DELETE(texture);
    SAFE_DELETE(vertexBuffer);
    SAFE_DELETE(indexBuffer);

    SAFE_RELEASE(rootSignature);
    SAFE_RELEASE(pipelineState);
}

void D3D12TriangleMesh::InitRootSignature() {
    // 디스크립터 레인지로 루트 디스크립터 테이블을 정의한다.
    // 디스크립터 레인지는 같은 타입의 디스크립터 여러개를 순차적으로 나타낸다.
    D3D12_DESCRIPTOR_RANGE descriptorRanges[2] = {};

    // texture
    descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[0].NumDescriptors = 1;
    descriptorRanges[0].BaseShaderRegister = 0; // t0 부터 시작
    descriptorRanges[0].RegisterSpace = 0;
    descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // constant buffer
    descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descriptorRanges[1].NumDescriptors = 1;
    descriptorRanges[1].BaseShaderRegister = 0; // b0 부터 시작
    descriptorRanges[1].RegisterSpace = 0;
    // NOTE: D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND 를 주면 Root Signature 를 만들 때,
    // rootParameters[1].DescriptorTable.pDescriptorRanges 의 순서대로 Descriptor Table 의 참조 정보가 만들어진다.
    descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Root Parameter 하나 당 Descriptor Table 하나를 참조하게 된다.
    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    // Descriptor Table 하나는 여러개의 Descriptor Range 로 구성된다.
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
    // Shader Compile
    const char* shaderText = R"(
struct VSInput {
    float4 position : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

cbuffer CONSTANT_BUFFER_DEFAULT : register(b0) {
    float4 offset;
};

Texture2D defaultTexture : register(t0);
SamplerState defaultSampler : register(s0);

PSInput VSMain(VSInput input) {
    PSInput result = (PSInput)0;

    result.position = input.position;
    result.position.xy += offset.xy;
    result.color = input.color;
    result.texCoord = input.texCoord;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 color = defaultTexture.Sample(defaultSampler, input.texCoord);
    return color * input.color;
})";

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ID3DBlob *errorBlob = nullptr;
    ID3DBlob* compiledVertexShader = nullptr;
    if (FAILED(D3DCompile(shaderText, strlen(shaderText), "shaderText", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &compiledVertexShader, &errorBlob))) {
        renderer.PrintCompileErrorMessages(errorBlob);
    }
    SAFE_RELEASE(errorBlob);

    ID3DBlob* compiledPixelShader = nullptr;
    if (FAILED(D3DCompile(shaderText, strlen(shaderText), "shaderText", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &compiledPixelShader, &errorBlob))) {
        renderer.PrintCompileErrorMessages(errorBlob);
    }
    SAFE_RELEASE(errorBlob);

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // PSO 만들기
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    // NOTE: 나중에 호출할 SetGraphicsRootSignature() 에서 PSO 에 지정된 RootSignature 와 다르면 안된다.
    // 여기서 RootSignature 를 지정하는 이유는 파이프라인 호환성 검사 및 최적화 때문이다.
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(compiledVertexShader->GetBufferPointer(), compiledVertexShader->GetBufferSize());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(compiledPixelShader->GetBufferPointer(), compiledPixelShader->GetBufferSize());
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.InputLayout = { inputElementDescs, COUNT_OF(inputElementDescs) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc.Count = 1;
    renderer.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

    SAFE_RELEASE(compiledVertexShader);
    SAFE_RELEASE(compiledPixelShader);
}

void D3D12TriangleMesh::DrawMesh() {
    ID3D12GraphicsCommandList* currentCommandList = renderer.currentFrameCommandList->commandList;
    D3D12DescriptorPool* currentRootDescriptorPool = renderer.currentFrameData->rootDescriptorPool;

    D3D12_CPU_DESCRIPTOR_HANDLE *cbvDescriptorHandlePtr = nullptr;
    void *writePtr = renderer.currentFrameData->AllocConstant(sizeof(TriangleConstants), &cbvDescriptorHandlePtr);
    if (!writePtr) {
        return;
    }

    TriangleConstants *constantPtr = (reinterpret_cast<TriangleConstants*>(writePtr));
    constantPtr->offset.x = offset.x;
    constantPtr->offset.y = offset.y;

    // 루트 디스크립터 테이블을 할당한다. 여기서 디스크립터 테이블은 연속된 디스크립터 핸들을 말한다.
    D3D12_CPU_DESCRIPTOR_HANDLE cpuRootDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuRootDescriptorHandle;
    if (!currentRootDescriptorPool->AllocDescriptors(2, &cpuRootDescriptorHandle, &gpuRootDescriptorHandle)) {
        return;
    }

    // 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap* descriptorHeaps[] = { currentRootDescriptorPool->descriptorHeap };
    currentCommandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // 루트 시그니쳐를 세팅한다.
    currentCommandList->SetGraphicsRootSignature(rootSignature);

    // 루트 디스크립터 테이블에 SRV 디스크립터 카피 - 0
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvDest(cpuRootDescriptorHandle, 0, currentRootDescriptorPool->descriptorHandleSize);
    renderer.device->CopyDescriptorsSimple(1, srvDest, *texture->descriptorHandlePtr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 루트 디스크립터 테이블에 CBV 디스크립터 카피 - 1
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(cpuRootDescriptorHandle, 1, currentRootDescriptorPool->descriptorHandleSize);
    renderer.device->CopyDescriptorsSimple(1, cbvDest, *cbvDescriptorHandlePtr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    // 루트 디스크립터 테이블을 세팅한다.
    currentCommandList->SetGraphicsRootDescriptorTable(0, gpuRootDescriptorHandle);

    //gpuDescriptorHandle.Offset(1, currentRootDescriptorPool->descriptorHandleSize);
    //renderer.currentCommandList->SetGraphicsRootDescriptorTable(1, gpuRootDescriptorHandle);

    currentCommandList->SetPipelineState(pipelineState);

    currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    currentCommandList->IASetVertexBuffers(0, 1, &vertexBuffer->vbv);
    currentCommandList->IASetIndexBuffer(&indexBuffer->ibv);
    currentCommandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
}
