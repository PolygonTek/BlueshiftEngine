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
#include "Engine/Coords2D.h"
#include "Render/Render.h"
#include "Render/Font.h"
#include "RenderInternal.h"
#include "SIMD/SIMD.h"

BE_NAMESPACE_BEGIN

GuiMesh::GuiMesh() {
    coordFrame = CoordFrame::CoordFrame2D;

    totalVerts = 0;
    totalIndexes = 0;

    clipRect = Rect::zero;
}

void GuiMesh::Clear() {
    totalVerts = 0;
    totalIndexes = 0;

    surfaces.SetCount(0, false);

    currentColor = Color4::white.ToUInt32();
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

void GuiMesh::SetTextBorderColor(const Color4 &rgba) {
    currentTextBorderColor = rgba.ToUInt32();
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

    // Cache 4 vertices in the dynamic vertex buffer.
    BufferCache vertexCache;
    bufferCacheManager.AllocVertex(4, sizeof(VertexGeneric), verts, &vertexCache);

    // Set/Modify vertex cache info for the current surface.
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
    static constexpr TriIndex quadTrisIndexes[6] = { 0, 1, 2, 0, 2, 3 };

    // Cache all indices in the dynamic index buffer.
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

    // Set index cache info for each surfaces.
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
    if (coordFrame == CoordFrame::CoordFrame2D && !clipRect.IsEmpty()) {
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
        // Completely clipped away.
        return;
    }

    ALIGN_AS16 VertexGeneric localVerts[4];

    if (coordFrame == CoordFrame::CoordFrame2D) {
        // 2D frame
        //  +-----> +X
        //  |
        //  |
        // +Y
        localVerts[0].xyz.Set(x, y, 0);
        localVerts[1].xyz.Set(x, y + h, 0);
        localVerts[2].xyz.Set(x + w, y + h, 0);
        localVerts[3].xyz.Set(x + w, y, 0);
    } else {
        // 3D frame
        // +Z
        //  |
        //  |
        //  +-----> +Y
        localVerts[0].xyz = Coords2D::To3D(x, -y);
        localVerts[1].xyz = Coords2D::To3D(x, -(y + h));
        localVerts[2].xyz = Coords2D::To3D(x + w, -(y + h));
        localVerts[3].xyz = Coords2D::To3D(x + w, -y);
    }
    
    // TODO: SIMD optimization
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

float GuiMesh::DrawChar(float x, float y, float sx, float sy, Font *font, char32_t unicodeChar, RenderObject::TextDrawMode::Enum drawMode) {
    if (unicodeChar == U' ') {
        return font->GetGlyphAdvanceX(unicodeChar) * sx;
    }

    // Clip away before accessing to the font glyph.
    if (!clipRect.IsEmpty()) {
        if (x >= clipRect.X2() || y >= clipRect.Y2()) {
            return 0;
        }
    }

    if (drawMode == RenderObject::TextDrawMode::DropShadows || drawMode == RenderObject::TextDrawMode::AddOutlines) {
        FontGlyph *glyph = font->GetGlyph(unicodeChar, Font::RenderMode::Border);
        if (!glyph) {
            return 0;
        }

        float charX = x + glyph->offsetX * sx;
        float charY = y + glyph->offsetY * sy;
        float charW = glyph->width * sx;
        float charH = glyph->height * sy;

        if (drawMode == RenderObject::TextDrawMode::DropShadows) {
            float shadowOffsetX = 1.0f;
            float shadowOffsetY = 1.0f;

            charX += shadowOffsetX * sx;
            charY += shadowOffsetY * sy;
        }

        uint32_t oldColor = currentColor;

        currentColor = currentTextBorderColor;

        DrawPic(charX, charY, charW, charH, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->material);

        currentColor = oldColor;
    }

    FontGlyph *glyph = font->GetGlyph(unicodeChar);
    if (!glyph) {
        return 0;
    }

    float charX = x + glyph->offsetX * sx;
    float charY = y + glyph->offsetY * sy;
    float charW = glyph->width * sx;
    float charH = glyph->height * sy;

    DrawPic(charX, charY, charW, charH, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->material);

    float pitch = glyph->advanceX * sx;
    return pitch;
}

void GuiMesh::DrawText(Font *font, RenderObject::TextDrawMode::Enum drawMode, RenderObject::TextAnchor::Enum anchor,
    RenderObject::TextHorzAlignment::Enum horzAlignment, float lineSpacing, float textScale, const Str &text) {
    static constexpr int MaxTextLines = 256;
    int lineCharOffsets[MaxTextLines];
    int lineLengths[MaxTextLines];
    int numLines = 0;
    float maxWidth = 0;
    float currentLineWidth = 0;
    int currentLineLength = 0;
    int charOffset = 0;
    char32_t unicodeChar;

    lineCharOffsets[0] = 0;
    
    while ((unicodeChar = text.UTF8CharAdvance(charOffset))) {
        if (unicodeChar == U'\n') {
            // Save current line length.
            lineLengths[numLines++] = currentLineLength;

            if (currentLineWidth > maxWidth) {
                maxWidth = currentLineWidth;
            }

            currentLineWidth = 0;
            currentLineLength = 0;

            // Save next line offset.
            lineCharOffsets[numLines] = charOffset;
        } else {
            float charWidth = font->GetGlyphAdvanceX(unicodeChar) * textScale;
            currentLineWidth += charWidth;
            currentLineLength++;
        }
    }

    if (currentLineLength > 0) {
        lineLengths[numLines++] = currentLineLength;

        if (currentLineWidth > maxWidth) {
            maxWidth = currentLineWidth;
        }
    }

    // Calculate the y coordinate.
    float y = 0;
    if (anchor != RenderObject::TextAnchor::UpperLeft &&
        anchor != RenderObject::TextAnchor::UpperCenter &&
        anchor != RenderObject::TextAnchor::UpperRight) {
        float totalHeight = textScale * (font->GetFontHeight() * numLines + lineSpacing * (numLines - 1));

        if (anchor == RenderObject::TextAnchor::LowerLeft ||
            anchor == RenderObject::TextAnchor::LowerCenter ||
            anchor == RenderObject::TextAnchor::LowerRight) {
            y = -totalHeight;
        } else if (
            anchor == RenderObject::TextAnchor::MiddleLeft ||
            anchor == RenderObject::TextAnchor::MiddleCenter ||
            anchor == RenderObject::TextAnchor::MiddleRight) {
            y = -totalHeight * 0.5f;
        }
    }

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        int offset = lineCharOffsets[lineIndex];

        // Calculate the x coordinate.
        float x = 0;
        if (anchor == RenderObject::TextAnchor::UpperRight || 
            anchor == RenderObject::TextAnchor::MiddleRight || 
            anchor == RenderObject::TextAnchor::LowerRight) {
            x = -maxWidth;
        } else if (
            anchor == RenderObject::TextAnchor::UpperCenter || 
            anchor == RenderObject::TextAnchor::MiddleCenter || 
            anchor == RenderObject::TextAnchor::LowerCenter) {
            x = -maxWidth * 0.5f;
        }

        if (horzAlignment != RenderObject::TextHorzAlignment::Left) {
            float lineWidth = font->TextWidth(&text[offset], lineLengths[lineIndex], false, false, textScale);

            if (horzAlignment == RenderObject::TextHorzAlignment::Right) {
                x += maxWidth - lineWidth;
            } else if (horzAlignment == RenderObject::TextHorzAlignment::Center) {
                x += (maxWidth - lineWidth) * 0.5f;
            }
        }

        for (int lineTextIndex = 0; lineTextIndex < lineLengths[lineIndex]; lineTextIndex++) {
            x += DrawChar(x, y, textScale, textScale, font, text.UTF8CharAdvance(offset), drawMode);
        }

        y += (font->GetFontHeight() + lineSpacing) * textScale;
    }
}

void GuiMesh::DrawTextRect(Font *font, RenderObject::TextDrawMode::Enum drawMode, const RectF &rect,
    RenderObject::TextHorzAlignment::Enum horzAlignment, RenderObject::TextVertAlignment::Enum vertAlignment,
    RenderObject::TextHorzOverflow::Enum horzOverflow, RenderObject::TextVertOverflow::Enum vertOverflow, float lineSpacing, float textScale, const Str &text) {
    static constexpr int MaxTextLines = 256;
    int lineCharOffsets[MaxTextLines];
    int lineLengths[MaxTextLines];
    int numLines = 0;
    float currentLineWidth = 0;
    int currentLineLength = 0;
    int charOffset = 0;
    char32_t unicodeChar;

    lineCharOffsets[0] = 0;

    auto PrepareNextLine = [&]() -> bool {
        // Save current line length.
        lineLengths[numLines++] = currentLineLength;

        currentLineWidth = 0;
        currentLineLength = 0;

        if (vertOverflow == RenderObject::TextVertOverflow::Truncate) {
            int currentTextHeight = textScale * (font->GetFontHeight() * numLines + lineSpacing * (numLines - 1));

            if (currentTextHeight > rect.h) {
                numLines--;
                return false;
            }
        }

        // Save next line offset.
        lineCharOffsets[numLines] = charOffset;
        return true;
    };

    int charPrevOffset = 0;

    while ((unicodeChar = text.UTF8CharAdvance(charOffset))) {
        if (unicodeChar == U'\n') {
            if (!PrepareNextLine()) {
                break;
            }
        } else {
            float charWidth = font->GetGlyphAdvanceX(unicodeChar) * textScale;

            if (horzOverflow == RenderObject::TextHorzOverflow::Wrap && currentLineWidth + charWidth > rect.w) {
                charOffset = charPrevOffset;

                if (!PrepareNextLine()) {
                    break;
                }
            } else {
                currentLineWidth += charWidth;
                currentLineLength++;
            }
        }

        charPrevOffset = charOffset;
    }

    if (currentLineLength > 0) {
        PrepareNextLine();
    }    

    // Calculate the y coordinate.
    float y = -rect.Y2();
    if (vertAlignment != RenderObject::TextVertAlignment::Top) {
        float totalHeight = textScale * (font->GetFontHeight() * numLines + lineSpacing * (numLines - 1));

        if (vertAlignment == RenderObject::TextVertAlignment::Bottom) {
            y += (rect.h - totalHeight);
        } else if (vertAlignment == RenderObject::TextVertAlignment::Middle) {
            y += (rect.h - totalHeight) * 0.5f;
        }
    }

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        int offset = lineCharOffsets[lineIndex];

        // Calculate the x coordinate.
        float x = rect.x;
        if (horzAlignment != RenderObject::TextHorzAlignment::Left) {
            float lineWidth = font->TextWidth(&text[offset], lineLengths[lineIndex], false, false, textScale);

            if (horzAlignment == RenderObject::TextHorzAlignment::Right) {
                x += rect.w - lineWidth;
            } else if (horzAlignment == RenderObject::TextHorzAlignment::Center) {
                x += (rect.w - lineWidth) * 0.5f;
            }
        }

        for (int lineTextIndex = 0; lineTextIndex < lineLengths[lineIndex]; lineTextIndex++) {
            x += DrawChar(x, y, textScale, textScale, font, text.UTF8CharAdvance(offset), drawMode);
        }

        y += (font->GetFontHeight() + lineSpacing) * textScale;
    }
}

