// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

namespace RHI {
    enum class GraphicsAPI : uint8_t {
        Default,
        D3D12,
        Vulkan,
        Metal
    };

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

    enum class GPUResourceStatus : uint8_t {
        Pending,
        Ready,
        Failed,
        Cancelled
    };

    enum class BufferUsage : uint8_t {
        Default,
        Upload,
        Readback
    };

    enum class BufferFlag : uint32_t {
        None = 0,
        ConstantBuffer                  = BIT(0),
        VertexBuffer                    = BIT(1),
        IndexBuffer                     = BIT(2),
        IndirectBuffer                  = BIT(3),
        ShaderResource                  = BIT(10),
        UnorderedAccess                 = BIT(11)
    };

    enum class TextureFlag : uint32_t {
        None = 0,
        RenderTarget                    = BIT(0),
        DepthStencil                    = BIT(1),
        Typeless                        = BIT(2),
        ShaderResource                  = BIT(10),
        UnorderedAccess                 = BIT(11),
        SkipDefaultViews                = BIT(12)
    };

    enum class TextureMipInitMode : uint8_t {
        FromImage,
        AllocateEmpty,
        GenerateOnGPU
    };

    enum class BufferType : uint8_t {
        Static,
        Dynamic
    };

    enum class TextureType : uint8_t {
        Unknown,
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

    enum class ViewType : uint8_t {
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

    enum class Format : uint8_t {
        Unknown,
        Auto,

        R32G32B32A32_FLOAT,
        R32G32B32A32_UINT,
        R32G32B32A32_SINT,

        R32G32B32_FLOAT,
        R32G32B32_UINT,
        R32G32B32_SINT,

        R16G16B16A16_FLOAT,
        R16G16B16A16_UNORM,
        R16G16B16A16_UINT,
        R16G16B16A16_SNORM,
        R16G16B16A16_SINT,

        R32G32_FLOAT,
        R32G32_UINT,
        R32G32_SINT,

        D32_FLOAT_S8X24_UINT,       // DSV (Depth 32-bit, Stencil 8-bit, Unused 24-bit)
        R32_FLOAT_X8X24_TYPELESS,   // SRV (Red 32-bit, Unused 8-bit, Unused 24-bit)
        X32_TYPELESS_G8X24_UINT,    // SRV (Unused 32-bit, Green 8-bit, Unused 24-bit)

        R10G10B10A2_UNORM,
        R10G10B10A2_UINT,

        R11G11B10_FLOAT,

        R8G8B8A8_UNORM,
        R8G8B8A8_UNORM_SRGB,
        R8G8B8A8_UINT,
        R8G8B8A8_SNORM,
        R8G8B8A8_SINT,

        R16G16_FLOAT,
        R16G16_UNORM,
        R16G16_UINT,
        R16G16_SNORM,
        R16G16_SINT,

        R32_FLOAT,
        D32_FLOAT,
        R32_UINT,
        R32_SINT,

        D24_UNORM_S8_UINT,          // DSV (Depth 24-bit, Stencil 8-bit)
        R24_UNORM_X8_TYPELESS,      // SRV (Red 24-bit, Unused 8-bit)
        X24_TYPELESS_G8_UINT,       // SRV (Unused 24-bit, Green 8-bit)

        R8G8_UNORM,
        R8G8_UINT,
        R8G8_SNORM,
        R8G8_SINT,

        R16_FLOAT,
        D16_UNORM,
        R16_UNORM,
        R16_UINT,
        R16_SNORM,
        R16_SINT,

        R8_UNORM,
        R8_UINT,
        R8_SNORM,
        R8_SINT,
        A8_UNORM,

        B5G6R5_UNORM,
        B5G5R5A1_UNORM,
        B8G8R8A8_UNORM,
        B8G8R8A8_UNORM_SRGB,
        B8G8R8X8_UNORM,
        B8G8R8X8_UNORM_SRGB,

        B4G4R4A4_UNORM,
        A4B4G4R4_UNORM,

        R9G9B9E5_SHAREDEXP,

        BC1_UNORM,
        BC1_UNORM_SRGB,
        BC2_UNORM,
        BC2_UNORM_SRGB,
        BC3_UNORM,
        BC3_UNORM_SRGB,
        BC4_UNORM,
        BC4_SNORM,
        BC5_UNORM,
        BC5_SNORM,
        BC6H_UF16,
        BC6H_SF16,
        BC7_UNORM,
        BC7_UNORM_SRGB
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

