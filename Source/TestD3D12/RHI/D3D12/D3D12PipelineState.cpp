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
#include "D3D12Renderer.h"
#include "D3D12PipelineState.h"
#include "D3D12Shader.h"
#include "D3D12CommandList.h"
#include "D3D12ShaderVisibleDescriptorHeap.h"
#include <cmath>
#include <limits>

static INT ToD3D12DepthBias(float constantDepthBias) {
    const float roundedDepthBias = std::round(constantDepthBias);
    if (roundedDepthBias <= static_cast<float>(std::numeric_limits<INT>::min())) {
        return std::numeric_limits<INT>::min();
    }
    if (roundedDepthBias >= static_cast<float>(std::numeric_limits<INT>::max())) {
        return std::numeric_limits<INT>::max();
    }
    return static_cast<INT>(roundedDepthBias);
}

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
        return D3D12_STENCIL_OP_INCR_SAT;
    case RHI::StencilOp::Decr:
        return D3D12_STENCIL_OP_DECR_SAT;
    case RHI::StencilOp::IncrWrap:
        return D3D12_STENCIL_OP_INCR;
    case RHI::StencilOp::DecrWrap:
        return D3D12_STENCIL_OP_DECR;
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

RHI::PipelineState *D3D12Renderer::CreateGraphicsPSO(const RHI::PipelineStateDesc *psDesc) {
    struct GraphicsPSStream1 {
        CD3DX12_PIPELINE_STATE_STREAM_FLAGS flags;
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopologyType;
    };

    struct GraphicsPSStream2 {
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blendDesc;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 depthStencil;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT depthStencilFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rasterizer;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS renderTargetFormats;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC sampleDesc;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK sampleMask;
    };

    struct GraphicsPSStream {
        GraphicsPSStream1 stream1;
        CD3DX12_PIPELINE_STATE_STREAM_VS vs;
        CD3DX12_PIPELINE_STATE_STREAM_GS gs;
        CD3DX12_PIPELINE_STATE_STREAM_HS hs;
        CD3DX12_PIPELINE_STATE_STREAM_DS ds;
        CD3DX12_PIPELINE_STATE_STREAM_PS ps;
        //CD3DX12_PIPELINE_STATE_STREAM_MS ms;
        //CD3DX12_PIPELINE_STATE_STREAM_AS as;
        GraphicsPSStream2 stream2;
        CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO cachedPSO;
    };

    struct PSOHashData {
        struct ShaderHashData {
            uint64_t vsHash = 0;
            uint64_t gsHash = 0;
            uint64_t hsHash = 0;
            uint64_t dsHash = 0;
            uint64_t psHash = 0;
        } shaderHashData;
        uint64_t inputLayoutHash = 0;
        uint64_t blendHash = 0;
        uint64_t depthStencilHash = 0;
        uint64_t rasterizerHash = 0;
        uint64_t renderDestHash = 0;
        uint32_t primitiveTopologyType = 0;
        uint32_t sampleMask = 0xFFFFFFFF;
    } psoHashData;
    // psoHashData 변수 전체로 hash 키를 만들기 때문에 패딩이 없도록 해야한다.
    static_assert(std::has_unique_object_representations_v<PSOHashData>);

    ID3D12RootSignature *rootSignature = nullptr;
    ID3D12VersionedRootSignatureDeserializer *rootSignatureDeserializer = nullptr;
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *rootSignatureDesc = nullptr;

    if (psDesc->vs) {
        const D3D12Shader *vs = static_cast<const D3D12Shader *>(psDesc->vs);
        psoHashData.shaderHashData.vsHash = vs->hash;
        if (!rootSignature && vs->rootSignature) {
            rootSignature = vs->rootSignature;
            rootSignatureDeserializer = vs->rootSignatureDeserializer;
            rootSignatureDesc = vs->rootSignatureDesc;
        }
    }

    if (psDesc->gs) {
        const D3D12Shader *gs = static_cast<const D3D12Shader *>(psDesc->gs);
        psoHashData.shaderHashData.gsHash = gs->hash;
        if (!rootSignature && gs->rootSignature) {
            rootSignature = gs->rootSignature;
            rootSignatureDeserializer = gs->rootSignatureDeserializer;
            rootSignatureDesc = gs->rootSignatureDesc;
        }
    }

    if (psDesc->hs) {
        const D3D12Shader *hs = static_cast<const D3D12Shader *>(psDesc->hs);
        psoHashData.shaderHashData.hsHash = hs->hash;
        if (!rootSignature && hs->rootSignature) {
            rootSignature = hs->rootSignature;
            rootSignatureDeserializer = hs->rootSignatureDeserializer;
            rootSignatureDesc = hs->rootSignatureDesc;
        }
    }

    if (psDesc->ds) {
        const D3D12Shader *ds = static_cast<const D3D12Shader *>(psDesc->ds);
        psoHashData.shaderHashData.dsHash = ds->hash;
        if (!rootSignature && ds->rootSignature) {
            rootSignature = ds->rootSignature;
            rootSignatureDeserializer = ds->rootSignatureDeserializer;
            rootSignatureDesc = ds->rootSignatureDesc;
        }
    }

    if (psDesc->ps) {
        const D3D12Shader *ps = static_cast<const D3D12Shader *>(psDesc->ps);
        psoHashData.shaderHashData.psHash = ps->hash;
        if (!rootSignature && ps->rootSignature) {
            rootSignature = ps->rootSignature;
            rootSignatureDeserializer = ps->rootSignatureDeserializer;
            rootSignatureDesc = ps->rootSignatureDesc;
        }
    }

    if (psDesc->inputLayout) {
        size_t hash = 0;
        for (const RHI::InputLayoutElement &element : psDesc->inputLayout->elements) {
            hash = hash_combine(hash, element);
        }
        psoHashData.inputLayoutHash = static_cast<uint64_t>(hash);
    }

    if (psDesc->blendState) {
        std::hash<RHI::BlendState> hasher;
        psoHashData.blendHash = static_cast<uint64_t>(hasher(*psDesc->blendState));
    }

    if (psDesc->depthStencilState) {
        std::hash<RHI::DepthStencilState> hasher;
        psoHashData.depthStencilHash = static_cast<uint64_t>(hasher(*psDesc->depthStencilState));
    }

    if (psDesc->rasterizerState) {
        std::hash<RHI::RasterizerState> hasher;
        psoHashData.rasterizerHash = static_cast<uint64_t>(hasher(*psDesc->rasterizerState));
    }

    if (psDesc->renderDest) {
        psoHashData.renderDestHash = psDesc->renderDest->GetHash();
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = ToD3D12TopologyType(psDesc->primitiveTopology);
    psoHashData.primitiveTopologyType = primitiveTopologyType;
    psoHashData.sampleMask = psDesc->sampleMask;

    // 전체 hash 값으로 완전히 동일한 PSO 가 존재하는지 찾아본다.
    const uint64_t psoHash = BE1::CityHash64((char *)&psoHashData, sizeof(psoHashData));
    {
        BE1::ScopedLock lock(psoMapMutex);
        const auto *psoEntry = graphicsPsoMap.Get(psoHash);
        if (psoEntry) {
            // 동일한 PSO 를 찾았다면, 레퍼런스 카운트를 증가시키고 리턴한다.
            D3D12PipelineState *pso = psoEntry->second;
            pso->refCount.fetch_add(1);
            return pso;
        }
    }

    // 없다면 PSO 객체를 새로 만든다.
    D3D12PipelineState *pipelineState = new D3D12PipelineState;
    pipelineState->hash = psoHash;
    pipelineState->sortId = nextGraphicsPipelineStateSortId.fetch_add(1);
    pipelineState->isGraphics = true;

    // PSO 객체를 만들려면 PSO 스트림을 구성해야 한다.
    // PSO 스트림이란, PSO 객체를 생성하는데 필요한 모든 정보를 담은 구조체로, D3D12PipelineState::GraphicsPSStream 구조체로 정의되어 있다.
    uint32_t psoStreamSize = 0;
    void *psoStream = nullptr;
    GraphicsPSStream1 *stream1 = nullptr;
    GraphicsPSStream2 *stream2 = nullptr;

    psoStreamSize = sizeof(GraphicsPSStream);
    psoStream = _alloca16(psoStreamSize);
    new (psoStream) GraphicsPSStream();

    GraphicsPSStream *stream = static_cast<GraphicsPSStream *>(psoStream);
    stream1 = &stream->stream1;
    stream2 = &stream->stream2;

#ifdef USE_CACHED_PSO_BLOB
    // PSO hash 값으로 동일한 cached PSO 가 존재하는지 찾아보고, 찾았다면 재활용한다.
    bool cachedPsoBlobEntry = false;

    // cachedPsoBlob 은 미리 컴파일된 cached PSO 정보가 담긴 ID3DBlob 객체이다.
    Microsoft::WRL::ComPtr<ID3DBlob> cachedPsoBlob;
    {
        BE1::ScopedLock lock(psoMapMutex);
        const auto *entry = cachedPsoBlobMap.Get(psoHash);
        if (entry) {
            // 다른 스레드가 캐시를 무효화해도 PSO 생성 중에는 blob 을 유지한다.
            cachedPsoBlob = entry->second;
            cachedPsoBlobEntry = true;
        }
    }

    // cached PSO 가 없다면 파일로 저장된 cached PSO 파일을 로딩해본다.
    // NOTE: hash 값 충돌로 엉뚱한 cached PSO 파일을 읽어오지는 않는지 체크 필요
    if (!cachedPsoBlob) {
        LoadCachedPSO(psoHash, cachedPsoBlob.GetAddressOf());
    }

    // PSO 스트림에 cached PSO blob 정보를 담아서 PSO 를 생성하면, D3D12 디바이스가 cached PSO blob 정보를 이용해서 PSO 객체를 빠르게 생성할 수 있다.
    // cached PSO blob 이 있어도 동일한 PSO 를 설명하는 stream 정보는 다시 구성해야 한다.
    if (cachedPsoBlob) {
        stream->cachedPSO = { cachedPsoBlob->GetBufferPointer(), cachedPsoBlob->GetBufferSize() };
    }
#endif

    if (psDesc->vs) {
        const D3D12Shader *vs = static_cast<const D3D12Shader *>(psDesc->vs);
        stream->vs = { vs->compiledShaderData, vs->compiledShaderDataSize };
    }

    if (psDesc->gs) {
        const D3D12Shader *gs = static_cast<const D3D12Shader *>(psDesc->gs);
        stream->gs = { gs->compiledShaderData, gs->compiledShaderDataSize };
    }

    if (psDesc->hs) {
        const D3D12Shader *hs = static_cast<const D3D12Shader *>(psDesc->hs);
        stream->hs = { hs->compiledShaderData, hs->compiledShaderDataSize };
    }

    if (psDesc->ds) {
        const D3D12Shader *ds = static_cast<const D3D12Shader *>(psDesc->ds);
        stream->ds = { ds->compiledShaderData, ds->compiledShaderDataSize };
    }

    if (psDesc->ps) {
        const D3D12Shader *ps = static_cast<const D3D12Shader *>(psDesc->ps);
        stream->ps = { ps->compiledShaderData, ps->compiledShaderDataSize };
    }

    // Flags
    stream1->flags = CD3DX12_PIPELINE_STATE_STREAM_FLAGS(D3D12_PIPELINE_STATE_FLAG_NONE);

    // RootSignature
    if (rootSignature) {
        // rootSignature 를 유지하기 위해 레퍼런스 카운트를 증가시킨다.
        rootSignature->AddRef();
        // rootSignatureDesc 를 유지하기 위해 rootSignatureDeserializer 의 레퍼런스 카운트를 증가시킨다.
        rootSignatureDeserializer->AddRef();

        pipelineState->rootSignature = rootSignature;
        pipelineState->rootSignatureDesc = rootSignatureDesc;

        stream1->rootSignature = pipelineState->rootSignature;

        // 루트 시그니처의 모든 리소스에 대한 바인딩 정보를 미리 계산한다.
        pipelineState->binder.Init(rootSignatureDesc->Desc_1_1);
    }

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    BE1::Array<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

    if (psDesc->inputLayout) {
        int numElements = psDesc->inputLayout->elements.Count();
        inputElementDescs.SetCount(numElements);

        for (int i = 0; i < numElements; ++i) {
            const RHI::InputLayoutElement *element = &psDesc->inputLayout->elements[i];
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
    pipelineState->primitiveTopology = ToD3DPrimitiveTopology(psDesc->primitiveTopology);

    // BlendState
    if (psDesc->blendState) {
        CD3DX12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = psDesc->blendState->alphaToCoverageEnabled;
        blendDesc.IndependentBlendEnable = psDesc->blendState->independentBlendEnabled;

        for (int i = 0; i < COUNT_OF(blendDesc.RenderTarget); ++i) {
            const RHI::RenderTargetBlendState *renderTargetBlendState = &psDesc->blendState->renderTargets[i];
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
    if (psDesc->depthStencilState) {
        CD3DX12_DEPTH_STENCIL_DESC1 depthStencilDesc = {};
        depthStencilDesc.DepthEnable = psDesc->depthStencilState->depthTestEnabled;
        depthStencilDesc.DepthWriteMask = ToD3D12DepthWriteMask(psDesc->depthStencilState->depthWriteMask);
        depthStencilDesc.DepthFunc = ToD3D12ComparisonFunc(psDesc->depthStencilState->depthFunc);
        depthStencilDesc.StencilEnable = psDesc->depthStencilState->stencilTestEnabled;
        depthStencilDesc.StencilReadMask = psDesc->depthStencilState->stencilReadMask;
        depthStencilDesc.StencilWriteMask = psDesc->depthStencilState->stencilWriteMask;
        depthStencilDesc.FrontFace.StencilFailOp = ToD3D12StencilOp(psDesc->depthStencilState->frontFace.failOp);
        depthStencilDesc.FrontFace.StencilDepthFailOp = ToD3D12StencilOp(psDesc->depthStencilState->frontFace.depthFailOp);
        depthStencilDesc.FrontFace.StencilPassOp = ToD3D12StencilOp(psDesc->depthStencilState->frontFace.passOp);
        depthStencilDesc.FrontFace.StencilFunc = ToD3D12ComparisonFunc(psDesc->depthStencilState->frontFace.stencilFunc);
        depthStencilDesc.BackFace.StencilFailOp = ToD3D12StencilOp(psDesc->depthStencilState->backFace.failOp);
        depthStencilDesc.BackFace.StencilDepthFailOp = ToD3D12StencilOp(psDesc->depthStencilState->backFace.depthFailOp);
        depthStencilDesc.BackFace.StencilPassOp = ToD3D12StencilOp(psDesc->depthStencilState->backFace.passOp);
        depthStencilDesc.BackFace.StencilFunc = ToD3D12ComparisonFunc(psDesc->depthStencilState->backFace.stencilFunc);
        depthStencilDesc.DepthBoundsTestEnable = (supportsDepthBoundsTest && psDesc->depthStencilState->depthBoundTestEnabled) ? TRUE : FALSE;

        stream2->depthStencil = depthStencilDesc;
    }

    // RasterizeState
    if (psDesc->rasterizerState) {
        CD3DX12_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = ToD3D12FillMode(psDesc->rasterizerState->fillMode);
        rasterizerDesc.CullMode = ToD3D12CullMode(psDesc->rasterizerState->cullMode);
        rasterizerDesc.FrontCounterClockwise = TRUE;
        rasterizerDesc.DepthBias = ToD3D12DepthBias(psDesc->rasterizerState->constantDepthBias);
        rasterizerDesc.DepthBiasClamp = psDesc->rasterizerState->depthBiasClamp;
        rasterizerDesc.SlopeScaledDepthBias = psDesc->rasterizerState->slopeScaledDepthBias;
        rasterizerDesc.DepthClipEnable = psDesc->rasterizerState->depthClipEnabled;
        rasterizerDesc.MultisampleEnable = FALSE;
        rasterizerDesc.AntialiasedLineEnable = psDesc->rasterizerState->smoothLineEnabled;
        rasterizerDesc.ForcedSampleCount = 0;
        rasterizerDesc.ConservativeRaster = (supportsConservativeRasterization && psDesc->rasterizerState->conservativeRasterization) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        stream2->rasterizer = rasterizerDesc;
    }

    // RenderPass
    if (psDesc->renderDest) {
        D3D12_RT_FORMAT_ARRAY renderTargetFormatArray = {};
        renderTargetFormatArray.NumRenderTargets = psDesc->renderDest->renderTargetCount;

        for (int i = 0; i < renderTargetFormatArray.NumRenderTargets; ++i) {
            renderTargetFormatArray.RTFormats[i] = RHIFormatToDXGIFormat(psDesc->renderDest->renderTargetFormats[i]);
        }
        DXGI_FORMAT depthStencilFormat = RHIFormatToDXGIFormat(psDesc->renderDest->depthStencilFormat);

        DXGI_SAMPLE_DESC sampleDesc = {};
        sampleDesc.Count = psDesc->renderDest->sampleCount;
        sampleDesc.Quality = psDesc->renderDest->sampleQuality;

        stream2->depthStencilFormat = depthStencilFormat;
        stream2->renderTargetFormats = renderTargetFormatArray;
        stream2->sampleDesc = sampleDesc;
    }

    // SampleMask
    stream2->sampleMask = psDesc->sampleMask;

    // streamDesc 에 담아서 최종 PSO stream 완성
    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
    streamDesc.pPipelineStateSubobjectStream = psoStream;
    streamDesc.SizeInBytes = psoStreamSize;

    // streamDesc 를 통해 PSO 를 생성한다.
    HRESULT hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState->pso));
#ifdef USE_CACHED_PSO_BLOB
    if (FAILED(hr) && cachedPsoBlob) {
        // 캐시를 사용한 생성이 실패하면, 실패 원인과 관계없이 캐시 없이 한 번 재시도한다.
        stream->cachedPSO = CD3DX12_PIPELINE_STATE_STREAM_CACHED_PSO();

        {
            BE1::ScopedLock lock(psoMapMutex);
            const auto *entry = cachedPsoBlobMap.Get(psoHash);
            // 다른 스레드가 이미 교체한 캐시는 유지하고, 실패한 blob 의 맵 참조만 해제한다.
            if (entry && entry->second == cachedPsoBlob.Get()) {
                ID3DBlob *invalidBlob = entry->second;
                cachedPsoBlobMap.Remove(psoHash);
                invalidBlob->Release();
            }
        }

        // cached PSO 의 로컬 참조도 해제한다. 재시도 성공 시 새 blob 을 저장한다.
        cachedPsoBlob.Reset();
        cachedPsoBlobEntry = false;

        BE1::Str psoCacheFilename;
        GetCachedPSOFilename(psoHash, psoCacheFilename);
        BE_WARNLOG("Cached PSO '%s' failed, ERROR: 0x%x\nRetrying without the cache\n", psoCacheFilename.c_str(), hr);

        SAFE_RELEASE(pipelineState->pso);
        hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState->pso));
    }
#endif

    if (FAILED(hr)) {
        BE_WARNLOG("D3D12Renderer::CreateGraphicsPSO: CreatePipelineState() failed, ERROR: 0x%x\n", hr);
        delete pipelineState;
        return nullptr;
    }

#ifdef USE_CACHED_PSO_BLOB
    if (!cachedPsoBlob) {
        // cached PSO 가 없거나 무효화되었다면 PSO 로부터 가져와서 파일에 저장한다.
        if (SUCCEEDED(pipelineState->pso->GetCachedBlob(cachedPsoBlob.GetAddressOf()))) {
            WriteCachedPSO(psoHash, cachedPsoBlob.Get());
        }
    }
#endif

    {
        BE1::ScopedLock lock(psoMapMutex);
        const auto *psoEntry = graphicsPsoMap.Get(psoHash);
        if (psoEntry) {
            // lock 없이 PSO 를 생성하는 동안 같은 PSO 가 추가되었다면, 캐시된 PSO 를 사용한다.
            D3D12PipelineState *pso = psoEntry->second;
            pso->refCount.fetch_add(1);
            delete pipelineState;

            return pso;
        }

        graphicsPsoMap.Set(psoHash, pipelineState);

#ifdef USE_CACHED_PSO_BLOB
        // cached PSO 를 캐싱한다. blob 의 로컬 소유권을 맵으로 이전한다.
        if (!cachedPsoBlobEntry && cachedPsoBlob) {
            const auto *entry = cachedPsoBlobMap.Get(psoHash);
            if (!entry) {
                cachedPsoBlobMap.Set(psoHash, cachedPsoBlob.Detach());
            }
        }
#endif
    }

    return pipelineState;
}

RHI::PipelineState *D3D12Renderer::CreateComputePSO(const RHI::Shader *computeShader) {
    struct ComputePSStream {
        CD3DX12_PIPELINE_STATE_STREAM_FLAGS flags;
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS cs;
    };

    const D3D12Shader *cs = static_cast<const D3D12Shader *>(computeShader);

    // compute shader 외에는 다른 상태가 없으므로 compute shader hash 값을 PSO hash 값으로 사용한다.
    const uint64_t psoHash = cs->hash;
    {
        BE1::ScopedLock lock(psoMapMutex);
        const auto *psoEntry = computePsoMap.Get(psoHash);
        if (psoEntry) {
            // 동일한 PSO 를 찾았다면, 레퍼런스 카운트를 증가시키고 리턴한다.
            D3D12PipelineState *pso = psoEntry->second;
            pso->refCount.fetch_add(1);
            return pso;
        }
    }

    D3D12PipelineState *pipelineState = new D3D12PipelineState;
    pipelineState->hash = psoHash;
    pipelineState->isGraphics = false;

    // rootSignature 를 유지하기 위해 레퍼런스 카운트를 증가시킨다.
    cs->rootSignature->AddRef();

    // rootSignatureDesc 를 유지하기 위해 rootSignatureDeserializer 의 레퍼런스 카운트를 증가시킨다.
    cs->rootSignatureDeserializer->AddRef();

    pipelineState->rootSignature = cs->rootSignature;
    pipelineState->rootSignatureDesc = cs->rootSignatureDesc;

    // 루트 시그니처의 모든 리소스에 대한 바인딩 정보를 미리 계산한다.
    pipelineState->binder.Init(cs->rootSignatureDesc->Desc_1_1);

    ComputePSStream stream = {};
    stream.cs = { cs->compiledShaderData, cs->compiledShaderDataSize };
    stream.flags = CD3DX12_PIPELINE_STATE_STREAM_FLAGS(D3D12_PIPELINE_STATE_FLAG_NONE);
    stream.rootSignature = cs->rootSignature;

    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
    streamDesc.pPipelineStateSubobjectStream = &stream;
    streamDesc.SizeInBytes = sizeof(stream);

    // streamDesc 를 통해 PSO 를 생성한다.
    HRESULT hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState->pso));
    if (FAILED(hr)) {
        BE_WARNLOG("D3D12Renderer::CreateComputePSO: CreatePipelineState() failed, ERROR: 0x%x\n", hr);
        delete pipelineState;
        return nullptr;
    }

    {
        BE1::ScopedLock lock(psoMapMutex);
        const auto *psoEntry = computePsoMap.Get(psoHash);
        if (psoEntry) {
            // lock 없이 PSO 를 생성하는 동안 같은 PSO 가 추가되었다면, 캐시된 PSO 를 사용한다.
            D3D12PipelineState *pso = psoEntry->second;
            pso->refCount.fetch_add(1);
            delete pipelineState;
            return pso;
        }

        computePsoMap.Set(psoHash, pipelineState);
    }

    return pipelineState;
}

