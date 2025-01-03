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

#include "Image/Image.h"

struct RHI {
    enum class ClearFlag : uint32_t {
        None                            = 0,
        Color                           = BIT(0),
        Depth                           = BIT(1),
        Stencil                         = BIT(2)
    };

    enum class FillMode : uint8_t {
        Wire,
        Solid
    };

    enum class CullMode : uint8_t {
        Back,
        Front,
        None
    };

    enum class DepthWriteMask : uint8_t {
        Zero,
        All
    };

    enum class ColorWriteMask : uint8_t {
        Red                             = BIT(0),
        Green                           = BIT(1),
        Blue                            = BIT(2),
        Alpha                           = BIT(3),
        All                             = Red | Green | Blue | Alpha,
    };

    enum class ComparisonFunc : uint8_t {
        Always,
        Less,
        LEqual,
        Equal,
        NotEqual,
        GEqual,
        Greater,
        Never
    };

    enum class Blend : uint8_t {
        Zero,
        One,
        SrcColor,
        InvSrcColor,
        SrcAlpha,
        InvSrcAlpha,
        SrcAlphaSat,
        DestColor,
        InvDestColor,
        DestAlpha,
        InvDestAlpha,
        BlendFactor,
        InvBlendFactor,
        Src1Color,
        InvSrc1Color,
        Src1Alpha,
        InvSrc1Alpha
    };

    enum class BlendOp : uint8_t {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    enum class StencilOp : uint8_t {
        Keep,
        Zero,
        Replace,
        Incr,
        IncrWrap,
        Decr,
        DecrWrap,
        Invert
    };

    enum class TextureFilter : uint8_t {
        NearestMipmapNearest,   // Min/Mag/Mip : Point
        LinearMipmapNearest,    // Min/Mag : Linear, Mip : Point
        NearestMipmapLinear,    // Min/Mag : Point, Mip : Linear
        LinearMipmapLinear,     // Min/Mag/Mip : Linear
        Anisotropic
    };

    enum class TextureAddressMode : uint8_t {
        Repeat,
        MirroredRepeat,
        Clamp,
        ClampToBorder
    };

    enum class TextureBorderColor : uint8_t {
        TransparentBlack,
        OpaqueBlack,
        OpaqueWhite
    };

    enum class PrimitiveTopology : uint8_t {
        Undefined,
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        PatchList
    };

    enum class BufferUsage : uint8_t {
        Default,
        Upload,
        Readback
    };

    enum class ResourceFlag : uint32_t {
        None                            = 0,
        ConstantBuffer                  = BIT(0),
        VertexBuffer                    = BIT(1),
        IndexBuffer                     = BIT(2),
        ShaderResource                  = BIT(3),
        RenderTarget                    = BIT(4),
        DepthStencil                    = BIT(5),
        UnorderedAccess                 = BIT(6),
        Typeless                        = BIT(7),
        SkipDefaultViews                = BIT(8)
    };

    enum class BufferType : uint8_t {
        Static,
        Dynamic
    };

    enum class TextureType : uint8_t {
        Texture1D,
        Texture1DArray,
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube,
        TextureCubeArray
    };

    enum class SubresourceType : uint8_t {
        SRV,        // Shader Resource View
        UAV,        // Unordered Access View
        RTV,        // Render Target View
        DSV         // Depth Stencil View
    };

    enum class ShaderFormat : uint8_t {
        None,
        HLSL5,      // DXBC (Shader Model 5.0)
        HLSL6,      // DXIL (Shader Model 5.1 이상)
        SPIRV       // SPIR-V
    };

    enum class ShaderModel : uint8_t {
        SM_5_0,
        SM_6_0,
        SM_6_1,
        SM_6_2,
        SM_6_3,
        SM_6_4,
        SM_6_5,
        SM_6_6,
        SM_6_7,
    };

    enum class ShaderStage : uint8_t {
        Vertex,
        Hull,
        Domain,
        Geometry,
        Fragment,
        Compute,
        Count
    };

