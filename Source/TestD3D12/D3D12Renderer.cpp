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
#include "D3D12Renderer.h"
#include "D3D12CommandList.h"
#include "D3D12CommandListPool.h"
#include "D3D12RootDescriptorPool.h"
#include "D3D12DescriptorPool.h"

// D3D12.dll 이 D3D12Core.dll 을 찾기 위한 설정
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }
extern "C" { __declspec(dllexport) extern const char *D3D12SDKPath = u8"."; }

D3D12Renderer       renderer;

void D3D12Renderer::Init(HWND hwnd) {
#ifdef USE_DEBUG_LAYER
    bool enableDebugLayer = true;
    bool withGpuValidation = true;
#else
    bool enableDebugLayer = false;
    bool withGpuValidation = false;
#endif

    DWORD createFactoryFlags = 0;
    HRESULT hr;

    if (enableDebugLayer) {
        // 디버그 레이어 활성화
        ID3D12Debug* debugController = nullptr;
        hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
        if (SUCCEEDED(hr)) {
            debugController->EnableDebugLayer();
            createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            // GPU Validation 활성화
            if (withGpuValidation) {
                ID3D12Debug5* debugController5 = nullptr;
                if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))))
                {
                    debugController5->SetEnableGPUBasedValidation(TRUE);
                    debugController5->SetEnableAutoName(TRUE);
                    debugController5->Release();
                }
            }
            debugController->Release();
        }
    }

    IDXGIFactory4* factory4 = nullptr;
    CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory4));

    // 어댑터 정보 얻어오기
    IDXGIAdapter1* adapter1 = nullptr;
    factory4->EnumAdapters1(0, &adapter1);
    adapter1->GetDesc1(&adapterDesc);

    // D3D12 디바이스 생성
    hr = D3D12CreateDevice(adapter1, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device));
    if (FAILED(hr)) {
        BE_FATALERROR("D3D12CreateDevice : failed");
    }

    // 디버그 표시 정보 설정
    ID3D12InfoQueue *infoQueue = nullptr;
    device->QueryInterface(IID_PPV_ARGS(&infoQueue));
    if (infoQueue) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

        D3D12_MESSAGE_ID hideMessages[] = {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // Workarounds for debug layer issues on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = (UINT)COUNT_OF(hideMessages);
        filter.DenyList.pIDList = hideMessages;
        infoQueue->AddStorageFilterEntries(&filter);
        infoQueue->Release();
    }

    // 디스크립터 힙 타입 별 디스크립터 핸들 사이즈 정보 얻기 (보통은 32바이트를 차지)
    descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // CommandQueue 생성
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandQueue : failed");
    }

    // 스왑 체인 (백버퍼) 생성
    RECT rc;
    GetClientRect(hwnd, &rc);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = rc.right;
    swapChainDesc.Height = rc.bottom;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
    //swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = NumSwapChainBuffers;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = {};
    swapChainFullscreenDesc.Windowed = TRUE;

    IDXGISwapChain1 *swapChain1 = nullptr;
    hr = factory4->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
    if (FAILED(hr)) {
        BE_FATALERROR("CreateSwapChainForHwnd : failed");
    }
    hr = factory4->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr)) {
        BE_FATALERROR("MakeWindowAssociation : failed");
    }
    swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain));
    swapChain1->Release();
    factory4->Release();

    // Viewport 설정을 백버퍼 크기에 맞게 설정
    viewport.Width = (float)swapChainDesc.Width;
    viewport.Height = (float)swapChainDesc.Height;
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
        BE_FATALERROR("CreateDescriptorHeap for back buffers: failed");
    }

    // 뎁스/스텐실 버퍼 용 디스크립터 힙 생성
    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.NumDescriptors = 1;
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateDescriptorHeap for depth/stencil buffer : failed");
    }

    CreateRTVs();

    CreateDSV(swapChainDesc.Width, swapChainDesc.Height);

    commandListPool = new D3D12CommandListPool(D3D12_COMMAND_LIST_TYPE_DIRECT, 8);
    resourceCommandList = commandListPool->Alloc();

    // Fence 객체 생성
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateFence : failed");
    }
    // Fence 초기값
    fenceValue = 0;

    // Fence 를 대기하기 위한 이벤트 객체 생성
    fenceEventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // 현재 백버퍼 인덱스 초기화
    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

