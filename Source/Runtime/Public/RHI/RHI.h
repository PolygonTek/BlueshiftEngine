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

/*
===============================================================================
 
    RHI (Rendering Hardware Interface)
 
===============================================================================
*/

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class RHI {
public:
    static constexpr int MaxTMU             = 16;       // maximum texture map units
    static constexpr int MaxTCU             = 8;        // maximum texture coordinates units
    static constexpr int MaxVertexStream    = 8;

    enum {
        NullContext                         = 0,
        NullStencilState                    = 0,
        NullTexture                         = 0,
        NullRenderTarget                    = 0,
        NullBuffer                          = 0,
        NullSync                            = 0,
        NullShader                          = 0,
        NullVertexFormat                    = 0,
        NullQuery                           = 0
    };

    enum StateBits {
        //----------------------------------------------------------------------------------------------
        // polygon mode
        //----------------------------------------------------------------------------------------------
        PM_Solid                            = 0x00000000,
        PM_Wireframe                        = 0x00000001,
        PM_Point                            = 0x00000002,
        
        //----------------------------------------------------------------------------------------------
        // depth mask: enabled (if set) or disabled (else)
        //----------------------------------------------------------------------------------------------
        DepthWrite                          = 0x00000004,

        //----------------------------------------------------------------------------------------------
        // color mask
        //----------------------------------------------------------------------------------------------
        RedWrite                            = 0x00000010,
        GreenWrite                          = 0x00000020,
        BlueWrite                           = 0x00000040,
        AlphaWrite                          = 0x00000080,
        ColorWrite                          = RedWrite | GreenWrite | BlueWrite,

        //----------------------------------------------------------------------------------------------
        // depth func
        //----------------------------------------------------------------------------------------------
        DF_None                             = 0x00000000,
        DF_Always                           = 0x00000100,
        DF_Less                             = 0x00000200,
        DF_LEqual                           = 0x00000300,
        DF_Equal                            = 0x00000400,
        DF_NotEqual                         = 0x00000500,
        DF_GEqual                           = 0x00000600,
        DF_Greater                          = 0x00000700,
        DF_Never                            = 0x00000800,

        //----------------------------------------------------------------------------------------------
        // source blending factor
        //----------------------------------------------------------------------------------------------
        BS_None                             = 0x00000000,
        BS_Zero                             = 0x00001000,
        BS_One                              = 0x00002000,
        BS_DstColor                         = 0x00003000,
        BS_OneMinusDstColor                 = 0x00004000,
        BS_SrcAlpha                         = 0x00005000,
        BS_OneMinusSrcAlpha                 = 0x00006000,
        BS_DstAlpha                         = 0x00007000,
        BS_OneMinusDstAlpha                 = 0x00008000,
        BS_SrcAlphaSaturate                 = 0x00009000,
        BS_ConstantColor                    = 0x0000A000,
        BS_OneMinusConstantColor            = 0x0000B000,

        //----------------------------------------------------------------------------------------------
        // dest blending factor
        //----------------------------------------------------------------------------------------------
        BD_None                             = 0x00000000,
        BD_Zero                             = 0x00010000,
        BD_One                              = 0x00020000,
        BD_SrcColor                         = 0x00030000,
        BD_OneMinusSrcColor                 = 0x00040000,
        BD_SrcAlpha                         = 0x00050000,
        BD_OneMinusSrcAlpha                 = 0x00060000,
        BD_DstAlpha                         = 0x00070000,
        BD_OneMinusDstAlpha                 = 0x00080000,
        BD_ConstantColor                    = 0x000A0000,
        BD_OneMinusConstantColor            = 0x000B0000,

        //----------------------------------------------------------------------------------------------
        // state bits mask
        //----------------------------------------------------------------------------------------------
        MaskPM                              = 0x00000003,   // polygon mode mask
        MaskColor                           = 0x000000F0,   // color mask
        MaskDF                              = 0x00000F00,   // depth func mask
        MaskBS                              = 0x0000F000,   // blend source mask
        MaskBD                              = 0x000F0000,   // blend dest mask
        MaskBF                              = 0x000FF000,   // blend func mask
    };

    struct ClearBit {
        enum Enum {
            Color                           = BIT(0),
            Depth                           = BIT(1),
            Stencil                         = BIT(2)
        };
    };

    struct CullType {
        enum Enum {
            Back,
            Front,
            None
        };
    };

    struct StencilFunc {
        enum Enum {
            Always,
            Never,
            Less,
            LEqual,
            Greater,
            GEqual,
            Equal,
            NotEqual
        };
    };

    struct StencilOp {
        enum Enum {
            Keep,
            Zero,
            Replace,
            Incr,
            IncrWrap,
            Decr,
            DecrWrap,
            Invert
        };
    };

    struct BufferType {
        enum Enum {
            Vertex,
            Index,
            PixelPack,
            PixelUnpack,
            Texel,
            Uniform,
            TransformFeedback,
            CopyRead,
            DrawIndirect,
            Count
        };
    };

    struct BufferUsage {
        enum Enum {
            Static,
            Dynamic,
            Stream
        };
    };

    struct BufferLockMode {
        enum Enum {
            WriteOnly,
            WriteOnlyExplicitFlush,
            WriteOnlyPersistent
        };
    };

    struct TextureType {
        enum Enum {
            Texture2D,
            TextureRectangle,
            Texture3D,
            TextureCubeMap,
            Texture2DArray,
            TextureBuffer
        };
    };

    struct CubeMapFace {
        enum Enum {
            PositiveX,
            NegativeX,
            PositiveY,
            NegativeY,
            PositiveZ,
            NegativeZ
        };
    };

    struct TextureFilter {
        enum Enum {
            Nearest,
            Linear,
            NearestMipmapNearest,
            LinearMipmapNearest,
            NearestMipmapLinear,
            LinearMipmapLinear
        };
    };

    struct AddressMode {
        enum Enum {
            Repeat,
            MirroredRepeat,
            Clamp,
            ClampToBorder
        };
    };

    struct RenderTargetType {
        enum Enum {
            RT2D,
            RTCubeMap,
            RT2DArray
        };
    };

    struct RenderTargetFlag {
        enum Enum {
            HasColorBuffer                  = BIT(0),
            HasDepthBuffer                  = BIT(1),
            HasStencilBuffer                = BIT(2),
            SRGBWrite                       = BIT(3)
        };
    };

    struct BlitMask {
        enum Enum {
            Color                           = BIT(0),
            Depth                           = BIT(1)
        };
    };

    struct BlitFilter {
        enum Enum {
            Nearest,
            Linear
        };
    };

    struct QueryType {
        enum Enum {
            Undefined,
            Occlusion,      // Result is the number of samples that are not culled
            Timestamp       // Result is time in micro seconds (1/1000000 sec)
        };
    };

    // Shader type.
    struct ShaderType {
        enum Enum {
            Vertex,
            Fragment,
            Geometry,
            TessControl,
            TessEval,
            Compute
        };
    };

    // Primitive topology.
    struct Topology {
        enum Enum {
            TriangleList                    = 0,
            TriangleFan                     = 1,
            TriangleStrip                   = 2,
            LineList                        = 3,
            LineStrip                       = 4,
            LineLoop                        = 5,
            PointList                       = 6
        };
    };

    struct VertexElement {
        struct Type {
            enum Enum {
                Byte                        = 0,
                UByte                       = 1,
                Int                         = 2,
                UInt                        = 3,
                Float                       = 4,
                Half                        = 5,
            };
        };

        struct Usage {
            enum Enum {
                Position                    = 0,
                Normal                      = 1,
                Color                       = 2,
                SecondaryColor              = 3,
                Tangent                     = 3,
                WeightIndex                 = 4,
                WeightIndex0                = 4,
                WeightIndex1                = 5,
                WeightValue                 = 6,
                WeightValue0                = 6,
                WeightValue1                = 7,
                TexCoord                    = 8,
                TexCoord0                   = 8,
                TexCoord1                   = 9,
                TexCoord2                   = 10,
                TexCoord3                   = 11,
                TexCoord4                   = 12,
                TexCoord5                   = 13,
                TexCoord6                   = 14,
                TexCoord7                   = 15,
                Count
            };
        };
        
        int                 stream;
        int                 offset;
        Usage::Enum         usage;
        int                 components;
        Type::Enum          type;
        bool                normalize;
        int                 divisor;
    };

    struct SamplerInfo {
        Str                 name;
        unsigned int        unit;
        bool                isPublic;
    };

    struct DrawArraysIndirectCommand {
        uint32_t            vertexCount;
        uint32_t            instanceCount;
        uint32_t            firstVertex;
        uint32_t            baseInstance;
    };

    struct DrawElementsIndirectCommand {
        uint32_t            vertexCount;
        uint32_t            instanceCount;
        uint32_t            firstIndex;
        uint32_t            baseVertex;
        uint32_t            baseInstance;
    };

    struct HWLimit {
        int                 maxTextureSize;
        int                 max3dTextureSize;
        int                 maxCubeMapTextureSize;
        int                 maxRectangleTextureSize;
        int                 maxTextureBufferSize;
        int                 maxTextureAnisotropy;
        int                 maxTextureImageUnits;

        int                 maxVertexAttribs;
        int                 maxVertexUniformComponents;
        int                 maxVertexUniformVectors;
        int                 maxVertexTextureImageUnits;
        int                 maxFragmentUniformComponents;
        int                 maxFragmentUniformVectors;
        int                 maxFragmentInputComponents;
        int                 maxGeometryTextureImageUnits;
        int                 maxGeometryOutputVertices;
        int                 maxUniformBufferBindings;
        int                 maxUniformBlockSize;
        int                 uniformBufferOffsetAlignment;

        int                 maxRenderBufferSize;
        int                 maxColorAttachments;
        int                 maxDrawBuffers;

        int                 nvVertexProgramVersion;
        int                 nvFragmentProgramVersion;
    };

    struct DisplayMetrics {
        int                 screenWidth;
        int                 screenHeight;
        int                 backingWidth;
        int                 backingHeight;
        Rect                safeAreaInsets;
    };

    struct Settings {
        int                 colorBits;
        int                 alphaBits;
        int                 depthBits;
        int                 stencilBits;
        int                 multiSamples;
    };

    using Handle = int;
    using WindowHandle = void *;
    using DisplayContextFunc = void (*)(Handle ctxHandle, void *dataPtr);
};

BE_NAMESPACE_END