    enum class ShadingRate : uint8_t {
        InvalidRate,
        Rate1X1,
        Rate1X2,
        Rate2X1,
        Rate2X2,
        Rate2X4,
        Rate4X2,
        Rate4X4
    };

    struct SamplerDesc {
        TextureFilter                   filter = TextureFilter::NearestMipmapNearest;
        TextureAddressMode              addressModeU = TextureAddressMode::Clamp;
        TextureAddressMode              addressModeV = TextureAddressMode::Clamp;
        TextureAddressMode              addressModeW = TextureAddressMode::Clamp;
        float                           mipLodBias = 0.0f;
        uint32_t                        maxAnisotropy = 0;
        TextureBorderColor              borderColor = TextureBorderColor::OpaqueBlack;
        float                           minLod = 0.0f;
        float                           maxLod = std::numeric_limits<float>::max();
    };

    struct ShaderCompileInput {
        ShaderFormat                    shaderFormat;
        ShaderModel                     shaderModel;
        ShaderStage                     shaderStage;
        uint32_t                        shaderTextSize;
        const char *                    sourceName;
        const char *                    shaderText;
        const char *                    entryPoint;
    };

    struct ShaderCompileOutput {
        byte *                          compiledShaderData = nullptr;
        uint32_t                        compiledShaderDataSize = 0;
        BE1::Str                        errorMessage;
    };

    struct RasterizerState {
        FillMode                        fillMode = FillMode::Solid;
        CullMode                        cullMode = CullMode::None;
        int32_t                         depthBias = 0;
        float                           depthBiasClamp = 0;
        float                           slopeScaledDepthBias = 0;
        bool                            depthClipEnabled = false;
        bool                            smoothLineEnabled = false;
        bool                            conservativeRasterization = false;
    };

    struct DepthStencilOp {
        StencilOp                       failOp = StencilOp::Keep;
        StencilOp                       depthFailOp = StencilOp::Keep;
        StencilOp                       passOp = StencilOp::Keep;
        ComparisonFunc                  stencilFunc = ComparisonFunc::Never;
    };

    struct DepthStencilState {
        bool                            depthTestEnabled = false;
        DepthWriteMask                  depthWriteMask = DepthWriteMask::Zero;
        ComparisonFunc                  depthFunc = ComparisonFunc::Never;
        bool                            stencilTestEnabled = false;
        uint8_t                         stencilReadMask = 0xff;
        uint8_t                         stencilWriteMask = 0xff;
        DepthStencilOp                  frontFace;
        DepthStencilOp                  backFace;
        bool                            depthBoundTestEnabled = false;
    };

    struct RenderTargetBlendState {
        bool                            blendEnabled = false;
        Blend                           srcFactorColor = Blend::SrcAlpha;
        Blend                           destFactorColor = Blend::InvSrcAlpha;
        BlendOp                         blendOpColor = BlendOp::Add;
        Blend                           srcFactorAlpha = Blend::One;
        Blend                           destFactorAlpha = Blend::One;
        BlendOp                         blendOpAlpha = BlendOp::Add;
        ColorWriteMask                  colorWriteMask = ColorWriteMask::All;
    };

    struct BlendState {
        bool                            alphaToCoverageEnabled = false;
        bool                            independentBlendEnabled = false;
        RenderTargetBlendState          renderTargets[8];
    };

    struct InputLayoutElement {
        enum class Format : uint8_t {
            Unknown,
            Float4,
            Float3,
            Float2,
            Float1,
            UInt4,
            UInt3,
            UInt2,
            UInt1,
            Int4,
            Int3,
            Int2,
            Int1,
            Half4,
            Half2,
            Half1,
            UShort4,
            UShort2,
            UShort1,
            UShort4N,
            UShort2N,
            UShort1N,
            Short4,
            Short2,
            Short1,
            Short4N,
            Short2N,
            Short1N,
            UByte4,
            UByte2,
            UByte1,
            UByte4N,
            UByte2N,
            UByte1N,
            Byte4,
            Byte2,
            Byte1,
            Byte4N,
            Byte2N,
            Byte1N
        };

