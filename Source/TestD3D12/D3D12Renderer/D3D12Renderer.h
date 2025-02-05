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

#include "../RHI.h"
#include "D3D12Common.h"

#ifdef USE_D3D12_MEMALLOC
#include "D3D12MemoryAllocator/D3D12MemAlloc.h"
#endif

class D3D12Buffer;
class D3D12Texture;
class D3D12PipelineState;
class D3D12CommandList;
class D3D12CommandListPool;
class D3D12DescriptorPool;

struct D3D12PendingResource {
    UINT64                              fenceValue = 0;
    ID3D12Resource *                    resourceToRelease = nullptr;
    RHI::GPUObject *                    objectToDelete = nullptr;
};

class D3D12Renderer : public RHI::Renderer {
public:
    virtual void                        Init(const void *mainWindowHandle) override;
    virtual void                        Shutdown() override;

    RHI::ShaderFormat                   GetShaderFormat() const;

    virtual uint64_t                    SignalFence(RHI::CommandQueueType queueType) override;
    virtual bool                        IsFenceComplete(uint64_t checkFenceValue) override;
    virtual void                        WaitFence(uint64_t expectedFenceValue) override;
    virtual void                        Finish(RHI::CommandQueueType queueType) override;

    void                                MarkForDelete(RHI::GPUObject *object);
    void                                MarkForRelease(ID3D12Resource *resource);
    void                                OnPendingResourceAdded();
    virtual void                        FreePendingResources(bool waitPendings = false) override;

    virtual bool                        IsSupportedImageFormat(BE1::Image::Format imageFormat) const override { return ImageFormatToDXGIFormat(imageFormat, false, nullptr); }
    virtual BE1::Image::Format          ToUncompressedImageFormat(BE1::Image::Format imageFormat) const override;
    virtual BE1::Image::Format          ToCompressedImageFormat(BE1::Image::Format inFormat, bool useNormalMap) const override;

    virtual RHI::FrameThreadData *      CreateFrameThreadData() override;
    virtual void                        DestroyFrameThreadData(RHI::FrameThreadData *frameThreadData) override;

