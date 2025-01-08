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
#include "D3D12SwapChain.h"
#include "D3D12CommandList.h"
#include "D3D12CommandListPool.h"
#include "D3D12RootDescriptorPool.h"
#include "D3D12DescriptorPool.h"
#include "D3D12Texture.h"
#include "../VisObject.h"

// D3D12.dll 이 D3D12Core.dll 을 찾기 위한 설정
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }
extern "C" { __declspec(dllexport) extern const char *D3D12SDKPath = u8"."; }

D3D12Renderer *     renderer;

void D3D12Renderer::Init(HWND hwnd) {
    RHIRenderer::Init(hwnd);

#if defined(USE_DEBUG_LAYER) && (defined(_DEBUG) || defined(_DEVELOPMENT))
    bool enableDebugLayer = true;
    bool withGpuValidation = true;
#else
    bool enableDebugLayer = false;
    bool withGpuValidation = false;
#endif

    HRESULT hr;

    if (enableDebugLayer) {
        // 디버그 레이어 활성화
        ID3D12Debug* debugController = nullptr;
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
        if (SUCCEEDED(hr)) {
            debugController->EnableDebugLayer();

            // GPU Validation 활성화
            if (withGpuValidation) {
                ID3D12Debug5* debugController5 = nullptr;
                if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))))
                {
                    debugController5->SetEnableGPUBasedValidation(TRUE);
                    debugController5->SetEnableSynchronizedCommandQueueValidation(TRUE);
                    debugController5->SetEnableAutoName(TRUE);
                    debugController5->Release();
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

    // Factory 생성
    hr = CreateDXGIFactory2(enableDebugLayer ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateDXGIFactory2 failed, ERROR: 0x%x", hr);
    }

    IDXGIFactory5 *dxgiFactory5 = nullptr;
    if (SUCCEEDED(dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory5)))) {
        // VRR (Variable Refresh Rate) 지원 여부 체크
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

    // 어댑터 정보 얻어오기
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

    BE_LOG("Adapter: %s\n", adapterName.c_str());
    BE_LOG("Dedicated VideoMem Size: %s\n", BE1::Str::FormatBytes(dedicatedVideoMemSize).c_str());
    BE_LOG("Dedicated SystemMem Size: %s\n", BE1::Str::FormatBytes(dedicatedSystemMemSize).c_str());
    BE_LOG("Shared SystemMem Size: %s\n", BE1::Str::FormatBytes(sharedSystemMemSize).c_str());

    if (enableDebugLayer) {
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

    adapter1->Release();

    // Init feature check (https://devblogs.microsoft.com/directx/introducing-a-new-api-for-checking-feature-support-in-direct3d-12/)
    CD3DX12FeatureSupport features;
    hr = features.Init(device);
    assert(SUCCEEDED(hr));

    resourceHeapTier = features.ResourceHeapTier();

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
    if (features.DepthBoundsTestSupported() == TRUE) {
        supportsDepthBoundsTest = true;
    }
    if (features.CastingFullyTypedFormatSupported() == TRUE) {
        // https://microsoft.github.io/DirectX-Specs/d3d/RelaxedCasting.html#casting-rules-for-rs2-drivers
        supportsCastingFullyTypedFormat = true;
    }

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

#ifdef USE_SECONDARY_COMMAND_LISTS
    uint32_t maxSecondaryCommandLists = 8;
#else
    uint32_t maxSecondaryCommandLists = 0;
#endif
    // 커맨드 리스트 풀 생성
    graphicsCommandListPool = new D3D12CommandListPool(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, 8, maxSecondaryCommandLists);

    // 리소스 생성 용 커맨드 리스트
    resourceCommandList = graphicsCommandListPool->Alloc();

    // CPU 디스크립터 풀 생성
    resCpuDescriptorPool = new D3D12DescriptorPool(device, D3D12DescriptorPool::Type::CBV_SRV_UAV, 1000000, false);
    rtvCpuDescriptorPool = new D3D12DescriptorPool(device, D3D12DescriptorPool::Type::RTV, 16, false);
    dsvCpuDescriptorPool = new D3D12DescriptorPool(device, D3D12DescriptorPool::Type::DSV, 16, false);
    samCpuDescriptorPool = new D3D12DescriptorPool(device, D3D12DescriptorPool::Type::Sampler, 2048, false);

    // UAV 의 경우에만 CPU & GPU 디스크립터가 필요하다.
    // ClearUnorderedAccessViewUint 함수에서 CPU (원본) 디스크립터와 GPU 디스크립터가 모두 필요하기 때문에..
    uavCpuDescriptorPool = new D3D12DescriptorPool(device, D3D12DescriptorPool::Type::CBV_SRV_UAV, 4096, false);
    uavGpuDescriptorPool = new D3D12DescriptorPool(device, D3D12DescriptorPool::Type::CBV_SRV_UAV, 4096, true);

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

    maxPendingResources = 1024;
    pendingResourceBuffer = new D3D12PendingResource[maxPendingResources];

#ifdef USE_RENDER_TASK
    // 렌더 태스크 스레드 개수는 물리코어 개수를 넘지 않는다.
    int numCores = BE1::PlatformSystem::NumCPUCores();
    int numTaskThreads = BE1::Min(numCores, MaxRenderTaskThreads);

    renderTaskManager.Start(numTaskThreads);

    BE_LOG("Rendering task threads (%i) started\n", numTaskThreads);
#endif

    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        frameData[frameIndex].Init();
    }

    // Fence 객체 생성
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateFence failed, ERROR: 0x%x", hr);
    }
    // Fence 초기값
    fenceValue = 0;

    // Fence 를 대기하기 위한 이벤트 객체 생성
    fenceEventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    currentFrameIndex = 0;
    frameData[currentFrameIndex].SetFenceValue(SignalFence(RHI::CommandQueueType::Graphics));

#ifdef USE_RENDER_THREAD
    InitRenderThread();
#endif

    // 윈도우 크기 얻기
    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT backBufferWidth = rc.right;
    UINT backBufferHeight = rc.bottom;

    // 스왑 체인 (백버퍼) 생성
    DXGI_FORMAT dxgiFormat;
    ImageFormatToDXGIFormat(BE1::Image::Format::RGBA_8_8_8_8, false, &dxgiFormat);
    swapChain = CreateSwapChain(hwnd, backBufferWidth, backBufferHeight, dxgiFormat);

    CreateMainRenderTextures(backBufferWidth, backBufferHeight);

    InitFullScreenTrianglePSO();

    renderObjects.Reserve(16384);
}

void D3D12Renderer::Shutdown() {
    RHIRenderer::Shutdown();

#ifdef USE_RENDER_THREAD
    ShutdownRenderThread();
#endif

#ifdef USE_RENDER_TASK
    renderTaskManager.Stop();
#endif

    Finish(RHI::CommandQueueType::Graphics);
    Finish(RHI::CommandQueueType::Compute);

    DestroyPSO(imagePSO);

    if (mainRTColorTexture) {
        DestroyTexture(mainRTColorTexture, true);
    }
    if (mainRTColorMSAATexture) {
        DestroyTexture(mainRTColorMSAATexture, true);
    }
    if (mainRTDepthTexture) {
        DestroyTexture(mainRTDepthTexture, true);
    }

    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        frameData[frameIndex].Shutdown();
    }

    FreePendingResources(true);
    SAFE_DELETE(pendingResourceBuffer);
    maxPendingResources = 0;

    graphicsPsoMap.DeleteContents(true);
    computePsoMap.DeleteContents(true);

    for (int i = 0; i < cachedPsoBlobMap.Count(); ++i) {
        auto *entry = cachedPsoBlobMap.GetByIndex(i);
        SAFE_RELEASE(entry->second);
    }

    DestroySwapChain(swapChain);

    SAFE_DELETE(resCpuDescriptorPool);
    SAFE_DELETE(uavCpuDescriptorPool);
    SAFE_DELETE(uavGpuDescriptorPool);
    SAFE_DELETE(rtvCpuDescriptorPool);
    SAFE_DELETE(dsvCpuDescriptorPool);
    SAFE_DELETE(samCpuDescriptorPool);
    SAFE_DELETE(graphicsCommandListPool);

    SAFE_RELEASE(dxcCompiler);
    SAFE_RELEASE(dxcUtils);
    SAFE_RELEASE(dxcLibrary);
    SAFE_RELEASE(commandQueues[to_int(RHI::CommandQueueType::Graphics)]);
    SAFE_RELEASE(commandQueues[to_int(RHI::CommandQueueType::Compute)]);
    SAFE_RELEASE(fence);
    SAFE_RELEASE(dxgiFactory);

