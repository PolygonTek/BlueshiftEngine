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

#include "../RHIRenderer.h"
#include "D3D12Common.h"

#ifdef USE_D3D12_MEMALLOC
#include "D3D12MemoryAllocator/D3D12MemAlloc.h"
#endif

#include "D3D12FrameData.h"
#include "../D3D12RenderObject.h"

class D3D12PipelineState;
class D3D12CommandList;
class D3D12DescriptorPool;

#ifdef USE_RENDER_THREAD
enum class FrameSyncState : uint8_t {
    WaitingForUpdateCompleted,          // (렌더 스레드가 렌더링이 완료되어) 메인 스레드의 다음 업데이트 작업이 완료되기를 기다리는 상태
    WaitingForRenderCompleted           // (메인 스레드가 업데이트가 완료되어) 렌더 스레드의 다음 렌더링 작업이 완료되기를 기다리는 상태
};
#endif

struct D3D12PendingResource {
    UINT64                              fenceValue = 0;
    ID3D12Resource *                    resourceToRelease = nullptr;
    RHIRenderer::GPUResource *          resourceToDelete = nullptr;
};

class D3D12Renderer : public RHIRenderer {
public:
    virtual void                        Init(HWND hwnd) override;
    virtual void                        Shutdown() override;

    virtual void                        BeginFrame() override;
    virtual void                        EndFrame() override;
    virtual void                        SwapChainBuffers(bool vsync) override;

    virtual void                        OnResize(int width, int height) override;

    ShaderFormat                        GetShaderFormat() const;

    D3D12CommandList *                  FlushCommandList(D3D12CommandList *commandList);

    uint64_t                            SignalFence();
    bool                                IsFenceComplete(uint64_t checkFenceValue);
    void                                WaitFence(uint64_t expectedFenceValue);
    void                                Finish();

    void                                WaitAllFrameFences();

    void                                MarkForDelete(GPUResource *resource);
    void                                MarkForRelease(ID3D12Resource *resource);
    void                                OnPendingResourceAdded();
    void                                FreePendingResources(bool waitPendings = false);

    virtual Buffer *                    CreateBuffer(BufferUsage usage, int flags, int size) override;
    virtual void                        DestroyBuffer(Buffer *buffer, bool immediate = false) override;

    virtual VertexBuffer *              CreateVertexBuffer(BufferType type, int vertexSize, int numVerts, void *data) override;
    virtual void                        DestroyVertexBuffer(VertexBuffer *vertexBuffer, bool immediate = false) override;

    virtual IndexBuffer *               CreateIndexBuffer(BufferType type, int indexSize, int numIndexes, void *data) override;
    virtual void                        DestroyIndexBuffer(IndexBuffer *indexBuffer, bool immediate = false) override;

    virtual ConstantBuffer *            CreateConstantBuffer(BufferType type, int size, void *data) override;
    virtual void                        DestroyConstantBuffer(ConstantBuffer *constantBuffer, bool immediate = false) override;

    virtual Texture *                   CreateTexture(TextureType textureType, const Image *image) override;
    virtual Texture *                   CreateTexture(TextureType textureType, const Image *image, Image::Format::Enum dstFormat, bool useMipmaps) override;
    virtual Texture *                   CreateTextureFromFile(TextureType textureType, const char *filename, bool useCompression = true, bool useNormalMap = false) override;
    virtual void                        DestroyTexture(Texture *texture, bool immediate = false) override;
    virtual void                        GetTextureImage2D(Texture *texture, int level, Image::Format::Enum imageFormat, void *outPixels) override;
    virtual bool                        SetTextureSubImage2D(Texture *texture, int level, int x, int y, int width, int height, Image::Format::Enum imageFormat, const void *pixels) override;
    virtual bool                        SetTextureSubImage3D(Texture *texture, int level, int x, int y, int z, int width, int height, int depth, Image::Format::Enum imageFormat, const void *pixels) override;

    virtual Shader *                    CreateShader(ShaderModel shaderModel, ShaderStage shaderStage, const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint) override;
    virtual Shader *                    CreateShaderFromFile(ShaderModel shaderModel, ShaderStage shaderStage, const char *filename, const char *entryPoint) override;
    virtual void                        DestroyShader(Shader *shader, bool immediate = false) override;

    virtual Sampler *                   CreateSampler(const SamplerDesc *desc) override;
    virtual void                        DestroySampler(Sampler *sampler, bool immediate = false) override;

    virtual PipelineState *             CreatePSO(const PipelineStateDesc *desc) override;
    virtual void                        DestroyPSO(PipelineState *pipelineState, bool immediate = false) override;

    virtual QueryHeap *                 CreateQueryHeap(const QueryHeapDesc *desc) override;
    virtual void                        DestroyQueryHeap(QueryHeap *queryHeap, bool immediate = false) override;

