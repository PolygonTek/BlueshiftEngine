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
#include "RenderContext.h"
#include "RenderInternal.h"

struct CubeVertex {
    BE1::Vec3       position;
    uint32_t        color;
    BE1::float16_t  texCoord[2];
};

// 상수 버퍼는 16 바이트 정렬을 요구한다.
struct ALIGN_AS16 CubeConstantData {
    BE1::Mat4       modelViewProjMatrix;
};

struct ALIGN_AS16 CubeInstancedConstantData {
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
        { { -1.0f, -1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(1.0f) } },
        { { -1.0f,  1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(1.0f) } },
        { {  1.0f,  1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(0.0f) } },
        { {  1.0f, -1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(0.0f) } },

        { {  1.0f, -1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(1.0f) } },
        { {  1.0f,  1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(1.0f) } },
        { { -1.0f,  1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(0.0f) } },
        { { -1.0f, -1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(0.0f) } },

        { {  1.0f, -1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(1.0f) } },
        { {  1.0f,  1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(1.0f) } },
        { {  1.0f,  1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(0.0f) } },
        { {  1.0f, -1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(0.0f) } },

        { {  1.0f,  1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(1.0f) } },
        { { -1.0f,  1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(1.0f) } },
        { { -1.0f,  1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(0.0f) } },
        { {  1.0f,  1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(0.0f) } },

        { { -1.0f,  1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(1.0f) } },
        { { -1.0f, -1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(1.0f) } },
        { { -1.0f, -1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(0.0f) } },
        { { -1.0f,  1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(0.0f) } },

        { { -1.0f, -1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(1.0f) } },
        { {  1.0f, -1.0f, -1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(1.0f) } },
        { {  1.0f, -1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(1.0f), BE1::F16Converter::FromF32(0.0f) } },
        { { -1.0f, -1.0f,  1.0f }, 0xffffffff, { BE1::F16Converter::FromF32(0.0f), BE1::F16Converter::FromF32(0.0f) } },
    };

    ALIGN_AS32 const uint16_t indexes[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };

    vertexBuffer = RHI::renderer->CreateVertexBuffer(RHI::BufferUsage::Default, sizeof(verts[0]), COUNT_OF(verts), (void *)verts);
    indexBuffer = RHI::renderer->CreateIndexBuffer(RHI::BufferUsage::Default, sizeof(indexes[0]), COUNT_OF(indexes), (void *)indexes);
    texture = RHI::renderer->CreateTextureFromFile(RHI::TextureType::Texture2D, RHI::ResourceFlag::ShaderResource, "Data/EngineTextures/a.bmp", false);

    InitPipelineState();
}

void CubeMesh::FreeMesh() {
    RHI::renderer->DestroyTexture(texture);
    RHI::renderer->DestroyVertexBuffer(vertexBuffer);
    RHI::renderer->DestroyIndexBuffer(indexBuffer);
    RHI::renderer->DestroyPSO(singlePSO);
    RHI::renderer->DestroyPSO(instancingPSO);
}

BE1::AABB CubeMesh::GetAABB() const {
    return BE1::AABB(BE1::Vec3(-1.0f, -1.0f, -1.0f), BE1::Vec3(1.0f, 1.0f, 1.0f));
}

