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

class RHIRenderer {
public:
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

    enum class ColorWriteMask {
        Red                         = BIT(0),
        Green                       = BIT(1),
        Blue                        = BIT(2),
        Alpha                       = BIT(3),
        All                         = Red | Green | Blue | Alpha,
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

    struct BufferFlag {
        enum Enum {
            None                    = 0,
            ConstantBuffer          = BIT(0),
            VertexBuffer            = BIT(1),
            IndexBuffer             = BIT(2),
            UnorderedAccess         = BIT(3),
            ShaderResource          = BIT(4)
        };
    };

    enum class BufferType : uint8_t {
        Static,
        Dynamic
    };

    enum class TextureType : uint8_t {
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube,
        TextureCubeArray,
        TextureBuffer
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
        TextureFilter               filter = TextureFilter::NearestMipmapNearest;
        TextureAddressMode          addressModeU = TextureAddressMode::Clamp;
        TextureAddressMode          addressModeV = TextureAddressMode::Clamp;
        TextureAddressMode          addressModeW = TextureAddressMode::Clamp;
        float                       mipLodBias = 0.0f;
        uint32_t                    maxAnisotropy = 0;
        TextureBorderColor          borderColor = TextureBorderColor::OpaqueBlack;
        float                       minLod = 0.0f;
        float                       maxLod = std::numeric_limits<float>::max();
    };

    struct ShaderCompileInput {
        ShaderFormat                shaderFormat;
        ShaderModel                 shaderModel;
        ShaderStage                 shaderStage;
        const char *                sourceName;
        const char *                shaderText;
        int                         shaderTextSize;
        const char *                entryPoint;
    };

    struct ShaderCompileOutput {
        byte *                      compiledShaderData = nullptr;
        uint32_t                    compiledShaderDataSize = 0;
        Str                         errorMessage;
    };

    struct RasterizerState {
        FillMode                    fillMode = FillMode::Solid;
        CullMode                    cullMode = CullMode::None;
        int32_t                     depthBias = 0;
        float                       depthBiasClamp = 0;
        float                       slopeScaledDepthBias = 0;
        bool                        depthClipEnabled = false;
        bool                        smoothLineEnabled = false;
        bool                        conservativeRasterization = false;
    };

    struct DepthStencilOp {
        StencilOp                   failOp = StencilOp::Keep;
        StencilOp                   depthFailOp = StencilOp::Keep;
        StencilOp                   passOp = StencilOp::Keep;
        ComparisonFunc              stencilFunc = ComparisonFunc::Never;
    };

    struct DepthStencilState {
        bool                        depthTestEnabled = false;
        DepthWriteMask              depthWriteMask = DepthWriteMask::Zero;
        ComparisonFunc              depthFunc = ComparisonFunc::Never;
        bool                        stencilTestEnabled = false;
        uint8_t                     stencilReadMask = 0xff;
        uint8_t                     stencilWriteMask = 0xff;
        DepthStencilOp              frontFace;
        DepthStencilOp              backFace;
        bool                        depthBoundTestEnabled = false;
    };

    struct RenderTargetBlendState {
        bool                        blendEnabled = false;
        Blend                       srcFactorColor = Blend::SrcAlpha;
        Blend                       destFactorColor = Blend::InvSrcAlpha;
        BlendOp                     blendOpColor = BlendOp::Add;
        Blend                       srcFactorAlpha = Blend::One;
        Blend                       destFactorAlpha = Blend::One;
        BlendOp                     blendOpAlpha = BlendOp::Add;
        ColorWriteMask              colorWriteMask = ColorWriteMask::All;
    };

    struct BlendState {
        bool                        alphaToCoverageEnabled = false;
        bool                        independentBlendEnabled = false;
        RenderTargetBlendState      renderTargets[8];
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

        Str                         semanticName;
        uint32_t                    semanticIndex = 0;
        uint32_t                    offset = 0;
        uint32_t                    inputSlot = 0;
        Format                      format = Format::Unknown;
    };

    struct InputLayout {
        Array<InputLayoutElement>   elements;
    };

    enum class QueryType : uint8_t {
        TimeStamp,
        Occlusion,
        OcclusionBinary
    };

    struct QueryHeapDesc {
        QueryType                   type = QueryType::TimeStamp;
        uint32_t                    queryCount = 0;
    };

    class GPUResource {
    public:
        virtual ~GPUResource() = default;
    };

    class Buffer : public GPUResource {
    public:
    };

    class VertexBuffer : public GPUResource {
    public:
        BufferType                  bufferType;
    };

    class IndexBuffer : public GPUResource {
    public:
        BufferType                  bufferType;
    };