    virtual void                        SetVertexBuffer(CommandList *commandList, int slot, const VertexBuffer *vertexBuffer) override;
    virtual void                        SetIndexBuffer(CommandList *commandList, const IndexBuffer *indexBuffer) override;
    virtual void                        SetConstantBuffer(CommandList *commandList, int slot, const ConstantBuffer *constantBuffer) override;
    virtual void                        SetConstants(CommandList *commandList, const void *data, uint32_t size, uint32_t offset) override;
    virtual void                        SetTexture(CommandList *commandList, int slot, const Texture *texture) override;
    virtual void                        SetSubResource(CommandList *commandList, int slot, GPUSubResource *subResource) override;
    virtual void                        SetSampler(CommandList *commandList, int slot, Sampler *sampler) override;
    virtual void                        SetPSO(CommandList *commandList, const PipelineState *pipelineState) override;
    virtual void                        SetBlendFactor(CommandList *commandList, const Color4 &rgba) override;
    virtual void                        SetStencilRef(CommandList *commandList, uint32_t value) override;
    virtual void                        SetShadingRate(CommandList *commandList, ShadingRate shadingRate) override;
    virtual void                        SetViewport(CommandList *commandList, const Rect &viewportRect) override;
    virtual void                        SetScissorRect(CommandList *commandList, const Rect &scissorRect) override;
    virtual void                        SetDepthBounds(CommandList *commandList, float depthMin, float depthMax) override;
    virtual void                        BeginQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index) override;
    virtual void                        EndQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index) override;
    virtual void                        ResolveQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index, uint32_t count, const Buffer *destBuffer, uint64_t destOffset) override;
    virtual void                        ResetQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index, uint32_t count) override;

    virtual void                        Draw(CommandList *commandList, uint32_t vertexCount, uint32_t startVertexLocation) override;
    virtual void                        DrawIndexed(CommandList *commandList, uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation) override;
    virtual void                        DrawInstanced(CommandList *commandList, uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) override;
    virtual void                        DrawIndexedInstanced(CommandList *commandList, uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) override;

    PipelineState *                     CreateBasicPSO(ID3D12RootSignature *rootSignature, const D3D12_SHADER_BYTECODE &byteCodeVS, const D3D12_SHADER_BYTECODE &byteCodePS, const D3D12_INPUT_LAYOUT_DESC &inputLayout);
    PipelineState *                     CreateBasicPSO(ID3D12RootSignature *rootSignature, const char *shaderFilename, const D3D12_INPUT_LAYOUT_DESC &inputLayout);
    ID3D12PipelineState *               CreatePSOFromLibrary(const D3D12_PIPELINE_STATE_STREAM_DESC *streamDesc, ID3D12PipelineLibrary1 *library, const TCHAR *name);

    bool                                LoadCachedPSO(const uint64_t hash, ID3DBlob **cachedPSOBlob);
    void                                WriteCachedPSO(const uint64_t hash, ID3DBlob *cachedPSOBlob);

    bool                                CompileShader(const ShaderCompileInput *compileInput, ShaderCompileOutput *compileOutput);
    bool                                CompileShaderD3D(const ShaderCompileInput *compileInput, ShaderCompileOutput *compileOutput);
    bool                                CompileShaderDXC(const ShaderCompileInput *compileInput, ShaderCompileOutput *compileOutput);
    bool                                LoadCompiledShader(const char *name, const uint64_t hash, byte **compiledShaderDataPtr, uint32_t *compiledShaderDataSizePtr);
    void                                WriteCompiledShader(const char *name, const uint64_t hash, const byte *compiledShaderData, uint32_t compiledShaderDataSize);

    void                                CreateDevice(IDXGIAdapter1 **adapterPtr);
    void                                CreateSwapChain(HWND hwnd, int width, int height);
    void                                CreateRTVs();
    void                                CreateDSV(int width, int height);
    void                                CreateShaderCompiler();

#ifdef USE_D3D12_MEMALLOC
    void                                PrintMemoryAllocatorStats();
#endif

    struct DrawObjectTaskDesc {
        D3D12Renderer *                 renderer = nullptr;
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

    ID3D12Device5 *                     device = nullptr;
    IDXGIFactory4 *                     dxgiFactory = nullptr;
    IDXGISwapChain3 *                   dxgiSwapChain = nullptr;
    ID3D12CommandQueue *                commandQueues[to_int(CommandQueueType::Count)] = {};
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

    SharedLib                           dxcompilerLibrary = nullptr;
    Str                                 shaderCacheDir;
    Str                                 psoCacheDir;

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
    Rect                                viewportRect;
    Rect                                scissorRect;
    D3D12DescriptorPool *               cbvDescriptorPool = nullptr;
    D3D12DescriptorPool *               srvDescriptorPool = nullptr;
    D3D12DescriptorPool *               rtvDescriptorPool = nullptr;
    D3D12DescriptorPool *               dsvDescriptorPool = nullptr;
    D3D12DescriptorPool *               samplerDescriptorPool = nullptr;

    IDxcCompiler3 *                     dxcCompiler = nullptr;
    IDxcUtils *                         dxcUtils = nullptr;
    IDxcLibrary *                       dxcLibrary = nullptr;

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

extern D3D12Renderer *                  renderer;
