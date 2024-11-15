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
#include "D3D12App.h"

struct Vertex3D {
    Vec3        position;
    Vec4        color;
    Vec2        texCoord;
};

struct DefaultConstantBuffer {
    Vec4        offset;
};

D3D12App        app;

void D3D12App::Init(HWND hwnd) {
    renderer.Init(hwnd, true, true);

    InitMesh();
}

void D3D12App::Shutdown() {
    renderer.Finish();

    FreeMesh();

    renderer.Shutdown();
}

void D3D12App::Draw(int frameMsec) {
    renderer.BeginRender();

    DrawMesh();

    renderer.EndRender();
    renderer.Present();
}

void D3D12App::RunFrame(int frameMsec) {
    elapsedMsec += frameMsec;

    cmdSystem.ExecuteCommandBuffer();
}

void D3D12App::InitMesh() {
    defaultTexture = D3D12Texture::CreateTexture2D("Data/EngineTextures/checker.dds");

    // CB 용 업로드 버퍼 생성
    // NOTE: 256 바이트 주소/사이즈 정렬되어 있어야 한다.
    // 상수 버퍼의 float 최대 개수는 4096
    UINT constantBufferSize = (UINT)AlignUp(sizeof(DefaultConstantBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    renderer.device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&constantBuffer));

    // 디스크립터에 CBV 정보 기록하기
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = constantBufferSize;

    cbvDescriptorHandlePtr = renderer.singleDescriptorAllocator->Alloc();
    renderer.device->CreateConstantBufferView(&cbvDesc, *cbvDescriptorHandlePtr);

    // Map and initialize the constant buffer. We don't unmap this until the
    // app closes. Keeping things mapped for the lifetime of the resource is okay.
    CD3DX12_RANGE writeRange(0, 0); // We do not intend to read from this resource on the CPU.
    constantBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedConstantBase));

    InitRootSignature();

    InitPipelineState();

    // 삼각형의 버텍스/인덱스 버퍼 내용을 작성
    // NOTE: UV 좌표의 V 는 아래쪽으로 증가함을 주의한다. 나중에 통합 렌더러를 작성한다면, shader code 에서 하는게 좋을 듯..
    const Vertex3D vertices[] = {
        { { 0.0f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.5f, 0.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    };

    const uint16_t indexes[] = {
        0, 1, 2
    };

    vertexBuffer = D3D12VertexBuffer::CreateVertexBuffer(sizeof(vertices[0]), 3, (void *)vertices);
    indexBuffer = D3D12IndexBuffer::CreateIndexBuffer(sizeof(indexes[0]), 3, (void *)indexes);
}

void D3D12App::InitRootSignature() {
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

void D3D12App::InitPipelineState() {
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

    ID3DBlob* vertexShader = nullptr;
    D3DCompile(shaderText, strlen(shaderText), "shaderText", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);

    ID3DBlob* pixelShader = nullptr;
    D3DCompile(shaderText, strlen(shaderText), "shaderText", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // PSO 만들기
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    // NOTE: 나중에 호출할 SetGraphicsRootSignature() 에서 PSO 에 지정된 RootSignature 와 다르면 안된다.
    // 여기서 RootSignature 를 지정하는 이유는 파이프라인 호환성 검사 및 최적화 때문이다.
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
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

    SAFE_RELEASE(vertexShader);
    SAFE_RELEASE(pixelShader);
}

void D3D12App::FreeMesh() {
    renderer.singleDescriptorAllocator->Free(cbvDescriptorHandlePtr);

    SAFE_DELETE(defaultTexture);
    SAFE_DELETE(vertexBuffer);
    SAFE_DELETE(indexBuffer);

    SAFE_RELEASE(rootSignature);
    SAFE_RELEASE(pipelineState);
    SAFE_RELEASE(constantBuffer);
}

void D3D12App::DrawMesh() {
    float currentSec = MILLI2SEC(elapsedMsec);

    Vec4* offset = reinterpret_cast<Vec4*>(mappedConstantBase);
    offset->x = 0.5f * Math::Cos(currentSec);
    offset->y = 0.5f * Math::Sin(currentSec * 3);

    ID3D12GraphicsCommandList* currentCommandList = renderer.currentFrameCommandList->commandList;
    D3D12DescriptorPool* currentRootDescriptorPool = renderer.currentFrameData->rootDescriptorPool;

    // 루트 디스크립터 테이블을 할당한다. 여기서 디스크립터 테이블은 연속된 디스크립터 핸들을 말한다.
    D3D12_CPU_DESCRIPTOR_HANDLE cpuRootDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuRootDescriptorHandle;
    currentRootDescriptorPool->AllocDescriptors(2, &cpuRootDescriptorHandle, &gpuRootDescriptorHandle);

    // 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap* descriptorHeaps[] = { currentRootDescriptorPool->descriptorHeap };
    currentCommandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // 루트 시그니쳐를 세팅한다.
    currentCommandList->SetGraphicsRootSignature(rootSignature);

    // 루트 디스크립터 테이블에 SRV 디스크립터 카피 - 0
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvDest(cpuRootDescriptorHandle, 0, currentRootDescriptorPool->descriptorHandleSize);
    renderer.device->CopyDescriptorsSimple(1, srvDest, *defaultTexture->descriptorHandlePtr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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
