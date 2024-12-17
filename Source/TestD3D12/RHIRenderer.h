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

    class Resource {
    public:
        virtual ~Resource() = default;
    };

    class Buffer : public Resource {
    public:
    };

    class VertexBuffer : public Resource {
    public:
    };

    class IndexBuffer : public Resource {
    public:
    };

    class ConstantBuffer : public Resource {
    public:
    };

    class Texture : public Resource {
    public:
    };

    class Shader : public Resource {
    public:
        ShaderStage                 shaderStage = ShaderStage::Count;
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

    class PipelineState : public Resource {
    public:
        uint64_t                    hash = 0;
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

    const RasterizerState *         GetRasterizerState(RasterizerStateType::Enum type) const { return &rasterizerStates[type]; }
    const DepthStencilState *       GetDepthStencilState(DepthStencilStateType::Enum type) const { return &depthStencilStates[type]; }
    const BlendState *              GetBlendState(BlendStateType::Enum type) const { return &blendStates[type]; }

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
