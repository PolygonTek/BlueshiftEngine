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
#include "Platform/Windows/PlatformWinUtils.h"
#include "D3D12Renderer.h"
#include "D3D12Shader.h"

void D3D12Shader::Release() {
    SAFE_MEM_FREE(compiledShaderData);
    SAFE_RELEASE(rootSignatureDeserializer);
    SAFE_RELEASE(rootSignature);
}

bool D3D12Renderer::CompileShader(const RHI::ShaderCompileInput *compileInput, RHI::ShaderCompileOutput *compileOutput) {
    switch (compileInput->shaderFormat) {
    case RHI::ShaderFormat::HLSL5:
        return CompileShaderD3D(compileInput, compileOutput);
    case RHI::ShaderFormat::HLSL6:
        return CompileShaderDXC(compileInput, compileOutput);
    }
    return false;
}

bool D3D12Renderer::CompileShaderD3D(const RHI::ShaderCompileInput *compileInput, RHI::ShaderCompileOutput *compileOutput) {
    assert(compileInput->shaderModel <= RHI::ShaderModel::SM_5_0);

    LPCSTR target = nullptr;
    switch (compileInput->shaderStage) {
    case RHI::ShaderStage::Vertex:
        target = "vs_5_0";
        break;
    case RHI::ShaderStage::Hull:
        target = "hs_5_0";
        break;
    case RHI::ShaderStage::Domain:
        target = "ds_5_0";
        break;
    case RHI::ShaderStage::Geometry:
        target = "gs_5_0";
        break;
    case RHI::ShaderStage::Fragment:
        target = "ps_5_0";
        break;
    case RHI::ShaderStage::Compute:
        target = "cs_5_0";
        break;
    default:
        return false;
    }

    UINT compileFlags = 0;
#if defined(_DEBUG)
    compileFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS);
#endif
    ID3DBlob *errorBlob = nullptr;
    ID3DBlob *compiledShaderBlob = nullptr;

    HRESULT hr = D3DCompile(compileInput->shaderText, compileInput->shaderTextSize, compileInput->sourceName, nullptr, nullptr, compileInput->entryPoint, target, compileFlags, 0, &compiledShaderBlob, &errorBlob);
    if (errorBlob && errorBlob->GetBufferSize() > 0) {
        compileOutput->errorMessage = static_cast<const char *>(errorBlob->GetBufferPointer());
        errorBlob->Release();
    }

    if (FAILED(hr) || !compiledShaderBlob) {
        return false;
    }

    compileOutput->compiledShaderDataSize = compiledShaderBlob->GetBufferSize();
    compileOutput->compiledShaderData = (byte *)Mem_Alloc(compiledShaderBlob->GetBufferSize());
    memcpy(compileOutput->compiledShaderData, compiledShaderBlob->GetBufferPointer(), compiledShaderBlob->GetBufferSize());
    compiledShaderBlob->Release();
    return true;
}