AABB GuiMesh::Compute3DTextAABB(Font *font, RenderObject::TextAnchor::Enum anchor, float lineSpacing, float textScale, const Str &text) const {
    float currentLineWidth = 0;
    float maxWidth = 0;
    int numLines = 0;
    int charOffset = 0;
    char32_t unicodeChar;

    while ((unicodeChar = text.UTF8CharAdvance(charOffset))) {
        if (unicodeChar == U'\n') {
            numLines++;

            if (currentLineWidth > maxWidth) {
                maxWidth = currentLineWidth;
            }
            currentLineWidth = 0;
        } else {
            float charWidth = font->GetGlyphAdvanceX(unicodeChar) * textScale;
            currentLineWidth += charWidth;
        }
    }

    if (unicodeChar != U'\n') {
        numLines++;

        if (currentLineWidth > maxWidth) {
            maxWidth = currentLineWidth;
        }
    }

    float maxHeight = textScale * (font->GetFontHeight() * numLines + lineSpacing * (numLines - 1));

    AABB bounds;
    bounds[0].z = -CentiToUnit(0.1f);
    bounds[1].z = +CentiToUnit(0.1f);

    if (anchor == RenderObject::TextAnchor::UpperLeft || 
        anchor == RenderObject::TextAnchor::MiddleLeft || 
        anchor == RenderObject::TextAnchor::LowerLeft) {
        bounds[0].x = 0;
        bounds[1].x = +maxWidth;
    } else if (
        anchor == RenderObject::TextAnchor::UpperRight || 
        anchor == RenderObject::TextAnchor::MiddleRight || 
        anchor == RenderObject::TextAnchor::LowerRight) {
        bounds[0].x = -maxWidth;
        bounds[1].x = 0;
    } else {
        float h = maxWidth * 0.5f;
        bounds[0].x = -h;
        bounds[1].x = +h;
    }

    if (anchor == RenderObject::TextAnchor::UpperLeft || 
        anchor == RenderObject::TextAnchor::UpperCenter || 
        anchor == RenderObject::TextAnchor::UpperRight) {
        bounds[0].y = -maxHeight;
        bounds[1].y = 0;
    } else if (
        anchor == RenderObject::TextAnchor::LowerLeft || 
        anchor == RenderObject::TextAnchor::LowerCenter || 
        anchor == RenderObject::TextAnchor::LowerRight) {
        bounds[0].y = 0;
        bounds[1].y = +maxHeight;
    } else {
        float h = maxHeight * 0.5f;
        bounds[0].y = -h;
        bounds[1].y = +h;
    }

    return bounds;
}

BE_NAMESPACE_END
