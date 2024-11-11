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

#define SAFE_RELEASE(p)         if (p) { p->Release(); p = nullptr; }
#define SAFE_RELEASE_ARRAY(p)   for (int i = 0; i < _countof(p); i++) if (p[i]) { (p[i])->Release(); (p[i]) = nullptr; }

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

    bool                        ImageFormatToDXGIFormat(BE1::Image::Format::Enum imageFormat, bool isSRGB, DXGI_FORMAT* dxgiFormat) const;
    bool                        IsSupportedImageFormat(BE1::Image::Format::Enum imageFormat) const { return ImageFormatToDXGIFormat(imageFormat, false, nullptr); }
    BE1::Image::Format::Enum    ToSupportedUncompressedFormat(BE1::Image::Format::Enum imageFormat);

private:
    void                        InitMesh();
    void                        FreeMesh();
    void                        DrawMesh();

    static constexpr UINT       backBufferCount = 2;

    ID3D12Device5 *             pD3DDevice = nullptr;
    ID3D12CommandQueue *        pCommandQueue = nullptr;
    ID3D12CommandAllocator *    pCommandAllocator = nullptr;
    ID3D12GraphicsCommandList * pCommandList = nullptr;
    DXGI_ADAPTER_DESC1          adapterDesc;
    ID3D12DescriptorHeap *      pRTVDescriptorHeap = nullptr;
    IDXGISwapChain3 *           pSwapChain = nullptr;
    UINT                        DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    ID3D12Resource *            pBackBuffers[backBufferCount] = {};
    HANDLE                      hFenceEvent = nullptr;
    ID3D12Fence *               pFence = nullptr;
    UINT64                      fenceValue = 0;
    UINT                        currentBackBufferIndex = 0;
    D3D12_VIEWPORT              viewport = {};
    D3D12_RECT                  scissorRect = {};

    ID3D12Resource *            defaultTexture = nullptr;
    ID3D12Resource *            pConstantBuffer = nullptr;
    ID3D12DescriptorHeap *      meshDescriptorHeap = nullptr;
    ID3D12RootSignature *       pRootSignature = nullptr;
    ID3D12PipelineState *       pPipelineState = nullptr;
    ID3D12Resource *            pVertexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW    vertexBufferView;
    ID3D12Resource *            pIndexBuffer = nullptr;
    D3D12_INDEX_BUFFER_VIEW     indexBufferView;

    void *                      mappedConstantBase;

    bool                        initialized = false;
};

extern D3D12App                 app;