    virtual RHI::SwapChain *            CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height, BE1::Image::Format format) override;
    virtual void                        DestroySwapChain(RHI::SwapChain *swapChain) override;

    virtual RHI::Buffer *               CreateBuffer(RHI::BufferUsage usage, RHI::ResourceFlag flags, uint64_t size, BE1::Image::Format format, uint32_t structuredStride, const void *data) override;
    virtual void                        DestroyBuffer(RHI::Buffer *buffer, bool immediate = false) override;

    virtual RHI::VertexBuffer *         CreateVertexBuffer(RHI::BufferUsage usage, uint32_t vertexSize, uint32_t numVerts, const void *data) override;
    virtual void                        DestroyVertexBuffer(RHI::VertexBuffer *vertexBuffer, bool immediate = false) override;

    virtual RHI::IndexBuffer *          CreateIndexBuffer(RHI::BufferUsage usage, uint32_t indexSize, uint32_t numIndexes, const void *data) override;
    virtual void                        DestroyIndexBuffer(RHI::IndexBuffer *indexBuffer, bool immediate = false) override;

    virtual RHI::ConstantBuffer *       CreateConstantBuffer(RHI::BufferUsage usage, uint32_t size, const void *data) override;
    virtual void                        DestroyConstantBuffer(RHI::ConstantBuffer *constantBuffer, bool immediate = false) override;

    virtual RHI::Texture *              CreateTexture(RHI::TextureType textureType, RHI::ResourceFlag flags, const BE1::Image *image, bool allocateEmptyMipmaps, const RHI::ClearValue &clearValue = {}, uint32_t sampleCount = 1, RHI::GPUResourceState initialState = RHI::GPUResourceState::Undefined) override;
    virtual RHI::Texture *              CreateTexture(RHI::TextureType textureType, RHI::ResourceFlag flags, const BE1::Image *image, BE1::Image::Format dstFormat, bool useMipmaps) override;
    virtual void                        DestroyTexture(RHI::Texture *texture, bool immediate = false) override;

    virtual void                        GetTextureImage2D(RHI::Texture *texture, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) override;
    virtual void                        GetTextureImage3D(RHI::Texture *texture, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) override;
    virtual void                        GetTextureImageCubeFace(RHI::Texture *texture, RHI::CubemapFace face, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) override;

    virtual bool                        SetTextureSubImage2D(RHI::Texture *texture, int mipLevel, int x, int y, int width, int height, BE1::Image::Format imageFormat, const void *pixels) override;
    virtual bool                        SetTextureSubImage3D(RHI::Texture *texture, int mipLevel, int x, int y, int z, int width, int height, int depth, BE1::Image::Format imageFormat, const void *pixels) override;
    virtual bool                        SetTextureSubImageCubeFace(RHI::Texture *texture, RHI::CubemapFace face, int mipLevel, int x, int y, int width, int height, BE1::Image::Format imageFormat, const void *pixels) override;

    void                                GetTextureImage(RHI::Texture *texture, int mipLevel, int sliceIndex, BE1::Image::Format imageFormat, void *outPixels);
    bool                                SetTextureSubImage(RHI::Texture *texture, int mipLevel, int sliceIndex, int x, int y, int z, int width, int height, int depth, BE1::Image::Format imageFormat, const void *pixels);

    virtual int                         CreateSubresource(RHI::Buffer *buffer, RHI::SubresourceType type, uint64_t offset = 0, uint64_t size = ~0, const BE1::Image::Format *newFormat = nullptr) override;
    virtual int                         CreateSubresource(RHI::Texture *texture, RHI::SubresourceType type, uint32_t firstSlice = 0, uint32_t sliceCount = ~0, uint32_t firstMipLevel = 0, uint32_t mipCount = ~0, const BE1::Image::Format *typelessCompatibleFormat = nullptr, bool isSRGB = false) override;

    virtual void                        DestroySubresource(RHI::Buffer *buffer, RHI::SubresourceType type, int subresourceIndex = -1) override;
    virtual void                        DestroySubresource(RHI::Texture *texture, RHI::SubresourceType type, int subresourceIndex = -1) override;

    int                                 CreateSubresourceSRV(D3D12Buffer *buffer, uint64_t offset = 0, uint64_t size = ~0, const BE1::Image::Format *newFormat = nullptr);
    int                                 CreateSubresourceUAV(D3D12Buffer *buffer, uint64_t offset = 0, uint64_t size = ~0, const BE1::Image::Format *newFormat = nullptr);

    int                                 CreateSubresourceSRV(D3D12Texture *texture, uint32_t firstSlice = 0, uint32_t sliceCount = ~0, uint32_t firstMipLevel = 0, uint32_t mipCount = ~0, const BE1::Image::Format *typelessCompatibleFormat = nullptr, bool isSRGB = false);
    int                                 CreateSubresourceRTV(D3D12Texture *texture, uint32_t firstSlice = 0, uint32_t sliceCount = ~0, uint32_t firstMipLevel = 0, const BE1::Image::Format *typelessCompatibleFormat = nullptr, bool isSRGB = false);
    int                                 CreateSubresourceDSV(D3D12Texture *texture, uint32_t firstSlice = 0, uint32_t sliceCount = ~0, uint32_t firstMipLevel = 0, const BE1::Image::Format *typelessCompatibleFormat = nullptr, bool isSRGB = false);
    int                                 CreateSubresourceUAV(D3D12Texture *texture, uint32_t firstSlice = 0, uint32_t sliceCount = ~0, uint32_t firstMipLevel = 0, const BE1::Image::Format *typelessCompatibleFormat = nullptr, bool isSRGB = false);

    virtual RHI::Shader *               CreateShader(RHI::ShaderModel shaderModel, RHI::ShaderStage shaderStage, const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint) override;
    virtual RHI::Shader *               CreateShaderFromFile(RHI::ShaderModel shaderModel, RHI::ShaderStage shaderStage, const char *filename, const char *entryPoint) override;
    virtual void                        DestroyShader(RHI::Shader *shader, bool immediate = false) override;

    virtual RHI::Sampler *              CreateSampler(const RHI::SamplerDesc *desc) override;
    virtual void                        DestroySampler(RHI::Sampler *sampler, bool immediate = false) override;
    virtual void                        RecreateSampler(RHI::Sampler *sampler, const RHI::SamplerDesc *desc) override;

    virtual RHI::PipelineState *        CreateGraphicsPSO(const RHI::PipelineStateDesc *desc) override;
    virtual RHI::PipelineState *        CreateComputePSO(const RHI::Shader *computeShader) override;
    virtual void                        DestroyPSO(RHI::PipelineState *pipelineState, bool immediate = false) override;

    virtual RHI::QueryHeap *            CreateQueryHeap(const RHI::QueryHeapDesc *desc) override;
    virtual void                        DestroyQueryHeap(RHI::QueryHeap *queryHeap, bool immediate = false) override;

    virtual void                        SetVertexBuffer(RHI::CommandList *commandList, int slot, const RHI::VertexBuffer *vertexBuffer) override;
    virtual void                        SetIndexBuffer(RHI::CommandList *commandList, const RHI::IndexBuffer *indexBuffer) override;
    virtual void                        SetConstantBuffer(RHI::CommandList *commandList, int slot, const RHI::ConstantBuffer *constantBuffer) override;
    virtual void                        SetConstants(RHI::CommandList *commandList, const void *data, uint32_t size, uint32_t offset = 0) override;
    virtual void                        SetTexture(RHI::CommandList *commandList, int slot, bool shaderWritable, const RHI::Texture *texture, int subresourceIndex = -1) override;
    virtual void                        SetBuffer(RHI::CommandList *commandList, int slot, bool shaderWritable, const RHI::Buffer *buffer, int subresourceIndex = -1) override;
    virtual void                        SetSampler(RHI::CommandList *commandList, int slot, RHI::Sampler *sampler) override;
    virtual void                        SetPSO(RHI::CommandList *commandList, const RHI::PipelineState *pipelineState) override;
    virtual void                        SetBlendFactor(RHI::CommandList *commandList, const BE1::Color4 &rgba) override;
    virtual void                        SetStencilRef(RHI::CommandList *commandList, uint32_t value) override;
    virtual void                        SetShadingRate(RHI::CommandList *commandList, RHI::ShadingRate shadingRate) override;
    virtual void                        SetViewport(RHI::CommandList *commandList, const BE1::Rect &viewportRect) override;
    virtual void                        SetScissorRect(RHI::CommandList *commandList, const BE1::Rect &scissorRect) override;
    virtual void                        SetDepthBounds(RHI::CommandList *commandList, float depthMin, float depthMax) override;
    virtual void                        BeginQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index) override;
    virtual void                        EndQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index) override;
    virtual void                        ResolveQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index, uint32_t count, const RHI::Buffer *destBuffer, uint64_t destOffset) override;
    virtual void                        ResetQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index, uint32_t count) override;
    virtual void                        ClearUAV(RHI::CommandList *commandList, const RHI::GPUResource *resource, uint32_t value) override;
    virtual void                        CopyBuffer(RHI::CommandList *commandList, const RHI::Buffer *dstBuffer, uint32_t dstOffset, const RHI::Buffer *srcBuffer, uint32_t srcOffset, uint32_t size) override;
    virtual void                        CopyTexture(RHI::CommandList *commandList, const RHI::Texture *dstTexture, uint32_t dstSlice, uint32_t dstMipLevel, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const RHI::Texture *srcTexture, uint32_t srcSlice, uint32_t srcMipLevel, uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t width, uint32_t height, uint32_t depth) override;
    virtual void                        GenerateMipmaps(RHI::CommandList *commandList, const RHI::Texture *texture) override;
    virtual void                        Barrier(RHI::CommandList *commandList, const RHI::GPUBarrier *barriers, uint32_t barrierCount) override;
    virtual void                        ReadPixels(RHI::CommandList *commandList, const RHI::SwapChain *swapChain, int x, int y, int width, int height, BE1::Image::Format dstFormat, void *outPixels) override;
    virtual void                        BeginRenderPass(RHI::CommandList *commandList, const RHI::SwapChain *swapChain, const RHI::Texture *depthStencilTexture, const BE1::Color4 &clearColor = {}, float clearDepth = 0, uint8_t clearStencil = 0, RHI::ClearFlag clearFlags = RHI::ClearFlag::None) override;
    virtual void                        BeginRenderPass(RHI::CommandList *commandList, const RHI::RenderPassImage renderPassImages[], int numRenderPassImages, RHI::RenderPassFlag flags = RHI::RenderPassFlag::None) override;
    virtual void                        EndRenderPass(RHI::CommandList *commandList) override;

    virtual void                        Draw(RHI::CommandList *commandList, uint32_t vertexCount, uint32_t startVertexLocation) override;
    virtual void                        DrawIndexed(RHI::CommandList *commandList, uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation) override;
    virtual void                        DrawInstanced(RHI::CommandList *commandList, uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) override;
    virtual void                        DrawInstancedIndirect(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset) override;
    virtual void                        DrawInstancedIndirectCount(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset, const RHI::Buffer *countBuffer, uint32_t countOffset, uint32_t maxCount) override;
    virtual void                        DrawIndexedInstanced(RHI::CommandList *commandList, uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) override;
    virtual void                        DrawIndexedInstancedIndirect(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset) override;
    virtual void                        DrawIndexedInstancedIndirectCount(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset, const RHI::Buffer *countBuffer, uint32_t countOffset, uint32_t maxCount) override;

    virtual void                        Dispatch(RHI::CommandList *commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) override;
    virtual void                        DispatchIndirect(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset) override;
    virtual void                        DispatchMesh(RHI::CommandList *commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) override;
    virtual void                        DispatchMeshIndirect(RHI::CommandList *commandList, const RHI::Buffer *argsBuffer, uint32_t argsOffset) override;

    virtual void                        SetMarker(RHI::CommandList *commandList, const char *string, uint8_t colorIndex) override;
    virtual void                        BeginEvent(RHI::CommandList *commandList, const char *string, uint8_t colorIndex) override;
    virtual void                        EndEvent(RHI::CommandList *commandList) override;

    void                                InitGenMipmapsPSO();
    void                                FreeGenMipmapsPSO();

    RHI::PipelineState *                CreateBasicPSO(ID3D12RootSignature *rootSignature, const D3D12_SHADER_BYTECODE &byteCodeVS, const D3D12_SHADER_BYTECODE &byteCodePS, const D3D12_INPUT_LAYOUT_DESC &inputLayout);
    RHI::PipelineState *                CreateBasicPSO(ID3D12RootSignature *rootSignature, const char *shaderFilename, const D3D12_INPUT_LAYOUT_DESC &inputLayout);
    ID3D12PipelineState *               CreatePSOFromLibrary(const D3D12_PIPELINE_STATE_STREAM_DESC *streamDesc, ID3D12PipelineLibrary1 *library, const TCHAR *name);

    void                                GetCachedPSOFilename(const uint64_t hash, BE1::Str &outFilename) const;
    bool                                LoadCachedPSO(const uint64_t hash, ID3DBlob **cachedPSOBlob);
    void                                WriteCachedPSO(const uint64_t hash, ID3DBlob *cachedPSOBlob);

    bool                                CompileShader(const RHI::ShaderCompileInput *compileInput, RHI::ShaderCompileOutput *compileOutput);
    bool                                CompileShaderD3D(const RHI::ShaderCompileInput *compileInput, RHI::ShaderCompileOutput *compileOutput);
    bool                                CompileShaderDXC(const RHI::ShaderCompileInput *compileInput, RHI::ShaderCompileOutput *compileOutput);
    bool                                LoadCompiledShader(const char *name, const uint64_t hash, byte **compiledShaderDataPtr, uint32_t *compiledShaderDataSizePtr);
    void                                WriteCompiledShader(const char *name, const uint64_t hash, const byte *compiledShaderData, uint32_t compiledShaderDataSize);

    void                                BindRootParameters(D3D12CommandList *commandList, bool graphics);

    void                                CreateDevice(IDXGIAdapter1 **adapterPtr);
    void                                CreateShaderCompiler();

