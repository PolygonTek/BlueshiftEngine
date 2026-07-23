// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
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

static constexpr uint32_t CompiledShaderIdent = MAKE_FOURCC('B', 'S', 'C', 'S');
static constexpr uint32_t CompiledShaderVersion = 1;

struct CompiledShaderHeader {
    uint32_t    ident;
    uint32_t    version;
    uint64_t    hash = 0;
    uint32_t    dependencyCount = 0;
    uint32_t    byteCodeSize = 0;
};

struct CompiledShaderDependencyHeader {
    uint64_t    contentHash = 0;
    uint32_t    filenameLength = 0;
};

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
#ifdef USE_SHADER_DEBUG
    compileFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS);
#endif
    ID3DBlob *errorBlob = nullptr;
    ID3DBlob *compiledShaderBlob = nullptr;

    std::vector<BE1::Str> defineNames;
    std::vector<BE1::Str> defineValues;
    std::vector<D3D_SHADER_MACRO> macros;
    if (compileInput->defines.Count() > 0) {
        defineNames.reserve(compileInput->defines.Count());
        defineValues.reserve(compileInput->defines.Count());
        macros.reserve(compileInput->defines.Count() + 1);

        for (const BE1::Str &define : compileInput->defines) {
            int equalIndex = define.Find('=');
            if (equalIndex >= 0) {
                defineNames.push_back(define.Left(equalIndex));
                defineValues.push_back(define.Mid(equalIndex + 1, define.Length() - equalIndex - 1));
            } else {
                defineNames.push_back(define);
                defineValues.push_back("1");
            }
        }

        for (int i = 0; i < static_cast<int>(defineNames.size()); ++i) {
            macros.push_back({ defineNames[i].c_str(), defineValues[i].c_str() });
        }
        macros.push_back({ nullptr, nullptr });
    }

    const D3D_SHADER_MACRO *d3dDefines = macros.empty() ? nullptr : macros.data();
    HRESULT hr = D3DCompile(compileInput->shaderText, compileInput->shaderTextSize, compileInput->sourceName, d3dDefines, nullptr, compileInput->entryPoint, target, compileFlags, 0, &compiledShaderBlob, &errorBlob);
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

#ifdef USE_SHADER_DEBUG
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
        case RHI::ShaderModel::SM_6_8:
            args.push_back(L"vs_6_8");
            break;
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
        case RHI::ShaderModel::SM_6_8:
            args.push_back(L"hs_6_8");
            break;
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
        case RHI::ShaderModel::SM_6_8:
            args.push_back(L"ds_6_8");
            break;
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
        case RHI::ShaderModel::SM_6_8:
            args.push_back(L"gs_6_8");
            break;
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
        case RHI::ShaderModel::SM_6_8:
            args.push_back(L"ps_6_8");
            break;
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
        case RHI::ShaderModel::SM_6_8:
            args.push_back(L"cs_6_8");
            break;
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

            char *shaderText = nullptr;
            int shaderTextSize = BE1::fileSystem.LoadFile(filename, true, (void **)&shaderText);
            if (!shaderText) {
                // Could not open include file filename
                return E_FAIL;
            }

            if (dependencies) {
                bool alreadyAdded = false;
                for (const RHI::ShaderSourceDependency &dependency : *dependencies) {
                    if (!BE1::Str::Cmp(dependency.filename.c_str(), filename)) {
                        alreadyAdded = true;
                        break;
                    }
                }

                if (!alreadyAdded) {
                    RHI::ShaderSourceDependency &dependency = dependencies->Alloc();
                    dependency.filename = filename;
                    dependency.contentHash = BE1::CityHash64(shaderText, shaderTextSize);
                }
            }

            // shaderText 를 IDxcBlob 으로 변환
            Microsoft::WRL::ComPtr<IDxcBlobEncoding> blobEncoding;
            HRESULT hr = D3D12Renderer::GetRenderer()->dxcLibrary->CreateBlobWithEncodingFromPinned(
                reinterpret_cast<LPBYTE>(const_cast<char *>(shaderText)),
                static_cast<UINT32>(shaderTextSize), CP_UTF8, &blobEncoding);
            if (FAILED(hr)) {
                BE1::fileSystem.FreeFile(shaderText);
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
        BE1::Array<RHI::ShaderSourceDependency> *dependencies = nullptr;
        std::vector<void *> shaderTextPtrs;
    };

    // Custom include 핸들러 생성
    Microsoft::WRL::ComPtr<CustomIncludeHandler> customIncludeHandler;
    customIncludeHandler.Attach(new CustomIncludeHandler);
    customIncludeHandler->dependencies = &compileOutput->dependencies;

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