    struct ShaderSourceDependency {
        BE1::Str                        filename;
        uint64_t                        contentHash = 0;
    };

    struct ShaderCompileOutput {
        byte *                          compiledShaderData = nullptr;
        uint32_t                        compiledShaderDataSize = 0;
        BE1::Str                        errorMessage;
        BE1::Array<ShaderSourceDependency> dependencies;
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

    struct MemoryUsage {
        uint64_t                        budgetBytes = 0;    // total video memory available for use by the current application
        uint64_t                        usageBytes = 0;     // used video memory by the current application
    };

    struct RendererInfo {
        static constexpr int            MaxNameLength = 128;

        char                            name[MaxNameLength] = {};
        uint32_t                        vendorId = 0;
        uint32_t                        deviceId = 0;
        uint64_t                        dedicatedVideoMemoryBytes = 0;
        uint64_t                        dedicatedSystemMemoryBytes = 0;
        uint64_t                        sharedSystemMemoryBytes = 0;
    };

    enum class QueryType : uint8_t {
        Timestamp,
        Occlusion,
        OcclusionBinary
    };

    struct QueryHeapDesc {
        QueryType                       type = QueryType::Timestamp;
        uint32_t                        queryCount = 0;
    };

    struct ViewID {
        uint32_t                        value = InvalidValue;

        static constexpr uint32_t       InvalidValue = UINT32_MAX;
        static constexpr uint32_t       PrimaryValue = 0;

        static const ViewID             InvalidID;
        static const ViewID             PrimaryID;

        constexpr bool IsValid() const { return value != InvalidValue; }
        constexpr bool IsPrimary() const { return value == PrimaryValue; }
    };

    struct BufferViewInfo {
        ViewID                          viewID = ViewID::InvalidID;
        ViewType                        viewType = ViewType::SRV;
        Format                          format = Format::Unknown;
        uint64_t                        byteOffset = 0;
        uint64_t                        byteSize = 0;
        uint64_t                        firstElement = 0;
        uint32_t                        elementCount = 0;
        uint32_t                        structureByteStride = 0;
        bool                            raw = false;
    };

    struct TextureViewInfo {
        ViewID                          viewID = ViewID::InvalidID;
        ViewType                        viewType = ViewType::SRV;
        Format                          format = Format::Unknown;
        uint32_t                        firstSlice = 0;
        uint32_t                        sliceCount = 0;
        uint32_t                        firstMipLevel = 0;
        uint32_t                        mipCount = 0;
        uint32_t                        width = 0;
        uint32_t                        height = 0;
        uint32_t                        depth = 0;
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

        GPUResourceStatus               GetStatus() const { return status.load(); }
        void                            SetStatus(GPUResourceStatus newStatus) { status.store(newStatus); }

        GPUResource &                   operator=(const GPUResource &rhs) { status.store(rhs.status.load()); return *this; }

    protected:
        std::atomic<GPUResourceStatus>  status = GPUResourceStatus::Ready;
    };

    class Buffer : public GPUResource {
    public:
        virtual bool                    HasValidView(RHI::ViewType viewType, RHI::ViewID viewID = RHI::ViewID::PrimaryID) const = 0;
        virtual uint32_t                GetViewCount(RHI::ViewType viewType) const = 0;
        virtual RHI::ViewID             GetViewID(RHI::ViewType viewType, uint32_t viewIndex) const = 0;
        virtual bool                    GetViewInfo(RHI::ViewType viewType, RHI::ViewID viewID, BufferViewInfo &outInfo) const = 0;

        virtual uint32_t                GetBindlessIndex(RHI::ViewType viewType, RHI::ViewID viewID = RHI::ViewID::PrimaryID) const = 0;

        void *                          mappedPtr = nullptr; // Upload or Readback or Dynamic (Upload) 버퍼는 CPU 가 접근할 수 있도록 매핑된 포인터를 갖는다.
    };

    class VertexBuffer : public GPUResource {
    public:
        virtual uint32_t                GetOffset() const = 0;
        virtual uint32_t                GetSizeInBytes() const = 0;

        void *                          mappedPtr = nullptr; // Dynamic (Upload) 버퍼는 CPU 가 접근할 수 있도록 매핑된 포인터를 갖는다.
    };

    class IndexBuffer : public GPUResource {
    public:
        virtual uint32_t                GetOffset() const = 0;
        virtual uint32_t                GetSizeInBytes() const = 0;

