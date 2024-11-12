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
    BE1::Vec3   position;
    BE1::Vec4   color;
    BE1::Vec2   texCoord;
};

struct DefaultConstantBuffer {
    BE1::Vec4   offset;
};

D3D12App        app;

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

    // 디스크립터 힙 타입 별 디스크립터 사이즈 정보 얻기 (보통은 32바이트 차지)
    descriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    descriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

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

        rtvDescriptorHandle.Offset(1, descriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
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

    descriptorPool = new D3D12DescriptorPool;
    descriptorPool->Init(device, 256);

    InitMesh();

    initialized = true;
}

void D3D12App::Shutdown() {
    Finish();

    FreeMesh();

    delete descriptorPool;

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

void D3D12App::Draw(float t) {
    // 커맨드 리스트 초기화
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, nullptr);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(backBuffersDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex, descriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);

    // 백버퍼 RTV 를 렌더 타겟 상태로 전환
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // RTV 를 파란색으로 Clear
    const float BackColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvDescriptorHandle, BackColor, 0, nullptr);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    commandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, nullptr);

    DrawMesh();

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
}

void D3D12App::RunFrame() {
    BE1::cmdSystem.ExecuteCommandBuffer();
}

void D3D12App::Finish() {
    // 커맨드 큐가 완전히 끝날 때 까지 기다리기 위한 fence 추가
    fenceValue++;
    commandQueue->Signal(fence, fenceValue);

    const UINT64 expectedFenceValue = fenceValue;

    if (fence->GetCompletedValue() < expectedFenceValue) {
        fence->SetEventOnCompletion(expectedFenceValue, fenceEventHandle);
        WaitForSingleObject(fenceEventHandle, INFINITE);
    }
}

ID3D12Resource* D3D12App::CreateVertexBuffer(int vertexSize, int numVerts, void *data, D3D12_VERTEX_BUFFER_VIEW *pOutVertexBufferView) {
    ID3D12Resource* pOutVertexBuffer = nullptr;
    UINT bufferSize = vertexSize * numVerts;
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT; // D3D12_HEAP_TYPE_UPLOAD

    // GPU 에 버텍스 버퍼 생성
    if (FAILED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&pOutVertexBuffer)))) {
        return nullptr;
    }

    ID3D12Resource *pUploadBuffer = nullptr;

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

            UINT8 *mappedPtr = nullptr;
            CD3DX12_RANGE writeRange(0, 0);
            pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            pUploadBuffer->Unmap(0, nullptr);

            // 업로드 버퍼에서 버텍스 버퍼로 데이터 카피
            commandAllocator->Reset();
            commandList->Reset(commandAllocator, nullptr);
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

ID3D12Resource *D3D12App::CreateIndexBuffer(int indexSize, int numIndexes, void *data, D3D12_INDEX_BUFFER_VIEW *pOutIndexBufferView) {
    assert(indexSize == 2 || indexSize == 4);

    ID3D12Resource *pOutIndexBuffer = nullptr;
    UINT bufferSize = indexSize * numIndexes;
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT; // D3D12_HEAP_TYPE_UPLOAD

    // GPU 에 버텍스 버퍼 생성
    if (FAILED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(heapType),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&pOutIndexBuffer)))) {
        return nullptr;
    }

    ID3D12Resource *pUploadBuffer = nullptr;

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

            UINT8 *mappedPtr = nullptr;
            CD3DX12_RANGE writeRange(0, 0);
            pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedPtr));
            memcpy(mappedPtr, data, bufferSize);
            pUploadBuffer->Unmap(0, nullptr);

            // 업로드 버퍼에서 인덱스 버퍼로 데이터 카피
            commandAllocator->Reset();
            commandList->Reset(commandAllocator, nullptr);
            commandList->CopyBufferRegion(pOutIndexBuffer, 0, pUploadBuffer, 0, bufferSize);
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pOutIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));
            commandList->Close();

            // 커맨드 큐 실행
            ID3D12CommandList *ppCommandLists[] = { commandList };
            commandQueue->ExecuteCommandLists(COUNT_OF(ppCommandLists), ppCommandLists);
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

    if (pUploadBuffer) {
        // 업로드 버퍼 사용이 끝날 때 까지 기다린 후 Release 한다.
        Finish();

        pUploadBuffer->Release();
    }

    return pOutIndexBuffer;
}

