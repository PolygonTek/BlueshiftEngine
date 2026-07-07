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
#include "Platform/PlatformSystem.h"
#include "Platform/Windows/PlatformWinUtils.h"
#include "D3D12Renderer.h"
#include "D3D12SwapChain.h"
#include "D3D12CommandList.h"
#include "D3D12CommandListPool.h"
#include "D3D12ShaderVisibleDescriptorHeap.h"
#include "D3D12DescriptorPool.h"
#include "D3D12Texture.h"
#include "D3D12FrameThreadData.h"
#include "D3D12ResourceRequest.h"
#include "D3D12ImGuiRenderer.h"

// D3D12.dll 이 D3D12Core.dll 을 찾기 위한 설정
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }
extern "C" { __declspec(dllexport) extern const char *D3D12SDKPath = u8"."; }

#if defined(_DEBUG)
static CVAR(d3d12_debugLayer, "1", BE1::CVar::Flag::Bool, "enable D3D12 debug layer at renderer startup");
#else
static CVAR(d3d12_debugLayer, "0", BE1::CVar::Flag::Bool, "enable D3D12 debug layer at renderer startup");
#endif
static CVAR(d3d12_gpuValidation, "1", BE1::CVar::Flag::Bool, "enable D3D12 GPU validation when debug layer is enabled at renderer startup");

void D3D12Renderer::Init(const void *mainWindowHandle) {
    RHI::Renderer::Init(mainWindowHandle);

    projectionPolicy.clipDepthRange = RHI::ClipDepthRange::ZeroToOne;
    projectionPolicy.reverseZ = true;
    projectionPolicy.infiniteZFar = true;

#if defined(_DEBUG) || defined(_DEVELOPMENT)
    const bool requestDebugLayer = d3d12_debugLayer.GetBool();
    const bool requestGpuValidation = requestDebugLayer && d3d12_gpuValidation.GetBool();
#else
    const bool requestDebugLayer = false;
    const bool requestGpuValidation = false;
#endif

    HRESULT hr;

    if (requestDebugLayer) {
        // 디버그 레이어 활성화
        ID3D12Debug* debugController = nullptr;
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
        if (SUCCEEDED(hr)) {
            debugController->EnableDebugLayer();

            validationLayerEnabled = true;

            // GPU Validation 활성화
            if (requestGpuValidation) {
                ID3D12Debug5* debugController5 = nullptr;
                if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))))
                {
                    debugController5->SetEnableGPUBasedValidation(TRUE);
                    debugController5->SetEnableSynchronizedCommandQueueValidation(TRUE);
                    debugController5->SetEnableAutoName(TRUE);
                    debugController5->Release();

                    gpuValidationEnabled = true;
                }
            }
            debugController->Release();
        }

        // DRED (Device Removed Extended Data) 기능을 활성화
        ID3D12DeviceRemovedExtendedDataSettings1 *dredSettings;
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings));
        if (SUCCEEDED(hr)) {
            // Turn on auto-breadcrumbs and page fault reporting.
            dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            dredSettings->Release();
        }
    }

    // DXGI Factory 생성
    hr = CreateDXGIFactory2(validationLayerEnabled ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateDXGIFactory2 failed, ERROR: 0x%x", hr);
    }

    IDXGIFactory5 *dxgiFactory5 = nullptr;
    if (SUCCEEDED(dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory5)))) {
        // VSync 를 끈 상태로 Present 가능한지 체크
        // VRR (Variable Refresh Rate): G-SYNC / FreeSync 계열 지원을 위한 체크
        BOOL allowTearing = FALSE;
        hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        if (FAILED(hr) || !allowTearing) {
            supportsTearing = false;

            BE_WARNLOG("Unsupported variable refresh rate displays\n");
        } else {
            supportsTearing = true;
        }
        dxgiFactory5->Release();
    }

    IDXGIAdapter1 *adapter1 = nullptr;
    CreateDevice(&adapter1);

    // 어댑터 (그래픽 카드) 정보 얻어오기
    DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
    adapter1->GetDesc1(&dxgiAdapterDesc);
    vendorId = dxgiAdapterDesc.VendorId;
    deviceId = dxgiAdapterDesc.DeviceId;
    dedicatedVideoMemSize = dxgiAdapterDesc.DedicatedVideoMemory;
    dedicatedSystemMemSize = dxgiAdapterDesc.DedicatedSystemMemory;
    sharedSystemMemSize = dxgiAdapterDesc.SharedSystemMemory;
    char temp[128] = "";
    BE1::PlatformWinUtils::UCS2ToUTF8(dxgiAdapterDesc.Description, temp, COUNT_OF(temp));
    adapterName = temp;

    BE1::Str::Copynz(rendererInfo.name, adapterName.c_str(), COUNT_OF(rendererInfo.name));
    rendererInfo.vendorId = vendorId;
    rendererInfo.deviceId = deviceId;
    rendererInfo.dedicatedVideoMemoryBytes = dedicatedVideoMemSize;
    rendererInfo.dedicatedSystemMemoryBytes = dedicatedSystemMemSize;
    rendererInfo.sharedSystemMemoryBytes = sharedSystemMemSize;

    BE_LOG("Adapter: %s\n", adapterName.c_str());
    BE_LOG("Dedicated VideoMem Size: %s\n", BE1::Str::FormatBytes(dedicatedVideoMemSize).c_str());
    BE_LOG("Dedicated SystemMem Size: %s\n", BE1::Str::FormatBytes(dedicatedSystemMemSize).c_str());
    BE_LOG("Shared SystemMem Size: %s\n", BE1::Str::FormatBytes(sharedSystemMemSize).c_str());

    if (validationLayerEnabled) {
        // 디버그 표시 정보 설정
        ID3D12InfoQueue *infoQueue = nullptr;
        hr = device->QueryInterface(IID_PPV_ARGS(&infoQueue));
        if (SUCCEEDED(hr)) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

            D3D12_MESSAGE_SEVERITY enabledSeverities[] = {
                D3D12_MESSAGE_SEVERITY_CORRUPTION,
                D3D12_MESSAGE_SEVERITY_ERROR,
                D3D12_MESSAGE_SEVERITY_WARNING,
                D3D12_MESSAGE_SEVERITY_MESSAGE
            };

            D3D12_MESSAGE_ID disabledMessages[] = {
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                // Workarounds for debug layer issues on hybrid-graphics systems
                D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
                D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.AllowList.NumSeverities = (UINT)COUNT_OF(enabledSeverities);
            filter.AllowList.pSeverityList = enabledSeverities;
            filter.DenyList.NumIDs = (UINT)COUNT_OF(disabledMessages);
            filter.DenyList.pIDList = disabledMessages;
            infoQueue->AddStorageFilterEntries(&filter);
            infoQueue->Release();
        }
    }

#ifdef USE_D3D12_MEMALLOC
    // D3D12MA Allocator 생성
    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = device;
    allocatorDesc.pAdapter = adapter1;
    allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
    allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_DONT_PREFER_SMALL_BUFFERS_COMMITTED;

    hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocator);
    if (FAILED(hr)) {
        BE_FATALERROR("D3D12MA::CreateAllocator failed, ERROR: 0x%x", hr);
    }
#endif

    // IDXGIAdapter3 인터페이스 쿼리해서 얻어오기
    adapter1->QueryInterface(IID_PPV_ARGS(&dxgiAdapter3));

    adapter1->Release();

    // 텍스쳐의 최대 크기 정보
    // Vulkan 이나 OpenGL 에서는 최소 크기에 대한 스펙만 정해져있고, 실제 최대 크기는 쿼리하는 형식이지만,
    // D3D12 에서는 스펙상 최대 크기가 정해져 있음
    max1DTextureSize = D3D12_REQ_TEXTURE1D_U_DIMENSION;
    max2DTextureSize = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    max3DTextureSize = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    maxCubeTextureSize = D3D12_REQ_TEXTURECUBE_DIMENSION;
    max1DTextureArraySize = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
    max2DTextureArraySize = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
    maxCubeTextureArraySize = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / 6;

    CheckFeatures();

    // Graphics 커맨드 큐용 Fence
    D3D12QueueFence &graphicsQueueFence = queueFences[to_int(RHI::CommandQueueType::Graphics)];

    // Fence 객체 생성
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&graphicsQueueFence.fence));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateFence failed, ERROR: 0x%x", hr);
    }

    // Fence 를 대기하기 위한 이벤트 객체 생성
    graphicsQueueFence.eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // Compute 커맨드 큐용 Fence
    D3D12QueueFence &computeQueueFence = queueFences[to_int(RHI::CommandQueueType::Compute)];

    // Fence 객체 생성
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&computeQueueFence.fence));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateFence failed, ERROR: 0x%x", hr);
    }

    // Fence 를 대기하기 위한 이벤트 객체 생성
    computeQueueFence.eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // Graphics CommandQueue 생성
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueues[to_int(RHI::CommandQueueType::Graphics)]));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandQueue (Graphics) failed, ERROR: 0x%x", hr);
    }
    commandQueues[to_int(RHI::CommandQueueType::Graphics)]->SetName(L"GraphicsCommandQueue");

    // Compute CommandQueue 생성
    queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

    hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueues[to_int(RHI::CommandQueueType::Compute)]));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandQueue (Compute) failed, ERROR: 0x%x", hr);
    }
    commandQueues[to_int(RHI::CommandQueueType::Compute)]->SetName(L"ComputeCommandQueue");

    // ExecuteIndirect 에서 사용할 Command Signature 생성
    D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};

    D3D12_INDIRECT_ARGUMENT_DESC drawInstancedArgs[1];
    drawInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

    commandSignatureDesc.ByteStride = sizeof(D3D12_DRAW_ARGUMENTS);
    commandSignatureDesc.NumArgumentDescs = 1;
    commandSignatureDesc.pArgumentDescs = drawInstancedArgs;
    hr = device->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&drawInstancedIndirectCommandSignature));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW) failed, ERROR: 0x%x", hr);
    }

    D3D12_INDIRECT_ARGUMENT_DESC drawIndexedInstancedArgs[1];
    drawIndexedInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    commandSignatureDesc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
    commandSignatureDesc.NumArgumentDescs = 1;
    commandSignatureDesc.pArgumentDescs = drawIndexedInstancedArgs;
    hr = device->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&drawIndexedInstancedIndirectCommandSignature));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED) failed, ERROR: 0x%x", hr);
    }

    D3D12_INDIRECT_ARGUMENT_DESC dispatchArgs[1];
    dispatchArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

    commandSignatureDesc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
    commandSignatureDesc.NumArgumentDescs = 1;
    commandSignatureDesc.pArgumentDescs = dispatchArgs;
    hr = device->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&dispatchIndirectCommandSignature));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH) failed, ERROR: 0x%x", hr);
    }

    if (supportsMeshShader) {
        D3D12_INDIRECT_ARGUMENT_DESC dispatchMeshArgs[1];
        dispatchMeshArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;

        commandSignatureDesc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
        commandSignatureDesc.NumArgumentDescs = 1;
        commandSignatureDesc.pArgumentDescs = dispatchMeshArgs;
        hr = device->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&dispatchMeshIndirectCommandSignature));
        if (FAILED(hr)) {
            BE_FATALERROR("CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH) failed, ERROR: 0x%x", hr);
        }
    }

    // CPU 접근 전용 디스크립터 풀 생성
    // NOTE: non-shader visible 디스크립터 힙은 개수 제한을 받지 않는다. 더 커도 상관이 없음.
    resStagingDescriptorPool = new D3D12DescriptorPool(device, D3D12DescriptorPool::Type::CBV_SRV_UAV, 1000000, false);
    samStagingDescriptorPool = new D3D12DescriptorPool(device, D3D12DescriptorPool::Type::Sampler, 2048, false);
    rtvDescriptorPool = new D3D12DescriptorPool(device, D3D12DescriptorPool::Type::RTV, 16, false);
    dsvDescriptorPool = new D3D12DescriptorPool(device, D3D12DescriptorPool::Type::DSV, 16, false);

    // shader-visible 리소스/샘플러 디스크립터 힙 생성
    resShaderVisibleDescriptorHeap.Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, MaxShaderVisibleResourceDescriptors);
    samShaderVisibleDescriptorHeap.Init(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, MaxShaderVisibleSamplerDescriptors);

    // shader-visible 리소스 디스크립터 힙의 [0, MaxBindlessResourceDescriptors) 영역은 bindless 디스크립터로 사용 (0번 인덱스는 실제로는 invalid index 로 사용)
    bindlessResDescriptorAllocator.Init({ &resShaderVisibleDescriptorHeap, 0, MaxBindlessResourceDescriptors });

    // shader-visible 샘플러 디스크립터 힙의 [0, MaxBindlessSamplerDescriptors) 영역은 bindless 디스크립터로 사용 (0번 인덱스는 실제로는 invalid index 로 사용)
    bindlessSamDescriptorAllocator.Init({ &samShaderVisibleDescriptorHeap, 0, MaxBindlessSamplerDescriptors });

    // shader cache 디렉토리 초기화
    shaderCacheDir = "Cache/D3D12CompiledShaderCache";
    if (!BE1::PlatformFile::DirectoryExists(shaderCacheDir)) {
        BE1::PlatformFile::CreateDirectoryTree(shaderCacheDir);
    }

    // PSO cache 디렉토리 초기화
    psoCacheDir = "Cache/D3D12PSOCache";
    if (!BE1::PlatformFile::DirectoryExists(psoCacheDir)) {
        BE1::PlatformFile::CreateDirectoryTree(psoCacheDir);
    }

    CreateShaderCompiler();

    releaseRequestMutex = BE1::PlatformMutex::Create();
    resourceRequestMutex = BE1::PlatformMutex::Create();
    psoMapMutex = BE1::PlatformMutex::Create();

    InitDefaultStates();

    utilGraphicsCommandListMutex = BE1::PlatformMutex::Create();
    utilComputeCommandListMutex = BE1::PlatformMutex::Create();

    // 메인 스레드의 그래픽스 (Direct) 커맨드 리스트 풀을 생성한다.
    utilGraphicsCommandListPool = new D3D12CommandListPool(nullptr, D3D12_COMMAND_LIST_TYPE_DIRECT, 8, 0);

    // 메인 스레드의 컴퓨트 커맨드 리스트 풀을 생성한다.
    utilComputeCommandListPool = new D3D12CommandListPool(nullptr, D3D12_COMMAND_LIST_TYPE_COMPUTE, 8);

    InitGenMipmapsPSO();

    imguiRenderer = new D3D12ImGuiRenderer;
    if (!imguiRenderer->Init()) {
        BE_WARNLOG("D3D12Renderer::Init: failed to initialize ImGui renderer\n");
        SAFE_DELETE(imguiRenderer);
    }
}

