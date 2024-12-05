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
#include "D3D12CommandList.h"
#include "D3D12CommandListPool.h"
#include "D3D12RootDescriptorPool.h"
#include "D3D12DescriptorPool.h"
#include "D3D12CompiledShaderBlob.h"
#include "D3D12VisObject.h"

// D3D12.dll 이 D3D12Core.dll 을 찾기 위한 설정
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }
extern "C" { __declspec(dllexport) extern const char *D3D12SDKPath = u8"."; }

D3D12Renderer       renderer;

static Str          shaderCacheDir = "Cache/D3D12CompiledShaderCache";

void D3D12Renderer::Init(HWND hwnd) {
#if defined(_DEBUG) || defined(_DEVELOPMENT)
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
    BE_LOG("Dedicated VideoMem Size: %s\n", Str::FormatBytes(dedicatedVideoMemSize).c_str());
    BE_LOG("Dedicated SystemMem Size: %s\n", Str::FormatBytes(dedicatedSystemMemSize).c_str());
    BE_LOG("Shared SystemMem Size: %s\n", Str::FormatBytes(sharedSystemMemSize).c_str());

    if (enableDebugLayer) {
        // 디버그 표시 정보 설정
        ID3D12InfoQueue *infoQueue = nullptr;
        hr = device->QueryInterface(IID_PPV_ARGS(&infoQueue));
        if (SUCCEEDED(hr)) {
#ifdef _DEBUG
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
#endif

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

    // Graphics CommandQueue 생성
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueues[to_int(D3D12CommandQueueType::Graphics)]));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandQueue (Graphics) failed, ERROR: 0x%x", hr);
    }
    commandQueues[to_int(D3D12CommandQueueType::Graphics)]->SetName(L"GraphicsCommandQueue");

    // Compute CommandQueue 생성
    queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

    hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueues[to_int(D3D12CommandQueueType::Compute)]));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandQueue (Compute) failed, ERROR: 0x%x", hr);
    }
    commandQueues[to_int(D3D12CommandQueueType::Compute)]->SetName(L"ComputeCommandQueue");

    // Fence 객체 생성
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateFence failed, ERROR: 0x%x", hr);
    }
    // Fence 초기값
    fenceValue = 0;

    // Fence 를 대기하기 위한 이벤트 객체 생성
    fenceEventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // 디스크립터 힙 타입 별 디스크립터 핸들 사이즈 정보 얻기 (보통은 32바이트를 차지)
    descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // 디스크립터 풀 생성
    srvDescriptorPool = new D3D12DescriptorPool(D3D12DescriptorPool::Type::SRV, 1000000, false);
    rtvDescriptorPool = new D3D12DescriptorPool(D3D12DescriptorPool::Type::RTV, 16, false);
    dsvDescriptorPool = new D3D12DescriptorPool(D3D12DescriptorPool::Type::DSV, 16, false);
    samplerDescriptorPool = new D3D12DescriptorPool(D3D12DescriptorPool::Type::Sampler, 2048, true);

    // Init feature check (https://devblogs.microsoft.com/directx/introducing-a-new-api-for-checking-feature-support-in-direct3d-12/)
    CD3DX12FeatureSupport features;
    hr = features.Init(device);
    assert(SUCCEEDED(hr));

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

    // 윈도우 크기 얻기
    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT backBufferWidth = rc.right;
    UINT backBufferHeight = rc.bottom;

    // 스왑 체인 (백버퍼) 생성
    CreateSwapChain(hwnd, backBufferWidth, backBufferHeight);

    //IDXGIOutput *output = nullptr;
    //hr = swapChain->GetContainingOutput(&output);

    // Viewport 설정을 백버퍼 크기에 맞게 설정
    viewport.Width = (float)backBufferWidth;
    viewport.Height = (float)backBufferHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    // ScissorRect 설정을 백버퍼 크기에 맞게 설정
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = viewport.Width;
    scissorRect.bottom = viewport.Height;

    // 백버퍼 용 디스크립터 힙 생성
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = NumSwapChainBuffers;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateDescriptorHeap for back buffers failed, ERROR: 0x%x", hr);
    }

    // 뎁스/스텐실 버퍼 용 디스크립터 힙 생성
    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateDescriptorHeap for depth/stencil buffer failed, ERROR: 0x%x", hr);
    }

    CreateRTVs();

    CreateDSV(backBufferWidth, backBufferHeight);

    // 커맨드 리스트 풀 생성
    commandListPool = new D3D12CommandListPool(D3D12_COMMAND_LIST_TYPE_DIRECT, 8);

    // 리소스 생성 용 커맨드 리스트
    resourceCommandList = commandListPool->Alloc();

    // 현재 백버퍼 인덱스 초기화
    currentBackBufferIndex = dxgiSwapChain->GetCurrentBackBufferIndex();

    maxPendingResources = 1024;
    pendingResourceBuffer = new D3D12PendingResource[maxPendingResources];

