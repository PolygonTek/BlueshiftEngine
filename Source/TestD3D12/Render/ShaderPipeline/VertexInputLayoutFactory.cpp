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

static constexpr int NumVertexInputLayouts = static_cast<int>(VertexInputLayoutId::Count);

static RHI::InputLayout inputLayouts[NumVertexInputLayouts];
static bool inputLayoutsInitialized = false;

static_assert(static_cast<int>(VertexInputLayoutId::XyzSkinning1) == static_cast<int>(VertexInputLayoutId::Xyz) + 1);
static_assert(static_cast<int>(VertexInputLayoutId::XyzSkinning4) == static_cast<int>(VertexInputLayoutId::Xyz) + 2);
static_assert(static_cast<int>(VertexInputLayoutId::XyzSkinning8) == static_cast<int>(VertexInputLayoutId::Xyz) + 3);
static_assert(static_cast<int>(VertexInputLayoutId::XyzStSkinning1) == static_cast<int>(VertexInputLayoutId::XyzSt) + 1);
static_assert(static_cast<int>(VertexInputLayoutId::XyzStSkinning4) == static_cast<int>(VertexInputLayoutId::XyzSt) + 2);
static_assert(static_cast<int>(VertexInputLayoutId::XyzStSkinning8) == static_cast<int>(VertexInputLayoutId::XyzSt) + 3);
static_assert(static_cast<int>(VertexInputLayoutId::XyzStColorSkinning1) == static_cast<int>(VertexInputLayoutId::XyzStColor) + 1);
static_assert(static_cast<int>(VertexInputLayoutId::XyzStColorSkinning4) == static_cast<int>(VertexInputLayoutId::XyzStColor) + 2);
static_assert(static_cast<int>(VertexInputLayoutId::XyzStColorSkinning8) == static_cast<int>(VertexInputLayoutId::XyzStColor) + 3);
static_assert(static_cast<int>(VertexInputLayoutId::XyzStColorNormalTangentSkinning1) == static_cast<int>(VertexInputLayoutId::XyzStColorNormalTangent) + 1);
static_assert(static_cast<int>(VertexInputLayoutId::XyzStColorNormalTangentSkinning4) == static_cast<int>(VertexInputLayoutId::XyzStColorNormalTangent) + 2);
static_assert(static_cast<int>(VertexInputLayoutId::XyzStColorNormalTangentSkinning8) == static_cast<int>(VertexInputLayoutId::XyzStColorNormalTangent) + 3);

static RHI::InputLayout CreateXyzInputLayout() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, OFFSET_OF(BE1::VertexGeneric, xyz), 0, RHI::InputLayoutElement::Format::Float3 },
    };
    return inputLayout;
}

static RHI::InputLayout CreateXyzColorInputLayout() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, 0,                 0, RHI::InputLayoutElement::Format::Float3 },
        { "COLOR",    0, sizeof(float) * 3, 0, RHI::InputLayoutElement::Format::UByte4N },
    };
    return inputLayout;
}

static RHI::InputLayout CreateXyzStInputLayout() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, OFFSET_OF(BE1::VertexGeneric, xyz), 0, RHI::InputLayoutElement::Format::Float3 },
        { "TEXCOORD", 0, OFFSET_OF(BE1::VertexGeneric, st), 0, RHI::InputLayoutElement::Format::Half2 },
    };
    return inputLayout;
}

static RHI::InputLayout CreateXyzStColorInputLayout() {
    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, OFFSET_OF(BE1::VertexGeneric, xyz), 0, RHI::InputLayoutElement::Format::Float3 },
        { "TEXCOORD", 0, OFFSET_OF(BE1::VertexGeneric, st), 0, RHI::InputLayoutElement::Format::Half2 },
        { "COLOR", 0, OFFSET_OF(BE1::VertexGeneric, color), 0, RHI::InputLayoutElement::Format::UByte4N },
    };
    return inputLayout;
}

static constexpr RHI::InputLayoutElement::Format GetNormalTangentFormat() {
#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    return RHI::InputLayoutElement::Format::UByte4N;
#else
    return RHI::InputLayoutElement::Format::Half4;
#endif
}

