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
#include "D3D12CompiledShaderBlob.h"
#include "Platform/PlatformFile.h"

static Str shaderCacheDir = "Cache/D3D12CompiledShaderCache";

bool D3D12Mesh::LoadCompiledShader(const char *name, const uint32_t hash, ID3DBlob **compiledShaderBlob) {
    Str filename;// = shaderCacheDir;
    filename.AppendPath(name);
    filename.SetFileExtension(".cso");

    PlatformFileMapping *fileMapping = PlatformFileMapping::OpenFileRead(filename);
    if (!fileMapping) {
        return false;
    }

    const byte *fileData = (const byte *)fileMapping->GetData();
    if (*(uint32_t *)fileData != hash) {
        delete fileMapping;
        return false;
    }

    *compiledShaderBlob = new D3D12CompiledShaderBlob(fileData, fileMapping->GetSize());
    delete fileMapping;

    return true;
}

void D3D12Mesh::CacheCompiledShader(const char *name, const uint32_t hash, ID3DBlob *compiledShaderBlob) {
    if (!compiledShaderBlob || compiledShaderBlob->GetBufferSize() == 0) {
        return;
    }

    Str filename;// = shaderCacheDir;
    filename.AppendPath(name);
    filename.SetFileExtension(".cso");
    PlatformFile *file = (PlatformFile *)PlatformFile::OpenFileWrite(filename);
    if (!file) {
        return;
    }

    int fileDataSize = compiledShaderBlob->GetBufferSize() + sizeof(uint32_t);
    byte *fileData = (byte *)Mem_Alloc32(fileDataSize);
    *(uint32_t *)fileData = hash;
    memcpy(fileData + sizeof(uint32_t), compiledShaderBlob->GetBufferPointer(), compiledShaderBlob->GetBufferSize());

    file->Write(fileData, fileDataSize);

    Mem_AlignedFree(fileData);
    delete file;
}

bool D3D12Mesh::CreateShader(const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint, const char *target, ID3DBlob **compiledShaderBlob) {
    Str fileName = sourceName;
    Str fileBase;
    fileName.ExtractFileBase(fileBase);
    char mangledFilename[256];
    Str::snPrintf(mangledFilename, sizeof(mangledFilename), "%s-%s-%s", fileBase.c_str(), entryPoint, target);

    Str extension;
    fileName.ExtractFileExtension(extension);
    fileName.StripFileName();
    fileName.AppendPath(mangledFilename);
    fileName.SetFileExtension(extension);

    // 이미 컴파일된 cso 파일을 로드해본다.
    const uint32_t shaderTextHash = MD5_BlockChecksum(shaderText, shaderTextSize);
    bool shouldCompileShader = !LoadCompiledShader(fileName, shaderTextHash, compiledShaderBlob);

    // hash 값이 다르거나 파일이 없다면 새로 컴파일한다.
    if (shouldCompileShader) {
#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        ID3DBlob *errorBlob = nullptr;

        *compiledShaderBlob = nullptr;

        if (FAILED(D3DCompile(shaderText, shaderTextSize, sourceName, nullptr, nullptr, entryPoint, target, compileFlags, 0, compiledShaderBlob, &errorBlob))) {
            renderer.PrintCompileErrorMessages(errorBlob);
            SAFE_RELEASE(errorBlob);
            return false;
        }

        // 컴파일했으므로 cso 파일을 저장한다.
        CacheCompiledShader(fileName, shaderTextHash, *compiledShaderBlob);

        SAFE_RELEASE(errorBlob);
    }
    return true;
}

bool D3D12Mesh::CreateShaderFromFile(const char *shaderFilename, const char *entryPoint, const char *target, ID3DBlob **compiledShaderBlob) {
    char *shaderText;
    int shaderTextSize = fileSystem.LoadFile(shaderFilename, true, (void **)&shaderText);
    if (!shaderText) {
        return false;
    }

    if (!CreateShader(shaderFilename, shaderText, shaderTextSize, entryPoint, target, compiledShaderBlob)) {
        fileSystem.FreeFile(shaderText);
        return false;
    }

    fileSystem.FreeFile(shaderText);
    return true;
}

bool D3D12Mesh::CreateVertexAndPixelShaderFromFile(const char *shaderFilename, ID3DBlob **compiledVertexShaderBlob, ID3DBlob **compiledPixelShaderBlob) {
    char* shaderText;
    int shaderTextSize = fileSystem.LoadFile(shaderFilename, true, (void **)&shaderText);
    if (!shaderText) {
        return false;
    }

    bool vsCreated = CreateShader(shaderFilename, shaderText, shaderTextSize, "VSMain", "vs_5_0", compiledVertexShaderBlob);
    bool psCreated = CreateShader(shaderFilename, shaderText, shaderTextSize, "PSMain", "ps_5_0", compiledPixelShaderBlob);

    fileSystem.FreeFile(shaderText);

    if (!vsCreated || !psCreated) {
        SAFE_RELEASE(*compiledVertexShaderBlob);
        SAFE_RELEASE(*compiledPixelShaderBlob);
        return false;
    }
    return true;
}

ID3D12PipelineState* D3D12Mesh::CreatePSO(ID3D12RootSignature* rootSignature, const D3D12_SHADER_BYTECODE &compiledVertexShaderByteCode, const D3D12_SHADER_BYTECODE &compiledPixelShaderByteCode, const D3D12_INPUT_LAYOUT_DESC &inputLayout) {
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    };
    ID3D12PipelineState* pso = nullptr;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    // NOTE: 나중에 호출할 SetGraphicsRootSignature() 에서 PSO 에 지정된 RootSignature 와 다르면 안된다.
    // 여기서 RootSignature 를 지정하는 이유는 파이프라인 호환성 검사 및 최적화 때문이다.
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = compiledVertexShaderByteCode;
    psoDesc.PS = compiledPixelShaderByteCode;
    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
        psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
    }
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    psoDesc.DepthStencilState.FrontFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    psoDesc.DepthStencilState.BackFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
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
    ID3DBlob *compiledVertexShaderBlob = nullptr;
    ID3DBlob *compiledPixelShaderBlob = nullptr;

    if (!CreateVertexAndPixelShaderFromFile(shaderFilename, &compiledVertexShaderBlob, &compiledPixelShaderBlob)) {
        return nullptr;
    }

    D3D12_SHADER_BYTECODE compiledVertexShaderByteCode = CD3DX12_SHADER_BYTECODE(compiledVertexShaderBlob->GetBufferPointer(), compiledVertexShaderBlob->GetBufferSize());
    D3D12_SHADER_BYTECODE compiledPixelShaderByteCode = CD3DX12_SHADER_BYTECODE(compiledPixelShaderBlob->GetBufferPointer(), compiledPixelShaderBlob->GetBufferSize());

    ID3D12PipelineState *pso = CreatePSO(rootSignature, compiledVertexShaderByteCode, compiledPixelShaderByteCode, inputLayout);

    SAFE_RELEASE(compiledVertexShaderBlob);
    SAFE_RELEASE(compiledPixelShaderBlob);

    return pso;
}