#ifdef USE_RENDER_TASK
    // 렌더 태스크 스레드를 최대 물리코어 개수만큼만 생성한다.
    int numCores = PlatformSystem::NumCPUCores();
    renderTaskManager.Start(Min(numCores, MaxRenderTaskThreads));
#endif

    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        frameData[frameIndex].Init();
    }

    currentFrameIndex = 0;
    frameData[currentFrameIndex].SetFenceValue(SignalFence());

#ifdef USE_RENDER_THREAD
    InitRenderThread();
#endif

    renderObjects.Reserve(16384);

    initialized = true;
}

void D3D12Renderer::Shutdown() {
#ifdef USE_RENDER_THREAD
    ShutdownRenderThread();
#endif

#ifdef USE_RENDER_TASK
    renderTaskManager.Stop();
#endif

    Finish();

    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        frameData[frameIndex].Shutdown();
    }

    FreePendingResources(true);
    SAFE_DELETE(pendingResourceBuffer);
    maxPendingResources = 0;

    SAFE_DELETE(srvDescriptorPool);
    SAFE_DELETE(rtvDescriptorPool);
    SAFE_DELETE(dsvDescriptorPool);
    SAFE_DELETE(samplerDescriptorPool);
    SAFE_DELETE(commandListPool);

    SAFE_RELEASE(rtvDescriptorHeap);
    SAFE_RELEASE(dsvDescriptorHeap);
    SAFE_RELEASE_ARRAY(renderTargetBuffers);
    SAFE_RELEASE(depthStencilBuffer);
    SAFE_RELEASE(dxgiSwapChain);
    SAFE_RELEASE(commandQueues[to_int(D3D12CommandQueueType::Graphics)]);
    SAFE_RELEASE(commandQueues[to_int(D3D12CommandQueueType::Compute)]);
    SAFE_RELEASE(fence);
    SAFE_RELEASE(dxgiFactory);

#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(allocator);
#endif

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
        PlatformSystem::DebugBreak();
    }
}

void D3D12Renderer::CreateSwapChain(HWND hwnd, int width, int height) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = (UINT)width;
    swapChainDesc.Height = (UINT)height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
    //swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = NumSwapChainBuffers;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    if (supportsTearing) {
        swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = {};
    swapChainFullscreenDesc.Windowed = TRUE;

    IDXGISwapChain1 *swapChain1 = nullptr;
    HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(commandQueues[to_int(D3D12CommandQueueType::Graphics)], hwnd, &swapChainDesc, &swapChainFullscreenDesc, nullptr, &swapChain1);
    if (FAILED(hr)) {
        BE_FATALERROR("CreateSwapChainForHwnd failed, ERROR: 0x%x", hr);
    }
    hr = dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr)) {
        BE_FATALERROR("MakeWindowAssociation failed, ERROR: 0x%x", hr);
    }
    hr = swapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain));
    swapChain1->Release();
    if (FAILED(hr)) {
        BE_FATALERROR("Failed to create swapchain, ERROR: 0x%x", hr);
    }
}