#ifdef USE_D3D12_MEMALLOC
    void                                PrintMemoryAllocatorStats();
#endif

    static bool                         ImageFormatToDXGIFormat(BE1::Image::Format imageFormat, bool isSRGB, DXGI_FORMAT *dxgiFormat);
    static bool                         DXGIFormatToImageFormat(DXGI_FORMAT dxgiFormat, BE1::Image::Format *imageFormat, bool *isSRGB);
    static bool                         IsDepthFormat(DXGI_FORMAT format);
    static bool                         IsStencilFormat(DXGI_FORMAT format);
    static bool                         IsTypelessFormat(DXGI_FORMAT format);
    static DXGI_FORMAT                  ToTypelessFormat(DXGI_FORMAT format);
    static D3D12_RESOURCE_STATES        ToD3D12ResourceState(RHI::GPUResourceState resourceState);

    static D3D12Renderer *              GetRenderer() { return static_cast<D3D12Renderer *>(RHI::renderer); }

    ID3D12Device5 *                     device = nullptr;
    IDXGIFactory4 *                     dxgiFactory = nullptr;
    ID3D12Fence *                       fence = nullptr;
    uint64_t                            fenceValue = 0;
    HANDLE                              fenceEventHandle = nullptr;
    ID3D12CommandQueue *                commandQueues[to_int(RHI::CommandQueueType::Count)] = {};
    D3D12CommandListPool *              graphicsCommandListPool = nullptr;
    D3D12CommandList *                  resourceCommandList = nullptr;
    ID3D12CommandSignature *            drawInstancedIndirectCommandSignature = nullptr;
    ID3D12CommandSignature *            drawIndexedInstancedIndirectCommandSignature = nullptr;
    ID3D12CommandSignature *            dispatchIndirectCommandSignature = nullptr;
    ID3D12CommandSignature *            dispatchMeshIndirectCommandSignature = nullptr;
