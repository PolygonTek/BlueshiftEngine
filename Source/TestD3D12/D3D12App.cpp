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

// D3D12.dll 이 D3D12Core.dll 을 찾기 위한 설정
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }
extern "C" { __declspec(dllexport) extern const char *D3D12SDKPath = u8"."; }

struct Vertex3D {
    DirectX::XMFLOAT3   position;
    DirectX::XMFLOAT4   color;
};

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
            ID3D12Debug1 *pDebugController1 = nullptr;
            if (SUCCEEDED(pDebugController->QueryInterface(IID_PPV_ARGS(&pDebugController1))))
            {
                pDebugController1->SetEnableGPUBasedValidation(TRUE);
                pDebugController1->Release();
            }
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
    swapChainDesc.BufferCount = backBufferCount;
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
    descriptorHeapDesc.NumDescriptors = backBufferCount;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = pD3DDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&pRTVDescriptorHeap));
    if (FAILED(hr)) {
        BE_FATALERROR("CreateDescriptorHeap : failed");
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 스왑 체인의 버퍼 (백버퍼, 프론트버퍼) 를 가져와서 각 RTV 에 연결한다.
    for (UINT renderTargetIndex = 0; renderTargetIndex < backBufferCount; ++renderTargetIndex) {
        pSwapChain->GetBuffer(renderTargetIndex, IID_PPV_ARGS(&pBackBuffers[renderTargetIndex]));
        pD3DDevice->CreateRenderTargetView(pBackBuffers[renderTargetIndex], nullptr, rtvDescriptorHandle);
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

    // Root Signature 만들기
    // NOTE: 현재는 Input Assembler 에서 Input Layout 을 사용할 수 있다라는 정보 밖에 없다.
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.NumParameters = 0;
    rootSignatureDesc.pParameters = nullptr;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob *pSignatureBlob = nullptr;
    ID3DBlob *pErrorBlob = nullptr;

    if (SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignatureBlob, &pErrorBlob))) {
        pD3DDevice->CreateRootSignature(0, pSignatureBlob->GetBufferPointer(), pSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature));
    }

    SAFE_RELEASE(pSignatureBlob);
    SAFE_RELEASE(pErrorBlob);

    // Shader Compile
    const char *shaderText = R"(
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
})";

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ID3DBlob *pVertexShader = nullptr;
    D3DCompile(shaderText, strlen(shaderText), "shaderText", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &pVertexShader, nullptr);

    ID3DBlob *pPixelShader = nullptr;
    D3DCompile(shaderText, strlen(shaderText), "shaderText", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pPixelShader, nullptr);

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // PSO 만들기
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    // NOTE: 나중에 호출할 SetGraphicsRootSignature() 에서 PSO 에 지정된 RootSignature 와 다르면 안된다.
    // 여기서 RootSignature 를 지정하는 이유는 파이프라인 호환성 검사 및 최적화 때문이다.
    psoDesc.pRootSignature = pRootSignature;
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShader->GetBufferPointer(), pVertexShader->GetBufferSize());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShader->GetBufferPointer(), pPixelShader->GetBufferSize());
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.InputLayout = { inputElementDescs, COUNT_OF(inputElementDescs) };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pPipelineState));

    SAFE_RELEASE(pVertexShader);
    SAFE_RELEASE(pPixelShader);

    // Create the vertex buffer.
    // Define the geometry for a triangle.
    const Vertex3D vertices[] = {
        { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };

    const uint16_t indexes[] = {
        0, 1, 2
    };

    pVertexBuffer = CreateVertexBuffer(sizeof(Vertex3D), 3, (void*)vertices, &vertexBufferView);
    pIndexBuffer = CreateIndexBuffer(sizeof(uint16_t), 3, (void *)indexes, &indexBufferView);

    initialized = true;
}

