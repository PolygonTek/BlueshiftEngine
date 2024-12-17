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
#include "Platform/PlatformSystem.h"
#include "Platform/PlatformFile.h"
#include "Platform/Windows/PlatformWinUtils.h"
#include "D3D12Renderer.h"

static constexpr D3D12_FILL_MODE ToD3D12FillMode(RHIRenderer::FillMode fillMode) {
    switch (fillMode) {
    case RHIRenderer::FillMode::Wire:
        return D3D12_FILL_MODE_WIREFRAME;
    case RHIRenderer::FillMode::Solid:
        return D3D12_FILL_MODE_SOLID;
    }
    assert(0);
    return D3D12_FILL_MODE_SOLID;
}

static constexpr D3D12_CULL_MODE ToD3D12CullMode(RHIRenderer::CullMode cullMode) {
    switch (cullMode) {
    case RHIRenderer::CullMode::Back:
        return D3D12_CULL_MODE_BACK;
    case RHIRenderer::CullMode::Front:
        return D3D12_CULL_MODE_FRONT;
    case RHIRenderer::CullMode::None:
        return D3D12_CULL_MODE_NONE;
    }
    assert(0);
    return D3D12_CULL_MODE_NONE;
}

static constexpr UINT8 ToD3D12ColorWriteMask(RHIRenderer::ColorWriteMask colorWriteMask) {
    UINT8 mask = 0;
    if (static_cast<uint32_t>(colorWriteMask) & static_cast<uint32_t>(RHIRenderer::ColorWriteMask::Red)) {
        mask |= D3D12_COLOR_WRITE_ENABLE_RED;
    }
    if (static_cast<uint32_t>(colorWriteMask) & static_cast<uint32_t>(RHIRenderer::ColorWriteMask::Green)) {
        mask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
    }
    if (static_cast<uint32_t>(colorWriteMask) & static_cast<uint32_t>(RHIRenderer::ColorWriteMask::Blue)) {
        mask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
    }
    if (static_cast<uint32_t>(colorWriteMask) & static_cast<uint32_t>(RHIRenderer::ColorWriteMask::Alpha)) {
        mask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
    }
    return mask;
}

static constexpr D3D12_DEPTH_WRITE_MASK ToD3D12DepthWriteMask(RHIRenderer::DepthWriteMask depthWriteMask) {
    if (depthWriteMask == RHIRenderer::DepthWriteMask::Zero) {
        return D3D12_DEPTH_WRITE_MASK_ZERO;
    }
    if (depthWriteMask == RHIRenderer::DepthWriteMask::All) {
        return D3D12_DEPTH_WRITE_MASK_ALL;
    }
    assert(0);
    return D3D12_DEPTH_WRITE_MASK_ZERO;
}

static constexpr D3D12_COMPARISON_FUNC ToD3D12ComparisonFunc(RHIRenderer::ComparisonFunc comparisonFunc) {
    switch (comparisonFunc) {
    case RHIRenderer::ComparisonFunc::Always:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    case RHIRenderer::ComparisonFunc::Less:
        return D3D12_COMPARISON_FUNC_LESS;
    case RHIRenderer::ComparisonFunc::LEqual:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case RHIRenderer::ComparisonFunc::Equal:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case RHIRenderer::ComparisonFunc::NotEqual:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case RHIRenderer::ComparisonFunc::GEqual:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case RHIRenderer::ComparisonFunc::Greater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case RHIRenderer::ComparisonFunc::Never:
        return D3D12_COMPARISON_FUNC_NEVER;
    }
    assert(0);
    return D3D12_COMPARISON_FUNC_NEVER;
}

static constexpr D3D12_STENCIL_OP ToD3D12StencilOp(RHIRenderer::StencilOp stencilOp) {
    switch (stencilOp) {
    case RHIRenderer::StencilOp::Keep:
        return D3D12_STENCIL_OP_KEEP;
    case RHIRenderer::StencilOp::Zero:
        return D3D12_STENCIL_OP_ZERO;
    case RHIRenderer::StencilOp::Replace:
        return D3D12_STENCIL_OP_REPLACE;
    case RHIRenderer::StencilOp::Incr:
        return D3D12_STENCIL_OP_INCR;
    case RHIRenderer::StencilOp::Decr:
        return D3D12_STENCIL_OP_DECR;
    case RHIRenderer::StencilOp::IncrWrap:
        return D3D12_STENCIL_OP_INCR_SAT;
    case RHIRenderer::StencilOp::DecrWrap:
        return D3D12_STENCIL_OP_DECR_SAT;
    case RHIRenderer::StencilOp::Invert:
        return D3D12_STENCIL_OP_INVERT;
    }
    assert(0);
    return D3D12_STENCIL_OP_KEEP;
}