        void *                          mappedPtr = nullptr; // Dynamic (Upload) 버퍼는 CPU 가 접근할 수 있도록 매핑된 포인터를 갖는다.
    };

    class ConstantBuffer : public GPUResource {
    public:
        void *                          mappedPtr = nullptr; // Dynamic (Upload) 버퍼는 CPU 가 접근할 수 있도록 매핑된 포인터를 갖는다.
    };

    class Texture : public GPUResource {
    public:
        virtual bool                    HasValidView(RHI::ViewType viewType, RHI::ViewID viewID = RHI::ViewID::PrimaryID) const = 0;
        virtual uint32_t                GetViewCount(RHI::ViewType viewType) const = 0;
        virtual RHI::ViewID             GetViewID(RHI::ViewType viewType, uint32_t viewIndex) const = 0;
        virtual bool                    GetViewInfo(RHI::ViewType viewType, RHI::ViewID viewID, TextureViewInfo &outInfo) const = 0;

        virtual uint32_t                GetBindlessIndex(RHI::ViewType viewType, RHI::ViewID viewID = RHI::ViewID::PrimaryID) const = 0;

        virtual Format                  GetFormat() const = 0;
        virtual uint32_t                GetWidth() const = 0;
        virtual uint32_t                GetHeight() const = 0;
        virtual uint32_t                GetDepth() const = 0;
        virtual uint32_t                GetArraySize() const = 0;
        virtual uint32_t                GetMipLevelCount() const = 0;
        virtual uint32_t                GetSampleCount() const = 0;

        TextureType                     type;
        TextureFlag                     flags = TextureFlag::None;
    };

    class Shader : public GPUObject {
    public:
        ShaderStage                     shaderStage = ShaderStage::Count;
    };

    class Sampler : public GPUObject {
    public:
        virtual uint32_t                GetBindlessIndex() const = 0;

        SamplerDesc                     desc = {};
    };

    class PipelineState : public GPUObject {
    public:
        uint64_t                        hash = 0;
        uint32_t                        sortIndex = 0;
        bool                            isGraphics;
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
            Texture,    // 텍스쳐의 상태 전이 배리어
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

        struct TextureBarrier {
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
            TextureBarrier              textureBarrier;
            AliasingBarrier             aliasingBarrier;
        };
    };

    struct RenderDest {
        uint32_t                        renderTargetCount = 0;
        Format                          renderTargetFormats[8] = {};
        Format                          depthStencilFormat = Format::Unknown;
        uint32_t                        sampleCount = 1;
        uint32_t                        sampleQuality = 0;

