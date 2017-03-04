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
    vertexFormats[Xy].Append(0, 0, Renderer::VertexElement::Position, 2, Renderer::VertexElement::FloatType, false);
    vertexFormats[Xy].Create();

    // XySt
    vertexFormats[XySt].Append(0, 0, Renderer::VertexElement::Position, 2, Renderer::VertexElement::FloatType, false);
    vertexFormats[XySt].Append(0, 8, Renderer::VertexElement::TexCoord0, 2, Renderer::VertexElement::FloatType, false);
    vertexFormats[XySt].Create();

    // XyStr
    vertexFormats[XyStr].Append(0, 0, Renderer::VertexElement::Position, 2, Renderer::VertexElement::FloatType, false);
    vertexFormats[XyStr].Append(0, 8, Renderer::VertexElement::TexCoord0, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[XyStr].Create();

    // XyzStr
    vertexFormats[XyzStr].Append(0, 0, Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[XyzStr].Append(0, 12, Renderer::VertexElement::TexCoord0, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[XyzStr].Create();

    // Xyz
    vertexFormats[Xyz].Append(0, 0, Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[Xyz].Create();

    // XyzColor
    vertexFormats[XyzColor].Append(0, 0, Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[XyzColor].Append(0, 12, Renderer::VertexElement::Color, 4, Renderer::VertexElement::UByteType, true);
    vertexFormats[XyzColor].Create();

    // PicXyzStColor
    vertexFormats[PicXyzStColor].Append(0, OFFSET_OF(VertexNoLit, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[PicXyzStColor].Append(0, OFFSET_OF(VertexNoLit, st), Renderer::VertexElement::TexCoord0, 2, Renderer::VertexElement::HalfType, false);
    vertexFormats[PicXyzStColor].Append(0, OFFSET_OF(VertexNoLit, color), Renderer::VertexElement::Color, 4, Renderer::VertexElement::UByteType, true);
    vertexFormats[PicXyzStColor].Create();

#ifdef COMPRESSED_VERTEX_NORMAL_TANGENTS
    Renderer::VertexElement::Type normalTangentType = Renderer::VertexElement::UByteType;
    bool normalTangentNormalize = true;
#else
    Renderer::VertexElement::Type normalTangentType = Renderer::VertexElement::HalfType;
    bool normalTangentNormalize = false;
#endif

    // GenericXyz
    vertexFormats[GenericXyz].Append(0, OFFSET_OF(VertexLightingGeneric, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[GenericXyz].Create();

    CreateSkinningVertexFormats(GenericXyz, GenericXyzSkinning1, GenericXyzSkinning4, GenericXyzSkinning8);

    // GenericXyzSt
    vertexFormats[GenericXyzSt].Append(0, OFFSET_OF(VertexLightingGeneric, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[GenericXyzSt].Append(0, OFFSET_OF(VertexLightingGeneric, st), Renderer::VertexElement::TexCoord0, 2, Renderer::VertexElement::HalfType, false);
    vertexFormats[GenericXyzSt].Create();

    CreateSkinningVertexFormats(GenericXyzSt, GenericXyzStSkinning1, GenericXyzStSkinning4, GenericXyzStSkinning8);

    // GenericXyzStColor
    vertexFormats[GenericXyzStColor].Append(0, OFFSET_OF(VertexLightingGeneric, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[GenericXyzStColor].Append(0, OFFSET_OF(VertexLightingGeneric, st), Renderer::VertexElement::TexCoord0, 2, Renderer::VertexElement::HalfType, false);
    vertexFormats[GenericXyzStColor].Append(0, OFFSET_OF(VertexLightingGeneric, color), Renderer::VertexElement::Color, 4, Renderer::VertexElement::UByteType, true);
    vertexFormats[GenericXyzStColor].Create();

    CreateSkinningVertexFormats(GenericXyzStColor, GenericXyzColorStSkinning1, GenericXyzColorStSkinning4, GenericXyzColorStSkinning8);

    // GenericXyzNormal
    vertexFormats[GenericXyzNormal].Append(0, OFFSET_OF(VertexLightingGeneric, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[GenericXyzNormal].Append(0, OFFSET_OF(VertexLightingGeneric, normal), Renderer::VertexElement::Normal, 3, normalTangentType, normalTangentNormalize);
    vertexFormats[GenericXyzNormal].Create();

    CreateSkinningVertexFormats(GenericXyzNormal, GenericXyzNormalSkinning1, GenericXyzNormalSkinning4, GenericXyzNormalSkinning8);

    // GenericLit
    vertexFormats[GenericLit].Append(0, OFFSET_OF(VertexLightingGeneric, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[GenericLit].Append(0, OFFSET_OF(VertexLightingGeneric, st), Renderer::VertexElement::TexCoord, 2, Renderer::VertexElement::HalfType, false);
    vertexFormats[GenericLit].Append(0, OFFSET_OF(VertexLightingGeneric, normal), Renderer::VertexElement::Normal, 3, normalTangentType, normalTangentNormalize);
    vertexFormats[GenericLit].Append(0, OFFSET_OF(VertexLightingGeneric, tangent), Renderer::VertexElement::TexCoord1, 4, normalTangentType, normalTangentNormalize);
    vertexFormats[GenericLit].Create();

    CreateSkinningVertexFormats(GenericLit, GenericLitSkinning1, GenericLitSkinning4, GenericLitSkinning8);

    // GenericLitColor
    vertexFormats[GenericLitColor].Append(0, OFFSET_OF(VertexLightingGeneric, xyz), Renderer::VertexElement::Position, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[GenericLitColor].Append(0, OFFSET_OF(VertexLightingGeneric, st), Renderer::VertexElement::TexCoord0, 2, Renderer::VertexElement::HalfType, false);
    vertexFormats[GenericLitColor].Append(0, OFFSET_OF(VertexLightingGeneric, color), Renderer::VertexElement::Color, 4, Renderer::VertexElement::UByteType, true);
    vertexFormats[GenericLitColor].Append(0, OFFSET_OF(VertexLightingGeneric, normal), Renderer::VertexElement::Normal, 3, normalTangentType, normalTangentNormalize);
    vertexFormats[GenericLitColor].Append(0, OFFSET_OF(VertexLightingGeneric, tangent), Renderer::VertexElement::TexCoord1, 4, normalTangentType, normalTangentNormalize);
    vertexFormats[GenericLitColor].Create();

    CreateSkinningVertexFormats(GenericLitColor, GenericLitColorSkinning1, GenericLitColorSkinning4, GenericLitColorSkinning8);	

    // Occludee
    vertexFormats[Occludee].Append(0, 0, Renderer::VertexElement::Position, 2, Renderer::VertexElement::FloatType, false);
    vertexFormats[Occludee].Append(0, 8, Renderer::VertexElement::TexCoord0, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[Occludee].Append(0, 20, Renderer::VertexElement::TexCoord1, 3, Renderer::VertexElement::FloatType, false);
    vertexFormats[Occludee].Create();
}

void VertexFormat::CreateSkinningVertexFormats(int originalIndex, int skinning1Index, int skinning4Index, int skinning8Index) {	
    Renderer::VertexElement::Type weightType;

    if (sizeof(VertexWeightValue) == sizeof(byte)) {
        weightType = Renderer::VertexElement::UByteType;
    } else {
        weightType = Renderer::VertexElement::FloatType;
    }

    VertexFormat *vf = &vertexFormats[skinning1Index];
    vf->CopyFrom(vertexFormats[originalIndex]);
    vf->Append(1, OFFSET_OF(VertexWeight1, index), Renderer::VertexElement::WeightIndex, 1, Renderer::VertexElement::UIntType, false);
    vf->Create();

    vf = &vertexFormats[skinning4Index];
    vf->CopyFrom(vertexFormats[originalIndex]);
    vf->Append(1, OFFSET_OF(VertexWeight4, index[0]), Renderer::VertexElement::WeightIndex, 4, Renderer::VertexElement::UByteType, false);
    vf->Append(1, OFFSET_OF(VertexWeight4, weight[0]), Renderer::VertexElement::WeightValue, 4, weightType, true);
    vf->Create();

    vf = &vertexFormats[skinning8Index];
    vf->CopyFrom(vertexFormats[originalIndex]);
    vf->Append(1, OFFSET_OF(VertexWeight8, index[0]), Renderer::VertexElement::WeightIndex0, 4, Renderer::VertexElement::UByteType, false);
    vf->Append(1, OFFSET_OF(VertexWeight8, index[4]), Renderer::VertexElement::WeightIndex1, 4, Renderer::VertexElement::UByteType, false);
    vf->Append(1, OFFSET_OF(VertexWeight8, weight[0]), Renderer::VertexElement::WeightValue0, 4, weightType, true);
    vf->Append(1, OFFSET_OF(VertexWeight8, weight[4]), Renderer::VertexElement::WeightValue1, 4, weightType, true);
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
    Renderer::VertexElement ve;
    ve.offset		= offset;
    ve.stream		= stream;
    ve.usage		= usage;
    ve.components	= components;
    ve.type			= type;
    ve.normalize	= normalize;

    elements.Append(ve);

    int typeSize;
    switch (type) {
    case Renderer::VertexElement::UByteType: typeSize = 1; break;
    case Renderer::VertexElement::UIntType: typeSize = 4; break;
    case Renderer::VertexElement::FloatType: typeSize = 4; break;
    case Renderer::VertexElement::HalfType: typeSize = 2; break;
    default: assert(0); break;
    }
}

void VertexFormat::CopyFrom(const VertexFormat &other) {
    elements	= other.elements;
}

void VertexFormat::Create() {
    vertexFormatHandle = glr.CreateVertexFormat(elements.Count(), elements.Ptr());
}

void VertexFormat::Delete() {
    glr.DeleteVertexFormat(vertexFormatHandle);
}

BE_NAMESPACE_END