void D3D12Renderer::CheckFeatures() {
    // Init feature check (https://devblogs.microsoft.com/directx/introducing-a-new-api-for-checking-feature-support-in-direct3d-12/)
    CD3DX12FeatureSupport features;
    HRESULT hr = features.Init(device);
    assert(SUCCEEDED(hr));

    resourceHeapTier = features.ResourceHeapTier();

    //features.ResourceBindingTier();

    if (features.ConservativeRasterizationTier() >= D3D12_CONSERVATIVE_RASTERIZATION_TIER_1) {
        supportsConservativeRasterization = true;
    }
    if (features.VariableShadingRateTier() >= D3D12_VARIABLE_SHADING_RATE_TIER_1) {
        supportsVRS = true;
    }
    if (features.RaytracingTier() >= D3D12_RAYTRACING_TIER_1_1) {
        supportsRayTracing = true;
    }
    if (features.MeshShaderTier() >= D3D12_MESH_SHADER_TIER_1) {
        supportsMeshShader = true;
    }
    if (features.DepthBoundsTestSupported()) {
        supportsDepthBoundsTest = true;
    }
    if (features.CastingFullyTypedFormatSupported()) {
        // https://microsoft.github.io/DirectX-Specs/d3d/RelaxedCasting.html#casting-rules-for-rs2-drivers
        supportsCastingFullyTypedFormat = true;
    }
    if (features.TypedUAVLoadAdditionalFormats()) {
        // More info about UAV format load support: https://docs.microsoft.com/en-us/windows/win32/direct3d12/typed-unordered-access-view-loads
        // UAV 포맷은 어떤 포맷도 sRGB 를 지원하지 않는다.
        //
        // Common (D3D11.3) UAV Formats:
        // R32_FLOAT
        // R32_UINT
        // R32_SINT
        // R32G32B32A32_FLOAT
        // R32G32B32A32_UINT
        // R32G32B32A32_SINT
        // R16G16B16A16_FLOAT
        // R16G16B16A16_UINT
        // R16G16B16A16_SINT
        // R8G8B8A8_UNORM
        // R8G8B8A8_UINT
        // R8G8B8A8_SINT
        // R16_FLOAT
        // R16_UINT
        // R16_SINT
        // R8_UNORM
        // R8_UINT
        // R8_SINT
        supportsUAVFormatCommon = true;

        const DXGI_FORMAT optionalUAVFormats[] = {
            DXGI_FORMAT_R16G16B16A16_UNORM,
            DXGI_FORMAT_R16G16B16A16_SNORM,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R32G32_SINT,
            DXGI_FORMAT_R10G10B10A2_UNORM,
            DXGI_FORMAT_R10G10B10A2_UINT,
            DXGI_FORMAT_R11G11B10_FLOAT,
            DXGI_FORMAT_R8G8B8A8_SNORM,
            DXGI_FORMAT_R16G16_FLOAT,
            DXGI_FORMAT_R16G16_UNORM,
            DXGI_FORMAT_R16G16_UINT,
            DXGI_FORMAT_R16G16_SNORM,
            DXGI_FORMAT_R16G16_SINT,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R8G8_UINT,
            DXGI_FORMAT_R8G8_SNORM,
            DXGI_FORMAT_R8G8_SINT,
            DXGI_FORMAT_R16_UNORM,
            DXGI_FORMAT_R16_SNORM,
            DXGI_FORMAT_R8_SNORM,
            DXGI_FORMAT_A8_UNORM,
            DXGI_FORMAT_B5G6R5_UNORM,
            DXGI_FORMAT_B5G5R5A1_UNORM,
            DXGI_FORMAT_B4G4R4A4_UNORM
        };

        for (DXGI_FORMAT uavFormat : optionalUAVFormats) {
            D3D12_FORMAT_SUPPORT1 formatSupport1 = D3D12_FORMAT_SUPPORT1_NONE;
            D3D12_FORMAT_SUPPORT2 formatSupport2 = D3D12_FORMAT_SUPPORT2_NONE;

            hr = features.FormatSupport(uavFormat, formatSupport1, formatSupport2);
            const D3D12_FORMAT_SUPPORT2 requiredSupport = D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE;
            if (SUCCEEDED(hr) && (formatSupport2 & requiredSupport) == requiredSupport) {
                optionalUAVFormatMap.insert(uavFormat);
            }
        }
    }
}

void D3D12Renderer::InitGenMipmapsPSO() {
    // 2D 텍스쳐 용 밉맵 생성 컴퓨트 셰이더 PSO 생성
    {
        RHI::Shader *cs = static_cast<RHI::Shader *>(CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Compute, "Source/TestD3D12/Shaders/GenMip/GenMip2DFloat4.hlsl", "CSMain"));
        if (cs) {
            genMipmaps2DFloat4PSO = CreateComputePSO(cs);
            DestroyShader(cs, true);
        }
    }
    {
        RHI::Shader *cs = static_cast<RHI::Shader *>(CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Compute, "Source/TestD3D12/Shaders/GenMip/GenMip2DUNorm4.hlsl", "CSMain"));
        if (cs) {
            genMipmaps2DUNorm4PSO = CreateComputePSO(cs);
            DestroyShader(cs, true);
        }
    }
    // 큐브맵 텍스쳐 용 밉맵 생성 컴퓨트 셰이더 PSO 생성
    {
        RHI::Shader *cs = static_cast<RHI::Shader *>(CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Compute, "Source/TestD3D12/Shaders/GenMip/GenMipCubeFloat4.hlsl", "CSMain"));
        if (cs) {
            genMipmapsCubeFloat4PSO = CreateComputePSO(cs);
            DestroyShader(cs, true);
        }
    }
    {
        RHI::Shader *cs = static_cast<RHI::Shader *>(CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Compute, "Source/TestD3D12/Shaders/GenMip/GenMipCubeUNorm4.hlsl", "CSMain"));
        if (cs) {
            genMipmapsCubeUNorm4PSO = CreateComputePSO(cs);
            DestroyShader(cs, true);
        }
    }
    // 3D 텍스쳐 용 밉맵 생성 컴퓨트 셰이더 PSO 생성
    {
        RHI::Shader *cs = static_cast<RHI::Shader *>(CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Compute, "Source/TestD3D12/Shaders/GenMip/GenMip3DFloat4.hlsl", "CSMain"));
        if (cs) {
            genMipmaps3DFloat4PSO = CreateComputePSO(cs);
            DestroyShader(cs, true);
        }
    }
    {
        RHI::Shader *cs = static_cast<RHI::Shader *>(CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Compute, "Source/TestD3D12/Shaders/GenMip/GenMip3DUNorm4.hlsl", "CSMain"));
        if (cs) {
            genMipmaps3DUNorm4PSO = CreateComputePSO(cs);
            DestroyShader(cs, true);
        }
    }
}

void D3D12Renderer::FreeGenMipmapsPSO() {
    if (genMipmaps2DFloat4PSO) {
        DestroyPSO(genMipmaps2DFloat4PSO, true);
        genMipmaps2DFloat4PSO = nullptr;
    }
    if (genMipmaps2DUNorm4PSO) {
        DestroyPSO(genMipmaps2DUNorm4PSO, true);
        genMipmaps2DUNorm4PSO = nullptr;
    }
    if (genMipmapsCubeFloat4PSO) {
        DestroyPSO(genMipmapsCubeFloat4PSO, true);
        genMipmapsCubeFloat4PSO = nullptr;
    }
    if (genMipmapsCubeUNorm4PSO) {
        DestroyPSO(genMipmapsCubeUNorm4PSO, true);
        genMipmapsCubeUNorm4PSO = nullptr;
    }
    if (genMipmaps3DFloat4PSO) {
        DestroyPSO(genMipmaps3DFloat4PSO, true);
        genMipmaps3DFloat4PSO = nullptr;
    }
    if (genMipmaps3DUNorm4PSO) {
        DestroyPSO(genMipmaps3DUNorm4PSO, true);
        genMipmaps3DUNorm4PSO = nullptr;
    }
}

void D3D12Renderer::Shutdown() {
    ProcessResourceRequests(nullptr);

    DrainReleaseRequestsToRetireList();

    Finish(RHI::CommandQueueType::Graphics);
    Finish(RHI::CommandQueueType::Compute);

    // 렌더 스레드 종료 후에도 삭제 대기중인 리소스가 있을 수 있기 때문에 여기서 한번 더 정리한다.
    FreeRetiredResources(true);

    if (imguiRenderer) {
        imguiRenderer->Shutdown();
        SAFE_DELETE(imguiRenderer);
    }

    BE1::PlatformMutex::Destroy(utilGraphicsCommandListMutex);
    utilGraphicsCommandListMutex = nullptr;

    BE1::PlatformMutex::Destroy(utilComputeCommandListMutex);
    utilComputeCommandListMutex = nullptr;

    BE1::PlatformMutex::Destroy(releaseRequestMutex);
    releaseRequestMutex = nullptr;

    BE1::PlatformMutex::Destroy(resourceRequestMutex);
    resourceRequestMutex = nullptr;

    FreeGenMipmapsPSO();

    FreeDefaultStates();

    SAFE_DELETE(utilGraphicsCommandListPool);
    SAFE_DELETE(utilComputeCommandListPool);

    graphicsPsoMap.DeleteContents(true);
    computePsoMap.DeleteContents(true);

#ifdef USE_CACHED_PSO_BLOB
    for (auto &entry : cachedPsoBlobMap) {
        ID3DBlob *blob = entry.second;
        SAFE_RELEASE(blob);
    }
#endif

    BE1::PlatformMutex::Destroy(psoMapMutex);
    psoMapMutex = nullptr;

    SAFE_DELETE(resStagingDescriptorPool);
    SAFE_DELETE(samStagingDescriptorPool);
    SAFE_DELETE(rtvDescriptorPool);
    SAFE_DELETE(dsvDescriptorPool);

    bindlessSamDescriptorAllocator.Shutdown();
    bindlessResDescriptorAllocator.Shutdown();
    samShaderVisibleDescriptorHeap.Shutdown();
    resShaderVisibleDescriptorHeap.Shutdown();

    SAFE_RELEASE(dxcCompiler);
    SAFE_RELEASE(dxcUtils);
    SAFE_RELEASE(dxcLibrary);
    SAFE_RELEASE(drawInstancedIndirectCommandSignature);
    SAFE_RELEASE(drawIndexedInstancedIndirectCommandSignature);
    SAFE_RELEASE(dispatchIndirectCommandSignature);
    SAFE_RELEASE(dispatchMeshIndirectCommandSignature);
    SAFE_RELEASE(commandQueues[to_int(RHI::CommandQueueType::Graphics)]);
    SAFE_RELEASE(commandQueues[to_int(RHI::CommandQueueType::Compute)]);
    SAFE_RELEASE(queueFences[to_int(RHI::CommandQueueType::Graphics)].fence);
    SAFE_RELEASE(queueFences[to_int(RHI::CommandQueueType::Compute)].fence);
    SAFE_RELEASE(dxgiAdapter3);
    SAFE_RELEASE(dxgiFactory);

#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(allocator);
#endif

    if (dxcompilerLibrary) {
        BE1::PlatformProcess::CloseLibrary(dxcompilerLibrary);
    }

    ULONG refCount = device->Release();
    if (refCount > 0) {
        IDXGIDebug1 *pDebug = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug)))) {
            pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, static_cast<DXGI_DEBUG_RLO_FLAGS>(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            pDebug->Release();
        }
        BE1::PlatformSystem::DebugBreak();
    }

    RHI::Renderer::Shutdown();
}

void D3D12Renderer::GetMemoryUsage(RHI::MemoryUsage &memoryUsage) const {
#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Budget localBudget;
    D3D12MA::Budget nonLocalBudget;
    allocator->GetBudget(&localBudget, &nonLocalBudget);

    memoryUsage.budgetBytes = localBudget.BudgetBytes;
    memoryUsage.usageBytes = localBudget.UsageBytes;
#else
    if (!dxgiAdapter3) {
        memoryUsage = {};
        return;
    }
    DXGI_QUERY_VIDEO_MEMORY_INFO localInfo = {};
    if (SUCCEEDED(dxgiAdapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &localInfo))) {
        memoryUsage.budgetBytes = localInfo.Budget;
        memoryUsage.usageBytes = localInfo.CurrentUsage;
    }
#endif
}

uint64_t D3D12Renderer::SignalFence(RHI::CommandQueueType queueType) {
    D3D12QueueFence &queueFence = queueFences[to_int(queueType)];

    queueFence.value++;
    commandQueues[to_int(queueType)]->Signal(queueFence.fence, queueFence.value);

    return queueFence.value;
}

bool D3D12Renderer::IsFenceComplete(RHI::CommandQueueType queueType, uint64_t checkFenceValue) {
    D3D12QueueFence &queueFence = queueFences[to_int(queueType)];
    return queueFence.fence->GetCompletedValue() >= checkFenceValue;
}

void D3D12Renderer::WaitFence(RHI::CommandQueueType queueType, uint64_t expectedFenceValue) {
    D3D12QueueFence &queueFence = queueFences[to_int(queueType)];

    if (queueFence.fence->GetCompletedValue() < expectedFenceValue) {
        queueFence.fence->SetEventOnCompletion(expectedFenceValue, queueFence.eventHandle);
        WaitForSingleObject(queueFence.eventHandle, INFINITE);
    }
}

void D3D12Renderer::Finish(RHI::CommandQueueType queueType) {
    WaitFence(queueType, SignalFence(queueType));
}

void D3D12Renderer::CreateShaderCompiler() {
#if 1
    dxcompilerLibrary = BE1::PlatformProcess::OpenLibrary("dxcompiler.dll");
    if (!dxcompilerLibrary) {
        return;
    }

    DxcCreateInstanceProc DxcCreateInstance = (DxcCreateInstanceProc)BE1::PlatformProcess::GetSymbol(dxcompilerLibrary, "DxcCreateInstance");
    if (!DxcCreateInstance) {
        BE_WARNLOG("Failed to get symbol \"DxcCreateInstance\"\n");
        return;
    }
#endif

    HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    if (FAILED(hr)) {
        BE_FATALERROR("Create dxc compiler failed, ERROR: 0x%x", hr);
    }

    uint32_t dxcVersionMajor = 0;
    uint32_t dxcVersionMinor = 0;

    IDxcVersionInfo *dxcVersionInfo = nullptr;
    if (SUCCEEDED(dxcCompiler->QueryInterface(IID_PPV_ARGS(&dxcVersionInfo)))) {
        if (SUCCEEDED(dxcVersionInfo->GetVersion(&dxcVersionMajor, &dxcVersionMinor))) {
            BE_LOG("DXC version: %i.%i\n", dxcVersionMajor, dxcVersionMinor);
        }
        dxcVersionInfo->Release();
    }

    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    if (FAILED(hr)) {
        BE_FATALERROR("Create dxc util failed, ERROR: 0x%x", hr);
    }

    hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&dxcLibrary));
    if (FAILED(hr)) {
        BE_FATALERROR("Create dxc library failed, ERROR: 0x%x", hr);
    }
}

RHI::ShaderFormat D3D12Renderer::GetShaderFormat() const {
    return RHI::ShaderFormat::HLSL6;
}

uint32_t D3D12Renderer::AllocBindlessResourceDescriptor() {
    return bindlessResDescriptorAllocator.Alloc();
}

