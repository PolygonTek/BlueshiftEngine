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

#include "Precompiled.h"
#include "D3D12Renderer.h"
#include "D3D12PipelineState.h"
#include "D3D12Shader.h"
#include "D3D12Texture.h"
#include "D3D12ConstantBuffer.h"
#include "D3D12CommandList.h"
#include "D3D12RootDescriptorPool.h"
#include "D3D12FrameData.h"

static constexpr D3D12_FILL_MODE ToD3D12FillMode(RHI::FillMode fillMode) {
    switch (fillMode) {
    case RHI::FillMode::Wire:
        return D3D12_FILL_MODE_WIREFRAME;
    case RHI::FillMode::Solid:
        return D3D12_FILL_MODE_SOLID;
    }
    assert(0);
    return D3D12_FILL_MODE_SOLID;
}

static constexpr D3D12_CULL_MODE ToD3D12CullMode(RHI::CullMode cullMode) {
    switch (cullMode) {
    case RHI::CullMode::Back:
        return D3D12_CULL_MODE_BACK;
    case RHI::CullMode::Front:
        return D3D12_CULL_MODE_FRONT;
    case RHI::CullMode::None:
        return D3D12_CULL_MODE_NONE;
    }
    assert(0);
    return D3D12_CULL_MODE_NONE;
}

static constexpr UINT8 ToD3D12ColorWriteMask(RHI::ColorWriteMask colorWriteMask) {
    UINT8 mask = 0;
    if (BE1::HasFlag(colorWriteMask, RHI::ColorWriteMask::Red)) {
        mask |= D3D12_COLOR_WRITE_ENABLE_RED;
    }
    if (BE1::HasFlag(colorWriteMask, RHI::ColorWriteMask::Green)) {
        mask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
    }
    if (BE1::HasFlag(colorWriteMask, RHI::ColorWriteMask::Blue)) {
        mask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
    }
    if (BE1::HasFlag(colorWriteMask, RHI::ColorWriteMask::Alpha)) {
        mask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
    }
    return mask;
}

static constexpr D3D12_DEPTH_WRITE_MASK ToD3D12DepthWriteMask(RHI::DepthWriteMask depthWriteMask) {
    if (depthWriteMask == RHI::DepthWriteMask::Zero) {
        return D3D12_DEPTH_WRITE_MASK_ZERO;
    }
    if (depthWriteMask == RHI::DepthWriteMask::All) {
        return D3D12_DEPTH_WRITE_MASK_ALL;
    }
    assert(0);
    return D3D12_DEPTH_WRITE_MASK_ZERO;
}

static constexpr D3D12_COMPARISON_FUNC ToD3D12ComparisonFunc(RHI::ComparisonFunc comparisonFunc) {
    switch (comparisonFunc) {
    case RHI::ComparisonFunc::Always:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    case RHI::ComparisonFunc::Less:
        return D3D12_COMPARISON_FUNC_LESS;
    case RHI::ComparisonFunc::LEqual:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case RHI::ComparisonFunc::Equal:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case RHI::ComparisonFunc::NotEqual:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case RHI::ComparisonFunc::GEqual:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case RHI::ComparisonFunc::Greater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case RHI::ComparisonFunc::Never:
        return D3D12_COMPARISON_FUNC_NEVER;
    }
    assert(0);
    return D3D12_COMPARISON_FUNC_NEVER;
}

static constexpr D3D12_STENCIL_OP ToD3D12StencilOp(RHI::StencilOp stencilOp) {
    switch (stencilOp) {
    case RHI::StencilOp::Keep:
        return D3D12_STENCIL_OP_KEEP;
    case RHI::StencilOp::Zero:
        return D3D12_STENCIL_OP_ZERO;
    case RHI::StencilOp::Replace:
        return D3D12_STENCIL_OP_REPLACE;
    case RHI::StencilOp::Incr:
        return D3D12_STENCIL_OP_INCR;
    case RHI::StencilOp::Decr:
        return D3D12_STENCIL_OP_DECR;
    case RHI::StencilOp::IncrWrap:
        return D3D12_STENCIL_OP_INCR_SAT;
    case RHI::StencilOp::DecrWrap:
        return D3D12_STENCIL_OP_DECR_SAT;
    case RHI::StencilOp::Invert:
        return D3D12_STENCIL_OP_INVERT;
    }
    assert(0);
    return D3D12_STENCIL_OP_KEEP;
}

static constexpr D3D12_BLEND ToD3D12Blend(RHI::Blend blend) {
    switch (blend) {
    case RHI::Blend::Zero:
        return D3D12_BLEND_ZERO;
    case RHI::Blend::One:
        return D3D12_BLEND_ONE;
    case RHI::Blend::SrcColor:
        return D3D12_BLEND_SRC_COLOR;
    case RHI::Blend::InvSrcColor:
        return D3D12_BLEND_INV_SRC_COLOR;
    case RHI::Blend::SrcAlpha:
        return D3D12_BLEND_SRC_ALPHA;
    case RHI::Blend::InvSrcAlpha:
        return D3D12_BLEND_INV_SRC_ALPHA;
    case RHI::Blend::SrcAlphaSat:
        return D3D12_BLEND_SRC_ALPHA_SAT;
    case RHI::Blend::DestColor:
        return D3D12_BLEND_DEST_COLOR;
    case RHI::Blend::InvDestColor:
        return D3D12_BLEND_INV_DEST_COLOR;
    case RHI::Blend::DestAlpha:
        return D3D12_BLEND_DEST_ALPHA;
    case RHI::Blend::InvDestAlpha:
        return D3D12_BLEND_INV_DEST_ALPHA;
    case RHI::Blend::BlendFactor:
        return D3D12_BLEND_BLEND_FACTOR;
    case RHI::Blend::InvBlendFactor:
        return D3D12_BLEND_INV_BLEND_FACTOR;
    case RHI::Blend::Src1Color:
        return D3D12_BLEND_SRC1_COLOR;
    case RHI::Blend::InvSrc1Color:
        return D3D12_BLEND_INV_SRC1_COLOR;
    case RHI::Blend::Src1Alpha:
        return D3D12_BLEND_SRC1_ALPHA;
    case RHI::Blend::InvSrc1Alpha:
        return D3D12_BLEND_INV_SRC1_ALPHA;
    }
    assert(0);
    return D3D12_BLEND_ZERO;
}

