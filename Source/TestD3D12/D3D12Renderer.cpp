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
#include <dxgidebug.h>

// D3D12.dll 이 D3D12Core.dll 을 찾기 위한 설정
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }
extern "C" { __declspec(dllexport) extern const char *D3D12SDKPath = u8"."; }

D3D12Renderer       renderer;

void D3D12Renderer::Init(HWND hwnd, bool enableDebugLayer, bool withGpuValidation) {
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

    // 커맨드 큐 생성
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
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = {};
    swapChainFullscreenDesc.Windowed = TRUE;

    IDXGISwapChain1 *swapChain1 = nullptr;
    hr = factory4->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, &swapChainFullscreenDesc, nullptr, &swapChain1);
    if (FAILED(hr)) {
        BE_FATALERROR("CreateSwapChainForHwnd : failed");
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

    // 그래픽스 커맨드 리스트를 위한 커맨드 할당자 생성
    hr = renderer.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandAllocator : failed");
    }

    // 그래픽스 커맨드 리스트 생성
    hr = renderer.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandList : failed");
    }

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    commandList->Close();

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

    singleDescriptorAllocator = new D3D12SingleDescriptorAllocator;
    singleDescriptorAllocator->Init(10000);

    for (int frameIndex = 0; frameIndex < NumFrames; ++frameIndex) {
        frameData[frameIndex].Init();
    }

    currentFrameIndex = 0;
    frameData[currentFrameIndex].fenceValue = SignalFence();

    maxPendingResources = 1024;
    pendingResourceBuffer = new D3D12PendingResource[maxPendingResources];

    initialized = true;
}

void D3D12Renderer::Shutdown() {
    Finish();

    FreePendingResources();
    SAFE_DELETE(pendingResourceBuffer);
    maxPendingResources = 0;

    for (int frameIndex = 0; frameIndex < NumFrames; ++frameIndex) {
        frameData[frameIndex].Shutdown();
    }

    SAFE_DELETE(singleDescriptorAllocator);

#ifdef USE_D3D12_MEMALLOC
    SAFE_RELEASE(allocator);
#endif
    SAFE_RELEASE(rtvDescriptorHeap);
    SAFE_RELEASE(dsvDescriptorHeap);
    SAFE_RELEASE_ARRAY(renderTargetBuffers);
    SAFE_RELEASE(depthStencilBuffer);
    SAFE_RELEASE(swapChain);
    SAFE_RELEASE(commandQueue);
    SAFE_RELEASE(commandList);
    SAFE_RELEASE(commandAllocator);
    SAFE_RELEASE(fence);

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
    depthStencilBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    HRESULT hr = device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
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

void D3D12Renderer::BeginRender() {
    currentFrameData = &frameData[currentFrameIndex];
    currentFrameData->BeginRender();

    // 이번 프레임에 사용할 프레임 데이터의 사용이 이전 프레임에서 완료될 때까지 기다린다.
    WaitFence(currentFrameData->fenceValue);

    // 루트 디스크립터 풀을 비운다.
    currentFrameData->rootDescriptorPool->Reset();

    // 커맨드 리스트 풀에서 커맨드 리스트를 얻어온다.
    currentFrameCommandList = currentFrameData->commandListPool->Alloc();

    // 커맨드 할당자를 재사용하도록 리셋
    currentFrameCommandList->commandAllocator->Reset();

    // 커맨드 리스트를 커맨드 할당자를 이용하여 초기 상태로 리셋
    currentFrameCommandList->commandList->Reset(currentFrameCommandList->commandAllocator, nullptr);

    // 뷰포트 & ScissorRect 설정
    currentFrameCommandList->commandList->RSSetViewports(1, &viewport);
    currentFrameCommandList->commandList->RSSetScissorRects(1, &scissorRect);

    // 백버퍼를 렌더 타겟 상태로 전환
    currentFrameCommandList->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargetBuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex, descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 백버퍼와 깊이버퍼를 Clear
    currentFrameCommandList->commandList->ClearRenderTargetView(rtvDescriptorHandle, Color4::blue, 0, nullptr);
    currentFrameCommandList->commandList->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    currentFrameCommandList->commandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, &dsvDescriptorHandle);
}

void D3D12Renderer::EndRender() {
    // 백버퍼 RTV 를 Present 할 수 있는 상태로 전환
    currentFrameCommandList->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargetBuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // 커맨드 리스트 기록을 마친다.
    currentFrameCommandList->commandList->Close();

    // 커맨드 큐에 커맨드 리스트 전달 (한번에 여러개의 커맨드 리스트들을 전달할 수 있다)
    ID3D12CommandList *ppCommandLists[] = { currentFrameCommandList->commandList };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 커맨드 리스트 풀에 커맨드 리스트 반납
    currentFrameData->commandListPool->Free(currentFrameCommandList);
    currentFrameCommandList = nullptr;
}

void D3D12Renderer::Present() {
    // 이번 프레임에서 수행하는 렌더링 커맨드들에 대한 펜스를 친다.
    SignalFence();

    currentFrameData->fenceValue = fenceValue;

    // 백버퍼를 전면버퍼와 교환한다.
    if (swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING) == DXGI_ERROR_DEVICE_REMOVED) {
        BE_FATALERROR("DXGI Device Removed");
    }

    // 다음 프레임에 사용할 백버퍼 인덱스 얻어오기
    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

    frameCount++;

    currentFrameIndex = (frameCount % NumFrames);

    FreePendingResources();
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

void D3D12Renderer::FreePendingResources() {
    while (headPendingIndex != tailPendingIndex) {
        D3D12PendingResource *pendingResource = &pendingResourceBuffer[tailPendingIndex];
        if (!IsFenceComplete(pendingResource->fenceValue)) {
            return;
        }

        pendingResource->resource->Release();

        tailPendingIndex = (tailPendingIndex + 1) % maxPendingResources;
    }
}

void D3D12Renderer::OnResize(int width, int height) {
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
