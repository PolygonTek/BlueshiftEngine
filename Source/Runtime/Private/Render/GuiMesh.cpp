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
    totalVerts = 0;
    totalIndexes = 0;

    coordFrame = CoordFrame2D;

    clipRect.Set(0, 0, 0, 0);
}

void GuiMesh::Clear() {
    totalVerts = 0;
    totalIndexes = 0;

    surfaces.SetCount(0, false);

    currentColor = 0xFFFFFFFF;
    currentSurf = nullptr;
}

void GuiMesh::PrepareNextSurf() {
    GuiMeshSurf newSurf;

    newSurf.numVerts = 0;
    newSurf.numIndexes = 0;

    memset(&newSurf.vertexCache, 0, sizeof(newSurf.vertexCache));
    memset(&newSurf.indexCache, 0, sizeof(newSurf.indexCache));

    if (surfaces.Count() > 0) {
        newSurf.color = currentColor;
        newSurf.material = currentSurf->material;
    } else {
        newSurf.color = currentColor;
        newSurf.material = materialManager.defaultMaterial;
    }

    surfaces.Append(newSurf);
    currentSurf = &surfaces.Last();
}

void GuiMesh::SetColor(const Color4 &rgba) {
    currentColor = rgba.ToUInt32();
}

void GuiMesh::SetClipRect(const Rect &clipRect) {
    this->clipRect = clipRect;
}

void GuiMesh::DrawQuad(const VertexGeneric *verts, const Material *material) {
    if (!verts || !material) {
        return;
    }

    if (!currentSurf || material != currentSurf->material) {
        PrepareNextSurf();

        currentSurf->material = material;
    }

    totalVerts += 4;
    totalIndexes += 6;

    currentSurf->numVerts += 4;
    currentSurf->numIndexes += 6;

    // Cache 4 vertices in the dynamic vertex buffer
    BufferCache vertexCache;
    bufferCacheManager.AllocVertex(4, sizeof(VertexGeneric), verts, &vertexCache);

    // Set/Modify vertex cache info for the current surface
    if (!bufferCacheManager.IsCached(&currentSurf->vertexCache)) {
        currentSurf->vertexCache = vertexCache;
    } else {
        currentSurf->vertexCache.bytes += vertexCache.bytes;
    }
}

void GuiMesh::CacheIndexes() {
    if (!totalIndexes) {
        return;
    }

    assert(totalIndexes % 6 == 0);
    static const TriIndex quadTrisIndexes[6] = { 0, 1, 2, 0, 2, 3 };

    // Cache all indices in the dynamic index buffer
    BufferCache indexCache;
    bufferCacheManager.AllocIndex(totalIndexes, sizeof(TriIndex), nullptr, &indexCache);
    TriIndex *indexPointer = (TriIndex *)bufferCacheManager.MapIndexBuffer(&indexCache);

    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        GuiMeshSurf *surf = &surfaces[surfaceIndex];

        int baseVertexIndex = surf->vertexCache.offset / sizeof(VertexGeneric);

        for (int index = 0; index < surf->numIndexes; index += 6) {
            *indexPointer++ = baseVertexIndex + quadTrisIndexes[0];
            *indexPointer++ = baseVertexIndex + quadTrisIndexes[1];
            *indexPointer++ = baseVertexIndex + quadTrisIndexes[2];
            *indexPointer++ = baseVertexIndex + quadTrisIndexes[3];
            *indexPointer++ = baseVertexIndex + quadTrisIndexes[4];
            *indexPointer++ = baseVertexIndex + quadTrisIndexes[5];

            baseVertexIndex += 4;
        }
    }
    bufferCacheManager.UnmapIndexBuffer(&indexCache);

    // Set index cache info for each surfaces
    int offset = indexCache.offset;
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        GuiMeshSurf *surf = &surfaces[surfaceIndex];
    
        surf->indexCache = indexCache;
        surf->indexCache.offset = offset;
        surf->indexCache.bytes = sizeof(TriIndex) * surf->numIndexes;
        // TODO: Prebuilt index cache in max batch size and just make use of base vertex index to use calling RHI::DrawElementsBaseVertex
        //surf->indexCache.baseVertexIndex = surf->vertexCache.offset / sizeof(VertexGeneric);

        offset += surf->indexCache.bytes;
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

    ALIGN16(VertexGeneric) localVerts[4];

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

    const float16_t hs1 = F16Converter::FromF32(s1);
    const float16_t ht1 = F16Converter::FromF32(t1);
    const float16_t hs2 = F16Converter::FromF32(s2);
    const float16_t ht2 = F16Converter::FromF32(t2);

    localVerts[0].st[0] = hs1;
    localVerts[0].st[1] = ht1;
    *reinterpret_cast<uint32_t *>(localVerts[0].color) = currentColor;

    localVerts[1].st[0] = hs1;
    localVerts[1].st[1] = ht2;
    *reinterpret_cast<uint32_t *>(localVerts[1].color) = currentColor;

    localVerts[2].st[0] = hs2;
    localVerts[2].st[1] = ht2;
    *reinterpret_cast<uint32_t *>(localVerts[2].color) = currentColor;

    localVerts[3].st[0] = hs2;
    localVerts[3].st[1] = ht1;
    *reinterpret_cast<uint32_t *>(localVerts[3].color) = currentColor;

    DrawQuad(localVerts, material);
}