bool D3D12Renderer::CompileShaderDXC(const RHI::ShaderCompileInput *compileInput, RHI::ShaderCompileOutput *compileOutput) {
    std::vector<std::wstring> args = {
        L"-res-may-alias",
        //L"-flegacy-macro-expansion",
        //L"-no-legacy-cbuf-layout",
        //L"-pack-optimized", // this has problem with tessellation shaders: https://github.com/microsoft/DirectXShaderCompiler/issues/3362
        //L"-all-resources-bound",
        //L"-Gis", // Force IEEE strictness
        //L"-Gec", // Enable backward compatibility mode
        //L"-Ges", // Enable strict mode
        //L"-O0", // Optimization Level 0
        //L"-enable-16bit-types",
        L"-Wno-conversion",
    };

#if defined(_DEBUG)
    // Disable optimization
    args.push_back(L"-Od");
#endif

    // only valid in HLSL6 compiler
    args.push_back(L"-Qstrip_reflect");

    //args.push_back(L"-rootsig-define");
    //args.push_back(L"DEFAULT_ROOTSIGNATURE");

    args.push_back(L"-T");

    switch (compileInput->shaderStage) {
    case RHI::ShaderStage::Vertex:
        switch (compileInput->shaderModel) {
        case RHI::ShaderModel::SM_6_7:
            args.push_back(L"vs_6_7");
            break;
        case RHI::ShaderModel::SM_6_6:
            args.push_back(L"vs_6_6");
            break;
        case RHI::ShaderModel::SM_6_5:
            args.push_back(L"vs_6_5");
            break;
        case RHI::ShaderModel::SM_6_4:
            args.push_back(L"vs_6_4");
            break;
        case RHI::ShaderModel::SM_6_3:
            args.push_back(L"vs_6_3");
            break;
        case RHI::ShaderModel::SM_6_2:
            args.push_back(L"vs_6_2");
            break;
        case RHI::ShaderModel::SM_6_1:
            args.push_back(L"vs_6_1");
            break;
        case RHI::ShaderModel::SM_6_0:
        default:
            args.push_back(L"vs_6_0");
            break;
        }
        break;
    case RHI::ShaderStage::Hull:
        switch (compileInput->shaderModel) {
        case RHI::ShaderModel::SM_6_7:
            args.push_back(L"hs_6_7");
            break;
        case RHI::ShaderModel::SM_6_6:
            args.push_back(L"hs_6_6");
            break;
        case RHI::ShaderModel::SM_6_5:
            args.push_back(L"hs_6_5");
            break;
        case RHI::ShaderModel::SM_6_4:
            args.push_back(L"hs_6_4");
            break;
        case RHI::ShaderModel::SM_6_3:
            args.push_back(L"hs_6_3");
            break;
        case RHI::ShaderModel::SM_6_2:
            args.push_back(L"hs_6_2");
            break;
        case RHI::ShaderModel::SM_6_1:
            args.push_back(L"hs_6_1");
            break;
        case RHI::ShaderModel::SM_6_0:
        default:
            args.push_back(L"hs_6_0");
            break;
        }
        break;
    case RHI::ShaderStage::Domain:
        switch (compileInput->shaderModel) {
        case RHI::ShaderModel::SM_6_7:
            args.push_back(L"ds_6_7");
            break;
        case RHI::ShaderModel::SM_6_6:
            args.push_back(L"ds_6_6");
            break;
        case RHI::ShaderModel::SM_6_5:
            args.push_back(L"ds_6_5");
            break;
        case RHI::ShaderModel::SM_6_4:
            args.push_back(L"ds_6_4");
            break;
        case RHI::ShaderModel::SM_6_3:
            args.push_back(L"ds_6_3");
            break;
        case RHI::ShaderModel::SM_6_2:
            args.push_back(L"ds_6_2");
            break;
        case RHI::ShaderModel::SM_6_1:
            args.push_back(L"ds_6_1");
            break;
        case RHI::ShaderModel::SM_6_0:
        default:
            args.push_back(L"ds_6_0");
            break;
        }
        break;
    case RHI::ShaderStage::Geometry:
        switch (compileInput->shaderModel) {
        case RHI::ShaderModel::SM_6_7:
            args.push_back(L"gs_6_7");
            break;
        case RHI::ShaderModel::SM_6_6:
            args.push_back(L"gs_6_6");
            break;
        case RHI::ShaderModel::SM_6_5:
            args.push_back(L"gs_6_5");
            break;
        case RHI::ShaderModel::SM_6_4:
            args.push_back(L"gs_6_4");
            break;
        case RHI::ShaderModel::SM_6_3:
            args.push_back(L"gs_6_3");
            break;
        case RHI::ShaderModel::SM_6_2:
            args.push_back(L"gs_6_2");
            break;
        case RHI::ShaderModel::SM_6_1:
            args.push_back(L"gs_6_1");
            break;
        case RHI::ShaderModel::SM_6_0:
        default:
            args.push_back(L"gs_6_0");
            break;
        }
        break;
    case RHI::ShaderStage::Fragment:
        switch (compileInput->shaderModel) {
        case RHI::ShaderModel::SM_6_7:
            args.push_back(L"ps_6_7");
            break;
        case RHI::ShaderModel::SM_6_6:
            args.push_back(L"ps_6_6");
            break;
        case RHI::ShaderModel::SM_6_5:
            args.push_back(L"ps_6_5");
            break;
        case RHI::ShaderModel::SM_6_4:
            args.push_back(L"ps_6_4");
            break;
        case RHI::ShaderModel::SM_6_3:
            args.push_back(L"ps_6_3");
            break;
        case RHI::ShaderModel::SM_6_2:
            args.push_back(L"ps_6_2");
            break;
        case RHI::ShaderModel::SM_6_1:
            args.push_back(L"ps_6_1");
            break;
        case RHI::ShaderModel::SM_6_0:
        default:
            args.push_back(L"ps_6_0");
            break;
        }
        break;
    case RHI::ShaderStage::Compute:
        switch (compileInput->shaderModel) {
        case RHI::ShaderModel::SM_6_7:
            args.push_back(L"cs_6_7");
            break;
        case RHI::ShaderModel::SM_6_6:
            args.push_back(L"cs_6_6");
            break;
        case RHI::ShaderModel::SM_6_5:
            args.push_back(L"cs_6_5");
            break;
        case RHI::ShaderModel::SM_6_4:
            args.push_back(L"cs_6_4");
            break;
        case RHI::ShaderModel::SM_6_3:
            args.push_back(L"cs_6_3");
            break;
        case RHI::ShaderModel::SM_6_2:
            args.push_back(L"cs_6_2");
            break;
        case RHI::ShaderModel::SM_6_1:
            args.push_back(L"cs_6_1");
            break;
        case RHI::ShaderModel::SM_6_0:
        default:
            args.push_back(L"cs_6_0");
            break;
        }
        break;
    default:
        return false;
    }

    // 상대 경로 추가
    for (const BE1::Str &includeDir : compileInput->includeDirs) {
        args.push_back(L"-I");
        wchar_t wIncludeDir[1024];
        BE1::PlatformWinUtils::UTF8ToUCS2(includeDir, wIncludeDir, COUNT_OF(wIncludeDir));
        args.push_back(wIncludeDir);
    }

    // 디파인 추가
    for (const BE1::Str &define : compileInput->defines) {
        args.push_back(L"-D");
        wchar_t wDefine[256];
        BE1::PlatformWinUtils::UTF8ToUCS2(define, wDefine, COUNT_OF(wDefine));
        args.push_back(wDefine);
    }

    // 엔트리 포인트 이름
    args.push_back(L"-E");
    wchar_t wEntryPoint[256];
    BE1::PlatformWinUtils::UTF8ToUCS2(compileInput->entryPoint, wEntryPoint, COUNT_OF(wEntryPoint));
    args.push_back(wEntryPoint);

    // 소스 파일 이름
    wchar_t wSourceName[256];
    BE1::PlatformWinUtils::UTF8ToUCS2(compileInput->sourceName, wSourceName, COUNT_OF(wSourceName));
    args.push_back(wSourceName);

    // wchar_t 포인터 배열을 만든다.
    std::vector<const wchar_t *> argv;
    argv.reserve(args.size());
    for (auto &arg : args) {
        argv.push_back(arg.c_str());
    }

    // 커스텀 인클루드 핸들러 클래스
    struct CustomIncludeHandler : public IDxcIncludeHandler {
        IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv) override {
            if (riid == __uuidof(IDxcIncludeHandler) || riid == __uuidof(IUnknown)) {
                *ppv = static_cast<IDxcIncludeHandler *>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }

        IFACEMETHODIMP_(ULONG) AddRef() override {
            return InterlockedIncrement(&refCount);
        }

        IFACEMETHODIMP_(ULONG) Release() override {
            ULONG res = InterlockedDecrement(&refCount);
            if (res == 0) {
                delete this;
            }
            return res;
        }

        IFACEMETHODIMP LoadSource(_In_z_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource) override {
            char filename[1024];
            BE1::PlatformWinUtils::UCS2ToUTF8(pFilename, filename, COUNT_OF(filename));

            char *shaderText;
            int shaderTextSize = BE1::fileSystem.LoadFile(filename, true, (void **)&shaderText);
            if (!shaderText) {
                // Could not open include file filename
                return E_FAIL;
            }

            // shaderText 를 IDxcBlob 으로 변환
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> blobEncoding;
            HRESULT hr = D3D12Renderer::GetRenderer()->dxcLibrary->CreateBlobWithEncodingFromPinned(
                reinterpret_cast<LPBYTE>(const_cast<char *>(shaderText)),
                static_cast<UINT32>(shaderTextSize), CP_UTF8, &blobEncoding
            );
            if (FAILED(hr)) {
                return hr;
            }

            shaderTextPtrs.push_back(shaderText);

            *ppIncludeSource = blobEncoding.Detach();
            return S_OK;
        }

        ~CustomIncludeHandler() {
            for (void *ptr : shaderTextPtrs) {
                BE1::fileSystem.FreeFile(ptr);
            }
            shaderTextPtrs.clear();
        }

        ULONG refCount = 1;
        std::vector<void *> shaderTextPtrs;
    };

    // Custom include 핸들러 생성
    Microsoft::WRL::ComPtr<CustomIncludeHandler> customIncludeHandler;
    customIncludeHandler.Attach(new CustomIncludeHandler);

    DxcBuffer source;
    source.Ptr = compileInput->shaderText;
    source.Size = compileInput->shaderTextSize;
    source.Encoding = DXC_CP_UTF8;

    // Shader 컴파일
    Microsoft::WRL::ComPtr<IDxcResult> dxcResult;
    dxcCompiler->Compile(&source, argv.data(), (UINT32)args.size(), customIncludeHandler.Get(), IID_PPV_ARGS(&dxcResult));

    // 경고 & 에러 메시지를 받아온다.
    IDxcBlobUtf8 *dxcErrorBlob = nullptr;
    dxcResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&dxcErrorBlob), nullptr);
    if (dxcErrorBlob) {
        compileOutput->errorMessage = dxcErrorBlob->GetStringPointer();
        dxcErrorBlob->Release();
    }

    HRESULT hrStatus;
    dxcResult->GetStatus(&hrStatus);
    if (FAILED(hrStatus)) {
        // 컴파일 실패
        return false;
    }

    IDxcBlob *dxcShaderBlob = nullptr;
    dxcResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxcShaderBlob), nullptr);
    if (!dxcShaderBlob) {
        return false;
    }

    compileOutput->compiledShaderDataSize = dxcShaderBlob->GetBufferSize();
    compileOutput->compiledShaderData = (byte *)Mem_Alloc(dxcShaderBlob->GetBufferSize());
    memcpy(compileOutput->compiledShaderData, (const byte *)dxcShaderBlob->GetBufferPointer(), dxcShaderBlob->GetBufferSize());
    dxcShaderBlob->Release();
    return true;
}

