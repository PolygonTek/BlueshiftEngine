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
#include "Platform/PlatformSystem.h"
#include "Platform/PlatformFile.h"
#include "Platform/Windows/PlatformWinUtils.h"
#include "D3D12Renderer.h"

void D3D12Shader::Release() {
    SAFE_RELEASE(compiledShaderBlob);
    SAFE_RELEASE(rootSignature);
}

Str D3D12Renderer::shaderCacheDir;

bool D3D12Renderer::LoadCompiledShader(const char *name, const uint64_t hash, ID3DBlob **compiledShaderBlob) {
    Str filename;// = shaderCacheDir;
    filename.AppendPath(name);
    filename.SetFileExtension(".cso");

    PlatformFileMapping *fileMapping = PlatformFileMapping::OpenFileRead(filename);
    if (!fileMapping) {
        return false;
    }

    const byte *fileData = (const byte *)fileMapping->GetData();
    // 저장된 cso 파일과 hash 값이 같은지 비교한다.
    if (*(uint64_t *)fileData != hash) {
        delete fileMapping;
        return false;
    }

    *compiledShaderBlob = new D3D12CompiledShaderBlob(fileData, fileMapping->GetSize());
    delete fileMapping;

    return true;
}

void D3D12Renderer::WriteCompiledShader(const char *name, const uint64_t hash, ID3DBlob *compiledShaderBlob) {
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

    int fileDataSize = compiledShaderBlob->GetBufferSize() + sizeof(uint64_t);
    byte *fileData = (byte *)Mem_Alloc32(fileDataSize);

    // 캐싱된 cso 파일의 첫 64 비트는 hash 값을 저장한다.
    *(uint64_t *)fileData = hash;
    memcpy(fileData + sizeof(uint64_t), compiledShaderBlob->GetBufferPointer(), compiledShaderBlob->GetBufferSize());

    file->Write(fileData, fileDataSize);

    Mem_AlignedFree(fileData);
    delete file;
}

RHIRenderer::Shader *D3D12Renderer::CreateShader(ShaderStage shaderStage, const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint) {
    LPCSTR target = nullptr;
    switch (shaderStage) {
    case ShaderStage::Vertex:
        target = "vs_5_0";
        break;
    case ShaderStage::Fragment:
        target = "ps_5_0";
        break;
    case ShaderStage::Geometry:
        target = "gs_5_0";
        break;
    case ShaderStage::Compute:
        target = "cs_5_0";
        break;
    default:
        return nullptr;
    }

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

    ID3DBlob *compiledShaderBlob = nullptr;

    // 이미 컴파일된 cso 파일을 로드해본다.
    const uint64_t shaderTextHash = CityHash64(shaderText, shaderTextSize);
    bool shouldCompileShader = !LoadCompiledShader(fileName, shaderTextHash, &compiledShaderBlob);

    // hash 값이 다르거나 파일이 없다면 새로 컴파일한다.
    if (shouldCompileShader) {
        UINT compileFlags = 0;
#if defined(_DEBUG)
        compileFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS);
#endif
        ID3DBlob *errorBlob = nullptr;

        if (FAILED(D3DCompile(shaderText, shaderTextSize, sourceName, nullptr, nullptr, entryPoint, target, compileFlags, 0, &compiledShaderBlob, &errorBlob))) {
            PrintCompileErrorMessages(errorBlob);
            SAFE_RELEASE(errorBlob);
            return nullptr;
        }

        // 경고 메시지 출력
        if (errorBlob && errorBlob->GetBufferSize() > 0) {
            PrintCompileErrorMessages(errorBlob);
        }
        SAFE_RELEASE(errorBlob);

        // 컴파일했으므로 cso 파일을 저장한다.
        WriteCompiledShader(fileName, shaderTextHash, compiledShaderBlob);
    }

#if 0
    ID3D12VersionedRootSignatureDeserializer *rootSignatureDeserializer = nullptr;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *rootSignatureDesc = nullptr;

    // shader text 에서 root signature 를 얻기 위한 deserializer 를 생성한다.
    HRESULT hr = D3D12CreateVersionedRootSignatureDeserializer(compiledShaderBlob->GetBufferPointer(), compiledShaderBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignatureDeserializer));
    if (SUCCEEDED(hr)) {
        // deserializer 로 부터 root signature desc 포인터를 얻어낸다. (deserializer 가 파괴될 때 까지 desc 의 메모리는 유지된다)
        rootSignatureDeserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &rootSignatureDesc);
        assert(rootSignatureDesc->Version == D3D_ROOT_SIGNATURE_VERSION_1_1);
    }
#endif

    ID3D12RootSignature *rootSignature = nullptr;
    if (FAILED(renderer.device->CreateRootSignature(0, compiledShaderBlob->GetBufferPointer(), compiledShaderBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)))) {
        return nullptr;
    }

    D3D12Shader *shader = new D3D12Shader;
    shader->shaderStage = shaderStage;
    shader->hash = shaderTextHash;
    shader->compiledShaderBlob = compiledShaderBlob;
    shader->rootSignature = rootSignature;
    return shader;
}

RHIRenderer::Shader *D3D12Renderer::CreateShaderFromFile(ShaderStage shaderStage, const char *filename, const char *entryPoint) {
    char *shaderText;
    int shaderTextSize = fileSystem.LoadFile(filename, true, (void **)&shaderText);
    if (!shaderText) {
        return nullptr;
    }

    Shader *shader = CreateShader(shaderStage, filename, shaderText, shaderTextSize, entryPoint);
    if (!shader) {
        fileSystem.FreeFile(shaderText);
        return nullptr;
    }

    fileSystem.FreeFile(shaderText);
    return shader;
}

void D3D12Renderer::DestroyShader(Shader *shader, bool immediate) {
    if (immediate) {
        delete shader;
    } else {
        MarkForDelete(shader);
    }
}

void D3D12Renderer::PrintCompileErrorMessages(ID3DBlob *errorBlob) {
    if (!errorBlob) {
        BE_WARNLOG("D3DCompile failed, but no error message was provided\n");
    }

    const char *errorMessage = static_cast<const char *>(errorBlob->GetBufferPointer());
    size_t errorMessageLength = errorBlob->GetBufferSize();

    Str errorMessageStr;
    errorMessageStr.EnsureAlloced(errorMessageLength + 1);
    Str::Copynz((char *)errorMessageStr, errorMessage, errorMessageLength + 1);

    BE_WARNLOG(errorMessageStr);
}