static bool GenerateFileContentHash(const char *filename, uint64_t *contentHash) {
    char *fileData = nullptr;
    const int fileDataSize = BE1::fileSystem.LoadFile(filename, true, (void **)&fileData);
    if (!fileData) {
        return false;
    }

    *contentHash = BE1::CityHash64(fileData, fileDataSize);
    BE1::fileSystem.FreeFile(fileData);
    return true;
}

static void GetCompiledShaderCacheRelativeName(const char *name, BE1::Str &outRelativeName) {
    outRelativeName = name;
    outRelativeName.CleanPath('/');

    static constexpr const char *shaderDirPrefix = "Shaders/";
    const size_t shaderDirPrefixLength = strlen(shaderDirPrefix);
    if (outRelativeName.StartsWith(shaderDirPrefix)) {
        outRelativeName = outRelativeName.c_str() + shaderDirPrefixLength;
        return;
    }

    static constexpr const char *shaderDirComponent = "/Shaders/";
    const char *shaderDir = strstr(outRelativeName.c_str(), shaderDirComponent);
    if (shaderDir) {
        outRelativeName = shaderDir + strlen(shaderDirComponent);
        return;
    }

    outRelativeName.StripPath();
}

void D3D12Renderer::GetCompiledShaderCacheFilename(const char *name, BE1::Str &outFilename) const {
    BE1::Str relativeName;
    GetCompiledShaderCacheRelativeName(name, relativeName);

    const char *relativeNamePtr = relativeName.c_str();
    while (*relativeNamePtr == '/') {
        relativeNamePtr++;
    }

    outFilename = shaderCacheDir;
    outFilename.AppendPath(relativeNamePtr, '/');
    outFilename.SetFileExtension(".cso");
}

bool D3D12Renderer::LoadCompiledShader(const char *name, const uint64_t hash, byte **compiledShaderDataPtr, uint32_t *compiledShaderDataSizePtr) {
    BE1::Str filename;
    GetCompiledShaderCacheFilename(name, filename);

    BE1::PlatformFileMapping *fileMapping = BE1::PlatformFileMapping::OpenFileRead(filename);
    if (!fileMapping) {
        return false;
    }

    const byte *filePtr = (const byte *)fileMapping->GetData();
    const byte *fileEnd = filePtr + fileMapping->GetSize();

    if (static_cast<size_t>(fileEnd - filePtr) < sizeof(CompiledShaderHeader)) {
        delete fileMapping;
        return false;
    }

    const CompiledShaderHeader *header = (const CompiledShaderHeader *)filePtr;
    filePtr += sizeof(CompiledShaderHeader);

    if (header->ident != CompiledShaderIdent || header->version != CompiledShaderVersion || header->hash != hash) {
        delete fileMapping;
        return false;
    }

    for (uint32_t i = 0; i < header->dependencyCount; i++) {
        if (static_cast<size_t>(fileEnd - filePtr) < sizeof(CompiledShaderDependencyHeader)) {
            delete fileMapping;
            return false;
        }

        const CompiledShaderDependencyHeader *dependencyHeader = (const CompiledShaderDependencyHeader *)filePtr;
        filePtr += sizeof(CompiledShaderDependencyHeader);

        if (static_cast<size_t>(fileEnd - filePtr) < dependencyHeader->filenameLength) {
            delete fileMapping;
            return false;
        }

        BE1::Str dependencyFilename;
        dependencyFilename.Append((const char *)filePtr, dependencyHeader->filenameLength);
        filePtr += dependencyHeader->filenameLength;

        // cso 파일에 저장된 include 파일 hash (이전 컴파일) 와 실제 파일의 hash 를 비교한다.
        uint64_t dependencyContentHash = 0;
        if (!GenerateFileContentHash(dependencyFilename, &dependencyContentHash) || dependencyContentHash != dependencyHeader->contentHash) {
            delete fileMapping;
            return false;
        }
    }

    if (static_cast<size_t>(fileEnd - filePtr) < header->byteCodeSize) {
        delete fileMapping;
        return false;
    }

    *compiledShaderDataSizePtr = header->byteCodeSize;
    *compiledShaderDataPtr = (byte *)Mem_Alloc(header->byteCodeSize);
    memcpy(*compiledShaderDataPtr, filePtr, header->byteCodeSize);

    delete fileMapping;
    return true;
}