        BE1::Str                        semanticName;
        uint32_t                        semanticIndex = 0;
        uint32_t                        offset = 0;
        uint32_t                        inputSlot = 0;
        Format                          format = Format::Unknown;
    };

    struct InputLayout {
        BE1::Array<InputLayoutElement>  elements;
    };

    enum class QueryType : uint8_t {
        TimeStamp,
        Occlusion,
        OcclusionBinary
    };

    struct QueryHeapDesc {
        QueryType                       type = QueryType::TimeStamp;
        uint32_t                        queryCount = 0;
    };

    class GPUObject {
    public:
        virtual ~GPUObject() = default;
    };

    class GPUResource : public GPUObject {
    public:
        virtual void *                  GetNativeResource() const = 0;
        virtual const void *            GetNativeBufferObject() const { return nullptr; }
        virtual const void *            GetNativeTextureObject() const { return nullptr; }
    };

    class Buffer : public GPUResource {
    public:
        void *                          writePtr = nullptr;
        BufferUsage                     bufferUsage;
        ResourceFlag                    flags = ResourceFlag::None;
    };

    class VertexBuffer : public GPUResource {
    public:
        void *                          writePtr = nullptr;
        BufferUsage                     bufferUsage;
    };

    class IndexBuffer : public GPUResource {
    public:
        void *                          writePtr = nullptr;
        BufferUsage                     bufferUsage;
    };

    class ConstantBuffer : public GPUResource {
    public:
        void *                          writePtr = nullptr;
        BufferUsage                     bufferUsage;
    };

    class Texture : public GPUResource {
    public:
        TextureType                     textureType;
    };

    class Shader : public GPUObject {
    public:
        ShaderStage                     shaderStage = ShaderStage::Count;
    };

    class Sampler : public GPUObject {
    public:
        SamplerDesc                     desc = {};
    };

    class PipelineState : public GPUObject {
    public:
        uint64_t                        hash = 0;
        bool                            graphics;
    };

    class QueryHeap : public GPUObject {
    public:
        QueryHeapDesc                   desc = {};
    };

    enum class GPUResourceState : uint32_t {
        Undefined                       = 0,
        ShaderResource                  = BIT(0),
        ShaderResourceCompute           = BIT(1),
        UnorderedAccess                 = BIT(2),
        CopySrc                         = BIT(3),
        CopyDst                         = BIT(4),
        RenderTarget                    = BIT(5),
        DepthWrite                      = BIT(6),
        DepthRead                       = BIT(7),
        ShadingRateSource               = BIT(8),
        VertexBuffer                    = BIT(9),
        IndexBuffer                     = BIT(10),
        ConstantBuffer                  = BIT(11),
        IndirectArgument                = BIT(12),
        RTAccelerationStructure         = BIT(13),
        Prediction                      = BIT(14)
    };

    class GPUBarrier {
    public:
        enum class Type : uint8_t {
            Memory,     // UAV 메모리 배리어
            Buffer,     // 버퍼의 상태 전이 배리어
            Image,      // 텍스쳐의 상태 전이 배리어
            Aliasing
        };

        struct MemoryBarrier {
            const GPUResource *         resource;
        };

        struct BufferBarrier {
            const Buffer *buffer;
            GPUResourceState            stateBefore;
            GPUResourceState            stateAfter;
        };

        struct ImageBarrier {
            const Texture *             texture;
            GPUResourceState            stateBefore;
            GPUResourceState            stateAfter;
            int                         slice;
            int                         mipLevel;
        };

        struct AliasingBarrier {
            const GPUResource *         resourceBefore;
            const GPUResource *         resourceAfter;
        };

        Type                            type;
        union {
            MemoryBarrier               memoryBarrier;
            BufferBarrier               bufferBarrier;
            ImageBarrier                imageBarrier;
            AliasingBarrier             aliasingBarrier;
        };
    };

    struct RenderDest {
        uint32_t                        renderTargetCount = 0;
        BE1::Image::Format::Enum        renderTargetFormats[8] = {};
        BE1::Image::Format::Enum        depthStencilFormat = BE1::Image::Format::Unknown;
        uint32_t                        sampleCount = 1;