void D3D12Renderer::FreeBindlessResourceDescriptor(uint32_t bindlessIndex) {
    // Bindless descriptor slots must be returned only after GPU work that can reference them is complete.
    bindlessResDescriptorAllocator.Free(bindlessIndex);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Renderer::GetBindlessResourceCPUDescriptorHandle(uint32_t bindlessIndex) const {
    return bindlessResDescriptorAllocator.GetCPUDescriptorHandleFromIndex(bindlessIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Renderer::GetBindlessResourceGPUDescriptorHandle(uint32_t bindlessIndex) const {
    return bindlessResDescriptorAllocator.GetGPUDescriptorHandleFromIndex(bindlessIndex);
}

uint32_t D3D12Renderer::AllocBindlessSamplerDescriptor() {
    return bindlessSamDescriptorAllocator.Alloc();
}

void D3D12Renderer::FreeBindlessSamplerDescriptor(uint32_t bindlessIndex) {
    // Bindless descriptor slots must be returned only after GPU work that can reference them is complete.
    bindlessSamDescriptorAllocator.Free(bindlessIndex);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Renderer::GetBindlessSamplerCPUDescriptorHandle(uint32_t bindlessIndex) const {
    return bindlessSamDescriptorAllocator.GetCPUDescriptorHandleFromIndex(bindlessIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Renderer::GetBindlessSamplerGPUDescriptorHandle(uint32_t bindlessIndex) const {
    return bindlessSamDescriptorAllocator.GetGPUDescriptorHandleFromIndex(bindlessIndex);
}

void D3D12Renderer::RequestRelease(ID3D12Resource *resource) {
    BE1::ScopedLock lock(releaseRequestMutex);

    releaseRequests.Append(D3D12ReleaseRequest{ resource, nullptr });
}

void D3D12Renderer::DrainReleaseRequestsToRetireList() {
    BE1::Array<D3D12ReleaseRequest> localRequests;
    {
        BE1::ScopedLock lock(releaseRequestMutex);
        localRequests.Swap(releaseRequests);
    }

    if (localRequests.IsEmpty()) {
        return;
    }

    uint64_t graphicsFenceValue = SignalFence(RHI::CommandQueueType::Graphics);
    uint64_t computeFenceValue = SignalFence(RHI::CommandQueueType::Compute);

    for (D3D12ReleaseRequest &request : localRequests) {
        retiredResources.Append(D3D12RetiredResource{ request.resourceToRelease, request.objectToDelete, graphicsFenceValue, computeFenceValue});
    }
}

void D3D12Renderer::FreeRetiredResources(bool waitPendings) {
    int retiredIndex = 0;

    const int numRetiredResources = retiredResources.Count();
    for (retiredIndex = 0; retiredIndex < numRetiredResources; ++retiredIndex) {
        D3D12RetiredResource &retiredResource = retiredResources[retiredIndex];
        if (waitPendings) {
            WaitFence(RHI::CommandQueueType::Graphics, retiredResource.graphicsFenceValue);
            WaitFence(RHI::CommandQueueType::Compute, retiredResource.computeFenceValue);

            retiredResource.graphicsFenceValue = 0;
            retiredResource.computeFenceValue = 0;
        } else {
            if (!IsFenceComplete(RHI::CommandQueueType::Graphics, retiredResource.graphicsFenceValue) ||
                !IsFenceComplete(RHI::CommandQueueType::Compute, retiredResource.computeFenceValue)) {
                break;
            }
        }
        SAFE_RELEASE(retiredResource.resourceToRelease);
        SAFE_DELETE(retiredResource.objectToDelete)
    }

    if (retiredIndex > 0) {
        // 아직 fence 가 완료되지 않은 리소스들을 앞으로 당긴다.
        int remainingCount = retiredResources.Count() - retiredIndex;
        for (int i = 0; i < remainingCount; ++i) {
            retiredResources[i] = retiredResources[retiredIndex + i];
        }
        retiredResources.SetCount(remainingCount, false);
    }
}

RHI::CommandList *D3D12Renderer::BeginUtilityCommandList(RHI::CommandQueueType queueType) {
    D3D12CommandListPool *commandListPool = nullptr;
    D3D12CommandList *commandList = nullptr;
    BE1::PlatformMutex *mutex = nullptr;

    if (queueType == RHI::CommandQueueType::Graphics) {
        mutex = utilGraphicsCommandListMutex;
        commandListPool = utilGraphicsCommandListPool;
    } else {
        mutex = utilComputeCommandListMutex;
        commandListPool = utilComputeCommandListPool;
    }

    BE1::PlatformMutex::Lock(mutex);

    commandList = static_cast<D3D12CommandList *>(commandListPool->Alloc());
    if (!commandList) {
        BE1::PlatformMutex::Unlock(mutex);
        return nullptr;
    }

    // command allocator 를 재사용하도록 리셋하고, command list 를 command allocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    return commandList;
}

void D3D12Renderer::EndUtilityCommandList(RHI::CommandList *commandList) {
    D3D12CommandList* _commandList = static_cast<D3D12CommandList *>(commandList);
    D3D12CommandListPool *commandListPool = _commandList->GetParentPool();
    BE1::PlatformMutex *mutex = nullptr;

    if (commandListPool == utilGraphicsCommandListPool) {
        mutex = utilGraphicsCommandListMutex;
    } else if (commandListPool == utilComputeCommandListPool) {
        mutex = utilComputeCommandListMutex;
    } else {
        BE_ERRLOG("D3D12Renderer::EndCommandList: invalid command list pool\n");
        return;
    }

    commandListPool->Free(static_cast<D3D12CommandList *>(commandList));

    BE1::PlatformMutex::Unlock(mutex);
}

void D3D12Renderer::SetConstants(RHI::CommandList *commandList, int shaderRegister, const void *data, uint32_t size, uint32_t offset) {
    assert(size % sizeof(uint32_t) == 0 && offset % sizeof(uint32_t) == 0);

    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);

    const D3D12PipelineState::RegisterBinder &binder = _commandList->currentPSO->binder;
    assert(shaderRegister < COUNT_OF(binder.rootParameterIndexLUT.constants));

    int rootParameterIndex = binder.rootParameterIndexLUT.constants[shaderRegister];
    if (rootParameterIndex >= 64) {
        BE_ERRLOG("D3D12Renderer::SetConstants: Invalid root parameter index\n");
        return;
    }

    const uint32_t write32BitIndex = offset / sizeof(uint32_t);

    D3D12RootBindingData *rootBindingData = _commandList->GetRootBindingData();
    assert(shaderRegister < COUNT_OF(rootBindingData->rootConstants));
    assert(write32BitIndex + size / sizeof(uint32_t) <= COUNT_OF(rootBindingData->rootConstants[shaderRegister]));

    const D3D12_ROOT_PARAMETER1 &rootParameter = _commandList->currentPSO->rootSignatureDesc->Desc_1_1.pParameters[rootParameterIndex];
    assert(rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);
    assert(rootParameter.Constants.ShaderRegister == shaderRegister);
    assert(rootParameter.Constants.RegisterSpace == 0);
    assert(write32BitIndex + size / sizeof(uint32_t) <= rootParameter.Constants.Num32BitValues);

    memcpy(rootBindingData->rootConstants[shaderRegister] + write32BitIndex, data, size);

    if (_commandList->currentPSO->isGraphics) {
        _commandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        _commandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}

void D3D12Renderer::CreateDevice(IDXGIAdapter1 **adapterPtr) {
    D3D_FEATURE_LEVEL featurelevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    IDXGIAdapter1 *currentAdapter = nullptr;

    for (UINT adapterIndex = 0; ; ++adapterIndex) {
        SAFE_RELEASE(currentAdapter);
        HRESULT hr = dxgiFactory->EnumAdapters1(adapterIndex, &currentAdapter);
        if (hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }

        DXGI_ADAPTER_DESC1 currentAdapterDesc;
        hr = currentAdapter->GetDesc1(&currentAdapterDesc);
        if (SUCCEEDED(hr)) {
            if (currentAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                continue;
            }

            for (const D3D_FEATURE_LEVEL &featureLevel : featurelevels) {
                // D3D12 디바이스 생성
                if (SUCCEEDED(D3D12CreateDevice(currentAdapter, featureLevel, IID_PPV_ARGS(&device)))) {
                    break;
                }
            }

            if (device) {
                break;
            }
        }
    }

    if (!device) {
        BE_FATALERROR("Failed to create D3D12 device");
    }

    *adapterPtr = currentAdapter;
}

#ifdef USE_D3D12_MEMALLOC
void D3D12Renderer::PrintMemoryAllocatorStats() {
    D3D12MA::Budget localBudget;
    D3D12MA::Budget nonLocalBudget;

    allocator->GetBudget(&localBudget, &nonLocalBudget);

    // TODO: 확인 필요
    // GPU 에서 사용 중인 메모리 크기, 사용 가능한 메모리 크기
    BE_LOG("D3D12 reports total usage %s with budget %s (%.2f %%)\n", BE1::Str::FormatBytes(localBudget.UsageBytes).c_str(), BE1::Str::FormatBytes(localBudget.BudgetBytes).c_str(), (100.0f * localBudget.UsageBytes) / localBudget.BudgetBytes);
    // D3D12 heap 에 할당된 리소스의 개수, 크기
    BE_LOG("allocated out of %u D3D12 memory heaps taking %s\n", localBudget.Stats.BlockCount, BE1::Str::FormatBytes(localBudget.Stats.BlockBytes).c_str());
    // 프로그램에서 실제 사용 중인 리소스 메모리의 개수, 크기
    BE_LOG("GPU memory currently has %u allocations taking %s\n", localBudget.Stats.AllocationCount, BE1::Str::FormatBytes(localBudget.Stats.AllocationBytes).c_str());
}
#endif

void D3D12Renderer::SetName(RHI::GPUResource *resource, const char *name) const {
    ID3D12Resource *_resource = reinterpret_cast<ID3D12Resource *>(resource->GetNativeResource());
    if (_resource) {
        wchar_t wName[1024];
        BE1::PlatformWinUtils::UTF8ToUCS2(name, wName, COUNT_OF(wName));
        _resource->SetName(wName);
    }
}

void D3D12Renderer::SetBlendFactor(RHI::CommandList *commandList, const BE1::Color4 &rgba) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    _commandList->SetBlendFactor(rgba);
}

void D3D12Renderer::SetStencilRef(RHI::CommandList *commandList, uint32_t value) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    _commandList->SetStencilRef(value);
}

void D3D12Renderer::SetShadingRate(RHI::CommandList *commandList, RHI::ShadingRate shadingRate) {
    if (!supportsVRS) {
        return;
    }
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    _commandList->SetShadingRate(shadingRate);
}

void D3D12Renderer::SetViewport(RHI::CommandList *commandList, const BE1::Rect &viewportRect) {
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = viewportRect.x;
    viewport.TopLeftY = viewportRect.y;
    viewport.Width = viewportRect.w;
    viewport.Height = viewportRect.h;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    _commandList->GetGraphicsCommandList()->RSSetViewports(1, &viewport);
}

void D3D12Renderer::SetScissorRect(RHI::CommandList *commandList, const BE1::Rect &scissorRect) {
    static_assert(sizeof(BE1::Rect) == sizeof(D3D12_RECT));
    static_assert(offsetof(BE1::Rect, x) == offsetof(D3D12_RECT, left));
    static_assert(offsetof(BE1::Rect, y) == offsetof(D3D12_RECT, top));
    static_assert(offsetof(BE1::Rect, w) == offsetof(D3D12_RECT, right));
    static_assert(offsetof(BE1::Rect, h) == offsetof(D3D12_RECT, bottom));

    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    _commandList->GetGraphicsCommandList()->RSSetScissorRects(1, (D3D12_RECT *)&scissorRect);
}

void D3D12Renderer::SetDepthBounds(RHI::CommandList *commandList, float depthMin, float depthMax) {
    if (!supportsDepthBoundsTest) {
        return;
    }
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    _commandList->GetGraphicsCommandList()->OMSetDepthBounds(depthMin, depthMax);
}

void D3D12Renderer::ClearUAV(RHI::CommandList *commandList, const RHI::GPUResource *resource, RHI::ViewID viewID, uint32_t value) {
    const D3D12UAVEntry *uavEntry = nullptr;

    const D3D12Buffer *buffer = reinterpret_cast<const D3D12Buffer *>(resource->GetNativeBufferObject());
    if (buffer) {
        if (buffer->HasValidView(RHI::ViewType::UAV, viewID)) {
            uavEntry = &buffer->uavEntries[viewID.value];
        }
    } else {
        const D3D12Texture *texture = reinterpret_cast<const D3D12Texture *>(resource->GetNativeTextureObject());
        if (texture) {
            if (texture->HasValidView(RHI::ViewType::UAV, viewID)) {
                uavEntry = &texture->uavEntries[viewID.value];
            }
            
        }
    }

    if (!uavEntry) {
        BE_ERRLOG("D3D12Renderer::ClearUAV: Couldn't find valid UAV entry\n");
        return;
    }

    if ((*uavEntry).cpuDescriptorHandle.ptr == 0) {
        BE_ERRLOG("D3D12Renderer::ClearUAV: Invalid descriptor handle\n");
        return;
    }

    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    D3D12DescriptorTablePool *descriptorTablePool = _commandList->resDescriptorTablePool;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTableStart;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTableStart;

    if (descriptorTablePool->AllocRange(1, &cpuDescriptorTableStart, &gpuDescriptorTableStart)) {
        const D3D12_CPU_DESCRIPTOR_HANDLE &srcDescriptorHandle = (*uavEntry).cpuDescriptorHandle;
        CD3DX12_CPU_DESCRIPTOR_HANDLE destCpuDescriptorHandle(cpuDescriptorTableStart, 0, descriptorTablePool->GetDescriptorHandleSize());
        CD3DX12_GPU_DESCRIPTOR_HANDLE destGpuDescriptorHandle(gpuDescriptorTableStart, 0, descriptorTablePool->GetDescriptorHandleSize());

        device->CopyDescriptorsSimple(1, destCpuDescriptorHandle, srcDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        const UINT values[4] = { value, value, value, value };
        _commandList->GetGraphicsCommandList()->ClearUnorderedAccessViewUint(destGpuDescriptorHandle, srcDescriptorHandle, reinterpret_cast<ID3D12Resource *>(resource->GetNativeResource()), values, 0, nullptr);
    }
}

void D3D12Renderer::CopyBuffer(RHI::CommandList *commandList, const RHI::Buffer *dstBuffer, uint32_t dstOffset, const RHI::Buffer *srcBuffer, uint32_t srcOffset, uint32_t size) {
    const D3D12Buffer *_dstBuffer = static_cast<const D3D12Buffer *>(dstBuffer);
    const D3D12Buffer *_srcBuffer = static_cast<const D3D12Buffer *>(srcBuffer);

    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    _commandList->GetGraphicsCommandList()->CopyBufferRegion(_dstBuffer->GetResource(), _dstBuffer->resourceOffset + dstOffset, _srcBuffer->GetResource(), _srcBuffer->resourceOffset + srcOffset, size);
}

void D3D12Renderer::CopyTexture(RHI::CommandList *commandList, const RHI::Texture *dstTexture, uint32_t dstSlice, uint32_t dstMipLevel, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
    const RHI::Texture *srcTexture, uint32_t srcSlice, uint32_t srcMipLevel, uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t width, uint32_t height, uint32_t depth) {
    const D3D12Texture *_dstTexture = static_cast<const D3D12Texture *>(dstTexture);
    const D3D12Texture *_srcTexture = static_cast<const D3D12Texture *>(srcTexture);

    CD3DX12_TEXTURE_COPY_LOCATION dstLocation(_dstTexture->GetResource(), D3D12CalcSubresource(dstMipLevel, dstSlice, 0, _dstTexture->textureDesc.MipLevels, _dstTexture->textureDesc.DepthOrArraySize));
    CD3DX12_TEXTURE_COPY_LOCATION srcLocation(_srcTexture->GetResource(), D3D12CalcSubresource(srcMipLevel, srcSlice, 0, _srcTexture->textureDesc.MipLevels, _srcTexture->textureDesc.DepthOrArraySize));

    D3D12_BOX srcBox = { srcX, srcY, srcZ, (UINT)width, (UINT)height, (UINT)depth };
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    _commandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, dstX, dstY, dstZ, &srcLocation, &srcBox);
}

D3D12_RESOURCE_STATES D3D12Renderer::ToD3D12ResourceState(RHI::GPUResourceState resourceState) {
    D3D12_RESOURCE_STATES ret = D3D12_RESOURCE_STATE_COMMON;
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::ShaderResource)) {
        ret |= D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::ShaderResourceCompute)) {
        ret |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::UnorderedAccess)) {
        ret |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::CopyDst)) {
        ret |= D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::CopySrc)) {
        ret |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::RenderTarget)) {
        ret |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::DepthWrite)) {
        ret |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::DepthRead)) {
        ret |= D3D12_RESOURCE_STATE_DEPTH_READ;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::ShadingRateSource)) {
        ret |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
    }
    if (BE1::HasAnyFlag(resourceState, RHI::GPUResourceState::VertexBuffer | RHI::GPUResourceState::ConstantBuffer)) {
        ret |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::IndexBuffer)) {
        ret |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::IndirectArgument)) {
        ret |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::RTAccelerationStructure)) {
        ret |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    }
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::Prediction)) {
        ret |= D3D12_RESOURCE_STATE_PREDICATION;
    }
    return ret;
}

