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
#include "D3D12Renderer/D3D12Renderer.h"
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

    vertexBuffer = renderer->CreateVertexBuffer(RHIRenderer::BufferType::Static, sizeof(verts[0]), COUNT_OF(verts), (void *)verts);
    indexBuffer = renderer->CreateIndexBuffer(RHIRenderer::BufferType::Static, sizeof(indexes[0]), COUNT_OF(indexes), (void *)indexes);
    texture = renderer->CreateTextureFromFile(RHIRenderer::TextureType::Texture2D, "Data/EngineTextures/checker.dds");

    InitPipelineState();
}

void D3D12CubeMesh::FreeMesh() {
    renderer->DestroyTexture(texture);
    renderer->DestroyVertexBuffer(vertexBuffer);
    renderer->DestroyIndexBuffer(indexBuffer);
    renderer->DestroyPSO(singlePSO);
    renderer->DestroyPSO(instancingPSO);
}

void D3D12CubeMesh::InitPipelineState() {
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

    RHIRenderer::Shader *cubeVS = static_cast<RHIRenderer::Shader *>(renderer->CreateShaderFromFile(RHIRenderer::ShaderModel::SM_6_0, RHIRenderer::ShaderStage::Vertex, "Source/TestD3D12/Shaders/Cube.hlsl", "VSMain"));
    RHIRenderer::Shader *cubePS = static_cast<RHIRenderer::Shader *>(renderer->CreateShaderFromFile(RHIRenderer::ShaderModel::SM_6_0, RHIRenderer::ShaderStage::Fragment, "Source/TestD3D12/Shaders/Cube.hlsl", "PSMain"));

    if (cubeVS && cubePS) {
        RHIRenderer::PipelineStateDesc psoDesc;
        psoDesc.vs = cubeVS;
        psoDesc.ps = cubePS;
        psoDesc.rasterizerState = renderer->GetRasterizerState(RHIRenderer::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = renderer->GetDepthStencilState(RHIRenderer::DepthStencilStateType::Default);
        psoDesc.blendState = renderer->GetBlendState(RHIRenderer::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHIRenderer::PrimitiveTopology::TriangleList;
        psoDesc.renderPass = &renderPass;
        singlePSO = renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (cubeVS) {
        renderer->DestroyShader(cubeVS, true);
    }
    if (cubePS) {
        renderer->DestroyShader(cubePS, true);
    }

    RHIRenderer::Shader *cubeInstancingVS = static_cast<RHIRenderer::Shader *>(renderer->CreateShaderFromFile(RHIRenderer::ShaderModel::SM_6_0, RHIRenderer::ShaderStage::Vertex, "Source/TestD3D12/Shaders/CubeInstancing.hlsl", "VSMain"));
    RHIRenderer::Shader *cubeInstancingPS = static_cast<RHIRenderer::Shader *>(renderer->CreateShaderFromFile(RHIRenderer::ShaderModel::SM_6_0, RHIRenderer::ShaderStage::Fragment, "Source/TestD3D12/Shaders/CubeInstancing.hlsl", "PSMain"));

    if (cubeInstancingVS && cubeInstancingPS) {
        RHIRenderer::PipelineStateDesc psoDesc;
        psoDesc.vs = cubeInstancingVS;
        psoDesc.ps = cubeInstancingPS;
        psoDesc.rasterizerState = renderer->GetRasterizerState(RHIRenderer::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = renderer->GetDepthStencilState(RHIRenderer::DepthStencilStateType::Default);
        psoDesc.blendState = renderer->GetBlendState(RHIRenderer::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHIRenderer::PrimitiveTopology::TriangleList;
        psoDesc.renderPass = &renderPass;
        instancingPSO = renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (cubeInstancingVS) {
        renderer->DestroyShader(cubeInstancingVS, true);
    }
    if (cubeInstancingPS) {
        renderer->DestroyShader(cubeInstancingPS, true);
    }
}

void D3D12CubeMesh::DrawMesh(RHIRenderer::CommandList* commandList, const Mat3x4& worldMatrix) {
    int threadIndex = commandList->GetThreadIndex();

    // 상수 버퍼 공간을 할당한다.
    RHIRenderer::GPUSubResource *cbSubResource = renderer->GetCurrentFrameData()->AllocConstant(threadIndex, sizeof(CubeConstantData));
    if (!cbSubResource) {
        return;
    }

    CubeConstantData *constantDataPtr = reinterpret_cast<CubeConstantData*>(cbSubResource->writePtr);
    constantDataPtr->viewProjMatrix = app.viewProjMatrix;
    constantDataPtr->worldMatrix = worldMatrix;

    renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    renderer->SetIndexBuffer(commandList, indexBuffer);

    renderer->SetPSO(commandList, singlePSO);
    renderer->SetTexture(commandList, 0, texture);
    renderer->SetSubResource(commandList, 0, cbSubResource);

    renderer->DrawIndexed(commandList, 36, 0, 0);
}

void D3D12CubeMesh::DrawMeshInstanced(RHIRenderer::CommandList *commandList, const Mat3x4 *instanceData, int instanceCount) {
    int threadIndex = commandList->GetThreadIndex();

    // 상수 버퍼 공간을 할당한다.
    RHIRenderer::GPUSubResource *cbSubResource = renderer->GetCurrentFrameData()->AllocConstant(threadIndex, sizeof(CubeInstancedConstantData));
    if (!cbSubResource) {
        return;
    }

    CubeInstancedConstantData *constantPtr = reinterpret_cast<CubeInstancedConstantData *>(cbSubResource->writePtr);
    constantPtr->viewProjMatrix = app.viewProjMatrix;

    for (int i = 0; i < instanceCount; ++i) {
        constantPtr->worldMatrix[i] = instanceData[i];
    }

    renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    renderer->SetIndexBuffer(commandList, indexBuffer);

    renderer->SetPSO(commandList, instancingPSO);
    renderer->SetTexture(commandList, 0, texture);
    renderer->SetSubResource(commandList, 0, cbSubResource);

    renderer->DrawIndexedInstanced(commandList, 36, instanceCount, 0, 0, 0);
}