#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(allocator);
#endif

    if (dxcompilerLibrary) {
        BE1::PlatformProcess::CloseLibrary(dxcompilerLibrary);
    }

    if (fenceEventHandle) {
        CloseHandle(fenceEventHandle);
        fenceEventHandle = nullptr;
    }

    ULONG refCount = device->Release();
    if (refCount > 0) {
        IDXGIDebug1 *pDebug = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug)))) {
            pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
            pDebug->Release();
        }
        BE1::PlatformSystem::DebugBreak();
    }
}

void D3D12Renderer::CreateMainRenderTextures(uint32_t width, uint32_t height) {
    BE1::Image colorImage;
    colorImage.InitFromMemory(width, height, 1, 1, 1, GetMainRTColorFormat(), BE1::Image::GammaSpace::Linear, nullptr, 0);
    mainRTColorTexture = CreateTexture(RHI::TextureType::Texture2D, RHI::ResourceFlag::RenderTarget | RHI::ResourceFlag::ShaderResource | RHI::ResourceFlag::UnorderedAccess,
        &colorImage, RHI::ClearValue::Color(0.0f, 0.0f, 1.0f, 0.0f), 1);

    if (GetMainRTSampleCount() > 1) {
        mainRTColorMSAATexture = CreateTexture(RHI::TextureType::Texture2D, RHI::ResourceFlag::RenderTarget | RHI::ResourceFlag::ShaderResource,
            &colorImage, RHI::ClearValue::Color(0.0f, 0.0f, 1.0f, 0.0f), GetMainRTSampleCount());
    }

    BE1::Image depthStencilImage;
    depthStencilImage.InitFromMemory(width, height, 1, 1, 1, GetMainRTDepthFormat(), BE1::Image::GammaSpace::Linear, nullptr, 0);
    mainRTDepthTexture = CreateTexture(RHI::TextureType::Texture2D, RHI::ResourceFlag::DepthStencil,
        &depthStencilImage, RHI::ClearValue::DepthStencil(1.0f, 0), GetMainRTSampleCount(), RHI::GPUResourceState::DepthWrite);
}

void D3D12Renderer::InitFullScreenTrianglePSO() {
    BE1::Image::Format::Enum imageFormat = BE1::Image::Format::Unknown;
    bool isSRGB = false;
    DXGI_FORMAT swapChainDxgiFormat = swapChain->GetDXGIFormat();
    DXGIFormatToImageFormat(swapChainDxgiFormat, &imageFormat, &isSRGB);

    RHI::RenderDest renderDest;
    renderDest.renderTargetCount = 1;
    renderDest.renderTargetFormats[0] = imageFormat;
    renderDest.renderTargetForematSRGBs[0] = isSRGB;

    RHI::Shader *vs = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Vertex, "Source/TestD3D12/Shaders/FullScreenTriangle.hlsl", "VSMain"));
    RHI::Shader *ps = static_cast<RHI::Shader *>(renderer->CreateShaderFromFile(RHI::ShaderModel::SM_6_0, RHI::ShaderStage::Fragment, "Source/TestD3D12/Shaders/FullScreenTriangle.hlsl", "PSMain"));

    if (vs && ps) {
        RHI::PipelineStateDesc psoDesc;
        psoDesc.vs = vs;
        psoDesc.ps = ps;
        psoDesc.rasterizerState = renderer->GetRasterizerState(RHI::RasterizerStateType::SolidFrontSided);
        psoDesc.depthStencilState = renderer->GetDepthStencilState(RHI::DepthStencilStateType::Never);
        psoDesc.blendState = renderer->GetBlendState(RHI::BlendStateType::Opaque);
        psoDesc.primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        psoDesc.renderDest = &renderDest;
        imagePSO = renderer->CreateGraphicsPSO(&psoDesc);
    }

    if (vs) {
        renderer->DestroyShader(vs, true);
    }
    if (ps) {
        renderer->DestroyShader(ps, true);
    }
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

void D3D12Renderer::BeginFrame() {
    PIX_SCOPED_EVENT(commandQueues[to_int(RHI::CommandQueueType::Graphics)], 0, "D3D12Renderer::BeginFrame");

    currentFrameData = &frameData[currentFrameIndex];

    // 프레임 데이터를 초기화하고, 이전 프레임에 대한 펜스를 기다린다.
    currentFrameData->BeginFrame();

    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    D3D12CommandList *commandList = currentFrameData->threadData[0].graphicsCommandListPool->Alloc();
    mainCommandList = commandList;

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    // 뷰포트 & ScissorRect 설정
    SetViewport(commandList, swapChain->viewportRect);
    SetScissorRect(commandList, swapChain->scissorRect);

#ifdef USE_SECONDARY_COMMAND_LISTS
    //BeginRenderPass(commandList, swapChain, mainRTDepthTexture, BE1::Color4::blue, 1.0f, 0, RHI::ClearFlag::Color | RHI::ClearFlag::Depth);

    if (GetMainRTSampleCount() > 1) {
        RHI::RenderPassImage renderPassImages[] = {
            RHI::RenderPassImage::Color(mainRTColorMSAATexture, 0, RHI::RenderPassImage::LoadAction::Clear),
            RHI::RenderPassImage::DepthStencil(mainRTDepthTexture, 0, RHI::RenderPassImage::LoadAction::Clear),
            RHI::RenderPassImage::ResolveColor(mainRTColorTexture, 0, 0)
        };
        BeginRenderPass(commandList, renderPassImages, COUNT_OF(renderPassImages));
    } else {
        RHI::RenderPassImage renderPassImages[] = {
            RHI::RenderPassImage::Color(mainRTColorTexture, 0, RHI::RenderPassImage::LoadAction::Clear),
            RHI::RenderPassImage::DepthStencil(mainRTDepthTexture, 0, RHI::RenderPassImage::LoadAction::Clear)
        };
        BeginRenderPass(commandList, renderPassImages, COUNT_OF(renderPassImages));
    }
#else
    // 백버퍼를 렌더 타겟 상태로 전환
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChain->GetCurrentBackBuffer();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->GetGraphicsCommandList()->ResourceBarrier(1, &barrier);

    commandList->GetGraphicsCommandList()->ClearRenderTargetView(swapChain->GetCurrentBackBufferRTVDescriptorHandle(), BE1::Color4::blue, 0, nullptr);
    commandList->GetGraphicsCommandList()->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    commandList->GetGraphicsCommandList()->OMSetRenderTargets(1, &swapChain->GetCurrentBackBufferRTVDescriptorHandle(), FALSE, &dsvDescriptorHandle);

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute(RHI::CommandQueueType::Graphics);
#endif
}

void D3D12Renderer::EndFrame() {
    PIX_SCOPED_EVENT(commandQueues[to_int(RHI::CommandQueueType::Graphics)], 1, "D3D12Renderer::EndFrame");

    // TODO: 렌더큐에 종료 마킹을 하고, 렌더큐를 실행한다.

#ifdef USE_SECONDARY_COMMAND_LISTS
    D3D12CommandList *commandList = mainCommandList;

    EndRenderPass(commandList);

    BeginRenderPass(commandList, swapChain, nullptr);
    SetPSO(commandList, imagePSO);
    SetTexture(commandList, 0, false, mainRTColorTexture);
    Draw(commandList, 3, 0);
    EndRenderPass(commandList);
#else
    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    D3D12CommandList *commandList = currentFrameData->threadData[0].graphicsCommandListPool->Alloc();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset(false);

    // 백버퍼 RTV 를 Present 할 수 있는 상태로 전환
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChain->GetCurrentBackBuffer();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->GetGraphicsCommandList()->ResourceBarrier(1, &barrier);
#endif

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute(RHI::CommandQueueType::Graphics);

    // 이번 프레임에서 수행하는 렌더링 커맨드들에 대한 펜스를 친다.
    currentFrameData->EndFrame();

    // 백버퍼를 전면버퍼와 교환한다.
    SwapChainBuffers(false);

    frameCount++;

    currentFrameIndex = frameCount % NumFrameResources;

    // 메인 스레드에서 사용할 수 있도록 이전 프레임에 할당했던 메모리를 초기화한다.
    frameData[currentFrameIndex].ClearMemAllocs();

    FreePendingResources();
}

void D3D12Renderer::SwapChainBuffers(bool vsync) {
    PIX_SCOPED_EVENT(commandQueues[to_int(RHI::CommandQueueType::Graphics)], 2, "D3D12Renderer::SwapChainBuffers");

    swapChain->SwapBuffers(vsync);
}