void D3D12Renderer::CreateRTVs() {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 스왑 체인의 버퍼를 가져와서 각 RTV 에 연결한다.
    for (UINT renderTargetIndex = 0; renderTargetIndex < NumSwapChainBuffers; ++renderTargetIndex) {
        dxgiSwapChain->GetBuffer(renderTargetIndex, IID_PPV_ARGS(&renderTargetBuffers[renderTargetIndex]));

        device->CreateRenderTargetView(renderTargetBuffers[renderTargetIndex], nullptr, rtvDescriptorHandle);

        rtvDescriptorHandle.Offset(1, descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    }
}

void D3D12Renderer::CreateDSV(int width, int height) {
    // 뎁스/스텐실 버퍼 생성
    D3D12_CLEAR_VALUE depthStencilOptimizedClearValue = {};
    depthStencilOptimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthStencilOptimizedClearValue.DepthStencil.Stencil = 0;

    D3D12_RESOURCE_DESC depthStencilBufferDesc = {};
    depthStencilBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilBufferDesc.Alignment = 0;
    depthStencilBufferDesc.Width = width;
    depthStencilBufferDesc.Height = height;
    depthStencilBufferDesc.DepthOrArraySize = 1;
    depthStencilBufferDesc.MipLevels = 1;
    depthStencilBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilBufferDesc.SampleDesc.Count = 1;
    depthStencilBufferDesc.SampleDesc.Quality = 0;
    depthStencilBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

    D3D12_HEAP_PROPERTIES heapProperties;
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilBufferDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthStencilOptimizedClearValue,
        IID_PPV_ARGS(&depthStencilBuffer));
    if (FAILED(hr)) {
        BE_FATALERROR("Create depth/stencil buffer failed, ERROR: 0x%x", hr);
    }
    //depthStencilBuffer->SetName(L"depthStencilBuffer");

    // 뎁스/스텐실 버퍼를 DSV 에 연결한다.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    device->CreateDepthStencilView(depthStencilBuffer, &dsvDesc, dsvDescriptorHandle);
}

void D3D12Renderer::BeginFrame() {
    PIX_SCOPED_EVENT(commandQueues[to_int(D3D12CommandQueueType::Graphics)], 0, "D3D12Renderer::BeginFrame");

    currentFrameData = &frameData[currentFrameIndex];

    // 프레임 데이터를 초기화하고, 이전 프레임에 대한 펜스를 기다린다.
    currentFrameData->BeginFrame();

    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    D3D12CommandList* commandList = currentFrameData->GetThreadData(0).commandListPool->Alloc();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    // 백버퍼를 렌더 타겟 상태로 전환
    commandList->ResourceBarrier(renderTargetBuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 뷰포트 & ScissorRect 설정
    commandList->graphicsCommandList->RSSetViewports(1, &viewport);
    commandList->graphicsCommandList->RSSetScissorRects(1, &scissorRect);

    rtvDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex, descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    dsvDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 백버퍼와 깊이버퍼를 Clear
    commandList->graphicsCommandList->ClearRenderTargetView(rtvDescriptorHandle, Color4::blue, 0, nullptr);
    commandList->graphicsCommandList->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    commandList->graphicsCommandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, &dsvDescriptorHandle);

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute(D3D12CommandQueueType::Graphics);
}

void D3D12Renderer::EndFrame() {
    PIX_SCOPED_EVENT(commandQueues[to_int(D3D12CommandQueueType::Graphics)], 1, "D3D12Renderer::EndFrame");

    // TODO: 렌더큐에 종료 마킹을 하고, 렌더큐를 실행한다.

    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    D3D12CommandList *commandList = currentFrameData->GetThreadData(0).commandListPool->Alloc();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset(false);

    // 백버퍼 RTV 를 Present 할 수 있는 상태로 전환
    commandList->ResourceBarrier(renderTargetBuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute(D3D12CommandQueueType::Graphics);

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
    PIX_SCOPED_EVENT(commandQueues[to_int(D3D12CommandQueueType::Graphics)], 2, "D3D12Renderer::SwapChainBuffers");

    if (dxgiSwapChain->Present(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING) == DXGI_ERROR_DEVICE_REMOVED) {
        BE_FATALERROR("DXGI Device Removed");
    }

    // 다음 프레임에 사용할 백버퍼 인덱스 얻어오기
    currentBackBufferIndex = dxgiSwapChain->GetCurrentBackBufferIndex();
}

D3D12CommandList* D3D12Renderer::FlushCommandList(D3D12CommandList* commandList) {
    PIX_SCOPED_EVENT(commandQueues[to_int(D3D12CommandQueueType::Graphics)], 3, "D3D12Renderer::FlushCommandList");

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute(D3D12CommandQueueType::Graphics);

    // 커맨드 리스트 풀에서 새로운 커맨드 리스트를 얻어온다.
    commandList = commandList->parentPool->Alloc();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    // 뷰포트 & ScissorRect 설정
    commandList->graphicsCommandList->RSSetViewports(1, &viewport);
    commandList->graphicsCommandList->RSSetScissorRects(1, &scissorRect);
    commandList->graphicsCommandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, &dsvDescriptorHandle);

    return commandList;
}

