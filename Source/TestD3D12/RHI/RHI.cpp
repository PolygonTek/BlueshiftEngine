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

#include "Precompiled.h"
#include "RHI.h"

#if defined(_WIN32)
#include "D3D12/D3D12Renderer.h"
#endif

namespace RHI {
    Renderer *renderer = nullptr;

    const ViewID ViewID::InvalidID = { ViewID::InvalidValue };
    const ViewID ViewID::PrimaryID = { ViewID::PrimaryValue };

    bool CreateRenderer(GraphicsAPI graphicsAPI) {
        assert(!renderer);

        if (graphicsAPI == GraphicsAPI::Default) {
#if defined(_WIN32)
            graphicsAPI = GraphicsAPI::D3D12;
#else
            return false;
#endif
        }

        switch (graphicsAPI) {
#if defined(_WIN32)
        case GraphicsAPI::D3D12:
            renderer = new D3D12Renderer;
            return true;
#endif
        default:
            return false;
        }
    }

    void DestroyRenderer() {
        SAFE_DELETE(renderer);
    }
}

const char *RHI::Renderer::TextureTypeName(RHI::TextureType textureType) {
    switch (textureType) {
    case RHI::TextureType::Unknown: return "Unknown";
    case RHI::TextureType::Texture1D: return "Texture1D";
    case RHI::TextureType::Texture1DArray: return "Texture1DArray";
    case RHI::TextureType::Texture2D: return "Texture2D";
    case RHI::TextureType::Texture2DArray: return "Texture2DArray";
    case RHI::TextureType::TextureCube: return "TextureCube";
    case RHI::TextureType::TextureCubeArray: return "TextureCubeArray";
    case RHI::TextureType::Texture3D: return "Texture3D";
    }
    return "Invalid";
}

const char *RHI::Renderer::TextureFilterName(RHI::TextureFilter textureFilter) {
    switch (textureFilter) {
    case RHI::TextureFilter::NearestMipmapNearest: return "NearestMipmapNearest";
    case RHI::TextureFilter::LinearMipmapNearest: return "LinearMipmapNearest";
    case RHI::TextureFilter::NearestMipmapLinear: return "NearestMipmapLinear";
    case RHI::TextureFilter::LinearMipmapLinear: return "LinearMipmapLinear";
    case RHI::TextureFilter::Anisotropic: return "Anisotropic";
    }
    return "Invalid";
}

