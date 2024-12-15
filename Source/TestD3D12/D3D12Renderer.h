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

#include "RHIRenderer.h"
#include "D3D12Common.h"

#ifdef USE_D3D12_MEMALLOC
#include "D3D12MemoryAllocator/D3D12MemAlloc.h"
#endif

#include "D3D12FrameData.h"
#include "D3D12RenderObject.h"
#include "D3D12CompiledShaderBlob.h"

class D3D12CommandList;
class D3D12DescriptorPool;

class D3D12Buffer : public RHIRenderer::Buffer {
public:
    D3D12Buffer() = default;
    virtual ~D3D12Buffer() { Release(); }

    void                                Release();

    ID3D12Resource *                    GetResource();
    uint64_t                            GetSize();

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocation *               bufferAllocation = nullptr;
#else
    ID3D12Resource *                    bufferResource = nullptr;
#endif
};

class D3D12VertexBuffer : public RHIRenderer::VertexBuffer {
public:
    virtual ~D3D12VertexBuffer() { Release(); }

    void                                Release() { SAFE_DELETE(buffer); }

    D3D12Buffer *                       buffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW            vbv = {};
};

class D3D12IndexBuffer : public RHIRenderer::IndexBuffer {
public:
    virtual ~D3D12IndexBuffer() { Release(); }

    void                                Release() { SAFE_DELETE(buffer); }

    D3D12Buffer *                       buffer = nullptr;
    D3D12_INDEX_BUFFER_VIEW             ibv = {};
};

class D3D12ConstantBuffer : public RHIRenderer::ConstantBuffer {
public:
    virtual ~D3D12ConstantBuffer() { Release(); }

    void                                Release() { SAFE_DELETE(buffer); }

    D3D12Buffer *                       buffer = nullptr;
};

class D3D12Texture : public RHIRenderer::Texture {
public:
    virtual ~D3D12Texture() { Release(); }

    void                                Release();

    static void                         AdjustTextureFormat(bool useCompression, bool useNormalMap, Image::Format::Enum inFormat, Image::Format::Enum *outFormat);

#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocation *               textureAllocation = nullptr;
#else
    ID3D12Resource *                    textureResource = nullptr;
#endif
    D3D12_RESOURCE_DESC                 textureDesc;
    D3D12_CPU_DESCRIPTOR_HANDLE         descriptorHandle = {0};
};

class D3D12Shader : public RHIRenderer::Shader {
public:
    virtual ~D3D12Shader() { Release(); }

    void                                Release();

    uint64_t                            hash = 0;
    ID3DBlob *                          compiledShaderBlob = nullptr;
    ID3D12RootSignature *               rootSignature = nullptr;
};

class D3D12PipelineState : public RHIRenderer::PipelineState {
public:
    virtual ~D3D12PipelineState() { Release(); }

    void                                Release();

    struct PipelineStateStream1 {
        CD3DX12_PIPELINE_STATE_STREAM_FLAGS flags;
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
    };

    struct PipelineStateStream2 {
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blendDesc;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 depthStencil;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT depthStencilFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rasterizer;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS renderTargetFormats;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC sampleDesc;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK sampleMask;
    };

    struct PipelineStateStream {
        PipelineStateStream1 stream1;
        CD3DX12_PIPELINE_STATE_STREAM_VS vs;
        CD3DX12_PIPELINE_STATE_STREAM_GS gs;
        CD3DX12_PIPELINE_STATE_STREAM_HS hs;
        CD3DX12_PIPELINE_STATE_STREAM_DS ds;
        CD3DX12_PIPELINE_STATE_STREAM_PS ps;
        PipelineStateStream2 stream2;
    };

    struct CachedPipelineStateStream {
        PipelineStateStream1 stream1;
        PipelineStateStream2 stream2;
        CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO shaderCachedPSO;
    };

    ID3D12PipelineState *               pso = nullptr;
    ID3D12RootSignature *               rootSignature = nullptr;
};

#ifdef USE_RENDER_THREAD
enum class FrameSyncState : uint8_t {
    WaitingForUpdateCompleted,          // (렌더 스레드가 렌더링이 완료되어) 메인 스레드의 다음 업데이트 작업이 완료되기를 기다리는 상태
    WaitingForRenderCompleted           // (메인 스레드가 업데이트가 완료되어) 렌더 스레드의 다음 렌더링 작업이 완료되기를 기다리는 상태
};
#endif

enum class D3D12CommandQueueType : uint8_t {
    Graphics,
    Compute,
    Count
};

struct D3D12PendingResource {
    UINT64                              fenceValue = 0;
    ID3D12Resource *                    resourceToRelease = nullptr;
    RHIRenderer::Resource *             resourceToDelete = nullptr;
};

class D3D12Renderer : public RHIRenderer {
public:
    virtual void                        Init(HWND hwnd) override;
    virtual void                        Shutdown() override;