D3D12CommandList* D3D12Renderer::FlushCommandList(D3D12CommandList* commandList) {
    PIX_SCOPED_EVENT(commandQueues[to_int(RHI::CommandQueueType::Graphics)], 3, "D3D12Renderer::FlushCommandList");

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute(RHI::CommandQueueType::Graphics);

    // 커맨드 리스트 풀에서 새로운 커맨드 리스트를 얻어온다.
    commandList = commandList->parentPool->Alloc();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    // 뷰포트 & ScissorRect 설정
    SetViewport(commandList, swapChain->viewportRect);
    SetScissorRect(commandList, swapChain->scissorRect);

    commandList->GetGraphicsCommandList()->OMSetRenderTargets(1, &swapChain->GetCurrentBackBufferRTVDescriptorHandle(), FALSE, &static_cast<D3D12Texture *>(mainRTDepthTexture)->dsvDescriptors[0].cpuDescriptorHandle);

    return commandList;
}

uint64_t D3D12Renderer::SignalFence(RHI::CommandQueueType queueType) {
    fenceValue++;
    commandQueues[to_int(queueType)]->Signal(fence, fenceValue);

    return fenceValue;
}

bool D3D12Renderer::IsFenceComplete(uint64_t checkFenceValue) {
    return fence->GetCompletedValue() < checkFenceValue ? false : true;
}

void D3D12Renderer::WaitFence(uint64_t expectedFenceValue) {
    if (fence->GetCompletedValue() < expectedFenceValue) {
        fence->SetEventOnCompletion(expectedFenceValue, fenceEventHandle);
        WaitForSingleObject(fenceEventHandle, INFINITE);
    }
}

void D3D12Renderer::Finish(RHI::CommandQueueType queueType) {
    WaitFence(SignalFence(queueType));
}

void D3D12Renderer::WaitAllFrameFences() {
    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        WaitFence(frameData[frameIndex].GetFenceValue());
    }
}

void D3D12Renderer::MarkForDelete(RHI::GPUObject *object) {
    D3D12PendingResource *newPendingResource = &pendingResourceBuffer[headPendingIndex];
    newPendingResource->fenceValue = SignalFence(RHI::CommandQueueType::Graphics);
    newPendingResource->objectToDelete = object;

    OnPendingResourceAdded();
}

void D3D12Renderer::MarkForRelease(ID3D12Resource *resource) {
    D3D12PendingResource *newPendingResource = &pendingResourceBuffer[headPendingIndex];
    newPendingResource->fenceValue = SignalFence(RHI::CommandQueueType::Graphics);
    newPendingResource->resourceToRelease = resource;

    OnPendingResourceAdded();
}

void D3D12Renderer::OnPendingResourceAdded() {
    headPendingIndex = headPendingIndex + 1;

    // 버퍼가 꽉 찼다면, 가장 오래된 pending resource 를 기다린 후 Release 한다.
    if (headPendingIndex % maxPendingResources == tailPendingIndex) {
        D3D12PendingResource *oldestPendingResource = &pendingResourceBuffer[tailPendingIndex];

        WaitFence(oldestPendingResource->fenceValue);

        SAFE_RELEASE(oldestPendingResource->resourceToRelease);
        SAFE_DELETE(oldestPendingResource->objectToDelete)

        oldestPendingResource->fenceValue = 0;

        tailPendingIndex = (tailPendingIndex + 1) % maxPendingResources;
    }

    headPendingIndex = headPendingIndex % maxPendingResources;
}

void D3D12Renderer::FreePendingResources(bool waitPendings) {
    while (headPendingIndex != tailPendingIndex) {
        D3D12PendingResource *pendingResource = &pendingResourceBuffer[tailPendingIndex];
        if (waitPendings) {
            WaitFence(pendingResource->fenceValue);
        } else {
            if (!IsFenceComplete(pendingResource->fenceValue)) {
                continue;
            }
        }

        SAFE_RELEASE(pendingResource->resourceToRelease);
        SAFE_DELETE(pendingResource->objectToDelete)

        tailPendingIndex = (tailPendingIndex + 1) % maxPendingResources;
    }
}

void D3D12Renderer::DestroySwapChain(D3D12SwapChain *swapChain) {
    SAFE_DELETE(swapChain);
}

void D3D12Renderer::SetConstants(RHI::CommandList *commandList, const void *data, uint32_t size, uint32_t offset) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    int threadIndex = d3d12CommandList->GetThreadIndex();

    int rootParameterIndex = d3d12CommandList->currentPSO->binder.rootParameterBinder.constants;

    D3D12FrameData::DataPerThread &threadData = currentFrameData->threadData[threadIndex];
    memcpy(threadData.rootConstants + offset / sizeof(uint32_t), data, size);

    if (d3d12CommandList->GetCommandListType() == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
        d3d12CommandList->computeRootParametersDirtyMask |= BIT64(rootParameterIndex);
    } else {
        d3d12CommandList->graphicsRootParametersDirtyMask |= BIT64(rootParameterIndex);
    }
}

void D3D12Renderer::OnResize(int width, int height) {
#ifdef USE_RENDER_THREAD
    WaitRenderCompleted();
#endif

    Finish(RHI::CommandQueueType::Graphics);

    swapChain->Resize(width, height);

    if (mainRTColorTexture) {
        DestroyTexture(mainRTColorTexture, true);
    }
    if (mainRTColorMSAATexture) {
        DestroyTexture(mainRTColorMSAATexture, true);
    }
    if (mainRTDepthTexture) {
        DestroyTexture(mainRTDepthTexture, true);
    }

    CreateMainRenderTextures(width, height);
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

void D3D12Renderer::SetBlendFactor(RHI::CommandList *commandList, const BE1::Color4 &rgba) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->SetBlendFactor(rgba);
}

void D3D12Renderer::SetStencilRef(RHI::CommandList *commandList, uint32_t value) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->SetStencilRef(value);
}

void D3D12Renderer::SetShadingRate(RHI::CommandList *commandList, RHI::ShadingRate shadingRate) {
    if (!supportsVRS) {
        return;
    }
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->SetShadingRate(shadingRate);
}

void D3D12Renderer::SetViewport(RHI::CommandList *commandList, const BE1::Rect &viewportRect) {
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = viewportRect.x;
    viewport.TopLeftY = viewportRect.y;
    viewport.Width = viewportRect.w;
    viewport.Height = viewportRect.h;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->GetGraphicsCommandList()->RSSetViewports(1, &viewport);
}

void D3D12Renderer::SetScissorRect(RHI::CommandList *commandList, const BE1::Rect &scissorRect) {
    static_assert(sizeof(BE1::Rect) == sizeof(D3D12_RECT));
    static_assert(offsetof(BE1::Rect, x) == offsetof(D3D12_RECT, left));
    static_assert(offsetof(BE1::Rect, y) == offsetof(D3D12_RECT, top));
    static_assert(offsetof(BE1::Rect, w) == offsetof(D3D12_RECT, right));
    static_assert(offsetof(BE1::Rect, h) == offsetof(D3D12_RECT, bottom));

    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->GetGraphicsCommandList()->RSSetScissorRects(1, (D3D12_RECT *)&scissorRect);
}

void D3D12Renderer::SetDepthBounds(RHI::CommandList *commandList, float depthMin, float depthMax) {
    if (!supportsDepthBoundsTest) {
        return;
    }
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->GetGraphicsCommandList()->OMSetDepthBounds(depthMin, depthMax);
}

