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
        { { -0.5f, -0.5f, 0.0f }, Color4::green.ToUInt32(), { 1.0f, 1.0f } },
        { { 0.5f, -0.5f, 0.0f }, Color4::blue.ToUInt32(), { 0.0f, 1.0f } },
    };

    ALIGN_AS32 const uint16_t indexes[] = {
        0, 1, 2
    };

    vertexBuffer = renderer->CreateVertexBuffer(RHIRenderer::BufferType::Static, sizeof(verts[0]), COUNT_OF(verts), (void *)verts);
    indexBuffer = renderer->CreateIndexBuffer(RHIRenderer::BufferType::Static, sizeof(indexes[0]), COUNT_OF(indexes), (void *)indexes);
    texture = renderer->CreateTextureFromFile(RHIRenderer::TextureType::Texture2D, "Data/EngineTextures/checker.dds");

    InitPipelineState();
}

void D3D12TriangleMesh::FreeMesh() {
    renderer->DestroyTexture(texture);
    renderer->DestroyVertexBuffer(vertexBuffer);
    renderer->DestroyIndexBuffer(indexBuffer);
    renderer->DestroyPSO(singlePSO);
    renderer->DestroyPSO(instancingPSO);
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

    RHIRenderer::Shader *triangleVS = static_cast<RHIRenderer::Shader *>(renderer->CreateShaderFromFile(RHIRenderer::ShaderModel::SM_6_0, RHIRenderer::ShaderStage::Vertex, "Source/TestD3D12/Shaders/Triangle.hlsl", "VSMain"));
    RHIRenderer::Shader *trianglePS = static_cast<RHIRenderer::Shader *>(renderer->CreateShaderFromFile(RHIRenderer::ShaderModel::SM_6_0, RHIRenderer::ShaderStage::Fragment, "Source/TestD3D12/Shaders/Triangle.hlsl", "PSMain"));

    if (triangleVS && trianglePS) {
        RHIRenderer::PipelineStateDesc psoDesc;
        psoDesc.vs = triangleVS;
        psoDesc.ps = trianglePS;
        psoDesc.rasterizerState = renderer->GetRasterizerState(RHIRenderer::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = renderer->GetDepthStencilState(RHIRenderer::DepthStencilStateType::Default);
        psoDesc.blendState = renderer->GetBlendState(RHIRenderer::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHIRenderer::PrimitiveTopology::TriangleList;
        psoDesc.renderPass = &renderPass;
        singlePSO = renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (triangleVS) {
        renderer->DestroyShader(triangleVS, true);
    }
    if (trianglePS) {
        renderer->DestroyShader(trianglePS, true);
    }

    RHIRenderer::Shader *triangleInstancingVS = static_cast<RHIRenderer::Shader *>(renderer->CreateShaderFromFile(RHIRenderer::ShaderModel::SM_6_0, RHIRenderer::ShaderStage::Vertex, "Source/TestD3D12/Shaders/TriangleInstancing.hlsl", "VSMain"));
    RHIRenderer::Shader *triangleInstancingPS = static_cast<RHIRenderer::Shader *>(renderer->CreateShaderFromFile(RHIRenderer::ShaderModel::SM_6_0, RHIRenderer::ShaderStage::Fragment, "Source/TestD3D12/Shaders/TriangleInstancing.hlsl", "PSMain"));

    if (triangleInstancingVS && triangleInstancingPS) {
        RHIRenderer::PipelineStateDesc psoDesc;
        psoDesc.vs = triangleInstancingVS;
        psoDesc.ps = triangleInstancingPS;
        psoDesc.rasterizerState = renderer->GetRasterizerState(RHIRenderer::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = renderer->GetDepthStencilState(RHIRenderer::DepthStencilStateType::Default);
        psoDesc.blendState = renderer->GetBlendState(RHIRenderer::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHIRenderer::PrimitiveTopology::TriangleList;
        psoDesc.renderPass = &renderPass;
        instancingPSO = renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (triangleInstancingVS) {
        renderer->DestroyShader(triangleInstancingVS, true);
    }
    if (triangleInstancingPS) {
        renderer->DestroyShader(triangleInstancingPS, true);
    }
}

void D3D12TriangleMesh::DrawMesh(RHIRenderer::CommandList *commandList, const Vec2 &offset) {
    int threadIndex = commandList->GetThreadIndex();

    // 상수 버퍼 공간을 할당한다.
    RHIRenderer::GPUSubResource *cbSubResource = renderer->GetCurrentFrameData()->AllocConstant(threadIndex, sizeof(TriangleConstantData));
    if (!cbSubResource) {
        return;
    }

    TriangleConstantData *constantDataPtr = reinterpret_cast<TriangleConstantData*>(cbSubResource->writePtr);
    constantDataPtr->offset.x = offset.x;
    constantDataPtr->offset.y = offset.y;

    renderer->SetTexture(commandList, 0, texture);
    renderer->SetSubResource(commandList, 1, cbSubResource);
    renderer->SetPSO(commandList, singlePSO);
    renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    renderer->SetIndexBuffer(commandList, indexBuffer);
    renderer->DrawIndexed(commandList, 3, 0, 0);
}

void D3D12TriangleMesh::DrawMeshInstanced(RHIRenderer::CommandList *commandList, const Vec2 *instanceData, int instanceCount) {
    int threadIndex = commandList->GetThreadIndex();

    // 상수 버퍼 공간을 할당한다.
    RHIRenderer::GPUSubResource *cbSubResource = renderer->GetCurrentFrameData()->AllocConstant(threadIndex, sizeof(TriangleInstancedConstantData));
    if (!cbSubResource) {
        return;
    }

    TriangleInstancedConstantData *constantDataPtr = reinterpret_cast<TriangleInstancedConstantData *>(cbSubResource->writePtr);
    for (int i = 0; i < instanceCount; ++i) {
        constantDataPtr->offset[i] = Vec4(instanceData[i], Vec2::zero);
    }

    renderer->SetTexture(commandList, 0, texture);
    renderer->SetSubResource(commandList, 1, cbSubResource);
    renderer->SetPSO(commandList, instancingPSO);
    renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    renderer->SetIndexBuffer(commandList, indexBuffer);
    renderer->DrawIndexedInstanced(commandList, 3, instanceCount, 0, 0, 0);
}