    void                                BeginFrame();
    void                                EndFrame();
    void                                SwapChainBuffers(bool vsync);

    void                                OnResize(int width, int height);

    void                                CreateDevice(IDXGIAdapter1 **adapterPtr);
    void                                CreateSwapChain(HWND hwnd, int width, int height);
    void                                CreateRTVs();
    void                                CreateDSV(int width, int height);

    D3D12CommandList *                  FlushCommandList(D3D12CommandList *commandList);

    uint64_t                            SignalFence();
    bool                                IsFenceComplete(uint64_t checkFenceValue);
    void                                WaitFence(uint64_t expectedFenceValue);
    void                                Finish();

    void                                WaitAllFrameFences();

    void                                MarkForRelease(ID3D12Resource *resource);
    void                                MarkForDelete(Resource *resource);
    void                                OnPendingResourceAdded();
    void                                FreePendingResources(bool waitPendings = false);

    Buffer *                            CreateBuffer(BufferUsage usage, int size);

    VertexBuffer *                      CreateVertexBuffer(BufferType type, int vertexSize, int numVerts, void *data);
    void                                DestroyVertexBuffer(VertexBuffer *vertexBuffer, bool immediate = false);

    IndexBuffer *                       CreateIndexBuffer(BufferType type, int indexSize, int numIndexes, void *data);
    void                                DestroyIndexBuffer(IndexBuffer *indexBuffer, bool immediate = false);

    ConstantBuffer *                    CreateConstantBuffer(BufferType type, int size, void *data);
    void                                DestroyConstantBuffer(ConstantBuffer *constantBuffer, bool immediate = false);

    Texture *                           CreateTexture(TextureType textureType, const Image *image);
    Texture *                           CreateTexture(TextureType textureType, const Image *image, Image::Format::Enum dstFormat, bool useMipmaps);
    Texture *                           CreateTextureFromFile(TextureType textureType, const char *filename, bool useCompression = true, bool useNormalMap = false);
    void                                DestroyTexture(Texture *texture, bool immediate = false);

    void                                GetTextureImage2D(Texture *texture, int level, Image::Format::Enum imageFormat, void *outPixels);
    bool                                SetTextureSubImage2D(Texture *texture, int level, int x, int y, int width, int height, Image::Format::Enum imageFormat, const void *pixels);
    bool                                SetTextureSubImage3D(Texture *texture, int level, int x, int y, int z, int width, int height, int depth, Image::Format::Enum imageFormat, const void *pixels);

    Shader *                            CreateShader(ShaderStage shaderStage, const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint);
    Shader *                            CreateShaderFromFile(ShaderStage shaderStage, const char *filename, const char *entryPoint);
    void                                DestroyShader(Shader *shader, bool immediate = false);

    PipelineState *                     CreatePSO(RHIRenderer::PipelineStateDesc *desc);
    void                                DestroyPSO(PipelineState *pipelineState, bool immediate = false);

    PipelineState *                     CreateBasicPSO(ID3D12RootSignature *rootSignature, const D3D12_SHADER_BYTECODE &byteCodeVS, const D3D12_SHADER_BYTECODE &byteCodePS, const D3D12_INPUT_LAYOUT_DESC &inputLayout);
    PipelineState *                     CreateBasicPSO(ID3D12RootSignature *rootSignature, const char *shaderFilename, const D3D12_INPUT_LAYOUT_DESC &inputLayout);
    ID3D12PipelineState *               CreatePSOFromLibrary(const D3D12_PIPELINE_STATE_STREAM_DESC *streamDesc, ID3D12PipelineLibrary1 *library, const TCHAR *name);

    bool                                LoadCachedPSO(const uint64_t hash, ID3DBlob **cachedPSOBlob);
    void                                WriteCachedPSO(const uint64_t hash, ID3DBlob *cachedPSOBlob);

    bool                                LoadCompiledShader(const char *name, const uint64_t hash, ID3DBlob **compiledShaderBlob);
    void                                WriteCompiledShader(const char *name, const uint64_t hash, ID3DBlob *compiledShaderBlob);

    void                                PrintCompileErrorMessages(ID3DBlob *errorBlob);

#ifdef USE_D3D12_MEMALLOC
    void                                PrintMemoryAllocatorStats();
#endif

    struct DrawObjectTaskDesc {
        int                             threadIndex = -1;
        int                             visObjectStartIndex = -1;
        int                             visObjectEndIndex = -1;
        D3D12CommandList *              activeCommandList = nullptr;
    };

    int                                 AddRenderObject(const D3D12RenderObject::State &def);
    void                                UpdateRenderObject(int handle, const D3D12RenderObject::State &def);
    void                                RemoveRenderObject(int handle);

    void                                RenderScene();