void D3D12Renderer::Dispatch(RHI::CommandList *commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(d3d12CommandList, false);
    d3d12CommandList->GetGraphicsCommandList()->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void D3D12Renderer::DispatchMesh(RHI::CommandList *commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(d3d12CommandList, false);
    d3d12CommandList->GetGraphicsCommandList()->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void D3D12Renderer::ClearUAV(RHI::CommandList *commandList, const RHI::GPUResource *resource, uint32_t value) {
    const UINT values[4] = { value, value, value, value };
    const BE1::Array<D3D12UAVDescriptor> *uavDescriptors = nullptr;

    const D3D12Buffer *buffer = reinterpret_cast<const D3D12Buffer *>(resource->GetNativeBufferObject());
    if (buffer) {
        uavDescriptors = &buffer->uavDescriptors;
    } else {
        const D3D12Texture *texture = reinterpret_cast<const D3D12Texture *>(resource->GetNativeTextureObject());
        if (texture) {
            uavDescriptors = &texture->uavDescriptors;
        }
    }

    if (!uavDescriptors) {
        BE_ERRLOG("D3D12Renderer::ClearUAV: Invalid UAV descriptors\n");
        return;
    }
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    for (int i = 0; i < uavDescriptors->Count(); ++i) {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = (*uavDescriptors)[i].cpuDescriptorHandle;
        int descriptorIndex = uavCpuDescriptorPool->GetIndexFromCPUDescriptorHandle(cpuDescriptorHandle);
        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle = uavGpuDescriptorPool->GetGPUDescriptorHandleFromIndex(descriptorIndex);

        d3d12CommandList->GetGraphicsCommandList()->ClearUnorderedAccessViewUint(gpuDescriptorHandle, cpuDescriptorHandle, reinterpret_cast<ID3D12Resource *>(resource->GetNativeResource()), values, 0, nullptr);
    }
}

void D3D12Renderer::CopyBuffer(RHI::CommandList *commandList, const RHI::Buffer *dstBuffer, uint32_t dstOffset, const RHI::Buffer *srcBuffer, uint32_t srcOffset, uint32_t size) {
    const D3D12Buffer *d3d12DstBuffer = static_cast<const D3D12Buffer *>(dstBuffer);
    const D3D12Buffer *d3d12SrcBuffer = static_cast<const D3D12Buffer *>(srcBuffer);

    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->GetGraphicsCommandList()->CopyBufferRegion(d3d12DstBuffer->GetResource(), dstOffset, d3d12SrcBuffer->GetResource(), srcOffset, size);
}

void D3D12Renderer::CopyTexture(RHI::CommandList *commandList, const RHI::Texture *dstTexture, uint32_t dstSlice, uint32_t dstMipLevel, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
    const RHI::Texture *srcTexture, uint32_t srcSlice, uint32_t srcMipLevel, uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t width, uint32_t height, uint32_t depth) {
    const D3D12Texture *d3d12DstTexture = static_cast<const D3D12Texture *>(dstTexture);
    const D3D12Texture *d3d12SrcTexture = static_cast<const D3D12Texture *>(srcTexture);

    CD3DX12_TEXTURE_COPY_LOCATION dstLocation(d3d12DstTexture->GetResource(), D3D12CalcSubresource(dstMipLevel, dstSlice, 0, d3d12DstTexture->textureDesc.MipLevels, d3d12DstTexture->textureDesc.DepthOrArraySize));
    CD3DX12_TEXTURE_COPY_LOCATION srcLocation(d3d12SrcTexture->GetResource(), D3D12CalcSubresource(srcMipLevel, srcSlice, 0, d3d12SrcTexture->textureDesc.MipLevels, d3d12SrcTexture->textureDesc.DepthOrArraySize));

    D3D12_BOX srcBox = { srcX, srcY, srcZ, (UINT)width, (UINT)height, (UINT)depth };
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->GetGraphicsCommandList()->CopyTextureRegion(&dstLocation, dstX, dstY, dstZ, &srcLocation, &srcBox);
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
    if (BE1::HasFlag(resourceState, RHI::GPUResourceState::VertexBuffer | RHI::GPUResourceState::ConstantBuffer)) {
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
    BE1::Array<D3D12_RESOURCE_BARRIER> barrierDescs;
    barrierDescs.Reserve(barrierCount);

    for (uint32_t barrierIndex = 0; barrierIndex < barrierCount; ++barrierIndex) {
        const RHI::GPUBarrier *barrier = &barriers[barrierIndex];
        D3D12_RESOURCE_BARRIER &barrierDesc = barrierDescs.Alloc();

        switch (barrier->type) {
        case RHI::GPUBarrier::Type::Memory:
            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            // UAV.pResource == nullptr 일 때는 특정 UAV 가 아닌 모든 UAV 에 대한 메모리 배리어가 된다.
            barrierDesc.UAV.pResource = !barrier->memoryBarrier.resource ? nullptr : reinterpret_cast<ID3D12Resource *>(barrier->memoryBarrier.resource->GetNativeResource());
            break;
        case RHI::GPUBarrier::Type::Buffer:
            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrierDesc.Transition.pResource = reinterpret_cast<ID3D12Resource *>(barrier->bufferBarrier.buffer->GetNativeResource());
            barrierDesc.Transition.StateBefore = ToD3D12ResourceState(barrier->bufferBarrier.stateBefore);
            barrierDesc.Transition.StateAfter = ToD3D12ResourceState(barrier->bufferBarrier.stateAfter);
            barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            break;
        case RHI::GPUBarrier::Type::Image:
            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrierDesc.Transition.pResource = reinterpret_cast<ID3D12Resource *>(barrier->imageBarrier.texture->GetNativeResource());
            barrierDesc.Transition.StateBefore = ToD3D12ResourceState(barrier->imageBarrier.stateBefore);
            barrierDesc.Transition.StateAfter = ToD3D12ResourceState(barrier->imageBarrier.stateAfter);

            if (barrier->imageBarrier.slice >= 0 || barrier->imageBarrier.mipLevel >= 0) {
                const D3D12Texture *d3d12Texture = static_cast<const D3D12Texture *>(barrier->imageBarrier.texture);
                barrierDesc.Transition.Subresource = D3D12CalcSubresource(barrier->imageBarrier.mipLevel, barrier->imageBarrier.slice, 0, d3d12Texture->textureDesc.MipLevels, d3d12Texture->textureDesc.DepthOrArraySize);
            } else {
                barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            }
            break;
        case RHI::GPUBarrier::Type::Aliasing:
            barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
            barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrierDesc.Aliasing.pResourceBefore = reinterpret_cast<ID3D12Resource *>(barrier->aliasingBarrier.resourceBefore->GetNativeResource());
            barrierDesc.Aliasing.pResourceAfter = reinterpret_cast<ID3D12Resource *>(barrier->aliasingBarrier.resourceAfter->GetNativeResource());
            break;
        }
    }

    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->GetGraphicsCommandList()->ResourceBarrier(barrierCount, barrierDescs.Ptr());
}

void D3D12Renderer::BeginRenderPass(RHI::CommandList *commandList, const RHI::SwapChain *swapChain, const RHI::Texture *depthStencilTexture, const BE1::Color4 &clearColor, float clearDepth, uint8_t clearStencil, RHI::ClearFlag clearFlags) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);

    if (swapChain) {
        // 백버퍼를 렌더 타겟 상태로 전환
        D3D12_RESOURCE_BARRIER barrier;
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = static_cast<const D3D12SwapChain *>(swapChain)->GetCurrentBackBuffer();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        d3d12CommandList->GetGraphicsCommandList()->ResourceBarrier(1, &barrier);

        // EndRenderPass 에서 사용할 Barrier 를 미리 등록
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        d3d12CommandList->endRenderPassBarriers.Append(barrier);
    }
#if 1
    // 렌더 타겟
    D3D12_RENDER_PASS_RENDER_TARGET_DESC rtDesc = {};
    D3D12_RENDER_PASS_RENDER_TARGET_DESC *rtDescPtr = nullptr;
    if (swapChain) {
        rtDesc.cpuDescriptor = static_cast<const D3D12SwapChain *>(swapChain)->GetCurrentBackBufferRTVDescriptorHandle();
        if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Color)) {
            rtDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            rtDesc.BeginningAccess.Clear.ClearValue.Color[0] = clearColor[0];
            rtDesc.BeginningAccess.Clear.ClearValue.Color[1] = clearColor[1];
            rtDesc.BeginningAccess.Clear.ClearValue.Color[2] = clearColor[2];
            rtDesc.BeginningAccess.Clear.ClearValue.Color[3] = clearColor[3];
        } else {
            rtDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
        }
        rtDesc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        rtDescPtr = &rtDesc;
    }

    // 뎁스/스텐실
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC dsDesc = {};
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC *dsDescPtr = nullptr;
    if (depthStencilTexture) {
        dsDesc.cpuDescriptor = static_cast<const D3D12Texture *>(depthStencilTexture)->dsvDescriptors[0].cpuDescriptorHandle;
        if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Depth)) {
            dsDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            dsDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = clearDepth;
        } else {
            dsDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
        }
        dsDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
        if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Stencil)) {
            dsDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
            dsDesc.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = clearStencil;
        } else {
            dsDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
        }
        dsDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
        dsDescPtr = &dsDesc;
    }

    d3d12CommandList->GetGraphicsCommandList()->BeginRenderPass(1, rtDescPtr, dsDescPtr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);