void D3D12Renderer::WriteCompiledShader(const char *name, const uint64_t hash, const byte *compiledShaderData, uint32_t compiledShaderDataSize, const BE1::Array<RHI::ShaderSourceDependency> &dependencies) {
    if (!compiledShaderData || compiledShaderDataSize == 0) {
        return;
    }

    BE1::Str filename;
    GetCompiledShaderCacheFilename(name, filename);

    BE1::Str cacheDir = filename;
    cacheDir.StripFileName();
    if (!BE1::PlatformFile::DirectoryExists(cacheDir)) {
        BE1::PlatformFile::CreateDirectoryTree(cacheDir);
    }

    BE1::PlatformFile *file = (BE1::PlatformFile *)BE1::PlatformFile::OpenFileWrite(filename);
    if (!file) {
        return;
    }

    CompiledShaderHeader header = {};
    header.ident = CompiledShaderIdent;
    header.version = CompiledShaderVersion;
    header.hash = hash;
    header.dependencyCount = static_cast<uint32_t>(dependencies.Count());
    header.byteCodeSize = compiledShaderDataSize;

    file->Write(&header, sizeof(header));

    for (const RHI::ShaderSourceDependency &dependency : dependencies) {
        CompiledShaderDependencyHeader dependencyHeader = {};
        dependencyHeader.contentHash = dependency.contentHash;
        dependencyHeader.filenameLength = static_cast<uint32_t>(dependency.filename.Length());

        file->Write(&dependencyHeader, sizeof(dependencyHeader));
        file->Write(dependency.filename.c_str(), dependencyHeader.filenameLength);
    }

    file->Write(compiledShaderData, compiledShaderDataSize);
    delete file;
}

