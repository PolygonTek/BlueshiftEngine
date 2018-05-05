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

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Vertex Format

-------------------------------------------------------------------------------
*/

class VertexFormat {
public:
    enum Type {
        // --- For Debug ---
        Xy,
        XySt,
        XyStr,
        XyzStr,
        Xyz,
        XyzColor,
        // --- VertexGeneric ---
        GenericXyz,
        GenericXyzSkinning1,
        GenericXyzSkinning4,
        GenericXyzSkinning8,
        GenericXyzInstancing,
        GenericXyzInstancingSkinning1,
        GenericXyzInstancingSkinning4,
        GenericXyzInstancingSkinning8,
        GenericXyzSt,
        GenericXyzStSkinning1,
        GenericXyzStSkinning4,
        GenericXyzStSkinning8,
        GenericXyzStInstancing,
        GenericXyzStInstancingSkinning1,
        GenericXyzStInstancingSkinning4,
        GenericXyzStInstancingSkinning8,
        GenericXyzStColor,
        GenericXyzStColorSkinning1,
        GenericXyzStColorSkinning4,
        GenericXyzStColorSkinning8,
        GenericXyzStColorInstancing,
        GenericXyzStColorInstancingSkinning1,
        GenericXyzStColorInstancingSkinning4,
        GenericXyzStColorInstancingSkinning8,
        // --- VertexGenericLit ---
        GenericXyzNormal,
        GenericXyzNormalSkinning1,
        GenericXyzNormalSkinning4,
        GenericXyzNormalSkinning8,
        GenericXyzNormalInstancing,
        GenericXyzNormalInstancingSkinning1,
        GenericXyzNormalInstancingSkinning4,
        GenericXyzNormalInstancingSkinning8,
        GenericXyzStNT,
        GenericXyzStNTSkinning1,
        GenericXyzStNTSkinning4,
        GenericXyzStNTSkinning8,
        GenericXyzStNTInstancing,
        GenericXyzStNTInstancingSkinning1,
        GenericXyzStNTInstancingSkinning4,
        GenericXyzStNTInstancingSkinning8,
        GenericXyzStColorNT,
        GenericXyzStColorNTSkinning1,
        GenericXyzStColorNTSkinning4,
        GenericXyzStColorNTSkinning8,
        GenericXyzStColorNTInstancing,
        GenericXyzStColorNTInstancingSkinning1,
        GenericXyzStColorNTInstancingSkinning4,
        GenericXyzStColorNTInstancingSkinning8,
        // -----------------------------
        Occludee,
        MaxVertexFormats
    };

    VertexFormat();

    static void             Init();
    static void             Shutdown();

    Array<RHI::VertexElement> elements;
    RHI::Handle             vertexFormatHandle;

private:
    static void             CreateSkinningVertexFormats(int stream, int originalIndex, int skinning1Index, int skinning4Index, int skinning8Index);
    static void             CreateInstancingVertexFormats(int stream, int originalIndex, int instancingIndex, bool useVtfSkinning);

    void                    Append(int stream, int offset, RHI::VertexElement::Usage usage, int components, RHI::VertexElement::Type type, bool normalize, int divisor);
    void                    CopyFrom(const VertexFormat &other);

    void                    Create();
    void                    Delete();
};

BE_INLINE VertexFormat::VertexFormat() {
    vertexFormatHandle = RHI::NullVertexFormat;
}

extern VertexFormat         vertexFormats[VertexFormat::MaxVertexFormats];

BE_NAMESPACE_END
