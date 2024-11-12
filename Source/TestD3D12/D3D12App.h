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

class D3D12App {
public:
    void                        Init(HWND windowHandle);
    void                        Shutdown();

    bool                        IsInitialized() const { return initialized; }

    void                        OnResize(int width, int height);

    void                        Draw(float t);

    void                        RunFrame();

    void                        Finish();

    ID3D12Resource *            CreateVertexBuffer(int vertexSize, int numVerts, void *data, D3D12_VERTEX_BUFFER_VIEW *pOutVertexBufferView);
    ID3D12Resource *            CreateIndexBuffer(int indexSize, int numIndexes, void *data, D3D12_INDEX_BUFFER_VIEW *pOutIndexBufferView);

    ID3D12Resource *            CreateTexture2D(const BE1::Image *image, BE1::Image::Format::Enum dstFormat, bool useMipmaps);
    ID3D12Resource *            CreateTexture2D(const BE1::Image *image);

    static bool                 ImageFormatToDXGIFormat(BE1::Image::Format::Enum imageFormat, bool isSRGB, DXGI_FORMAT* dxgiFormat);
    static bool                 IsSupportedImageFormat(BE1::Image::Format::Enum imageFormat) { return ImageFormatToDXGIFormat(imageFormat, false, nullptr); }
    static BE1::Image::Format::Enum ToUncompressedImageFormat(BE1::Image::Format::Enum imageFormat);
    static BE1::Image::Format::Enum ToCompressedImageFormat(BE1::Image::Format::Enum inFormat, bool useNormalMap);
    static void                 AdjustTextureFormat(bool useCompression, bool useNormalMap, BE1::Image::Format::Enum inFormat, BE1::Image::Format::Enum *outFormat);

    ID3D12Device5 *             device = nullptr;

private:
    void                        InitMesh();
    void                        FreeMesh();
    void                        DrawMesh();

    static constexpr UINT       BackBufferCount = 2;

    DXGI_ADAPTER_DESC1          adapterDesc = {};
    ID3D12CommandQueue *        commandQueue = nullptr;
    ID3D12CommandAllocator *    commandAllocator = nullptr;
    ID3D12GraphicsCommandList * commandList = nullptr;
    ID3D12DescriptorHeap *      backBuffersDescriptorHeap = nullptr;
    IDXGISwapChain3 *           swapChain = nullptr;
    UINT                        descriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    ID3D12Resource *            backBuffers[BackBufferCount] = {};
    HANDLE                      fenceEventHandle = nullptr;
    ID3D12Fence *               fence = nullptr;
    UINT64                      fenceValue = 0;
    UINT                        currentBackBufferIndex = 0;
    D3D12_VIEWPORT              viewport = {};
    D3D12_RECT                  scissorRect = {};

    D3D12DescriptorPool *       descriptorPool = nullptr;

    ID3D12Resource *            defaultTexture = nullptr;
    ID3D12Resource *            constantBuffer = nullptr;
    void *                      mappedConstantBase = nullptr;

    ID3D12RootSignature *       rootSignature = nullptr;
    ID3D12PipelineState *       pipelineState = nullptr;

    ID3D12Resource *            vertexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW    vertexBufferView = {};
    ID3D12Resource *            indexBuffer = nullptr;
    D3D12_INDEX_BUFFER_VIEW     indexBufferView = {};

    bool                        initialized = false;
};

extern D3D12App                 app;
