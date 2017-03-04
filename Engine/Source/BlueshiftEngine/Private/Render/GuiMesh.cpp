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
#include "Render/Font.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

GuiMesh::GuiMesh() {
    numVerts = 0;
    numIndexes = 0;

    coordFrame = CoordFrame2D;

    clipRect.Set(0, 0, 0, 0);
}

void GuiMesh::Clear() {
    numVerts = 0;
    numIndexes = 0;

    surfaces.SetCount(0, false);

    PrepareNextSurf();
}

void GuiMesh::PrepareNextSurf() {
    GuiMeshSurf newSurf;

    newSurf.numVerts = 0;
    newSurf.numIndexes = 0;

    memset(&newSurf.vertexCache, 0, sizeof(newSurf.vertexCache));
    memset(&newSurf.indexCache, 0, sizeof(newSurf.indexCache));

    if (surfaces.Count() > 0) {
        newSurf.color = currentSurf->color;
        newSurf.material = currentSurf->material;
    } else {
        newSurf.color = 0xFFFFFFFF;
        newSurf.material = materialManager.defaultMaterial;
    }

    surfaces.Append(newSurf);
    currentSurf = &surfaces[surfaces.Count() - 1];
}

void GuiMesh::SetColor(const Color4 &rgba) {
    currentSurf->color = rgba.ToUInt32();
}

void GuiMesh::SetClipRect(const Rect &clipRect) {
    this->clipRect = clipRect;
}

void GuiMesh::DrawTris(const VertexNoLit *verts, const TriIndex *indexes, int vertCount, int indexCount, const Material *material) {
    if (!verts || !indexes || !vertCount || !indexCount || !material) {
        return;
    }

    if (material != currentSurf->material) {
        if (currentSurf->numVerts > 0) {
            PrepareNextSurf();
        }
        currentSurf->material = material;
    }

    numVerts += vertCount;
    numIndexes += indexCount;

    currentSurf->numVerts += vertCount;
    currentSurf->numIndexes += indexCount;

    // Fills in dynamic vertex buffer
    BufferCache vertexCache;
    bufferCacheManager.AllocVertex(vertCount, sizeof(VertexNoLit), verts, &vertexCache);

    int filledVertexCount = vertexCache.offset / sizeof(VertexNoLit);

    // Fills in dynamic index buffer
    BufferCache indexCache;
    bufferCacheManager.AllocIndex(indexCount, sizeof(TriIndex), nullptr, &indexCache);

    TriIndex *indexPointer = (TriIndex *)bufferCacheManager.MapIndexBuffer(&indexCache);
    for (int i = 0; i < indexCount; i++) {
        indexPointer[i] = filledVertexCount + indexes[i];
    }
    bufferCacheManager.UnmapIndexBuffer(&indexCache);

    if (!bufferCacheManager.IsCached(&currentSurf->vertexCache)) {
        currentSurf->vertexCache = vertexCache;
        currentSurf->indexCache = indexCache;
    } else {
        currentSurf->vertexCache.bytes += vertexCache.bytes;
        currentSurf->indexCache.bytes += indexCache.bytes;
    }
}

