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

namespace RHI {
    enum class ClearFlag : uint32_t {
        None                            = 0,
        Color                           = BIT(0),
        Depth                           = BIT(1),
        Stencil                         = BIT(2)
    };

    struct ClearValue {
        union {
            float                       color[4] = {};
            struct DepthStencilValue {
                float                   depth;
                uint32_t                stencil = 0;
            } depthStencil;
        };

        static ClearValue Color(float r, float g, float b, float a) {
            ClearValue clearValue;
            clearValue.color[0] = r;
            clearValue.color[1] = g;
            clearValue.color[2] = b;
            clearValue.color[3] = a;
            return clearValue;
        }

        static ClearValue DepthStencil(float depth, uint32_t stencil) {
            ClearValue clearValue;
            clearValue.depthStencil.depth = depth;
            clearValue.depthStencil.stencil = stencil;
            return clearValue;
        }
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
        All = Red | Green | Blue | Alpha,
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
        Clamp,
        MirroredRepeat,
        MirroredClamp,
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
        None = 0,
        ConstantBuffer                  = BIT(0),
        VertexBuffer                    = BIT(1),
        IndexBuffer                     = BIT(2),
        IndirectBuffer                  = BIT(3),
        ShaderResource                  = BIT(4),
        RenderTarget                    = BIT(5),
        DepthStencil                    = BIT(6),
        UnorderedAccess                 = BIT(7),
        Typeless                        = BIT(8),
        SkipDefaultViews                = BIT(9)
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
        TextureCube,
        TextureCubeArray,
        Texture3D,
    };

    enum class CubemapFace : uint8_t {
        PositiveX,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ
    };

    enum class SubresourceType : uint8_t {
        SRV,                    // Shader Resource View
        UAV,                    // Unordered Access View
        RTV,                    // Render Target View
        DSV                     // Depth Stencil View
    };

    enum class ShaderFormat : uint8_t {
        None,
        HLSL5,                  // DXBC (Shader Model 5.0)
        HLSL6,                  // DXIL (Shader Model 5.1 이상)
        SPIRV                   // SPIR-V
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
        uint8_t                         maxAnisotropy = 1;
        TextureBorderColor              borderColor = TextureBorderColor::OpaqueBlack;
        float                           mipLodBias = 0.0f;
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
        BE1::StrArray                   includeDirs;
        BE1::StrArray                   defines;
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
        DepthWriteMask                  depthWriteMask = DepthWriteMask::Zero;
        ComparisonFunc                  depthFunc = ComparisonFunc::Never;
        uint8_t                         stencilReadMask = 0xff;
        uint8_t                         stencilWriteMask = 0xff;
        DepthStencilOp                  frontFace;
        DepthStencilOp                  backFace;
        bool                            depthTestEnabled = false;
        bool                            stencilTestEnabled = false;
        bool                            depthBoundTestEnabled = false;
    };

    struct RenderTargetBlendState {
        Blend                           srcFactorColor = Blend::SrcAlpha;
        Blend                           destFactorColor = Blend::InvSrcAlpha;
        BlendOp                         blendOpColor = BlendOp::Add;
        Blend                           srcFactorAlpha = Blend::One;
        Blend                           destFactorAlpha = Blend::One;
        BlendOp                         blendOpAlpha = BlendOp::Add;
        ColorWriteMask                  colorWriteMask = ColorWriteMask::All;
        bool                            blendEnabled = false;
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
        virtual ~GPUObject() = 0 {}
    };

    class GPUResource : public GPUObject {
    public:
        ~GPUResource() = 0 {}

        virtual void *                  GetNativeResource() const = 0;
        virtual const void *            GetNativeBufferObject() const { return nullptr; }
        virtual const void *            GetNativeTextureObject() const { return nullptr; }
    };

    class Buffer : public GPUResource {
    public:
        virtual bool                    IsValidSubresource(RHI::SubresourceType type, int subresourceIndex) const = 0;

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
        virtual bool                    IsValidSubresource(RHI::SubresourceType type, int subresourceIndex) const = 0;