#else
    if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Color)) {
        d3d12CommandList->GetGraphicsCommandList()->ClearRenderTargetView(d3d12SwapChain->GetCurrentBackBufferDescriptorHandle(), clearColor, 0, nullptr);
    }
    if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Depth | RHI::ClearFlag::Stencil)) {
        D3D12_CLEAR_FLAGS clearDepthStencilFlags = 0;
        if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Depth)) {
            clearDepthStencilFlags |= D3D12_CLEAR_FLAG_DEPTH;
        }
        if (BE1::HasFlag(clearFlags, RHI::ClearFlag::Stencil)) {
            clearDepthStencilFlags |= D3D12_CLEAR_FLAG_STENCIL;
        }
        d3d12CommandList->GetGraphicsCommandList()->ClearDepthStencilView(dsvDescriptorHandle, clearDepthStencilFlags, clearDepth, clearStencil, 0, nullptr);
    }
    d3d12CommandList->GetGraphicsCommandList()->OMSetRenderTargets(1, &d3d12SwapChain->GetCurrentBackBufferDescriptorHandle(), FALSE, &dsvDescriptorHandle);
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
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);

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
        bool useEntireSubresources = true;
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
            const D3D12RTVDescriptor &rtvDescriptor = texture->rtvDescriptors[renderPassImage.subresourceIndex];
            useEntireSubresources = texture->rtvDescriptors.Count() == 1;
            D3D12_RENDER_PASS_RENDER_TARGET_DESC &rtDesc = rtDescs[rtCount];
            rtDesc.cpuDescriptor = rtvDescriptor.cpuDescriptorHandle;
            rtDesc.BeginningAccess.Type = beginningAccessType;
            rtDesc.BeginningAccess.Clear.ClearValue = texture->clearValue;
            rtDesc.EndingAccess.Type = endingAccessType;

            // 텍스쳐의 RTV 정보를 가져온다.
            GetInfoFromRTVDesc(rtvDescriptor.rtvDesc, destMipSlice, destFirstArraySlice, destArraySize);

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
            const D3D12DSVDescriptor &dsvDescriptor = texture->dsvDescriptors[renderPassImage.subresourceIndex];
            useEntireSubresources = texture->dsvDescriptors.Count() == 1;
            dsDesc.cpuDescriptor = dsvDescriptor.cpuDescriptorHandle;
            dsDesc.DepthBeginningAccess.Type = beginningAccessType;
            dsDesc.DepthBeginningAccess.Clear.ClearValue = texture->clearValue;
            dsDesc.DepthEndingAccess.Type = endingAccessType;

            if (IsStencilFormat(texture->textureDesc.Format)) {
                dsDesc.StencilBeginningAccess = dsDesc.DepthBeginningAccess;
                dsDesc.StencilEndingAccess = dsDesc.DepthEndingAccess;
            }

            // 텍스쳐의 DSV 정보를 가져온다.
            GetInfoFromDSVDesc(dsvDescriptor.dsvDesc, destMipSlice, destFirstArraySlice, destArraySize);

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
            const D3D12RTVDescriptor &rtvDescriptor = texture->rtvDescriptors[renderPassImage.subresourceIndex];
            useEntireSubresources = texture->rtvDescriptors.Count() == 1;
            const ResolveSource &resolveSource = rtResolveSources[renderPassImage.resolveSourceIndex];
            D3D12_RENDER_PASS_RENDER_TARGET_DESC &rtDesc = rtDescs[renderPassImage.resolveSourceIndex];
            rtDesc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
            rtDesc.EndingAccess.Resolve.Format = texture->textureDesc.Format;
            rtDesc.EndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE::D3D12_RESOLVE_MODE_AVERAGE;
            rtDesc.EndingAccess.Resolve.pSrcResource = resolveSource.resource;
            rtDesc.EndingAccess.Resolve.pDstResource = reinterpret_cast<ID3D12Resource *>(texture->GetNativeResource());
            rtDesc.EndingAccess.Resolve.PreserveResolveSource = resolveSource.preserve;

            // 텍스쳐의 RTV 정보를 가져온다.
            GetInfoFromRTVDesc(rtvDescriptor.rtvDesc, destMipSlice, destFirstArraySlice, destArraySize);

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
            const D3D12DSVDescriptor &dsvDescriptor = texture->dsvDescriptors[renderPassImage.subresourceIndex];
            useEntireSubresources = texture->dsvDescriptors.Count() == 1;
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
            GetInfoFromDSVDesc(dsvDescriptor.dsvDesc, destMipSlice, destFirstArraySlice, destArraySize);

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
            }
            break;
        }
        case RHI::RenderPassImage::Type::ShadingRateSource:
            d3d12CommandList->shadingRateImage = reinterpret_cast<ID3D12Resource *>(texture->GetNativeResource());
            break;
        default:
            assert(0);
            break;
        }

        // RenderPass 시작 배리어
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

                if (useEntireSubresources) {
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

        // RenderPass 종료 배리어
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

                if (useEntireSubresources) {
                    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    d3d12CommandList->endRenderPassBarriers.Append(barrier);
                } else {
                    for (UINT slice = 0; slice < destArraySize; ++slice) {
                        barrier.Transition.Subresource = D3D12CalcSubresource(destMipSlice, destFirstArraySlice + slice, 0, texture->textureDesc.MipLevels, texture->textureDesc.DepthOrArraySize);
                        d3d12CommandList->endRenderPassBarriers.Append(barrier);
                    }
                }
            }
        }
    }

    if (!barriers.IsEmpty()) {
        d3d12CommandList->GetGraphicsCommandList()->ResourceBarrier(barriers.Count(), barriers.Ptr());
    }

    if (d3d12CommandList->shadingRateImage) {
        d3d12CommandList->GetGraphicsCommandList()->RSSetShadingRateImage(d3d12CommandList->shadingRateImage);
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

    d3d12CommandList->GetGraphicsCommandList()->BeginRenderPass(rtCount, rtDescs, dsDesc.cpuDescriptor.ptr == 0 ? nullptr : &dsDesc, renderPassFlags);
}

void D3D12Renderer::EndRenderPass(RHI::CommandList *commandList) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);

#if 1
    d3d12CommandList->GetGraphicsCommandList()->EndRenderPass();
#endif

    if (d3d12CommandList->shadingRateImage) {
        d3d12CommandList->GetGraphicsCommandList()->RSSetShadingRateImage(nullptr);
        d3d12CommandList->shadingRateImage = nullptr;
    }

    if (!d3d12CommandList->endRenderPassBarriers.IsEmpty()) {
        d3d12CommandList->GetGraphicsCommandList()->ResourceBarrier((UINT)d3d12CommandList->endRenderPassBarriers.Count(), d3d12CommandList->endRenderPassBarriers.Ptr());
    }

    d3d12CommandList->endRenderPassBarriers.SetCount(0, false);
}

void D3D12Renderer::Draw(RHI::CommandList *commandList, uint32_t vertexCount, uint32_t startVertexLocation) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(d3d12CommandList, true);
    d3d12CommandList->GetGraphicsCommandList()->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
}

void D3D12Renderer::DrawIndexed(RHI::CommandList *commandList, uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(d3d12CommandList, true);
    d3d12CommandList->GetGraphicsCommandList()->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
}

void D3D12Renderer::DrawInstanced(RHI::CommandList *commandList, uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(d3d12CommandList, true);
    d3d12CommandList->GetGraphicsCommandList()->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
}

void D3D12Renderer::DrawIndexedInstanced(RHI::CommandList *commandList, uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    BindRootParameters(d3d12CommandList, true);
    d3d12CommandList->GetGraphicsCommandList()->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

int D3D12Renderer::AddRenderObject(const RenderObject::State &def) {
    assert(BE1::Engine::IsInMainThread());

    int index = renderObjects.FindNull();
    if (index == -1) {
        index = renderObjects.Append(nullptr);
    }

    UpdateRenderObject(index, def);
    return index;
}

void D3D12Renderer::UpdateRenderObject(int index, const RenderObject::State &def) {
    assert(BE1::Engine::IsInMainThread());

    while (index >= renderObjects.Count()) {
        renderObjects.Append(nullptr);
    }

    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        renderObject = new RenderObject;
        renderObject->index = index;
        renderObjects[index] = renderObject;
    }

    renderObject->Update(def);
}

void D3D12Renderer::RemoveRenderObject(int index) {
    assert(BE1::Engine::IsInMainThread());

    if (!renderObjects.IsValidIndex(index)) {
        BE_WARNLOG("D3D12Renderer::RemoveRenderObject: invalid index %i\n", index);
        return;
    }

    RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        BE_WARNLOG("D3D12Renderer::RemoveRenderObject: index %i is nullptr\n", index);
        return;
    }

    delete renderObjects[index];
    renderObjects[index] = nullptr;
}