static RHI::InputLayout CreateXyzStColorNormalTangentInputLayout() {
    constexpr RHI::InputLayoutElement::Format normalTangentFormat = GetNormalTangentFormat();

    RHI::InputLayout inputLayout;
    inputLayout.elements = {
        { "POSITION", 0, OFFSET_OF(BE1::VertexGenericLit, xyz), 0, RHI::InputLayoutElement::Format::Float3 },
        { "TEXCOORD", 0, OFFSET_OF(BE1::VertexGenericLit, st), 0, RHI::InputLayoutElement::Format::Half2 },
        { "COLOR", 0, OFFSET_OF(BE1::VertexGenericLit, color), 0, RHI::InputLayoutElement::Format::UByte4N },
        { "NORMAL", 0, OFFSET_OF(BE1::VertexGenericLit, normal), 0, normalTangentFormat },
        { "TEXCOORD", 1, OFFSET_OF(BE1::VertexGenericLit, tangent), 0, normalTangentFormat },
    };
    return inputLayout;
}

static RHI::InputLayoutElement::Format GetJointWeightFormat() {
    return sizeof(BE1::JointWeightType) == sizeof(uint8_t) ? RHI::InputLayoutElement::Format::UByte4N : RHI::InputLayoutElement::Format::Float4;
}

static void AppendSkinningInputLayoutElements(RHI::InputLayout &inputLayout, VertexInputLayoutId id) {
    const RHI::InputLayoutElement::Format jointWeightFormat = GetJointWeightFormat();

    switch (id) {
    case VertexInputLayoutId::XyzSkinning1:
    case VertexInputLayoutId::XyzStSkinning1:
    case VertexInputLayoutId::XyzStColorSkinning1:
    case VertexInputLayoutId::XyzStColorNormalTangentSkinning1:
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDINDICES", 0, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight1, jointIndex)), 1, RHI::InputLayoutElement::Format::UInt1 });
        break;
    case VertexInputLayoutId::XyzSkinning4:
    case VertexInputLayoutId::XyzStSkinning4:
    case VertexInputLayoutId::XyzStColorSkinning4:
    case VertexInputLayoutId::XyzStColorNormalTangentSkinning4:
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDINDICES", 0, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight4, jointIndexes[0])), 1, RHI::InputLayoutElement::Format::UByte4 });
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDWEIGHT", 0, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight4, jointWeights[0])), 1, jointWeightFormat });
        break;
    case VertexInputLayoutId::XyzSkinning8:
    case VertexInputLayoutId::XyzStSkinning8:
    case VertexInputLayoutId::XyzStColorSkinning8:
    case VertexInputLayoutId::XyzStColorNormalTangentSkinning8:
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDINDICES", 0, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight8, jointIndexes[0])), 1, RHI::InputLayoutElement::Format::UByte4 });
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDINDICES", 1, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight8, jointIndexes[4])), 1, RHI::InputLayoutElement::Format::UByte4 });
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDWEIGHT", 0, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight8, jointWeights[0])), 1, jointWeightFormat });
        inputLayout.elements.Append(RHI::InputLayoutElement{ "BLENDWEIGHT", 1, static_cast<uint32_t>(OFFSET_OF(BE1::VertexWeight8, jointWeights[4])), 1, jointWeightFormat });
        break;
    default:
        assert(0);
        break;
    }
}

static RHI::InputLayout CreateSkinnedInputLayout(VertexInputLayoutId id) {
    RHI::InputLayout inputLayout;

    switch (id) {
    case VertexInputLayoutId::XyzSkinning1:
    case VertexInputLayoutId::XyzSkinning4:
    case VertexInputLayoutId::XyzSkinning8:
        inputLayout = CreateXyzInputLayout();
        break;
    case VertexInputLayoutId::XyzStSkinning1:
    case VertexInputLayoutId::XyzStSkinning4:
    case VertexInputLayoutId::XyzStSkinning8:
        inputLayout = CreateXyzStInputLayout();
        break;
    case VertexInputLayoutId::XyzStColorSkinning1:
    case VertexInputLayoutId::XyzStColorSkinning4:
    case VertexInputLayoutId::XyzStColorSkinning8:
        inputLayout = CreateXyzStColorInputLayout();
        break;
    case VertexInputLayoutId::XyzStColorNormalTangentSkinning1:
    case VertexInputLayoutId::XyzStColorNormalTangentSkinning4:
    case VertexInputLayoutId::XyzStColorNormalTangentSkinning8:
        inputLayout = CreateXyzStColorNormalTangentInputLayout();
        break;
    default:
        assert(0);
        break;
    }

    AppendSkinningInputLayoutElements(inputLayout, id);

    return inputLayout;
}