void D3D12App::Shutdown() {
    Finish();

    SAFE_RELEASE(pVertexBuffer);
    SAFE_RELEASE(pIndexBuffer);
    SAFE_RELEASE(pRootSignature);
    SAFE_RELEASE(pPipelineState);
    SAFE_RELEASE(pRTVDescriptorHeap);
    SAFE_RELEASE_ARRAY(pBackBuffers);
    SAFE_RELEASE(pSwapChain);
    SAFE_RELEASE(pCommandList);
    SAFE_RELEASE(pCommandAllocator);
    SAFE_RELEASE(pCommandQueue);
    SAFE_RELEASE(pFence);

    if (hFenceEvent) {
        CloseHandle(hFenceEvent);
        hFenceEvent = nullptr;
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
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBackBuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // RTV 를 파란색으로 Clear
    const float BackColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
    pCommandList->ClearRenderTargetView(rtvDescriptorHandle, BackColor, 0, nullptr);

    pCommandList->RSSetViewports(1, &viewport);
    pCommandList->RSSetScissorRects(1, &scissorRect);
    pCommandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

    // 삼각형 그리기
    pCommandList->SetGraphicsRootSignature(pRootSignature);
    pCommandList->SetPipelineState(pPipelineState);
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    pCommandList->IASetIndexBuffer(&indexBufferView);
    pCommandList->DrawIndexedInstanced(3, 1, 0, 0, 0);

    // 백버퍼 RTV 를 Present 할 수 있는 상태로 전환
    pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pBackBuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // 커맨드 리스트 기록을 마친다.
    pCommandList->Close();

    // 커맨드 큐에 커맨드 리스트 전달
    ID3D12CommandList *ppCommandLists[] = { pCommandList };
    pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 백버퍼를 전면버퍼와 교환한다.
    if (pSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING) == DXGI_ERROR_DEVICE_REMOVED) {
        BE_FATALERROR("DXGI Device Removed");
    }

    // 다음 프레임에 사용할 백버퍼 인덱스 초기화
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

ID3D12Resource* D3D12App::CreateVertexBuffer(int vertexSize, int numVerts, void *data, D3D12_VERTEX_BUFFER_VIEW *pOutVertexBufferView)
{
    ID3D12Resource* pOutVertexBuffer = nullptr;
    UINT bufferSize = vertexSize * numVerts;
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT; // D3D12_HEAP_TYPE_UPLOAD
    D3D12_RESOURCE_STATES initialBufferState = D3D12_RESOURCE_STATE_COMMON; // D3D12_RESOURCE_STATE_GENERIC_READ;

    // GPU 에 버텍스 버퍼 생성
    if (FAILED(pD3DDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
        initialBufferState,
        nullptr, IID_PPV_ARGS(&pOutVertexBuffer)))) {
        return nullptr;
    }

    ID3D12Resource *pUploadBuffer = nullptr;

    if (data) {
        if (heapType == D3D12_HEAP_TYPE_DEFAULT) {
            // CPU 에서 GPU 로 업로드할 버텍스 버퍼 생성
            if (FAILED(pD3DDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                D3D12_RESOURCE_STATE_COMMON,
                nullptr, IID_PPV_ARGS(&pUploadBuffer)))) {
                pOutVertexBuffer->Release();
                return nullptr;
            }

            UINT8 *mappedPtr = nullptr;
            CD3DX12_RANGE writeRange(0, 0);
            pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            pUploadBuffer->Unmap(0, nullptr);

            // 데이터 카피
            pCommandAllocator->Reset();
            pCommandList->Reset(pCommandAllocator, nullptr);
            pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pOutVertexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
            pCommandList->CopyBufferRegion(pOutVertexBuffer, 0, pUploadBuffer, 0, bufferSize);
            pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pOutVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
            pCommandList->Close();

            // 커맨드 큐 실행
            ID3D12CommandList *ppCommandLists[] = { pCommandList };
            pCommandQueue->ExecuteCommandLists(COUNT_OF(ppCommandLists), ppCommandLists);
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

    if (pUploadBuffer)
    {
        // 업로드 버퍼 사용이 끝날 때 까지 기다린 후 Release 한다.
        Finish();

        pUploadBuffer->Release();
    }

    return pOutVertexBuffer;
}

ID3D12Resource *D3D12App::CreateIndexBuffer(int indexSize, int numIndexes, void *data, D3D12_INDEX_BUFFER_VIEW *pOutIndexBufferView)
{
    assert(indexSize == 2 || indexSize == 4);

    ID3D12Resource *pOutIndexBuffer = nullptr;
    UINT bufferSize = indexSize * numIndexes;
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT; // D3D12_HEAP_TYPE_UPLOAD
    D3D12_RESOURCE_STATES initialBufferState = D3D12_RESOURCE_STATE_COMMON; // D3D12_RESOURCE_STATE_GENERIC_READ;

    // GPU 에 버텍스 버퍼 생성
    if (FAILED(pD3DDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
        initialBufferState,
        nullptr, IID_PPV_ARGS(&pOutIndexBuffer)))) {
        return nullptr;
    }

    ID3D12Resource *pUploadBuffer = nullptr;

    if (data) {
        if (heapType == D3D12_HEAP_TYPE_DEFAULT) {
            // CPU 에서 GPU 로 업로드할 버텍스 버퍼 생성
            if (FAILED(pD3DDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
                D3D12_RESOURCE_STATE_COMMON,
                nullptr, IID_PPV_ARGS(&pUploadBuffer)))) {
                pOutIndexBuffer->Release();
                return nullptr;
            }

            UINT8 *mappedPtr = nullptr;
            CD3DX12_RANGE writeRange(0, 0);
            pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            pUploadBuffer->Unmap(0, nullptr);

            // 데이터 카피
            pCommandAllocator->Reset();
            pCommandList->Reset(pCommandAllocator, nullptr);
            pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pOutIndexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
            pCommandList->CopyBufferRegion(pOutIndexBuffer, 0, pUploadBuffer, 0, bufferSize);
            pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pOutIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));
            pCommandList->Close();

            // 커맨드 큐 실행
            ID3D12CommandList *ppCommandLists[] = { pCommandList };
            pCommandQueue->ExecuteCommandLists(COUNT_OF(ppCommandLists), ppCommandLists);
        } else if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
            UINT8 *mappedPtr = nullptr;
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

    if (pUploadBuffer)
    {
        // 업로드 버퍼 사용이 끝날 때 까지 기다린 후 Release 한다.
        Finish();

        pUploadBuffer->Release();
    }

    return pOutIndexBuffer;
}

void D3D12App::OnResize(int width, int height)
{
    // 기존 백버퍼 해제
    SAFE_RELEASE_ARRAY(pBackBuffers);

    // 스왑 체인 버퍼의 사이즈를 조정한다.
    pSwapChain->ResizeBuffers(backBufferCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 스왑 체인에 연결된 백버퍼로 다시 각각의 RTV 에 연결한다.
    for (UINT backBufferIndex = 0; backBufferIndex < backBufferCount; ++backBufferIndex)
    {
        pSwapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(&pBackBuffers[backBufferIndex]));
        pD3DDevice->CreateRenderTargetView(pBackBuffers[backBufferIndex], nullptr, rtvHandle);
        rtvHandle.Offset(1, DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    }

    currentBackBufferIndex = pSwapChain->GetCurrentBackBufferIndex();

    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);

    scissorRect.right = width;
    scissorRect.bottom = height;
}