ID3D12Resource *D3D12App::CreateTexture2D(const BE1::Image *srcImage, BE1::Image::Format::Enum dstFormat, bool useMipmaps) {
    BE1::Image::Format::Enum srcFormat = srcImage->GetFormat();

    bool srcCompressed = BE1::Image::IsCompressed(srcFormat);
    bool dstCompressed = BE1::Image::IsCompressed(dstFormat);

    bool srcFormatSupported = IsSupportedImageFormat(srcFormat);
    bool dstFormatSupported = IsSupportedImageFormat(dstFormat);

    if (!dstFormatSupported) {
        BE_WARNLOG("Unsupported internal image format %s\n", BE1::Image::FormatName(dstFormat));
        return nullptr;
    }

    BE1::Image uncompressedImage;

    if (useMipmaps && srcImage->NumMipmaps() == 1) {
        if (srcImage->IsPacked() || srcImage->IsCompressed()) {
            // 밉맵을 생성해야 한다면, 지원되는 가장 비슷한 무압축 포맷으로 컨버팅한다.
            BE1::Image::Format::Enum supportedUncompressedFormat = ToUncompressedImageFormat(srcFormat);

            srcImage->ConvertFormat(supportedUncompressedFormat, uncompressedImage);
            srcImage = &uncompressedImage;

            srcFormat = supportedUncompressedFormat;
            srcFormatSupported = IsSupportedImageFormat(srcFormat);
            srcCompressed = false;
        }
    }

    BE1::Image mipmapedImage;

    // 밉맵을 직접 생성한다.
    if (useMipmaps && srcImage->NumMipmaps() == 1) {
        int w = srcImage->GetWidth();
        int h = srcImage->GetHeight();
        int d = srcImage->GetDepth();
        int maxGenLevels = BE1::Image::MaxMipMapLevels(w, h, d);

        mipmapedImage.Create(w, h, d, srcImage->NumSlices(), maxGenLevels, srcImage->GetFormat(), srcImage->GetGammaSpace(), nullptr, srcImage->GetFlags());
        mipmapedImage.CopyFrom(*srcImage, 0, 1);
        mipmapedImage.GenerateMipmaps();
        srcImage = &mipmapedImage;
    }

    BE1::Image dstImage;

    // dstFormat 으로 컨버팅
    if (srcFormat != dstFormat) {
        srcImage->ConvertFormat(dstFormat, dstImage);
        srcImage = &dstImage;
    }

    return CreateTexture2D(srcImage);
}