void D3D12Renderer::RenderScene(/*const D3D12Camera *camera*/) {
    assert(BE1::Engine::IsInMainThread());

    PIX_CPU_SCOPED_EVENT(3, "D3D12Renderer::RenderScene");

#ifdef USE_RENDER_THREAD
    WaitRenderCompleted();

    // 렌더 스레드에서 다음 렌더링에 사용할 VisObject 들을 준비한다.
    // 
    // TODO: 보이는 오브젝트 수를 계산한다.
    int numVisObjects = renderObjects.Count();

    D3D12FrameData* writeFrameData = &frameData[currentFrameIndex];
    // TODO: RenderScene 을 여러번 호출할 수 있어야함
    VisObject* visObjects = writeFrameData->AllocVisObjects(numVisObjects);

    // TODO 1: 현재 카메라에 기반해 SceneGraph 나 Frustum culling 등으로 렌더링에 사용할 렌더 오브젝트들을 추려낸다. 추려낸 렌더 오브젝트들의 변수는 복사 or (레퍼런스 카운트를 이용한) 공유를 해서 가지고 있어야 한다.
    for (int i = 0; i < numVisObjects; ++i) {
        visObjects[i].GetState() = renderObjects[i]->GetState();
    }

    // TODO 2: 렌더링에 사용할 라이트들도 추려낸다.
    // TODO 3: 렌더링할 Surface 리스트를 작성한다.
    // TODO 4: Surface 들을 소팅한다.
    // TODO 5: 이후에는 Surface 단위로 그려야 한다.

    {
        BE1::ScopedWriteLock lock(smpLock);

        // (렌더 스레드의) 다음 렌더링이 끝나기를 기다리는 상태로 변경
        frameSyncState = FrameSyncState::WaitingForRenderCompleted;

        // 업데이트가 완료되었다고 신호를 보내고, 이후 다음 프레임의 업데이트를 진행한다.
        BE1::PlatformCondition::Signal(updateCompletedCondition);
    }
#else
    int numVisObjects = renderObjects.Count();

    D3D12FrameData *writeFrameData = &frameData[currentFrameIndex];
    VisObject *visObjects = writeFrameData->AllocVisObjects(numVisObjects);

    for (int i = 0; i < numVisObjects; ++i) {
        visObjects[i].GetState() = renderObjects[i]->GetState();
    }
#endif
}

void D3D12Renderer::RenderFrame() {
    PIX_SCOPED_EVENT(commandQueues[to_int(RHI::CommandQueueType::Graphics)], 4, "D3D12Renderer::RenderFrame");

    int numVisObjects = currentFrameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

#ifdef USE_RENDER_TASK
#ifdef USE_RENDEROBJECT_INSTANCING
    int numDrawCalls = (int)BE1::Math::Ceil((float)numVisObjects / 1024);
#else
    int numDrawCalls = numVisObjects;
#endif

    int numTasks = BE1::Min(renderTaskManager.NumThreads(), (int)BE1::Math::Ceil((float)numDrawCalls / MaxDrawCallsPerTask));
    if (numTasks > 1) {
        DrawVisObjectsWithTask(numTasks);
    } else {
        DrawVisObjectsWithoutTask();
    }
#else
    DrawVisObjectsWithoutTask();
#endif
}

// 특정 인덱스 범위의 visObjects 를 그린다.
void D3D12Renderer::DrawVisObjects(int threadIndex, D3D12CommandList *commandList, int startIndex, int endIndex) {
    PIX_SCOPED_EVENT(commandList->GetGraphicsCommandList(), 6, "D3D12VisObject::DrawVisObjects");

    int numVisObjects = currentFrameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

    VisObject *visObjects = currentFrameData->GetVisObjects();
    int index = startIndex;

    while (index <= endIndex) {
        VisObject *currentVisObjectPtr = &visObjects[index];

#ifdef USE_RENDEROBJECT_INSTANCING
        int instanceCount = BE1::Min(1024, endIndex - index + 1);
        if (instanceCount > 1) {
            VisObject::DrawInstanced(commandList, currentVisObjectPtr, instanceCount);
            index += instanceCount;
        } else {
            VisObject::Draw(commandList, &currentVisObjectPtr[0]);
            ++index;
        }
#else
        VisObject::Draw(commandList, &currentVisObjectPtr[0]);
        ++index;
#endif
    }
}

// 전체 visObjects 를 task 없이 한번에 그린다.
void D3D12Renderer::DrawVisObjectsWithoutTask() {
    PIX_CPU_SCOPED_EVENT(4, "D3D12Renderer::DrawVisObjectsWithoutTask");

    int numVisObjects = currentFrameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

    D3D12FrameData::DataPerThread &currentThreadData = currentFrameData->threadData[0];

#ifdef USE_SECONDARY_COMMAND_LISTS
    // ExecuteBundle 을 실행하기 전에 Primary CommandList 의 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { currentThreadData.rootDescriptorPool->descriptorHeap };
    mainCommandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // Secondary CommandList 를 얻어온다.
    D3D12CommandList *commandList = currentThreadData.graphicsCommandListPool->Alloc(RHI::CommandListType::Secondary);
    commandList->Reset(true, mainCommandList);

    // Secondary CommandList 의 루트 디스크립터 힙을 지정한다.
    // 반드시 Primary CommandList 와 동일한 디스크립터 힙을 사용해야 한다.
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // 플러시된 렌더 오브젝트들을 인덱스 범위 만큼 그린다.
    DrawVisObjects(0, commandList, 0, numVisObjects - 1);

    // Secondary CommandList 를 닫고 메인 CommandList 에 등록한다.
    commandList->CloseAndExecuteSecondary(mainCommandList);
#else
    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    D3D12CommandList *commandList = currentThreadData.graphicsCommandListPool->Alloc();
    commandList->Reset(true, mainCommandList);

    // 뷰포트 설정
    SetViewport(commandList, swapChain->viewportRect);
    // ScissorRect 설정
    SetScissorRect(commandList, swapChain->scissorRect);

    commandList->GetGraphicsCommandList()->OMSetRenderTargets(1, &swapChain->GetCurrentBackBufferRTVDescriptorHandle(), FALSE, &dsvDescriptorHandle);

    // 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { currentThreadData.rootDescriptorPool->descriptorHeap };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // 플러시된 렌더 오브젝트들을 인덱스 범위 만큼 그린다.
    DrawVisObjects(0, commandList, 0, numVisObjects - 1);

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute(RHI::CommandQueueType::Graphics);
#endif
}

#ifdef USE_RENDER_TASK
void D3D12Renderer::DrawVisObjectsByTask(D3D12Renderer::DrawObjectTaskDesc *taskDesc) {
    PIX_CPU_SCOPED_EVENT(5, "D3D12Renderer::DrawVisObjectsByTask");

    int threadIndex = taskDesc->threadIndex;
    D3D12FrameData::DataPerThread &currentThreadData = currentFrameData->threadData[threadIndex];

#ifdef USE_SECONDARY_COMMAND_LISTS
    D3D12CommandList *commandList = currentThreadData.graphicsCommandListPool->Alloc(RHI::CommandListType::Secondary);
    commandList->Reset();

    // Secondary CommandList 의 루트 디스크립터 힙을 지정한다.
    // 반드시 Primary CommandList 와 동일한 디스크립터 힙을 사용해야 한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { currentThreadData.rootDescriptorPool->descriptorHeap };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // 플러시된 렌더 오브젝트들을 인덱스 범위 만큼 그린다.
    DrawVisObjects(taskDesc->threadIndex, commandList, taskDesc->visObjectStartIndex, taskDesc->visObjectEndIndex);
#else
    D3D12CommandList *commandList = currentThreadData.graphicsCommandListPool->Alloc();
    commandList->Reset();

    // 뷰포트 & ScissorRect 설정
    SetViewport(commandList, swapChain->viewportRect);
    SetScissorRect(commandList, swapChain->scissorRect);

    commandList->GetGraphicsCommandList()->OMSetRenderTargets(1, &swapChain->GetCurrentBackBufferRTVDescriptorHandle(), FALSE, &dsvDescriptorHandle);

    // 루트 디스크립터 힙을 지정한다.
    ID3D12DescriptorHeap *descriptorHeaps[] = { currentThreadData.rootDescriptorPool->descriptorHeap };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    // 플러시된 렌더 오브젝트들을 인덱스 범위 만큼 그린다.
    DrawVisObjects(taskDesc->threadIndex, commandList, taskDesc->visObjectStartIndex, taskDesc->visObjectEndIndex);

    // 렌더링 시나리오에 따라 중간에 Flush 할 수도 있다.
    //commandList = FlushCommandList(commandList);
#endif
    // CommandList 기록을 마친다.
    commandList->GetGraphicsCommandList()->Close();

    // 사용 중인 커맨드 리스트를 나중에 실행하기 위해 저장한다.
    taskDesc->activeCommandList = commandList;
}

static void DrawVisObjectsByTaskFunction(void *data) {
    D3D12Renderer::DrawObjectTaskDesc *taskDesc = reinterpret_cast<D3D12Renderer::DrawObjectTaskDesc *>(data);
    taskDesc->renderer->DrawVisObjectsByTask(taskDesc);
}