uint64_t D3D12Renderer::SignalFence() {
    fenceValue++;
    commandQueues[to_int(D3D12CommandQueueType::Graphics)]->Signal(fence, fenceValue);

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

void D3D12Renderer::Finish() {
    WaitFence(SignalFence());
}

void D3D12Renderer::WaitAllFrameFences() {
    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        WaitFence(frameData[frameIndex].GetFenceValue());
    }
}

void D3D12Renderer::MarkForRelease(ID3D12Resource *resource) {
    D3D12PendingResource *newPendingResource = &pendingResourceBuffer[headPendingIndex];
    newPendingResource->fenceValue = SignalFence();
    newPendingResource->resourceToRelease = resource;

    OnPendingResourceAdded();
}

void D3D12Renderer::MarkForDelete(Resource *resource) {
    D3D12PendingResource *newPendingResource = &pendingResourceBuffer[headPendingIndex];
    newPendingResource->fenceValue = SignalFence();
    newPendingResource->resourceToDelete = resource;

    OnPendingResourceAdded();
}

void D3D12Renderer::OnPendingResourceAdded() {
    headPendingIndex = headPendingIndex + 1;

    // 버퍼가 꽉 찼다면, 가장 오래된 pending resource 를 기다린 후 Release 한다.
    if (headPendingIndex % maxPendingResources == tailPendingIndex) {
        D3D12PendingResource *oldestPendingResource = &pendingResourceBuffer[tailPendingIndex];

        WaitFence(oldestPendingResource->fenceValue);

        SAFE_RELEASE(oldestPendingResource->resourceToRelease);
        SAFE_DELETE(oldestPendingResource->resourceToDelete)

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
        SAFE_DELETE(pendingResource->resourceToDelete)

        tailPendingIndex = (tailPendingIndex + 1) % maxPendingResources;
    }
}

void D3D12Renderer::OnResize(int width, int height) {
#ifdef USE_RENDER_THREAD
    WaitRenderCompleted();
#endif

    Finish();

    // 기존 백버퍼 해제
    SAFE_RELEASE_ARRAY(renderTargetBuffers);

    // 기존 뎁스/스텐실 버퍼 해제
    SAFE_RELEASE(depthStencilBuffer);

    // 스왑 체인 버퍼의 사이즈를 조정한다.
    dxgiSwapChain->ResizeBuffers(NumSwapChainBuffers, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

    CreateRTVs();

    CreateDSV(width, height);

    currentBackBufferIndex = dxgiSwapChain->GetCurrentBackBufferIndex();

    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);

    scissorRect.right = width;
    scissorRect.bottom = height;
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

void D3D12Renderer::CacheCompiledShader(const char *name, const uint64_t hash, ID3DBlob *compiledShaderBlob) {
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
#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif
        ID3DBlob *errorBlob = nullptr;

        if (FAILED(D3DCompile(shaderText, shaderTextSize, sourceName, nullptr, nullptr, entryPoint, target, compileFlags, 0, &compiledShaderBlob, &errorBlob))) {
            PrintCompileErrorMessages(errorBlob);
            SAFE_RELEASE(errorBlob);
            return nullptr;
        }

        SAFE_RELEASE(errorBlob);

        // 컴파일했으므로 cso 파일을 저장한다.
        CacheCompiledShader(fileName, shaderTextHash, compiledShaderBlob);
    }

    D3D12Shader *shader = new D3D12Shader;
    shader->shaderStage = shaderStage;
    shader->compiledShaderBlob = compiledShaderBlob;
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

ID3D12PipelineState *D3D12Renderer::CreatePSOFromLibrary(const D3D12_PIPELINE_STATE_STREAM_DESC *streamDesc, ID3D12PipelineLibrary1 *library, const TCHAR *name) {
    ID3D12PipelineState *pso = nullptr;
    HRESULT hr;

    if (library) {
        hr = library->LoadPipeline(name, streamDesc, IID_PPV_ARGS(&pso));
        if (hr == E_INVALIDARG) {
            hr = device->CreatePipelineState(streamDesc, IID_PPV_ARGS(&pso));
            if (SUCCEEDED(hr)) {
                library->StorePipeline(name, pso);
            }
        }
    } else {
        hr = device->CreatePipelineState(streamDesc, IID_PPV_ARGS(&pso));
        if (FAILED(hr)) {
            BE_ERRLOG("Failed to create pipeline state with name %s, ERROR: 0x%x.", name, hr);
        }
    }

    return pso;
}

ID3D12PipelineState *D3D12Renderer::CreatePSO(ID3D12RootSignature *rootSignature, const D3D12_SHADER_BYTECODE &byteCodeVS, const D3D12_SHADER_BYTECODE &byteCodePS, const D3D12_INPUT_LAYOUT_DESC &inputLayout) {
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    };
    ID3D12PipelineState *pso = nullptr;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    // NOTE: 나중에 호출할 SetGraphicsRootSignature() 에서 PSO 에 지정된 RootSignature 와 다르면 안된다.
    // 여기서 RootSignature 를 지정하는 이유는 파이프라인 호환성 검사 및 최적화 때문이다.
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = byteCodeVS;
    psoDesc.PS = byteCodePS;
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

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr)) {
        BE_WARNLOG("device->CreateGraphicsPipelineState() failed, ERROR: 0x%x\n", hr);
        return nullptr;
    }
    return pso;
}