void D3D12Renderer::Barrier(RHI::CommandList *commandList, const RHI::GPUBarrier *barriers, uint32_t barrierCount) {
    D3D12_RESOURCE_BARRIER *actualBarriers = (D3D12_RESOURCE_BARRIER *)_alloca(sizeof(D3D12_RESOURCE_BARRIER) * barrierCount);
    int actualBarrierCount = 0;

    for (uint32_t barrierIndex = 0; barrierIndex < barrierCount; ++barrierIndex) {
        const RHI::GPUBarrier *barrier = &barriers[barrierIndex];

        if (barrier->type == RHI::GPUBarrier::Type::Memory) {
            D3D12_RESOURCE_BARRIER &actualBarrier = actualBarriers[actualBarrierCount++];
            actualBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            actualBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            // UAV.pResource == nullptr 일 때는 특정 UAV 가 아닌 모든 UAV 에 대한 메모리 배리어가 된다.
            actualBarrier.UAV.pResource = !barrier->memoryBarrier.resource ? nullptr : reinterpret_cast<ID3D12Resource *>(barrier->memoryBarrier.resource->GetNativeResource());
        } else if (barrier->type == RHI::GPUBarrier::Type::Buffer) {
            if (barrier->bufferBarrier.stateBefore != barrier->bufferBarrier.stateAfter) {
                D3D12_RESOURCE_BARRIER &actualBarrier = actualBarriers[actualBarrierCount++];
                actualBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                actualBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                actualBarrier.Transition.pResource = reinterpret_cast<ID3D12Resource *>(barrier->bufferBarrier.buffer->GetNativeResource());
                actualBarrier.Transition.StateBefore = ToD3D12ResourceState(barrier->bufferBarrier.stateBefore);
                actualBarrier.Transition.StateAfter = ToD3D12ResourceState(barrier->bufferBarrier.stateAfter);
                actualBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            }
        } else if (barrier->type == RHI::GPUBarrier::Type::Texture) {
            if (barrier->textureBarrier.stateBefore != barrier->textureBarrier.stateAfter) {
                D3D12_RESOURCE_BARRIER &actualBarrier = actualBarriers[actualBarrierCount++];
                actualBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                actualBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                actualBarrier.Transition.pResource = reinterpret_cast<ID3D12Resource *>(barrier->textureBarrier.texture->GetNativeResource());
                actualBarrier.Transition.StateBefore = ToD3D12ResourceState(barrier->textureBarrier.stateBefore);
                actualBarrier.Transition.StateAfter = ToD3D12ResourceState(barrier->textureBarrier.stateAfter);

                if (barrier->textureBarrier.slice >= 0 || barrier->textureBarrier.mipLevel >= 0) {
                    const D3D12Texture *_texture = static_cast<const D3D12Texture *>(barrier->textureBarrier.texture);
                    actualBarrier.Transition.Subresource = D3D12CalcSubresource(barrier->textureBarrier.mipLevel, barrier->textureBarrier.slice, 0, _texture->textureDesc.MipLevels, _texture->textureDesc.DepthOrArraySize);
                } else {
                    actualBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                }
            }
        } else if (barrier->type == RHI::GPUBarrier::Type::Aliasing) {
            D3D12_RESOURCE_BARRIER &actualBarrier = actualBarriers[actualBarrierCount++];
            actualBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
            actualBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            actualBarrier.Aliasing.pResourceBefore = reinterpret_cast<ID3D12Resource *>(barrier->aliasingBarrier.resourceBefore->GetNativeResource());
            actualBarrier.Aliasing.pResourceAfter = reinterpret_cast<ID3D12Resource *>(barrier->aliasingBarrier.resourceAfter->GetNativeResource());
        }
    }

    if (actualBarrierCount > 0) {
        // NOTE: Compute 커맨드 리스트라면, ResourceState 에 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE 를 지정할 수 없음에 주의한다.
        D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
        _commandList->GetGraphicsCommandList()->ResourceBarrier(actualBarrierCount, actualBarriers);
    }
}

void D3D12Renderer::ReadPixels(RHI::CommandList *commandList, const RHI::SwapChain *swapChain, int x, int y, int width, int height, BE1::Image::Format dstFormat, void *outPixels) {
    const D3D12SwapChain *swapChainInternal = static_cast<const D3D12SwapChain *>(swapChain);
    ID3D12Resource *backBufferResource = swapChainInternal->GetCurrentBackBuffer();
    D3D12_RESOURCE_DESC backBufferDesc = backBufferResource->GetDesc();

    // 백버퍼 리소스의 특정 밉레벨 (서브 리소스) 의 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLevelFootprint;
    UINT64 mipLevelSize;
    device->GetCopyableFootprints(&backBufferDesc, 0, 1, 0, &mipLevelFootprint, nullptr, nullptr, &mipLevelSize);

    mipLevelFootprint.Footprint.Width = width;
    mipLevelFootprint.Footprint.Height = height;

    // 리드백 버퍼의 크기를 계산한다.
    UINT64 readBufferSize = mipLevelFootprint.Footprint.RowPitch * height;

    // 리드백 버퍼를 생성한다.
    BE1::Image::Format backBufferFormat;
    bool isSRGB;
    D3D12Renderer::DXGIFormatToImageFormat(backBufferDesc.Format, &backBufferFormat, &isSRGB);
    D3D12Buffer *readbackBuffer = static_cast<D3D12Buffer *>(D3D12Renderer::GetRenderer()->CreateBuffer(RHI::BufferUsage::Readback, RHI::BufferFlag::None, readBufferSize, nullptr));

    // 백버퍼에서 리드백 버퍼로 복사한다.
    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = backBufferResource;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    D3D12_BOX srcBox = {};
    srcBox.left = x;
    srcBox.top = y;
    srcBox.front = 0;
    srcBox.right = x + width;
    srcBox.bottom = y + height;
    srcBox.back = 1;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = readbackBuffer->GetResource();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLocation.PlacedFootprint = mipLevelFootprint;

    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    _commandList->ResourceBarrier(backBufferResource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_SOURCE);
    _commandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, &srcBox);
    _commandList->ResourceBarrier(backBufferResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);
    _commandList->CloseAndExecute();

    // GPU 에서 복사가 끝날 때까지 기다린다.
    Finish(RHI::CommandQueueType::Graphics);

    // 리드백 버퍼의 내용을 메모리로 읽어온다.
    const byte *srcPtr = (byte *)readbackBuffer->mappedPtr;
    byte *dstPtr = nullptr;

    BE1::Image tempImage;
    if (backBufferFormat != dstFormat) {
        // 컨버팅이 필요하다면, 리드백 버퍼의 내용을 tempImage 에 카피할 준비를 한다.
        tempImage.Create2D(width, height, 1, backBufferFormat, isSRGB ? BE1::Image::GammaSpace::sRGB : BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
        dstPtr = tempImage.GetPixels();
    } else {
        // 컨버팅할 필요가 없다면, 리드백 버퍼의 내용을 그대로 outPixels 로 카피할 준비를 한다.
        dstPtr = (byte *)outPixels;
    }

    int srcPitch = mipLevelFootprint.Footprint.RowPitch;
    int dstPitch = static_cast<int>(BE1::Image::MemRequired(width, 1, 1, 1, 1, backBufferFormat));

    for (UINT y = 0; y < height; ++y) {
        BE1::simdProcessor->Memcpy(dstPtr, srcPtr, srcPitch);
        srcPtr += srcPitch;
        dstPtr += dstPitch;
    }

    // 리드백 버퍼 삭제
    DestroyBuffer(readbackBuffer, true);

    // 필요하다면 컨버팅한다.
    if (backBufferFormat != dstFormat) {
        BE1::Image dstImage;
        if (tempImage.ConvertFormat(dstFormat, dstImage)) {
            BE1::simdProcessor->Memcpy(outPixels, dstImage.GetPixels(), static_cast<int>(dstImage.SizeInBytes()));
        }
    }
}

void D3D12Renderer::BeginRenderPass(RHI::CommandList *commandList, const RHI::SwapChain *swapChain, const RHI::Texture *depthStencilTexture, const BE1::Color4 &clearColor, float clearDepth, uint8_t clearStencil, RHI::ClearFlag clearFlags) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    const D3D12SwapChain *d3d12SwapChain = static_cast<const D3D12SwapChain *>(swapChain);

    if (swapChain) {
        // 백버퍼를 렌더 타겟 상태로 전환하는 Barrier
        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = d3d12SwapChain->GetCurrentBackBuffer();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        _commandList->GetGraphicsCommandList()->ResourceBarrier(1, &barrier);

        // EndRenderPass 에서 자동으로 실행될 Barrier 를 미리 등록
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        _commandList->endRenderPassBarriers.Append(barrier);
    }
#if 1
    // 렌더 타겟
    D3D12_RENDER_PASS_RENDER_TARGET_DESC *rtDescPtr = nullptr;
    D3D12_RENDER_PASS_RENDER_TARGET_DESC rtDesc = {};
    if (swapChain) {
        rtDesc.cpuDescriptor = d3d12SwapChain->GetCurrentBackBufferRTVDescriptorHandle();
        if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Color)) {
            rtDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            rtDesc.BeginningAccess.Clear.ClearValue.Format = RHIFormatToDXGIFormat(d3d12SwapChain->GetRTVFormat());
            rtDesc.BeginningAccess.Clear.ClearValue.Color[0] = clearColor[0];
            rtDesc.BeginningAccess.Clear.ClearValue.Color[1] = clearColor[1];
            rtDesc.BeginningAccess.Clear.ClearValue.Color[2] = clearColor[2];
            rtDesc.BeginningAccess.Clear.ClearValue.Color[3] = clearColor[3];
        } else {
            rtDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
        }
        rtDesc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        rtDescPtr = &rtDesc;
    }

    // 뎁스/스텐실
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC *dsDescPtr = nullptr;
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC dsDesc = {};
    if (depthStencilTexture) {
        const D3D12Texture *_depthStencilTexture = static_cast<const D3D12Texture *>(depthStencilTexture);
        dsDesc.cpuDescriptor = _depthStencilTexture->dsvEntries[0].cpuDescriptorHandle;
        if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Depth)) {
            dsDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            dsDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = clearDepth;
        } else {
            dsDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
        }
        // depth 를 나중에 후처리할 일이 없다면 preserve 대신 discard 해도 된다.
        dsDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;

        if (IsStencilFormat(_depthStencilTexture->textureDesc.Format)) {
            if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Stencil)) {
                dsDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                dsDesc.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = clearStencil;
            } else {
                dsDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
            }
            dsDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
        } else {
            dsDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
            dsDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
        }
        dsDescPtr = &dsDesc;
    }

    _commandList->GetGraphicsCommandList()->BeginRenderPass(1, rtDescPtr, dsDescPtr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);
#else
    if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Color)) {
        _commandList->GetGraphicsCommandList()->ClearRenderTargetView(d3d12SwapChain->GetCurrentBackBufferDescriptorHandle(), clearColor, 0, nullptr);
    }
    if (BE1::HasAnyFlag(clearFlags, RHI::ClearFlag::Depth | RHI::ClearFlag::Stencil)) {
        D3D12_CLEAR_FLAGS clearDepthStencilFlags = 0;
        if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Depth)) {
            clearDepthStencilFlags |= D3D12_CLEAR_FLAG_DEPTH;
        }
        if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Stencil)) {
            clearDepthStencilFlags |= D3D12_CLEAR_FLAG_STENCIL;
        }
        _commandList->GetGraphicsCommandList()->ClearDepthStencilView(dsvDescriptorHandle, clearDepthStencilFlags, clearDepth, clearStencil, 0, nullptr);
    }
    _commandList->GetGraphicsCommandList()->OMSetRenderTargets(1, &d3d12SwapChain->GetCurrentBackBufferDescriptorHandle(), FALSE, &dsvDescriptorHandle);
#endif
}

static void GetInfoFromRTVDesc(const D3D12_RENDER_TARGET_VIEW_DESC &rtvDesc, UINT &mipSlice, UINT &firstArraySlice, UINT &arraySize) {
    switch (rtvDesc.ViewDimension) {
    case D3D12_RTV_DIMENSION_TEXTURE1D:
        mipSlice = rtvDesc.Texture1D.MipSlice;
        firstArraySlice = 0;
        arraySize = 1;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
        mipSlice = rtvDesc.Texture1DArray.MipSlice;
        firstArraySlice = rtvDesc.Texture1DArray.FirstArraySlice;
        arraySize = rtvDesc.Texture1DArray.ArraySize;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE2D:
        mipSlice = rtvDesc.Texture2D.MipSlice;
        firstArraySlice = 0;
        arraySize = 1;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE2DMS:
        mipSlice = 0;
        firstArraySlice = 0;
        arraySize = 1;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
        mipSlice = rtvDesc.Texture2DArray.MipSlice;
        firstArraySlice = rtvDesc.Texture2DArray.FirstArraySlice;
        arraySize = rtvDesc.Texture2DArray.ArraySize;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
        mipSlice = 0;
        firstArraySlice = rtvDesc.Texture2DMSArray.FirstArraySlice;
        arraySize = rtvDesc.Texture2DMSArray.ArraySize;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE3D:
        mipSlice = rtvDesc.Texture3D.MipSlice;
        firstArraySlice = 0;
        arraySize = 1;
        break;
    default:
        assert(0);
        break;
    }
}

static void GetInfoFromDSVDesc(const D3D12_DEPTH_STENCIL_VIEW_DESC &dsvDesc, UINT &mipSlice, UINT &firstArraySlice, UINT &arraySize) {
    switch (dsvDesc.ViewDimension) {
    case D3D12_DSV_DIMENSION_TEXTURE1D:
        mipSlice = dsvDesc.Texture1D.MipSlice;
        firstArraySlice = 0;
        arraySize = 1;
        break;
    case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
        mipSlice = dsvDesc.Texture1DArray.MipSlice;
        firstArraySlice = dsvDesc.Texture1DArray.FirstArraySlice;
        arraySize = dsvDesc.Texture1DArray.ArraySize;
        break;
    case D3D12_DSV_DIMENSION_TEXTURE2D:
        mipSlice = dsvDesc.Texture2D.MipSlice;
        firstArraySlice = 0;
        arraySize = 1;
        break;
    case D3D12_DSV_DIMENSION_TEXTURE2DMS:
        mipSlice = 0;
        firstArraySlice = 0;
        arraySize = 1;
        break;
    case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
        mipSlice = dsvDesc.Texture2DArray.MipSlice;
        firstArraySlice = dsvDesc.Texture2DArray.FirstArraySlice;
        arraySize = dsvDesc.Texture2DArray.ArraySize;
        break;
    case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
        mipSlice = 0;
        firstArraySlice = dsvDesc.Texture2DMSArray.FirstArraySlice;
        arraySize = dsvDesc.Texture2DMSArray.ArraySize;
        break;
    default:
        assert(0);
        break;
    }
}