const char *RHI::Renderer::FormatName(RHI::Format format) {
#define RHI_FORMAT_NAME(x) case RHI::Format::x: return #x
    switch (format) {
    RHI_FORMAT_NAME(Unknown);
    RHI_FORMAT_NAME(Auto);
    RHI_FORMAT_NAME(R32G32B32A32_FLOAT);
    RHI_FORMAT_NAME(R32G32B32A32_UINT);
    RHI_FORMAT_NAME(R32G32B32A32_SINT);
    RHI_FORMAT_NAME(R32G32B32_FLOAT);
    RHI_FORMAT_NAME(R32G32B32_UINT);
    RHI_FORMAT_NAME(R32G32B32_SINT);
    RHI_FORMAT_NAME(R16G16B16A16_FLOAT);
    RHI_FORMAT_NAME(R16G16B16A16_UNORM);
    RHI_FORMAT_NAME(R16G16B16A16_UINT);
    RHI_FORMAT_NAME(R16G16B16A16_SNORM);
    RHI_FORMAT_NAME(R16G16B16A16_SINT);
    RHI_FORMAT_NAME(R32G32_FLOAT);
    RHI_FORMAT_NAME(R32G32_UINT);
    RHI_FORMAT_NAME(R32G32_SINT);
    RHI_FORMAT_NAME(D32_FLOAT_S8X24_UINT);
    RHI_FORMAT_NAME(R32_FLOAT_X8X24_TYPELESS);
    RHI_FORMAT_NAME(X32_TYPELESS_G8X24_UINT);
    RHI_FORMAT_NAME(R10G10B10A2_UNORM);
    RHI_FORMAT_NAME(R10G10B10A2_UINT);
    RHI_FORMAT_NAME(R11G11B10_FLOAT);
    RHI_FORMAT_NAME(R8G8B8A8_UNORM);
    RHI_FORMAT_NAME(R8G8B8A8_UNORM_SRGB);
    RHI_FORMAT_NAME(R8G8B8A8_UINT);
    RHI_FORMAT_NAME(R8G8B8A8_SNORM);
    RHI_FORMAT_NAME(R8G8B8A8_SINT);
    RHI_FORMAT_NAME(R16G16_FLOAT);
    RHI_FORMAT_NAME(R16G16_UNORM);
    RHI_FORMAT_NAME(R16G16_UINT);
    RHI_FORMAT_NAME(R16G16_SNORM);
    RHI_FORMAT_NAME(R16G16_SINT);
    RHI_FORMAT_NAME(R32_FLOAT);
    RHI_FORMAT_NAME(D32_FLOAT);
    RHI_FORMAT_NAME(R32_UINT);
    RHI_FORMAT_NAME(R32_SINT);
    RHI_FORMAT_NAME(D24_UNORM_S8_UINT);
    RHI_FORMAT_NAME(R24_UNORM_X8_TYPELESS);
    RHI_FORMAT_NAME(X24_TYPELESS_G8_UINT);
    RHI_FORMAT_NAME(R8G8_UNORM);
    RHI_FORMAT_NAME(R8G8_UINT);
    RHI_FORMAT_NAME(R8G8_SNORM);
    RHI_FORMAT_NAME(R8G8_SINT);
    RHI_FORMAT_NAME(R16_FLOAT);
    RHI_FORMAT_NAME(D16_UNORM);
    RHI_FORMAT_NAME(R16_UNORM);
    RHI_FORMAT_NAME(R16_UINT);
    RHI_FORMAT_NAME(R16_SNORM);
    RHI_FORMAT_NAME(R16_SINT);
    RHI_FORMAT_NAME(R8_UNORM);
    RHI_FORMAT_NAME(R8_UINT);
    RHI_FORMAT_NAME(R8_SNORM);
    RHI_FORMAT_NAME(R8_SINT);
    RHI_FORMAT_NAME(A8_UNORM);
    RHI_FORMAT_NAME(B5G6R5_UNORM);
    RHI_FORMAT_NAME(B5G5R5A1_UNORM);
    RHI_FORMAT_NAME(B8G8R8A8_UNORM);
    RHI_FORMAT_NAME(B8G8R8A8_UNORM_SRGB);
    RHI_FORMAT_NAME(B8G8R8X8_UNORM);
    RHI_FORMAT_NAME(B8G8R8X8_UNORM_SRGB);
    RHI_FORMAT_NAME(B4G4R4A4_UNORM);
    RHI_FORMAT_NAME(A4B4G4R4_UNORM);
    RHI_FORMAT_NAME(R9G9B9E5_SHAREDEXP);
    RHI_FORMAT_NAME(BC1_UNORM);
    RHI_FORMAT_NAME(BC1_UNORM_SRGB);
    RHI_FORMAT_NAME(BC2_UNORM);
    RHI_FORMAT_NAME(BC2_UNORM_SRGB);
    RHI_FORMAT_NAME(BC3_UNORM);
    RHI_FORMAT_NAME(BC3_UNORM_SRGB);
    RHI_FORMAT_NAME(BC4_UNORM);
    RHI_FORMAT_NAME(BC4_SNORM);
    RHI_FORMAT_NAME(BC5_UNORM);
    RHI_FORMAT_NAME(BC5_SNORM);
    RHI_FORMAT_NAME(BC6H_UF16);
    RHI_FORMAT_NAME(BC6H_SF16);
    RHI_FORMAT_NAME(BC7_UNORM);
    RHI_FORMAT_NAME(BC7_UNORM_SRGB);
    }
#undef RHI_FORMAT_NAME
    return "Invalid";
}

const char *RHI::Renderer::BufferFlagName(RHI::BufferFlag flag) {
    switch (flag) {
    case RHI::BufferFlag::None: return "None";
    case RHI::BufferFlag::ConstantBuffer: return "ConstantBuffer";
    case RHI::BufferFlag::VertexBuffer: return "VertexBuffer";
    case RHI::BufferFlag::IndexBuffer: return "IndexBuffer";
    case RHI::BufferFlag::IndirectBuffer: return "IndirectBuffer";
    case RHI::BufferFlag::ShaderResource: return "ShaderResource";
    case RHI::BufferFlag::UnorderedAccess: return "UnorderedAccess";
    }
    return "Invalid";
}

const char *RHI::Renderer::TextureFlagName(RHI::TextureFlag flag) {
    switch (flag) {
    case RHI::TextureFlag::None: return "None";
    case RHI::TextureFlag::RenderTarget: return "RenderTarget";
    case RHI::TextureFlag::DepthStencil: return "DepthStencil";
    case RHI::TextureFlag::Typeless: return "Typeless";
    case RHI::TextureFlag::ShaderResource: return "ShaderResource";
    case RHI::TextureFlag::UnorderedAccess: return "UnorderedAccess";
    case RHI::TextureFlag::SkipDefaultViews: return "SkipDefaultViews";
    }
    return "Invalid";
}

const char *RHI::Renderer::ShaderModelName(RHI::ShaderModel shaderModel) {
    switch (shaderModel) {
    case RHI::ShaderModel::SM_5_0: return "SM_5_0";
    case RHI::ShaderModel::SM_6_0: return "SM_6_0";
    case RHI::ShaderModel::SM_6_1: return "SM_6_1";
    case RHI::ShaderModel::SM_6_2: return "SM_6_2";
    case RHI::ShaderModel::SM_6_3: return "SM_6_3";
    case RHI::ShaderModel::SM_6_4: return "SM_6_4";
    case RHI::ShaderModel::SM_6_5: return "SM_6_5";
    case RHI::ShaderModel::SM_6_6: return "SM_6_6";
    case RHI::ShaderModel::SM_6_7: return "SM_6_7";
    case RHI::ShaderModel::SM_6_8: return "SM_6_8";
    }
    return "Invalid";
}