static constexpr D3D12_BLEND_OP ToD3D12BlendOp(RHI::BlendOp blendOp) {
    switch (blendOp) {
    case RHI::BlendOp::Add:
        return D3D12_BLEND_OP_ADD;
    case RHI::BlendOp::Subtract:
        return D3D12_BLEND_OP_SUBTRACT;
    case RHI::BlendOp::ReverseSubtract:
        return D3D12_BLEND_OP_REV_SUBTRACT;
    case RHI::BlendOp::Min:
        return D3D12_BLEND_OP_MIN;
    case RHI::BlendOp::Max:
        return D3D12_BLEND_OP_MAX;
    }
    assert(0);
    return D3D12_BLEND_OP_ADD;
}

static constexpr DXGI_FORMAT ToD3D12InputLayoutElementFormat(RHI::InputLayoutElement::Format format) {
    switch (format) {
    case RHI::InputLayoutElement::Format::Unknown:
        return DXGI_FORMAT_UNKNOWN;
    case RHI::InputLayoutElement::Format::Float4:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case RHI::InputLayoutElement::Format::Float3:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case RHI::InputLayoutElement::Format::Float2:
        return DXGI_FORMAT_R32G32_FLOAT;
    case RHI::InputLayoutElement::Format::Float1:
        return DXGI_FORMAT_R32_FLOAT;
    case RHI::InputLayoutElement::Format::UInt4:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case RHI::InputLayoutElement::Format::UInt3:
        return DXGI_FORMAT_R32G32B32_UINT;
    case RHI::InputLayoutElement::Format::UInt2:
        return DXGI_FORMAT_R32G32_UINT;
    case RHI::InputLayoutElement::Format::UInt1:
        return DXGI_FORMAT_R32_UINT;
    case RHI::InputLayoutElement::Format::Int4:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case RHI::InputLayoutElement::Format::Int3:
        return DXGI_FORMAT_R32G32B32_SINT;
    case RHI::InputLayoutElement::Format::Int2:
        return DXGI_FORMAT_R32G32_SINT;
    case RHI::InputLayoutElement::Format::Int1:
        return DXGI_FORMAT_R32_SINT;
    case RHI::InputLayoutElement::Format::Half4:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case RHI::InputLayoutElement::Format::Half2:
        return DXGI_FORMAT_R16G16_FLOAT;
    case RHI::InputLayoutElement::Format::Half1:
        return DXGI_FORMAT_R16_FLOAT;
    case RHI::InputLayoutElement::Format::UShort4:
        return DXGI_FORMAT_R16G16B16A16_UINT;
    case RHI::InputLayoutElement::Format::UShort2:
        return DXGI_FORMAT_R16G16_UINT;
    case RHI::InputLayoutElement::Format::UShort1:
        return DXGI_FORMAT_R16_UINT;
    case RHI::InputLayoutElement::Format::UShort4N:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
    case RHI::InputLayoutElement::Format::UShort2N:
        return DXGI_FORMAT_R16G16_UNORM;
    case RHI::InputLayoutElement::Format::UShort1N:
        return DXGI_FORMAT_R16_UNORM;
    case RHI::InputLayoutElement::Format::Short4:
        return DXGI_FORMAT_R16G16B16A16_SINT;
    case RHI::InputLayoutElement::Format::Short2:
        return DXGI_FORMAT_R16G16_SINT;
    case RHI::InputLayoutElement::Format::Short1:
        return DXGI_FORMAT_R16_SINT;
    case RHI::InputLayoutElement::Format::Short4N:
        return DXGI_FORMAT_R16G16B16A16_SNORM;
    case RHI::InputLayoutElement::Format::Short2N:
        return DXGI_FORMAT_R16G16_SNORM;
    case RHI::InputLayoutElement::Format::Short1N:
        return DXGI_FORMAT_R16_SNORM;
    case RHI::InputLayoutElement::Format::UByte4:
        return DXGI_FORMAT_R8G8B8A8_UINT;
    case RHI::InputLayoutElement::Format::UByte2:
        return DXGI_FORMAT_R8G8_UINT;
    case RHI::InputLayoutElement::Format::UByte1:
        return DXGI_FORMAT_R8_UINT;
    case RHI::InputLayoutElement::Format::UByte4N:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case RHI::InputLayoutElement::Format::UByte2N:
        return DXGI_FORMAT_R8G8_UNORM;
    case RHI::InputLayoutElement::Format::UByte1N:
        return DXGI_FORMAT_R8_UNORM;
    case RHI::InputLayoutElement::Format::Byte4:
        return DXGI_FORMAT_R8G8B8A8_SINT;
    case RHI::InputLayoutElement::Format::Byte2:
        return DXGI_FORMAT_R8G8_SINT;
    case RHI::InputLayoutElement::Format::Byte1:
        return DXGI_FORMAT_R8_SINT;
    case RHI::InputLayoutElement::Format::Byte4N:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
    case RHI::InputLayoutElement::Format::Byte2N:
        return DXGI_FORMAT_R8G8_SNORM;
    case RHI::InputLayoutElement::Format::Byte1N:
        return DXGI_FORMAT_R8_SNORM;
    }
    assert(0);
    return DXGI_FORMAT_UNKNOWN;
}

static constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3D12TopologyType(RHI::PrimitiveTopology primitiveTopology) {
    switch (primitiveTopology) {
    case RHI::PrimitiveTopology::PointList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case RHI::PrimitiveTopology::LineList:
    case RHI::PrimitiveTopology::LineStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case RHI::PrimitiveTopology::TriangleList:
    case RHI::PrimitiveTopology::TriangleStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case RHI::PrimitiveTopology::PatchList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }
    assert(0);
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

static constexpr D3D12_PRIMITIVE_TOPOLOGY ToD3DPrimitiveTopology(RHI::PrimitiveTopology primitiveTopology) {
    switch (primitiveTopology) {
    case RHI::PrimitiveTopology::PointList:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case RHI::PrimitiveTopology::LineList:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case RHI::PrimitiveTopology::LineStrip:
        return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case RHI::PrimitiveTopology::TriangleList:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case RHI::PrimitiveTopology::TriangleStrip:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    }
    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

void D3D12PipelineState::Release() {
    SAFE_RELEASE(pso);
    SAFE_RELEASE(rootSignature);
}

RHI::PipelineState *D3D12Renderer::CreateGraphicsPSO(const RHI::PipelineStateDesc *desc) {
    struct PSOHashData {
        struct ShaderHashData {
            uint64_t vsHash = 0;
            uint64_t gsHash = 0;
            uint64_t hsHash = 0;
            uint64_t dsHash = 0;
            uint64_t psHash = 0;
        } shaderHashData;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        uint64_t inputLayoutHash = 0;
        uint64_t blendHash = 0;
        uint64_t depthStencilHash = 0;
        uint64_t rasterizerHash = 0;
        uint64_t renderDestHash = 0;
    } psoHashData;

    ID3D12RootSignature *rootSignature = nullptr;
    ID3D12VersionedRootSignatureDeserializer *rootSignatureDeserializer = nullptr;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *rootSignatureDesc = nullptr;

    if (desc->vs) {
        const D3D12Shader *vs = static_cast<const D3D12Shader *>(desc->vs);
        psoHashData.shaderHashData.vsHash = vs->hash;
        if (!rootSignature && vs->rootSignature) {
            rootSignature = vs->rootSignature;
            rootSignatureDeserializer = vs->rootSignatureDeserializer;
            rootSignatureDesc = vs->rootSignatureDesc;
        }
    }

    if (desc->gs) {
        const D3D12Shader *gs = static_cast<const D3D12Shader *>(desc->gs);
        psoHashData.shaderHashData.gsHash = gs->hash;
        if (!rootSignature && gs->rootSignature) {
            rootSignature = gs->rootSignature;
            rootSignatureDeserializer = gs->rootSignatureDeserializer;
            rootSignatureDesc = gs->rootSignatureDesc;
        }
    }

    if (desc->hs) {
        const D3D12Shader *hs = static_cast<const D3D12Shader *>(desc->hs);
        psoHashData.shaderHashData.hsHash = hs->hash;
        if (!rootSignature && hs->rootSignature) {
            rootSignature = hs->rootSignature;
            rootSignatureDeserializer = hs->rootSignatureDeserializer;
            rootSignatureDesc = hs->rootSignatureDesc;
        }
    }

    if (desc->ds) {
        const D3D12Shader *ds = static_cast<const D3D12Shader *>(desc->ds);
        psoHashData.shaderHashData.dsHash = ds->hash;
        if (!rootSignature && ds->rootSignature) {
            rootSignature = ds->rootSignature;
            rootSignatureDeserializer = ds->rootSignatureDeserializer;
            rootSignatureDesc = ds->rootSignatureDesc;
        }
    }

    if (desc->ps) {
        const D3D12Shader *ps = static_cast<const D3D12Shader *>(desc->ps);
        psoHashData.shaderHashData.psHash = ps->hash;
        if (!rootSignature && ps->rootSignature) {
            rootSignature = ps->rootSignature;
            rootSignatureDeserializer = ps->rootSignatureDeserializer;
            rootSignatureDesc = ps->rootSignatureDesc;
        }
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = ToD3D12TopologyType(desc->primitiveTopology);
    psoHashData.primitiveTopologyType = primitiveTopologyType;

    if (desc->inputLayout) {
        size_t hash = 0;
        for (const RHI::InputLayoutElement &element : desc->inputLayout->elements) {
            hash = hash_combine(hash, std::hash<RHI::InputLayoutElement>()(element));
        }
        psoHashData.inputLayoutHash = static_cast<uint64_t>(hash);
    }

    if (desc->blendState) {
        std::hash<RHI::BlendState> hasher;
        psoHashData.blendHash = static_cast<uint64_t>(hasher(*desc->blendState));
    }

    if (desc->depthStencilState) {
        std::hash<RHI::DepthStencilState> hasher;
        psoHashData.depthStencilHash = static_cast<uint64_t>(hasher(*desc->depthStencilState));
    }

    if (desc->rasterizerState) {
        std::hash<RHI::RasterizerState> hasher;
        psoHashData.rasterizerHash = static_cast<uint64_t>(hasher(*desc->rasterizerState));
    }

    if (desc->renderDest) {
        psoHashData.renderDestHash = desc->renderDest->GetHash();
    }

    // 전체 hash 값으로 완전히 동일한 PSO 가 존재하는지 찾아본다.
    const uint64_t psoHash = BE1::CityHash64((char *)&psoHashData, sizeof(psoHashData));
    const auto *psoEntry = graphicsPsoMap.Get(psoHash);
    if (psoEntry) {
        // 동일한 PSO 를 찾았다면, 레퍼런스 카운트를 증가시키고 리턴한다.
        D3D12PipelineState *pso = psoEntry->second;
        pso->refCount.fetch_add(1);
        return pso;
    }

    // 없다면 새로 만든다.
    D3D12PipelineState *pipelineState = new D3D12PipelineState;
    pipelineState->hash = psoHash;
    pipelineState->graphics = true;

    // PSO hash 값으로 동일한 cached PSO 가 존재하는지 찾아보고, 찾았다면 재활용한다.
    const auto *cachedPsoBlobEntry = cachedPsoBlobMap.Get(psoHash);

    ID3DBlob *cachedPsoBlob = nullptr;
    if (cachedPsoBlobEntry) {
        cachedPsoBlob = cachedPsoBlobEntry->second;
    }

#ifndef _DEBUG
    // 없다면 cached PSO 파일을 로딩해본다.
    // NOTE: hash 값 충돌로 엉뚱한 cached PSO 파일을 읽어오지는 않는지 체크 필요
    if (!cachedPsoBlob) {
        LoadCachedPSO(psoHash, &cachedPsoBlob);
    }
#endif

    uint32_t psoStreamSize = 0;
    void *psoStream = nullptr;
    D3D12PipelineState::GraphicsPSStream1 *stream1 = nullptr;
    D3D12PipelineState::GraphicsPSStream2 *stream2 = nullptr;

    psoStreamSize = sizeof(D3D12PipelineState::GraphicsPSStream);
    psoStream = _alloca16(psoStreamSize);
    new (psoStream) D3D12PipelineState::GraphicsPSStream();

    D3D12PipelineState::GraphicsPSStream *stream = static_cast<D3D12PipelineState::GraphicsPSStream *>(psoStream);
    stream1 = &stream->stream1;
    stream2 = &stream->stream2;

    if (cachedPsoBlob) {
        stream->cachedPSO = { cachedPsoBlob->GetBufferPointer(), cachedPsoBlob->GetBufferSize() };
    }

    if (desc->vs) {
        const D3D12Shader *vs = static_cast<const D3D12Shader *>(desc->vs);
        stream->vs = { vs->compiledShaderData, vs->compiledShaderDataSize };
    }

    if (desc->gs) {
        const D3D12Shader *gs = static_cast<const D3D12Shader *>(desc->gs);
        stream->gs = { gs->compiledShaderData, gs->compiledShaderDataSize };
    }

    if (desc->hs) {
        const D3D12Shader *hs = static_cast<const D3D12Shader *>(desc->hs);
        stream->hs = { hs->compiledShaderData, hs->compiledShaderDataSize };
    }

    if (desc->ds) {
        const D3D12Shader *ds = static_cast<const D3D12Shader *>(desc->ds);
        stream->ds = { ds->compiledShaderData, ds->compiledShaderDataSize };
    }

    if (desc->ps) {
        const D3D12Shader *ps = static_cast<const D3D12Shader *>(desc->ps);
        stream->ps = { ps->compiledShaderData, ps->compiledShaderDataSize };
    }

    // Flags
    stream1->flags = CD3DX12_PIPELINE_STATE_STREAM_FLAGS(D3D12_PIPELINE_STATE_FLAG_NONE);

    // RootSignature
    if (rootSignature) {
        rootSignature->AddRef();
        rootSignatureDeserializer->AddRef();

        pipelineState->rootSignature = rootSignature;
        pipelineState->rootSignatureDesc = rootSignatureDesc;

        stream1->rootSignature = pipelineState->rootSignature;

        pipelineState->binder.Init(rootSignatureDesc->Desc_1_1);
    }

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    BE1::Array<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

    if (desc->inputLayout) {
        int numElements = desc->inputLayout->elements.Count();
        inputElementDescs.SetCount(numElements);

        for (int i = 0; i < numElements; ++i) {
            const RHI::InputLayoutElement *element = &desc->inputLayout->elements[i];
            D3D12_INPUT_ELEMENT_DESC *elementDesc = &inputElementDescs[i];

            elementDesc->SemanticName = element->semanticName.c_str();
            elementDesc->SemanticIndex = element->semanticIndex;
            elementDesc->Format = ToD3D12InputLayoutElementFormat(element->format);
            elementDesc->InputSlot = element->inputSlot;
            elementDesc->AlignedByteOffset = element->offset;
            elementDesc->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            elementDesc->InstanceDataStepRate = 0;
        }
        inputLayoutDesc.NumElements = numElements;
        inputLayoutDesc.pInputElementDescs = inputElementDescs.Ptr();

        stream1->inputLayout = inputLayoutDesc;
    }

    // PrimitiveTopology
    stream1->primitiveTopologyType = primitiveTopologyType;
    pipelineState->primitiveTopology = ToD3DPrimitiveTopology(desc->primitiveTopology);

    // BlendState
    if (desc->blendState) {
        CD3DX12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = desc->blendState->alphaToCoverageEnabled;
        blendDesc.IndependentBlendEnable = desc->blendState->independentBlendEnabled;

        for (int i = 0; i < COUNT_OF(blendDesc.RenderTarget); ++i) {
            const RHI::RenderTargetBlendState *renderTargetBlendState = &desc->blendState->renderTargets[i];
            D3D12_RENDER_TARGET_BLEND_DESC *renderTargetBlendDesc = &blendDesc.RenderTarget[i];

            renderTargetBlendDesc->BlendEnable = renderTargetBlendState->blendEnabled;
            renderTargetBlendDesc->SrcBlend = ToD3D12Blend(renderTargetBlendState->srcFactorColor);
            renderTargetBlendDesc->DestBlend = ToD3D12Blend(renderTargetBlendState->destFactorColor);
            renderTargetBlendDesc->BlendOp = ToD3D12BlendOp(renderTargetBlendState->blendOpColor);
            renderTargetBlendDesc->SrcBlendAlpha = ToD3D12Blend(renderTargetBlendState->srcFactorAlpha);
            renderTargetBlendDesc->DestBlendAlpha = ToD3D12Blend(renderTargetBlendState->destFactorAlpha);
            renderTargetBlendDesc->BlendOpAlpha = ToD3D12BlendOp(renderTargetBlendState->blendOpAlpha);
            renderTargetBlendDesc->RenderTargetWriteMask = ToD3D12ColorWriteMask(renderTargetBlendState->colorWriteMask);
        }
        stream2->blendDesc = blendDesc;
    }

    // DepthStencilState
    if (desc->depthStencilState) {
        CD3DX12_DEPTH_STENCIL_DESC1 depthStencilDesc = {};
        depthStencilDesc.DepthEnable = desc->depthStencilState->depthTestEnabled;
        depthStencilDesc.DepthWriteMask = ToD3D12DepthWriteMask(desc->depthStencilState->depthWriteMask);
        depthStencilDesc.DepthFunc = ToD3D12ComparisonFunc(desc->depthStencilState->depthFunc);
        depthStencilDesc.StencilEnable = desc->depthStencilState->stencilTestEnabled;
        depthStencilDesc.StencilReadMask = desc->depthStencilState->stencilReadMask;
        depthStencilDesc.StencilWriteMask = desc->depthStencilState->stencilWriteMask;
        depthStencilDesc.FrontFace.StencilFailOp = ToD3D12StencilOp(desc->depthStencilState->frontFace.failOp);
        depthStencilDesc.FrontFace.StencilDepthFailOp = ToD3D12StencilOp(desc->depthStencilState->frontFace.depthFailOp);
        depthStencilDesc.FrontFace.StencilPassOp = ToD3D12StencilOp(desc->depthStencilState->frontFace.passOp);
        depthStencilDesc.FrontFace.StencilFunc = ToD3D12ComparisonFunc(desc->depthStencilState->frontFace.stencilFunc);
        depthStencilDesc.BackFace.StencilFailOp = ToD3D12StencilOp(desc->depthStencilState->backFace.failOp);
        depthStencilDesc.BackFace.StencilDepthFailOp = ToD3D12StencilOp(desc->depthStencilState->backFace.depthFailOp);
        depthStencilDesc.BackFace.StencilPassOp = ToD3D12StencilOp(desc->depthStencilState->backFace.passOp);
        depthStencilDesc.BackFace.StencilFunc = ToD3D12ComparisonFunc(desc->depthStencilState->backFace.stencilFunc);
        depthStencilDesc.DepthBoundsTestEnable = (supportsDepthBoundsTest && desc->depthStencilState->depthBoundTestEnabled) ? TRUE : FALSE;

        stream2->depthStencil = depthStencilDesc;
    }

    // RasterizeState
    if (desc->rasterizerState) {
        CD3DX12_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = ToD3D12FillMode(desc->rasterizerState->fillMode);
        rasterizerDesc.CullMode = ToD3D12CullMode(desc->rasterizerState->cullMode);
        rasterizerDesc.FrontCounterClockwise = TRUE;
        rasterizerDesc.DepthBias = desc->rasterizerState->depthBias;
        rasterizerDesc.DepthBiasClamp = desc->rasterizerState->depthBiasClamp;
        rasterizerDesc.SlopeScaledDepthBias = desc->rasterizerState->slopeScaledDepthBias;
        rasterizerDesc.DepthClipEnable = desc->rasterizerState->depthClipEnabled;
        rasterizerDesc.MultisampleEnable = FALSE;
        rasterizerDesc.AntialiasedLineEnable = desc->rasterizerState->smoothLineEnabled;
        rasterizerDesc.ForcedSampleCount = 0;
        rasterizerDesc.ConservativeRaster = (supportsConservativeRasterization && desc->rasterizerState->conservativeRasterization) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        stream2->rasterizer = rasterizerDesc;
    }

    // RenderPass
    if (desc->renderDest) {
        D3D12_RT_FORMAT_ARRAY renderTargetFormatArray = {};
        renderTargetFormatArray.NumRenderTargets = desc->renderDest->renderTargetCount;

        for (int i = 0; i < renderTargetFormatArray.NumRenderTargets; ++i) {
            DXGI_FORMAT renderTargetFormat;
            ImageFormatToDXGIFormat(desc->renderDest->renderTargetFormats[i], desc->renderDest->renderTargetForematSRGBs[i], &renderTargetFormat);
            renderTargetFormatArray.RTFormats[i] = renderTargetFormat;
        }
        DXGI_FORMAT depthStencilFormat;
        ImageFormatToDXGIFormat(desc->renderDest->depthStencilFormat, false, &depthStencilFormat);

        DXGI_SAMPLE_DESC sampleDesc = {};
        sampleDesc.Count = desc->renderDest->sampleCount;
        sampleDesc.Quality = desc->renderDest->sampleQuality;

        stream2->depthStencilFormat = depthStencilFormat;
        stream2->renderTargetFormats = renderTargetFormatArray;
        stream2->sampleDesc = sampleDesc;
    }

    // SampleMask
    stream2->sampleMask = desc->sampleMask;

    // 최종 PSO stream 작성
    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
    streamDesc.pPipelineStateSubobjectStream = psoStream;
    streamDesc.SizeInBytes = psoStreamSize;

    // streamDesc 를 통해 PSO 를 생성한다.
    HRESULT hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState->pso));
    if (hr == D3D12_ERROR_DRIVER_VERSION_MISMATCH) {
        // cached PSO 버젼이 맞지 않는다면, cached PSO 없이 PSO 를 새로 생성한다.
        stream->cachedPSO = CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO();
        // cached PSO 를 릴리즈한다.
        SAFE_RELEASE(cachedPsoBlob);

        BE1::Str psoCacheFilename;
        GetCachedPSOFilename(psoHash, psoCacheFilename);
        BE_WARNLOG("Cached PSO file '%s' has a mismatched driver version\nThe file will be updated\n", psoCacheFilename.c_str());

        hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState->pso));
    }

    if (FAILED(hr)) {
        BE_WARNLOG("device->CreatePipelineState() failed, ERROR: 0x%x\n", hr);
        delete pipelineState;
        return nullptr;
    }

    graphicsPsoMap.Set(psoHash, pipelineState);