// 전체 visObjects 를 task 로 나눠서 그린다.
void D3D12Renderer::DrawVisObjectsWithTask(int numTasks) {
    PIX_CPU_SCOPED_EVENT(6, "D3D12Renderer::DrawVisObjectsWithTask");

    int numVisObjects = currentFrameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

    int numVisObjectsPerTasks = (int)BE1::Math::Ceil((float)numVisObjects / numTasks);
    int threadIndex = 0;
    int lastEndIndex = -1;

    // 태스크 정보 초기화
    objectDrawingTaskDescs.Reserve(renderTaskManager.NumThreads());
    objectDrawingTaskDescs.SetCount(0, false);

    // 최대 쓰레드 개수만큼 task 를 실행한다.
    while (lastEndIndex < numVisObjects - 1) {
        DrawObjectTaskDesc &currentThreadDesc = objectDrawingTaskDescs.Alloc();

        currentThreadDesc.renderer = this;
        currentThreadDesc.threadIndex = threadIndex++;
        currentThreadDesc.visObjectStartIndex = lastEndIndex + 1;
        currentThreadDesc.visObjectEndIndex = BE1::Min(currentThreadDesc.visObjectStartIndex + numVisObjectsPerTasks, numVisObjects) - 1;
        renderTaskManager.AddTask(::DrawVisObjectsByTaskFunction, &currentThreadDesc, false);

        lastEndIndex = currentThreadDesc.visObjectEndIndex;
    }

    renderTaskManager.WaitFinish(true);

    // 태스크 별로 execute 할 CommandList 들을 모두 모은다.
    int renderTaskCount = objectDrawingTaskDescs.Count();
    ID3D12CommandList *execCommandLists[MaxRenderTaskThreads];
    for (int threadIndex = 0; threadIndex < renderTaskCount; ++threadIndex) {
        execCommandLists[threadIndex] = objectDrawingTaskDescs[threadIndex].activeCommandList->GetGraphicsCommandList();
    }
#ifdef USE_SECONDARY_COMMAND_LISTS
    for (int threadIndex = 0; threadIndex < renderTaskCount; ++threadIndex) {
        // ExecuteBundle 을 실행하기 전에 Primary CommandList 의 루트 디스크립터 힙을 지정한다.
        ID3D12DescriptorHeap *descriptorHeaps[] = { currentFrameData->threadData[threadIndex].rootDescriptorPool->descriptorHeap };
        mainCommandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

        mainCommandList->GetGraphicsCommandList()->ExecuteBundle(static_cast<ID3D12GraphicsCommandList6 *>(execCommandLists[threadIndex]));
    }
#else
    // CommandList 들을 한꺼번에 실행
    if (renderTaskCount > 0) {
        commandQueues[to_int(RHI::CommandQueueType::Graphics)]->ExecuteCommandLists(renderTaskCount, execCommandLists);
    }
#endif
}
#endif

#ifdef USE_RENDER_THREAD
void D3D12Renderer::InitRenderThread() {
    smpLock = BE1::PlatformSRWLock::Create();
    renderCompletedCondition = BE1::PlatformCondition::Create();
    updateCompletedCondition = BE1::PlatformCondition::Create();

    renderThread = BE1::PlatformThread::Start(RenderThreadProc, this);
}

void D3D12Renderer::ShutdownRenderThread() {
    {
        BE1::ScopedWriteLock lock(smpLock);
        isStoppingRenderThread = true;
        BE1::PlatformCondition::Signal(updateCompletedCondition);
    }
    BE1::PlatformThread::Join(renderThread);
    renderThread = nullptr;

    BE1::PlatformCondition::Destroy(renderCompletedCondition);
    BE1::PlatformCondition::Destroy(updateCompletedCondition);
    BE1::PlatformSRWLock::Destroy(smpLock);
}

void D3D12Renderer::WaitRenderCompleted() {
    assert(BE1::Engine::IsInMainThread());

    if (!renderThread) {
        return;
    }

    BE1::ScopedReadLock lock(smpLock);

    // 렌더 스레드가 렌더링이 완료되어 (다음) 업데이트를 기다리는 상태가 될 때까지 기다린다.
    BE1::PlatformCondition::Wait(renderCompletedCondition, smpLock, false, [this] {
        return frameSyncState == FrameSyncState::WaitingForUpdateCompleted;
    });
}

unsigned int RenderThreadProc(void *param) {
    D3D12Renderer *renderer = reinterpret_cast<D3D12Renderer *>(param);

    BE1::PlatformThread::SetCurrentThreadName("RenderThreadProc");

    BE1::SIMD::SetDenormalFlushMode(true);

    while (1) {
        PIX_CPU_SCOPED_EVENT(7, "RenderThreadProcLoop");
        {
            BE1::ScopedReadLock lock(renderer->smpLock);

            // 메인 스레드가 업데이트가 완료되어 (다음) 렌더링을 기다리는 상태가 될 때까지 기다린다.
            BE1::PlatformCondition::Wait(renderer->updateCompletedCondition, renderer->smpLock, false, [renderer] {
                return renderer->frameSyncState == FrameSyncState::WaitingForRenderCompleted || renderer->isStoppingRenderThread;
            });

            if (renderer->isStoppingRenderThread) {
                break;
            }
        }

        renderer->BeginFrame();
        renderer->RenderFrame();
        renderer->EndFrame();

        {
            BE1::ScopedWriteLock lock(renderer->smpLock);

            renderer->renderFrameIndex ^= renderer->renderFrameIndex;

            // (메인 스레드의) 다음 업데이트가 끝나기를 기다리는 상태로 변경
            renderer->frameSyncState = FrameSyncState::WaitingForUpdateCompleted;

            BE1::PlatformCondition::Signal(renderer->renderCompletedCondition);
        }
    }
    return 0;
}
#endif

bool D3D12Renderer::ImageFormatToDXGIFormat(BE1::Image::Format::Enum imageFormat, bool isSRGB, DXGI_FORMAT *dxgiFormat) {
    switch (imageFormat) {
    case BE1::Image::Format::Unknown:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_UNKNOWN;
        return true;
    case BE1::Image::Format::R_32_TYPELESS:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32_TYPELESS;
        return true;
    case BE1::Image::Format::L_8:
    case BE1::Image::Format::R_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8_UNORM;
        return true;
    case BE1::Image::Format::A_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_A8_UNORM;
        return true;
    case BE1::Image::Format::RG_8_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8G8_UNORM;
        return true;
    case BE1::Image::Format::RGBA_8_8_8_8:
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        return true;
    case BE1::Image::Format::BGRA_8_8_8_8:
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
        return true;
    case BE1::Image::Format::BGRX_8_8_8_8:
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB : DXGI_FORMAT_B8G8R8X8_UNORM;
        return true;
    case BE1::Image::Format::R_8_SNORM:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8_SNORM;
        return true;
    case BE1::Image::Format::RG_8_8_SNORM:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8G8_SNORM;
        return true;
    case BE1::Image::Format::RGBA_8_8_8_8_SNORM:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
        return true;
    case BE1::Image::Format::BGR_5_6_5:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_B5G6R5_UNORM;
        return true;
    case BE1::Image::Format::BGRA_4_4_4_4:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_B4G4R4A4_UNORM;
        return true;
    case BE1::Image::Format::ABGR_4_4_4_4:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_A4B4G4R4_UNORM;
        return true;
    case BE1::Image::Format::BGRA_5_5_5_1:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_B5G5R5A1_UNORM;
        return true;
    case BE1::Image::Format::RGBA_10_10_10_2:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
        return true;
    case BE1::Image::Format::L_16F:
    case BE1::Image::Format::R_16F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R16_FLOAT;
        return true;
    case BE1::Image::Format::RG_16F_16F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R16G16_FLOAT;
        return true;
    case BE1::Image::Format::RGBA_16F_16F_16F_16F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
        return true;
    case BE1::Image::Format::R_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32_FLOAT;
        return true;
    case BE1::Image::Format::RG_32F_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32G32_FLOAT;
        return true;
    case BE1::Image::Format::RGB_32F_32F_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        return true;
    case BE1::Image::Format::RGBA_32F_32F_32F_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        return true;
    case BE1::Image::Format::RGBE_9_9_9_5:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        return true;
    case BE1::Image::Format::RGB_11F_11F_10F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R11G11B10_FLOAT;
        return true;
    case BE1::Image::Format::DXT1: // BC1
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
        return true;
    case BE1::Image::Format::DXT3: // BC2
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
        return true;
    case BE1::Image::Format::DXT5: // BC3
        if (dxgiFormat) *dxgiFormat = isSRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
        return true;
    case BE1::Image::Format::DXN1: // BC4
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_BC4_UNORM;
        return true;
    case BE1::Image::Format::DXN2: // BC5
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_BC5_UNORM;
        return true;
    case BE1::Image::Format::Depth_16:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D16_UNORM;
        return true;
    case BE1::Image::Format::Depth_24:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        return true;
    case BE1::Image::Format::Depth_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D32_FLOAT;
        return true;
    case BE1::Image::Format::DepthStencil_24_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        return true;
    }
    return false;
}