        constexpr uint64_t              GetHash() const {
            union Hasher {
                struct {
                    uint64_t renderTargetFormat_0 : 6;
                    uint64_t renderTargetFormat_1 : 6;
                    uint64_t renderTargetFormat_2 : 6;
                    uint64_t renderTargetFormat_3 : 6;
                    uint64_t renderTargetFormat_4 : 6;
                    uint64_t renderTargetFormat_5 : 6;
                    uint64_t renderTargetFormat_6 : 6;
                    uint64_t renderTargetFormat_7 : 6;
                    uint64_t depthStencilFormat : 6;
                } bits;
                uint64_t value;
            } hasher = {};
            static_assert(sizeof(Hasher) == sizeof(uint64_t));
            hasher.bits.renderTargetFormat_0 = (uint64_t)renderTargetFormats[0];
            hasher.bits.renderTargetFormat_1 = (uint64_t)renderTargetFormats[1];
            hasher.bits.renderTargetFormat_2 = (uint64_t)renderTargetFormats[2];
            hasher.bits.renderTargetFormat_3 = (uint64_t)renderTargetFormats[3];
            hasher.bits.renderTargetFormat_4 = (uint64_t)renderTargetFormats[4];
            hasher.bits.renderTargetFormat_5 = (uint64_t)renderTargetFormats[5];
            hasher.bits.renderTargetFormat_6 = (uint64_t)renderTargetFormats[6];
            hasher.bits.renderTargetFormat_7 = (uint64_t)renderTargetFormats[7];
            hasher.bits.depthStencilFormat = (uint64_t)depthStencilFormat;
            return hasher.value;
        }
    };

    struct PipelineStateDesc {
        const Shader *                  vs = nullptr;
        const Shader *                  ps = nullptr;
        const Shader *                  ds = nullptr;
        const Shader *                  hs = nullptr;
        const Shader *                  gs = nullptr;
        const InputLayout *             inputLayout;
        const RasterizerState *         rasterizerState = nullptr;
        const DepthStencilState *       depthStencilState = nullptr;
        const BlendState *              blendState = nullptr;
        const RenderDest *              renderDest;
        PrimitiveTopology               primitiveTopology = PrimitiveTopology::TriangleList;
        uint32_t                        sampleMask = 0xffffffff;
        uint32_t                        sampleCount = 1;
        uint32_t                        sampleQuality = 0;
    };

    enum class RenderPassFlag : uint8_t {
        None                            = 0,
        AllowUAVWrites                  = BIT(0),
        Suspending                      = BIT(1),
        Resuming                        = BIT(2)
    };

    struct RenderPass {
        struct RenderTargetEntry {
        };

        struct DepthStencilEntry {
        };
    };

    class SwapChain {
    public:
        virtual uint32_t                GetWidth() const = 0;
        virtual uint32_t                GetHeight() const = 0;

        virtual void                    Resize(uint32_t width, uint32_t height) = 0;
        virtual void                    SwapBuffers(bool vsync) = 0;
    };

    enum class CommandQueueType : uint8_t {
        Graphics,
        Compute,
        Count
    };

    enum class CommandListType : uint8_t {
        Primary,
        Secondary,
        Count
    };

    class CommandList {
    public:
        virtual void                    Reset(bool resetCacheStates = true, const CommandList *primaryCommandList = nullptr) = 0;

        virtual void                    CloseAndExecute(RHI::CommandQueueType queueType) = 0;
        virtual void                    CloseAndExecuteSecondary(RHI::CommandList *primaryCommandList) = 0;

        virtual int                     GetThreadIndex() const = 0;
    };

    enum class RasterizerStateType {
        SolidFrontSided,
        SolidBackSided,
        Wire,
        WireSmooth,
        Count
    };

    enum class DepthStencilStateType {
        Default,
        Count
    };

    enum class BlendStateType {
        Opaque,
        AlphaBlend,
        Add,
        Count
    };
};

class RHIRenderer {
public:
    virtual void                        Init(HWND hwnd);
    virtual void                        Shutdown();