    class ConstantBuffer : public GPUResource {
    public:
        BufferType                  bufferType;
    };

    class Texture : public GPUResource {
    public:
        TextureType                 textureType;
    };

    class Shader : public GPUResource {
    public:
        ShaderStage                 shaderStage = ShaderStage::Count;
    };

    class Sampler : public GPUResource {
    public:
        SamplerDesc                 desc = {};
    };

    class PipelineState : public GPUResource {
    public:
        uint64_t                    hash = 0;
    };

    class QueryHeap : public GPUResource {
    public:
        QueryHeapDesc               desc = {};
    };

    class GPUSubResource {
    public:
        void *                      writePtr = nullptr;
    };

    struct RenderPass {
        uint32_t                    renderTargetCount = 0;
        Image::Format::Enum         renderTargetFormats[8] = {};
        Image::Format::Enum         depthStencilFormat = Image::Format::Unknown;
        uint32_t                    sampleCount = 1;

        constexpr uint64_t          GetHash() const {
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
        const Shader *              vs = nullptr;
        const Shader *              ps = nullptr;
        const Shader *              ds = nullptr;
        const Shader *              hs = nullptr;
        const Shader *              gs = nullptr;
        const InputLayout *         inputLayout;
        PrimitiveTopology           primitiveTopology = PrimitiveTopology::TriangleList;
        const RasterizerState *     rasterizerState = nullptr;
        const DepthStencilState *   depthStencilState = nullptr;
        const BlendState *          blendState = nullptr;
        const RenderPass *          renderPass;
        uint32_t                    sampleMask = 0xffffffff;
        uint32_t                    sampleCount = 1;
        uint32_t                    sampleQuality = 0;
    };

    enum class CommandQueueType : uint8_t {
        Graphics,
        Compute,
        Count
    };

    class CommandList {
    public:
        virtual void                Reset(bool resetCacheStates = true) = 0;

        virtual void                CloseAndExecute(RHIRenderer::CommandQueueType queueType) = 0;

        virtual int                 GetThreadIndex() const = 0;
    };

    struct RasterizerStateType {
        enum Enum {
            SolidFrontSided,
            SolidBackSided,
            Wire,
            WireSmooth,
            Count
        };
    };

    struct DepthStencilStateType {
        enum Enum {
            Default,
            Count
        };
    };

    struct BlendStateType {
        enum Enum {
            Opaque,
            AlphaBlend,
            Add,
            Count
        };
    };

    virtual void                    Init(HWND hwnd);
    virtual void                    Shutdown();

    bool                            IsInitialized() const { return initialized; }

    virtual void                    BeginFrame() = 0;
    virtual void                    EndFrame() = 0;
    virtual void                    SwapChainBuffers(bool vsync) = 0;

    virtual void                    OnResize(int width, int height) = 0;

    const RasterizerState *         GetRasterizerState(RasterizerStateType::Enum type) const { return &rasterizerStates[type]; }
    const DepthStencilState *       GetDepthStencilState(DepthStencilStateType::Enum type) const { return &depthStencilStates[type]; }
    const BlendState *              GetBlendState(BlendStateType::Enum type) const { return &blendStates[type]; }

    virtual Buffer *                CreateBuffer(BufferUsage usage, int flags, int size) = 0;
    virtual void                    DestroyBuffer(Buffer *buffer, bool immediate = false) = 0;

    virtual VertexBuffer *          CreateVertexBuffer(BufferType type, int vertexSize, int numVerts, void *data) = 0;
    virtual void                    DestroyVertexBuffer(VertexBuffer *vertexBuffer, bool immediate = false) = 0;

    virtual IndexBuffer *           CreateIndexBuffer(BufferType type, int indexSize, int numIndexes, void *data) = 0;
    virtual void                    DestroyIndexBuffer(IndexBuffer *indexBuffer, bool immediate = false) = 0;

    virtual ConstantBuffer *        CreateConstantBuffer(BufferType type, int size, void *data) = 0;
    virtual void                    DestroyConstantBuffer(ConstantBuffer *constantBuffer, bool immediate = false) = 0;

    virtual Texture *               CreateTexture(TextureType textureType, const Image *image) = 0;
    virtual Texture *               CreateTexture(TextureType textureType, const Image *image, Image::Format::Enum dstFormat, bool useMipmaps) = 0;
    virtual Texture *               CreateTextureFromFile(TextureType textureType, const char *filename, bool useCompression = true, bool useNormalMap = false) = 0;
    virtual void                    DestroyTexture(Texture *texture, bool immediate = false) = 0;
    virtual void                    GetTextureImage2D(Texture *texture, int level, Image::Format::Enum imageFormat, void *outPixels) = 0;
    virtual bool                    SetTextureSubImage2D(Texture *texture, int level, int x, int y, int width, int height, Image::Format::Enum imageFormat, const void *pixels) = 0;
    virtual bool                    SetTextureSubImage3D(Texture *texture, int level, int x, int y, int z, int width, int height, int depth, Image::Format::Enum imageFormat, const void *pixels) = 0;