#ifndef _DEBUG
    if (!cachedPsoBlobEntry) {
        if (!cachedPsoBlob) {
            // cached PSO 가 없었다면 PSO 로부터 가져와서 파일에 저장한다.
            if (SUCCEEDED(pipelineState->pso->GetCachedBlob(&cachedPsoBlob))) {
                WriteCachedPSO(psoHash, cachedPsoBlob);
            }
        }

        // cached PSO 를 캐싱한다.
        if (cachedPsoBlob) {
            cachedPsoBlobMap.Set(psoHash, cachedPsoBlob);
        }
    }
#endif

    return pipelineState;
}

RHI::PipelineState *D3D12Renderer::CreateComputePSO(const RHI::Shader *computeShader) {
    const D3D12Shader *cs = static_cast<const D3D12Shader *>(computeShader);
    // compute shader 외에 다른 상태가 없으므로 compute shader hash 값을 PSO hash 값으로 사용한다.
    const uint64_t psoHash = cs->hash; 
    const auto *psoEntry = computePsoMap.Get(psoHash);
    if (psoEntry) {
        return psoEntry->second;
    }

    D3D12PipelineState *pipelineState = new D3D12PipelineState;
    pipelineState->hash = psoHash;
    pipelineState->graphics = false;

    cs->rootSignature->AddRef();
    cs->rootSignatureDeserializer->AddRef();

    D3D12PipelineState::ComputePSStream stream = {};
    stream.cs = { cs->compiledShaderData, cs->compiledShaderDataSize };
    stream.flags = CD3DX12_PIPELINE_STATE_STREAM_FLAGS(D3D12_PIPELINE_STATE_FLAG_NONE);
    stream.rootSignature = cs->rootSignature;

    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
    streamDesc.pPipelineStateSubobjectStream = &stream;
    streamDesc.SizeInBytes = sizeof(stream);

    HRESULT hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState->pso));
    if (FAILED(hr)) {
        BE_WARNLOG("device->CreatePipelineState() failed, ERROR: 0x%x\n", hr);
        delete pipelineState;
        return nullptr;
    }

    computePsoMap.Set(psoHash, pipelineState);

    return pipelineState;
}