        uint64_t                        GetHash() const {
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
                    uint64_t sampleCount : 6;
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
            hasher.bits.sampleCount = (uint64_t)sampleCount;
            return hash_combine(hasher.value, sampleQuality);
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
        RHI::ViewID                     viewID = RHI::ViewID::PrimaryID;
        int                             resolveSourceIndex = 0;
        Type                            type = Type::Color;
        LoadAction                      loadAction = LoadAction::Load;
        StoreAction                     storeAction = StoreAction::Store;
        DepthResolveMode                depthResolveMode = DepthResolveMode::None;
        ClearValue                      clearValue = {};
        bool                            hasClearValue = false;
        GPUResourceState                beforeState = GPUResourceState::Undefined;
        GPUResourceState                duringState = GPUResourceState::Undefined;
        GPUResourceState                afterState = GPUResourceState::Undefined;

        RenderPassImage WithClearValue(const ClearValue &value) const {
            RenderPassImage image = *this;
            image.clearValue = value;
            image.hasClearValue = true;
            return image;
        }

        static RenderPassImage Color(const Texture *texture, RHI::ViewID viewID = RHI::ViewID::PrimaryID, LoadAction loadAction = LoadAction::Load, StoreAction storeAction = StoreAction::Store,
            GPUResourceState beforeState = GPUResourceState::ShaderResource, GPUResourceState afterState = GPUResourceState::ShaderResource) {
            RenderPassImage image;
            image.texture = texture;
            image.viewID = viewID;
            image.type = Type::Color;
            image.loadAction = loadAction;
            image.storeAction = storeAction;
            image.beforeState = beforeState;
            image.duringState = GPUResourceState::RenderTarget;
            image.afterState = afterState;
            return image;
        }

        static RenderPassImage DepthStencil(const Texture *texture, RHI::ViewID viewID = RHI::ViewID::PrimaryID, LoadAction loadAction = LoadAction::Load, StoreAction storeAction = StoreAction::Store,
            GPUResourceState beforeState = GPUResourceState::DepthWrite, GPUResourceState duringState = GPUResourceState::DepthWrite, GPUResourceState afterState = GPUResourceState::DepthWrite) {
            RenderPassImage image;
            image.texture = texture;
            image.viewID = viewID;
            image.type = Type::DepthStencil;
            image.loadAction = loadAction;
            image.storeAction = storeAction;
            image.beforeState = beforeState;
            image.duringState = duringState;
            image.afterState = afterState;
            return image;
        }

        static RenderPassImage ResolveColor(const Texture *texture, RHI::ViewID viewID = RHI::ViewID::PrimaryID, int resolveSourceIndex = 0,
            GPUResourceState beforeState = GPUResourceState::ShaderResource, GPUResourceState afterState = GPUResourceState::ShaderResource) {
            RenderPassImage image;
            image.texture = texture;
            image.viewID = viewID;
            image.resolveSourceIndex = resolveSourceIndex;
            image.type = Type::ResolveColor;
            image.beforeState = beforeState;
            image.duringState = GPUResourceState::CopyDst;
            image.afterState = afterState;
            return image;
        }

        static RenderPassImage ResolveDepth(const Texture *texture, RHI::ViewID viewID = RHI::ViewID::PrimaryID, DepthResolveMode depthResolveMode = DepthResolveMode::Min,
            GPUResourceState beforeState = GPUResourceState::ShaderResource, GPUResourceState afterState = GPUResourceState::ShaderResource) {
            RenderPassImage image;
            image.texture = texture;
            image.viewID = viewID;
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
        virtual Format                  GetFormat() const = 0;
        virtual Format                  GetRTVFormat() const = 0;
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

    enum class CommandListKind : uint8_t {
        Primary,
        Secondary,
        Count
    };

    class FrameThreadData;
    class CommandList {
    public:
        virtual ~CommandList() = 0 {}

                                        // 커맨드 리스트를 재사용하기 위해 Reset() 함수를 호출하여 초기화한다.
        virtual void                    Reset(bool resetCacheStates = true, const CommandList *primaryCommandList = nullptr) = 0;

        virtual void                    Close() = 0;
        virtual void                    Execute() = 0;
        virtual void                    CloseAndExecute() = 0;
        virtual void                    ExecuteSecondary(RHI::CommandList *secondaryCommandList) = 0;
    };

    class ImGuiOverlay {
    public:
        virtual ~ImGuiOverlay() = default;

        virtual bool                    BeginFrameCapture() = 0;
        virtual void *                  EndFrameCapture() = 0;

        virtual bool                    HandleWndProc(void *hwnd, uint32_t message, uintptr_t wParam, intptr_t lParam) = 0;
        virtual bool                    WantsCaptureMouse() const = 0;
        virtual bool                    WantsCaptureKeyboard() const = 0;
    };

    class FrameThreadData {
    public:
        virtual ~FrameThreadData() = 0 {}

        virtual void                    Reset() = 0;

        virtual VertexBuffer *          AllocVertex(uint32_t vertexSize, uint32_t count, const void *data = nullptr) = 0;
        virtual IndexBuffer *           AllocIndex(uint32_t indexSize, uint32_t count, const void *data = nullptr) = 0;
        virtual ConstantBuffer *        AllocConstant(uint32_t size, const void *data = nullptr) = 0;
        virtual Buffer *                AllocRawBuffer(uint32_t size, uint32_t alignSize, const void *data = nullptr) = 0;
        virtual Buffer *                AllocStructuredBuffer(uint32_t stride, uint32_t count, const void *data = nullptr) = 0;

        virtual void *                  AppendVertex(RHI::VertexBuffer *vertexBuffer, uint32_t vertexSize, uint32_t count, const void *data = nullptr) = 0;
        virtual void *                  AppendIndex(RHI::IndexBuffer *indexBuffer, uint32_t indexSize, uint32_t count, const void *data = nullptr) = 0;

        virtual CommandList *           AllocGraphicsCommandList(RHI::CommandListKind kind = RHI::CommandListKind::Primary) = 0;
        virtual CommandList *           AllocComputeCommandList() = 0;

        virtual CommandList *           BeginCommandList(RHI::CommandQueueType queueType) = 0;
        virtual CommandList *           BeginSecondaryCommandList(const RHI::CommandList *primaryCommandList) = 0;
    };

    enum class RasterizerStateType : uint8_t {
        SolidFrontSided,
        SolidBackSided,
        SolidTwoSided,
        Wire,
        WireSmooth,
        Count
    };

    enum class DepthStencilStateType : uint8_t {
        Disabled,
        LessWrite,
        LessNoWrite,
        LEqualWrite,
        LEqualNoWrite,
        EqualWrite,
        EqualNoWrite,
        GEqualWrite,
        GEqualNoWrite,
        GreaterWrite,
        GreaterNoWrite,
        Count
    };

    enum class BlendStateType : uint8_t {
        Disabled,
        AlphaBlend,
        AddBlend,
        Add,
        Modulate,
        Modulate2X,
        Count
    };

    class Renderer {
    public:
        virtual ~Renderer() = 0 {}

        virtual void                    Init(const void *mainWindowHandle);
        virtual void                    Shutdown();

        bool                            IsInitialized() const { return initialized; }
        bool                            IsValidationLayerEnabled() const { return validationLayerEnabled; }
        bool                            IsGpuValidationEnabled() const { return gpuValidationEnabled; }

        virtual void                    GetMemoryUsage(MemoryUsage &memoryUsage) const = 0;
        virtual const RendererInfo &    GetRendererInfo() const = 0;

        virtual uint64_t                SignalFence(CommandQueueType queueType) = 0;
        virtual bool                    IsFenceComplete(CommandQueueType queueType, uint64_t checkFenceValue) = 0;
        virtual void                    WaitFence(CommandQueueType queueType, uint64_t expectedFenceValue) = 0;
        virtual void                    Finish(CommandQueueType queueType) = 0;

        static bool                     ImageFormatToRHIFormat(BE1::Image::Format imageFormat, bool isSRGB, Format *rhiFormat);
        static bool                     RHIFormatToImageFormat(Format rhiFormat, BE1::Image::Format *imageFormat, bool *isSRGB);
        static bool                     IsSRGBFormat(Format format);
        static Format                   ToLinearFormat(Format format);
        static Format                   ToSRGBFormat(Format format);
        static bool                     IsDepthFormat(Format format);
        static bool                     IsStencilFormat(Format format);
        static bool                     IsDepthStencilFormat(Format format);
        static const char *             TextureTypeName(TextureType textureType);
        static const char *             FormatName(Format format);
        static const char *             BufferFlagName(BufferFlag flag);
        static const char *             TextureFlagName(TextureFlag flag);
        static const char *             ShaderModelName(ShaderModel shaderModel);

        virtual bool                    IsSupportedImageFormat(BE1::Image::Format imageFormat) const = 0;
        virtual BE1::Image::Format      ToUncompressedImageFormat(BE1::Image::Format imageFormat) const = 0;
        virtual BE1::Image::Format      ToCompressedImageFormat(BE1::Image::Format inFormat, bool useNormalMap) const = 0;
        virtual BE1::Image::Format      ToSupportedImageFormat(BE1::Image::Format imageFormat) const = 0;
        virtual BE1::Image::Format      ToUAVImageFormat(BE1::Image::Format inFormat) const = 0;
        virtual bool                    CanGenerateMipmapsOnGPU(TextureType textureType, BE1::Image::Format imageFormat, int arraySize) const = 0;

        virtual SwapChain *             CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height, Format format) = 0;
        virtual void                    DestroySwapChain(SwapChain *swapChain) = 0;

        virtual ImGuiOverlay *          CreateImGuiOverlay(void *windowHandle, SwapChain *swapChain) = 0;
        virtual void                    DestroyImGuiOverlay(ImGuiOverlay *overlay) = 0;
        virtual void                    RenderImGuiDrawData(FrameThreadData *frameThreadData, CommandList *commandList, ImGuiOverlay *overlay, void *userData) = 0;
        virtual void                    FreeImGuiDrawData(ImGuiOverlay *overlay, void *userData) = 0;

        virtual RHI::FrameThreadData *  CreateFrameThreadData(int32_t frameIndex, int32_t numFrames, int32_t threadIndex, int32_t numRenderTaskThreads) = 0;
        virtual void                    DestroyFrameThreadData(FrameThreadData *frameThreadData) = 0;

        virtual void                    DrainReleaseRequestsToRetireList() = 0;
        virtual void                    FreeRetiredResources(bool waitPendings = false) = 0;
        virtual void                    ProcessResourceRequests(FrameThreadData *frameThreadData) = 0;

        const RasterizerState *         GetRasterizerState(RasterizerStateType type) const { return &rasterizerStates[to_int(type)]; }
        const DepthStencilState *       GetDepthStencilState(DepthStencilStateType type) const { return &depthStencilStates[to_int(type)]; }
        const BlendState *              GetBlendState(BlendStateType type) const { return &blendStates[to_int(type)]; }

        virtual void                    SetName(GPUResource *resource, const char *name) const {}

        virtual Buffer *                CreateBuffer(BufferUsage usage, BufferFlag flags, uint64_t size, const void *data) = 0;
        virtual void                    DestroyBuffer(Buffer *buffer, bool immediate = false) = 0;

        virtual VertexBuffer *          CreateVertexBuffer(BufferUsage usage, uint32_t vertexSize, uint32_t numVerts, const void *data) = 0;
        virtual void                    DestroyVertexBuffer(VertexBuffer *vertexBuffer, bool immediate = false) = 0;

        virtual IndexBuffer *           CreateIndexBuffer(BufferUsage usage, uint32_t indexSize, uint32_t numIndexes, const void *data) = 0;
        virtual void                    DestroyIndexBuffer(IndexBuffer *indexBuffer, bool immediate = false) = 0;

        virtual ConstantBuffer *        CreateConstantBuffer(BufferUsage usage, uint32_t size, const void *data) = 0;
        virtual void                    DestroyConstantBuffer(ConstantBuffer *constantBuffer, bool immediate = false) = 0;

        void                            AdjustTextureFormat(bool useCompression, bool useNormalMap, bool useUAV, BE1::Image::Format inFormat, BE1::Image::Format *outFormat);
        void                            AdjustTextureSize(TextureType textureType, bool useNPOT, uint32_t inWidth, uint32_t inHeight, uint32_t inDepth, uint32_t *outWidth, uint32_t *outHeight, uint32_t *outDepth);

        virtual Texture *               CreateTexture(TextureType textureType, TextureFlag flags, std::shared_ptr<const BE1::Image> image, TextureMipInitMode mipInitMode, const ClearValue &clearValue = {}, uint32_t sampleCount = 1, GPUResourceState initialState = GPUResourceState::Undefined) = 0;
        virtual void                    DestroyTexture(Texture *texture, bool immediate = false) = 0;

        virtual bool                    SetBufferSubData(Buffer *buffer, uint64_t offset, uint64_t size, const void *data, GPUResourceState stateBefore, GPUResourceState stateAfter) = 0;

        virtual bool                    UploadTextureMipRange(Texture *texture, std::shared_ptr<const BE1::Image> image, int firstMip) = 0;

        virtual bool                    GetTextureImage2D(Texture *texture, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) = 0;
        virtual bool                    GetTextureImage3D(Texture *texture, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) = 0;
        virtual bool                    GetTextureImageCubeFace(Texture *texture, CubemapFace face, int mipLevel, BE1::Image::Format imageFormat, void *outPixels) = 0;

        virtual bool                    SetTextureSubImage2D(Texture *texture, int mipLevel, int x, int y, int width, int height, BE1::Image::Format imageFormat, const void *pixels) = 0;
        virtual bool                    SetTextureSubImage3D(Texture *texture, int mipLevel, int x, int y, int z, int width, int height, int depth, BE1::Image::Format imageFormat, const void *pixels) = 0;
        virtual bool                    SetTextureSubImageCubeFace(Texture *texture, CubemapFace face, int mipLevel, int x, int y, int width, int height, BE1::Image::Format imageFormat, const void *pixels) = 0;

        virtual ViewID                  CreateView(Buffer *buffer, ViewType subresourceType, Format format, uint32_t structuredStride, uint64_t offset = 0, uint64_t size = ~0) = 0;
        virtual ViewID                  CreateView(Texture *texture, ViewType type, Format typelessCompatibleFormat = Format::Auto, uint32_t firstSlice = 0, uint32_t sliceCount = ~0, uint32_t firstMipLevel = 0, uint32_t mipCount = ~0) = 0;

        virtual void                    DestroyView(Buffer *buffer, ViewType viewType, ViewID viewID) = 0;
        virtual void                    DestroyView(Texture *texture, ViewType viewType, ViewID viewID) = 0;

        virtual Shader *                CreateShader(ShaderModel shaderModel, ShaderStage shaderStage, const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint, const BE1::StrArray *defines = nullptr) = 0;
        virtual Shader *                CreateShaderFromFile(ShaderModel shaderModel, ShaderStage shaderStage, const char *filename, const char *entryPoint, const BE1::StrArray *defines = nullptr) = 0;
        virtual void                    DestroyShader(Shader *shader, bool immediate = false) = 0;

        virtual Sampler *               CreateSampler(const SamplerDesc *desc) = 0;
        virtual void                    DestroySampler(Sampler *sampler, bool immediate = false) = 0;
        virtual void                    RecreateSampler(Sampler *sampler, const SamplerDesc *desc) = 0;

        virtual PipelineState *         CreateGraphicsPSO(const PipelineStateDesc *desc) = 0;
        virtual PipelineState *         CreateComputePSO(const Shader *computeShader) = 0;
        virtual void                    DestroyPSO(PipelineState *pipelineState, bool immediate = false) = 0;

        virtual QueryHeap *             CreateQueryHeap(const QueryHeapDesc *desc) = 0;
        virtual void                    DestroyQueryHeap(QueryHeap *queryHeap, bool immediate = false) = 0;

        virtual void                    SetVertexBuffer(CommandList *commandList, int slot, const VertexBuffer *vertexBuffer) = 0;
        virtual void                    SetIndexBuffer(CommandList *commandList, const IndexBuffer *indexBuffer) = 0;
        virtual void                    SetConstantBuffer(CommandList *commandList, int shaderRegister, const ConstantBuffer *constantBuffer) = 0;
        virtual void                    SetConstants(CommandList *commandList, int shaderRegister, const void *data, uint32_t size, uint32_t offset) = 0;
        virtual void                    SetTexture(CommandList *commandList, int shaderRegister, bool shaderWritable, const Texture *texture, ViewID viewID = ViewID::PrimaryID) = 0;
        virtual void                    SetBuffer(CommandList *commandList, int shaderRegister, bool shaderWritable, const Buffer *buffer, ViewID viewID = ViewID::PrimaryID) = 0;
        virtual void                    SetSampler(CommandList *commandList, int shaderRegister, Sampler *sampler) = 0;
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
        virtual uint64_t                GetTimestampFrequency(CommandQueueType queueType) const = 0;
        virtual void                    ClearUAV(CommandList *commandList, const GPUResource *resource, ViewID viewID, uint32_t value) = 0;
        virtual void                    CopyBuffer(CommandList *commandList, const Buffer *dstBuffer, uint32_t dstOffset, const Buffer *srcBuffer, uint32_t srcOffset, uint32_t size) = 0;
        virtual void                    CopyTexture(CommandList *commandList, const Texture *dstTexture, uint32_t dstSlice, uint32_t dstMipLevel, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const Texture *srcTexture, uint32_t srcSlice, uint32_t srcMipLevel, uint32_t srcX, uint32_t srcY, uint32_t srcZ, uint32_t width, uint32_t height, uint32_t depth) = 0;
        virtual void                    GenerateMipChain(CommandList *commandList, const Texture *texture, const ViewID *srvMipSubresourceIDs, const ViewID *uavMipSubresourceIDs, bool preserveCoverage) = 0;
        virtual void                    Barrier(CommandList *commandList, const GPUBarrier *barriers, uint32_t barrierCount) = 0;
        void                            Barrier(CommandList *commandList, const GPUBarrier &barrier) { Barrier(commandList, &barrier, 1); }
        virtual void                    ReadPixels(CommandList *commandList, const SwapChain *swapChain, int x, int y, int width, int height, BE1::Image::Format dstFormat, void *outPixels) = 0;
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
        static GPUBarrier               MakeTextureBarrier(const Texture *texture, GPUResourceState stateBefore, GPUResourceState stateAfter, int slice = -1, int mipLevel = -1);
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
        bool                            validationLayerEnabled = false;
        bool                            gpuValidationEnabled = false;
        bool                            initialized = false;
    };

    extern Renderer *                   renderer;
    bool                                CreateRenderer(GraphicsAPI graphicsAPI = GraphicsAPI::Default);
    void                                DestroyRenderer();

    class ScopedProfilerEvent {
    public:
        ScopedProfilerEvent() = delete;
        ScopedProfilerEvent(CommandList *commandList, const char *string, uint8_t colorIndex) : eventCommandList(commandList) { renderer->BeginEvent(commandList, string, colorIndex); }
        ScopedProfilerEvent(const ScopedProfilerEvent &) = delete;
        ScopedProfilerEvent &operator=(const ScopedProfilerEvent &) = delete;
        ~ScopedProfilerEvent() { renderer->EndEvent(eventCommandList); }

    private:
        CommandList *                   eventCommandList = nullptr;
    };
};

template<>
struct enable_bitmask_operators<RHI::ClearFlag> {
    static constexpr bool enable = true;
};

template<>
struct enable_bitmask_operators<RHI::ColorWriteMask> {
    static constexpr bool enable = true;
};

template<>
struct enable_bitmask_operators<RHI::BufferFlag> {
    static constexpr bool enable = true;
};

template<>
struct enable_bitmask_operators<RHI::TextureFlag> {
    static constexpr bool enable = true;
};

template<>
struct enable_bitmask_operators<RHI::GPUResourceState> {
    static constexpr bool enable = true;
};

template<>
struct enable_bitmask_operators<RHI::RenderPassFlag> {
    static constexpr bool enable = true;
};

namespace std {
    template <>
    struct hash<RHI::InputLayoutElement> {
        size_t operator()(const RHI::InputLayoutElement &element) const {
            size_t hash = element.semanticName.ToHash64();
            hash = hash_combine(hash, element.semanticIndex);
            hash = hash_combine(hash, element.offset);
            hash = hash_combine(hash, element.inputSlot);
            hash = hash_combine(hash, element.format);
            return hash;
        }
    };

