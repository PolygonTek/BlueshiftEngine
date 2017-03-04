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
    enum {
        Xy,
        XySt,
        XyStr,
        XyzStr,
        Xyz,
        XyzColor,
        PicXyzStColor,
        GenericXyz,
        GenericXyzSkinning1,
        GenericXyzSkinning4,
        GenericXyzSkinning8,
        GenericXyzSt,
        GenericXyzStSkinning1,
        GenericXyzStSkinning4,
        GenericXyzStSkinning8,
        GenericXyzStColor,
        GenericXyzColorStSkinning1,
        GenericXyzColorStSkinning4,
        GenericXyzColorStSkinning8,
        GenericXyzNormal,
        GenericXyzNormalSkinning1,
        GenericXyzNormalSkinning4,
        GenericXyzNormalSkinning8,
        GenericLit,
        GenericLitSkinning1,
        GenericLitSkinning4,
        GenericLitSkinning8,
        GenericLitColor,
        GenericLitColorSkinning1,
        GenericLitColorSkinning4,
        GenericLitColorSkinning8,
        Occludee,
        MaxVertexFormats
    };

    VertexFormat();

    void                    Append(int stream, int offset, Renderer::VertexElement::Usage usage, int components, Renderer::VertexElement::Type type, bool normalize);
    void                    CopyFrom(const VertexFormat &other);
    void                    Create();
    void                    Delete();

    static void             Init();
    static void             Free();
    static void             CreateSkinningVertexFormats(int originalIndex, int skinning1Index, int skinning4Index, int skinning8Index);

    Array<Renderer::VertexElement> elements;
    Renderer::Handle        vertexFormatHandle;
};

extern VertexFormat         vertexFormats[VertexFormat::MaxVertexFormats];

BE_NAMESPACE_END