    virtual Shader *                CreateShader(ShaderModel shaderModel, ShaderStage shaderStage, const char *sourceName, const char *shaderText, int shaderTextSize, const char *entryPoint) = 0;
    virtual Shader *                CreateShaderFromFile(ShaderModel shaderModel, ShaderStage shaderStage, const char *filename, const char *entryPoint) = 0;
    virtual void                    DestroyShader(Shader *shader, bool immediate = false) = 0;

    virtual Sampler *               CreateSampler(const SamplerDesc *desc) = 0;
    virtual void                    DestroySampler(Sampler *sampler, bool immediate = false) = 0;

    virtual PipelineState *         CreatePSO(const PipelineStateDesc *desc) = 0;
    virtual void                    DestroyPSO(PipelineState *pipelineState, bool immediate = false) = 0;

    virtual QueryHeap *             CreateQueryHeap(const QueryHeapDesc *desc) = 0;
    virtual void                    DestroyQueryHeap(QueryHeap *queryHeap, bool immediate = false) = 0;

    virtual void                    SetVertexBuffer(CommandList *commandList, int slot, const VertexBuffer *vertexBuffer) = 0;
    virtual void                    SetIndexBuffer(CommandList *commandList, const IndexBuffer *indexBuffer) = 0;
    virtual void                    SetConstantBuffer(CommandList *commandList, int slot, const ConstantBuffer *constantBuffer) = 0;
    virtual void                    SetConstants(CommandList *commandList, const void *data, uint32_t size, uint32_t offset) = 0;
    virtual void                    SetTexture(CommandList *commandList, int slot, const Texture *texture) = 0;
    virtual void                    SetSubResource(CommandList *commandList, int slot, GPUSubResource *subResource) = 0;
    virtual void                    SetSampler(CommandList *commandList, int slot, Sampler *sampler) = 0;
    virtual void                    SetPSO(CommandList *commandList, const PipelineState *pipelineState) = 0;
    virtual void                    SetBlendFactor(CommandList *commandList, const Color4 &rgba) = 0;
    virtual void                    SetStencilRef(CommandList *commandList, uint32_t value) = 0;
    virtual void                    SetShadingRate(CommandList *commandList, ShadingRate shadingRate) = 0;
    virtual void                    SetViewport(CommandList *commandList, const Rect &viewportRect) = 0;
    virtual void                    SetScissorRect(CommandList *commandList, const Rect &scissorRect) = 0;
    virtual void                    SetDepthBounds(CommandList *commandList, float depthMin, float depthMax) = 0;
    virtual void                    BeginQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index) = 0;
    virtual void                    EndQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index) = 0;
    virtual void                    ResolveQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index, uint32_t count, const Buffer *destBuffer, uint64_t destOffset) = 0;
    virtual void                    ResetQuery(CommandList *commandList, const QueryHeap *queryHeap, uint32_t index, uint32_t count) = 0;

    virtual void                    Draw(CommandList *commandList, uint32_t vertexCount, uint32_t startVertexLocation) = 0;
    virtual void                    DrawIndexed(CommandList *commandList, uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation) = 0;
    virtual void                    DrawInstanced(CommandList *commandList, uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;
    virtual void                    DrawIndexedInstanced(CommandList *commandList, uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation) = 0;

protected:
    void                            SetupStates();

    RasterizerState                 rasterizerStates[RasterizerStateType::Count];
    DepthStencilState               depthStencilStates[DepthStencilStateType::Count];
    BlendState                      blendStates[BlendStateType::Count];
    bool                            initialized = false;
};

namespace std {
    template <>
    struct hash<RHIRenderer::InputLayoutElement> {
        size_t operator()(const RHIRenderer::InputLayoutElement &element) const {
            size_t hash = element.semanticName.ToHash64();
            hash = hash_combine(hash, std::hash<uint32_t>()(element.semanticIndex));
            hash = hash_combine(hash, std::hash<uint32_t>()(element.offset));
            hash = hash_combine(hash, std::hash<uint32_t>()(element.inputSlot));
            hash = hash_combine(hash, std::hash<uint8_t>()(static_cast<uint8_t>(element.format)));
            return hash;
        }
    };
}