float GuiMesh::DrawChar(float x, float y, float sx, float sy, Font *font, wchar_t charCode) {
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

void GuiMesh::Draw(Font *font, RenderObject::TextAnchor anchor, RenderObject::TextAlignment alignment, float lineSpacing, float textScale, const wchar_t *text) {
    static const int MaxTextLines = 256;
    const wchar_t *textLines[MaxTextLines];
    int lineLen[MaxTextLines];
    int numLines = 0;
    float lineWidth = 0;
    float maxWidth = 0;

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
            float charWidth = font->GetGlyphAdvance(*ptr) * textScale;
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

    // Calculate the coordinate y
    float y = 0;
    if (anchor == RenderObject::TextAnchor::LowerLeft || anchor == RenderObject::TextAnchor::LowerCenter || anchor == RenderObject::TextAnchor::LowerRight) {
        float totalHeight = font->GetFontHeight() * textScale * numLines + lineSpacing * (numLines - 1);

        y = -totalHeight;
    } else if (anchor == RenderObject::TextAnchor::MiddleLeft || anchor == RenderObject::TextAnchor::MiddleCenter || anchor == RenderObject::TextAnchor::MiddleRight) {
        float totalHeight = font->GetFontHeight() * textScale * numLines + lineSpacing * (numLines - 1);

        y = -totalHeight / 2;
    }

    GuiMesh &guiMesh = renderSystem.GetCurrentRenderContext()->GetGuiMesh();

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        ptr = textLines[lineIndex];

        // Calculate the coordinate x
        float x = 0;
        if (anchor == RenderObject::TextAnchor::UpperRight || anchor == RenderObject::TextAnchor::MiddleRight || anchor == RenderObject::TextAnchor::LowerRight) {
            x = -maxWidth;
        } else if (anchor == RenderObject::TextAnchor::UpperCenter || anchor == RenderObject::TextAnchor::MiddleCenter || anchor == RenderObject::TextAnchor::LowerCenter) {
            x = -maxWidth / 2;
        }

        if (alignment == RenderObject::TextAlignment::Right) {
            x += maxWidth - font->StringWidth(ptr, lineLen[lineIndex], false, false, textScale);
        } else if (alignment == RenderObject::TextAlignment::Center) {
            x += (maxWidth - font->StringWidth(ptr, lineLen[lineIndex], false, false, textScale)) / 2;
        }

        for (int lineTextIndex = 0; lineTextIndex < lineLen[lineIndex]; lineTextIndex++) {
            x += DrawChar(x, y, textScale, textScale, font, *ptr);
            ptr++;
        }

        y += (font->GetFontHeight() + lineSpacing) * textScale;
    }
}

AABB GuiMesh::Compute3DTextAABB(Font *font, RenderObject::TextAnchor anchor, float lineSpacing, float textScale, const wchar_t *text) const {
    static const int MaxTextLines = 256;
    const wchar_t *textLines[MaxTextLines];
    int lineLen[MaxTextLines];
    int numLines = 0;
    float lineWidth = 0;
    float maxWidth = 0;

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
            float charWidth = font->GetGlyphAdvance(*ptr) * textScale;
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

    float totalHeight = font->GetFontHeight() * textScale * numLines + lineSpacing * (numLines - 1);

    AABB bounds;
    bounds[0][0] = -CentiToUnit(0.1f);
    bounds[1][0] = +CentiToUnit(0.1f);

    if (anchor == RenderObject::TextAnchor::UpperLeft || anchor == RenderObject::TextAnchor::MiddleLeft || anchor == RenderObject::TextAnchor::LowerLeft) {
        bounds[0][1] = 0;
        bounds[1][1] = +maxWidth;
    } else if (anchor == RenderObject::TextAnchor::UpperRight || anchor == RenderObject::TextAnchor::MiddleRight || anchor == RenderObject::TextAnchor::LowerRight) {
        bounds[0][1] = -maxWidth;
        bounds[1][1] = 0;
    } else {
        int h = maxWidth / 2;
        bounds[0][1] = -h;
        bounds[1][1] = +h;
    }

    if (anchor == RenderObject::TextAnchor::UpperLeft || anchor == RenderObject::TextAnchor::UpperCenter || anchor == RenderObject::TextAnchor::UpperRight) {
        bounds[0][2] = -totalHeight;
        bounds[1][2] = 0;
    } else if (anchor == RenderObject::TextAnchor::LowerLeft || anchor == RenderObject::TextAnchor::LowerCenter || anchor == RenderObject::TextAnchor::LowerRight) {
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
