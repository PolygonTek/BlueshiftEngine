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
#include "TriangleMesh.h"

struct TriangleVertex {
    BE1::Vec3       position;
    uint32_t        color;
    BE1::Vec2       texCoord;
};

struct TriangleConstantData {
    BE1::Vec4       offset;
};

struct TriangleInstancedConstantData {
    BE1::Vec4       offset[1024];
};

std::shared_ptr<TriangleMesh> TriangleMesh::CreateMesh() {
    std::shared_ptr triangleMesh = std::make_shared<TriangleMesh>();
    triangleMesh->InitMesh();
    return triangleMesh;
}

void TriangleMesh::DestroyMesh(std::shared_ptr<TriangleMesh>& triangleMesh) {
    if (triangleMesh) {
        triangleMesh->FreeMesh();
        triangleMesh.reset();
    }
}

void TriangleMesh::InitMesh() {
    // 삼각형의 버텍스/인덱스 버퍼 내용을 작성
    // NOTE: UV 좌표의 V 는 아래쪽으로 증가함을 주의한다. 나중에 통합 렌더러를 작성한다면, shader code 에서 하는게 좋을 듯..
    ALIGN_AS32 const TriangleVertex verts[] = {
        { { 0.0f, 0.5f, 0.0f }, BE1::Color4::red.ToUInt32(), { 0.5f, 0.0f }},
        { { -0.5f, -0.5f, 0.0f }, BE1::Color4::green.ToUInt32(), { 1.0f, 1.0f } },
        { { 0.5f, -0.5f, 0.0f }, BE1::Color4::blue.ToUInt32(), { 0.0f, 1.0f } },
    };

    ALIGN_AS32 const uint16_t indexes[] = {
        0, 1, 2
    };

    vertexBuffer = renderer->CreateVertexBuffer(RHI::BufferUsage::Default, sizeof(verts[0]), COUNT_OF(verts), (void *)verts);
    indexBuffer = renderer->CreateIndexBuffer(RHI::BufferUsage::Default, sizeof(indexes[0]), COUNT_OF(indexes), (void *)indexes);
    texture = renderer->CreateTextureFromFile(RHI::TextureType::Texture2D, RHI::ResourceFlag::ShaderResource, "Data/EngineTextures/checker.dds");

    InitPipelineState();
}

void TriangleMesh::FreeMesh() {
    renderer->DestroyTexture(texture);
    renderer->DestroyVertexBuffer(vertexBuffer);
    renderer->DestroyIndexBuffer(indexBuffer);
    renderer->DestroyPSO(singlePSO);
    renderer->DestroyPSO(instancingPSO);
}

void TriangleMesh::InitPipelineState() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, 0, 0, RHI::InputLayoutElement::Format::Float3 },
        { "COLOR", 0, 12, 0, RHI::InputLayoutElement::Format::UByte4N },
        { "TEXCOORD", 0, 16, 0, RHI::InputLayoutElement::Format::Float2 },
    };

    RHI::RenderDest renderDest;
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = renderer->GetMainRTColorFormat();
    renderDest.depthStencilFormat = renderer->GetMainRTDepthFormat();
    renderDest.sampleCount = renderer->GetMainRTSampleCount();

    RHI::Shader *triangleVS = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/Triangle.hlsl", "VSMain"));
    RHI::Shader *trianglePS = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/Triangle.hlsl", "PSMain"));

    if (triangleVS && trianglePS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = triangleVS;
        psoDesc.ps = trianglePS;
        psoDesc.rasterizerState = renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        singlePSO = renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (triangleVS) {
        renderer->DestroyShader(triangleVS, true);
    }
    if (trianglePS) {
        renderer->DestroyShader(trianglePS, true);
    }

    RHI::Shader *triangleInstancingVS = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/TriangleInstancing.hlsl", "VSMain"));
    RHI::Shader *triangleInstancingPS = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/TriangleInstancing.hlsl", "PSMain"));

    if (triangleInstancingVS && triangleInstancingPS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = triangleInstancingVS;
        psoDesc.ps = triangleInstancingPS;
        psoDesc.rasterizerState = renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        instancingPSO = renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (triangleInstancingVS) {
        renderer->DestroyShader(triangleInstancingVS, true);
    }
    if (triangleInstancingPS) {
        renderer->DestroyShader(triangleInstancingPS, true);
    }
}

void TriangleMesh::DrawMesh(RHI::CommandList *commandList, const BE1::Vec2 &offset) {
    int threadIndex = commandList->GetThreadIndex();

    // 다이나믹 상수 버퍼 공간을 할당한다.
    RHI::ConstantBuffer *constantBuffer = renderer->GetCurrentFrameData()->AllocConstant(threadIndex, sizeof(TriangleConstantData));
    if (!constantBuffer) {
        return;
    }

    TriangleConstantData *constantDataPtr = reinterpret_cast<TriangleConstantData*>(constantBuffer->writePtr);
    constantDataPtr->offset.x = offset.x;
    constantDataPtr->offset.y = offset.y;

    renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    renderer->SetIndexBuffer(commandList, indexBuffer);

    renderer->SetPSO(commandList, singlePSO);
    renderer->SetTexture(commandList, 0, false, texture);
    renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    renderer->DrawIndexed(commandList, 3, 0, 0);
}

void TriangleMesh::DrawMeshInstanced(RHI::CommandList *commandList, const BE1::Vec2 *instanceData, int instanceCount) {
    int threadIndex = commandList->GetThreadIndex();

    // 다이나믹 상수 버퍼 공간을 할당한다.
    RHI::ConstantBuffer *constantBuffer = renderer->GetCurrentFrameData()->AllocConstant(threadIndex, sizeof(TriangleInstancedConstantData));
    if (!constantBuffer) {
        return;
    }

    TriangleInstancedConstantData *constantDataPtr = reinterpret_cast<TriangleInstancedConstantData *>(constantBuffer->writePtr);
    for (int i = 0; i < instanceCount; ++i) {
        constantDataPtr->offset[i] = BE1::Vec4(instanceData[i], BE1::Vec2::zero);
    }

    renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    renderer->SetIndexBuffer(commandList, indexBuffer);

    renderer->SetPSO(commandList, instancingPSO);
    renderer->SetTexture(commandList, 0, false, texture);
    renderer->SetConstantBuffer(commandList, 0, constantBuffer);
    
    renderer->DrawIndexedInstanced(commandList, 3, instanceCount, 0, 0, 0);
}