#ifdef USE_D3D12_MEMALLOC
    // D3D12MA Allocator 생성
    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = device;
    allocatorDesc.pAdapter = adapter1;
    allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
    allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_DONT_PREFER_SMALL_BUFFERS_COMMITTED;

    hr = D3D12MA::CreateAllocator(&allocatorDesc, &allocator);
    if (FAILED(hr)) {
        BE_FATALERROR("D3D12MA::CreateAllocator : failed");
    }
#endif

    adapter1->Release();

    srvDescriptorPool = new D3D12DescriptorPool(D3D12DescriptorPool::Type::SRV, 100000, false);
    rtvDescriptorPool = new D3D12DescriptorPool(D3D12DescriptorPool::Type::RTV, 16, false);
    dsvDescriptorPool = new D3D12DescriptorPool(D3D12DescriptorPool::Type::DSV, 16, false);

    maxPendingResources = 1024;
    pendingResourceBuffer = new D3D12PendingResource[maxPendingResources];

    renderObjects.Reserve(16384);
    flushedRenderObjects[0].Reserve(16384);
    flushedRenderObjects[1].Reserve(16384);

#ifdef USE_RENDER_TASK
    // 태스크 스레드를 최대 물리코어 개수만큼만 생성한다.
    int numCores = PlatformSystem::NumCPUCores();
    taskManager.Start(Min(numCores, MaxRenderTaskThreads));
#endif

    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        frameData[frameIndex].Init();
    }

    currentFrameIndex = 0;
    frameData[currentFrameIndex].lastFrameFenceValue = SignalFence();

#ifdef USE_RENDER_THREAD
    InitRenderThread();
#endif

    initialized = true;
}