ID3D12Resource *D3D12App::CreateTexture2D(const BE1::Image* srcImage) {
    BE1::Image::Format::Enum srcFormat = srcImage->GetFormat();
    bool isLinearSpace = srcImage->GetGammaSpace() == BE1::Image::GammaSpace::Linear;

    DXGI_FORMAT dxgiFormat;
    bool srcFormatSupported = ImageFormatToDXGIFormat(srcFormat, !isLinearSpace, &dxgiFormat);
    if (!srcFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", BE1::Image::FormatName(srcFormat));
        return nullptr;
    }

    int maxMipLevels = srcImage->NumMipmaps();

    // GPU 에 텍스쳐 리소스 생성
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = static_cast<UINT16>(maxMipLevels);
    textureDesc.Format = dxgiFormat;
    textureDesc.Width = static_cast<UINT>(srcImage->GetWidth());
    textureDesc.Height = static_cast<UINT>(srcImage->GetHeight());
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ID3D12Resource *pOutTexture = nullptr;
    if (FAILED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr, IID_PPV_ARGS(&pOutTexture)))) {
        return nullptr;
    }

    // 텍스쳐 리소스에 write 할 수 있는 (Footprint = 차지하는 공간) 메모리 정보를 얻어온다.
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipFootprints[16];
    UINT64 size;

    device->GetCopyableFootprints(&textureDesc, 0, textureDesc.MipLevels, 0, mipFootprints, nullptr, nullptr, &size);

    // 업로드 버퍼 생성
    ID3D12Resource *pUploadBuffer = nullptr;
    if (FAILED(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(size),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr, IID_PPV_ARGS(&pUploadBuffer)))) {
        pOutTexture->Release();
        return nullptr;
    }

    // 이미지 데이터를 업로드 버퍼에 write
    UINT8 *mappedPtr = nullptr;
    CD3DX12_RANGE writeRange(0, 0);
    pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedPtr));

    byte *dstPtr = mappedPtr;
    int bpp = srcImage->IsCompressed() ? srcImage->BytesPerBlock() : srcImage->BytesPerPixel();

    for (int mipLevel = 0; mipLevel < maxMipLevels; ++mipLevel) {
        int srcWidth = srcImage->GetWidth(mipLevel);
        int srcHeight = srcImage->GetHeight(mipLevel);
        int srcPitch = (srcImage->IsCompressed() ? (srcWidth >> 2) : srcWidth) * bpp;
        int srcRows = srcImage->IsCompressed() ? (srcHeight >> 2) : srcHeight;
        const byte *srcPtr = srcImage->GetPixels(mipLevel);

        while (srcRows--) {
            memcpy(dstPtr, srcPtr, srcPitch);
            srcPtr += srcPitch;
            dstPtr += mipFootprints[mipLevel].Footprint.RowPitch;
        }
    }

    pUploadBuffer->Unmap(0, nullptr);

    // 업로드 버퍼에서 텍스쳐로 데이터 카피
    commandAllocator->Reset();
    commandList->Reset(commandAllocator, nullptr);

    for (int mipLevel = 0; mipLevel < maxMipLevels; ++mipLevel) {
        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.PlacedFootprint = mipFootprints[mipLevel];
        srcLocation.pResource = pUploadBuffer;
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.PlacedFootprint = mipFootprints[mipLevel];
        dstLocation.pResource = pOutTexture;
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = mipLevel;

        commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }

    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pOutTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
    commandList->Close();

    // 커맨드 큐 실행
    ID3D12CommandList *ppCommandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(COUNT_OF(ppCommandLists), ppCommandLists);

    if (pUploadBuffer) {
        // 업로드 버퍼 사용이 끝날 때 까지 기다린 후 Release 한다.
        Finish();

        pUploadBuffer->Release();
    }
    
    return pOutTexture;
}

bool D3D12App::ImageFormatToDXGIFormat(BE1::Image::Format::Enum imageFormat, bool isSRGB, DXGI_FORMAT *dxgiFormat) {
    switch (imageFormat) {
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
    case BE1::Image::Format::Depth_32F:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D32_FLOAT;
        return true;
    case BE1::Image::Format::DepthStencil_24_8:
        if (dxgiFormat) *dxgiFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        return true;
    }
    return false;
}