ID3D12PipelineState *D3D12Renderer::CreatePSO(ID3D12RootSignature *rootSignature, const char *shaderFilename, const D3D12_INPUT_LAYOUT_DESC &inputLayout) {
    char *shaderText;
    int shaderTextSize = fileSystem.LoadFile(shaderFilename, true, (void **)&shaderText);
    if (!shaderText) {
        return nullptr;
    }

    D3D12Shader *vs = static_cast<D3D12Shader *>(CreateShader(ShaderStage::Vertex, shaderFilename, shaderText, shaderTextSize, "VSMain"));
    D3D12Shader *ps = static_cast<D3D12Shader *>(CreateShader(ShaderStage::Fragment, shaderFilename, shaderText, shaderTextSize, "PSMain"));

    fileSystem.FreeFile(shaderText);

    if (!vs || !ps) {
        SAFE_DELETE(vs);
        SAFE_DELETE(ps);
        return nullptr;
    }

    D3D12_SHADER_BYTECODE byteCodeVS = CD3DX12_SHADER_BYTECODE(vs->compiledShaderBlob->GetBufferPointer(), vs->compiledShaderBlob->GetBufferSize());
    D3D12_SHADER_BYTECODE byteCodePS = CD3DX12_SHADER_BYTECODE(ps->compiledShaderBlob->GetBufferPointer(), ps->compiledShaderBlob->GetBufferSize());

    ID3D12PipelineState *pso = CreatePSO(rootSignature, byteCodeVS, byteCodePS, inputLayout);

    SAFE_DELETE(vs);
    SAFE_DELETE(ps);

    return pso;
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

#ifdef USE_D3D12_MEMALLOC
void D3D12Renderer::PrintMemoryAllocatorStats() {
    D3D12MA::Budget localBudget;
    D3D12MA::Budget nonLocalBudget;

    allocator->GetBudget(&localBudget, &nonLocalBudget);

    // TODO: 확인 필요
    // GPU 에서 사용 중인 메모리 크기, 사용 가능한 메모리 크기
    BE_LOG("D3D12 reports total usage %s with budget %s (%.2f %%)\n", Str::FormatBytes(localBudget.UsageBytes).c_str(), Str::FormatBytes(localBudget.BudgetBytes).c_str(), (100.0f * localBudget.UsageBytes) / localBudget.BudgetBytes);
    // D3D12 heap 에 할당된 리소스의 개수, 크기
    BE_LOG("allocated out of %u D3D12 memory heaps taking %s\n", localBudget.Stats.BlockCount, Str::FormatBytes(localBudget.Stats.BlockBytes).c_str());
    // 프로그램에서 실제 사용 중인 리소스 메모리의 개수, 크기
    BE_LOG("GPU memory currently has %u allocations taking %s\n", localBudget.Stats.AllocationCount, Str::FormatBytes(localBudget.Stats.AllocationBytes).c_str());
}
#endif

int D3D12Renderer::AddRenderObject(const D3D12RenderObject::State &def) {
    assert(Engine::IsInMainThread());

    int index = renderObjects.FindNull();
    if (index == -1) {
        index = renderObjects.Append(nullptr);
    }

    UpdateRenderObject(index, def);
    return index;
}

void D3D12Renderer::UpdateRenderObject(int index, const D3D12RenderObject::State &def) {
    assert(Engine::IsInMainThread());

    while (index >= renderObjects.Count()) {
        renderObjects.Append(nullptr);
    }

    D3D12RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        renderObject = new D3D12RenderObject;
        renderObject->index = index;
        renderObjects[index] = renderObject;
    }

    renderObject->Update(def);
}