    template <>
    struct hash<RHI::RasterizerState> {
        size_t operator()(const RHI::RasterizerState &state) const {
            size_t hash = std::hash<uint8_t>()(static_cast<uint8_t>(state.fillMode));
            hash = hash_combine(hash, state.cullMode);
            hash = hash_combine(hash, state.depthBias);
            hash = hash_combine(hash, state.depthBiasClamp);
            hash = hash_combine(hash, state.slopeScaledDepthBias);
            hash = hash_combine(hash, state.depthClipEnabled);
            hash = hash_combine(hash, state.smoothLineEnabled);
            hash = hash_combine(hash, state.conservativeRasterization);
            return hash;
        }
    };

    template <>
    struct hash<RHI::DepthStencilOp> {
        size_t operator()(const RHI::DepthStencilOp &op) const {
            size_t hash = std::hash<uint8_t>()(static_cast<uint8_t>(op.failOp));
            hash = hash_combine(hash, op.depthFailOp);
            hash = hash_combine(hash, op.passOp);
            hash = hash_combine(hash, op.stencilFunc);
            return hash;
        }
    };

    template <>
    struct hash<RHI::DepthStencilState> {
        size_t operator()(const RHI::DepthStencilState &state) const {
            size_t hash = std::hash<uint8_t>()(static_cast<uint8_t>(state.depthWriteMask));
            hash = hash_combine(hash, state.depthFunc);
            hash = hash_combine(hash, state.stencilReadMask);
            hash = hash_combine(hash, state.stencilWriteMask);
            hash = hash_combine(hash, state.frontFace);
            hash = hash_combine(hash, state.backFace);
            hash = hash_combine(hash, state.depthTestEnabled);
            hash = hash_combine(hash, state.stencilTestEnabled);
            hash = hash_combine(hash, state.depthBoundTestEnabled);
            return hash;
        }
    };

    template <>
    struct hash<RHI::RenderTargetBlendState> {
        size_t operator()(const RHI::RenderTargetBlendState &state) const {
            size_t hash = std::hash<uint8_t>()(static_cast<uint8_t>(state.srcFactorColor));
            hash = hash_combine(hash, state.destFactorColor);
            hash = hash_combine(hash, state.blendOpColor);
            hash = hash_combine(hash, state.srcFactorAlpha);
            hash = hash_combine(hash, state.destFactorAlpha);
            hash = hash_combine(hash, state.blendOpAlpha);
            hash = hash_combine(hash, state.colorWriteMask);
            hash = hash_combine(hash, state.blendEnabled);
            return hash;
        }
    };

    template <>
    struct hash<RHI::BlendState> {
        size_t operator()(const RHI::BlendState &state) const {
            size_t hash = std::hash<bool>()(static_cast<bool>(state.alphaToCoverageEnabled));
            hash = hash_combine(hash, state.independentBlendEnabled);
            for (int i = 0; i < COUNT_OF(state.renderTargets); ++i) {
                hash = hash_combine(hash, state.renderTargets[i]);
            }
            return hash;
        }
    };
}