void D3D12Renderer::DestroyPSO(RHI::PipelineState *pipelineState, bool immediate) {
    int oldRefCount = pipelineState->refCount.fetch_sub(1);
    if (oldRefCount > 1) {
        return;
    }

    if (pipelineState->graphics) {
        graphicsPsoMap.Remove(pipelineState->hash);
    } else {
        computePsoMap.Remove(pipelineState->hash);
    }

    if (immediate) {
        delete pipelineState;
    } else {
        MarkForDelete(pipelineState);
    }
}

void D3D12Renderer::GetCachedPSOFilename(const uint64_t hash, BE1::Str &outFilename) const {
    outFilename = psoCacheDir;
    outFilename.AppendPath(BE1::va("%016llx", hash));
    outFilename.SetFileExtension(".pso");
}

bool D3D12Renderer::LoadCachedPSO(const uint64_t hash, ID3DBlob **cachedPSOBlob) {
    BE1::Str filename;
    GetCachedPSOFilename(hash, filename);

    BE1::PlatformFileMapping *fileMapping = BE1::PlatformFileMapping::OpenFileRead(filename);
    if (!fileMapping) {
        return false;
    }

    size_t fileSize = fileMapping->GetSize();
    if (FAILED(D3DCreateBlob(fileSize, cachedPSOBlob))) {
        delete fileMapping;
        return false;
    }

    std::memcpy((*cachedPSOBlob)->GetBufferPointer(), (const byte *)fileMapping->GetData(), fileSize);

    delete fileMapping;
    return true;
}