void D3D12Renderer::RemoveRenderObject(int index) {
    assert(Engine::IsInMainThread());

    if (!renderObjects.IsValidIndex(index)) {
        BE_WARNLOG("D3D12Renderer::RemoveRenderObject: invalid index %i\n", index);
        return;
    }

    D3D12RenderObject *renderObject = renderObjects[index];
    if (!renderObject) {
        BE_WARNLOG("D3D12Renderer::RemoveRenderObject: index %i is nullptr\n", index);
        return;
    }

    delete renderObjects[index];
    renderObjects[index] = nullptr;
}

void D3D12Renderer::RenderScene(/*const D3D12Camera *camera*/) {
    assert(Engine::IsInMainThread());

    PIX_CPU_SCOPED_EVENT(3, "D3D12Renderer::RenderScene");

#ifdef USE_RENDER_THREAD
    WaitRenderCompleted();

    // 렌더 스레드에서 다음 렌더링에 사용할 VisObject 들을 준비한다.
    // 
    // TODO: 보이는 오브젝트 수를 계산한다.
    int numVisObjects = renderObjects.Count();

    D3D12FrameData* writeFrameData = &frameData[currentFrameIndex];
    // TODO: RenderScene 을 여러번 호출할 수 있어야함
    D3D12VisObject* visObjects = writeFrameData->AllocVisObjects(numVisObjects);

    // TODO 1: 현재 카메라에 기반해 SceneGraph 나 Frustum culling 등으로 렌더링에 사용할 렌더 오브젝트들을 추려낸다. 추려낸 렌더 오브젝트들의 변수는 복사 or (레퍼런스 카운트를 이용한) 공유를 해서 가지고 있어야 한다.
    for (int i = 0; i < numVisObjects; ++i) {
        visObjects[i].GetState() = renderObjects[i]->GetState();
    }

    // TODO 2: 렌더링에 사용할 라이트들도 추려낸다.
    // TODO 3: 렌더링할 Surface 리스트를 작성한다.
    // TODO 4: Surface 들을 소팅한다.
    // TODO 5: 이후에는 Surface 단위로 그려야 한다.

    {
        ScopedWriteLock lock(smpLock);

        // (렌더 스레드의) 다음 렌더링이 끝나기를 기다리는 상태로 변경
        frameSyncState = FrameSyncState::WaitingForRenderCompleted;

        // 업데이트가 완료되었다고 신호를 보내고, 이후 다음 프레임의 업데이트를 진행한다.
        PlatformCondition::Signal(updateCompletedCondition);
    }
#else
    int numVisObjects = renderObjects.Count();

    D3D12FrameData *writeFrameData = &frameData[currentFrameIndex];
    D3D12VisObject *visObjects = writeFrameData->AllocVisObjects(numVisObjects);

    for (int i = 0; i < numVisObjects; ++i) {
        visObjects[i].state = renderObjects[i]->state;
    }
#endif
}