void D3D12Renderer::BeginRenderPass(RHI::CommandList *commandList, const RHI::RenderPassImage renderPassImages[], int numRenderPassImages, RHI::RenderPassFlag flags) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);

    struct ResolveSource {
        ID3D12Resource *resource = nullptr;
        BOOL preserve = FALSE;
        uint32_t maxMipLevels;
        uint32_t maxArraySize;
        uint32_t mipSlice;
        uint32_t firstArraySlice;
        uint32_t arraySize;
    };
    ResolveSource rtResolveSources[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ResolveSource dsResolveSource = {};
    D3D12_RENDER_PASS_RENDER_TARGET_DESC rtDescs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC dsDesc = {};
    BE1::Array<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS> resolveSubresourceParams[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
    uint32_t rtCount = 0;
    BE1::Array<D3D12_RESOURCE_BARRIER> barriers;

    for (int i = 0; i < numRenderPassImages; ++i) {
        const RHI::RenderPassImage &renderPassImage = renderPassImages[i];
        const D3D12Texture *texture = static_cast<const D3D12Texture *>(renderPassImage.texture);
        UINT destMipSlice = 0;
        UINT destFirstArraySlice = 0;
        UINT destArraySize = 1;

        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE beginningAccessType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
        if (renderPassImage.loadAction == RHI::RenderPassImage::LoadAction::Load) {
            beginningAccessType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
        } else if (renderPassImage.loadAction == RHI::RenderPassImage::LoadAction::Clear) {
            beginningAccessType = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
        }
        D3D12_RENDER_PASS_ENDING_ACCESS_TYPE endingAccessType = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
        if (renderPassImage.storeAction == RHI::RenderPassImage::StoreAction::Store) {
            endingAccessType = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        }

        switch (renderPassImage.type) {
        case RHI::RenderPassImage::Type::Color:
        {
            // 사용할 Color 렌더타겟 정보를 기록한다.
            assert(texture->HasValidView(RHI::ViewType::RTV, renderPassImage.viewID));
            const D3D12RTVEntry &rtvEntry = texture->rtvEntries[renderPassImage.viewID.value];
            D3D12_RENDER_PASS_RENDER_TARGET_DESC &rtDesc = rtDescs[rtCount];
            rtDesc.cpuDescriptor = rtvEntry.cpuDescriptorHandle;
            rtDesc.BeginningAccess.Type = beginningAccessType;
            rtDesc.BeginningAccess.Clear.ClearValue = texture->optimizedClearValue;
            if (renderPassImage.hasClearValue) {
                rtDesc.BeginningAccess.Clear.ClearValue.Color[0] = renderPassImage.clearValue.color[0];
                rtDesc.BeginningAccess.Clear.ClearValue.Color[1] = renderPassImage.clearValue.color[1];
                rtDesc.BeginningAccess.Clear.ClearValue.Color[2] = renderPassImage.clearValue.color[2];
                rtDesc.BeginningAccess.Clear.ClearValue.Color[3] = renderPassImage.clearValue.color[3];
            }
            rtDesc.EndingAccess.Type = endingAccessType;

            // 텍스쳐의 RTV 정보를 가져온다.
            GetInfoFromRTVDesc(rtvEntry.viewDesc, destMipSlice, destFirstArraySlice, destArraySize);

            // Resolve 가능하도록 미리 정보를 기록한다.
            ResolveSource &rtResolveSource = rtResolveSources[rtCount];
            rtResolveSource.resource = reinterpret_cast<ID3D12Resource *>(texture->GetNativeResource());
            rtResolveSource.preserve = endingAccessType == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
            rtResolveSource.maxMipLevels = texture->textureDesc.MipLevels;
            rtResolveSource.maxArraySize = texture->textureDesc.DepthOrArraySize;
            rtResolveSource.mipSlice = destMipSlice;
            rtResolveSource.firstArraySlice = destFirstArraySlice;
            rtResolveSource.arraySize = destArraySize;
            rtCount++;
            break;
        }
        case RHI::RenderPassImage::Type::DepthStencil:
        {
            // 사용할 Depth/Stencil 정보를 기록한다.
            assert(texture->HasValidView(RHI::ViewType::DSV, renderPassImage.viewID));
            const D3D12DSVEntry &dsvEntry = texture->dsvEntries[renderPassImage.viewID.value];
            dsDesc.cpuDescriptor = dsvEntry.cpuDescriptorHandle;
            dsDesc.DepthBeginningAccess.Type = beginningAccessType;
            dsDesc.DepthBeginningAccess.Clear.ClearValue = texture->optimizedClearValue;
            if (renderPassImage.hasClearValue) {
                dsDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = renderPassImage.clearValue.depthStencil.depth;
                dsDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = renderPassImage.clearValue.depthStencil.stencil;
            }
            dsDesc.DepthEndingAccess.Type = endingAccessType;

            if (IsStencilFormat(texture->textureDesc.Format)) {
                dsDesc.StencilBeginningAccess = dsDesc.DepthBeginningAccess;
                dsDesc.StencilEndingAccess = dsDesc.DepthEndingAccess;
            } else {
                dsDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
                dsDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
            }

            // 텍스쳐의 DSV 정보를 가져온다.
            GetInfoFromDSVDesc(dsvEntry.viewDesc, destMipSlice, destFirstArraySlice, destArraySize);

            // Resolve 가능하도록 미리 정보를 기록한다.
            dsResolveSource.resource = reinterpret_cast<ID3D12Resource *>(texture->GetNativeResource());
            dsResolveSource.preserve = endingAccessType == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
            dsResolveSource.maxMipLevels = texture->textureDesc.MipLevels;
            dsResolveSource.maxArraySize = texture->textureDesc.DepthOrArraySize;
            dsResolveSource.mipSlice = destMipSlice;
            dsResolveSource.firstArraySlice = destFirstArraySlice;
            dsResolveSource.arraySize = destArraySize;
            break;
        }
        case RHI::RenderPassImage::Type::ResolveColor:
        {
            // 지정한 Color 렌더타겟의 Resolve 정보를 기록한다.
            const ResolveSource &resolveSource = rtResolveSources[renderPassImage.resolveSourceIndex];
            D3D12_RENDER_PASS_RENDER_TARGET_DESC &rtDesc = rtDescs[renderPassImage.resolveSourceIndex];
            rtDesc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
            rtDesc.EndingAccess.Resolve.Format = texture->textureDesc.Format;
            rtDesc.EndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE::D3D12_RESOLVE_MODE_AVERAGE;
            rtDesc.EndingAccess.Resolve.pSrcResource = resolveSource.resource;
            rtDesc.EndingAccess.Resolve.pDstResource = reinterpret_cast<ID3D12Resource *>(texture->GetNativeResource());
            rtDesc.EndingAccess.Resolve.PreserveResolveSource = resolveSource.preserve;

            if (texture->HasValidView(RHI::ViewType::RTV, renderPassImage.viewID)) {
                const D3D12RTVEntry &rtvEntry = texture->rtvEntries[renderPassImage.viewID.value];
                GetInfoFromRTVDesc(rtvEntry.viewDesc, destMipSlice, destFirstArraySlice, destArraySize);
            } else {
                assert(renderPassImage.viewID.IsPrimary());
                destMipSlice = 0;
                destFirstArraySlice = 0;
                destArraySize = texture->textureDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : texture->textureDesc.DepthOrArraySize;
            }

            UINT destWidth = BE1::Max((int)(texture->textureDesc.Width >> destMipSlice), 1);
            UINT destHeight = BE1::Max((int)(texture->textureDesc.Height >> destMipSlice), 1);
            UINT arraySize = BE1::Min(destArraySize, resolveSource.arraySize);

            BE1::Array<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS> &subresourceParams = resolveSubresourceParams[renderPassImage.resolveSourceIndex];

            for (UINT slice = 0; slice < arraySize; ++slice) {
                D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS &params = subresourceParams.Alloc();
                params.SrcSubresource = D3D12CalcSubresource(resolveSource.mipSlice, resolveSource.firstArraySlice + slice, 0, resolveSource.maxMipLevels, resolveSource.maxArraySize);
                params.SrcRect.left = 0;
                params.SrcRect.top = 0;
                params.SrcRect.right = destWidth;
                params.SrcRect.bottom = destHeight;
                params.DstSubresource = D3D12CalcSubresource(destMipSlice, destFirstArraySlice + slice, 0, texture->textureDesc.MipLevels, texture->textureDesc.DepthOrArraySize);
                params.DstX = 0;
                params.DstY = 0;
            }

            rtDesc.EndingAccess.Resolve.pSubresourceParameters = subresourceParams.Ptr();
            rtDesc.EndingAccess.Resolve.SubresourceCount = subresourceParams.Count();
            break;
        }
        case RHI::RenderPassImage::Type::ResolveDepth:
        {
            // Depth/Stencil 의 Resolve 정보를 기록한다.
            assert(texture->HasValidView(RHI::ViewType::DSV, renderPassImage.viewID));
            const D3D12DSVEntry &dsvEntry = texture->dsvEntries[renderPassImage.viewID.value];
            dsDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
            dsDesc.DepthEndingAccess.Resolve.Format = texture->textureDesc.Format;
            switch (renderPassImage.depthResolveMode) {
            case RHI::RenderPassImage::DepthResolveMode::Min:
                dsDesc.DepthEndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE_MIN;
                break;
            case RHI::RenderPassImage::DepthResolveMode::Max:
                dsDesc.DepthEndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE_MAX;
                break;
            }
            dsDesc.DepthEndingAccess.Resolve.pSrcResource = dsResolveSource.resource;
            dsDesc.DepthEndingAccess.Resolve.pDstResource = reinterpret_cast<ID3D12Resource *>(texture->GetNativeResource());
            dsDesc.DepthEndingAccess.Resolve.PreserveResolveSource = dsResolveSource.preserve;

            // 텍스쳐의 DSV 정보를 가져온다.
            GetInfoFromDSVDesc(dsvEntry.viewDesc, destMipSlice, destFirstArraySlice, destArraySize);

            UINT destWidth = BE1::Max((int)(texture->textureDesc.Width >> destMipSlice), 1);
            UINT destHeight = BE1::Max((int)(texture->textureDesc.Height >> destMipSlice), 1);
            UINT arraySize = BE1::Min(destArraySize, dsResolveSource.arraySize);

            BE1::Array<D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS> &subresourceParams = resolveSubresourceParams[renderPassImage.resolveSourceIndex];

            for (UINT slice = 0; slice < arraySize; ++slice) {
                D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS &params = subresourceParams.Alloc();
                params.SrcSubresource = D3D12CalcSubresource(dsResolveSource.mipSlice, dsResolveSource.firstArraySlice + slice, 0, dsResolveSource.maxMipLevels, dsResolveSource.maxArraySize);
                params.SrcRect.left = 0;
                params.SrcRect.top = 0;
                params.SrcRect.right = destWidth;
                params.SrcRect.bottom = destHeight;
                params.DstSubresource = D3D12CalcSubresource(destMipSlice, destFirstArraySlice + slice, 0, texture->textureDesc.MipLevels, texture->textureDesc.DepthOrArraySize);
                params.DstX = 0;
                params.DstY = 0;
            }

            dsDesc.DepthEndingAccess.Resolve.pSubresourceParameters = subresourceParams.Ptr();
            dsDesc.DepthEndingAccess.Resolve.SubresourceCount = subresourceParams.Count();

            if (IsStencilFormat(texture->textureDesc.Format)) {
                dsDesc.StencilEndingAccess = dsDesc.DepthEndingAccess;
            } else {
                dsDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
                dsDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
            }
            break;
        }
        case RHI::RenderPassImage::Type::ShadingRateSource:
            _commandList->shadingRateImage = reinterpret_cast<ID3D12Resource *>(texture->GetNativeResource());
            break;
        default:
            assert(0);
            break;
        }

        // RenderPass 시작 배리어를 barriers 에 모은다.
        {
            D3D12_RESOURCE_STATES beforeState = ToD3D12ResourceState(renderPassImage.beforeState);
            D3D12_RESOURCE_STATES afterState = ToD3D12ResourceState(renderPassImage.duringState);

            if (renderPassImage.type == RHI::RenderPassImage::Type::ResolveColor || renderPassImage.type == RHI::RenderPassImage::Type::ResolveDepth) {
                afterState = D3D12_RESOURCE_STATE_RESOLVE_DEST;
            }
            if (beforeState != afterState) {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.Transition.pResource = reinterpret_cast<ID3D12Resource *>(texture->GetNativeResource());
                barrier.Transition.StateBefore = beforeState;
                barrier.Transition.StateAfter = afterState;

                if (renderPassImage.viewID.IsPrimary()) {
                    // NOTE: viewID 가 Primary 라는 게 절대적으로 텍스쳐 전체 View 를 가리키는 게 아닌데 일단은 그렇게 가정한다.
                    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    barriers.Append(barrier);
                } else {
                    for (UINT slice = 0; slice < destArraySize; ++slice) {
                        barrier.Transition.Subresource = D3D12CalcSubresource(destMipSlice, destFirstArraySlice + slice, 0, texture->textureDesc.MipLevels, texture->textureDesc.DepthOrArraySize);
                        barriers.Append(barrier);
                    }
                }
            }
        }

        // RenderPass 종료 배리어를 _commandList->endRenderPassBarriers 에 모은다. 추후에 EndRenderPass 에서 자동으로 실행된다.
        {
            D3D12_RESOURCE_STATES beforeState = ToD3D12ResourceState(renderPassImage.duringState);
            D3D12_RESOURCE_STATES afterState = ToD3D12ResourceState(renderPassImage.afterState);

            if (renderPassImage.type == RHI::RenderPassImage::Type::ResolveColor || renderPassImage.type == RHI::RenderPassImage::Type::ResolveDepth) {
                beforeState = D3D12_RESOURCE_STATE_RESOLVE_DEST;
            }
            if (beforeState != afterState) {
                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barrier.Transition.pResource = reinterpret_cast<ID3D12Resource *>(texture->GetNativeResource());
                barrier.Transition.StateBefore = beforeState;
                barrier.Transition.StateAfter = afterState;

                if (renderPassImage.viewID.IsPrimary()) {
                    // NOTE: viewID 가 Primary 라는 게 절대적으로 텍스쳐 전체 View 를 가리키는 게 아닌데 일단은 그렇게 가정한다.
                    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    _commandList->endRenderPassBarriers.Append(barrier);
                } else {
                    for (UINT slice = 0; slice < destArraySize; ++slice) {
                        barrier.Transition.Subresource = D3D12CalcSubresource(destMipSlice, destFirstArraySlice + slice, 0, texture->textureDesc.MipLevels, texture->textureDesc.DepthOrArraySize);
                        _commandList->endRenderPassBarriers.Append(barrier);
                    }
                }
            }
        }
    }

    if (!barriers.IsEmpty()) {
        _commandList->GetGraphicsCommandList()->ResourceBarrier(barriers.Count(), barriers.Ptr());
    }

    if (_commandList->shadingRateImage) {
        _commandList->GetGraphicsCommandList()->RSSetShadingRateImage(_commandList->shadingRateImage);
    }

    D3D12_RENDER_PASS_FLAGS renderPassFlags = D3D12_RENDER_PASS_FLAG_NONE;
    if (BE1::HasFlag(flags, RHI::RenderPassFlag::AllowUAVWrites)) {
        renderPassFlags |= D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
    }
    if (BE1::HasFlag(flags, RHI::RenderPassFlag::Suspending)) {
        renderPassFlags |= D3D12_RENDER_PASS_FLAG_SUSPENDING_PASS;
    }
    if (BE1::HasFlag(flags, RHI::RenderPassFlag::Resuming)) {
        renderPassFlags |= D3D12_RENDER_PASS_FLAG_RESUMING_PASS;
    }

    _commandList->GetGraphicsCommandList()->BeginRenderPass(rtCount, rtDescs, dsDesc.cpuDescriptor.ptr == 0 ? nullptr : &dsDesc, renderPassFlags);
}

void D3D12Renderer::EndRenderPass(RHI::CommandList *commandList) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);

    _commandList->GetGraphicsCommandList()->EndRenderPass();

    if (_commandList->shadingRateImage) {
        _commandList->GetGraphicsCommandList()->RSSetShadingRateImage(nullptr);
        _commandList->shadingRateImage = nullptr;
    }

    // BeginRenderPass 미리 등록한 Barrier 를 실행한다.
    if (!_commandList->endRenderPassBarriers.IsEmpty()) {
        _commandList->GetGraphicsCommandList()->ResourceBarrier((UINT)_commandList->endRenderPassBarriers.Count(), _commandList->endRenderPassBarriers.Ptr());
    }
    _commandList->endRenderPassBarriers.SetCount(0, false);
}

