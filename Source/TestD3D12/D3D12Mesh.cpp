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
#include "D3D12Mesh.h"
#include "D3D12Renderer.h"

bool D3D12Mesh::CreateShader(const char *shaderText, int shaderTextSize, ID3DBlob **compiledVertexShader, ID3DBlob **compiledPixelShader) {
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    *compiledVertexShader = nullptr;
    *compiledPixelShader = nullptr;
    ID3DBlob *errorBlob = nullptr;

    if (FAILED(D3DCompile(shaderText, shaderTextSize, "shaderText", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, compiledVertexShader, &errorBlob))) {
        renderer.PrintCompileErrorMessages(errorBlob);
        SAFE_RELEASE(errorBlob);
        return false;
    }
    SAFE_RELEASE(errorBlob);

    if (FAILED(D3DCompile(shaderText, shaderTextSize, "shaderText", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, compiledPixelShader, &errorBlob))) {
        renderer.PrintCompileErrorMessages(errorBlob);
        SAFE_RELEASE(errorBlob);
        return false;
    }
    SAFE_RELEASE(errorBlob);
    return true;
}

bool D3D12Mesh::CreateShaderFromFile(const char *shaderFilename, ID3DBlob **compiledVertexShader, ID3DBlob **compiledPixelShader) {
    char* shaderText;
    int shaderTextSize = fileSystem.LoadFile(shaderFilename, true, (void **)&shaderText);
    if (!shaderText) {
        return false;
    }

    bool ret = CreateShader(shaderText, shaderTextSize, compiledVertexShader, compiledPixelShader);

    fileSystem.FreeFile(shaderText);
    return ret;
}

ID3D12PipelineState* D3D12Mesh::CreatePSO(ID3D12RootSignature* rootSignature, ID3DBlob *compiledVertexShader, ID3DBlob *compiledPixelShader, const D3D12_INPUT_LAYOUT_DESC &inputLayout) {
    ID3D12PipelineState* pso = nullptr;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    // NOTE: 나중에 호출할 SetGraphicsRootSignature() 에서 PSO 에 지정된 RootSignature 와 다르면 안된다.
    // 여기서 RootSignature 를 지정하는 이유는 파이프라인 호환성 검사 및 최적화 때문이다.
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(compiledVertexShader->GetBufferPointer(), compiledVertexShader->GetBufferSize());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(compiledPixelShader->GetBufferPointer(), compiledPixelShader->GetBufferSize());
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.InputLayout = inputLayout;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc.Count = 1;
    renderer.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    return pso;
}

ID3D12PipelineState *D3D12Mesh::CreatePSO(ID3D12RootSignature *rootSignature, const char *shaderFilename, const D3D12_INPUT_LAYOUT_DESC &inputLayout) {
    ID3DBlob *compiledVertexShader = nullptr;
    ID3DBlob *compiledPixelShader = nullptr;

    if (!CreateShaderFromFile(shaderFilename, &compiledVertexShader, &compiledPixelShader)) {
        return nullptr;
    }

    ID3D12PipelineState *pso = CreatePSO(rootSignature, compiledVertexShader, compiledPixelShader, inputLayout);

    SAFE_RELEASE(compiledVertexShader);
    SAFE_RELEASE(compiledPixelShader);

    return pso;
}