void RHI::Renderer::Init(const void *mainWindowHandle) {
    validationLayerEnabled = false;
    gpuValidationEnabled = false;
    initialized = true;
}

void RHI::Renderer::Shutdown() {
    validationLayerEnabled = false;
    gpuValidationEnabled = false;
    initialized = false;
}

void RHI::Renderer::InitDefaultStates() {
    RHI::RasterizerState *rs = &rasterizerStates[to_int(RHI::RasterizerStateType::SolidFrontSided)];
    rs->fillMode = RHI::FillMode::Solid;
    rs->cullMode = RHI::CullMode::Back;

    rs = &rasterizerStates[to_int(RHI::RasterizerStateType::SolidBackSided)];
    rs->fillMode = RHI::FillMode::Solid;
    rs->cullMode = RHI::CullMode::Front;

    rs = &rasterizerStates[to_int(RHI::RasterizerStateType::SolidTwoSided)];
    rs->fillMode = RHI::FillMode::Solid;
    rs->cullMode = RHI::CullMode::None;

    rs = &rasterizerStates[to_int(RHI::RasterizerStateType::Wire)];
    rs->fillMode = RHI::FillMode::Wire;
    rs->cullMode = RHI::CullMode::None;

    rs = &rasterizerStates[to_int(RHI::RasterizerStateType::WireSmooth)];
    rs->fillMode = RHI::FillMode::Wire;
    rs->cullMode = RHI::CullMode::None;
    rs->smoothLineEnabled = true;

    RHI::DepthStencilState *dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::Disabled)];
    dss->depthTestEnabled = false;
    dss->depthWriteMask = RHI::DepthWriteMask::Zero;
    dss->depthFunc = RHI::ComparisonFunc::Never;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::LessWrite)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::All;
    dss->depthFunc = RHI::ComparisonFunc::Less;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::LessNoWrite)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::Zero;
    dss->depthFunc = RHI::ComparisonFunc::Less;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::LEqualWrite)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::All;
    dss->depthFunc = RHI::ComparisonFunc::LEqual;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::LEqualNoWrite)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::Zero;
    dss->depthFunc = RHI::ComparisonFunc::LEqual;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::EqualWrite)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::All;
    dss->depthFunc = RHI::ComparisonFunc::Equal;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::EqualNoWrite)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::Zero;
    dss->depthFunc = RHI::ComparisonFunc::Equal;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::GEqualWrite)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::All;
    dss->depthFunc = RHI::ComparisonFunc::GEqual;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::GEqualNoWrite)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::Zero;
    dss->depthFunc = RHI::ComparisonFunc::GEqual;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::GreaterWrite)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::All;
    dss->depthFunc = RHI::ComparisonFunc::Greater;

    dss = &depthStencilStates[to_int(RHI::DepthStencilStateType::GreaterNoWrite)];
    dss->depthTestEnabled = true;
    dss->depthWriteMask = RHI::DepthWriteMask::Zero;
    dss->depthFunc = RHI::ComparisonFunc::Greater;

    RHI::BlendState *bs = &blendStates[to_int(RHI::BlendStateType::AlphaBlend)];
    bs->renderTargets[0].blendEnabled = true;
    bs->renderTargets[0].srcFactorColor = RHI::Blend::SrcAlpha;
    bs->renderTargets[0].destFactorColor = RHI::Blend::InvSrcAlpha;
    bs->renderTargets[0].blendOpColor = RHI::BlendOp::Add;
    bs->renderTargets[0].srcFactorAlpha = RHI::Blend::One;
    bs->renderTargets[0].destFactorAlpha = RHI::Blend::Zero;
    bs->renderTargets[0].blendOpAlpha = RHI::BlendOp::Add;

    bs = &blendStates[to_int(RHI::BlendStateType::AddBlend)];
    bs->renderTargets[0].blendEnabled = true;
    bs->renderTargets[0].srcFactorColor = RHI::Blend::SrcAlpha;
    bs->renderTargets[0].destFactorColor = RHI::Blend::One;
    bs->renderTargets[0].blendOpColor = RHI::BlendOp::Add;
    bs->renderTargets[0].srcFactorAlpha = RHI::Blend::One;
    bs->renderTargets[0].destFactorAlpha = RHI::Blend::Zero;
    bs->renderTargets[0].blendOpAlpha = RHI::BlendOp::Add;

    bs = &blendStates[to_int(RHI::BlendStateType::Add)];
    bs->renderTargets[0].blendEnabled = true;
    bs->renderTargets[0].srcFactorColor = RHI::Blend::One;
    bs->renderTargets[0].destFactorColor = RHI::Blend::One;
    bs->renderTargets[0].blendOpColor = RHI::BlendOp::Add;
    bs->renderTargets[0].srcFactorAlpha = RHI::Blend::One;
    bs->renderTargets[0].destFactorAlpha = RHI::Blend::Zero;
    bs->renderTargets[0].blendOpAlpha = RHI::BlendOp::Add;

    bs = &blendStates[to_int(RHI::BlendStateType::Modulate)];
    bs->renderTargets[0].blendEnabled = true;
    bs->renderTargets[0].srcFactorColor = RHI::Blend::DestColor;
    bs->renderTargets[0].destFactorColor = RHI::Blend::Zero;
    bs->renderTargets[0].blendOpColor = RHI::BlendOp::Add;
    bs->renderTargets[0].srcFactorAlpha = RHI::Blend::One;
    bs->renderTargets[0].destFactorAlpha = RHI::Blend::Zero;
    bs->renderTargets[0].blendOpAlpha = RHI::BlendOp::Add;

    bs = &blendStates[to_int(RHI::BlendStateType::Modulate2X)];
    bs->renderTargets[0].blendEnabled = true;
    bs->renderTargets[0].srcFactorColor = RHI::Blend::DestColor;
    bs->renderTargets[0].destFactorColor = RHI::Blend::SrcColor;
    bs->renderTargets[0].blendOpColor = RHI::BlendOp::Add;
    bs->renderTargets[0].srcFactorAlpha = RHI::Blend::One;
    bs->renderTargets[0].destFactorAlpha = RHI::Blend::Zero;
    bs->renderTargets[0].blendOpAlpha = RHI::BlendOp::Add;
}

