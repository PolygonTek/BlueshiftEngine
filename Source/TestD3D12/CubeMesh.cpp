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
#include "CubeMesh.h"
#include "App.h"
#include "D3D12Renderer/D3D12Renderer.h"
#include "RenderContext.h"
#include "VisObject.h"

struct CubeVertex {
    BE1::Vec3       position;
    uint32_t        color;
    BE1::Vec2       texCoord;
};

struct CubeConstantData {
    BE1::Mat4       viewProjMatrix;
    BE1::Mat3x4     worldMatrix;
};

struct CubeInstancedConstantData {
    BE1::Mat4       viewProjMatrix;
    BE1::Mat3x4     worldMatrix[1024];
};

std::shared_ptr<CubeMesh> CubeMesh::CreateMesh() {
    std::shared_ptr cubeMesh = std::make_shared<CubeMesh>();
    cubeMesh->InitMesh();
    return cubeMesh;
}

void CubeMesh::DestroyMesh(std::shared_ptr<CubeMesh> &cubeMesh) {
    if (cubeMesh) {
        cubeMesh->FreeMesh();
        cubeMesh.reset();
    }
}

void CubeMesh::InitMesh() {
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

    vertexBuffer = renderer->CreateVertexBuffer(RHI::BufferUsage::Default, sizeof(verts[0]), COUNT_OF(verts), (void *)verts);
    indexBuffer = renderer->CreateIndexBuffer(RHI::BufferUsage::Default, sizeof(indexes[0]), COUNT_OF(indexes), (void *)indexes);
    texture = renderer->CreateTextureFromFile(RHI::TextureType::Texture2D, RHI::ResourceFlag::ShaderResource, "Data/EngineTextures/checker.dds");

    InitPipelineState();
}

void CubeMesh::FreeMesh() {
    renderer->DestroyTexture(texture);
    renderer->DestroyVertexBuffer(vertexBuffer);
    renderer->DestroyIndexBuffer(indexBuffer);
    renderer->DestroyPSO(singlePSO);
    renderer->DestroyPSO(instancingPSO);
}

void CubeMesh::InitPipelineState() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, 0, 0, RHI::InputLayoutElement::Format::Float3 },
        { "COLOR", 0, 12, 0, RHI::InputLayoutElement::Format::UByte4N },
        { "TEXCOORD", 0, 16, 0, RHI::InputLayoutElement::Format::Float2 },
    };

    RHI::RenderDest renderDest;
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = app.mainRenderContext->GetMainRTColorFormat();
    renderDest.depthStencilFormat = app.mainRenderContext->GetMainRTDepthFormat();
    renderDest.sampleCount = app.mainRenderContext->GetMainRTSampleCount();

    RHI::Shader *cubeVS = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/Cube.hlsl", "VSMain"));
    RHI::Shader *cubePS = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/Cube.hlsl", "PSMain"));

    if (cubeVS && cubePS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = cubeVS;
        psoDesc.ps = cubePS;
        psoDesc.rasterizerState = renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        singlePSO = renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (cubeVS) {
        renderer->DestroyShader(cubeVS, true);
    }
    if (cubePS) {
        renderer->DestroyShader(cubePS, true);
    }

    RHI::Shader *cubeInstancingVS = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/CubeInstancing.hlsl", "VSMain"));
    RHI::Shader *cubeInstancingPS = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/CubeInstancing.hlsl", "PSMain"));

    if (cubeInstancingVS && cubeInstancingPS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = cubeInstancingVS;
        psoDesc.ps = cubeInstancingPS;
        psoDesc.rasterizerState = renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        instancingPSO = renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (cubeInstancingVS) {
        renderer->DestroyShader(cubeInstancingVS, true);
    }
    if (cubeInstancingPS) {
        renderer->DestroyShader(cubeInstancingPS, true);
    }
}

void CubeMesh::DrawMesh(const RenderContext *renderContext, RHI::CommandList* commandList, const BE1::Mat3x4& worldMatrix) {
    // 한 프레임 동안만 유지되는 다이나믹 상수 버퍼 공간을 할당한다.
    RHI::FrameThreadData *frameThreadData = commandList->GetFrameThreadData();
    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(CubeConstantData));
    if (!constantBuffer) {
        return;
    }

    const RenderFrameData *currentFrameData = renderContext->GetCurrentFrameData();

    CubeConstantData *constantDataPtr = reinterpret_cast<CubeConstantData*>(constantBuffer->writePtr);
    constantDataPtr->viewProjMatrix = currentFrameData->GetVisCamera()->viewProjMatrix;
    constantDataPtr->worldMatrix = worldMatrix;

    renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    renderer->SetIndexBuffer(commandList, indexBuffer);

    renderer->SetPSO(commandList, singlePSO);
    renderer->SetTexture(commandList, 0, false, texture);
    renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    renderer->DrawIndexed(commandList, 36, 0, 0);
}

void CubeMesh::DrawMeshInstanced(const RenderContext *renderContext, RHI::CommandList *commandList, const BE1::Mat3x4 *instanceData, int instanceCount) {
    // 한 프레임 동안만 유지되는 다이나믹 상수 버퍼 공간을 할당한다.
    RHI::FrameThreadData *frameThreadData = commandList->GetFrameThreadData();
    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(CubeInstancedConstantData));
    if (!constantBuffer) {
        return;
    }

    const RenderFrameData *currentFrameData = renderContext->GetCurrentFrameData();

    CubeInstancedConstantData *constantPtr = reinterpret_cast<CubeInstancedConstantData *>(constantBuffer->writePtr);
    constantPtr->viewProjMatrix = currentFrameData->GetVisCamera()->viewProjMatrix;

    for (int i = 0; i < instanceCount; ++i) {
        constantPtr->worldMatrix[i] = instanceData[i];
    }

    renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    renderer->SetIndexBuffer(commandList, indexBuffer);

    renderer->SetPSO(commandList, instancingPSO);
    renderer->SetTexture(commandList, 0, false, texture);
    renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    renderer->DrawIndexedInstanced(commandList, 36, instanceCount, 0, 0, 0);
}