BE1::Image::Format::Enum D3D12App::ToUncompressedImageFormat(BE1::Image::Format::Enum inFormat) {
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

BE1::Image::Format::Enum D3D12App::ToCompressedImageFormat(BE1::Image::Format::Enum inFormat, bool useNormalMap) {
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

void D3D12App::AdjustTextureFormat(bool useCompression, bool useNormalMap, BE1::Image::Format::Enum inFormat, BE1::Image::Format::Enum *outFormat) {
    if (BE1::Image::IsDepthFormat(inFormat) || BE1::Image::IsDepthStencilFormat(inFormat)) {
        *outFormat = inFormat;
        return;
    }

    if (BE1::Image::IsCompressed(inFormat)) {
        if (IsSupportedImageFormat(inFormat)) {
            *outFormat = inFormat;
            return;
        }

        inFormat = ToUncompressedImageFormat(inFormat);
    }

    *outFormat = useCompression ? ToCompressedImageFormat(inFormat, useNormalMap) : ToUncompressedImageFormat(inFormat);
}

void D3D12App::OnResize(int width, int height) {
    // 기존 백버퍼 해제
    SAFE_RELEASE_ARRAY(backBuffers);

    // 스왑 체인 버퍼의 사이즈를 조정한다.
    swapChain->ResizeBuffers(BackBufferCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(backBuffersDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // 스왑 체인에 연결된 백버퍼로 다시 각각의 RTV 에 연결한다.
    for (UINT backBufferIndex = 0; backBufferIndex < BackBufferCount; ++backBufferIndex) {
        swapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(&backBuffers[backBufferIndex]));
        device->CreateRenderTargetView(backBuffers[backBufferIndex], nullptr, rtvHandle);
        rtvHandle.Offset(1, descriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    }

    currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);

    scissorRect.right = width;
    scissorRect.bottom = height;
}

void D3D12App::InitMesh() {
    // 이미지 파일로 텍스쳐 만들기
    BE1::Image *image = BE1::Image::NewImageFromFile("Data/EngineTextures/checker.dds");
    if (image) {
        BE1::Image::Format::Enum dstFormat;
        AdjustTextureFormat(true, false, image->GetFormat(), &dstFormat);

        defaultTexture = CreateTexture2D(image, dstFormat, true);
        delete image;
    }

    // CB 용 업로드 버퍼 생성
    UINT constantBufferSize = (UINT)BE1::AlignUp(sizeof(DefaultConstantBuffer), 256);
    device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr, IID_PPV_ARGS(&constantBuffer));

    // 디스크립터에 CBV 정보 기록하기
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = constantBufferSize;

    D3D12_CPU_DESCRIPTOR_HANDLE cbvDescriptorHandle;
    descriptorPool->AllocDescriptors(1, &cbvDescriptorHandle, nullptr);
    device->CreateConstantBufferView(&cbvDesc, cbvDescriptorHandle);

    // Map and initialize the constant buffer. We don't unmap this until the
    // app closes. Keeping things mapped for the lifetime of the resource is okay.
    CD3DX12_RANGE writeRange(0, 0); // We do not intend to read from this resource on the CPU.
    constantBuffer->Map(0, &writeRange, reinterpret_cast<void **>(&mappedConstantBase));

    // 디스크립터에 SRV 정보 기록하기
    D3D12_RESOURCE_DESC defaultTextureDesc = defaultTexture->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = defaultTextureDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = defaultTextureDesc.MipLevels;

    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle;
    descriptorPool->AllocDescriptors(1, &srvDescriptorHandle, nullptr);
    device->CreateShaderResourceView(defaultTexture, &srvDesc, srvDescriptorHandle);

    // 루트 시그니쳐를 만들기 위해 우선 Descriptor Range 를 정의한다.
    // TODO: 필요한 루트 시그니쳐를 캐싱하는 방식으로 접근하자.
    // Descriptor Range 는 같은 타입의 Descriptor 여러개를 나타낸다.
    D3D12_DESCRIPTOR_RANGE descriptorRanges[2] = {};

    // constant buffer
    descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    descriptorRanges[0].NumDescriptors = 1;
    descriptorRanges[0].BaseShaderRegister = 0; // b0 부터 시작
    descriptorRanges[0].RegisterSpace = 0;
    // NOTE: D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND 를 주면 Root Signature 를 만들 때,
    // rootParameters[0].DescriptorTable.pDescriptorRanges 의 순서대로 Descriptor Table 의 참조 정보가 만들어진다.
    descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // texture
    descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[1].NumDescriptors = 1;
    descriptorRanges[1].BaseShaderRegister = 0; // t0 부터 시작
    descriptorRanges[1].RegisterSpace = 0;
    descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // Root Parameter 하나 당 Descriptor Table 하나를 참조하게 된다.
    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    // Descriptor Table 하나는 여러개의 Descriptor Range 로 구성된다.
    rootParameters[0].DescriptorTable.NumDescriptorRanges = COUNT_OF(descriptorRanges);
    rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRanges;

    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // Trilinear 필터링
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    samplerDesc.MinLOD = -FLT_MAX;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.NumParameters = COUNT_OF(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 1;
    rootSignatureDesc.pStaticSamplers = &samplerDesc;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob *pSignatureBlob = nullptr;
    ID3DBlob *pErrorBlob = nullptr;

    if (SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignatureBlob, &pErrorBlob))) {
        device->CreateRootSignature(0, pSignatureBlob->GetBufferPointer(), pSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    }

    SAFE_RELEASE(pSignatureBlob);
    SAFE_RELEASE(pErrorBlob);

    // Shader Compile
    const char *shaderText = R"(
struct VSInput {
    float4 position : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

cbuffer CONSTANT_BUFFER_DEFAULT : register(b0) {
    float4 offset;
};

Texture2D defaultTexture : register(t0);
SamplerState defaultSampler : register(s0);

PSInput VSMain(VSInput input) {
    PSInput result = (PSInput)0;

    result.position = input.position;
    result.position.xy += offset.xy;
    result.color = input.color;
    result.texCoord = input.texCoord;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 color = defaultTexture.Sample(defaultSampler, input.texCoord);
    return color * input.color;
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
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // PSO 만들기
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    // NOTE: 나중에 호출할 SetGraphicsRootSignature() 에서 PSO 에 지정된 RootSignature 와 다르면 안된다.
    // 여기서 RootSignature 를 지정하는 이유는 파이프라인 호환성 검사 및 최적화 때문이다.
    psoDesc.pRootSignature = rootSignature;
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
    device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));

    SAFE_RELEASE(pVertexShader);
    SAFE_RELEASE(pPixelShader);

    // 삼각형의 버텍스/인덱스 버퍼 내용을 작성
    // NOTE: UV 좌표의 V 는 아래쪽으로 증가함을 주의한다. 나중에 통합 렌더러를 작성한다면, shader code 에서 하는게 좋을 듯..
    const Vertex3D vertices[] = {
        { { 0.0f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.5f, 0.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
        { { -0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    };

    const uint16_t indexes[] = {
        0, 1, 2
    };

    vertexBuffer = CreateVertexBuffer(sizeof(Vertex3D), 3, (void *)vertices, &vertexBufferView);
    indexBuffer = CreateIndexBuffer(sizeof(uint16_t), 3, (void *)indexes, &indexBufferView);
}

void D3D12App::FreeMesh() {
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(indexBuffer);
    SAFE_RELEASE(rootSignature);
    SAFE_RELEASE(pipelineState);
    SAFE_RELEASE(defaultTexture);
    SAFE_RELEASE(constantBuffer);
}

void D3D12App::DrawMesh() {
    float currentTime = BE1::PlatformTime::Seconds();

    BE1::Vec4* offset = reinterpret_cast<BE1::Vec4*>(mappedConstantBase);
    offset->x = 0.5f * BE1::Math::Cos(currentTime);
    offset->y = 0.5f * BE1::Math::Sin(currentTime * 3);

    ID3D12DescriptorHeap *descriptorHeaps[] = { descriptorPool->descriptorHeap };
    commandList->SetDescriptorHeaps(COUNT_OF(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootSignature(rootSignature);

    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable(descriptorPool->gpuDescriptorHandleForHeapStart);
    commandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

    //gpuDescriptorTable.Offset(1, DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    //pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTable);

    commandList->SetPipelineState(pipelineState);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->IASetIndexBuffer(&indexBufferView);
    commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);
}