void D3D12Renderer::Draw(RHI::CommandList *commandList, uint32_t vertexCount, uint32_t startVertexLocation) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
}

void D3D12Renderer::DrawIndexed(RHI::CommandList *commandList, uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
}

void D3D12Renderer::DrawInstanced(RHI::CommandList *commandList, uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
}

void D3D12Renderer::DrawIndexedInstanced(RHI::CommandList *commandList, uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void D3D12Renderer::DrawInstancedIndirect(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->ExecuteIndirect(drawInstancedIndirectCommandSignature, 1, static_cast<const D3D12Buffer *>(argsBuffer)->GetResource(), argsOffset, nullptr, 0);
}

void D3D12Renderer::DrawInstancedIndirectCount(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset, const RHI::Buffer *countBuffer, uint32_t countOffset, uint32_t maxCount) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->ExecuteIndirect(drawInstancedIndirectCommandSignature, maxCount, static_cast<const D3D12Buffer *>(argsBuffer)->GetResource(), argsOffset, static_cast<const D3D12Buffer *>(countBuffer)->GetResource(), countOffset);
}

void D3D12Renderer::DrawIndexedInstancedIndirect(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->ExecuteIndirect(drawIndexedInstancedIndirectCommandSignature, 1, static_cast<const D3D12Buffer *>(argsBuffer)->GetResource(), argsOffset, nullptr, 0);
}

void D3D12Renderer::DrawIndexedInstancedIndirectCount(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset, const RHI::Buffer *countBuffer, uint32_t countOffset, uint32_t maxCount) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->ExecuteIndirect(drawIndexedInstancedIndirectCommandSignature, maxCount, static_cast<const D3D12Buffer *>(argsBuffer)->GetResource(), argsOffset, static_cast<const D3D12Buffer *>(countBuffer)->GetResource(), countOffset);
}