void RHI::Renderer::FreeDefaultStates() {
}

RHI::GPUBarrier RHI::Renderer::MakeMemoryBarrier(const RHI::GPUResource *resource) {
    RHI::GPUBarrier barrier;
    barrier.type = RHI::GPUBarrier::Type::Memory;
    barrier.memoryBarrier.resource = resource;
    return barrier;
}

RHI::GPUBarrier RHI::Renderer::MakeBufferBarrier(const RHI::Buffer *buffer, RHI::GPUResourceState stateBefore, RHI::GPUResourceState stateAfter) {
    RHI::GPUBarrier barrier;
    barrier.type = RHI::GPUBarrier::Type::Buffer;
    barrier.bufferBarrier.buffer = buffer;
    barrier.bufferBarrier.stateBefore = stateBefore;
    barrier.bufferBarrier.stateAfter = stateAfter;
    return barrier;
}

RHI::GPUBarrier RHI::Renderer::MakeTextureBarrier(const RHI::Texture *texture, RHI::GPUResourceState stateBefore, RHI::GPUResourceState stateAfter, int slice, int mipLevel) {
    RHI::GPUBarrier barrier;
    barrier.type = RHI::GPUBarrier::Type::Texture;
    barrier.textureBarrier.texture = texture;
    barrier.textureBarrier.stateBefore = stateBefore;
    barrier.textureBarrier.stateAfter = stateAfter;
    barrier.textureBarrier.slice = slice;
    barrier.textureBarrier.mipLevel = mipLevel;
    return barrier;
}

RHI::GPUBarrier RHI::Renderer::MakeAliasingBarrier(const RHI::GPUResource *resourceBefore, const RHI::GPUResource *resourceAfter) {
    RHI::GPUBarrier barrier;
    barrier.type = RHI::GPUBarrier::Type::Aliasing;
    barrier.aliasingBarrier.resourceBefore = resourceBefore;
    barrier.aliasingBarrier.resourceAfter = resourceAfter;
    return barrier;
}

void RHI::Renderer::AdjustTextureFormat(bool useCompression, bool useNormalMap, bool useUAV, BE1::Image::Format inFormat, BE1::Image::Format *outFormat) {
    if (useUAV) {
        *outFormat = ToUAVImageFormat(inFormat);
        return;
    }

    if (BE1::Image::IsDepthFormat(inFormat) || BE1::Image::IsDepthStencilFormat(inFormat)) {
        *outFormat = inFormat;
        return;
    }

    BE1::Image::Format candidateFormat = inFormat;

    // 지원되지 않는 포맷일 경우, 먼저 무압축 포맷으로 변환한다.
    if (!IsSupportedImageFormat(candidateFormat)) {
        candidateFormat = ToUncompressedImageFormat(candidateFormat);
    }

    // 압축을 원할 경우, 지원되는 적절한 압축 포맷을 고른다.
    if (useCompression && !BE1::Image::IsCompressed(candidateFormat)) {
        candidateFormat = ToCompressedImageFormat(candidateFormat, useNormalMap);
    }

    // 최종적으로 지원하지 않는 포맷은 지원하는 대체 포맷으로 바꾼다.
    if (!IsSupportedImageFormat(candidateFormat)) {
        candidateFormat = ToSupportedImageFormat(candidateFormat);
    }

    *outFormat = candidateFormat;
}

