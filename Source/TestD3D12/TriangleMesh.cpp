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
#include "TriangleMesh.h"
#include "App.h"
#include "RenderContext.h"

struct TriangleVertex {
    BE1::Vec3       position;
    uint32_t        color;
    BE1::float16_t  texCoord[2];
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
        { {  0.0f,  0.5f, 0.0f }, BE1::Color4::red.ToUInt32(),   { BE1::F16Converter::FromF32(0.5f), BE1::F16Converter::FromF32(0.0f) }},
        { { -0.5f, -0.5f, 0.0f }, BE1::Color4::green.ToUInt32(), { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(1.0f) } },
        { {  0.5f, -0.5f, 0.0f }, BE1::Color4::blue.ToUInt32(),  { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(1.0f) } },
    };

    ALIGN_AS32 const uint16_t indexes[] = {
        0, 1, 2
    };

    vertexBuffer = RHI::renderer->CreateVertexBuffer(RHI::BufferUsage::Default, sizeof(verts[0]), COUNT_OF(verts), (void *)verts);
    indexBuffer = RHI::renderer->CreateIndexBuffer(RHI::BufferUsage::Default, sizeof(indexes[0]), COUNT_OF(indexes), (void *)indexes);
    texture = RHI::renderer->CreateTextureFromFile(RHI::TextureType::Texture2D, RHI::ResourceFlag::ShaderResource, "Data/EngineTextures/checker.dds");

    InitPipelineState();
}

void TriangleMesh::FreeMesh() {
    RHI::renderer->DestroyTexture(texture);
    RHI::renderer->DestroyVertexBuffer(vertexBuffer);
    RHI::renderer->DestroyIndexBuffer(indexBuffer);
    RHI::renderer->DestroyPSO(singlePSO);
    RHI::renderer->DestroyPSO(instancingPSO);
}

void TriangleMesh::InitPipelineState() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, 0, 0, RHI::InputLayoutElement::Format::Float3 },
        { "COLOR", 0, 12, 0, RHI::InputLayoutElement::Format::UByte4N },
        { "TEXCOORD", 0, 16, 0, RHI::InputLayoutElement::Format::Half2 },
    };

    RHI::RenderDest renderDest;
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = app.mainRenderContext->GetMainRTColorFormat();
    renderDest.depthStencilFormat = app.mainRenderContext->GetMainRTDepthFormat();
    renderDest.sampleCount = app.mainRenderContext->GetMainRTSampleCount();

    RHI::Shader *triangleVS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/Triangle.hlsl", "VSMain"));
    RHI::Shader *trianglePS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/Triangle.hlsl", "PSMain"));

    if (triangleVS && trianglePS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = triangleVS;
        psoDesc.ps = trianglePS;
        psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        singlePSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (triangleVS) {
        RHI::renderer->DestroyShader(triangleVS, true);
    }
    if (trianglePS) {
        RHI::renderer->DestroyShader(trianglePS, true);
    }

    RHI::Shader *triangleInstancingVS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/TriangleInstancing.hlsl", "VSMain"));
    RHI::Shader *triangleInstancingPS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/TriangleInstancing.hlsl", "PSMain"));

    if (triangleInstancingVS && triangleInstancingPS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = triangleInstancingVS;
        psoDesc.ps = triangleInstancingPS;
        psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        instancingPSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (triangleInstancingVS) {
        RHI::renderer->DestroyShader(triangleInstancingVS, true);
    }
    if (triangleInstancingPS) {
        RHI::renderer->DestroyShader(triangleInstancingPS, true);
    }
}

void TriangleMesh::DrawMesh(RHI::CommandList *commandList, const VisCamera *visCamera, const BE1::Vec2 &offset) {
    RHI::FrameThreadData *frameThreadData = commandList->GetFrameThreadData();
    // 한 프레임 동안만 유지되는 다이나믹 상수 버퍼 공간을 할당한다.
    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(TriangleConstantData));
    if (!constantBuffer) {
        return;
    }

    // 삼각형의 위치를 기록
    TriangleConstantData *constantDataPtr = reinterpret_cast<TriangleConstantData *>(constantBuffer->writePtr);
    constantDataPtr->offset.x = offset.x;
    constantDataPtr->offset.y = offset.y;

    RHI::renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    RHI::renderer->SetIndexBuffer(commandList, indexBuffer);

    RHI::renderer->SetPSO(commandList, singlePSO);
    RHI::renderer->SetTexture(commandList, 0, false, texture);
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexed(commandList, 3, 0, 0);
}

void TriangleMesh::DrawMeshInstanced(RHI::CommandList *commandList, const VisCamera *visCamera, const BE1::Vec2 *instanceData, int instanceCount) {
    RHI::FrameThreadData *frameThreadData = commandList->GetFrameThreadData();
    // 한 프레임 동안만 유지되는 다이나믹 상수 버퍼 공간을 할당한다.
    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(TriangleInstancedConstantData));
    if (!constantBuffer) {
        return;
    }

    // 삼각형 인스턴스들의 위치를 기록
    TriangleInstancedConstantData *constantDataPtr = reinterpret_cast<TriangleInstancedConstantData *>(constantBuffer->writePtr);
    for (int i = 0; i < instanceCount; ++i) {
        constantDataPtr->offset[i] = BE1::Vec4(instanceData[i], BE1::Vec2::zero);
    }

    RHI::renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    RHI::renderer->SetIndexBuffer(commandList, indexBuffer);

    RHI::renderer->SetPSO(commandList, instancingPSO);
    RHI::renderer->SetTexture(commandList, 0, false, texture);
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);
    
    RHI::renderer->DrawIndexedInstanced(commandList, 3, instanceCount, 0, 0, 0);
}