#ifdef USE_D3D12_MEMALLOC
    D3D12MA::Allocator *                allocator = nullptr;
#endif

    uint32_t                            vendorId;
    uint32_t                            deviceId;
    BE1::Str                            adapterName;
    uint64_t                            dedicatedVideoMemSize = 0;
    uint64_t                            dedicatedSystemMemSize = 0;
    uint64_t                            sharedSystemMemSize = 0;
    D3D12_RESOURCE_HEAP_TIER            resourceHeapTier = {};
    bool                                supportsTearing = false;
    bool                                supportsConservativeRasterization = false;
    bool                                supportsVRS = false;
    bool                                supportsRayTracing = false;
    bool                                supportsMeshShader = false;
    bool                                supportsDepthBoundsTest = false;
    bool                                supportsCastingFullyTypedFormat = false;
    bool                                supportsUAVFormatCommon = false;
    bool                                supportsUAVFormatRGB_11F_11F_10F = false;

    D3D12DescriptorPool *               resCpuDescriptorPool = nullptr;
    D3D12DescriptorPool *               uavCpuDescriptorPool = nullptr;
    D3D12DescriptorPool *               uavGpuDescriptorPool = nullptr;
    D3D12DescriptorPool *               rtvCpuDescriptorPool = nullptr;
    D3D12DescriptorPool *               dsvCpuDescriptorPool = nullptr;
    D3D12DescriptorPool *               samCpuDescriptorPool = nullptr;

    BE1::SharedLib                      dxcompilerLibrary = nullptr;
    BE1::Str                            shaderCacheDir;
    BE1::Str                            psoCacheDir;

    BE1::HashMap<uint64_t, D3D12PipelineState *> graphicsPsoMap;
    BE1::HashMap<uint64_t, D3D12PipelineState *> computePsoMap;
    BE1::HashMap<uint64_t, ID3DBlob *>  cachedPsoBlobMap;

    IDxcCompiler3 *                     dxcCompiler = nullptr;
    IDxcUtils *                         dxcUtils = nullptr;
    IDxcLibrary *                       dxcLibrary = nullptr;

    D3D12PendingResource *              pendingResourceBuffer = nullptr;
    int                                 maxPendingResources = 0;
    int                                 headPendingIndex = 0;
    int                                 tailPendingIndex = 0;

    RHI::PipelineState *                genMipmaps2DFloat4PSO = nullptr;
    RHI::PipelineState *                genMipmaps2DUNorm4PSO = nullptr;
    RHI::PipelineState *                genMipmapsCubeFloat4PSO = nullptr;
    RHI::PipelineState *                genMipmapsCubeUNorm4PSO = nullptr;
    RHI::PipelineState *                genMipmaps3DFloat4PSO = nullptr;
    RHI::PipelineState *                genMipmaps3DUNorm4PSO = nullptr;
};
