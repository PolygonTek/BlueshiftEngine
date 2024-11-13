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

void D3D12Renderer::Init(HWND hwnd) {
    DWORD dwCreateFactoryFlags = 0;
    bool bWithGPUValidation = true;

    // 디버그 레이어 활성화
    ID3D12Debug* pDebugController = nullptr;
    HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController));
    if (SUCCEEDED(hr)) {
        pDebugController->EnableDebugLayer();
        dwCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

        // GPU Validation 활성화
        if (bWithGPUValidation) {
            ID3D12Debug1* pDebugController1 = nullptr;
            if (SUCCEEDED(pDebugController->QueryInterface(IID_PPV_ARGS(&pDebugController1))))
            {
                pDebugController1->SetEnableGPUBasedValidation(TRUE);
                pDebugController1->Release();
            }
        }
        pDebugController->Release();
    }

    IDXGIFactory4* pFactory = nullptr;
    CreateDXGIFactory2(dwCreateFactoryFlags, IID_PPV_ARGS(&pFactory));

    // 어댑터 정보 얻어오기
    IDXGIAdapter1* pAdapter = nullptr;
    pFactory->EnumAdapters1(0, &pAdapter);
    pAdapter->GetDesc1(&adapterDesc);

    // D3D12 디바이스 생성
    hr = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device));
    if (FAILED(hr)) {
        BE_FATALERROR("D3D12CreateDevice : failed");
    }
    pAdapter->Release();

    // 디버그 표시 정보 설정
    ID3D12InfoQueue *pInfoQueue = nullptr;
    device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
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
    swapChainDesc.BufferCount = BackBufferCount;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = {};
    swapChainFullscreenDesc.Windowed = TRUE;

    IDXGISwapChain1 *pSwapChain1 = nullptr;
    hr = pFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, &swapChainFullscreenDesc, nullptr, &pSwapChain1);
    if (FAILED(hr)) {
        BE_FATALERROR("CreateSwapChainForHwnd : failed");
    }
    pSwapChain1->QueryInterface(IID_PPV_ARGS(&swapChain));
    pSwapChain1->Release();
    pFactory->Release();

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

    // 렌더타겟 용 디스크립터 힙 생성 (렌더 타겟 2개)
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.NumDescriptors = BackBufferCount;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&backBuffersDescriptorHeap));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateDescriptorHeap : failed");
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(backBuffersDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 스왑 체인의 버퍼 (백버퍼, 프론트버퍼) 를 가져와서 각 RTV 에 연결한다.
    for (UINT renderTargetIndex = 0; renderTargetIndex < BackBufferCount; ++renderTargetIndex) {
        swapChain->GetBuffer(renderTargetIndex, IID_PPV_ARGS(&backBuffers[renderTargetIndex]));

        device->CreateRenderTargetView(backBuffers[renderTargetIndex], nullptr, rtvDescriptorHandle);

        rtvDescriptorHandle.Offset(1, descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    }

    // 그래픽스 커맨드 리스트를 위한 커맨드 할당자 생성
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateCommandAllocator : failed");
    }

    // 그래픽스 커맨드 리스트 생성
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
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

    // 렌더링에 사용할 디스크립터 힙을 생성한다.
    // 최대 1000 개의 CBV_SRV_UAV 용 디스크립터를 담을 수 있다.
    rootDescriptorPool = new D3D12DescriptorPool;
    rootDescriptorPool->Init(1000);

    singleDescriptorAllocator = new D3D12SingleDescriptorAllocator;
    singleDescriptorAllocator->Init(10000);

    initialized = true;
}

void D3D12Renderer::Shutdown() {
    Finish();

    SAFE_DELETE(rootDescriptorPool);
    SAFE_DELETE(singleDescriptorAllocator);

    SAFE_RELEASE(backBuffersDescriptorHeap);
    SAFE_RELEASE_ARRAY(backBuffers);
    SAFE_RELEASE(swapChain);
    SAFE_RELEASE(commandList);
    SAFE_RELEASE(commandAllocator);
    SAFE_RELEASE(commandQueue);
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
        BE1::PlatformSystem::DebugBreak();
    }
}

void D3D12Renderer::BeginRender() {
    // 커맨드 리스트 초기화
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, nullptr);

    // 백버퍼 RTV 를 렌더 타겟 상태로 전환
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(backBuffersDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex, descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);

    // RTV 를 파란색으로 Clear
    const float BackColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvDescriptorHandle, BackColor, 0, nullptr);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    commandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);
}

void D3D12Renderer::EndRender() {
    // 백버퍼 RTV 를 Present 할 수 있는 상태로 전환
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // 커맨드 리스트 기록을 마친다.
    commandList->Close();

    // 커맨드 큐에 커맨드 리스트 전달
    ID3D12CommandList *ppCommandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 백버퍼를 전면버퍼와 교환한다.
    if (swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING) == DXGI_ERROR_DEVICE_REMOVED) {
        BE_FATALERROR("DXGI Device Removed");
    }

    // 다음 프레임에 사용할 백버퍼 인덱스 초기화
    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

    // 커맨드 큐 실행이 다 끝날 때까지 기다린다.
    Finish();

    // 루트 디스크립터 풀을 비운다.
    rootDescriptorPool->Reset();
}

void D3D12Renderer::Finish() {
    // 커맨드 큐가 완전히 끝날 때 까지 기다리기 위한 fence 추가
    fenceValue++;
    commandQueue->Signal(fence, fenceValue);

    const UINT64 expectedFenceValue = fenceValue;

    if (fence->GetCompletedValue() < expectedFenceValue) {
        fence->SetEventOnCompletion(expectedFenceValue, fenceEventHandle);
        WaitForSingleObject(fenceEventHandle, INFINITE);
    }
}

