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

#pragma once

#include "D3D12DescriptorPool.h"
#include "D3D12Texture.h"

class D3D12Renderer {
public:
    void                        Init(HWND hwnd);
    void                        Shutdown();

    bool                        IsInitialized() const { return initialized; }

    void                        BeginRender();
    void                        EndRender();

    void                        OnResize(int width, int height);

    ID3D12Resource *            CreateVertexBuffer(int vertexSize, int numVerts, void *data, D3D12_VERTEX_BUFFER_VIEW *pOutVertexBufferView);
    ID3D12Resource *            CreateIndexBuffer(int indexSize, int numIndexes, void *data, D3D12_INDEX_BUFFER_VIEW *pOutIndexBufferView);

    void                        Finish();

    static constexpr UINT       BackBufferCount = 2;

    ID3D12Device5 *             device = nullptr;
    DXGI_ADAPTER_DESC1          adapterDesc = {};
    ID3D12CommandQueue *        commandQueue = nullptr;
    ID3D12CommandAllocator *    commandAllocator = nullptr;
    ID3D12GraphicsCommandList * commandList = nullptr;
    ID3D12DescriptorHeap *      backBuffersDescriptorHeap = nullptr;
    IDXGISwapChain3 *           swapChain = nullptr;
    UINT                        descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    ID3D12Resource *            backBuffers[BackBufferCount] = {};
    HANDLE                      fenceEventHandle = nullptr;
    ID3D12Fence *               fence = nullptr;
    UINT64                      fenceValue = 0;
    UINT                        currentBackBufferIndex = 0;
    D3D12_VIEWPORT              viewport = {};
    D3D12_RECT                  scissorRect = {};
    D3D12DescriptorPool *       rootDescriptorPool = nullptr;
    bool                        initialized = false;
};

extern D3D12Renderer            renderer;
