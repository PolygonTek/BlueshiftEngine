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

#include "Core/Vertex.h"
#include "BufferCache.h"
#include "SceneEntity.h"

BE_NAMESPACE_BEGIN

class Material;
class Font;

struct GuiMeshSurf {
    const Material *        material;
    uint32_t                color;

    int                     numVerts;
    int                     numIndexes;
    
    BufferCache             vertexCache;
    BufferCache             indexCache;
};

class GuiMesh {
public:
    enum CoordFrame {
        CoordFrame2D,
        CoordFrame3D
    };

    GuiMesh();

    CoordFrame              GetCoordFrame() const { return coordFrame; }
    void                    SetCoordFrame(CoordFrame frame) { coordFrame = frame; }

    int                     NumSurfaces() const { return surfaces.Count(); }
    const GuiMeshSurf *     Surface(int surfaceIndex) const { return &surfaces[surfaceIndex]; }

    void                    Clear();

    void                    SetClipRect(const Rect &clipRect);

    void                    SetColor(const Color4 &rgba);

    void                    DrawPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Material *material);
    
    int                     DrawChar(float x, float y, float sx, float sy, Font *font, wchar_t charCode);

    void                    Draw(Font *font, SceneEntity::TextAnchor anchor, SceneEntity::TextAlignment alignment, float lineSpacing, float textScale, const wchar_t *text);

                            // Call this function when drawing ends
    void                    CacheIndexes();

    AABB                    Compute3DTextAABB(Font *font, SceneEntity::TextAnchor anchor, float lineSpacing, float textScale, const wchar_t *text) const;

private:
    void                    PrepareNextSurf();
    void                    DrawQuad(const VertexGeneric *verts, const Material *material);
    
    Array<GuiMeshSurf>      surfaces;
    GuiMeshSurf *           currentSurf;
    uint32_t                currentColor;

    int                     totalVerts;         ///< Total number of the vertices
    int                     totalIndexes;       ///< Total number of the indices

    CoordFrame              coordFrame;
    Rect                    clipRect;
};

BE_NAMESPACE_END