    bool                                IsInitialized() const { return initialized; }

    virtual void                        BeginFrame() = 0;
    virtual void                        EndFrame() = 0;
    virtual void                        SwapChainBuffers(bool vsync) = 0;

    virtual void                        OnResize(int width, int height) = 0;

    virtual uint64_t                    SignalFence(RHI::CommandQueueType queueType) = 0;
    virtual bool                        IsFenceComplete(uint64_t checkFenceValue) = 0;
    virtual void                        WaitFence(uint64_t expectedFenceValue) = 0;
    virtual void                        Finish(RHI::CommandQueueType queueType) = 0;

    const RHI::RasterizerState *        GetRasterizerState(RHI::RasterizerStateType type) const { return &rasterizerStates[to_int(type)]; }
    const RHI::DepthStencilState *      GetDepthStencilState(RHI::DepthStencilStateType type) const { return &depthStencilStates[to_int(type)]; }
    const RHI::BlendState *             GetBlendState(RHI::BlendStateType type) const { return &blendStates[to_int(type)]; }

    virtual bool                        IsSupportedImageFormat(BE1::Image::Format::Enum imageFormat) = 0;
    virtual BE1::Image::Format::Enum    ToUncompressedImageFormat(BE1::Image::Format::Enum imageFormat) = 0;
    virtual BE1::Image::Format::Enum    ToCompressedImageFormat(BE1::Image::Format::Enum inFormat, bool useNormalMap) = 0;

    virtual RHI::Buffer *               CreateBuffer(RHI::BufferUsage usage, RHI::ResourceFlag flags, uint64_t size, BE1::Image::Format::Enum format, uint32_t stride, const void *data) = 0;
    virtual void                        DestroyBuffer(RHI::Buffer *buffer, bool immediate = false) = 0;

    virtual RHI::VertexBuffer *         CreateVertexBuffer(RHI::BufferUsage usage, uint32_t vertexSize, uint32_t numVerts, void *data) = 0;
    virtual void                        DestroyVertexBuffer(RHI::VertexBuffer *vertexBuffer, bool immediate = false) = 0;

    virtual RHI::IndexBuffer *          CreateIndexBuffer(RHI::BufferUsage usage, uint32_t indexSize, uint32_t numIndexes, void *data) = 0;
    virtual void                        DestroyIndexBuffer(RHI::IndexBuffer *indexBuffer, bool immediate = false) = 0;

    virtual RHI::ConstantBuffer *       CreateConstantBuffer(RHI::BufferUsage usage, uint32_t size, void *data) = 0;
    virtual void                        DestroyConstantBuffer(RHI::ConstantBuffer *constantBuffer, bool immediate = false) = 0;

    void                                AdjustTextureFormat(bool useCompression, bool useNormalMap, BE1::Image::Format::Enum inFormat, BE1::Image::Format::Enum *outFormat);

    virtual RHI::Texture *              CreateTexture(RHI::TextureType textureType, RHI::ResourceFlag flags, const BE1::Image *image, uint32_t sampleCount = 1) = 0;
    virtual RHI::Texture *              CreateTexture(RHI::TextureType textureType, RHI::ResourceFlag flags, const BE1::Image *image, BE1::Image::Format::Enum dstFormat, bool useMipmaps) = 0;
    virtual RHI::Texture *              CreateTextureFromFile(RHI::TextureType textureType, RHI::ResourceFlag flags, const char *filename, bool useCompression = true, bool useNormalMap = false);
    virtual void                        DestroyTexture(RHI::Texture *texture, bool immediate = false) = 0;
    virtual void                        GetTextureImage2D(RHI::Texture *texture, int level, BE1::Image::Format::Enum imageFormat, void *outPixels) = 0;
    virtual bool                        SetTextureSubImage2D(RHI::Texture *texture, int level, int x, int y, int width, int height, BE1::Image::Format::Enum imageFormat, const void *pixels) = 0;
    virtual bool                        SetTextureSubImage3D(RHI::Texture *texture, int level, int x, int y, int z, int width, int height, int depth, BE1::Image::Format::Enum imageFormat, const void *pixels) = 0;