void GuiMesh::DrawPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Material *material) {
    if (coordFrame == CoordFrame2D && !clipRect.IsEmpty()) {
        const float cx = clipRect.x - x;
        if (cx > 0) {
            s1 += (s2 - s1) * cx / w;
            w -= cx;
            x = clipRect.x;
        }

        const float cy = clipRect.y - y;
        if (cy > 0) {
            t1 += (t2 - t1) * cy / h;
            h -= cy;
            y = clipRect.y;
        }

        const float cx2 = x + w - clipRect.X2();
        if (cx2 > 0) {
            s2 -= (s2 - s1) * cx2 / w;
            w -= cx2;
        }

        const float cy2 = y + h - clipRect.Y2();
        if (cy2 > 0) {
            t2 -= (t2 - t1) * cy2 / h;
            h -= cy2;
        }
    }

    if (w <= 0 || h <= 0) {
        return; // completely clipped away
    }    

    ALIGN16(VertexNoLit) localVerts[4];

    if (coordFrame == CoordFrame2D) {
        // 2D frame
        //  +-----> +X
        //  |   
        //  |
        // +Y
        localVerts[0].xyz[0] = x;
        localVerts[0].xyz[1] = y;
        localVerts[0].xyz[2] = 0;

        localVerts[1].xyz[0] = x;
        localVerts[1].xyz[1] = y + h;
        localVerts[1].xyz[2] = 0;        

        localVerts[2].xyz[0] = x + w;
        localVerts[2].xyz[1] = y + h;
        localVerts[2].xyz[2] = 0;
        
        localVerts[3].xyz[0] = x + w;
        localVerts[3].xyz[1] = y;
        localVerts[3].xyz[2] = 0;
        
    } else {
        // 3D frame
        //  +-----> +Y
        //  |
        //  |
        // -Z
        localVerts[0].xyz[0] = 0;
        localVerts[0].xyz[1] = x;
        localVerts[0].xyz[2] = -y;

        localVerts[1].xyz[0] = 0;
        localVerts[1].xyz[1] = x;
        localVerts[1].xyz[2] = -(y + h);

        localVerts[2].xyz[0] = 0;
        localVerts[2].xyz[1] = x + w;
        localVerts[2].xyz[2] = -(y + h);

        localVerts[3].xyz[0] = 0;
        localVerts[3].xyz[1] = x + w;
        localVerts[3].xyz[2] = -y;
    }

    const float16_t hs1 = F32toF16(s1);
    const float16_t ht1 = F32toF16(t1);
    const float16_t hs2 = F32toF16(s2);
    const float16_t ht2 = F32toF16(t2);

    localVerts[0].st[0] = hs1;
    localVerts[0].st[1] = ht1;
    *reinterpret_cast<uint32_t *>(localVerts[0].color) = currentSurf->color;

    localVerts[1].st[0] = hs1;
    localVerts[1].st[1] = ht2;
    *reinterpret_cast<uint32_t *>(localVerts[1].color) = currentSurf->color;

    localVerts[2].st[0] = hs2;
    localVerts[2].st[1] = ht2;
    *reinterpret_cast<uint32_t *>(localVerts[2].color) = currentSurf->color;

    localVerts[3].st[0] = hs2;
    localVerts[3].st[1] = ht1;
    *reinterpret_cast<uint32_t *>(localVerts[3].color) = currentSurf->color;

    static const TriIndex quadTrisIndexes[6] = { 0, 1, 2, 0, 2, 3 };

    DrawTris(localVerts, quadTrisIndexes, COUNT_OF(localVerts), COUNT_OF(quadTrisIndexes), material);
}

int GuiMesh::DrawChar(float x, float y, float sx, float sy, Font *font, wchar_t charCode) {
    if (charCode == L' ') {
        return font->GetGlyphAdvance(charCode) * sx;
    }

    // Clip away before accessing to the font glyph
    if (!clipRect.IsEmpty()) {
        if (x >= clipRect.X2() || y >= clipRect.Y2()) {
            return 0;
        }
    }
    
    FontGlyph *glyph = font->GetGlyph(charCode);
    if (!glyph) {
        return 0;
    }

    float pitch = glyph->advance * sx;

    float _x = x + glyph->bearingX * sx;
    float _y = y + glyph->bearingY * sy;
    float _w = glyph->width * sx;
    float _h = glyph->height * sy;

    DrawPic(_x, _y, _w, _h, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->material);

    return pitch;
}