void D3D12Renderer::WriteCachedPSO(const uint64_t hash, ID3DBlob *cachedPSOBlob) {
    if (!cachedPSOBlob || cachedPSOBlob->GetBufferSize() == 0) {
        return;
    }

    BE1::Str filename;
    GetCachedPSOFilename(hash, filename);
    BE1::PlatformFile *file = (BE1::PlatformFile *)BE1::PlatformFile::OpenFileWrite(filename);
    if (!file) {
        return;
    }

    file->Write(cachedPSOBlob->GetBufferPointer(), cachedPSOBlob->GetBufferSize());

    delete file;
}

RHI::PipelineState *D3D12Renderer::CreateBasicPSO(ID3D12RootSignature *rootSignature, const D3D12_SHADER_BYTECODE &byteCodeVS, const D3D12_SHADER_BYTECODE &byteCodePS, const D3D12_INPUT_LAYOUT_DESC &inputLayout) {
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL
    };
    ID3D12PipelineState *pso = nullptr;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.VS = byteCodeVS;
    psoDesc.PS = byteCodePS;

    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
        psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
    }

    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.RasterizerState.DepthBias = 0;
    psoDesc.RasterizerState.DepthBiasClamp = 0;
    psoDesc.RasterizerState.DepthClipEnable = FALSE;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DepthStencilState.StencilReadMask = 0xFF;
    psoDesc.DepthStencilState.StencilWriteMask = 0xFF;
    psoDesc.DepthStencilState.FrontFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    psoDesc.DepthStencilState.BackFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

    psoDesc.InputLayout = inputLayout;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    // NOTE: 나중에 호출할 SetGraphicsRootSignature() 에서 PSO 에 지정된 RootSignature 와 다르면 안된다.
    // 여기서 RootSignature 를 지정하는 이유는 파이프라인 호환성 검사 및 최적화 때문이다.
    psoDesc.pRootSignature = rootSignature;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr)) {
        BE_WARNLOG("device->CreateGraphicsPipelineState() failed, ERROR: 0x%x\n", hr);
        return nullptr;
    }

    D3D12PipelineState *pipelineState = new D3D12PipelineState;
    pipelineState->pso = pso;
    return pipelineState;
}