void D3D12Renderer::Dispatch(RHI::CommandList *commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void D3D12Renderer::DispatchIndirect(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->ExecuteIndirect(dispatchIndirectCommandSignature, 1, static_cast<const D3D12Buffer *>(argsBuffer)->GetResource(), argsOffset, nullptr, 0);
}

void D3D12Renderer::DispatchMesh(RHI::CommandList *commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void D3D12Renderer::DispatchMeshIndirect(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset) {
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(_commandList);
    _commandList->GetGraphicsCommandList()->ExecuteIndirect(dispatchMeshIndirectCommandSignature, 1, static_cast<const D3D12Buffer *>(argsBuffer)->GetResource(), argsOffset, nullptr, 0);
}

void D3D12Renderer::SetMarker(RHI::CommandList *commandList, const char *string, uint8_t colorIndex) {
#if defined(_DEBUG) || defined(_DEVELOPMENT)
    if (commandList) {
        PIXSetMarker(static_cast<D3D12CommandList *>(commandList)->GetGraphicsCommandList(), PIX_COLOR_INDEX(colorIndex), string);
    } else {
        PIXSetMarker(PIX_COLOR_INDEX(colorIndex), string);
    }
#endif
}

void D3D12Renderer::BeginEvent(RHI::CommandList *commandList, const char *string, uint8_t colorIndex) {
#if defined(_DEBUG) || defined(_DEVELOPMENT)
    if (commandList) {
        PIXBeginEvent(static_cast<D3D12CommandList *>(commandList)->GetGraphicsCommandList(), PIX_COLOR_INDEX(colorIndex), string);
    } else {
        PIXBeginEvent(PIX_COLOR_INDEX(colorIndex), string);
    }
#endif
}

void D3D12Renderer::EndEvent(RHI::CommandList *commandList) {
#if defined(_DEBUG) || defined(_DEVELOPMENT)
    if (commandList) {
        PIXEndEvent(static_cast<D3D12CommandList *>(commandList)->GetGraphicsCommandList());
    } else {
        PIXEndEvent();
    }
#endif
}

DXGI_FORMAT D3D12Renderer::RHIFormatToDXGIFormat(RHI::Format rhiFormat) {
    switch (rhiFormat) {
    case RHI::Format::R32G32B32A32_FLOAT:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case RHI::Format::R32G32B32A32_UINT:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case RHI::Format::R32G32B32A32_SINT:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case RHI::Format::R32G32B32_FLOAT:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case RHI::Format::R32G32B32_UINT:
        return DXGI_FORMAT_R32G32B32_UINT;
    case RHI::Format::R32G32B32_SINT:
        return DXGI_FORMAT_R32G32B32_SINT;
    case RHI::Format::R16G16B16A16_FLOAT:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case RHI::Format::R16G16B16A16_UNORM:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
    case RHI::Format::R16G16B16A16_UINT:
        return DXGI_FORMAT_R16G16B16A16_UINT;
    case RHI::Format::R16G16B16A16_SNORM:
        return DXGI_FORMAT_R16G16B16A16_SNORM;
    case RHI::Format::R16G16B16A16_SINT:
        return DXGI_FORMAT_R16G16B16A16_SINT;
    case RHI::Format::R32G32_FLOAT:
        return DXGI_FORMAT_R32G32_FLOAT;
    case RHI::Format::R32G32_UINT:
        return DXGI_FORMAT_R32G32_UINT;
    case RHI::Format::R32G32_SINT:
        return DXGI_FORMAT_R32G32_SINT;
    case RHI::Format::D32_FLOAT_S8X24_UINT:
        return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    case RHI::Format::R32_FLOAT_X8X24_TYPELESS:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case RHI::Format::X32_TYPELESS_G8X24_UINT:
        return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
    case RHI::Format::R10G10B10A2_UNORM:
        return DXGI_FORMAT_R10G10B10A2_UNORM;
    case RHI::Format::R10G10B10A2_UINT:
        return DXGI_FORMAT_R10G10B10A2_UINT;
    case RHI::Format::R11G11B10_FLOAT:
        return DXGI_FORMAT_R11G11B10_FLOAT;
    case RHI::Format::R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case RHI::Format::R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case RHI::Format::R8G8B8A8_UINT:
        return DXGI_FORMAT_R8G8B8A8_UINT;
    case RHI::Format::R8G8B8A8_SNORM:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
    case RHI::Format::R8G8B8A8_SINT:
        return DXGI_FORMAT_R8G8B8A8_SINT;
    case RHI::Format::R16G16_FLOAT:
        return DXGI_FORMAT_R16G16_FLOAT;
    case RHI::Format::R16G16_UNORM:
        return DXGI_FORMAT_R16G16_UNORM;
    case RHI::Format::R16G16_UINT:
        return DXGI_FORMAT_R16G16_UINT;
    case RHI::Format::R16G16_SNORM:
        return DXGI_FORMAT_R16G16_SNORM;
    case RHI::Format::R16G16_SINT:
        return DXGI_FORMAT_R16G16_SINT;
    case RHI::Format::R32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;
    case RHI::Format::D32_FLOAT:
        return DXGI_FORMAT_D32_FLOAT;
    case RHI::Format::R32_UINT:
        return DXGI_FORMAT_R32_UINT;
    case RHI::Format::R32_SINT:
        return DXGI_FORMAT_R32_SINT;
    case RHI::Format::D24_UNORM_S8_UINT:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case RHI::Format::R24_UNORM_X8_TYPELESS:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case RHI::Format::X24_TYPELESS_G8_UINT:
        return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    case RHI::Format::R8G8_UNORM:
        return DXGI_FORMAT_R8G8_UNORM;
    case RHI::Format::R8G8_UINT:
        return DXGI_FORMAT_R8G8_UINT;
    case RHI::Format::R8G8_SNORM:
        return DXGI_FORMAT_R8G8_SNORM;
    case RHI::Format::R8G8_SINT:
        return DXGI_FORMAT_R8G8_SINT;
    case RHI::Format::R16_FLOAT:
        return DXGI_FORMAT_R16_FLOAT;
    case RHI::Format::D16_UNORM:
        return DXGI_FORMAT_D16_UNORM;
    case RHI::Format::R16_UNORM:
        return DXGI_FORMAT_R16_UNORM;
    case RHI::Format::R16_UINT:
        return DXGI_FORMAT_R16_UINT;
    case RHI::Format::R16_SNORM:
        return DXGI_FORMAT_R16_SNORM;
    case RHI::Format::R16_SINT:
        return DXGI_FORMAT_R16_SINT;
    case RHI::Format::R8_UNORM:
        return DXGI_FORMAT_R8_UNORM;
    case RHI::Format::R8_UINT:
        return DXGI_FORMAT_R8_UINT;
    case RHI::Format::R8_SNORM:
        return DXGI_FORMAT_R8_SNORM;
    case RHI::Format::R8_SINT:
        return DXGI_FORMAT_R8_SINT;
    case RHI::Format::A8_UNORM:
        return DXGI_FORMAT_A8_UNORM;
    case RHI::Format::R9G9B9E5_SHAREDEXP:
        return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
    case RHI::Format::BC1_UNORM:
        return DXGI_FORMAT_BC1_UNORM;
    case RHI::Format::BC1_UNORM_SRGB:
        return DXGI_FORMAT_BC1_UNORM_SRGB;
    case RHI::Format::BC2_UNORM:
        return DXGI_FORMAT_BC2_UNORM;
    case RHI::Format::BC2_UNORM_SRGB:
        return DXGI_FORMAT_BC2_UNORM_SRGB;
    case RHI::Format::BC3_UNORM:
        return DXGI_FORMAT_BC3_UNORM;
    case RHI::Format::BC3_UNORM_SRGB:
        return DXGI_FORMAT_BC3_UNORM_SRGB;
    case RHI::Format::BC4_UNORM:
        return DXGI_FORMAT_BC4_UNORM;
    case RHI::Format::BC4_SNORM:
        return DXGI_FORMAT_BC4_SNORM;
    case RHI::Format::BC5_UNORM:
        return DXGI_FORMAT_BC5_UNORM;
    case RHI::Format::BC5_SNORM:
        return DXGI_FORMAT_BC5_SNORM;
    case RHI::Format::B5G6R5_UNORM:
        return DXGI_FORMAT_B5G6R5_UNORM;
    case RHI::Format::B5G5R5A1_UNORM:
        return DXGI_FORMAT_B5G5R5A1_UNORM;
    case RHI::Format::B8G8R8A8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case RHI::Format::B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case RHI::Format::B8G8R8X8_UNORM:
        return DXGI_FORMAT_B8G8R8X8_UNORM;
    case RHI::Format::B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    case RHI::Format::B4G4R4A4_UNORM:
        return DXGI_FORMAT_B4G4R4A4_UNORM;
    case RHI::Format::A4B4G4R4_UNORM:
        return DXGI_FORMAT_A4B4G4R4_UNORM;
    case RHI::Format::BC6H_UF16:
        return DXGI_FORMAT_BC6H_UF16;
    case RHI::Format::BC6H_SF16:
        return DXGI_FORMAT_BC6H_SF16;
    case RHI::Format::BC7_UNORM:
        return DXGI_FORMAT_BC7_UNORM;
    case RHI::Format::BC7_UNORM_SRGB:
        return DXGI_FORMAT_BC7_UNORM_SRGB;
    }
    return DXGI_FORMAT_UNKNOWN;
}

RHI::Format D3D12Renderer::DXGIFormatToRHIFormat(DXGI_FORMAT dxgiFormat) {
    switch (dxgiFormat) {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return RHI::Format::R32G32B32A32_FLOAT;
    case DXGI_FORMAT_R32G32B32A32_UINT:
        return RHI::Format::R32G32B32A32_UINT;
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return RHI::Format::R32G32B32A32_SINT;
    case DXGI_FORMAT_R32G32B32_FLOAT:
        return RHI::Format::R32G32B32_FLOAT;
    case DXGI_FORMAT_R32G32B32_UINT:
        return RHI::Format::R32G32B32_UINT;
    case DXGI_FORMAT_R32G32B32_SINT:
        return RHI::Format::R32G32B32_SINT;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return RHI::Format::R16G16B16A16_FLOAT;
    case DXGI_FORMAT_R16G16B16A16_UNORM:
        return RHI::Format::R16G16B16A16_UNORM;
    case DXGI_FORMAT_R16G16B16A16_UINT:
        return RHI::Format::R16G16B16A16_UINT;
    case DXGI_FORMAT_R16G16B16A16_SNORM:
        return RHI::Format::R16G16B16A16_SNORM;
    case DXGI_FORMAT_R16G16B16A16_SINT:
        return RHI::Format::R16G16B16A16_SINT;
    case DXGI_FORMAT_R32G32_FLOAT:
        return RHI::Format::R32G32_FLOAT;
    case DXGI_FORMAT_R32G32_UINT:
        return RHI::Format::R32G32_UINT;
    case DXGI_FORMAT_R32G32_SINT:
        return RHI::Format::R32G32_SINT;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return RHI::Format::D32_FLOAT_S8X24_UINT;
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        return RHI::Format::R32_FLOAT_X8X24_TYPELESS;
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return RHI::Format::X32_TYPELESS_G8X24_UINT;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return RHI::Format::R10G10B10A2_UNORM;
    case DXGI_FORMAT_R10G10B10A2_UINT:
        return RHI::Format::R10G10B10A2_UINT;
    case DXGI_FORMAT_R11G11B10_FLOAT:
        return RHI::Format::R11G11B10_FLOAT;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return RHI::Format::R8G8B8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return RHI::Format::R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_R8G8B8A8_UINT:
        return RHI::Format::R8G8B8A8_UINT;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        return RHI::Format::R8G8B8A8_SNORM;
    case DXGI_FORMAT_R8G8B8A8_SINT:
        return RHI::Format::R8G8B8A8_SINT;
    case DXGI_FORMAT_R16G16_FLOAT:
        return RHI::Format::R16G16_FLOAT;
    case DXGI_FORMAT_R16G16_UNORM:
        return RHI::Format::R16G16_UNORM;
    case DXGI_FORMAT_R16G16_UINT:
        return RHI::Format::R16G16_UINT;
    case DXGI_FORMAT_R16G16_SNORM:
        return RHI::Format::R16G16_SNORM;
    case DXGI_FORMAT_R16G16_SINT:
        return RHI::Format::R16G16_SINT;
    case DXGI_FORMAT_R32_FLOAT:
        return RHI::Format::R32_FLOAT;
    case DXGI_FORMAT_D32_FLOAT:
        return RHI::Format::D32_FLOAT;
    case DXGI_FORMAT_R32_UINT:
        return RHI::Format::R32_UINT;
    case DXGI_FORMAT_R32_SINT:
        return RHI::Format::R32_SINT;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return RHI::Format::D24_UNORM_S8_UINT;
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        return RHI::Format::R24_UNORM_X8_TYPELESS;
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return RHI::Format::X24_TYPELESS_G8_UINT;
    case DXGI_FORMAT_R8G8_UNORM:
        return RHI::Format::R8G8_UNORM;
    case DXGI_FORMAT_R8G8_UINT:
        return RHI::Format::R8G8_UINT;
    case DXGI_FORMAT_R8G8_SNORM:
        return RHI::Format::R8G8_SNORM;
    case DXGI_FORMAT_R8G8_SINT:
        return RHI::Format::R8G8_SINT;
    case DXGI_FORMAT_R16_FLOAT:
        return RHI::Format::R16_FLOAT;
    case DXGI_FORMAT_D16_UNORM:
        return RHI::Format::D16_UNORM;
    case DXGI_FORMAT_R16_UNORM:
        return RHI::Format::R16_UNORM;
    case DXGI_FORMAT_R16_UINT:
        return RHI::Format::R16_UINT;
    case DXGI_FORMAT_R16_SNORM:
        return RHI::Format::R16_SNORM;
    case DXGI_FORMAT_R16_SINT:
        return RHI::Format::R16_SINT;
    case DXGI_FORMAT_R8_UNORM:
        return RHI::Format::R8_UNORM;
    case DXGI_FORMAT_R8_UINT:
        return RHI::Format::R8_UINT;
    case DXGI_FORMAT_R8_SNORM:
        return RHI::Format::R8_SNORM;
    case DXGI_FORMAT_R8_SINT:
        return RHI::Format::R8_SINT;
    case DXGI_FORMAT_A8_UNORM:
        return RHI::Format::A8_UNORM;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        return RHI::Format::R9G9B9E5_SHAREDEXP;
    case DXGI_FORMAT_BC1_UNORM:
        return RHI::Format::BC1_UNORM;
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return RHI::Format::BC1_UNORM_SRGB;
    case DXGI_FORMAT_BC2_UNORM:
        return RHI::Format::BC2_UNORM;
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        return RHI::Format::BC2_UNORM_SRGB;
    case DXGI_FORMAT_BC3_UNORM:
        return RHI::Format::BC3_UNORM;
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        return RHI::Format::BC3_UNORM_SRGB;
    case DXGI_FORMAT_BC4_UNORM:
        return RHI::Format::BC4_UNORM;
    case DXGI_FORMAT_BC4_SNORM:
        return RHI::Format::BC4_SNORM;
    case DXGI_FORMAT_BC5_UNORM:
        return RHI::Format::BC5_UNORM;
    case DXGI_FORMAT_BC5_SNORM:
        return RHI::Format::BC5_SNORM;
    case DXGI_FORMAT_B5G6R5_UNORM:
        return RHI::Format::B5G6R5_UNORM;
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        return RHI::Format::B5G5R5A1_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return RHI::Format::B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return RHI::Format::B8G8R8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return RHI::Format::B8G8R8X8_UNORM;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return RHI::Format::B8G8R8X8_UNORM_SRGB;
    case DXGI_FORMAT_BC6H_UF16:
        return RHI::Format::BC6H_UF16;
    case DXGI_FORMAT_BC6H_SF16:
        return RHI::Format::BC6H_SF16;
    case DXGI_FORMAT_BC7_UNORM:
        return RHI::Format::BC7_UNORM;
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return RHI::Format::BC7_UNORM_SRGB;
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return RHI::Format::B4G4R4A4_UNORM;
    case DXGI_FORMAT_A4B4G4R4_UNORM:
        return RHI::Format::A4B4G4R4_UNORM;
    }
    return RHI::Format::Unknown;
}

bool D3D12Renderer::ImageFormatToDXGIFormat(BE1::Image::Format imageFormat, bool isSRGB, DXGI_FORMAT *outDxgiFormat) {
    RHI::Format rhiFormat = RHI::Format::Unknown;
    if (!ImageFormatToRHIFormat(imageFormat, isSRGB, &rhiFormat)) {
        return false;
    }
    DXGI_FORMAT dxgiFormat = RHIFormatToDXGIFormat(rhiFormat);
    if (outDxgiFormat) {
        *outDxgiFormat = dxgiFormat;
    }
    return dxgiFormat != DXGI_FORMAT_UNKNOWN;
}

bool D3D12Renderer::DXGIFormatToImageFormat(DXGI_FORMAT dxgiFormat, BE1::Image::Format *imageFormat, bool *isSRGB) {
    RHI::Format rhiFormat = DXGIFormatToRHIFormat(dxgiFormat);
    return RHIFormatToImageFormat(rhiFormat, imageFormat, isSRGB);
}

bool D3D12Renderer::IsDepthFormat(DXGI_FORMAT format) {
    if (format == DXGI_FORMAT_D16_UNORM ||
        format == DXGI_FORMAT_D32_FLOAT ||
        format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
        format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
        return true;
    }
    return false;
}

bool D3D12Renderer::IsStencilFormat(DXGI_FORMAT format) {
    if (format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
        format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
        return true;
    }
    return false;
}

bool D3D12Renderer::IsDepthStencilFormat(DXGI_FORMAT format) {
    return IsDepthFormat(format) && IsStencilFormat(format);
}

bool D3D12Renderer::IsTypelessFormat(DXGI_FORMAT format) {
    if (format == DXGI_FORMAT_R32G32B32A32_TYPELESS ||
        format == DXGI_FORMAT_R32G32B32_TYPELESS ||
        format == DXGI_FORMAT_R16G16B16A16_TYPELESS ||
        format == DXGI_FORMAT_R32G32_TYPELESS ||
        format == DXGI_FORMAT_R32G8X24_TYPELESS ||
        format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT ||
        format == DXGI_FORMAT_R10G10B10A2_TYPELESS ||
        format == DXGI_FORMAT_R8G8B8A8_TYPELESS ||
        format == DXGI_FORMAT_R16G16_TYPELESS ||
        format == DXGI_FORMAT_R32_TYPELESS ||
        format == DXGI_FORMAT_R24G8_TYPELESS ||
        format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS ||
        format == DXGI_FORMAT_X24_TYPELESS_G8_UINT ||
        format == DXGI_FORMAT_R8G8_TYPELESS ||
        format == DXGI_FORMAT_R16_TYPELESS ||
        format == DXGI_FORMAT_R8_TYPELESS ||
        format == DXGI_FORMAT_BC1_TYPELESS ||
        format == DXGI_FORMAT_BC2_TYPELESS ||
        format == DXGI_FORMAT_BC3_TYPELESS ||
        format == DXGI_FORMAT_BC4_TYPELESS ||
        format == DXGI_FORMAT_BC5_TYPELESS ||
        format == DXGI_FORMAT_B8G8R8A8_TYPELESS ||
        format == DXGI_FORMAT_B8G8R8X8_TYPELESS ||
        format == DXGI_FORMAT_BC6H_TYPELESS ||
        format == DXGI_FORMAT_BC7_TYPELESS) {
        return true;
    }
    return false;
}

bool D3D12Renderer::IsTypelessCompatibleFormat(DXGI_FORMAT typelessFormat, DXGI_FORMAT viewFormat) {
    switch (typelessFormat) {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        if (viewFormat == DXGI_FORMAT_R32G32B32A32_FLOAT ||
            viewFormat == DXGI_FORMAT_R32G32B32A32_UINT ||
            viewFormat == DXGI_FORMAT_R32G32B32A32_SINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
        if (viewFormat == DXGI_FORMAT_R32G32B32_FLOAT ||
            viewFormat == DXGI_FORMAT_R32G32B32_UINT ||
            viewFormat == DXGI_FORMAT_R32G32B32_SINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        if (viewFormat == DXGI_FORMAT_R16G16B16A16_FLOAT ||
            viewFormat == DXGI_FORMAT_R16G16B16A16_UNORM ||
            viewFormat == DXGI_FORMAT_R16G16B16A16_UINT ||
            viewFormat == DXGI_FORMAT_R16G16B16A16_SNORM ||
            viewFormat == DXGI_FORMAT_R16G16B16A16_SINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_R32G32_TYPELESS:
        if (viewFormat == DXGI_FORMAT_R32G32_FLOAT ||
            viewFormat == DXGI_FORMAT_R32G32_UINT ||
            viewFormat == DXGI_FORMAT_R32G32_SINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
        if (viewFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT || // DSV
            viewFormat == DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS || // SRV for depth
            viewFormat == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT) { // SRV for stencil
            return true;
        }
        break;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        if (viewFormat == DXGI_FORMAT_R10G10B10A2_UNORM ||
            viewFormat == DXGI_FORMAT_R10G10B10A2_UINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        if (viewFormat == DXGI_FORMAT_R8G8B8A8_UNORM ||
            viewFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ||
            viewFormat == DXGI_FORMAT_R8G8B8A8_UINT ||
            viewFormat == DXGI_FORMAT_R8G8B8A8_SNORM ||
            viewFormat == DXGI_FORMAT_R8G8B8A8_SINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_R16G16_TYPELESS:
        if (viewFormat == DXGI_FORMAT_R16G16_FLOAT ||
            viewFormat == DXGI_FORMAT_R16G16_UNORM ||
            viewFormat == DXGI_FORMAT_R16G16_UINT ||
            viewFormat == DXGI_FORMAT_R16G16_SNORM ||
            viewFormat == DXGI_FORMAT_R16G16_SINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_R32_TYPELESS:
        if (viewFormat == DXGI_FORMAT_D32_FLOAT ||
            viewFormat == DXGI_FORMAT_R32_FLOAT ||
            viewFormat == DXGI_FORMAT_R32_UINT ||
            viewFormat == DXGI_FORMAT_R32_SINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_R24G8_TYPELESS:
        if (viewFormat == DXGI_FORMAT_D24_UNORM_S8_UINT || // DSV
            viewFormat == DXGI_FORMAT_R24_UNORM_X8_TYPELESS || // SRV for depth
            viewFormat == DXGI_FORMAT_X24_TYPELESS_G8_UINT) { // SRV for stencil
            return true;
        }
        break;
    case DXGI_FORMAT_R8G8_TYPELESS:
        if (viewFormat == DXGI_FORMAT_R8G8_UNORM ||
            viewFormat == DXGI_FORMAT_R8G8_UINT ||
            viewFormat == DXGI_FORMAT_R8G8_SNORM ||
            viewFormat == DXGI_FORMAT_R8G8_SINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_R16_TYPELESS:
        if (viewFormat == DXGI_FORMAT_R16_FLOAT ||
            viewFormat == DXGI_FORMAT_D16_UNORM ||
            viewFormat == DXGI_FORMAT_R16_UNORM ||
            viewFormat == DXGI_FORMAT_R16_UINT ||
            viewFormat == DXGI_FORMAT_R16_SNORM ||
            viewFormat == DXGI_FORMAT_R16_SINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_R8_TYPELESS:
        if (viewFormat == DXGI_FORMAT_R8_UNORM ||
            viewFormat == DXGI_FORMAT_R8_UINT ||
            viewFormat == DXGI_FORMAT_R8_SNORM ||
            viewFormat == DXGI_FORMAT_R8_SINT) {
            return true;
        }
        break;
    case DXGI_FORMAT_BC1_TYPELESS:
        if (viewFormat == DXGI_FORMAT_BC1_UNORM ||
            viewFormat == DXGI_FORMAT_BC1_UNORM_SRGB) {
            return true;
        }
        break;
    case DXGI_FORMAT_BC2_TYPELESS:
        if (viewFormat == DXGI_FORMAT_BC2_UNORM ||
            viewFormat == DXGI_FORMAT_BC2_UNORM_SRGB) {
            return true;
        }
        break;
    case DXGI_FORMAT_BC3_TYPELESS:
        if (viewFormat == DXGI_FORMAT_BC3_UNORM ||
            viewFormat == DXGI_FORMAT_BC3_UNORM_SRGB) {
            return true;
        }
        break;
    case DXGI_FORMAT_BC4_TYPELESS:
        if (viewFormat == DXGI_FORMAT_BC4_UNORM ||
            viewFormat == DXGI_FORMAT_BC4_SNORM) {
            return true;
        }
        break;
    case DXGI_FORMAT_BC5_TYPELESS:
        if (viewFormat == DXGI_FORMAT_BC5_UNORM ||
            viewFormat == DXGI_FORMAT_BC5_SNORM) {
            return true;
        }
        break;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        if (viewFormat == DXGI_FORMAT_B8G8R8A8_UNORM ||
            viewFormat == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) {
            return true;
        }
        break;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        if (viewFormat == DXGI_FORMAT_B8G8R8X8_UNORM ||
            viewFormat == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB) {
            return true;
        }
        break;
    case DXGI_FORMAT_BC6H_TYPELESS:
        if (viewFormat == DXGI_FORMAT_BC6H_UF16 ||
            viewFormat == DXGI_FORMAT_BC6H_SF16) {
            return true;
        }
        break;
    case DXGI_FORMAT_BC7_TYPELESS:
        if (viewFormat == DXGI_FORMAT_BC7_UNORM ||
            viewFormat == DXGI_FORMAT_BC7_UNORM_SRGB) {
            return true;
        }
        break;
    }
    return false;
}

bool D3D12Renderer::IsHDRSwapChainFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return true;
    }
    return false;
}

bool D3D12Renderer::IsSRGBFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return true;
    }
    return false;
}

DXGI_FORMAT D3D12Renderer::ToTypelessFormat(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return DXGI_FORMAT_R32G32B32A32_TYPELESS;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return DXGI_FORMAT_R32G32B32_TYPELESS;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
        return DXGI_FORMAT_R32G32_TYPELESS;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: // DSV
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS: // SRV for depth
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT: // SRV for stencil
        return DXGI_FORMAT_R32G8X24_TYPELESS;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
        return DXGI_FORMAT_R10G10B10A2_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
        return DXGI_FORMAT_R16G16_TYPELESS;
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
        return DXGI_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT: // DSV
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS: // SRV for depth
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT: // SRV for stencil
        return DXGI_FORMAT_R24G8_TYPELESS;
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
        return DXGI_FORMAT_R8G8_TYPELESS;
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
        return DXGI_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
        return DXGI_FORMAT_R8_TYPELESS;
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return DXGI_FORMAT_BC1_TYPELESS;
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        return DXGI_FORMAT_BC2_TYPELESS;
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        return DXGI_FORMAT_BC3_TYPELESS;
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return DXGI_FORMAT_BC4_TYPELESS;
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
        return DXGI_FORMAT_BC5_TYPELESS;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8X8_TYPELESS;
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
        return DXGI_FORMAT_BC6H_TYPELESS;
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return DXGI_FORMAT_BC7_TYPELESS;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT D3D12Renderer::ToLinearFormat(DXGI_FORMAT srgbFormat) {
    switch (srgbFormat) {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8X8_UNORM;
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        return DXGI_FORMAT_BC1_UNORM;
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        return DXGI_FORMAT_BC2_UNORM;
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        return DXGI_FORMAT_BC3_UNORM;
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return DXGI_FORMAT_BC7_UNORM;
    }
    return srgbFormat;
}

DXGI_FORMAT D3D12Renderer::ToSRGBFormat(DXGI_FORMAT linearFormat) {
    switch (linearFormat) {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
        return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    case DXGI_FORMAT_BC1_UNORM:
        return DXGI_FORMAT_BC1_UNORM_SRGB;
    case DXGI_FORMAT_BC2_UNORM:
        return DXGI_FORMAT_BC2_UNORM_SRGB;
    case DXGI_FORMAT_BC3_UNORM:
        return DXGI_FORMAT_BC3_UNORM_SRGB;
    case DXGI_FORMAT_BC7_UNORM:
        return DXGI_FORMAT_BC7_UNORM_SRGB;
    }
    return linearFormat;
}

UINT D3D12Renderer::GetComponentSwizzling(BE1::Image::Format format) {
    switch (format) {
    case BE1::Image::Format::L8:
    case BE1::Image::Format::L16_FLOAT:
    case BE1::Image::Format::L32_FLOAT:
        return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1);
    case BE1::Image::Format::A8:
    case BE1::Image::Format::A16_FLOAT:
    case BE1::Image::Format::A32_FLOAT:
        return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3);
    case BE1::Image::Format::L8A8:
    case BE1::Image::Format::L16A16_FLOAT:
    case BE1::Image::Format::L32A32_FLOAT:
    case BE1::Image::Format::BC4:
    case BE1::Image::Format::BC5:
        return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3);
    }
    return D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
}

BE1::Image::Format D3D12Renderer::ToUncompressedImageFormat(BE1::Image::Format inFormat) const {
    BE1::Image::Format outFormat;

    switch (inFormat) {
    case BE1::Image::Format::R5G6B5:
    case BE1::Image::Format::R8G8B8:
    case BE1::Image::Format::B5G6R5:
    case BE1::Image::Format::B8G8R8:
    case BE1::Image::Format::R4G4B4X4:
    case BE1::Image::Format::R5G5B5X1:
    case BE1::Image::Format::R8G8B8X8:
    case BE1::Image::Format::B4G4R4X4:
    case BE1::Image::Format::B5G5R5X1:
        outFormat = BE1::Image::Format::B8G8R8X8;
        break;
    case BE1::Image::Format::L8A8:
    case BE1::Image::Format::R4G4B4A4:
    case BE1::Image::Format::R8G8B8A8:
    case BE1::Image::Format::R5G5B5A1:
    case BE1::Image::Format::B4G4R4A4:
    case BE1::Image::Format::B5G5R5A1:
    case BE1::Image::Format::A4B4G4R4:
    case BE1::Image::Format::A1B5G5R5:
    case BE1::Image::Format::A8B8G8R8:
    case BE1::Image::Format::A4R4G4B4:
    case BE1::Image::Format::A1R5G5B5:
    case BE1::Image::Format::A8R8G8B8:
    case BE1::Image::Format::R10G10B10A2:
        outFormat = BE1::Image::Format::R8G8B8A8;
        break;
    case BE1::Image::Format::BC4:
    case BE1::Image::Format::BC5:
    case BE1::Image::Format::PVRTC12:
    case BE1::Image::Format::PVRTC14:
    case BE1::Image::Format::ETC1:
    case BE1::Image::Format::ETC2:
    case BE1::Image::Format::ATC:
        outFormat = BE1::Image::Format::B8G8R8X8;
        break;
    case BE1::Image::Format::BC1:
    case BE1::Image::Format::BC2:
    case BE1::Image::Format::BC3:
    case BE1::Image::Format::BC3XGBR:
    case BE1::Image::Format::BC7:
    case BE1::Image::Format::PVRTC12A:
    case BE1::Image::Format::PVRTC14A:
    case BE1::Image::Format::PVRTC22A:
    case BE1::Image::Format::PVRTC24A:
    case BE1::Image::Format::ETC2A1:
    case BE1::Image::Format::ETC2A:
    case BE1::Image::Format::ATCE:
    case BE1::Image::Format::ATCI:
        outFormat = BE1::Image::Format::R8G8B8A8;
        break;
    case BE1::Image::Format::EACR11:
    case BE1::Image::Format::EACR11_SNORM:
        outFormat = BE1::Image::Format::R16_FLOAT;
        break;
    case BE1::Image::Format::EACRG11:
    case BE1::Image::Format::EACRG11_SNORM:
        outFormat = BE1::Image::Format::R16G16_FLOAT;
        break;
    case BE1::Image::Format::BC6H_UF16:
    case BE1::Image::Format::BC6H_SF16:
    case BE1::Image::Format::R9G9B9E5_FLOAT:
    case BE1::Image::Format::R11G11B10_FLOAT:
        outFormat = BE1::Image::Format::R16G16B16A16_FLOAT;
        break;
    default:
        outFormat = inFormat;
        break;
    }
    return outFormat;
}

BE1::Image::Format D3D12Renderer::ToCompressedImageFormat(BE1::Image::Format inFormat, bool useNormalMap) const {
    if (BE1::Image::IsCompressed(inFormat)) {
        assert(0);
        return inFormat;
    }

    int redBits, greenBits, blueBits, alphaBits;
    BE1::Image::GetBits(inFormat, &redBits, &greenBits, &blueBits, &alphaBits);

    const bool hasRed = redBits > 0;
    const bool hasGreen = greenBits > 0;
    const bool hasBlue = blueBits > 0;
    const bool hasAlpha = alphaBits > 0;

    const bool isFloatFormat = BE1::Image::IsFloatFormat(inFormat) || BE1::Image::IsHalfFormat(inFormat);
    if (isFloatFormat) {
        if (hasRed && hasGreen && hasBlue && !hasAlpha) {
            return BE1::Image::Format::R11G11B10_FLOAT;
        }
        return inFormat;
    }

    const bool isLowPrecisionUNorm = !BE1::Image::NeedFloatConversion(inFormat) && redBits <= 8 && greenBits <= 8 && blueBits <= 8 && alphaBits <= 8;
    if (!isLowPrecisionUNorm) {
        return inFormat;
    }

    if (hasRed && !hasGreen && !hasBlue && !hasAlpha) {
        return BE1::Image::Format::BC4;
    }

    if (useNormalMap) {
        if (hasRed && hasGreen && !hasAlpha) {
            return BE1::Image::Format::BC5;
        }
        return inFormat;
    }

    if (hasRed && hasGreen && hasBlue) {
        if (alphaBits <= 1) {
            return BE1::Image::Format::BC1;
        }
        if (alphaBits <= 4) {
            return BE1::Image::Format::BC2;
        }
        return BE1::Image::Format::BC3;
    }

    return inFormat;
}

BE1::Image::Format D3D12Renderer::ToSupportedImageFormat(BE1::Image::Format inFormat) const {
    BE1::Image::Format outFormat;

    switch (inFormat) {
    case BE1::Image::Format::R16G16B16_FLOAT:
        outFormat = BE1::Image::Format::R16G16B16A16_FLOAT;
        break;
    case BE1::Image::Format::R16G16B16:
        outFormat = BE1::Image::Format::R16G16B16A16;
        break;
    case BE1::Image::Format::R16G16B16_SNORM:
        outFormat = BE1::Image::Format::R16G16B16A16_SNORM;
        break;
    case BE1::Image::Format::R16G16B16_SINT:
        outFormat = BE1::Image::Format::R16G16B16A16_SINT;
        break;
    case BE1::Image::Format::R16G16B16_UINT:
        outFormat = BE1::Image::Format::R16G16B16A16_UINT;
        break;
    case BE1::Image::Format::R8G8B8_SNORM:
        outFormat = BE1::Image::Format::R8G8B8A8_SNORM;
        break;
    case BE1::Image::Format::R8G8B8_SINT:
        outFormat = BE1::Image::Format::R8G8B8A8_SINT;
        break;
    case BE1::Image::Format::R8G8B8_UINT:
        outFormat = BE1::Image::Format::R8G8B8A8_UINT;
        break;
    default:
        outFormat = inFormat;
        break;
    }
    return outFormat;
}

BE1::Image::Format D3D12Renderer::ToUAVImageFormat(BE1::Image::Format format) const {
    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
    if (ImageFormatToDXGIFormat(format, false, &dxgiFormat) && IsSupportedUAVFormat(dxgiFormat)) {
        return format;
    }

    BE1::Image::Format candidateFormat = format;

    if (BE1::Image::IsCompressed(candidateFormat) || BE1::Image::IsPacked(candidateFormat)) {
        candidateFormat = ToUncompressedImageFormat(candidateFormat);
    }

    switch (candidateFormat) {
    case BE1::Image::Format::R32G32_FLOAT:
    case BE1::Image::Format::R32G32B32_FLOAT:
        candidateFormat = BE1::Image::Format::R32G32B32A32_FLOAT;
        break;
    case BE1::Image::Format::R32G32_UINT:
    case BE1::Image::Format::R32G32B32_UINT:
        candidateFormat = BE1::Image::Format::R32G32B32A32_UINT;
        break;
    case BE1::Image::Format::R32G32_SINT:
    case BE1::Image::Format::R32G32B32_SINT:
        candidateFormat = BE1::Image::Format::R32G32B32A32_SINT;
        break;
    case BE1::Image::Format::R16:
        candidateFormat = BE1::Image::Format::R16_FLOAT;
        break;
    case BE1::Image::Format::R16G16_FLOAT:
    case BE1::Image::Format::R16G16B16_FLOAT:
        candidateFormat = BE1::Image::Format::R16G16B16A16_FLOAT;
        break;
    case BE1::Image::Format::R16G16_UINT:
    case BE1::Image::Format::R16G16B16_UINT:
        candidateFormat = BE1::Image::Format::R16G16B16A16_UINT;
        break;
    case BE1::Image::Format::R16G16_SINT:
    case BE1::Image::Format::R16G16B16_SINT:
        candidateFormat = BE1::Image::Format::R16G16B16A16_SINT;
        break;
    case BE1::Image::Format::A8:
    case BE1::Image::Format::R8G8:
    case BE1::Image::Format::R8G8B8:
    case BE1::Image::Format::B8G8R8:
    case BE1::Image::Format::R8G8B8X8:
    case BE1::Image::Format::B8G8R8X8:
    case BE1::Image::Format::B8G8R8A8:
    case BE1::Image::Format::A8B8G8R8:
    case BE1::Image::Format::A8R8G8B8:
    case BE1::Image::Format::R10G10B10A2:
        candidateFormat = BE1::Image::Format::R8G8B8A8;
        break;
    case BE1::Image::Format::R8G8_UINT:
    case BE1::Image::Format::R8G8B8_UINT:
    case BE1::Image::Format::R10G10B10A2_UINT:
        candidateFormat = BE1::Image::Format::R8G8B8A8_UINT;
        break;
    case BE1::Image::Format::R8G8_SINT:
    case BE1::Image::Format::R8G8B8_SINT:
        candidateFormat = BE1::Image::Format::R8G8B8A8_SINT;
        break;
    }

    dxgiFormat = DXGI_FORMAT_UNKNOWN;
    if (ImageFormatToDXGIFormat(candidateFormat, false, &dxgiFormat) && IsSupportedUAVFormat(dxgiFormat)) {
        return candidateFormat;
    }

    assert(0);
    return BE1::Image::Format::Unknown;
}

bool D3D12Renderer::CanGenerateMipmapsOnGPU(RHI::TextureType textureType, BE1::Image::Format imageFormat, int arraySize) const {
    switch (textureType) {
    case RHI::TextureType::Texture2D:
    case RHI::TextureType::Texture3D:
        break;
    case RHI::TextureType::TextureCube:
        if (arraySize != 6) {
            return false;
        }
        break;
    default:
        return false;
    }

    if (BE1::Image::IsCompressed(imageFormat) ||
        BE1::Image::IsPacked(imageFormat) ||
        BE1::Image::IsDepthFormat(imageFormat) ||
        BE1::Image::IsDepthStencilFormat(imageFormat)) {
        return false;
    }

    // 현재 D3D12 mip generation compute shader 는 Float4 / UNorm4 텍스쳐만 지원한다.
    if (BE1::Image::NumComponents(imageFormat) != 4) {
        return false;
    }

    if (!BE1::Image::IsFloatFormat(imageFormat) && BE1::Image::NeedFloatConversion(imageFormat)) {
        return false;
    }

    DXGI_FORMAT dxgiFormat = DXGI_FORMAT_UNKNOWN;
    return ImageFormatToDXGIFormat(imageFormat, false, &dxgiFormat) && IsSupportedUAVFormat(dxgiFormat);
}

bool D3D12Renderer::IsSupportedUAVFormat(DXGI_FORMAT format) const {
    // D3D11.0 부터 지원하는 포맷
    if (format == DXGI_FORMAT_R32_FLOAT ||
        format == DXGI_FORMAT_R32_UINT ||
        format == DXGI_FORMAT_R32_SINT) {
        return true;
    }

    if (supportsUAVFormatCommon) {
        // D3D11.3 부터 지원하는 포맷
        if (format == DXGI_FORMAT_R32G32B32A32_FLOAT ||
            format == DXGI_FORMAT_R32G32B32A32_UINT ||
            format == DXGI_FORMAT_R32G32B32A32_SINT ||
            format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
            format == DXGI_FORMAT_R16G16B16A16_UINT ||
            format == DXGI_FORMAT_R16G16B16A16_SINT ||
            format == DXGI_FORMAT_R8G8B8A8_UNORM ||
            format == DXGI_FORMAT_R8G8B8A8_UINT ||
            format == DXGI_FORMAT_R8G8B8A8_SINT ||
            format == DXGI_FORMAT_R16_FLOAT ||
            format == DXGI_FORMAT_R16_UINT ||
            format == DXGI_FORMAT_R16_SINT ||
            format == DXGI_FORMAT_R8_UNORM ||
            format == DXGI_FORMAT_R8_UINT ||
            format == DXGI_FORMAT_R8_SINT) {
            return true;
        }
    }

    // Optional UAV formats:
    // DXGI_FORMAT_R16G16B16A16_UNORM,
    // DXGI_FORMAT_R16G16B16A16_SNORM,
    // DXGI_FORMAT_R32G32_FLOAT,
    // DXGI_FORMAT_R32G32_UINT,
    // DXGI_FORMAT_R32G32_SINT,
    // DXGI_FORMAT_R10G10B10A2_UNORM,
    // DXGI_FORMAT_R10G10B10A2_UINT,
    // DXGI_FORMAT_R11G11B10_FLOAT,
    // DXGI_FORMAT_R8G8B8A8_SNORM,
    // DXGI_FORMAT_R16G16_FLOAT,
    // DXGI_FORMAT_R16G16_UNORM,
    // DXGI_FORMAT_R16G16_UINT,
    // DXGI_FORMAT_R16G16_SNORM,
    // DXGI_FORMAT_R16G16_SINT,
    // DXGI_FORMAT_R8G8_UNORM,
    // DXGI_FORMAT_R8G8_UINT,
    // DXGI_FORMAT_R8G8_SNORM,
    // DXGI_FORMAT_R8G8_SINT,
    // DXGI_FORMAT_R16_UNORM,
    // DXGI_FORMAT_R16_SNORM,
    // DXGI_FORMAT_R8_SNORM,
    // DXGI_FORMAT_A8_UNORM,
    // DXGI_FORMAT_B5G6R5_UNORM,
    // DXGI_FORMAT_B5G5R5A1_UNORM,
    // DXGI_FORMAT_B4G4R4A4_UNORM
    if (optionalUAVFormatMap.find(format) != optionalUAVFormatMap.end()) {
        return true;
    }

    return false;
}