void RHI::Renderer::AdjustTextureSize(TextureType textureType, bool useNPOT, uint32_t inWidth, uint32_t inHeight, uint32_t inDepth, uint32_t *outWidth, uint32_t *outHeight, uint32_t *outDepth) {
    uint32_t w, h, d;

    if (useNPOT) {
        w = inWidth;
        h = inHeight;
        d = inDepth;
    } else {
        // Fit multiplier size of 2.
        w = BE1::Math::RoundUpPowerOfTwo(inWidth);
        h = BE1::Math::RoundUpPowerOfTwo(inHeight);
        d = BE1::Math::RoundUpPowerOfTwo(inDepth);
    }

    switch (textureType) {
    case TextureType::Texture1D:
    case TextureType::Texture1DArray:
        if (w > max1DTextureSize) w = max1DTextureSize;
        break;
    case TextureType::Texture2D:
    case TextureType::Texture2DArray:
        if (w > max2DTextureSize) w = max2DTextureSize;
        if (h > max2DTextureSize) h = max2DTextureSize;
        break;
    case TextureType::Texture3D:
        if (w > max3DTextureSize) w = max3DTextureSize;
        if (h > max3DTextureSize) h = max3DTextureSize;
        if (d > max3DTextureSize) d = max3DTextureSize;
        break;
    case TextureType::TextureCube:
    case TextureType::TextureCubeArray:
        if (w > maxCubeTextureSize) w = maxCubeTextureSize;
        if (h > maxCubeTextureSize) h = maxCubeTextureSize;

        // For cubemaps, set the width and height equally.
        if (w < h) {
            h = w;
        } else {
            w = h;
        }
        break;
    default:
        assert(0);
        break;
    }

    if (outWidth) {
        *outWidth = w;
    }
    if (outHeight) {
        *outHeight = h;
    }
    if (outDepth) {
        *outDepth = d;
    }
}

