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
#include "VertexInputLayoutFactory.h"
#include "Core/Vertex.h"

static RHI::InputLayout CreateGenericInputLayout() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, OFFSET_OF(BE1::VertexGeneric, xyz), 0, RHI::InputLayoutElement::Format::Float3 },
        { "TEXCOORD", 0, OFFSET_OF(BE1::VertexGeneric, st), 0, RHI::InputLayoutElement::Format::Half2 },
        { "COLOR", 0, OFFSET_OF(BE1::VertexGeneric, color), 0, RHI::InputLayoutElement::Format::UByte4N },
    };
    return inputLayout;
}

static RHI::InputLayout CreateGenericLitInputLayout() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, OFFSET_OF(BE1::VertexGenericLit, xyz), 0, RHI::InputLayoutElement::Format::Float3 },
        { "TEXCOORD", 0, OFFSET_OF(BE1::VertexGenericLit, st), 0, RHI::InputLayoutElement::Format::Half2 },
        { "COLOR", 0, OFFSET_OF(BE1::VertexGenericLit, color), 0, RHI::InputLayoutElement::Format::UByte4N },
        { "NORMAL", 0, OFFSET_OF(BE1::VertexGenericLit, normal), 0, RHI::InputLayoutElement::Format::UByte4N },
        { "TEXCOORD", 1, OFFSET_OF(BE1::VertexGenericLit, tangent), 0, RHI::InputLayoutElement::Format::UByte4N },
    };
    return inputLayout;
}

static RHI::InputLayoutElement::Format GetJointWeightFormat() {
    return sizeof(BE1::JointWeightType) == sizeof(uint8_t) ? RHI::InputLayoutElement::Format::UByte4N : RHI::InputLayoutElement::Format::Float4;
}

static RHI::InputLayout CreateGenericLitSkinningInputLayout(VertexSkinningLayout skinningLayout) {
    RHI::InputLayout inputLayout = CreateGenericLitInputLayout();
    const RHI::InputLayoutElement::Format jointWeightFormat = GetJointWeightFormat();

    switch (skinningLayout) {
    case VertexSkinningLayout::Weight1:
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDINDICES", 0, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight1, jointIndex)), 1, RHI::InputLayoutElement::Format::UInt1 });
        break;
    case VertexSkinningLayout::Weight4:
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDINDICES", 0, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight4, jointIndexes[0])), 1, RHI::InputLayoutElement::Format::UByte4 });
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDWEIGHT", 0, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight4, jointWeights[0])), 1, jointWeightFormat });
        break;
    case VertexSkinningLayout::Weight8:
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDINDICES", 0, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight8, jointIndexes[0])), 1, RHI::InputLayoutElement::Format::UByte4 });
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDINDICES", 1, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight8, jointIndexes[4])), 1, RHI::InputLayoutElement::Format::UByte4 });
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDWEIGHT", 0, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight8, jointWeights[0])), 1, jointWeightFormat });
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDWEIGHT", 1, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight8, jointWeights[4])), 1, jointWeightFormat });
        break;
    default:
        assert(0);
        break;
    }

    return inputLayout;
}

const RHI::InputLayout &VertexInputLayoutFactory::GetGeneric() {
    static const RHI::InputLayout inputLayout = CreateGenericInputLayout();
    return inputLayout;
}

const RHI::InputLayout &VertexInputLayoutFactory::GetGenericLit() {
    static const RHI::InputLayout inputLayout = CreateGenericLitInputLayout();
    return inputLayout;
}

const RHI::InputLayout &VertexInputLayoutFactory::GetGenericLit(VertexSkinningLayout skinningLayout) {
    switch (skinningLayout) {
    case VertexSkinningLayout::None:
        return GetGenericLit();
    case VertexSkinningLayout::Weight1: {
        static const RHI::InputLayout inputLayout = CreateGenericLitSkinningInputLayout(VertexSkinningLayout::Weight1);
        return inputLayout;
    }
    case VertexSkinningLayout::Weight4: {
        static const RHI::InputLayout inputLayout = CreateGenericLitSkinningInputLayout(VertexSkinningLayout::Weight4);
        return inputLayout;
    }
    case VertexSkinningLayout::Weight8: {
        static const RHI::InputLayout inputLayout = CreateGenericLitSkinningInputLayout(VertexSkinningLayout::Weight8);
        return inputLayout;
    }
    default:
        assert(0);
        return GetGenericLit();
    }
}