RHI::Shader *D3D12Renderer::CreateShader(RHI::ShaderModel shaderModel, RHI::ShaderStage shaderStage, const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint, const BE1::StrArray *defines) {
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

    const char *shaderModelName = nullptr;
    switch (shaderModel) {
    case RHI::ShaderModel::SM_5_0:
        shaderModelName = "SM50";
        break;
    case RHI::ShaderModel::SM_6_0:
        shaderModelName = "SM60";
        break;
    case RHI::ShaderModel::SM_6_1:
        shaderModelName = "SM61";
        break;
    case RHI::ShaderModel::SM_6_2:
        shaderModelName = "SM62";
        break;
    case RHI::ShaderModel::SM_6_3:
        shaderModelName = "SM63";
        break;
    case RHI::ShaderModel::SM_6_4:
        shaderModelName = "SM64";
        break;
    case RHI::ShaderModel::SM_6_5:
        shaderModelName = "SM65";
        break;
    case RHI::ShaderModel::SM_6_6:
        shaderModelName = "SM66";
        break;
    case RHI::ShaderModel::SM_6_7:
        shaderModelName = "SM67";
        break;
    case RHI::ShaderModel::SM_6_8:
        shaderModelName = "SM68";
        break;
    default:
        return nullptr;
    }

    uint64_t definesHash = 0;
    if (defines && defines->Count() > 0) {
        definesHash = 1469598103934665603ull;
        for (const BE1::Str &define : *defines) {
            definesHash = hash_combine(definesHash, define.ToHash64());
        }
    }

    BE1::Str fileName = sourceName;
    BE1::Str fileBase;
    fileName.ExtractFileBase(fileBase);
    char mangledFilename[256];
    if (definesHash != 0) {
        BE1::Str::snPrintf(mangledFilename, sizeof(mangledFilename), "%s-%s-%s-%s-%016llx", fileBase.c_str(), entryPoint, shaderStageName, shaderModelName, definesHash);
    } else {
        BE1::Str::snPrintf(mangledFilename, sizeof(mangledFilename), "%s-%s-%s-%s", fileBase.c_str(), entryPoint, shaderStageName, shaderModelName);
    }

    BE1::Str extension;
    fileName.ExtractFileExtension(extension);
    fileName.StripFileName();
    fileName.AppendPath(mangledFilename);
    fileName.SetFileExtension(extension);

    byte *compiledShaderData = nullptr;
    uint32_t compiledShaderDataSize = 0;

    // Include files are validated from the dependency table stored in the cso cache.
    uint64_t shaderHash = BE1::CityHash64(shaderText, shaderTextSize);
    shaderHash = hash_combine(shaderHash, definesHash);
#ifdef USE_COMPILED_SHADER
    // 이미 컴파일된 cso 파일을 로드해본다.
    bool shouldCompileShader = !LoadCompiledShader(fileName, shaderHash, &compiledShaderData, &compiledShaderDataSize);
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
        if (defines) {
            compileInput.defines = *defines;
        }

        RHI::ShaderCompileOutput compileOutput = {};
        bool compileSucceeded = CompileShader(&compileInput, &compileOutput);
        if (!compileOutput.errorMessage.IsEmpty()) {
            if (compileSucceeded) {
                BE_ERRLOG("%s", compileOutput.errorMessage.c_str());
            } else {
                BE_FATALERROR("%s", compileOutput.errorMessage.c_str());
            }
        }
        if (!compileSucceeded) {
            return nullptr;
        }

        compiledShaderData = compileOutput.compiledShaderData;
        compiledShaderDataSize = compileOutput.compiledShaderDataSize;

#ifdef USE_COMPILED_SHADER
        // 컴파일했으므로 cso 파일을 저장한다.
        WriteCompiledShader(fileName, shaderHash, compiledShaderData, compiledShaderDataSize, compileOutput.dependencies);
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

    // 컴파일된 쉐이더 바이너리로부터 루트 시그니쳐 객체를 생성한다.
    ID3D12RootSignature *rootSignature = nullptr;
    if (FAILED(device->CreateRootSignature(0, compiledShaderData, compiledShaderDataSize, IID_PPV_ARGS(&rootSignature)))) {
        rootSignatureDeserializer->Release();
        return nullptr;
    }

    const uint64_t compiledShaderHash = BE1::CityHash64(reinterpret_cast<const char *>(compiledShaderData), compiledShaderDataSize);

    D3D12Shader *shader = new D3D12Shader;
    shader->shaderStage = shaderStage;
    shader->hash = compiledShaderHash;
    shader->compiledShaderData = compiledShaderData;
    shader->compiledShaderDataSize = compiledShaderDataSize;
    shader->rootSignature = rootSignature;
    shader->rootSignatureDeserializer = rootSignatureDeserializer;
    shader->rootSignatureDesc = rootSignatureDesc;
    return shader;
}

RHI::Shader *D3D12Renderer::CreateShaderFromFile(RHI::ShaderModel shaderModel, RHI::ShaderStage shaderStage, const char *filename, const char *entryPoint, const BE1::StrArray *defines) {
    char *shaderText;
    int shaderTextSize = BE1::fileSystem.LoadFile(filename, true, (void **)&shaderText);
    if (!shaderText) {
        return nullptr;
    }

    RHI::Shader *shader = CreateShader(shaderModel, shaderStage, filename, shaderText, shaderTextSize, entryPoint, defines);
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
        DeleteGPUObject(shader);
    }
}