    void                                RenderFrame();
    void                                DrawVisObjects(int threadIndex, D3D12CommandList *commandList, int startIndex, int endIndex);
    void                                DrawVisObjectsWithoutTask();
#ifdef USE_RENDER_TASK
    void                                DrawVisObjectsWithTask(int numTasks);
    void                                DrawVisObjectsByTask(D3D12Renderer::DrawObjectTaskDesc *taskDesc);
#endif

    static bool                         ImageFormatToDXGIFormat(Image::Format::Enum imageFormat, bool isSRGB, DXGI_FORMAT *dxgiFormat);
    static bool                         DXGIFormatToImageFormat(DXGI_FORMAT dxgiFormat, Image::Format::Enum *imageFormat, bool *isSRGB);
    static bool                         IsSupportedImageFormat(Image::Format::Enum imageFormat) { return ImageFormatToDXGIFormat(imageFormat, false, nullptr); }
    static Image::Format::Enum          ToUncompressedImageFormat(Image::Format::Enum imageFormat);
    static Image::Format::Enum          ToCompressedImageFormat(Image::Format::Enum inFormat, bool useNormalMap);

    static constexpr int                NumSwapChainBuffers = 3;

    static Str                          shaderCacheDir;
    static Str                          psoCacheDir;

    ID3D12Device5 *                     device = nullptr;
    IDXGIFactory4 *                     dxgiFactory = nullptr;
    IDXGISwapChain3 *                   dxgiSwapChain = nullptr;
    ID3D12CommandQueue *                commandQueues[to_int(D3D12CommandQueueType::Count)] = {};
    D3D12CommandListPool *              commandListPool = nullptr;
    D3D12CommandList *                  resourceCommandList = nullptr;
    ID3D12Fence *                       fence = nullptr;
    uint64_t                            fenceValue = 0;
    HANDLE                              fenceEventHandle = nullptr;
#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocator *                allocator = nullptr;
#endif
    UINT                                descriptorHandleSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    ID3D12DescriptorHeap *              rtvDescriptorHeap = nullptr;
    ID3D12DescriptorHeap *              dsvDescriptorHeap = nullptr;
    ID3D12Resource *                    renderTargetBuffers[NumSwapChainBuffers] = {};
    ID3D12Resource *                    depthStencilBuffer = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE         rtvDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE();
    D3D12_CPU_DESCRIPTOR_HANDLE         dsvDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE();

    HashMap<uint64_t, D3D12PipelineState *> psoMap;
    HashMap<uint64_t, ID3DBlob *>       cachedPsoBlobMap;

    uint32_t                            vendorId;
    uint32_t                            deviceId;
    Str                                 adapterName;
    uint64_t                            dedicatedVideoMemSize = 0;
    uint64_t                            dedicatedSystemMemSize = 0;
    uint64_t                            sharedSystemMemSize = 0;
    bool                                supportsTearing = false;
    bool                                supportsConservativeRasterization = false;
    bool                                supportsVRS = false;
    bool                                supportsRayTracing = false;
    bool                                supportsMeshShader = false;
    bool                                supportsDepthBoundsTest = false;

    UINT                                currentBackBufferIndex = 0;
    D3D12_VIEWPORT                      viewport = {};
    D3D12_RECT                          scissorRect = {};
    D3D12DescriptorPool *               srvDescriptorPool = nullptr;
    D3D12DescriptorPool *               rtvDescriptorPool = nullptr;
    D3D12DescriptorPool *               dsvDescriptorPool = nullptr;
    D3D12DescriptorPool *               samplerDescriptorPool = nullptr;

    UINT                                frameCount = 0;
    D3D12FrameData                      frameData[NumFrameResources];
    D3D12FrameData *                    currentFrameData = nullptr;
    UINT                                currentFrameIndex = 0;

    D3D12PendingResource *              pendingResourceBuffer = nullptr;
    int                                 maxPendingResources = 0;
    int                                 headPendingIndex = 0;
    int                                 tailPendingIndex = 0;

    Array<D3D12RenderObject *>          renderObjects;
    Array<DrawObjectTaskDesc>           objectDrawingTaskDescs;

#ifdef USE_RENDER_TASK
    TaskManager                         renderTaskManager = TaskManager(MaxRenderTasks);
#endif

#ifdef USE_RENDER_THREAD
    friend unsigned int                 RenderThreadProc(void *param);

    void                                InitRenderThread();
    void                                ShutdownRenderThread();
    void                                WaitRenderCompleted();

    PlatformSRWLock *                   smpLock = nullptr;
    PlatformCondition *                 renderCompletedCondition = nullptr;
    PlatformCondition *                 updateCompletedCondition = nullptr;
    PlatformThread *                    renderThread = nullptr;
    bool                                isStoppingRenderThread = false;
    int                                 renderFrameIndex = 1;
    FrameSyncState                      frameSyncState = FrameSyncState::WaitingForUpdateCompleted;
#endif
};

extern D3D12Renderer                    renderer;