bool D3D12Renderer::LoadCompiledShader(const char *name, const uint64_t hash, byte **compiledShaderDataPtr, uint32_t *compiledShaderDataSizePtr) {
    BE1::Str filename;// = shaderCacheDir;
    filename.AppendPath(name);
    filename.SetFileExtension(".cso");

    BE1::PlatformFileMapping *fileMapping = BE1::PlatformFileMapping::OpenFileRead(filename);
    if (!fileMapping) {
        return false;
    }

    const byte *fileData = (const byte *)fileMapping->GetData();
    // 저장된 cso 파일과 hash 값이 같은지 비교한다.
    if (*(uint64_t *)fileData != hash) {
        delete fileMapping;
        return false;
    }

    // 해시값 스킵
    fileData += sizeof(uint64_t);

    *compiledShaderDataSizePtr = fileMapping->GetSize() - sizeof(uint64_t);
    *compiledShaderDataPtr = (byte *)Mem_Alloc(*compiledShaderDataSizePtr);
    memcpy(*compiledShaderDataPtr, fileData, *compiledShaderDataSizePtr);
    return true;
}

void D3D12Renderer::WriteCompiledShader(const char *name, const uint64_t hash, const byte *compiledShaderData, uint32_t compiledShaderDataSize) {
    if (!compiledShaderData || compiledShaderDataSize == 0) {
        return;
    }

    BE1::Str filename;// = shaderCacheDir;
    filename.AppendPath(name);
    filename.SetFileExtension(".cso");
    BE1::PlatformFile *file = (BE1::PlatformFile *)BE1::PlatformFile::OpenFileWrite(filename);
    if (!file) {
        return;
    }

    int fileDataSize = compiledShaderDataSize + sizeof(uint64_t);
    byte *fileData = (byte *)Mem_Alloc32(fileDataSize);

    // 캐싱된 cso 파일의 첫 64 비트는 hash 값을 저장한다.
    *(uint64_t *)fileData = hash;
    memcpy(fileData + sizeof(uint64_t), compiledShaderData, compiledShaderDataSize);

    file->Write(fileData, fileDataSize);

    Mem_AlignedFree(fileData);
    delete file;
}