RHI::PipelineState *D3D12Renderer::CreateBasicPSO(ID3D12RootSignature *rootSignature, const char *shaderFilename, const D3D12_INPUT_LAYOUT_DESC &inputLayout) {
    char *shaderText;
    int shaderTextSize = BE1::fileSystem.LoadFile(shaderFilename, true, (void **)&shaderText);
    if (!shaderText) {
        return nullptr;
    }

    D3D12Shader *vs = static_cast<D3D12Shader *>(CreateShader(RHI::ShaderModel::SM_5_0, RHI::ShaderStage::Vertex, shaderFilename, shaderText, shaderTextSize, "VSMain"));
    D3D12Shader *ps = static_cast<D3D12Shader *>(CreateShader(RHI::ShaderModel::SM_5_0, RHI::ShaderStage::Fragment, shaderFilename, shaderText, shaderTextSize, "PSMain"));

    BE1::fileSystem.FreeFile(shaderText);

    if (!vs || !ps) {
        SAFE_DELETE(vs);
        SAFE_DELETE(ps);
        return nullptr;
    }

    D3D12_SHADER_BYTECODE byteCodeVS = CD3DX12_SHADER_BYTECODE(vs->compiledShaderData, vs->compiledShaderDataSize);
    D3D12_SHADER_BYTECODE byteCodePS = CD3DX12_SHADER_BYTECODE(ps->compiledShaderData, ps->compiledShaderDataSize);

    RHI::PipelineState *pso = CreateBasicPSO(rootSignature, byteCodeVS, byteCodePS, inputLayout);

    SAFE_DELETE(vs);
    SAFE_DELETE(ps);

    return pso;
}

ID3D12PipelineState *D3D12Renderer::CreatePSOFromLibrary(const D3D12_PIPELINE_STATE_STREAM_DESC *streamDesc, ID3D12PipelineLibrary1 *library, const TCHAR *name) {
    ID3D12PipelineState *pso = nullptr;
    HRESULT hr;

    if (library) {
        hr = library->LoadPipeline(name, streamDesc, IID_PPV_ARGS(&pso));
        if (hr == E_INVALIDARG) {
            hr = device->CreatePipelineState(streamDesc, IID_PPV_ARGS(&pso));
            if (SUCCEEDED(hr)) {
                library->StorePipeline(name, pso);
            }
        }
    } else {
        hr = device->CreatePipelineState(streamDesc, IID_PPV_ARGS(&pso));
        if (FAILED(hr)) {
            BE_ERRLOG("Failed to create pipeline state with name %s, ERROR: 0x%x.", name, hr);
        }
    }

    return pso;
}

void D3D12Renderer::SetPSO(RHI::CommandList *commandList, const RHI::PipelineState *pipelineState) {
    D3D12CommandList *d3d12CommandList = static_cast<D3D12CommandList *>(commandList);
    d3d12CommandList->SetPipelineState(pipelineState);
}