        virtual uint32_t                GetWidth() const = 0;
        virtual uint32_t                GetHeight() const = 0;
        virtual uint32_t                GetDepth() const = 0;
        virtual uint32_t                GetArraySize() const = 0;
        virtual uint32_t                GetMipLevelCount() const = 0;

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
        std::atomic_int                 refCount = 1;
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
            const Buffer *              buffer;
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
        BE1::Image::Format              renderTargetFormats[8] = {};
        bool                            renderTargetForematSRGBs[8] = {};
        BE1::Image::Format              depthStencilFormat = BE1::Image::Format::Unknown;
        uint32_t                        sampleCount = 1;
        uint32_t                        sampleQuality = 0;

        constexpr uint64_t              GetHash() const {
            union Hasher {
                struct {
                    uint64_t renderTargetFormat_0 : 7;
                    uint64_t renderTargetFormat_1 : 7;
                    uint64_t renderTargetFormat_2 : 7;
                    uint64_t renderTargetFormat_3 : 7;
                    uint64_t renderTargetFormat_4 : 7;
                    uint64_t renderTargetFormat_5 : 7;
                    uint64_t renderTargetFormat_6 : 7;
                    uint64_t renderTargetFormat_7 : 7;
                    uint64_t depthStencilFormat : 4;
                    uint64_t sampleCount : 4;
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
            hasher.bits.depthStencilFormat = (uint64_t)depthStencilFormat - (uint64_t)BE1::Image::Format::D16;
            hasher.bits.sampleCount = (uint64_t)sampleCount;
            return hasher.value;
        }
    };

    struct PipelineStateDesc {
        const Shader *                  vs = nullptr;
        const Shader *                  ps = nullptr;
        const Shader *                  ds = nullptr;
        const Shader *                  hs = nullptr;
        const Shader *                  gs = nullptr;
        const InputLayout *             inputLayout = nullptr;
        const RasterizerState *         rasterizerState = nullptr;
        const DepthStencilState *       depthStencilState = nullptr;
        const BlendState *              blendState = nullptr;
        const RenderDest *              renderDest = nullptr;
        PrimitiveTopology               primitiveTopology = PrimitiveTopology::TriangleList;
        uint32_t                        sampleMask = 0xffffffff;
    };

    enum class RenderPassFlag : uint8_t {
        None                            = 0,
        AllowUAVWrites                  = BIT(0),
        Suspending                      = BIT(1),
        Resuming                        = BIT(2)
    };

    struct RenderPassImage {
        enum class Type : uint8_t {
            Color,
            DepthStencil,
            ResolveColor,
            ResolveDepth,
            ShadingRateSource
        };

        enum class LoadAction : uint8_t {
            Load,
            Clear,
            DontCare
        };

        enum class StoreAction : uint8_t {
            Store,
            DontCare
        };

        enum class DepthResolveMode : uint8_t {
            None,
            Min,
            Max
        };

        const Texture *                 texture = nullptr;
        int                             subresourceIndex = -1;
        int                             resolveSourceIndex = 0;
        Type                            type = Type::Color;
        LoadAction                      loadAction = LoadAction::Load;
        StoreAction                     storeAction = StoreAction::Store;
        DepthResolveMode                depthResolveMode = DepthResolveMode::None;
        GPUResourceState                beforeState = GPUResourceState::Undefined;
        GPUResourceState                duringState = GPUResourceState::Undefined;
        GPUResourceState                afterState = GPUResourceState::Undefined;

        static RenderPassImage Color(const Texture *texture, int subresourceIndex = -1, LoadAction loadAction = LoadAction::Load, StoreAction storeAction = StoreAction::Store,
            GPUResourceState beforeState = GPUResourceState::ShaderResource, GPUResourceState afterState = GPUResourceState::ShaderResource) {
            RenderPassImage image;
            image.texture = texture;
            image.subresourceIndex = subresourceIndex;
            image.type = Type::Color;
            image.loadAction = loadAction;
            image.storeAction = storeAction;
            image.beforeState = beforeState;
            image.duringState = GPUResourceState::RenderTarget;
            image.afterState = afterState;
            return image;
        }

        static RenderPassImage DepthStencil(const Texture *texture, int subresourceIndex = -1, LoadAction loadAction = LoadAction::Load, StoreAction storeAction = StoreAction::Store,
            GPUResourceState beforeState = GPUResourceState::DepthWrite, GPUResourceState duringState = GPUResourceState::DepthWrite, GPUResourceState afterState = GPUResourceState::DepthWrite) {
            RenderPassImage image;
            image.texture = texture;
            image.subresourceIndex = subresourceIndex;
            image.type = Type::DepthStencil;
            image.loadAction = loadAction;
            image.storeAction = storeAction;
            image.beforeState = beforeState;
            image.duringState = duringState;
            image.afterState = afterState;
            return image;
        }

        static RenderPassImage ResolveColor(const Texture *texture, int subresourceIndex = -1, int resolveSourceIndex = 0,
            GPUResourceState beforeState = GPUResourceState::ShaderResource, GPUResourceState afterState = GPUResourceState::ShaderResource) {
            RenderPassImage image;
            image.texture = texture;
            image.subresourceIndex = subresourceIndex;
            image.resolveSourceIndex = resolveSourceIndex;
            image.type = Type::ResolveColor;
            image.beforeState = beforeState;
            image.duringState = GPUResourceState::CopyDst;
            image.afterState = afterState;
            return image;
        }

        static RenderPassImage ResolveDepth(const Texture *texture, int subresourceIndex = -1, DepthResolveMode depthResolveMode = DepthResolveMode::Min,
            GPUResourceState beforeState = GPUResourceState::ShaderResource, GPUResourceState afterState = GPUResourceState::ShaderResource) {
            RenderPassImage image;
            image.texture = texture;
            image.subresourceIndex = subresourceIndex;
            image.type = Type::ResolveDepth;
            image.beforeState = beforeState;
            image.duringState = GPUResourceState::CopyDst;
            image.afterState = afterState;
            return image;
        }

        static RenderPassImage ShadingRateSource(const Texture *texture,
            GPUResourceState beforeState = GPUResourceState::ShadingRateSource, GPUResourceState afterState = GPUResourceState::ShadingRateSource) {
            RenderPassImage image;
            image.texture = texture;
            image.type = Type::ShadingRateSource;
            image.beforeState = beforeState;
            image.duringState = GPUResourceState::ShadingRateSource;
            image.afterState = afterState;
            return image;
        }
    };

    class SwapChain {
    public:
        virtual ~SwapChain() = 0 {}

        virtual bool                    IsSupportsHDR() const = 0;
        virtual bool                    GetFormat(BE1::Image::Format *imageFormat, bool *isSRGB) const = 0;
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

    class FrameThreadData;
    class CommandList {
    public:
        virtual ~CommandList() = 0 {}

        virtual void                    Reset(bool resetCacheStates = true, const CommandList *primaryCommandList = nullptr) = 0;

        virtual void                    Close() = 0;
        virtual void                    Execute() = 0;
        virtual void                    ExecuteSecondary(RHI::CommandList *primaryCommandList, const RHI::FrameThreadData *frameThreadData) = 0;
        virtual void                    CloseAndExecute() = 0;
        virtual void                    CloseAndExecuteSecondary(RHI::CommandList *primaryCommandList, const RHI::FrameThreadData *frameThreadData) = 0;

        virtual FrameThreadData *       GetFrameThreadData() const = 0;
    };

    class FrameThreadData {
    public:
        virtual ~FrameThreadData() = 0 {}

        virtual void                    Reset() = 0;

        virtual ConstantBuffer *        AllocConstant(uint32_t size) = 0;
        virtual VertexBuffer *          AllocVertex(uint32_t vertexSize, uint32_t count) = 0;
        virtual IndexBuffer *           AllocIndex(uint32_t indexSize, uint32_t count) = 0;
        virtual Buffer *                AllocBuffer(BE1::Image::Format format, uint32_t stride, uint32_t count) = 0;
        Buffer *                        AllocTypedBuffer(BE1::Image::Format format, uint32_t count) { return AllocBuffer(format, 0, count); }
        Buffer *                        AllocRawBuffer(uint32_t count) { return AllocBuffer(BE1::Image::Format::Unknown, 0, count); }
        Buffer *                        AllocStructuredBuffer(uint32_t stride, uint32_t count) { return AllocBuffer(BE1::Image::Format::Unknown, stride, count); }

        virtual CommandList *           AllocGraphicsCommandList(RHI::CommandListType type = RHI::CommandListType::Primary) = 0;
        virtual CommandList *           AllocComputeCommandList() = 0;

        virtual CommandList *           BeginCommandList(RHI::CommandQueueType queueType) = 0;
        virtual CommandList *           BeginSecondaryCommandList(const RHI::CommandList *primaryCommandList) = 0;
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
        Never,
        Count
    };

    enum class BlendStateType {
        Opaque,
        AlphaBlend,
        Add,
        Count
    };

    class Renderer {
    public:
        virtual ~Renderer() = 0 {}

        virtual void                    Init(const void *mainWindowHandle);
        virtual void                    Shutdown();

        bool                            IsInitialized() const { return initialized; }

        virtual uint64_t                SignalFence(CommandQueueType queueType) = 0;
        virtual bool                    IsFenceComplete(uint64_t checkFenceValue) = 0;
        virtual void                    WaitFence(uint64_t expectedFenceValue) = 0;
        virtual void                    Finish(CommandQueueType queueType) = 0;

        virtual void                    FreePendingResources(bool waitPendings = false) = 0;

        const RasterizerState *         GetRasterizerState(RasterizerStateType type) const { return &rasterizerStates[to_int(type)]; }
        const DepthStencilState *       GetDepthStencilState(DepthStencilStateType type) const { return &depthStencilStates[to_int(type)]; }
        const BlendState *              GetBlendState(BlendStateType type) const { return &blendStates[to_int(type)]; }

        virtual bool                    IsSupportedImageFormat(BE1::Image::Format imageFormat) const = 0;
        virtual BE1::Image::Format      ToUncompressedImageFormat(BE1::Image::Format imageFormat) const = 0;
        virtual BE1::Image::Format      ToCompressedImageFormat(BE1::Image::Format inFormat, bool useNormalMap) const = 0;
        virtual BE1::Image::Format      ToUAVImageFormat(BE1::Image::Format inFormat) const = 0;

        virtual RHI::FrameThreadData *  CreateFrameThreadData() = 0;
        virtual void                    DestroyFrameThreadData(FrameThreadData *frameThreadData) = 0;

        virtual SwapChain *             CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height, BE1::Image::Format format) = 0;
        virtual void                    DestroySwapChain(SwapChain *swapChain) = 0;

        virtual Buffer *                CreateBuffer(BufferUsage usage, ResourceFlag flags, uint64_t size, BE1::Image::Format format, uint32_t stride, const void *data) = 0;
        Buffer *                        CreateTypedBuffer(BufferUsage usage, ResourceFlag flags, uint64_t size, BE1::Image::Format format, const void *data) { return CreateBuffer(usage, flags, size, format, 0, data); }
        Buffer *                        CreateRawBuffer(BufferUsage usage, ResourceFlag flags, uint64_t size, const void *data) { return CreateBuffer(usage, flags, size, BE1::Image::Format::Unknown, 0, data); }
        Buffer *                        CreateStructuredBuffer(BufferUsage usage, ResourceFlag flags, uint64_t size, uint32_t stride, const void *data) { return CreateBuffer(usage, flags, size, BE1::Image::Format::Unknown, stride, data); }
        virtual void                    DestroyBuffer(Buffer *buffer, bool immediate = false) = 0;

        virtual VertexBuffer *          CreateVertexBuffer(BufferUsage usage, uint32_t vertexSize, uint32_t numVerts, const void *data) = 0;
        virtual void                    DestroyVertexBuffer(VertexBuffer *vertexBuffer, bool immediate = false) = 0;

        virtual IndexBuffer *           CreateIndexBuffer(BufferUsage usage, uint32_t indexSize, uint32_t numIndexes, const void *data) = 0;
        virtual void                    DestroyIndexBuffer(IndexBuffer *indexBuffer, bool immediate = false) = 0;

        virtual ConstantBuffer *        CreateConstantBuffer(BufferUsage usage, uint32_t size, const void *data) = 0;
        virtual void                    DestroyConstantBuffer(ConstantBuffer *constantBuffer, bool immediate = false) = 0;

        void                            AdjustTextureFormat(bool useCompression, bool useNormalMap, bool useUAV, BE1::Image::Format inFormat, BE1::Image::Format *outFormat);
        void                            AdjustTextureSize(TextureType textureType, bool useNPOT, uint32_t inWidth, uint32_t inHeight, uint32_t inDepth, uint32_t *outWidth, uint32_t *outHeight, uint32_t *outDepth);

        virtual Texture *               CreateTexture(TextureType textureType, ResourceFlag flags, const BE1::Image *image, bool allocateEmptyMipmaps, const ClearValue &clearValue = {}, uint32_t sampleCount = 1, GPUResourceState initialState = GPUResourceState::Undefined) = 0;
        virtual Texture *               CreateTexture(TextureType textureType, ResourceFlag flags, const BE1::Image *image, BE1::Image::Format dstFormat, bool generateMipmaps = true, bool allocateEmptyMipmaps = false) = 0;
        virtual Texture *               CreateTextureFromFile(TextureType textureType, ResourceFlag flags, const char *filename, bool useCompression = true, bool useNormalMap = false, bool generateMipmaps = true, bool allocateEmptyMipmaps = false);
        virtual void                    DestroyTexture(Texture *texture, bool immediate = false) = 0;

        virtual void                    GetTextureImage2D(Texture *texture, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) = 0;
        virtual void                    GetTextureImage3D(Texture *texture, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) = 0;
        virtual void                    GetTextureImageCubeFace(Texture *texture, CubemapFace face, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) = 0;

        virtual bool                    SetTextureSubImage2D(Texture *texture, int mipLevel, int x, int y, int width, int height, BE1::Image::Format imageFormat, const void *pixels) = 0;
        virtual bool                    SetTextureSubImage3D(Texture *texture, int mipLevel, int x, int y, int z, int width, int height, int depth, BE1::Image::Format imageFormat, const void *pixels) = 0;
        virtual bool                    SetTextureSubImageCubeFace(RHI::Texture *texture, RHI::CubemapFace face, int mipLevel, int x, int y, int width, int height, BE1::Image::Format imageFormat, const void *pixels) = 0;

        virtual int                     CreateSubresource(Buffer *buffer, SubresourceType subresourceType, uint64_t offset = 0, uint64_t size = ~0, const BE1::Image::Format *newFormat = nullptr) = 0;
        virtual int                     CreateSubresource(Texture *texture, SubresourceType type, uint32_t firstSlice = 0, uint32_t sliceCount = ~0, uint32_t firstMipLevel = 0, uint32_t mipCount = ~0, const BE1::Image::Format *typelessCompatibleFormat = nullptr, bool isSRGB = false) = 0;

        virtual void                    DestroySubresource(RHI::Buffer *buffer, RHI::SubresourceType type, int subresourceIndex = -1) = 0;
        virtual void                    DestroySubresource(RHI::Texture *texture, RHI::SubresourceType type, int subresourceIndex = -1) = 0;

        virtual Shader *                CreateShader(ShaderModel shaderModel, ShaderStage shaderStage, const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint) = 0;
        virtual Shader *                CreateShaderFromFile(ShaderModel shaderModel, ShaderStage shaderStage, const char *filename, const char *entryPoint) = 0;
        virtual void                    DestroyShader(Shader *shader, bool immediate = false) = 0;

        virtual Sampler *               CreateSampler(const SamplerDesc *desc) = 0;
        virtual void                    DestroySampler(Sampler *sampler, bool immediate = false) = 0;
        virtual void                    RecreateSampler(Sampler *sampler, const SamplerDesc *desc) = 0;

        virtual PipelineState *         CreateGraphicsPSO(const PipelineStateDesc *desc) = 0;
        virtual PipelineState *         CreateComputePSO(const Shader *computeShader) = 0;
        virtual void                    DestroyPSO(PipelineState *pipelineState, bool immediate = false) = 0;

        virtual QueryHeap *             CreateQueryHeap(const QueryHeapDesc *desc) = 0;
        virtual void                    DestroyQueryHeap(QueryHeap *queryHeap, bool immediate = false) = 0;

        virtual RHI::CommandList *      BeginCommandList(RHI::CommandQueueType queueType) = 0;
        virtual void                    EndCommandList(RHI::CommandList *commandList) = 0;

        virtual void                    SetVertexBuffer(CommandList *commandList, int slot, const VertexBuffer *vertexBuffer) = 0;
        virtual void                    SetIndexBuffer(CommandList *commandList, const IndexBuffer *indexBuffer) = 0;
        virtual void                    SetConstantBuffer(CommandList *commandList, int slot, const ConstantBuffer *constantBuffer) = 0;
        virtual void                    SetConstants(CommandList *commandList, const void *data, uint32_t size, uint32_t offset) = 0;
        virtual void                    SetTexture(CommandList *commandList, int slot, bool shaderWritable, const Texture *texture, int subresourceIndex = -1) = 0;
        virtual void                    SetBuffer(CommandList *commandList, int slot, bool shaderWritable, const Buffer *buffer, int subresourceIndex = -1) = 0;
        virtual void                    SetSampler(CommandList *commandList, int slot, Sampler *sampler) = 0;
        virtual void                    SetPSO(CommandList *commandList, const PipelineState *pipelineState) = 0;
        virtual void                    SetBlendFactor(CommandList *commandList, const BE1::Color4 &rgba) = 0;
        virtual void                    SetStencilRef(CommandList *commandList, uint32_t value) = 0;
        virtual void                    SetShadingRate(CommandList *commandList, ShadingRate shadingRate) = 0;
        virtual void                    SetViewport(CommandList *commandList, const BE1::Rect &viewportRect) = 0;
        virtual void                    SetScissorRect(CommandList *commandList, const BE1::Rect &scissorRect) = 0;
        virtual void                    SetDepthBounds(CommandList *commandList, float depthMin, float depthMax) = 0;
        virtual void                    BeginQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index) = 0;
        virtual void                    EndQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index) = 0;
        virtual void                    ResolveQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index, uint32_t count, const Buffer *destBuffer, uint64_t destOffset) = 0;
        virtual void                    ResetQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index, uint32_t count) = 0;
        virtual void                    ClearUAV(CommandList *commandList, const GPUResource *resource, uint32_t value) = 0;
        virtual void                    CopyBuffer(CommandList *commandList, const Buffer *dstBuffer, uint32_t dstOffset, const Buffer *srcBuffer, uint32_t srcOffset, uint32_t size) = 0;
        virtual void                    CopyTexture(CommandList *commandList, const Texture *dstTexture, uint32_t dstSlice, uint32_t dstMipLevel, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const Texture *srcTexture, uint32_t srcSlice, uint32_t srcMipLevel, uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t width, uint32_t height, uint32_t depth) = 0;
        virtual void                    GenerateMipmaps(CommandList *commandList, const Texture *texture) = 0;
        virtual void                    Barrier(CommandList *commandList, const GPUBarrier *barriers, uint32_t barrierCount) = 0;
        void                            Barrier(CommandList *commandList, const GPUBarrier &barrier) { Barrier(commandList, &barrier, 1); }
        virtual void                    ReadPixels(RHI::CommandList *commandList, const RHI::SwapChain *swapChain, int x, int y, int width, int height, BE1::Image::Format dstFormat, void *outPixels) = 0;
        virtual void                    BeginRenderPass(CommandList *commandList, const SwapChain *swapChain, const Texture *depthStencilTexture, const BE1::Color4 &clearColor = {}, float clearDepth = 0, uint8_t clearStencil = 0, ClearFlag clearFlags = ClearFlag::None) = 0;
        virtual void                    BeginRenderPass(CommandList *commandList, const RenderPassImage renderPassImages[], int numRenderPassImages, RenderPassFlag flags = RenderPassFlag::None) = 0;
        virtual void                    EndRenderPass(CommandList *commandList) = 0;

        virtual void                    Draw(CommandList *commandList, uint32_t vertexCount, uint32_t startVertexLocation) = 0;
        virtual void                    DrawIndexed(CommandList *commandList, uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation) = 0;
        virtual void                    DrawInstanced(CommandList *commandList, uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;
        virtual void                    DrawInstancedIndirect(CommandList *commandList, const Buffer *argsBuffer, uint32_t argsOffset) = 0;
        virtual void                    DrawInstancedIndirectCount(CommandList *commandList, const Buffer *argsBuffer, uint32_t argsOffset, const Buffer *countBuffer, uint32_t countOffset, uint32_t maxCount) = 0;
        virtual void                    DrawIndexedInstanced(CommandList *commandList, uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) = 0;
        virtual void                    DrawIndexedInstancedIndirect(CommandList *commandList, const Buffer *argsBuffer, uint32_t argsOffset) = 0;
        virtual void                    DrawIndexedInstancedIndirectCount(CommandList *commandList, const Buffer *argsBuffer, uint32_t argsOffset, const Buffer *countBuffer, uint32_t countOffset, uint32_t maxCount) = 0;

        virtual void                    Dispatch(CommandList *commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
        virtual void                    DispatchIndirect(CommandList *commandList, const Buffer *argsBuffer, uint32_t argsOffset) = 0;
        virtual void                    DispatchMesh(CommandList *commandList, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
        virtual void                    DispatchMeshIndirect(CommandList *commandList, const Buffer *argsBuffer, uint32_t argsOffset) = 0;

        virtual void                    SetMarker(CommandList *commandList, const char *string, uint8_t colorIndex) = 0;
        virtual void                    BeginEvent(CommandList *commandList, const char *string, uint8_t colorIndex) = 0;
        virtual void                    EndEvent(CommandList *commandList) = 0;

        static GPUBarrier               MakeMemoryBarrier(const GPUResource *resource);
        static GPUBarrier               MakeBufferBarrier(const Buffer *buffer, GPUResourceState stateBefore, GPUResourceState stateAfter);
        static GPUBarrier               MakeImageBarrier(const Texture *texture, GPUResourceState stateBefore, GPUResourceState stateAfter, int slice = -1, int mipLevel = -1);
        static GPUBarrier               MakeAliasingBarrier(const GPUResource *resourceBefore, const GPUResource *resourceAfter);

    protected:
        void                            InitDefaultStates();
        void                            FreeDefaultStates();

        RasterizerState                 rasterizerStates[to_int(RasterizerStateType::Count)];
        DepthStencilState               depthStencilStates[to_int(DepthStencilStateType::Count)];
        BlendState                      blendStates[to_int(BlendStateType::Count)];
        uint32_t                        max1DTextureSize = 0;
        uint32_t                        max2DTextureSize = 0;
        uint32_t                        max3DTextureSize = 0;
        uint32_t                        maxCubeTextureSize = 0;
        uint32_t                        max1DTextureArraySize = 0;
        uint32_t                        max2DTextureArraySize = 0;
        uint32_t                        maxCubeTextureArraySize = 0;
        bool                            initialized = false;
    };

    extern Renderer *                   renderer;

    class ScopedEventObject {
    public:
        ScopedEventObject() = delete;
        ScopedEventObject(CommandList *commandList, const char *string, uint8_t colorIndex) : eventCommandList(commandList) { renderer->BeginEvent(commandList, string, colorIndex); }
        ScopedEventObject(ScopedEventObject &scopedEventObject) = delete;
        ~ScopedEventObject() { renderer->EndEvent(eventCommandList); }

        CommandList *                   eventCommandList = nullptr;
    };
};

#if defined(USE_PROFILER) && (defined(_DEBUG) || defined(_DEVELOPMENT))
#define PROFILER_CPU_BEGIN_EVENT(string, colorIndex) RHI::renderer->BeginEvent(nullptr, string, colorIndex)
#define PROFILER_CPU_END_EVENT() RHI::renderer->EndEvent(nullptr);
#define PROFILER_CPU_SCOPED_EVENT(string, colorIndex) RHI::ScopedEventObject _scopedEventObject(nullptr, string, colorIndex)
#define PROFILER_CPU_MARKER(string, colorIndex) RHI::renderer->SetMarker(nullptr, string, colorIndex)
#define PROFILER_BEGIN_EVENT(commandList, string, colorIndex) RHI::renderer->BeginEvent(commandList, string, colorIndex)
#define PROFILER_END_EVENT(commandList) RHI::renderer->EndEvent(commandList)
#define PROFILER_SCOPED_EVENT(commandList, string, colorIndex) RHI::ScopedEventObject _scopedEventObject(commandList, string, colorIndex)
#define PROFILER_MARKER(commandList, string, colorIndex) RHI::renderer->SetMarker(commandList, string, colorIndex)
#else
#define PROFILER_CPU_BEGIN_EVENT(string, colorIndex)
#define PROFILER_CPU_END_EVENT()
#define PROFILER_CPU_SCOPED_EVENT(string, colorIndex)
#define PROFILER_CPU_MARKER(string, colorIndex)
#define PROFILER_BEGIN_EVENT(commandList, string, colorIndex)
#define PROFILER_END_EVENT(commandList)
#define PROFILER_SCOPED_EVENT(commandList, string, colorIndex)
#define PROFILER_MARKER(commandList, string, colorIndex)
#endif

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

    template <>
    struct hash<RHI::RasterizerState> {
        size_t operator()(const RHI::RasterizerState &state) const {
            size_t hash = std::hash<uint8_t>()(static_cast<uint8_t>(state.fillMode));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(state.cullMode)));
            hash = hash_combine(hash, std::hash<int32_t>()(state.depthBias));
            hash = hash_combine(hash, std::hash<float>()(state.depthBiasClamp));
            hash = hash_combine(hash, std::hash<float>()(state.slopeScaledDepthBias));
            hash = hash_combine(hash, std::hash<bool>()(state.depthClipEnabled));
            hash = hash_combine(hash, std::hash<bool>()(state.smoothLineEnabled));
            hash = hash_combine(hash, std::hash<bool>()(state.conservativeRasterization));
            return hash;
        }
    };

    template <>
    struct hash<RHI::DepthStencilOp> {
        size_t operator()(const RHI::DepthStencilOp &op) const {
            size_t hash = std::hash<uint8_t>()(static_cast<uint8_t>(op.failOp));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(op.depthFailOp)));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(op.passOp)));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(op.stencilFunc)));
            return hash;
        }
    };

    template <>
    struct hash<RHI::DepthStencilState> {
        size_t operator()(const RHI::DepthStencilState &state) const {
            size_t hash = std::hash<uint8_t>()(static_cast<uint8_t>(state.depthWriteMask));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(state.depthFunc)));
            hash = hash_combine(hash, std::hash<uint8_t>()(state.stencilReadMask));
            hash = hash_combine(hash, std::hash<uint8_t>()(state.stencilWriteMask));
            hash = hash_combine(hash, std::hash<RHI::DepthStencilOp>()(state.frontFace));
            hash = hash_combine(hash, std::hash<RHI::DepthStencilOp>()(state.backFace));
            hash = hash_combine(hash, std::hash<bool>()(state.depthTestEnabled));
            hash = hash_combine(hash, std::hash<bool>()(state.stencilTestEnabled));
            hash = hash_combine(hash, std::hash<bool>()(state.depthBoundTestEnabled));
            return hash;
        }
    };

    template <>
    struct hash<RHI::RenderTargetBlendState> {
        size_t operator()(const RHI::RenderTargetBlendState &state) const {
            size_t hash = std::hash<uint8_t>()(static_cast<uint8_t>(state.srcFactorColor));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(state.destFactorColor)));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(state.blendOpColor)));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(state.srcFactorAlpha)));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(state.destFactorAlpha)));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(state.blendOpAlpha)));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(state.colorWriteMask)));
            hash = hash_combine(hash, std::hash<bool>()(state.blendEnabled));
            return hash;
        }
    };

    template <>
    struct hash<RHI::BlendState> {
        size_t operator()(const RHI::BlendState &state) const {
            size_t hash = std::hash<bool>()(static_cast<bool>(state.alphaToCoverageEnabled));
            hash = hash_combine(hash, std::hash<bool>()(static_cast<bool>(state.independentBlendEnabled)));
            for (int i = 0; i < COUNT_OF(state.renderTargets); ++i) {
                hash = hash_combine(hash, std::hash<RHI::RenderTargetBlendState>()(static_cast<RHI::RenderTargetBlendState>(state.renderTargets[i])));
            }
            return hash;
        }
    };
}