RHI::Shader *D3D12Renderer::CreateShader(RHI::ShaderModel shaderModel, RHI::ShaderStage shaderStage, const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint) {
    const char *shaderStageName = nullptr;
    switch (shaderStage) {
    case RHI::ShaderStage::Vertex:
        shaderStageName = "vs";
        break;
    case RHI::ShaderStage::Hull:
        shaderStageName = "hs";
        break;
    case RHI::ShaderStage::Domain:
        shaderStageName = "ds";
        break;
    case RHI::ShaderStage::Geometry:
        shaderStageName = "gs";
        break;
    case RHI::ShaderStage::Fragment:
        shaderStageName = "ps";
        break;
    case RHI::ShaderStage::Compute:
        shaderStageName = "cs";
        break;
    default:
        return nullptr;
    }

    BE1::Str fileName = sourceName;
    BE1::Str fileBase;
    fileName.ExtractFileBase(fileBase);
    char mangledFilename[256];
    BE1::Str::snPrintf(mangledFilename, sizeof(mangledFilename), "%s-%s-%s", fileBase.c_str(), entryPoint, shaderStageName);

    BE1::Str extension;
    fileName.ExtractFileExtension(extension);
    fileName.StripFileName();
    fileName.AppendPath(mangledFilename);
    fileName.SetFileExtension(extension);

    byte *compiledShaderData = nullptr;
    uint32_t compiledShaderDataSize = 0;

    // 이미 컴파일된 cso 파일을 로드해본다.
    const uint64_t shaderTextHash = BE1::CityHash64(shaderText, shaderTextSize);
#ifndef _DEBUG
    bool shouldCompileShader = !LoadCompiledShader(fileName, shaderTextHash, &compiledShaderData, &compiledShaderDataSize);
#else
    bool shouldCompileShader = true;
#endif

    // hash 값이 다르거나 파일이 없다면 새로 컴파일한다.
    if (shouldCompileShader) {
        RHI::ShaderCompileInput compileInput = {};
        compileInput.shaderFormat = GetShaderFormat();
        compileInput.shaderModel = shaderModel;
        compileInput.shaderStage = shaderStage;
        compileInput.sourceName = sourceName;
        compileInput.shaderText = shaderText;
        compileInput.shaderTextSize = shaderTextSize;
        compileInput.entryPoint = entryPoint;

        RHI::ShaderCompileOutput compileOutput = {};
        bool compileSucceeded = CompileShader(&compileInput, &compileOutput);
        if (!compileOutput.errorMessage.IsEmpty()) {
            BE_WARNLOG(compileOutput.errorMessage);
        }
        if (!compileSucceeded) {
            return nullptr;
        }

        compiledShaderData = compileOutput.compiledShaderData;
        compiledShaderDataSize = compileOutput.compiledShaderDataSize;

#ifndef _DEBUG
        // 컴파일했으므로 cso 파일을 저장한다.
        WriteCompiledShader(fileName, shaderTextHash, compiledShaderData, compiledShaderDataSize);
#endif
    }

    // shader text 로부터 root signature 를 얻기 위한 deserializer 를 생성한다.
    ID3D12VersionedRootSignatureDeserializer *rootSignatureDeserializer = nullptr;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *rootSignatureDesc = nullptr;
    HRESULT hr = D3D12CreateVersionedRootSignatureDeserializer(compiledShaderData, compiledShaderDataSize, IID_PPV_ARGS(&rootSignatureDeserializer));
    if (SUCCEEDED(hr)) {
        // deserializer 로부터 root signature desc 포인터를 얻어낸다. (deserializer 가 파괴될 때까지 desc 의 메모리는 유지된다)
        rootSignatureDeserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &rootSignatureDesc);
        assert(rootSignatureDesc->Version == D3D_ROOT_SIGNATURE_VERSION_1_1);
    }

    ID3D12RootSignature *rootSignature = nullptr;
    if (FAILED(device->CreateRootSignature(0, compiledShaderData, compiledShaderDataSize, IID_PPV_ARGS(&rootSignature)))) {
        rootSignatureDeserializer->Release();
        return nullptr;
    }

    D3D12Shader *shader = new D3D12Shader;
    shader->shaderStage = shaderStage;
    shader->hash = shaderTextHash;
    shader->compiledShaderData = compiledShaderData;
    shader->compiledShaderDataSize = compiledShaderDataSize;
    shader->rootSignature = rootSignature;
    shader->rootSignatureDeserializer = rootSignatureDeserializer;
    shader->rootSignatureDesc = rootSignatureDesc;
    return shader;
}

RHI::Shader *D3D12Renderer::CreateShaderFromFile(RHI::ShaderModel shaderModel, RHI::ShaderStage shaderStage, const char *filename, const char *entryPoint) {
    char *shaderText;
    int shaderTextSize = BE1::fileSystem.LoadFile(filename, true, (void **)&shaderText);
    if (!shaderText) {
        return nullptr;
    }

    RHI::Shader *shader = CreateShader(shaderModel, shaderStage, filename, shaderText, shaderTextSize, entryPoint);
    if (!shader) {
        BE1::fileSystem.FreeFile(shaderText);
        return nullptr;
    }

    BE1::fileSystem.FreeFile(shaderText);
    return shader;
}

void D3D12Renderer::DestroyShader(RHI::Shader *shader, bool immediate) {
    if (immediate) {
        delete shader;
    } else {
        MarkForDelete(shader);
    }
}