void D3D12Renderer::RenderFrame() {
    PIX_SCOPED_EVENT(commandQueues[to_int(D3D12CommandQueueType::Graphics)], 4, "D3D12Renderer::RenderFrame");

    int numVisObjects = currentFrameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

#ifdef USE_RENDER_TASK
#ifdef USE_RENDEROBJECT_INSTANCING
    int numDrawCalls = (int)Math::Ceil((float)numVisObjects / 1024);
#else
    int numDrawCalls = numVisObjects;
#endif

    int numTasks = Min(renderTaskManager.NumThreads(), (int)Math::Ceil((float)numDrawCalls / MaxDrawCallsPerTask));
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
    int numVisObjects = currentFrameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

    D3D12VisObject *visObjects = currentFrameData->GetVisObjects();
    int index = startIndex;

    while (index <= endIndex) {
        D3D12VisObject *currentVisObjectPtr = &visObjects[index];

#ifdef USE_RENDEROBJECT_INSTANCING
        int instanceCount = Min(1024, endIndex - index + 1);
        if (instanceCount > 1) {
            D3D12VisObject::DrawInstanced(threadIndex, commandList, currentVisObjectPtr, instanceCount);
            index += instanceCount;
        } else {
            D3D12VisObject::Draw(threadIndex, commandList, &currentVisObjectPtr[0]);
            ++index;
        }
#else
        D3D12VisObject::Draw(threadIndex, commandList, &currentVisObjectPtr[0]);
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

    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    D3D12CommandList *commandList = currentFrameData->GetThreadData(0).commandListPool->Alloc();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    // 뷰포트 & ScissorRect 설정
    commandList->graphicsCommandList->RSSetViewports(1, &viewport);
    commandList->graphicsCommandList->RSSetScissorRects(1, &scissorRect);
    commandList->graphicsCommandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, &dsvDescriptorHandle);

    // 플러시된 렌더 오브젝트들을 인덱스 범위 만큼 그린다.
    DrawVisObjects(0, commandList, 0, numVisObjects - 1);

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute(D3D12CommandQueueType::Graphics);
}

#ifdef USE_RENDER_TASK
void D3D12Renderer::DrawVisObjectsByTask(D3D12Renderer::DrawObjectTaskDesc *taskDesc) {
    PIX_CPU_SCOPED_EVENT(5, "D3D12Renderer::DrawVisObjectsByTask");

    int threadIndex = taskDesc->threadIndex;
    D3D12CommandListPool *commandListPool = currentFrameData->GetThreadData(threadIndex).commandListPool;
    D3D12CommandList *commandList = commandListPool->Alloc();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    // 뷰포트 & ScissorRect 설정
    commandList->graphicsCommandList->RSSetViewports(1, &viewport);
    commandList->graphicsCommandList->RSSetScissorRects(1, &scissorRect);
    commandList->graphicsCommandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, &dsvDescriptorHandle);

    // 플러시된 렌더 오브젝트들을 인덱스 범위 만큼 그린다.
    DrawVisObjects(taskDesc->threadIndex, commandList, taskDesc->visObjectStartIndex, taskDesc->visObjectEndIndex);

    // 렌더링 시나리오에 따라 중간에 Flush 할 수도 있다.
    //commandList = FlushCommandList(commandList);

    // CommandList 기록을 마친다.
    commandList->graphicsCommandList->Close();

    // 사용 중인 커맨드 리스트를 나중에 실행하기 위해 저장한다.
    taskDesc->activeCommandList = commandList;
}

static void DrawVisObjectsByTaskFunction(void *data) {
    D3D12Renderer::DrawObjectTaskDesc *taskDesc = reinterpret_cast<D3D12Renderer::DrawObjectTaskDesc *>(data);
    renderer.DrawVisObjectsByTask(taskDesc);
}