void D3D12Renderer::BindRootParameters(D3D12CommandList *commandList, bool graphics) {
    D3D12FrameThreadData *threadData = static_cast<D3D12FrameThreadData *>(commandList->GetFrameThreadData());
    D3D12RootDescriptorPool *rootDescriptorPool = threadData->rootDescriptorPool;
    uint64_t &rootParameterDirtyMask = graphics ? commandList->graphicsRootParametersDirtyMask : commandList->computeRootParametersDirtyMask;
    if (rootParameterDirtyMask == 0) {
        return;
    }

    const D3D12_ROOT_SIGNATURE_DESC1 &rootSignatureDesc = commandList->currentPSO->rootSignatureDesc->Desc_1_1;

    for (int rootParameterIndex = 0; rootParameterIndex < rootSignatureDesc.NumParameters; ++rootParameterIndex) {
        // NOTE: dirty mask 를 이용하여, 바뀐 루트 파라미터들만 바인딩한다.
        // 전체 리소스 바인딩 중 일부만 바꾸는 경우 안바뀐 부분의 리소스 바인딩을 생략할 수 있다.
        const uint64_t rootParameterBitMask = BIT64(rootParameterIndex);
        if (!(rootParameterDirtyMask & rootParameterBitMask)) {
            continue;
        }
        rootParameterDirtyMask ^= rootParameterBitMask;

        const D3D12_ROOT_PARAMETER1 *rootParameter = &rootSignatureDesc.pParameters[rootParameterIndex];

        if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            int numDescriptors = 0;

            // 디스크립터 테이블이 사용하는 전체 디스크립터 개수 계산
            for (int rangeIndex = 0; rangeIndex < rootParameter->DescriptorTable.NumDescriptorRanges; ++rangeIndex) {
                const D3D12_DESCRIPTOR_RANGE1 &descriptorRange = rootParameter->DescriptorTable.pDescriptorRanges[rangeIndex];

                numDescriptors += descriptorRange.NumDescriptors;
            }

            // 디스크립터 풀에서 디스크립터 테이블 할당
            D3D12_CPU_DESCRIPTOR_HANDLE cpuRootDescriptorStart;
            D3D12_GPU_DESCRIPTOR_HANDLE gpuRootDescriptorStart;
            if (!rootDescriptorPool->AllocRange(numDescriptors, &cpuRootDescriptorStart, &gpuRootDescriptorStart)) {
                return;
            }

            int descriptorOffset = 0;

            // 필요한 디스크립터들을 GPU 측 디스크립터 테이블에 복사
            for (int rangeIndex = 0; rangeIndex < rootParameter->DescriptorTable.NumDescriptorRanges; ++rangeIndex) {
                const D3D12_DESCRIPTOR_RANGE1 &descriptorRange = rootParameter->DescriptorTable.pDescriptorRanges[rangeIndex];
                CD3DX12_CPU_DESCRIPTOR_HANDLE destDescriptorHandle(cpuRootDescriptorStart, descriptorOffset, rootDescriptorPool->descriptorHandleSize);

                assert(descriptorOffset < COUNT_OF(threadData->tableCpuDescriptorHandles[rootParameterIndex]));

                switch (descriptorRange.RangeType) {
                case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                    device->CopyDescriptorsSimple(descriptorRange.NumDescriptors, destDescriptorHandle, threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorOffset], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                    device->CopyDescriptorsSimple(descriptorRange.NumDescriptors, destDescriptorHandle, threadData->tableCpuDescriptorHandles[rootParameterIndex][descriptorOffset], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
                    break;
                }

                descriptorOffset += descriptorRange.NumDescriptors;
            }

            // 루트 파라미터 별 GPU 디스크립터 테이블을 기록
            threadData->tableGpuDescriptorStarts[rootParameterIndex] = gpuRootDescriptorStart;

            // 사용할 디스크립터 테이블 설정
            if (graphics) {
                commandList->GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(rootParameterIndex, gpuRootDescriptorStart);
            } else {
                commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(rootParameterIndex, gpuRootDescriptorStart);
            }
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
            // 루트 상수 설정
            UINT num32BitValues = rootParameter->Constants.Num32BitValues;
            // NOTE: 현재는 일부만 세팅하는 경우는 없다고 가정한다.
            if (graphics) {
                commandList->GetGraphicsCommandList()->SetGraphicsRoot32BitConstants(rootParameterIndex, num32BitValues, threadData->rootConstants, 0);
            } else {
                commandList->GetGraphicsCommandList()->SetComputeRoot32BitConstants(rootParameterIndex, num32BitValues, threadData->rootConstants, 0);
            }
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV) {
            // 루트 레벨 CBV 설정
            UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
            const RHI::GPUResource *cbvResource = threadData->cbvResources[shaderRegister];
            if (cbvResource) {
                D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = reinterpret_cast<ID3D12Resource *>(cbvResource->GetNativeResource())->GetGPUVirtualAddress();
                if (graphics) {
                    commandList->GetGraphicsCommandList()->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuAddress);
                } else {
                    commandList->GetGraphicsCommandList()->SetComputeRootConstantBufferView(rootParameterIndex, gpuAddress);
                }
            }
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV) {
            // 루트 레벨 SRV 설정
            UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
            const RHI::GPUResource *srvResource = threadData->srvResources[shaderRegister];
            if (srvResource) {
                D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = reinterpret_cast<ID3D12Resource *>(srvResource->GetNativeResource())->GetGPUVirtualAddress();
                if (graphics) {
                    commandList->GetGraphicsCommandList()->SetGraphicsRootShaderResourceView(rootParameterIndex, gpuAddress);
                } else {
                    commandList->GetGraphicsCommandList()->SetComputeRootShaderResourceView(rootParameterIndex, gpuAddress);
                }
            }
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV) {
            // 루트 레벨 UAV 설정
            UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
            const RHI::GPUResource *uavResource = threadData->uavResources[shaderRegister];
            if (uavResource) {
                D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = reinterpret_cast<ID3D12Resource *>(uavResource->GetNativeResource())->GetGPUVirtualAddress();
                if (graphics) {
                    commandList->GetGraphicsCommandList()->SetGraphicsRootUnorderedAccessView(rootParameterIndex, gpuAddress);
                } else {
                    commandList->GetGraphicsCommandList()->SetComputeRootUnorderedAccessView(rootParameterIndex, gpuAddress);
                }
            }
        }
    }
}