void D3D12Renderer::DestroyPSO(RHI::PipelineState *pipelineState, bool immediate) {
    {
        BE1::ScopedLock lock(psoMapMutex);
        int oldRefCount = pipelineState->refCount.fetch_sub(1);
        if (oldRefCount > 1) {
            return;
        }

        if (pipelineState->isGraphics) {
            graphicsPsoMap.Remove(pipelineState->hash);
        } else {
            computePsoMap.Remove(pipelineState->hash);
        }
    }

    if (immediate) {
        delete pipelineState;
    } else {
        DeleteGPUObject(pipelineState);
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
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
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
    pipelineState->sortId = nextGraphicsPipelineStateSortId.fetch_add(1);
    pipelineState->isGraphics = true;
    return pipelineState;
}

RHI::PipelineState *D3D12Renderer::CreateBasicPSO(ID3D12RootSignature *rootSignature, const char *shaderFilename, const D3D12_INPUT_LAYOUT_DESC &inputLayout) {
    char *shaderText;
    int shaderTextSize = BE1::fileSystem.LoadFile(shaderFilename, true, (void **)&shaderText);
    if (!shaderText) {
        return nullptr;
    }

    D3D12Shader *vs = static_cast<D3D12Shader *>(CreateShader(RHI::Renderer::MinimumShaderModel(), RHI::ShaderStage::Vertex, shaderFilename, shaderText, shaderTextSize, "VSMain"));
    D3D12Shader *ps = static_cast<D3D12Shader *>(CreateShader(RHI::Renderer::MinimumShaderModel(), RHI::ShaderStage::Fragment, shaderFilename, shaderText, shaderTextSize, "PSMain"));

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
    assert(pipelineState);
    D3D12CommandList *_commandList = static_cast<D3D12CommandList *>(commandList);
    _commandList->SetPipelineState(pipelineState);

    // direct descriptor heap (SM 6.6 부터 지원) 을 지원하지 않는다면 fallback 으로 모든 bindless 용 디스크립터 테이블을 세팅해줘야 한다.
    // 지원한다면 내부에서 아무일도 안한다.
    _commandList->SetBindlessRootDescriptorTables(bindlessRessourceDescriptorAllocator.GetBaseGPUDescriptorHandle(), bindlessSamplerDescriptorAllocator.GetBaseGPUDescriptorHandle());
}

// Draw 나 Dispatch 직전에 호출된다.
void D3D12Renderer::BindRootParameters(D3D12CommandList *commandList) {
    bool isGraphicsPSO = commandList->currentPSO->isGraphics;

    // NOTE: 64 bit dirty mask 를 이용해서 바뀐 루트 파라미터들만 바인딩한다.
    // 전체 리소스 바인딩 중 일부만 바꾸는 경우 안바뀐 부분의 리소스 바인딩을 생략할 수 있다.
    uint64_t &rootParameterDirtyMask = isGraphicsPSO ? commandList->graphicsRootParametersDirtyMask : commandList->computeRootParametersDirtyMask;
    if (rootParameterDirtyMask == 0) {
        return;
    }

    D3D12RootBindingData *rootBindingData = commandList->GetRootBindingData();
    D3D12DescriptorRangePool *resourceDescriptorRangePool = commandList->resourceDescriptorRangePool;
    D3D12DescriptorRangePool *samplerDescriptorRangePool = commandList->samplerDescriptorRangePool;
    ID3D12GraphicsCommandList6 *cmd = commandList->GetGraphicsCommandList();

    const D3D12_ROOT_SIGNATURE_DESC1 &rootSignatureDesc = commandList->currentPSO->rootSignatureDesc->Desc_1_1;

    for (int rootParameterIndex = 0; rootParameterIndex < rootSignatureDesc.NumParameters; ++rootParameterIndex) {
        // 루트 시그니처는 최대 64 DWORD 비용을 가진다.
        // 각 파라미터 타입에 따라 소비하는 DWORD 가 다르다.
        // 디스크립터 테이블은 1 DWORD, CBV/SRV/UAV 루트 디스크립터는 2 DWORD, Root 상수는 32bit 값 하나당 1 DWORD.
        const uint64_t rootParameterBitMask = BIT64(rootParameterIndex);
        if (!(rootParameterDirtyMask & rootParameterBitMask)) {
            continue;
        }
        rootParameterDirtyMask ^= rootParameterBitMask;

        const D3D12_ROOT_PARAMETER1 *rootParameter = &rootSignatureDesc.pParameters[rootParameterIndex];

        // 현재 루트 파라미터가 디스크립터 테이블인 경우
        if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            const D3D12_ROOT_DESCRIPTOR_TABLE1 &descriptorTableDesc = rootParameter->DescriptorTable;
            assert(descriptorTableDesc.NumDescriptorRanges > 0);

            // 각 디스크립터 range 의 개수가 1 인 디스크립터 테이블
            D3D12_CPU_DESCRIPTOR_HANDLE srcCpuDescriptorHandles[D3D12RootBindingData::MaxDescriptorsInDescriptorTable];
            int numTableDescriptors = 0;

            // 테이블에 복사할 원본 디스크립터 핸들을 수집하고 개수를 센다.
            // NOTE: 만약 rootBindingData 에 테이블의 일부만 세팅했다면, 나머지 항목은 남아있던 이전 디스크립터 핸들을 사용할 수 있음에 주의한다.
            for (int rangeIndex = 0; rangeIndex < descriptorTableDesc.NumDescriptorRanges; ++rangeIndex) {
                const D3D12_DESCRIPTOR_RANGE1 &descriptorRange = descriptorTableDesc.pDescriptorRanges[rangeIndex];
                if (descriptorRange.RegisterSpace != 0) {
                    continue;
                }

                assert(numTableDescriptors + descriptorRange.NumDescriptors <= COUNT_OF(rootBindingData->tableDescriptorHandles[rootParameterIndex]));

                for (UINT descriptorIndex = 0; descriptorIndex < descriptorRange.NumDescriptors; ++descriptorIndex) {
                    const D3D12_CPU_DESCRIPTOR_HANDLE &srcDescriptorHandle = rootBindingData->tableDescriptorHandles[rootParameterIndex][numTableDescriptors];
                    if (srcDescriptorHandle.ptr == 0) {
                        BE_ERRLOG("D3D12Renderer::BindRootParameters: Descriptor table contains an invalid descriptor handle\n");
                        return;
                    }

                    srcCpuDescriptorHandles[numTableDescriptors] = srcDescriptorHandle;
                    ++numTableDescriptors;
                }
            }

            // 루트 시그니처에 있는 디스크립터 테이블에 디스크립터가 하나도 없다?
            if (numTableDescriptors <= 0) {
                assert(0);
                continue;
            }

            D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType;
            D3D12DescriptorRangePool *descriptorRangePool;

            if (descriptorTableDesc.pDescriptorRanges[0].RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
                descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
                descriptorRangePool = samplerDescriptorRangePool;
            } else {
                descriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                descriptorRangePool = resourceDescriptorRangePool;
            }

            D3D12_CPU_DESCRIPTOR_HANDLE destCpuDescriptorTableBase;
            D3D12_GPU_DESCRIPTOR_HANDLE destGpuDescriptorTableBase;

            // 유효한 source 디스크립터들을 확인한 뒤, 연속된 디스크립터 슬롯들을 할당한다.
            if (!descriptorRangePool->AllocRange(numTableDescriptors, &destCpuDescriptorTableBase, &destGpuDescriptorTableBase)) {
                return;
            }

            const UINT destDescriptorRangeSize = numTableDescriptors; // 하나의 destination range 에 포함되는 디스크립터 개수

            // source 디스크립터 슬롯들은 연속하지 않을 수 있으므로 각각 크기가 1 인 디스크립터 range 들로 구성한다.
            // destination 디스크립터 슬롯들은 연속으로 존재하므로 하나의 복사 range 로 지정한다.
            device->CopyDescriptors(1, &destCpuDescriptorTableBase, &destDescriptorRangeSize, numTableDescriptors, srcCpuDescriptorHandles, nullptr, descriptorHeapType);

            // 루트 파라미터에 디스크립터 테이블의 시작 GPU 핸들을 바인딩한다.
            if (isGraphicsPSO) {
                cmd->SetGraphicsRootDescriptorTable(rootParameterIndex, destGpuDescriptorTableBase);
            } else {
                cmd->SetComputeRootDescriptorTable(rootParameterIndex, destGpuDescriptorTableBase);
            }
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
            // 루트 상수 설정 : 루트 파라미터에 연속된 32비트 상수 값을 세팅한다.
            // NOTE: offset 을 통해 상수 값들을 일부만 세팅하는 경우는 없다고 가정한다. (전체 상수를 세팅해야 함)
            UINT shaderRegister = rootParameter->Constants.ShaderRegister;
            const uint32_t* rootConstants = rootBindingData->rootConstants[shaderRegister];
            if (isGraphicsPSO) {
                cmd->SetGraphicsRoot32BitConstants(rootParameterIndex, rootParameter->Constants.Num32BitValues, rootConstants, 0);
            } else {
                cmd->SetComputeRoot32BitConstants(rootParameterIndex, rootParameter->Constants.Num32BitValues, rootConstants, 0);
            }
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV) {
            // 루트 CBV 설정 : 루트 파라미터에 CBV 로 사용할 버퍼의 GPU 주소를 직접 세팅한다.
            UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = rootBindingData->cbvGpuAddresses[shaderRegister];
            if (gpuAddress) {
                if (isGraphicsPSO) {
                    cmd->SetGraphicsRootConstantBufferView(rootParameterIndex, gpuAddress);
                } else {
                    cmd->SetComputeRootConstantBufferView(rootParameterIndex, gpuAddress);
                }
            }
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV) {
            // 루트 SRV 설정 : 루트 파라미터에 SRV 로 사용할 버퍼의 GPU 주소를 직접 세팅한다.
            UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = rootBindingData->srvGpuAddresses[shaderRegister];
            if (gpuAddress) {
                if (isGraphicsPSO) {
                    cmd->SetGraphicsRootShaderResourceView(rootParameterIndex, gpuAddress);
                } else {
                    cmd->SetComputeRootShaderResourceView(rootParameterIndex, gpuAddress);
                }
            }
        } else if (rootParameter->ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV) {
            // 루트 UAV 설정 : 루트 파라미터에 UAV 로 사용할 버퍼의 GPU 주소를 직접 세팅한다.
            UINT shaderRegister = rootParameter->Descriptor.ShaderRegister;
            D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = rootBindingData->uavGpuAddresses[shaderRegister];
            if (gpuAddress) {
                if (isGraphicsPSO) {
                    cmd->SetGraphicsRootUnorderedAccessView(rootParameterIndex, gpuAddress);
                } else {
                    cmd->SetComputeRootUnorderedAccessView(rootParameterIndex, gpuAddress);
                }
            }
        }
    }
}