void VertexInputLayoutFactory::Init() {
    assert(!inputLayoutsInitialized);

    inputLayouts[static_cast<int>(VertexInputLayoutId::Xyz)] = CreateXyzInputLayout();
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzSkinning1)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzSkinning1);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzSkinning4)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzSkinning4);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzSkinning8)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzSkinning8);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzColor)] = CreateXyzColorInputLayout();
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzSt)] = CreateXyzStInputLayout();
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStSkinning1)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzStSkinning1);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStSkinning4)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzStSkinning4);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStSkinning8)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzStSkinning8);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStColor)] = CreateXyzStColorInputLayout();
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStColorSkinning1)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzStColorSkinning1);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStColorSkinning4)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzStColorSkinning4);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStColorSkinning8)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzStColorSkinning8);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStColorNormalTangent)] = CreateXyzStColorNormalTangentInputLayout();
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStColorNormalTangentSkinning1)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzStColorNormalTangentSkinning1);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStColorNormalTangentSkinning4)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzStColorNormalTangentSkinning4);
    inputLayouts[static_cast<int>(VertexInputLayoutId::XyzStColorNormalTangentSkinning8)] = CreateSkinnedInputLayout(VertexInputLayoutId::XyzStColorNormalTangentSkinning8);

    inputLayoutsInitialized = true;
}

void VertexInputLayoutFactory::Shutdown() {
    for (int i = 0; i < NumVertexInputLayouts; ++i) {
        inputLayouts[i].elements.Clear();
    }

    inputLayoutsInitialized = false;
}

const RHI::InputLayout &VertexInputLayoutFactory::Get(VertexInputLayoutId id) {
    assert(inputLayoutsInitialized);

    const int index = static_cast<int>(id);
    assert(index >= 0 && index < NumVertexInputLayouts);

    return inputLayouts[index];
}

VertexInputLayoutId VertexInputLayoutFactory::GetIdForVertexWeights(VertexInputLayoutId baseId, int maxVertexWeights) {
    assert(baseId == VertexInputLayoutId::Xyz ||
        baseId == VertexInputLayoutId::XyzSt ||
        baseId == VertexInputLayoutId::XyzStColor ||
        baseId == VertexInputLayoutId::XyzStColorNormalTangent);

    int skinningOffset = 0;

    switch (maxVertexWeights) {
    case 0:
        skinningOffset = 0;
        break;
    case 1:
        skinningOffset = 1;
        break;
    case 4:
        skinningOffset = 2;
        break;
    case 8:
        skinningOffset = 3;
        break;
    default:
        assert(0);
        break;
    }

    return static_cast<VertexInputLayoutId>(static_cast<int>(baseId) + skinningOffset);
}

bool VertexInputLayoutFactory::IsSkinning(VertexInputLayoutId id) {
    switch (id) {
    case VertexInputLayoutId::XyzSkinning1:
    case VertexInputLayoutId::XyzSkinning4:
    case VertexInputLayoutId::XyzSkinning8:
    case VertexInputLayoutId::XyzStSkinning1:
    case VertexInputLayoutId::XyzStSkinning4:
    case VertexInputLayoutId::XyzStSkinning8:
    case VertexInputLayoutId::XyzStColorSkinning1:
    case VertexInputLayoutId::XyzStColorSkinning4:
    case VertexInputLayoutId::XyzStColorSkinning8:
    case VertexInputLayoutId::XyzStColorNormalTangentSkinning1:
    case VertexInputLayoutId::XyzStColorNormalTangentSkinning4:
    case VertexInputLayoutId::XyzStColorNormalTangentSkinning8:
        return true;
    default:
        return false;
    }
}