void D3D12Renderer::Shutdown() {
#ifdef USE_RENDER_THREAD
    ShutdownRenderThread();
#endif

#ifdef USE_RENDER_TASK
    taskManager.Stop();
#endif

    Finish();

    FreePendingResources(true);
    SAFE_DELETE(pendingResourceBuffer);
    maxPendingResources = 0;

    for (int frameIndex = 0; frameIndex < NumFrameResources; ++frameIndex) {
        frameData[frameIndex].Shutdown();
    }

    SAFE_DELETE(srvDescriptorPool);
    SAFE_DELETE(rtvDescriptorPool);
    SAFE_DELETE(dsvDescriptorPool);

    SAFE_RELEASE(rtvDescriptorHeap);
    SAFE_RELEASE(dsvDescriptorHeap);
    SAFE_RELEASE_ARRAY(renderTargetBuffers);
    SAFE_RELEASE(depthStencilBuffer);
    SAFE_RELEASE(swapChain);
    SAFE_RELEASE(commandQueue);
    SAFE_DELETE(commandListPool);
    SAFE_RELEASE(fence);

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

void D3D12Renderer::CreateRTVs() {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 스왑 체인의 버퍼를 가져와서 각 RTV 에 연결한다.
    for (UINT renderTargetIndex = 0; renderTargetIndex < NumSwapChainBuffers; ++renderTargetIndex) {
        swapChain->GetBuffer(renderTargetIndex, IID_PPV_ARGS(&renderTargetBuffers[renderTargetIndex]));

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
        BE_FATALERROR("Create depth/stencil buffer : failed");
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
    PIX_SCOPED_EVENT(commandQueue, 0, "D3D12Renderer::BeginFrame");

    currentFrameIndex = frameCount % NumFrameResources;
    currentFrameData = &frameData[currentFrameIndex];

    // 프레임 데이터를 초기화하고, 이전 프레임에 대한 펜스를 기다린다.
    currentFrameData->BeginFrame();

    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    D3D12CommandList* commandList = currentFrameData->threadData[0].commandListPool->Alloc();

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
    commandList->CloseAndExecute();
}

void D3D12Renderer::EndFrame() {
    PIX_SCOPED_EVENT(commandQueue, 1, "D3D12Renderer::EndFrame");

    // TODO: 렌더큐에 종료 마킹을 하고, 렌더큐를 실행한다.

    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    D3D12CommandList *commandList = currentFrameData->threadData[0].commandListPool->Alloc();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset(false);

    // 백버퍼 RTV 를 Present 할 수 있는 상태로 전환
    commandList->ResourceBarrier(renderTargetBuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute();

    // 이번 프레임에서 수행하는 렌더링 커맨드들에 대한 펜스를 친다.
    currentFrameData->EndFrame();

    // 백버퍼를 전면버퍼와 교환한다.
    SwapChainBuffers();

    frameCount++;

    FreePendingResources();
}

void D3D12Renderer::SwapChainBuffers() {
    PIX_SCOPED_EVENT(commandQueue, 2, "D3D12Renderer::SwapChainBuffers");

    if (swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING) == DXGI_ERROR_DEVICE_REMOVED) {
        BE_FATALERROR("DXGI Device Removed");
    }

    // 다음 프레임에 사용할 백버퍼 인덱스 얻어오기
    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
}

D3D12CommandList* D3D12Renderer::FlushCommandList(D3D12CommandList* commandList) {
    PIX_SCOPED_EVENT(commandQueue, 3, "D3D12Renderer::FlushCommandList");

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute();

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

UINT64 D3D12Renderer::SignalFence() {
    fenceValue++;
    commandQueue->Signal(fence, fenceValue);

    return fenceValue;
}

bool D3D12Renderer::IsFenceComplete(UINT64 checkFenceValue) {
    return fence->GetCompletedValue() < checkFenceValue ? false : true;
}

void D3D12Renderer::WaitFence(UINT64 expectedFenceValue) {
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
        WaitFence(frameData[frameIndex].lastFrameFenceValue);
    }
}

void D3D12Renderer::MarkForRelease(ID3D12Resource *resource) {
    D3D12PendingResource *newPendingResource = &pendingResourceBuffer[headPendingIndex];
    newPendingResource->fenceValue = SignalFence();
    newPendingResource->resource = resource;

    headPendingIndex = headPendingIndex + 1;

    // 버퍼가 꽉 찼다면, 가장 오래된 pending resource 를 기다린 후 Release 한다.
    if (headPendingIndex % maxPendingResources == tailPendingIndex) {
        D3D12PendingResource *oldestPendingResource = &pendingResourceBuffer[tailPendingIndex];

        WaitFence(oldestPendingResource->fenceValue);
        oldestPendingResource->resource->Release();
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

        pendingResource->resource->Release();

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
    swapChain->ResizeBuffers(NumSwapChainBuffers, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

    CreateRTVs();

    CreateDSV(width, height);

    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);

    scissorRect.right = width;
    scissorRect.bottom = height;
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

#ifdef USE_RENDER_THREAD
    // 렌더 스레드에서 읽는 중 (FlushRenderObjects() 호출로 포인터가 복사된 상태) 일 수 있으므로, 렌더링이 완료될 때까지 delete 를 지연시켜야 한다.
    WaitRenderCompleted();
#endif

    delete renderObjects[index];
    renderObjects[index] = nullptr;
}

void D3D12Renderer::FlushRenderObjects() {
    assert(Engine::IsInMainThread());

    PIX_CPU_SCOPED_EVENT(3, "D3D12Renderer::FlushRenderObjects");

#ifdef USE_RENDER_THREAD
    WaitRenderCompleted();

    Array<D3D12RenderObject *> &currentFlushedRenderObjects = flushedRenderObjects[renderFrameIndex ^ 1];
    currentFlushedRenderObjects.SetCount(0, false);

    for (int i = 0; i < renderObjects.Count(); ++i) {
        if (renderObjects[i]) {
            currentFlushedRenderObjects.Append(renderObjects[i]);
        }
    }

    {
        ScopeWriteLock scopeLock(smpLock);

        // (렌더 스레드의) 다음 렌더링이 끝나기를 기다리는 상태로 변경
        frameSyncState = FrameSyncState::WaitingForRenderCompleted;

        PlatformCondition::Signal(updateCompletedCondition);
    }
#else
    Array<D3D12RenderObject *> &currentFlushedRenderObjects = flushedRenderObjects[0];
    currentFlushedRenderObjects.SetCount(0, false);

    for (int i = 0; i < renderObjects.Count(); ++i) {
        if (renderObjects[i]) {
            currentFlushedRenderObjects.Append(renderObjects[i]);
        }
    }
#endif
}

void D3D12Renderer::RenderCamera() {
    PIX_SCOPED_EVENT(commandQueue, 4, "D3D12Renderer::RenderCamera");

#ifdef USE_RENDER_THREAD
    Array<D3D12RenderObject *> &currentFlushedRenderObjects = flushedRenderObjects[renderFrameIndex];
#else
    Array<D3D12RenderObject *> &currentFlushedRenderObjects = flushedRenderObjects[0];
#endif

    int numRenderObjects = currentFlushedRenderObjects.Count();
    if (numRenderObjects == 0) {
        return;
    }

    // TODO 1: 현재 카메라에 기반해 SceneGraph 나 Frustum culling 등으로 렌더링에 사용할 렌더 오브젝트들을 추려낸다.
    // TODO 2: 렌더링에 사용할 라이트들도 추려낸다.
    // TODO 3: 렌더링할 Surface 리스트를 작성한다.
    // TODO 4: Surface 들을 소팅한다.
    // TODO 5: 이후에는 Surface 단위로 그려야 한다.

#ifdef USE_RENDER_TASK
#ifdef USE_RENDEROBJECT_INSTANCING
    int numDrawCalls = (int)Math::Ceil((float)numRenderObjects / 1024);
#else
    int numDrawCalls = numRenderObjects;
#endif

    int numTasks = Min(taskManager.NumThreads(), (int)Math::Ceil((float)numDrawCalls / MaxDrawCallsPerTask));
    if (numTasks > 1) {
        DrawRenderObjectsWithTask(numTasks);
    } else {
        DrawRenderObjectsWithoutTask();
    }
#else
    DrawRenderObjectsWithoutTask();
#endif
}

// 특정 범위 인덱스의 flushedRenderObjects 를 그린다.
void D3D12Renderer::DrawRenderObjects(int threadIndex, D3D12CommandList *commandList, int startIndex, int endIndex) {
#ifdef USE_RENDER_THREAD
    Array<D3D12RenderObject *> &currentFlushedRenderObjects = flushedRenderObjects[renderFrameIndex];
#else
    Array<D3D12RenderObject *> &currentFlushedRenderObjects = flushedRenderObjects[0];
#endif

    int index = startIndex;

    while (index <= endIndex) {
        D3D12RenderObject **renderObjectPtr = &currentFlushedRenderObjects[index];

#ifdef USE_RENDEROBJECT_INSTANCING
        int instanceCount = Min(1024, endIndex - index + 1);
        if (instanceCount > 1) {
            D3D12RenderObject::DrawInstanced(threadIndex, commandList, renderObjectPtr, instanceCount);
            index += instanceCount;
        } else {
            renderObjectPtr[0]->Draw(threadIndex, commandList);
            ++index;
        }
#else
        renderObjectPtr[0]->Draw(threadIndex, commandList);
        ++index;
#endif
    }
}

// 전체 flushedRenderObjects 를 task 없이 한번에 그린다.
void D3D12Renderer::DrawRenderObjectsWithoutTask() {
    PIX_CPU_SCOPED_EVENT(4, "D3D12Renderer::DrawRenderObjectsWithoutTask");

#ifdef USE_RENDER_THREAD
    Array<D3D12RenderObject *> &currentFlushedRenderObjects = flushedRenderObjects[renderFrameIndex];
#else
    Array<D3D12RenderObject *> &currentFlushedRenderObjects = flushedRenderObjects[0];
#endif

    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    D3D12CommandList *commandList = currentFrameData->threadData[0].commandListPool->Alloc();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    // 뷰포트 & ScissorRect 설정
    commandList->graphicsCommandList->RSSetViewports(1, &viewport);
    commandList->graphicsCommandList->RSSetScissorRects(1, &scissorRect);
    commandList->graphicsCommandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, &dsvDescriptorHandle);

    // 플러시된 렌더 오브젝트들을 인덱스 범위 만큼 그린다.
    DrawRenderObjects(0, commandList, 0, currentFlushedRenderObjects.Count() - 1);

    // CommandList 기록을 마치고 CommandQueue 로 실행
    commandList->CloseAndExecute();
}

#ifdef USE_RENDER_TASK
void D3D12Renderer::DrawRenderObjectsByTask(D3D12Renderer::RenderObjectTaskDesc *taskDesc) {
    PIX_CPU_SCOPED_EVENT(5, "D3D12Renderer::DrawRenderObjectsByTask");

    int threadIndex = taskDesc->threadIndex;
    D3D12CommandListPool *commandListPool = currentFrameData->threadData[threadIndex].commandListPool;
    D3D12CommandList *commandList = commandListPool->Alloc();

    // CommandAllocator 를 재사용하도록 리셋하고, CommandList 를 CommandAllocator 를 이용하여 초기 상태로 리셋
    commandList->Reset();

    // 뷰포트 & ScissorRect 설정
    commandList->graphicsCommandList->RSSetViewports(1, &viewport);
    commandList->graphicsCommandList->RSSetScissorRects(1, &scissorRect);
    commandList->graphicsCommandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, &dsvDescriptorHandle);

    // 플러시된 렌더 오브젝트들을 인덱스 범위 만큼 그린다.
    DrawRenderObjects(taskDesc->threadIndex, commandList, taskDesc->renderObjectStartIndex, taskDesc->renderObjectEndIndex);

    // 렌더링 시나리오에 따라 중간에 Flush 할 수도 있다.
    //commandList = FlushCommandList(commandList);

    // CommandList 기록을 마친다.
    commandList->graphicsCommandList->Close();

    // 사용 중인 커맨드 리스트를 나중에 실행하기 위해 저장한다.
    taskDesc->activeCommandList = commandList;
}

static void RenderObjectsByTask(void *data) {
    D3D12Renderer::RenderObjectTaskDesc *taskDesc = reinterpret_cast<D3D12Renderer::RenderObjectTaskDesc *>(data);
    renderer.DrawRenderObjectsByTask(taskDesc);
}

// 전체 flushedRenderObjects 를 task 로 나눠서 그린다.
void D3D12Renderer::DrawRenderObjectsWithTask(int numTasks) {
    PIX_CPU_SCOPED_EVENT(6, "D3D12Renderer::DrawRenderObjectsWithTask");

#ifdef USE_RENDER_THREAD
    Array<D3D12RenderObject *> &currentFlushedRenderObjects = flushedRenderObjects[renderFrameIndex];
#else
    Array<D3D12RenderObject *> &currentFlushedRenderObjects = flushedRenderObjects[0];
#endif

    int numRenderObjects = currentFlushedRenderObjects.Count();
    int numRenderObjectsPerTasks = (int)Math::Ceil((float)numRenderObjects / numTasks);

    int threadIndex = 0;
    int lastEndIndex = -1;

    // 태스크 정보 초기화
    renderObjectTaskDescs.Reserve(taskManager.NumThreads());
    renderObjectTaskDescs.SetCount(0, false);

    // 최대 쓰레드 개수만큼 task 를 실행한다.
    while (lastEndIndex < numRenderObjects - 1) {
        RenderObjectTaskDesc &currentThreadDesc = renderObjectTaskDescs.Alloc();

        currentThreadDesc.threadIndex = threadIndex++;
        currentThreadDesc.renderObjectStartIndex = lastEndIndex + 1;
        currentThreadDesc.renderObjectEndIndex = Min(currentThreadDesc.renderObjectStartIndex + numRenderObjectsPerTasks, numRenderObjects) - 1;
        taskManager.AddTask(RenderObjectsByTask, &currentThreadDesc, false);

        lastEndIndex = currentThreadDesc.renderObjectEndIndex;
    }

    taskManager.WaitFinish(true);

    // 태스크 별로 execute 할 CommandList 들을 모두 모은다.
    int renderTaskCount = renderObjectTaskDescs.Count();
    ID3D12CommandList *execCommandLists[MaxRenderTaskThreads];
    for (int threadIndex = 0; threadIndex < renderTaskCount; ++threadIndex) {
        execCommandLists[threadIndex] = renderObjectTaskDescs[threadIndex].activeCommandList->graphicsCommandList;
    }

    // CommandList 들을 한꺼번에 실행
    if (renderTaskCount > 0) {
        commandQueue->ExecuteCommandLists(renderTaskCount, execCommandLists);
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
        ScopeWriteLock scopeLock(smpLock);
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

    ScopeReadLock scopeLock(smpLock);

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
            ScopeReadLock scopeLock(renderer.smpLock);

            // 메인 스레드가 업데이트가 완료되어 (다음) 렌더링을 기다리는 상태가 될 때까지 기다린다.
            PlatformCondition::Wait(renderer.updateCompletedCondition, renderer.smpLock, false, [] {
                return renderer.frameSyncState == FrameSyncState::WaitingForRenderCompleted || renderer.isStoppingRenderThread;
            });

            if (renderer.isStoppingRenderThread) {
                break;
            }
        }

        renderer.BeginFrame();
        renderer.RenderCamera();
        renderer.EndFrame();

        {
            ScopeWriteLock scopeLock(renderer.smpLock);

            renderer.renderFrameIndex ^= renderer.renderFrameIndex;

            // (메인 스레드의) 다음 업데이트가 끝나기를 기다리는 상태로 변경
            renderer.frameSyncState = FrameSyncState::WaitingForUpdateCompleted;

            PlatformCondition::Signal(renderer.renderCompletedCondition);
        }
    }
    return 0;
}
#endif