bool D3D12Renderer::DXGIFormatToImageFormat(DXGI_FORMAT dxgiFormat, BE1::Image::Format::Enum *imageFormat, bool *isSRGB) {
    if (isSRGB) {
        *isSRGB = false;
        switch (dxgiFormat) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
            *isSRGB = true;
            break;
        }
    }

    switch (dxgiFormat) {
    case DXGI_FORMAT_R8_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R_8;
        return true;
    case DXGI_FORMAT_A8_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::A_8;
        return true;
    case DXGI_FORMAT_R8G8_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::RG_8_8;
        return true;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::RGBA_8_8_8_8;
        return true;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::BGRA_8_8_8_8;
        return true;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::BGRX_8_8_8_8;
        return true;
    case DXGI_FORMAT_R8_SNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R_8_SNORM;
        return true;
    case DXGI_FORMAT_R8G8_SNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::RG_8_8_SNORM;
        return true;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::RGBA_8_8_8_8_SNORM;
        return true;
    case DXGI_FORMAT_B5G6R5_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::BGR_5_6_5;
        return true;
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::BGRA_4_4_4_4;
        return true;
    case DXGI_FORMAT_A4B4G4R4_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::ABGR_4_4_4_4;
        return true;
    case DXGI_FORMAT_B5G5R5A1_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::BGRA_5_5_5_1;
        return true;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::RGBA_10_10_10_2;
        return true;
    case DXGI_FORMAT_R16_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R_16F;
        return true;
    case DXGI_FORMAT_R16G16_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::RG_16F_16F;
        return true;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::RGBA_16F_16F_16F_16F;
        return true;
    case DXGI_FORMAT_R32_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R_32F;
        return true;
    case DXGI_FORMAT_R32G32_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::RG_32F_32F;
        return true;
    case DXGI_FORMAT_R32G32B32_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::RGB_32F_32F_32F;
        return true;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::RGBA_32F_32F_32F_32F;
        return true;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        if (imageFormat) *imageFormat = BE1::Image::Format::RGBE_9_9_9_5;
        return true;
    case DXGI_FORMAT_R11G11B10_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::RGB_11F_11F_10F;
        return true;
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::DXT1;
        return true;
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::DXT3;
        return true;
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::DXT5;
        return true;
    case DXGI_FORMAT_BC4_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::DXN1;
        return true;
    case DXGI_FORMAT_BC5_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::DXN2;
        return true;
    case DXGI_FORMAT_D16_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::Depth_16;
        return true;
    case DXGI_FORMAT_D32_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::Depth_32F;
        return true;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::DepthStencil_24_8;
        return true;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::DepthStencil_32F_8;
        return true;
    }
    return false;
}

bool D3D12Renderer::IsDepthFormat(DXGI_FORMAT format) {
    if (format == DXGI_FORMAT::DXGI_FORMAT_D16_UNORM ||
        format == DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT ||
        format == DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT ||
        format == DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
        return true;
    }
    return false;
}

bool D3D12Renderer::IsStencilFormat(DXGI_FORMAT format) {
    if (format == DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT ||
        format == DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
        return true;
    }
    return false;
}

BE1::Image::Format::Enum D3D12Renderer::ToUncompressedImageFormat(BE1::Image::Format::Enum inFormat) const {
    BE1::Image::Format::Enum outFormat;

    switch (inFormat) {
    case BE1::Image::Format::RGB_5_6_5:
    case BE1::Image::Format::RGB_8_8_8:
    case BE1::Image::Format::BGR_5_6_5:
    case BE1::Image::Format::BGR_8_8_8:
    case BE1::Image::Format::RGBX_4_4_4_4:
    case BE1::Image::Format::RGBX_5_5_5_1:
    case BE1::Image::Format::RGBX_8_8_8_8:
    case BE1::Image::Format::BGRX_4_4_4_4:
    case BE1::Image::Format::BGRX_5_5_5_1:
        outFormat = BE1::Image::Format::BGRX_8_8_8_8;
        break;
    case BE1::Image::Format::LA_8_8:
    case BE1::Image::Format::RGBA_4_4_4_4:
    case BE1::Image::Format::RGBA_5_5_5_1:
    case BE1::Image::Format::BGRA_4_4_4_4:
    case BE1::Image::Format::BGRA_5_5_5_1:
    case BE1::Image::Format::ABGR_4_4_4_4:
    case BE1::Image::Format::ABGR_1_5_5_5:
    case BE1::Image::Format::ABGR_8_8_8_8:
    case BE1::Image::Format::ARGB_4_4_4_4:
    case BE1::Image::Format::ARGB_1_5_5_5:
    case BE1::Image::Format::ARGB_8_8_8_8:
        outFormat = BE1::Image::Format::BGRA_8_8_8_8;
        break;
    case BE1::Image::Format::RGB_8_8_8_SNORM:
        outFormat = BE1::Image::Format::RGBA_8_8_8_8_SNORM;
        break;
    case BE1::Image::Format::RGB_16F_16F_16F:
        outFormat = BE1::Image::Format::RGBA_16F_16F_16F_16F;
        break;
    case BE1::Image::Format::RGB_32F_32F_32F:
        outFormat = BE1::Image::Format::RGBA_32F_32F_32F_32F;
        break;
    case BE1::Image::Format::DXN1:
    case BE1::Image::Format::DXN2:
    case BE1::Image::Format::RGB_PVRTC_2BPPV1:
    case BE1::Image::Format::RGB_PVRTC_4BPPV1:
    case BE1::Image::Format::RGB_8_ETC1:
    case BE1::Image::Format::RGB_8_ETC2:
    case BE1::Image::Format::RGB_ATC:
        outFormat = BE1::Image::Format::BGRX_8_8_8_8;
        break;
    case BE1::Image::Format::DXT1:
    case BE1::Image::Format::DXT3:
    case BE1::Image::Format::DXT5:
    case BE1::Image::Format::RGBA_PVRTC_2BPPV1:
    case BE1::Image::Format::RGBA_PVRTC_4BPPV1:
    case BE1::Image::Format::RGBA_PVRTC_2BPPV2:
    case BE1::Image::Format::RGBA_PVRTC_4BPPV2:
    case BE1::Image::Format::RGBA_8_1_ETC2:
    case BE1::Image::Format::RGBA_8_8_ETC2:
    case BE1::Image::Format::RGBA_EA_ATC:
    case BE1::Image::Format::RGBA_IA_ATC:
        outFormat = BE1::Image::Format::RGBA_8_8_8_8;
        break;
    case BE1::Image::Format::R_11_EAC:
    case BE1::Image::Format::SignedR_11_EAC:
        outFormat = BE1::Image::Format::R_16F;
        break;
    case BE1::Image::Format::RG_11_11_EAC:
    case BE1::Image::Format::SignedRG_11_11_EAC:
        outFormat = BE1::Image::Format::RG_16F_16F;
        break;
    default:
        assert(0);
        outFormat = inFormat;
        break;
    }
    return outFormat;
}

BE1::Image::Format::Enum D3D12Renderer::ToCompressedImageFormat(BE1::Image::Format::Enum inFormat, bool useNormalMap) const {
    if (BE1::Image::IsCompressed(inFormat)) {
        assert(0);
        return inFormat;
    }

    int redBits, greenBits, blueBits, alphaBits;
    BE1::Image::GetBits(inFormat, &redBits, &greenBits, &blueBits, &alphaBits);

    BE1::Image::Format::Enum outFormat = inFormat;

    if (redBits > 0 && greenBits > 0 && blueBits > 0) {
        if (BE1::Image::IsFloatFormat(inFormat) || BE1::Image::IsHalfFormat(inFormat)) {
            if (alphaBits == 0) {
                outFormat = BE1::Image::Format::RGBE_9_9_9_5;
            }
        } else if (useNormalMap) {
            outFormat = BE1::Image::Format::DXN2;
        } else {
            if (alphaBits <= 1) {
                outFormat = BE1::Image::Format::DXT1;
            } else if (alphaBits <= 4) {
                outFormat = BE1::Image::Format::DXT3;
            } else {
                outFormat = BE1::Image::Format::DXT5;
            }
        }
    }

    return outFormat;
}