bool RHI::Renderer::ImageFormatToRHIFormat(BE1::Image::Format imageFormat, bool isSRGB, RHI::Format *rhiFormat) {
    switch (imageFormat) {
    case BE1::Image::Format::Unknown:
        if (rhiFormat) *rhiFormat = RHI::Format::Unknown;
        return true;
    case BE1::Image::Format::R32_FLOAT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32_FLOAT;
        return true;
    case BE1::Image::Format::R32_SINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32_SINT;
        return true;
    case BE1::Image::Format::R32_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32_UINT;
        return true;
    case BE1::Image::Format::R32G32_FLOAT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32G32_FLOAT;
        return true;
    case BE1::Image::Format::R32G32_SINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32G32_SINT;
        return true;
    case BE1::Image::Format::R32G32_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32G32_UINT;
        return true;
    case BE1::Image::Format::R32G32B32_FLOAT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32G32B32_FLOAT;
        return true;
    case BE1::Image::Format::R32G32B32_SINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32G32B32_SINT;
        return true;
    case BE1::Image::Format::R32G32B32_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32G32B32_UINT;
        return true;
    case BE1::Image::Format::R32G32B32A32_FLOAT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32G32B32A32_FLOAT;
        return true;
    case BE1::Image::Format::R32G32B32A32_SINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32G32B32A32_SINT;
        return true;
    case BE1::Image::Format::R32G32B32A32_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R32G32B32A32_UINT;
        return true;
    case BE1::Image::Format::R16_FLOAT:
    case BE1::Image::Format::L16_FLOAT:
        if (rhiFormat) *rhiFormat = RHI::Format::R16_FLOAT;
        return true;
    case BE1::Image::Format::R16:
        if (rhiFormat) *rhiFormat = RHI::Format::R16_UNORM;
        return true;
    case BE1::Image::Format::R16_SNORM:
        if (rhiFormat) *rhiFormat = RHI::Format::R16_SNORM;
        return true;
    case BE1::Image::Format::R16_SINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R16_SINT;
        return true;
    case BE1::Image::Format::R16_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R16_UINT;
        return true;
    case BE1::Image::Format::R16G16_FLOAT:
        if (rhiFormat) *rhiFormat = RHI::Format::R16G16_FLOAT;
        return true;
    case BE1::Image::Format::R16G16:
        if (rhiFormat) *rhiFormat = RHI::Format::R16G16_UNORM;
        return true;
    case BE1::Image::Format::R16G16_SNORM:
        if (rhiFormat) *rhiFormat = RHI::Format::R16G16_SNORM;
        return true;
    case BE1::Image::Format::R16G16_SINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R16G16_SINT;
        return true;
    case BE1::Image::Format::R16G16_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R16G16_UINT;
        return true;
    case BE1::Image::Format::R16G16B16A16_FLOAT:
        if (rhiFormat) *rhiFormat = RHI::Format::R16G16B16A16_FLOAT;
        return true;
    case BE1::Image::Format::R16G16B16A16:
        if (rhiFormat) *rhiFormat = RHI::Format::R16G16B16A16_UNORM;
        return true;
    case BE1::Image::Format::R16G16B16A16_SNORM:
        if (rhiFormat) *rhiFormat = RHI::Format::R16G16B16A16_SNORM;
        return true;
    case BE1::Image::Format::R16G16B16A16_SINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R16G16B16A16_SINT;
        return true;
    case BE1::Image::Format::R16G16B16A16_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R16G16B16A16_UINT;
        return true;
    case BE1::Image::Format::R9G9B9E5_FLOAT:
        if (rhiFormat) *rhiFormat = RHI::Format::R9G9B9E5_SHAREDEXP;
        return true;
    case BE1::Image::Format::R11G11B10_FLOAT:
        if (rhiFormat) *rhiFormat = RHI::Format::R11G11B10_FLOAT;
        return true;
    case BE1::Image::Format::R8:
    case BE1::Image::Format::L8:
        if (rhiFormat) *rhiFormat = RHI::Format::R8_UNORM;
        return true;
    case BE1::Image::Format::R8_SNORM:
        if (rhiFormat) *rhiFormat = RHI::Format::R8_SNORM;
        return true;
    case BE1::Image::Format::R8_SINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R8_SINT;
        return true;
    case BE1::Image::Format::R8_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R8_UINT;
        return true;
    case BE1::Image::Format::A8:
        if (rhiFormat) *rhiFormat = RHI::Format::A8_UNORM;
        return true;
    case BE1::Image::Format::R8G8:
        if (rhiFormat) *rhiFormat = RHI::Format::R8G8_UNORM;
        return true;
    case BE1::Image::Format::R8G8_SNORM:
        if (rhiFormat) *rhiFormat = RHI::Format::R8G8_SNORM;
        return true;
    case BE1::Image::Format::R8G8_SINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R8G8_SINT;
        return true;
    case BE1::Image::Format::R8G8_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R8G8_UINT;
        return true;
    case BE1::Image::Format::R8G8B8A8:
        if (rhiFormat) *rhiFormat = isSRGB ? RHI::Format::R8G8B8A8_UNORM_SRGB : RHI::Format::R8G8B8A8_UNORM;
        return true;
    case BE1::Image::Format::R8G8B8A8_SNORM:
        if (rhiFormat) *rhiFormat = RHI::Format::R8G8B8A8_SNORM;
        return true;
    case BE1::Image::Format::R8G8B8A8_SINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R8G8B8A8_SINT;
        return true;
    case BE1::Image::Format::R8G8B8A8_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R8G8B8A8_UINT;
        return true;
    case BE1::Image::Format::B8G8R8A8:
        if (rhiFormat) *rhiFormat = isSRGB ? RHI::Format::B8G8R8A8_UNORM_SRGB : RHI::Format::B8G8R8A8_UNORM;
        return true;
    case BE1::Image::Format::B8G8R8X8:
        if (rhiFormat) *rhiFormat = isSRGB ? RHI::Format::B8G8R8X8_UNORM_SRGB : RHI::Format::B8G8R8X8_UNORM;
        return true;
    case BE1::Image::Format::B5G6R5:
        if (rhiFormat) *rhiFormat = RHI::Format::B5G6R5_UNORM;
        return true;
    case BE1::Image::Format::B5G5R5A1:
        if (rhiFormat) *rhiFormat = RHI::Format::B5G5R5A1_UNORM;
        return true;
    case BE1::Image::Format::B4G4R4A4:
        if (rhiFormat) *rhiFormat = RHI::Format::B4G4R4A4_UNORM;
        return true;
    case BE1::Image::Format::A4B4G4R4:
        if (rhiFormat) *rhiFormat = RHI::Format::A4B4G4R4_UNORM;
        return true;
    case BE1::Image::Format::R10G10B10A2:
        if (rhiFormat) *rhiFormat = RHI::Format::R10G10B10A2_UNORM;
        return true;
    case BE1::Image::Format::R10G10B10A2_UINT:
        if (rhiFormat) *rhiFormat = RHI::Format::R10G10B10A2_UINT;
        return true;
    case BE1::Image::Format::BC1:
        if (rhiFormat) *rhiFormat = isSRGB ? RHI::Format::BC1_UNORM_SRGB : RHI::Format::BC1_UNORM;
        return true;
    case BE1::Image::Format::BC2:
        if (rhiFormat) *rhiFormat = isSRGB ? RHI::Format::BC2_UNORM_SRGB : RHI::Format::BC2_UNORM;
        return true;
    case BE1::Image::Format::BC3:
        if (rhiFormat) *rhiFormat = isSRGB ? RHI::Format::BC3_UNORM_SRGB : RHI::Format::BC3_UNORM;
        return true;
    case BE1::Image::Format::BC4:
        if (rhiFormat) *rhiFormat = RHI::Format::BC4_UNORM;
        return true;
    case BE1::Image::Format::BC5:
        if (rhiFormat) *rhiFormat = RHI::Format::BC5_UNORM;
        return true;
    case BE1::Image::Format::BC6H_UF16:
        if (rhiFormat) *rhiFormat = RHI::Format::BC6H_UF16;
        return true;
    case BE1::Image::Format::BC6H_SF16:
        if (rhiFormat) *rhiFormat = RHI::Format::BC6H_SF16;
        return true;
    case BE1::Image::Format::BC7:
        if (rhiFormat) *rhiFormat = isSRGB ? RHI::Format::BC7_UNORM_SRGB : RHI::Format::BC7_UNORM;
        return true;
    case BE1::Image::Format::D16:
        if (rhiFormat) *rhiFormat = RHI::Format::D16_UNORM;
        return true;
    case BE1::Image::Format::D24S8:
        if (rhiFormat) *rhiFormat = RHI::Format::D24_UNORM_S8_UINT;
        return true;
    case BE1::Image::Format::D32_FLOAT:
        if (rhiFormat) *rhiFormat = RHI::Format::D32_FLOAT;
        return true;
    case BE1::Image::Format::D32_FLOAT_S8X24:
        if (rhiFormat) *rhiFormat = RHI::Format::D32_FLOAT_S8X24_UINT;
        return true;
    }
    return false;
}