static constexpr D3D12_BLEND ToD3D12Blend(RHIRenderer::Blend blend) {
    switch (blend) {
    case RHIRenderer::Blend::Zero:
        return D3D12_BLEND_ZERO;
    case RHIRenderer::Blend::One:
        return D3D12_BLEND_ONE;
    case RHIRenderer::Blend::SrcColor:
        return D3D12_BLEND_SRC_COLOR;
    case RHIRenderer::Blend::InvSrcColor:
        return D3D12_BLEND_INV_SRC_COLOR;
    case RHIRenderer::Blend::SrcAlpha:
        return D3D12_BLEND_SRC_ALPHA;
    case RHIRenderer::Blend::InvSrcAlpha:
        return D3D12_BLEND_INV_SRC_ALPHA;
    case RHIRenderer::Blend::SrcAlphaSat:
        return D3D12_BLEND_SRC_ALPHA_SAT;
    case RHIRenderer::Blend::DestColor:
        return D3D12_BLEND_DEST_COLOR;
    case RHIRenderer::Blend::InvDestColor:
        return D3D12_BLEND_INV_DEST_COLOR;
    case RHIRenderer::Blend::DestAlpha:
        return D3D12_BLEND_DEST_ALPHA;
    case RHIRenderer::Blend::InvDestAlpha:
        return D3D12_BLEND_INV_DEST_ALPHA;
    case RHIRenderer::Blend::BlendFactor:
        return D3D12_BLEND_BLEND_FACTOR;
    case RHIRenderer::Blend::InvBlendFactor:
        return D3D12_BLEND_INV_BLEND_FACTOR;
    case RHIRenderer::Blend::Src1Color:
        return D3D12_BLEND_SRC1_COLOR;
    case RHIRenderer::Blend::InvSrc1Color:
        return D3D12_BLEND_INV_SRC1_COLOR;
    case RHIRenderer::Blend::Src1Alpha:
        return D3D12_BLEND_SRC1_ALPHA;
    case RHIRenderer::Blend::InvSrc1Alpha:
        return D3D12_BLEND_INV_SRC1_ALPHA;
    }
    assert(0);
    return D3D12_BLEND_ZERO;
}

static constexpr D3D12_BLEND_OP ToD3D12BlendOp(RHIRenderer::BlendOp blendOp) {
    switch (blendOp) {
    case RHIRenderer::BlendOp::Add:
        return D3D12_BLEND_OP_ADD;
    case RHIRenderer::BlendOp::Subtract:
        return D3D12_BLEND_OP_SUBTRACT;
    case RHIRenderer::BlendOp::ReverseSubtract:
        return D3D12_BLEND_OP_REV_SUBTRACT;
    case RHIRenderer::BlendOp::Min:
        return D3D12_BLEND_OP_MIN;
    case RHIRenderer::BlendOp::Max:
        return D3D12_BLEND_OP_MAX;
    }
    assert(0);
    return D3D12_BLEND_OP_ADD;
}

