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

    enum class ColorWrite {
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
        DestAlpha,
        InvDestAlpha,
        DestColor,
        InvDestColor,
        SrcAlphaSat,
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
        TriangleStrip
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

    enum class ShaderStage : uint8_t {
        Vertex,
        Fragment,
        Geometry,
        Compute,
        Count
    };

    class Resource {
    public:
        virtual ~Resource() {}
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

    struct RasterizerState {
        FillMode                    fillMode = FillMode::Solid;
        CullMode                    cullMode = CullMode::None;
        int32_t                     depthBias = 0;
        float                       depthBiasClamp = 0;
        float                       slopeScaledDepthBias = 0;
        bool                        depthClipEnabled = false;
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
        ColorWrite                  colorWriteMask = ColorWrite::All;
    };

    struct BlendState {
        bool                        alphaToCoverageEnabled = false;
        bool                        independentBlendEnabled = false;
        RenderTargetBlendState      renderTargets[8];
    };

    struct PipelineStateDesc {
        const Shader *              vs = nullptr;
        const Shader *              ps = nullptr;
        const Shader *              gs = nullptr;
        const Shader *              cs = nullptr;
        const BlendState *          blendState = nullptr;
        const RasterizerState *     rasterizerState = nullptr;
        const DepthStencilState *   depthStencilState = nullptr;
        PrimitiveTopology           primitiveTopology = PrimitiveTopology::TriangleList;
    };

    struct PipelineState {
        PipelineStateDesc           desc;
    };
};