bool RHI::Renderer::RHIFormatToImageFormat(RHI::Format rhiFormat, BE1::Image::Format *imageFormat, bool *isSRGB) {
    if (isSRGB) {
        *isSRGB = false;
        switch (rhiFormat) {
        case RHI::Format::R8G8B8A8_UNORM_SRGB:
        case RHI::Format::B8G8R8A8_UNORM_SRGB:
        case RHI::Format::B8G8R8X8_UNORM_SRGB:
        case RHI::Format::BC1_UNORM_SRGB:
        case RHI::Format::BC2_UNORM_SRGB:
        case RHI::Format::BC3_UNORM_SRGB:
        case RHI::Format::BC7_UNORM_SRGB:
            *isSRGB = true;
            break;
        }
    }

    switch (rhiFormat) {
    case RHI::Format::R32_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32_FLOAT;
        return true;
    case RHI::Format::R32_SINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32_SINT;
        return true;
    case RHI::Format::R32_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32_UINT;
        return true;
    case RHI::Format::R32G32_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32G32_FLOAT;
        return true;
    case RHI::Format::R32G32_SINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32G32_SINT;
        return true;
    case RHI::Format::R32G32_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32G32_UINT;
        return true;
    case RHI::Format::R32G32B32_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32G32B32_FLOAT;
        return true;
    case RHI::Format::R32G32B32_SINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32G32B32_SINT;
        return true;
    case RHI::Format::R32G32B32_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32G32B32_UINT;
        return true;
    case RHI::Format::R32G32B32A32_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32G32B32A32_FLOAT;
        return true;
    case RHI::Format::R32G32B32A32_SINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32G32B32A32_SINT;
        return true;
    case RHI::Format::R32G32B32A32_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R32G32B32A32_UINT;
        return true;
    case RHI::Format::R16_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16_FLOAT;
        return true;
    case RHI::Format::R16_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16;
        return true;
    case RHI::Format::R16_SNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16_SNORM;
        return true;
    case RHI::Format::R16_SINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16_SINT;
        return true;
    case RHI::Format::R16_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16_UINT;
        return true;
    case RHI::Format::R16G16_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16G16_FLOAT;
        return true;
    case RHI::Format::R16G16_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16G16;
        return true;
    case RHI::Format::R16G16_SNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16G16_SNORM;
        return true;
    case RHI::Format::R16G16_SINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16G16_SINT;
        return true;
    case RHI::Format::R16G16_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16G16_UINT;
        return true;
    case RHI::Format::R16G16B16A16_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16G16B16A16_FLOAT;
        return true;
    case RHI::Format::R16G16B16A16_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16G16B16A16;
        return true;
    case RHI::Format::R16G16B16A16_SNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16G16B16A16_SNORM;
        return true;
    case RHI::Format::R16G16B16A16_SINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16G16B16A16_SINT;
        return true;
    case RHI::Format::R16G16B16A16_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R16G16B16A16_UINT;
        return true;
    case RHI::Format::R9G9B9E5_SHAREDEXP:
        if (imageFormat) *imageFormat = BE1::Image::Format::R9G9B9E5_FLOAT;
        return true;
    case RHI::Format::R11G11B10_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R11G11B10_FLOAT;
        return true;
    case RHI::Format::R8_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8;
        return true;
    case RHI::Format::R8_SNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8_SNORM;
        return true;
    case RHI::Format::R8_SINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8_SINT;
        return true;
    case RHI::Format::R8_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8_UINT;
        return true;
    case RHI::Format::A8_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::A8;
        return true;
    case RHI::Format::R8G8_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8G8;
        return true;
    case RHI::Format::R8G8_SNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8G8_SNORM;
        return true;
    case RHI::Format::R8G8_SINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8G8_SINT;
        return true;
    case RHI::Format::R8G8_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8G8_UINT;
        return true;
    case RHI::Format::R8G8B8A8_UNORM:
    case RHI::Format::R8G8B8A8_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8G8B8A8;
        return true;
    case RHI::Format::R8G8B8A8_SNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8G8B8A8_SNORM;
        return true;
    case RHI::Format::R8G8B8A8_SINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8G8B8A8_SINT;
        return true;
    case RHI::Format::R8G8B8A8_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R8G8B8A8_UINT;
        return true;
    case RHI::Format::B8G8R8A8_UNORM:
    case RHI::Format::B8G8R8A8_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::B8G8R8A8;
        return true;
    case RHI::Format::B8G8R8X8_UNORM:
    case RHI::Format::B8G8R8X8_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::B8G8R8X8;
        return true;
    case RHI::Format::R10G10B10A2_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::R10G10B10A2;
        return true;
    case RHI::Format::R10G10B10A2_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::R10G10B10A2_UINT;
        return true;
    case RHI::Format::B5G6R5_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::B5G6R5;
        return true;
    case RHI::Format::B5G5R5A1_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::B5G5R5A1;
        return true;
    case RHI::Format::B4G4R4A4_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::B4G4R4A4;
        return true;
    case RHI::Format::A4B4G4R4_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::A4B4G4R4;
        return true;
    case RHI::Format::D16_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::D16;
        return true;
    case RHI::Format::D32_FLOAT:
        if (imageFormat) *imageFormat = BE1::Image::Format::D32_FLOAT;
        return true;
    case RHI::Format::D24_UNORM_S8_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::D24S8;
        return true;
    case RHI::Format::D32_FLOAT_S8X24_UINT:
        if (imageFormat) *imageFormat = BE1::Image::Format::D32_FLOAT_S8X24;
        return true;
    case RHI::Format::BC1_UNORM:
    case RHI::Format::BC1_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::BC1;
        return true;
    case RHI::Format::BC2_UNORM:
    case RHI::Format::BC2_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::BC2;
        return true;
    case RHI::Format::BC3_UNORM:
    case RHI::Format::BC3_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::BC3;
        return true;
    case RHI::Format::BC4_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::BC4;
        return true;
    case RHI::Format::BC5_UNORM:
        if (imageFormat) *imageFormat = BE1::Image::Format::BC5;
        return true;
    case RHI::Format::BC6H_UF16:
        if (imageFormat) *imageFormat = BE1::Image::Format::BC6H_UF16;
        return true;
    case RHI::Format::BC6H_SF16:
        if (imageFormat) *imageFormat = BE1::Image::Format::BC6H_SF16;
        return true;
    case RHI::Format::BC7_UNORM:
    case RHI::Format::BC7_UNORM_SRGB:
        if (imageFormat) *imageFormat = BE1::Image::Format::BC7;
        return true;
    }
    return false;
}