void GuiMesh::DrawText(Font *font, SceneEntity::TextAnchor anchor, SceneEntity::TextAlignment alignment, float lineSpacing, float textScale, const wchar_t *text) {
    static const int MaxTextLines = 256;
    const wchar_t *textLines[MaxTextLines];
    int lineLen[MaxTextLines];
    int numLines = 0;
    int lineWidth = 0;
    int maxWidth = 0;

    textLines[0] = (const wchar_t *)text;
    
    const wchar_t *ptr = (const wchar_t *)text;

    while (*ptr) {
        if (*ptr == L'\n') {
            lineLen[numLines] = (int)(ptr - textLines[numLines]);
            numLines++;

            textLines[numLines] = ptr + 1; 
            
            if (lineWidth > maxWidth) {
                maxWidth = lineWidth;
            }
            lineWidth = 0;
        } else {
            int charWidth = font->GetGlyphAdvance(*ptr) * textScale;
            lineWidth += charWidth;
        }

        ptr++;
    }

    if (ptr - textLines[numLines] > 0) {
        lineLen[numLines] = (int)(ptr - textLines[numLines]);
        numLines++;
    }

    // Calculate the coordinate y
    int y = 0;
    if (anchor == SceneEntity::TextAnchor::LowerLeft || anchor == SceneEntity::TextAnchor::LowerCenter || anchor == SceneEntity::TextAnchor::LowerRight) {
        int totalHeight = font->GetFontHeight() * textScale * numLines + lineSpacing * (numLines - 1);

        y = -totalHeight;
    } else if (anchor == SceneEntity::TextAnchor::MiddleLeft || anchor == SceneEntity::TextAnchor::MiddleCenter || anchor == SceneEntity::TextAnchor::MiddleRight) {
        int totalHeight = font->GetFontHeight() * textScale * numLines + lineSpacing * (numLines - 1);

        y = -totalHeight / 2;
    }

    GuiMesh &guiMesh = renderSystem.GetCurrentRenderContext()->GetGuiMesh();

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        ptr = textLines[lineIndex];

        // Calculate the coordinate x
        int x = 0;
        if (anchor == SceneEntity::TextAnchor::UpperRight || anchor == SceneEntity::TextAnchor::MiddleRight || anchor == SceneEntity::TextAnchor::LowerRight) {
            x = -maxWidth;
        } else if (anchor == SceneEntity::TextAnchor::UpperCenter || anchor == SceneEntity::TextAnchor::MiddleCenter || anchor == SceneEntity::TextAnchor::LowerCenter) {
            x = -maxWidth / 2;
        }

        if (alignment == SceneEntity::TextAlignment::Right) {
            x += maxWidth - font->StringWidth(ptr, lineLen[lineIndex], false, false, textScale);
        } else if (alignment == SceneEntity::TextAlignment::Center) {
            x += (maxWidth - font->StringWidth(ptr, lineLen[lineIndex], false, false, textScale)) / 2;
        }

        for (int lineTextIndex = 0; lineTextIndex < lineLen[lineIndex]; lineTextIndex++) {
            x += DrawChar(x, y, textScale, textScale, font, *ptr);
            ptr++;
        }

        y += (font->GetFontHeight() + lineSpacing) * textScale;
    }
}

AABB GuiMesh::Compute3DTextAABB(Font *font, SceneEntity::TextAnchor anchor, float lineSpacing, float textScale, const wchar_t *text) const {
    static const int MaxTextLines = 256;
    const wchar_t *textLines[MaxTextLines];
    int lineLen[MaxTextLines];
    int numLines = 0;
    int lineWidth = 0;
    int maxWidth = 0;

    textLines[0] = (const wchar_t *)text;

    const wchar_t *ptr = (const wchar_t *)text;

    while (*ptr) {
        if (*ptr == L'\n') {
            lineLen[numLines] = (int)(ptr - textLines[numLines]);
            numLines++;

            textLines[numLines] = ptr + 1;

            if (lineWidth > maxWidth) {
                maxWidth = lineWidth;
            }
            lineWidth = 0;
        } else {
            int charWidth = font->GetGlyphAdvance(*ptr) * textScale;
            lineWidth += charWidth;
        }

        ptr++;
    }

    if (ptr - textLines[numLines] > 0) {
        lineLen[numLines] = (int)(ptr - textLines[numLines]);
        numLines++;

        if (lineWidth > maxWidth) {
            maxWidth = lineWidth;
        }
    }

    int totalHeight = font->GetFontHeight() * textScale * numLines + lineSpacing * (numLines - 1);

    AABB bounds;
    bounds[0][0] = -CentiToUnit(0.1f);
    bounds[1][0] = +CentiToUnit(0.1f);

    if (anchor == SceneEntity::TextAnchor::UpperLeft || anchor == SceneEntity::TextAnchor::MiddleLeft || anchor == SceneEntity::TextAnchor::LowerLeft) {
        bounds[0][1] = 0;
        bounds[1][1] = +maxWidth;
    } else if (anchor == SceneEntity::TextAnchor::UpperRight || anchor == SceneEntity::TextAnchor::MiddleRight || anchor == SceneEntity::TextAnchor::LowerRight) {
        bounds[0][1] = -maxWidth;
        bounds[1][1] = 0;
    } else {
        int h = maxWidth / 2;
        bounds[0][1] = -h;
        bounds[1][1] = +h;
    }

    if (anchor == SceneEntity::TextAnchor::UpperLeft || anchor == SceneEntity::TextAnchor::UpperCenter || anchor == SceneEntity::TextAnchor::UpperRight) {
        bounds[0][2] = -totalHeight;
        bounds[1][2] = 0;
    } else if (anchor == SceneEntity::TextAnchor::LowerLeft || anchor == SceneEntity::TextAnchor::LowerCenter || anchor == SceneEntity::TextAnchor::LowerRight) {
        bounds[0][2] = 0;
        bounds[1][2] = +totalHeight;
    } else {
        int h = totalHeight / 2;
        bounds[0][2] = -h;
        bounds[1][2] = +h;
    }

    return bounds;
}

BE_NAMESPACE_END