// 전체 visObjects 를 task 로 나눠서 그린다.
void D3D12Renderer::DrawVisObjectsWithTask(int numTasks) {
    PIX_CPU_SCOPED_EVENT(6, "D3D12Renderer::DrawVisObjectsWithTask");

    int numVisObjects = currentFrameData->NumVisObjects();
    if (numVisObjects == 0) {
        return;
    }

    int numVisObjectsPerTasks = (int)Math::Ceil((float)numVisObjects / numTasks);
    int threadIndex = 0;
    int lastEndIndex = -1;

    // 태스크 정보 초기화
    objectDrawingTaskDescs.Reserve(renderTaskManager.NumThreads());
    objectDrawingTaskDescs.SetCount(0, false);

    // 최대 쓰레드 개수만큼 task 를 실행한다.
    while (lastEndIndex < numVisObjects - 1) {
        DrawObjectTaskDesc &currentThreadDesc = objectDrawingTaskDescs.Alloc();

        currentThreadDesc.threadIndex = threadIndex++;
        currentThreadDesc.visObjectStartIndex = lastEndIndex + 1;
        currentThreadDesc.visObjectEndIndex = Min(currentThreadDesc.visObjectStartIndex + numVisObjectsPerTasks, numVisObjects) - 1;
        renderTaskManager.AddTask(::DrawVisObjectsByTaskFunction, &currentThreadDesc, false);

        lastEndIndex = currentThreadDesc.visObjectEndIndex;
    }

    renderTaskManager.WaitFinish(true);

    // 태스크 별로 execute 할 CommandList 들을 모두 모은다.
    int renderTaskCount = objectDrawingTaskDescs.Count();
    ID3D12CommandList *execCommandLists[MaxRenderTaskThreads];
    for (int threadIndex = 0; threadIndex < renderTaskCount; ++threadIndex) {
        execCommandLists[threadIndex] = objectDrawingTaskDescs[threadIndex].activeCommandList->graphicsCommandList;
    }

    // CommandList 들을 한꺼번에 실행
    if (renderTaskCount > 0) {
        commandQueues[to_int(D3D12CommandQueueType::Graphics)]->ExecuteCommandLists(renderTaskCount, execCommandLists);
    }
}
#endif

#ifdef USE_RENDER_THREAD
void D3D12Renderer::InitRenderThread() {
    smpLock = PlatformSRWLock::Create();
    renderCompletedCondition = PlatformCondition::Create();
    updateCompletedCondition = PlatformCondition::Create();

    renderThread = PlatformThread::Start(RenderThreadProc, this);
}

void D3D12Renderer::ShutdownRenderThread() {
    {
        ScopedWriteLock lock(smpLock);
        isStoppingRenderThread = true;
        PlatformCondition::Signal(updateCompletedCondition);
    }
    PlatformThread::Join(renderThread);
    renderThread = nullptr;

    PlatformCondition::Destroy(renderCompletedCondition);
    PlatformCondition::Destroy(updateCompletedCondition);
    PlatformSRWLock::Destroy(smpLock);
}

void D3D12Renderer::WaitRenderCompleted() {
    assert(Engine::IsInMainThread());

    if (!renderThread) {
        return;
    }

    ScopedReadLock lock(smpLock);

    // 렌더 스레드가 렌더링이 완료되어 (다음) 업데이트를 기다리는 상태가 될 때까지 기다린다.
    PlatformCondition::Wait(renderCompletedCondition, smpLock, false, [this] {
        return frameSyncState == FrameSyncState::WaitingForUpdateCompleted;
    });
}

unsigned int RenderThreadProc(void *param) {
    PlatformThread::SetCurrentThreadName("RenderThreadProc");

    SIMD::SetDenormalFlushMode(true);

    while (1) {
        PIX_CPU_SCOPED_EVENT(7, "RenderThreadProcLoop");
        {
            ScopedReadLock lock(renderer.smpLock);

            // 메인 스레드가 업데이트가 완료되어 (다음) 렌더링을 기다리는 상태가 될 때까지 기다린다.
            PlatformCondition::Wait(renderer.updateCompletedCondition, renderer.smpLock, false, [] {
                return renderer.frameSyncState == FrameSyncState::WaitingForRenderCompleted || renderer.isStoppingRenderThread;
            });

            if (renderer.isStoppingRenderThread) {
                break;
            }
        }

        renderer.BeginFrame();
        renderer.RenderFrame();
        renderer.EndFrame();

        {
            ScopedWriteLock lock(renderer.smpLock);

            renderer.renderFrameIndex ^= renderer.renderFrameIndex;

            // (메인 스레드의) 다음 업데이트가 끝나기를 기다리는 상태로 변경
            renderer.frameSyncState = FrameSyncState::WaitingForUpdateCompleted;

            PlatformCondition::Signal(renderer.renderCompletedCondition);
        }
    }
    return 0;
}
#endif