void CubeMesh::InitPipelineState() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, 0, 0, RHI::InputLayoutElement::Format::Float3 },
        { "COLOR", 0, 12, 0, RHI::InputLayoutElement::Format::UByte4N },
        { "TEXCOORD", 0, 16, 0, RHI::InputLayoutElement::Format::Half2 },
    };

    RHI::RenderDest renderDest;
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = app.GetMainRenderContext()->GetMainRTColorFormat();
    renderDest.depthStencilFormat = app.GetMainRenderContext()->GetMainRTDepthFormat();
    renderDest.sampleCount = app.GetMainRenderContext()->GetMainRTSampleCount();

    RHI::Shader *cubeVS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/Cube.hlsl", "VSMain"));
    RHI::Shader *cubePS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/Cube.hlsl", "PSMain"));

    if (cubeVS && cubePS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = cubeVS;
        psoDesc.ps = cubePS;
        psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        singlePSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (cubeVS) {
        RHI::renderer->DestroyShader(cubeVS, true);
    }
    if (cubePS) {
        RHI::renderer->DestroyShader(cubePS, true);
    }

    RHI::Shader *cubeInstancingVS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/CubeInstancing.hlsl", "VSMain"));
    RHI::Shader *cubeInstancingPS = static_cast<RHI::Shader *>(RHI::renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/CubeInstancing.hlsl", "PSMain"));

    if (cubeInstancingVS && cubeInstancingPS) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = cubeInstancingVS;
        psoDesc.ps = cubeInstancingPS;
        psoDesc.rasterizerState = RHI::renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = RHI::renderer->GetDepthStencilState(RHI::DepthStencilStateType::Default);
        psoDesc.blendState = RHI::renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.inputLayout = &inputLayout;
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        instancingPSO = RHI::renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (cubeInstancingVS) {
        RHI::renderer->DestroyShader(cubeInstancingVS, true);
    }
    if (cubeInstancingPS) {
        RHI::renderer->DestroyShader(cubeInstancingPS, true);
    }
}

void CubeMesh::DrawMesh(RHI::CommandList* commandList, const VisCamera *visCamera, const BE1::Mat3x4& worldMatrix) {
    RHI::FrameThreadData *frameThreadData = commandList->GetFrameThreadData();
    // 한 프레임 동안만 유지되는 다이나믹 상수 버퍼 공간을 할당한다.
    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(CubeConstantData));
    if (!constantBuffer) {
        return;
    }

    CubeConstantData *constantDataPtr = reinterpret_cast<CubeConstantData *>(constantBuffer->writePtr);

    // 큐브의 MVP 행렬을 기록
    constantDataPtr->modelViewProjMatrix = visCamera->viewProjMatrix * worldMatrix;

    RHI::renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    RHI::renderer->SetIndexBuffer(commandList, indexBuffer);

    RHI::renderer->SetPSO(commandList, singlePSO);
    RHI::renderer->SetTexture(commandList, 0, false, texture);
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexed(commandList, 36, 0, 0);
}

void CubeMesh::DrawMeshInstanced(RHI::CommandList *commandList, const VisCamera *visCamera, const BE1::Mat3x4 *instanceData, int instanceCount) {
    RHI::FrameThreadData *frameThreadData = commandList->GetFrameThreadData();
    // 한 프레임 동안만 유지되는 다이나믹 상수 버퍼 공간을 할당한다.
    RHI::ConstantBuffer *constantBuffer = frameThreadData->AllocConstant(sizeof(CubeInstancedConstantData));
    if (!constantBuffer) {
        return;
    }

    CubeInstancedConstantData *constantDataPtr = reinterpret_cast<CubeInstancedConstantData *>(constantBuffer->writePtr);

    // 카메라의 뷰-프로젝션 행렬을 기록
    constantDataPtr->viewProjMatrix = visCamera->viewProjMatrix;

    // 큐브 인스턴스들의 월드 행렬을 기록
    for (int i = 0; i < instanceCount; ++i) {
        constantDataPtr->worldMatrix[i] = instanceData[i];
    }

    RHI::renderer->SetVertexBuffer(commandList, 0, vertexBuffer);
    RHI::renderer->SetIndexBuffer(commandList, indexBuffer);

    RHI::renderer->SetPSO(commandList, instancingPSO);
    RHI::renderer->SetTexture(commandList, 0, false, texture);
    RHI::renderer->SetConstantBuffer(commandList, 0, constantBuffer);

    RHI::renderer->DrawIndexedInstanced(commandList, 36, instanceCount, 0, 0, 0);
}