bool RHI::Renderer::IsSRGBFormat(RHI::Format format) {
    switch (format) {
    case RHI::Format::R8G8B8A8_UNORM_SRGB:
    case RHI::Format::B8G8R8A8_UNORM_SRGB:
    case RHI::Format::B8G8R8X8_UNORM_SRGB:
    case RHI::Format::BC1_UNORM_SRGB:
    case RHI::Format::BC2_UNORM_SRGB:
    case RHI::Format::BC3_UNORM_SRGB:
    case RHI::Format::BC7_UNORM_SRGB:
        return true;
    }
    return false;
}

RHI::Format RHI::Renderer::ToLinearFormat(RHI::Format format) {
    switch (format) {
    case RHI::Format::R8G8B8A8_UNORM_SRGB:
        return RHI::Format::R8G8B8A8_UNORM;
    case RHI::Format::B8G8R8A8_UNORM_SRGB:
        return RHI::Format::B8G8R8A8_UNORM;
    case RHI::Format::B8G8R8X8_UNORM_SRGB:
        return RHI::Format::B8G8R8X8_UNORM;
    case RHI::Format::BC1_UNORM_SRGB:
        return RHI::Format::BC1_UNORM;
    case RHI::Format::BC2_UNORM_SRGB:
        return RHI::Format::BC2_UNORM;
    case RHI::Format::BC3_UNORM_SRGB:
        return RHI::Format::BC3_UNORM;
    case RHI::Format::BC7_UNORM_SRGB:
        return RHI::Format::BC7_UNORM;
    }
    return format;
}

RHI::Format RHI::Renderer::ToSRGBFormat(RHI::Format format) {
    switch (format) {
    case RHI::Format::R8G8B8A8_UNORM:
        return RHI::Format::R8G8B8A8_UNORM_SRGB;
    case RHI::Format::B8G8R8A8_UNORM:
        return RHI::Format::B8G8R8A8_UNORM_SRGB;
    case RHI::Format::B8G8R8X8_UNORM:
        return RHI::Format::B8G8R8X8_UNORM_SRGB;
    case RHI::Format::BC1_UNORM:
        return RHI::Format::BC1_UNORM_SRGB;
    case RHI::Format::BC2_UNORM:
        return RHI::Format::BC2_UNORM_SRGB;
    case RHI::Format::BC3_UNORM:
        return RHI::Format::BC3_UNORM_SRGB;
    case RHI::Format::BC7_UNORM:
        return RHI::Format::BC7_UNORM_SRGB;
    }
    return format;
}

bool RHI::Renderer::IsDepthFormat(RHI::Format format) {
    switch (format) {
    case RHI::Format::D16_UNORM:
    case RHI::Format::D32_FLOAT:
    case RHI::Format::D24_UNORM_S8_UINT:
    case RHI::Format::D32_FLOAT_S8X24_UINT:
        return true;
    }
    return false;
}

bool RHI::Renderer::IsStencilFormat(RHI::Format format) {
    switch (format) {
    case RHI::Format::D24_UNORM_S8_UINT:
    case RHI::Format::D32_FLOAT_S8X24_UINT:
        return true;
    }
    return false;
}

bool RHI::Renderer::IsDepthStencilFormat(RHI::Format format) {
    return IsDepthFormat(format) && IsStencilFormat(format);
}