ID3D12Resource*D3D12Renderer::CreateVertexBuffer(int vertexSize, int numVerts, void *data, D3D12_VERTEX_BUFFER_VIEW *pOutVertexBufferView) {
    ID3D12Resource* pOutVertexBuffer = nullptr;
    UINT bufferSize = vertexSize * numVerts;
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT; // D3D12_HEAP_TYPE_UPLOAD

    // GPU 에 버텍스 버퍼 생성
    if (FAILED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&pOutVertexBuffer)))) {
        return nullptr;
    }

    ID3D12Resource* pUploadBuffer = nullptr;

    if (data) {
        if (heapType == D3D12_HEAP_TYPE_DEFAULT) {
            // CPU 에서 GPU 로 업로드할 버텍스 버퍼 생성
            if (FAILED(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&pUploadBuffer)))) {
                pOutVertexBuffer->Release();
                return nullptr;
            }

            UINT8* mappedPtr = nullptr;
            CD3DX12_RANGE writeRange(0, 0);
            pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            pUploadBuffer->Unmap(0, nullptr);

            // 업로드 버퍼에서 버텍스 버퍼로 데이터 카피
            commandAllocator->Reset();
            commandList->Reset(commandAllocator, nullptr);
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pOutVertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
            commandList->CopyBufferRegion(pOutVertexBuffer, 0, pUploadBuffer, 0, bufferSize);
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pOutVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
            commandList->Close();

            // 커맨드 큐 실행
            ID3D12CommandList *ppCommandLists[] = { commandList };
            commandQueue->ExecuteCommandLists(COUNT_OF(ppCommandLists), ppCommandLists);
        } else if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
            UINT8 *mappedPtr = nullptr;
            CD3DX12_RANGE readRange(0, 0);
            pOutVertexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            pOutVertexBuffer->Unmap(0, nullptr);
        } else {
            return nullptr;
        }
    }

    pOutVertexBufferView->BufferLocation = pOutVertexBuffer->GetGPUVirtualAddress();
    pOutVertexBufferView->StrideInBytes = vertexSize;
    pOutVertexBufferView->SizeInBytes = bufferSize;

    if (pUploadBuffer) {
        // 업로드 버퍼 사용이 끝날 때 까지 기다린 후 Release 한다.
        Finish();

        pUploadBuffer->Release();
    }

    return pOutVertexBuffer;
}

ID3D12Resource *D3D12Renderer::CreateIndexBuffer(int indexSize, int numIndexes, void *data, D3D12_INDEX_BUFFER_VIEW *pOutIndexBufferView) {
    assert(indexSize == 2 || indexSize == 4);

    ID3D12Resource* pOutIndexBuffer = nullptr;
    UINT bufferSize = indexSize * numIndexes;
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT; // D3D12_HEAP_TYPE_UPLOAD

    // GPU 에 버텍스 버퍼 생성
    if (FAILED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&pOutIndexBuffer)))) {
        return nullptr;
    }

    ID3D12Resource* pUploadBuffer = nullptr;

    if (data) {
        if (heapType == D3D12_HEAP_TYPE_DEFAULT) {
            // CPU 에서 GPU 로 업로드할 버텍스 버퍼 생성
            if (FAILED(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&pUploadBuffer)))) {
                pOutIndexBuffer->Release();
                return nullptr;
            }

            UINT8* mappedPtr = nullptr;
            CD3DX12_RANGE writeRange(0, 0);
            pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            pUploadBuffer->Unmap(0, nullptr);

            // 업로드 버퍼에서 인덱스 버퍼로 데이터 카피
            commandAllocator->Reset();
            commandList->Reset(commandAllocator, nullptr);
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pOutIndexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
            commandList->CopyBufferRegion(pOutIndexBuffer, 0, pUploadBuffer, 0, bufferSize);
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pOutIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));
            commandList->Close();

            // 커맨드 큐 실행
            ID3D12CommandList *ppCommandLists[] = { commandList };
            commandQueue->ExecuteCommandLists(COUNT_OF(ppCommandLists), ppCommandLists);
        } else if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
            UINT8* mappedPtr = nullptr;
            CD3DX12_RANGE readRange(0, 0);
            pOutIndexBuffer->Map(0, &readRange, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            pOutIndexBuffer->Unmap(0, nullptr);
        } else {
            return nullptr;
        }
    }

    pOutIndexBufferView->BufferLocation = pOutIndexBuffer->GetGPUVirtualAddress();
    pOutIndexBufferView->Format = (indexSize == sizeof(uint16_t) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
    pOutIndexBufferView->SizeInBytes = bufferSize;

    if (pUploadBuffer) {
        // 업로드 버퍼 사용이 끝날 때 까지 기다린 후 Release 한다.
        Finish();

        pUploadBuffer->Release();
    }

    return pOutIndexBuffer;
}

void D3D12Renderer::OnResize(int width, int height) {
    // 기존 백버퍼 해제
    SAFE_RELEASE_ARRAY(backBuffers);

    // 스왑 체인 버퍼의 사이즈를 조정한다.
    swapChain->ResizeBuffers(BackBufferCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(backBuffersDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 스왑 체인에 연결된 백버퍼로 다시 각각의 RTV 에 연결한다.
    for (UINT backBufferIndex = 0; backBufferIndex < BackBufferCount; ++backBufferIndex) {
        swapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(&backBuffers[backBufferIndex]));
        device->CreateRenderTargetView(backBuffers[backBufferIndex], nullptr, rtvHandle);
        rtvHandle.Offset(1, descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    }

    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);

    scissorRect.right = width;
    scissorRect.bottom = height;
}
