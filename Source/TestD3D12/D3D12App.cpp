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
#include "D3D12App.h"
#include <dxgidebug.h>

D3D12App app;

void D3D12App::Init(HWND hwnd) {
    DWORD dwCreateFactoryFlags = 0;
    bool bWithGPUValidation = true;

    // 디버그 레이어 활성화
    ID3D12Debug *pDebugController = nullptr;
    HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController));
    if (SUCCEEDED(hr)) {
        pDebugController->EnableDebugLayer();
        dwCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

        // GPU Validation 활성화
        if (bWithGPUValidation) {
            ID3D12Debug1*pDebugController1;
            pDebugController->QueryInterface(IID_PPV_ARGS(&pDebugController1));
            pDebugController1->SetEnableGPUBasedValidation(TRUE);
            pDebugController1->Release();
        }
        pDebugController->Release();
    }

    IDXGIFactory4 *pFactory = nullptr;
    CreateDXGIFactory2(dwCreateFactoryFlags, IID_PPV_ARGS(&pFactory));

    // 어댑터 정보 얻어오기
    IDXGIAdapter1 *pAdapter = nullptr;
    pFactory->EnumAdapters1(0, &pAdapter);
    pAdapter->GetDesc1(&adapterDesc);

    // D3D12 디바이스 생성
    hr = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&pD3DDevice));
    if (FAILED(hr)) {
        BE_FATALERROR("D3D12CreateDevice : failed");
    }
    pAdapter->Release();

    // 디버그 표시 정보 설정
    ID3D12InfoQueue *pInfoQueue = nullptr;
    pD3DDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
    if (pInfoQueue) {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

        D3D12_MESSAGE_ID hide[] = {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // Workarounds for debug layer issues on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = (UINT)COUNT_OF(hide);
        filter.DenyList.pIDList = hide;
        pInfoQueue->AddStorageFilterEntries(&filter);
        pInfoQueue->Release();
    }

    // 디스크립터 힙 타입 별 디스크립터 사이즈 정보 얻기
    DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // 커맨드 큐 생성
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandQueue : failed");
    }

    // 두개의 버퍼를 갖는 스왑 체인 생성
    RECT rc;
    GetClientRect(hwnd, &rc);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = rc.right;
    swapChainDesc.Height = rc.bottom;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
    //swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = {};
    swapChainFullscreenDesc.Windowed = TRUE;

    IDXGISwapChain1 *pSwapChain1 = nullptr;
    hr = pFactory->CreateSwapChainForHwnd(pCommandQueue, hwnd, &swapChainDesc, &swapChainFullscreenDesc, nullptr, &pSwapChain1);
    if (FAILED(hr)) {
        BE_FATALERROR("CreateSwapChainForHwnd : failed");
    }
    pSwapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain));
    pSwapChain1->Release();
    pFactory->Release();

    // 렌더타겟 용 디스크립터 힙 생성 (렌더 타겟 2개)
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = 2;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = pD3DDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&pRTVDescriptorHeap));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateDescriptorHeap : failed");
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 스왑 체인의 버퍼 (백버퍼, 프론트버퍼) 를 가져와서 각 RTV 에 연결한다.
    for (UINT renderTargetIndex = 0; renderTargetIndex < 2; ++renderTargetIndex) {
        pSwapChain->GetBuffer(renderTargetIndex, IID_PPV_ARGS(&pRenderTargets[renderTargetIndex]));
        pD3DDevice->CreateRenderTargetView(pRenderTargets[renderTargetIndex], nullptr, rtvDescriptorHandle);
        rtvDescriptorHandle.Offset(1, DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    }

    // 그래픽스 커맨드 리스트를 위한 커맨드 할당자 생성
    hr = pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandAllocator : failed");
    }

    // 그래픽스 커맨드 리스트 생성
    hr = pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandList : failed");
    }

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    pCommandList->Close();

    // Fence 객체 생성
    hr = pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateFence : failed");
    }
    // Fence 초기값
    fenceValue = 0;

    // Fence 를 대기하기 위한 이벤트 객체 생성
    hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // 현재 백버퍼 인덱스 초기화
    currentBackBufferIndex = pSwapChain->GetCurrentBackBufferIndex();
}

void D3D12App::Shutdown() {
    Finish();

    if (pRTVDescriptorHeap) {
        pRTVDescriptorHeap->Release();
        pRTVDescriptorHeap = nullptr;
    }
    for (int i = 0; i < COUNT_OF(pRenderTargets); ++i) {
        if (pRenderTargets[i]) {
            pRenderTargets[i]->Release();
            pRenderTargets[i] = nullptr;
        }
    }
    if (pSwapChain) {
        pSwapChain->Release();
        pSwapChain = nullptr;
    }
    if (pCommandList) {
        pCommandList->Release();
        pCommandList = nullptr;
    }
    if (pCommandAllocator) {
        pCommandAllocator->Release();
        pCommandAllocator = nullptr;
    }
    if (pCommandQueue) {
        pCommandQueue->Release();
        pCommandQueue = nullptr;
    }
    if (hFenceEvent) {
        CloseHandle(hFenceEvent);
        hFenceEvent = nullptr;
    }
    if (pFence) {
        pFence->Release();
        pFence = nullptr;
    }

    ULONG refCount = pD3DDevice->Release();
    if (refCount > 0) {
        IDXGIDebug1 *pDebug = nullptr;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug)))) {
            pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
            pDebug->Release();
        }
        BE1::PlatformSystem::DebugBreak();
    }
}

void D3D12App::Draw(float t) {
    // 커맨드 리스트 초기화
    pCommandAllocator->Reset();
    pCommandList->Reset(pCommandAllocator, nullptr);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex, DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);

    // 백버퍼 RTV 를 렌더 타겟 상태로 전환
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargets[currentBackBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // RTV 를 파란색으로 Clear
    const float BackColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
    pCommandList->ClearRenderTargetView(rtvDescriptorHandle, BackColor, 0, nullptr);

    // 백버퍼 RTV 를 Present 할 수 있는 상태로 전환
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pRenderTargets[currentBackBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // 커맨드 리스트 기록을 마친다.
    pCommandList->Close();

    // 커맨드 큐에 커맨드 리스트 전달
    ID3D12CommandList *ppCommandLists[] = { pCommandList };
    pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 백버퍼를 전면버퍼와 교환한다.
    HRESULT hr = pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    if (hr == DXGI_ERROR_DEVICE_REMOVED) {
        BE_FATALERROR("DXGI Device Removed");
    }

    // 다음 프레임에 사용할 백버퍼 인덱스
    currentBackBufferIndex = pSwapChain->GetCurrentBackBufferIndex();

    // 커맨드 큐 실행이 다 끝날 때까지 기다린다.
    Finish();
}

void D3D12App::RunFrame() {
    BE1::cmdSystem.ExecuteCommandBuffer();
}

void D3D12App::Finish() {
    // 커맨드 큐가 완전히 끝날 때 까지 기다리기 위한 fence 추가
    fenceValue++;
    pCommandQueue->Signal(pFence, fenceValue);

    const UINT64 expectedFenceValue = fenceValue;

    if (pFence->GetCompletedValue() < expectedFenceValue) {
        pFence->SetEventOnCompletion(expectedFenceValue, hFenceEvent);
        WaitForSingleObject(hFenceEvent, INFINITE);
    }
}