    virtual int                         CreateSubresource(RHI::Buffer *buffer, RHI::SubresourceType subresourceType, uint64_t offset = 0, uint64_t size = ~0) = 0;
    virtual int                         CreateSubresource(RHI::Texture *texture, RHI::SubresourceType type, uint32_t firstSlice = 0, uint32_t sliceCount = ~0, uint32_t firstMipLevel = 0, uint32_t mipCount = ~0) = 0;

    virtual RHI::Shader *               CreateShader(RHI::ShaderModel shaderModel, RHI::ShaderStage shaderStage, const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint) = 0;
    virtual RHI::Shader *               CreateShaderFromFile(RHI::ShaderModel shaderModel, RHI::ShaderStage shaderStage, const char *filename, const char *entryPoint) = 0;
    virtual void                        DestroyShader(RHI::Shader *shader, bool immediate = false) = 0;

    virtual RHI::Sampler *              CreateSampler(const RHI::SamplerDesc *desc) = 0;
    virtual void                        DestroySampler(RHI::Sampler *sampler, bool immediate = false) = 0;

    virtual RHI::PipelineState *        CreateGraphicsPSO(const RHI::PipelineStateDesc *desc) = 0;
    virtual RHI::PipelineState *        CreateComputePSO(const RHI::Shader *computeShader) = 0;
    virtual void                        DestroyPSO(RHI::PipelineState *pipelineState, bool immediate = false) = 0;

    virtual RHI::QueryHeap *            CreateQueryHeap(const RHI::QueryHeapDesc *desc) = 0;
    virtual void                        DestroyQueryHeap(RHI::QueryHeap *queryHeap, bool immediate = false) = 0;

