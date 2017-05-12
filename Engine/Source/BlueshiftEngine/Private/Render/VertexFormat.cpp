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
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

VertexFormat vertexFormats[VertexFormat::MaxVertexFormats];

void VertexFormat::Init() {
    // Xy
    vertexFormats[VertexFormat::Type::Xy].Append(0, 0, Renderer::VertexElement::Position, 2, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::Xy].Create();

    // XySt
    vertexFormats[VertexFormat::Type::XySt].Append(0, 0, Renderer::VertexElement::Position, 2, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::XySt].Append(0, 8, Renderer::VertexElement::TexCoord0, 2, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::XySt].Create();

    // XyStr
    vertexFormats[VertexFormat::Type::XyStr].Append(0, 0, Renderer::VertexElement::Position, 2, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::XyStr].Append(0, 8, Renderer::VertexElement::TexCoord0, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::XyStr].Create();

    // XyzStr
    vertexFormats[VertexFormat::Type::XyzStr].Append(0, 0, Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::XyzStr].Append(0, 12, Renderer::VertexElement::TexCoord0, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::XyzStr].Create();

    // Xyz
    vertexFormats[VertexFormat::Type::Xyz].Append(0, 0, Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::Xyz].Create();

    // XyzColor
    vertexFormats[VertexFormat::Type::XyzColor].Append(0, 0, Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::XyzColor].Append(0, 12, Renderer::VertexElement::Color, 4, Renderer::VertexElement::UByteType, true);
    vertexFormats[VertexFormat::Type::XyzColor].Create();

#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    Renderer::VertexElement::Type normalTangentType = Renderer::VertexElement::UByteType;
    bool normalTangentNormalize = true;
#else
    Renderer::VertexElement::Type normalTangentType = Renderer::VertexElement::HalfType;
    bool normalTangentNormalize = false;
#endif

    // GenericXyz
    vertexFormats[VertexFormat::Type::GenericXyz].Append(0, OFFSET_OF(VertexGenericLit, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::GenericXyz].Create();

    // GenericXyzSkinning1, GenericXyzSkinning4, GenericXyzSkinning8
    CreateSkinningVertexFormats(VertexFormat::Type::GenericXyz, 
        VertexFormat::Type::GenericXyzSkinning1, 
        VertexFormat::Type::GenericXyzSkinning4, 
        VertexFormat::Type::GenericXyzSkinning8);

    // GenericXyzSt
    vertexFormats[VertexFormat::Type::GenericXyzSt].Append(0, OFFSET_OF(VertexGenericLit, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::GenericXyzSt].Append(0, OFFSET_OF(VertexGenericLit, st), Renderer::VertexElement::TexCoord0, 2, Renderer::VertexElement::HalfType, false);
    vertexFormats[VertexFormat::Type::GenericXyzSt].Create();

    // GenericXyzStSkinning1, GenericXyzStSkinning4, GenericXyzStSkinning8
    CreateSkinningVertexFormats(VertexFormat::Type::GenericXyzSt, 
        VertexFormat::Type::GenericXyzStSkinning1, 
        VertexFormat::Type::GenericXyzStSkinning4, 
        VertexFormat::Type::GenericXyzStSkinning8);

    // GenericXyzStColor
    vertexFormats[VertexFormat::Type::GenericXyzStColor].Append(0, OFFSET_OF(VertexGenericLit, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::GenericXyzStColor].Append(0, OFFSET_OF(VertexGenericLit, st), Renderer::VertexElement::TexCoord0, 2, Renderer::VertexElement::HalfType, false);
    vertexFormats[VertexFormat::Type::GenericXyzStColor].Append(0, OFFSET_OF(VertexGenericLit, color), Renderer::VertexElement::Color, 4, Renderer::VertexElement::UByteType, true);
    vertexFormats[VertexFormat::Type::GenericXyzStColor].Create();

    // GenericXyzStColorSkinning1, GenericXyzStColorSkinning4, GenericXyzStColorSkinning8
    CreateSkinningVertexFormats(VertexFormat::Type::GenericXyzStColor, 
        VertexFormat::Type::GenericXyzStColorSkinning1, 
        VertexFormat::Type::GenericXyzStColorSkinning4, 
        VertexFormat::Type::GenericXyzStColorSkinning8);

    // GenericXyzNormal
    vertexFormats[VertexFormat::Type::GenericXyzNormal].Append(0, OFFSET_OF(VertexGenericLit, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::GenericXyzNormal].Append(0, OFFSET_OF(VertexGenericLit, normal), Renderer::VertexElement::Normal, 3, normalTangentType, normalTangentNormalize);
    vertexFormats[VertexFormat::Type::GenericXyzNormal].Create();

    // GenericXyzNormalSkinning1, GenericXyzNormalSkinning4, GenericXyzNormalSkinning8
    CreateSkinningVertexFormats(VertexFormat::Type::GenericXyzNormal, 
        VertexFormat::Type::GenericXyzNormalSkinning1, 
        VertexFormat::Type::GenericXyzNormalSkinning4, 
        VertexFormat::Type::GenericXyzNormalSkinning8);

    // GenericXyzStNT
    vertexFormats[VertexFormat::Type::GenericXyzStNT].Append(0, OFFSET_OF(VertexGenericLit, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::GenericXyzStNT].Append(0, OFFSET_OF(VertexGenericLit, st), Renderer::VertexElement::TexCoord, 2, Renderer::VertexElement::HalfType, false);
    vertexFormats[VertexFormat::Type::GenericXyzStNT].Append(0, OFFSET_OF(VertexGenericLit, normal), Renderer::VertexElement::Normal, 3, normalTangentType, normalTangentNormalize);
    vertexFormats[VertexFormat::Type::GenericXyzStNT].Append(0, OFFSET_OF(VertexGenericLit, tangent), Renderer::VertexElement::TexCoord1, 4, normalTangentType, normalTangentNormalize);
    vertexFormats[VertexFormat::Type::GenericXyzStNT].Create();

    // GenericXyzStNTSkinning1, GenericXyzStNTSkinning4, GenericXyzStNTSkinning8
    CreateSkinningVertexFormats(VertexFormat::Type::GenericXyzStNT,
        VertexFormat::Type::GenericXyzStNTSkinning1, 
        VertexFormat::Type::GenericXyzStNTSkinning4, 
        VertexFormat::Type::GenericXyzStNTSkinning8);

    // GenericXyzStColorNT
    vertexFormats[VertexFormat::Type::GenericXyzStColorNT].Append(0, OFFSET_OF(VertexGenericLit, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::GenericXyzStColorNT].Append(0, OFFSET_OF(VertexGenericLit, st), Renderer::VertexElement::TexCoord0, 2, Renderer::VertexElement::HalfType, false);
    vertexFormats[VertexFormat::Type::GenericXyzStColorNT].Append(0, OFFSET_OF(VertexGenericLit, color), Renderer::VertexElement::Color, 4, Renderer::VertexElement::UByteType, true);
    vertexFormats[VertexFormat::Type::GenericXyzStColorNT].Append(0, OFFSET_OF(VertexGenericLit, normal), Renderer::VertexElement::Normal, 3, normalTangentType, normalTangentNormalize);
    vertexFormats[VertexFormat::Type::GenericXyzStColorNT].Append(0, OFFSET_OF(VertexGenericLit, tangent), Renderer::VertexElement::TexCoord1, 4, normalTangentType, normalTangentNormalize);
    vertexFormats[VertexFormat::Type::GenericXyzStColorNT].Create();

    // GenericXyzStColorNTSkinning1, GenericXyzStColorNTSkinning4, GenericXyzStColorNTSkinning8
    CreateSkinningVertexFormats(VertexFormat::Type::GenericXyzStColorNT,
        VertexFormat::Type::GenericXyzStColorNTSkinning1, 
        VertexFormat::Type::GenericXyzStColorNTSkinning4, 
        VertexFormat::Type::GenericXyzStColorNTSkinning8);

    // Occludee
    vertexFormats[VertexFormat::Type::Occludee].Append(0, 0, Renderer::VertexElement::Position, 2, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::Occludee].Append(0, 8, Renderer::VertexElement::TexCoord0, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::Occludee].Append(0, 20, Renderer::VertexElement::TexCoord1, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[VertexFormat::Type::Occludee].Create();
}

void VertexFormat::CreateSkinningVertexFormats(int originalIndex, int skinning1Index, int skinning4Index, int skinning8Index) {
    Renderer::VertexElement::Type weightType;

    if (sizeof(JointWeightType) == sizeof(byte)) {
        weightType = Renderer::VertexElement::UByteType;
    } else {
        weightType = Renderer::VertexElement::FloatType;
    }

    VertexFormat *vf = &vertexFormats[skinning1Index];
    vf->CopyFrom(vertexFormats[originalIndex]);
    vf->Append(1, OFFSET_OF(VertexWeight1, jointIndex), Renderer::VertexElement::WeightIndex, 1, Renderer::VertexElement::UIntType, false);
    vf->Create();

    vf = &vertexFormats[skinning4Index];
    vf->CopyFrom(vertexFormats[originalIndex]);
    vf->Append(1, OFFSET_OF(VertexWeight4, jointIndexes[0]), Renderer::VertexElement::WeightIndex, 4, Renderer::VertexElement::UByteType, false);
    vf->Append(1, OFFSET_OF(VertexWeight4, jointWeights[0]), Renderer::VertexElement::WeightValue, 4, weightType, true);
    vf->Create();

    vf = &vertexFormats[skinning8Index];
    vf->CopyFrom(vertexFormats[originalIndex]);
    vf->Append(1, OFFSET_OF(VertexWeight8, jointIndexes[0]), Renderer::VertexElement::WeightIndex0, 4, Renderer::VertexElement::UByteType, false);
    vf->Append(1, OFFSET_OF(VertexWeight8, jointIndexes[4]), Renderer::VertexElement::WeightIndex1, 4, Renderer::VertexElement::UByteType, false);
    vf->Append(1, OFFSET_OF(VertexWeight8, jointWeights[0]), Renderer::VertexElement::WeightValue0, 4, weightType, true);
    vf->Append(1, OFFSET_OF(VertexWeight8, jointWeights[4]), Renderer::VertexElement::WeightValue1, 4, weightType, true);
    vf->Create();
}

void VertexFormat::Free() {
    for (int i = 0; i < COUNT_OF(vertexFormats); i++) {
        vertexFormats[i].Delete();
    }
}

VertexFormat::VertexFormat() {
    vertexFormatHandle = Renderer::NullVertexFormat;
}

void VertexFormat::Append(int stream, int offset, Renderer::VertexElement::Usage usage, int components, Renderer::VertexElement::Type type, bool normalize) {
    Renderer::VertexElement element;
    element.stream      = stream;
    element.offset      = offset;
    element.usage       = usage;
    element.components  = components;
    element.type        = type;
    element.normalize   = normalize;

    elements.Append(element);

    int typeSize;
    switch (type) {
    case Renderer::VertexElement::UByteType:
        typeSize = 1; 
        break;
    case Renderer::VertexElement::HalfType:
        typeSize = 2;
        break;
    case Renderer::VertexElement::UIntType:
    case Renderer::VertexElement::FloatType:
        typeSize = 4; 
        break;
    default: 
        assert(0); 
        break;
    }
}

void VertexFormat::CopyFrom(const VertexFormat &other) {
    elements = other.elements;
}

void VertexFormat::Create() {
    vertexFormatHandle = glr.CreateVertexFormat(elements.Count(), elements.Ptr());
}

void VertexFormat::Delete() {
    glr.DeleteVertexFormat(vertexFormatHandle);
}

BE_NAMESPACE_END