static constexpr DXGI_FORMAT ToD3D12InputLayoutElementFormat(RHIRenderer::InputLayoutElement::Format format) {
    switch (format) {
    case RHIRenderer::InputLayoutElement::Format::Unknown:
        return DXGI_FORMAT_UNKNOWN;
    case RHIRenderer::InputLayoutElement::Format::Float4:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case RHIRenderer::InputLayoutElement::Format::Float3:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case RHIRenderer::InputLayoutElement::Format::Float2:
        return DXGI_FORMAT_R32G32_FLOAT;
    case RHIRenderer::InputLayoutElement::Format::Float1:
        return DXGI_FORMAT_R32_FLOAT;
    case RHIRenderer::InputLayoutElement::Format::UInt4:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case RHIRenderer::InputLayoutElement::Format::UInt3:
        return DXGI_FORMAT_R32G32B32_UINT;
    case RHIRenderer::InputLayoutElement::Format::UInt2:
        return DXGI_FORMAT_R32G32_UINT;
    case RHIRenderer::InputLayoutElement::Format::UInt1:
        return DXGI_FORMAT_R32_UINT;
    case RHIRenderer::InputLayoutElement::Format::Int4:
        return DXGI_FORMAT_R32G32B32A32_SINT;
    case RHIRenderer::InputLayoutElement::Format::Int3:
        return DXGI_FORMAT_R32G32B32_SINT;
    case RHIRenderer::InputLayoutElement::Format::Int2:
        return DXGI_FORMAT_R32G32_SINT;
    case RHIRenderer::InputLayoutElement::Format::Int1:
        return DXGI_FORMAT_R32_SINT;
    case RHIRenderer::InputLayoutElement::Format::Half4:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case RHIRenderer::InputLayoutElement::Format::Half2:
        return DXGI_FORMAT_R16G16_FLOAT;
    case RHIRenderer::InputLayoutElement::Format::Half1:
        return DXGI_FORMAT_R16_FLOAT;
    case RHIRenderer::InputLayoutElement::Format::UShort4:
        return DXGI_FORMAT_R16G16B16A16_UINT;
    case RHIRenderer::InputLayoutElement::Format::UShort2:
        return DXGI_FORMAT_R16G16_UINT;
    case RHIRenderer::InputLayoutElement::Format::UShort1:
        return DXGI_FORMAT_R16_UINT;
    case RHIRenderer::InputLayoutElement::Format::UShort4N:
        return DXGI_FORMAT_R16G16B16A16_UNORM;
    case RHIRenderer::InputLayoutElement::Format::UShort2N:
        return DXGI_FORMAT_R16G16_UNORM;
    case RHIRenderer::InputLayoutElement::Format::UShort1N:
        return DXGI_FORMAT_R16_UNORM;
    case RHIRenderer::InputLayoutElement::Format::Short4:
        return DXGI_FORMAT_R16G16B16A16_SINT;
    case RHIRenderer::InputLayoutElement::Format::Short2:
        return DXGI_FORMAT_R16G16_SINT;
    case RHIRenderer::InputLayoutElement::Format::Short1:
        return DXGI_FORMAT_R16_SINT;
    case RHIRenderer::InputLayoutElement::Format::Short4N:
        return DXGI_FORMAT_R16G16B16A16_SNORM;
    case RHIRenderer::InputLayoutElement::Format::Short2N:
        return DXGI_FORMAT_R16G16_SNORM;
    case RHIRenderer::InputLayoutElement::Format::Short1N:
        return DXGI_FORMAT_R16_SNORM;
    case RHIRenderer::InputLayoutElement::Format::UByte4:
        return DXGI_FORMAT_R8G8B8A8_UINT;
    case RHIRenderer::InputLayoutElement::Format::UByte2:
        return DXGI_FORMAT_R8G8_UINT;
    case RHIRenderer::InputLayoutElement::Format::UByte1:
        return DXGI_FORMAT_R8_UINT;
    case RHIRenderer::InputLayoutElement::Format::UByte4N:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case RHIRenderer::InputLayoutElement::Format::UByte2N:
        return DXGI_FORMAT_R8G8_UNORM;
    case RHIRenderer::InputLayoutElement::Format::UByte1N:
        return DXGI_FORMAT_R8_UNORM;
    case RHIRenderer::InputLayoutElement::Format::Byte4:
        return DXGI_FORMAT_R8G8B8A8_SINT;
    case RHIRenderer::InputLayoutElement::Format::Byte2:
        return DXGI_FORMAT_R8G8_SINT;
    case RHIRenderer::InputLayoutElement::Format::Byte1:
        return DXGI_FORMAT_R8_SINT;
    case RHIRenderer::InputLayoutElement::Format::Byte4N:
        return DXGI_FORMAT_R8G8B8A8_SNORM;
    case RHIRenderer::InputLayoutElement::Format::Byte2N:
        return DXGI_FORMAT_R8G8_SNORM;
    case RHIRenderer::InputLayoutElement::Format::Byte1N:
        return DXGI_FORMAT_R8_SNORM;
    }
    assert(0);
    return DXGI_FORMAT_UNKNOWN;
}

static constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE ToD3DTopologyType(RHIRenderer::PrimitiveTopology primitiveTopology) {
    switch (primitiveTopology) {
    case RHIRenderer::PrimitiveTopology::PointList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case RHIRenderer::PrimitiveTopology::LineList:
    case RHIRenderer::PrimitiveTopology::LineStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case RHIRenderer::PrimitiveTopology::TriangleList:
    case RHIRenderer::PrimitiveTopology::TriangleStrip:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case RHIRenderer::PrimitiveTopology::PatchList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }
    assert(0);
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

void D3D12PipelineState::Release() {
    SAFE_RELEASE(pso);
    SAFE_RELEASE(rootSignature);
}

RHIRenderer::PipelineState *D3D12Renderer::CreatePSO(RHIRenderer::PipelineStateDesc *desc) {
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
        uint64_t renderPassHash = 0;
    } psoHashData;

    ID3D12RootSignature *rootSignature = nullptr;

    if (desc->vs) {
        const D3D12Shader *vs = static_cast<const D3D12Shader *>(desc->vs);
        psoHashData.shaderHashData.vsHash = vs->hash;
        if (!rootSignature && vs->rootSignature) {
            rootSignature = vs->rootSignature;
        }
    }

    if (desc->gs) {
        const D3D12Shader *gs = static_cast<const D3D12Shader *>(desc->gs);
        psoHashData.shaderHashData.gsHash = gs->hash;
        if (!rootSignature && gs->rootSignature) {
            rootSignature = gs->rootSignature;
        }
    }

    if (desc->hs) {
        const D3D12Shader *hs = static_cast<const D3D12Shader *>(desc->hs);
        psoHashData.shaderHashData.hsHash = hs->hash;
        if (!rootSignature && hs->rootSignature) {
            rootSignature = hs->rootSignature;
        }
    }

    if (desc->ds) {
        const D3D12Shader *ds = static_cast<const D3D12Shader *>(desc->ds);
        psoHashData.shaderHashData.dsHash = ds->hash;
        if (!rootSignature && ds->rootSignature) {
            rootSignature = ds->rootSignature;
        }
    }

    if (desc->ps) {
        const D3D12Shader *ps = static_cast<const D3D12Shader *>(desc->ps);
        psoHashData.shaderHashData.psHash = ps->hash;
        if (!rootSignature && ps->rootSignature) {
            rootSignature = ps->rootSignature;
        }
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = ToD3DTopologyType(desc->primitiveTopology);
    psoHashData.primitiveTopologyType = primitiveTopologyType;

    if (desc->inputLayout) {
        size_t hash = 0;
        for (int i = 0; i < desc->inputLayout->elements.Count(); ++i) {
            hash = hash_combine(hash, std::hash<InputLayoutElement>()(desc->inputLayout->elements[i]));
        }
        psoHashData.inputLayoutHash = static_cast<uint64_t>(hash);
    }

    if (desc->blendState) {
        std::hash<const BlendState *> hasher;
        psoHashData.blendHash = static_cast<uint64_t>(hasher(desc->blendState));
    }

    if (desc->depthStencilState) {
        std::hash<const DepthStencilState *> hasher;
        psoHashData.depthStencilHash = static_cast<uint64_t>(hasher(desc->depthStencilState));
    }

    if (desc->rasterizerState) {
        std::hash<const RasterizerState *> hasher;
        psoHashData.rasterizerHash = static_cast<uint64_t>(hasher(desc->rasterizerState));
    }

    if (desc->renderPass) {
        psoHashData.renderPassHash = desc->renderPass->GetHash();
    }

    // 전체 hash 값으로 완전히 동일한 PSO 가 존재하는지 찾아보고, 있으면 리턴한다.
    const uint64_t psoHash = CityHash64((char *)&psoHashData, sizeof(psoHashData));
    const auto *psoEntry = psoMap.Get(psoHash);
    if (psoEntry) {
        return psoEntry->second;
    }

    // 없다면 새로 만든다.
    D3D12PipelineState *pipelineState = new D3D12PipelineState;
    pipelineState->hash = psoHash;

    // combined shader hash 값으로 동일한 PSO cache 가 존재하는지 찾아보고, 있으면 재활용한다.
    const uint64_t combinedShaderHash = CityHash64((char *)&psoHashData.shaderHashData, sizeof(psoHashData.shaderHashData));
    const auto *cachedPsoBlobEntry = cachedPsoBlobMap.Get(combinedShaderHash);

    ID3DBlob *cachedPsoBlob = nullptr;
    if (cachedPsoBlobEntry) {
        cachedPsoBlob = cachedPsoBlobEntry->second;
    }

#ifndef _DEBUG
    // 없다면 PSO cache 파일을 로딩해본다.
    if (!cachedPsoBlob) {
        // FIXME: 원인 불명의 에러 수정할 것
        // D3D12 ERROR: ID3D12Device::CreateInputLayout: Encoded Signature size doesn't match specified size. [ STATE_CREATION ERROR #63: CREATEINPUTLAYOUT_UNPARSEABLEINPUTSIGNATURE]
        //LoadCachedPSO(combinedShaderHash, &cachedPsoBlob);
    }
#endif

    uint32_t psoStreamSize = 0;
    void *psoStream = nullptr;
    D3D12PipelineState::PipelineStateStream1 *stream1 = nullptr;
    D3D12PipelineState::PipelineStateStream2 *stream2 = nullptr;

    if (cachedPsoBlob) {
        // PSO cache 를 이용해서 shader stream 은 생략한다.
        psoStreamSize = sizeof(D3D12PipelineState::CachedPipelineStateStream);
        psoStream = _alloca16(psoStreamSize);
        new (psoStream) D3D12PipelineState::CachedPipelineStateStream();

        D3D12PipelineState::CachedPipelineStateStream *cachedStream = static_cast<D3D12PipelineState::CachedPipelineStateStream *>(psoStream);
        stream1 = &cachedStream->stream1;
        stream2 = &cachedStream->stream2;

        cachedStream->shaderCachedPSO = { cachedPsoBlob->GetBufferPointer(), cachedPsoBlob->GetBufferSize() };
    } else {
        // 그래도 없으면 PSO 를 통째로 새로 만든다.
        psoStreamSize = sizeof(D3D12PipelineState::PipelineStateStream);
        psoStream = _alloca16(psoStreamSize);
        new (psoStream) D3D12PipelineState::PipelineStateStream();

        D3D12PipelineState::PipelineStateStream *nonCachedStream = static_cast<D3D12PipelineState::PipelineStateStream *>(psoStream);
        stream1 = &nonCachedStream->stream1;
        stream2 = &nonCachedStream->stream2;

        if (desc->vs) {
            const D3D12Shader *vs = static_cast<const D3D12Shader *>(desc->vs);
            nonCachedStream->vs = { vs->compiledShaderData, vs->compiledShaderDataSize };
        }

        if (desc->gs) {
            const D3D12Shader *gs = static_cast<const D3D12Shader *>(desc->gs);
            nonCachedStream->gs = { gs->compiledShaderData, gs->compiledShaderDataSize };
        }

        if (desc->hs) {
            const D3D12Shader *hs = static_cast<const D3D12Shader *>(desc->hs);
            nonCachedStream->hs = { hs->compiledShaderData, hs->compiledShaderDataSize };
        }

        if (desc->ds) {
            const D3D12Shader *ds = static_cast<const D3D12Shader *>(desc->ds);
            nonCachedStream->ds = { ds->compiledShaderData, ds->compiledShaderDataSize };
        }

        if (desc->ps) {
            const D3D12Shader *ps = static_cast<const D3D12Shader *>(desc->ps);
            nonCachedStream->ps = { ps->compiledShaderData, ps->compiledShaderDataSize };
        }
    }

    // Flags
    stream1->flags = CD3DX12_PIPELINE_STATE_STREAM_FLAGS(D3D12_PIPELINE_STATE_FLAG_NONE);

    // RootSignature
    if (rootSignature) {
        pipelineState->rootSignature = rootSignature;
        pipelineState->rootSignature->AddRef();

        stream1->rootSignature = pipelineState->rootSignature;
    }

    // InputLayout
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
    Array<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

    if (desc->inputLayout) {
        int numElements = desc->inputLayout->elements.Count();
        inputElementDescs.SetCount(numElements);

        for (int i = 0; i < numElements; ++i) {
            const InputLayoutElement *element = &desc->inputLayout->elements[i];
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
    stream1->primitiveTopology = primitiveTopologyType;

    // BlendState
    if (desc->blendState) {
        CD3DX12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = desc->blendState->alphaToCoverageEnabled;
        blendDesc.IndependentBlendEnable = desc->blendState->independentBlendEnabled;

        for (int i = 0; i < COUNT_OF(blendDesc.RenderTarget); ++i) {
            const RenderTargetBlendState *renderTargetBlendState = &desc->blendState->renderTargets[i];
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
    if (desc->renderPass) {
        D3D12_RT_FORMAT_ARRAY renderTargetFormatArray = {};
        renderTargetFormatArray.NumRenderTargets = desc->renderPass->renderTargetCount;

        for (int i = 0; i < renderTargetFormatArray.NumRenderTargets; ++i) {
            DXGI_FORMAT renderTargetFormat;
            ImageFormatToDXGIFormat(desc->renderPass->renderTargetFormats[i], false, &renderTargetFormat);
            renderTargetFormatArray.RTFormats[i] = renderTargetFormat;
        }
        DXGI_FORMAT depthStencilFormat;
        ImageFormatToDXGIFormat(desc->renderPass->depthStencilFormat, false, &depthStencilFormat);

        DXGI_SAMPLE_DESC sampleDesc = {};
        sampleDesc.Count = desc->sampleCount;
        sampleDesc.Quality = desc->sampleQuality;

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

    HRESULT hr = device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipelineState->pso));
    if (FAILED(hr)) {
        BE_WARNLOG("device->CreatePipelineState() failed, ERROR: 0x%x\n", hr);
        return nullptr;
    }

    psoMap.Set(psoHash, pipelineState);

#ifndef _DEBUG
    // PSO cache 가 없었다면 새로 추가한다.
    if (!cachedPsoBlobEntry) {
        ID3DBlob *cachedPSOBlob = nullptr;
        if (SUCCEEDED(pipelineState->pso->GetCachedBlob(&cachedPSOBlob))) {
            cachedPsoBlobMap.Set(combinedShaderHash, cachedPSOBlob);
            WriteCachedPSO(combinedShaderHash, cachedPSOBlob);
        }
    }
#endif

    return pipelineState;
}

bool D3D12Renderer::LoadCachedPSO(const uint64_t hash, ID3DBlob **cachedPSOBlob) {
    Str filename = psoCacheDir;
    filename.AppendPath(va("%016llx", hash));
    filename.SetFileExtension(".pso");

    PlatformFileMapping *fileMapping = PlatformFileMapping::OpenFileRead(filename);
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

    Str filename = psoCacheDir;
    filename.AppendPath(va("%016llx", hash));
    filename.SetFileExtension(".pso");
    PlatformFile *file = (PlatformFile *)PlatformFile::OpenFileWrite(filename);
    if (!file) {
        return;
    }

    file->Write(cachedPSOBlob->GetBufferPointer(), cachedPSOBlob->GetBufferSize());

    delete file;
}

RHIRenderer::PipelineState *D3D12Renderer::CreateBasicPSO(ID3D12RootSignature *rootSignature, const D3D12_SHADER_BYTECODE &byteCodeVS, const D3D12_SHADER_BYTECODE &byteCodePS, const D3D12_INPUT_LAYOUT_DESC &inputLayout) {
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

RHIRenderer::PipelineState *D3D12Renderer::CreateBasicPSO(ID3D12RootSignature *rootSignature, const char *shaderFilename, const D3D12_INPUT_LAYOUT_DESC &inputLayout) {
    char *shaderText;
    int shaderTextSize = fileSystem.LoadFile(shaderFilename, true, (void **)&shaderText);
    if (!shaderText) {
        return nullptr;
    }

    D3D12Shader *vs = static_cast<D3D12Shader *>(CreateShader(ShaderModel::SM_5_0, ShaderStage::Vertex, shaderFilename, shaderText, shaderTextSize, "VSMain"));
    D3D12Shader *ps = static_cast<D3D12Shader *>(CreateShader(ShaderModel::SM_5_0, ShaderStage::Fragment, shaderFilename, shaderText, shaderTextSize, "PSMain"));

    fileSystem.FreeFile(shaderText);

    if (!vs || !ps) {
        SAFE_DELETE(vs);
        SAFE_DELETE(ps);
        return nullptr;
    }

    D3D12_SHADER_BYTECODE byteCodeVS = CD3DX12_SHADER_BYTECODE(vs->compiledShaderData, vs->compiledShaderDataSize);
    D3D12_SHADER_BYTECODE byteCodePS = CD3DX12_SHADER_BYTECODE(ps->compiledShaderData, ps->compiledShaderDataSize);

    PipelineState *pso = CreateBasicPSO(rootSignature, byteCodeVS, byteCodePS, inputLayout);

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

void D3D12Renderer::DestroyPSO(PipelineState *pipelineState, bool immediate) {
    psoMap.Remove(pipelineState->hash);

    if (immediate) {
        delete pipelineState;
    } else {
        MarkForDelete(pipelineState);
    }
}