    virtual void                        SetVertexBuffer(RHI::CommandList *commandList, int slot, const RHI::VertexBuffer *vertexBuffer) = 0;
    virtual void                        SetIndexBuffer(RHI::CommandList *commandList, const RHI::IndexBuffer *indexBuffer) = 0;
    virtual void                        SetConstantBuffer(RHI::CommandList *commandList, int slot, const RHI::ConstantBuffer *constantBuffer) = 0;
    virtual void                        SetConstants(RHI::CommandList *commandList, const void *data, uint32_t size, uint32_t offset) = 0;
    virtual void                        SetTexture(RHI::CommandList *commandList, int slot, bool shaderWritable, const RHI::Texture *texture, int subresourceIndex = 0) = 0;
    virtual void                        SetBuffer(RHI::CommandList *commandList, int slot, bool shaderWritable, const RHI::Buffer *buffer, int subresourceIndex = 0) = 0;
    virtual void                        SetSampler(RHI::CommandList *commandList, int slot, RHI::Sampler *sampler) = 0;
    virtual void                        SetPSO(RHI::CommandList *commandList, const RHI::PipelineState *pipelineState) = 0;
    virtual void                        SetBlendFactor(RHI::CommandList *commandList, const BE1::Color4 &rgba) = 0;
    virtual void                        SetStencilRef(RHI::CommandList *commandList, uint32_t value) = 0;
    virtual void                        SetShadingRate(RHI::CommandList *commandList, RHI::ShadingRate shadingRate) = 0;
    virtual void                        SetViewport(RHI::CommandList *commandList, const BE1::Rect &viewportRect) = 0;
    virtual void                        SetScissorRect(RHI::CommandList *commandList, const BE1::Rect &scissorRect) = 0;
    virtual void                        SetDepthBounds(RHI::CommandList *commandList, float depthMin, float depthMax) = 0;
    virtual void                        BeginQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index) = 0;
    virtual void                        EndQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index) = 0;
    virtual void                        ResolveQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index, uint32_t count, const RHI::Buffer *destBuffer, uint64_t destOffset) = 0;
    virtual void                        ResetQuery(RHI::CommandList *commandList, const RHI::QueryHeap *queryHeap, uint32_t index, uint32_t count) = 0;
    virtual void                        Dispatch(RHI::CommandList *commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
    virtual void                        DispatchMesh(RHI::CommandList *commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
    virtual void                        ClearUAV(RHI::CommandList *commandList, const RHI::GPUResource *resource, uint32_t value) = 0;
    virtual void                        CopyBuffer(RHI::CommandList *commandList, const RHI::Buffer *dstBuffer, uint32_t dstOffset, const RHI::Buffer *srcBuffer, uint32_t srcOffset, uint32_t size) = 0;
    virtual void                        CopyTexture(RHI::CommandList *commandList, const RHI::Texture *dstTexture, uint32_t dstSlice, uint32_t dstMipLevel, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const RHI::Texture *srcTexture, uint32_t srcSlice, uint32_t srcMipLevel, uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t width, uint32_t height, uint32_t depth) = 0;
    virtual void                        Barrier(RHI::CommandList *commandList, const RHI::GPUBarrier *barriers, uint32_t barrierCount) = 0;
    virtual void                        Barrier(RHI::CommandList *commandList, const RHI::GPUBarrier &barrier) = 0;
    virtual void                        BeginRenderPass(RHI::CommandList *commandList, const RHI::SwapChain *swapChain, const BE1::Color4 &clearColor, float clearDepth, uint8_t clearStencil, RHI::ClearFlag clearFlag) = 0;
    virtual void                        EndRenderPass(RHI::CommandList *commandList) = 0;

    virtual void                        Draw(RHI::CommandList *commandList, uint32_t vertexCount, uint32_t startVertexLocation) = 0;
    virtual void                        DrawIndexed(RHI::CommandList *commandList, uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation) = 0;
    virtual void                        DrawInstanced(RHI::CommandList *commandList, uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;
    virtual void                        DrawIndexedInstanced(RHI::CommandList *commandList, uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) = 0;

    static RHI::GPUBarrier              MakeMemoryBarrier(const RHI::GPUResource *resource);
    static RHI::GPUBarrier              MakeBufferBarrier(const RHI::Buffer *buffer, RHI::GPUResourceState stateBefore, RHI::GPUResourceState stateAfter);
    static RHI::GPUBarrier              MakeImageBarrier(const RHI::Texture *texture, RHI::GPUResourceState stateBefore, RHI::GPUResourceState stateAfter, int slice = -1, int mipLevel = -1);
    static RHI::GPUBarrier              MakeAliasingBarrier(const RHI::GPUResource *resourceBefore, const RHI::GPUResource *resourceAfter);

protected:
    void                                SetupStates();

    RHI::RasterizerState                rasterizerStates[to_int(RHI::RasterizerStateType::Count)];
    RHI::DepthStencilState              depthStencilStates[to_int(RHI::DepthStencilStateType::Count)];
    RHI::BlendState                     blendStates[to_int(RHI::BlendStateType::Count)];
    bool                                initialized = false;
};

template<>
struct enable_bitmask_operators<RHI::ClearFlag> {
    static const bool enable = true;
};

template<>
struct enable_bitmask_operators<RHI::ColorWriteMask> {
    static const bool enable = true;
};

template<>
struct enable_bitmask_operators<RHI::ResourceFlag> {
    static const bool enable = true;
};

template<>
struct enable_bitmask_operators<RHI::GPUResourceState> {
    static const bool enable = true;
};

template<>
struct enable_bitmask_operators<RHI::RenderPassFlag> {
    static const bool enable = true;
};

namespace std {
    template <>
    struct hash<RHI::InputLayoutElement> {
        size_t operator()(const RHI::InputLayoutElement &element) const {
            size_t hash = element.semanticName.ToHash64();
            hash = hash_combine(hash, std::hash<uint32_t>()(element.semanticIndex));
            hash = hash_combine(hash, std::hash<uint32_t>()(element.offset));
            hash = hash_combine(hash, std::hash<uint32_t>()(element.inputSlot));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(element.format)));
            return hash;
        }
    };
}
