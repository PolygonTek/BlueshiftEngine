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
#include "GuiMesh.h"
#include "Texture.h"
#include "Font.h"
#include "Core/Vec4Color.h"
#include "RenderWorld.h"

void GuiMesh::Clear() {
    surfaces.SetCount(0, false);

    currentSurf = nullptr;
}

void GuiMesh::PrepareNewSurf() {
    Surface &newSurf = surfaces.Alloc();
    newSurf.texture = currentSurf ? currentSurf->texture : textureManager.defaultTexture;
    newSurf.numVerts = 0;
    newSurf.numIndexes = 0;
    newSurf.vertexBuffer = nullptr;
    newSurf.indexBuffer = nullptr;

    currentSurf = &newSurf;
}

void GuiMesh::DrawPic(RHI::FrameThreadData *frameThreadData, float x, float y, float w, float h, float s1, float t1, float s2, float t2, uint32_t color, const Texture *texture) {
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

    ALIGN_AS16 BE1::VertexGeneric localVerts[4];

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
        localVerts[0].xyz = BE1::Coords2D::To3D(x, -y);
        localVerts[1].xyz = BE1::Coords2D::To3D(x, -(y + h));
        localVerts[2].xyz = BE1::Coords2D::To3D(x + w, -(y + h));
        localVerts[3].xyz = BE1::Coords2D::To3D(x + w, -y);
    }

    // TODO: SIMD optimization
    const BE1::float16_t hs1 = BE1::F16Converter::FromF32(s1);
    const BE1::float16_t ht1 = BE1::F16Converter::FromF32(t1);
    const BE1::float16_t hs2 = BE1::F16Converter::FromF32(s2);
    const BE1::float16_t ht2 = BE1::F16Converter::FromF32(t2);

    localVerts[0].st[0] = hs1;
    localVerts[0].st[1] = ht1;
    *reinterpret_cast<uint32_t *>(localVerts[0].color) = color;

    localVerts[1].st[0] = hs1;
    localVerts[1].st[1] = ht2;
    *reinterpret_cast<uint32_t *>(localVerts[1].color) = color;

    localVerts[2].st[0] = hs2;
    localVerts[2].st[1] = ht2;
    *reinterpret_cast<uint32_t *>(localVerts[2].color) = color;

    localVerts[3].st[0] = hs2;
    localVerts[3].st[1] = ht1;
    *reinterpret_cast<uint32_t *>(localVerts[3].color) = color;

    DrawQuad(frameThreadData, localVerts, texture);
}

void GuiMesh::DrawQuad(RHI::FrameThreadData *frameThreadData, const BE1::VertexGeneric *verts, const Texture *texture) {
    if (!verts || !texture) {
        return;
    }

    // 다이나믹 버텍스 버퍼에 Quad 버텍스 데이터를 복사하기만 하고, 실제로 draw 하지는 않는다.
    // 다이나믹 인덱스 버퍼는 CacheIndexes() 에서 모든 Quad 에 대한 인덱스 데이터가 한꺼번에 만들어진다.
    if (currentSurf && texture == currentSurf->texture) {
        assert(currentSurf->vertexBuffer);
        if (frameThreadData->AppendVertex(currentSurf->vertexBuffer, sizeof(BE1::VertexGeneric), 4, verts)) {
            currentSurf->numVerts += 4;
            currentSurf->numIndexes += 6;
            return;
        }
    }

    PrepareNewSurf();

    currentSurf->texture = texture;
    currentSurf->vertexBuffer = frameThreadData->AllocVertex(sizeof(BE1::VertexGeneric), 4, verts);
    currentSurf->numVerts += 4;
    currentSurf->numIndexes += 6;
}

float GuiMesh::DrawChar(RHI::FrameThreadData *frameThreadData, float x, float y, char32_t unicodeChar) {
    if (unicodeChar == U' ') {
        return currentFont->GetGlyphAdvanceX(unicodeChar) * currentTextScale;
    }

    // Clip away before accessing to the font glyph.
    if (!clipRect.IsEmpty()) {
        if (x >= clipRect.X2() || y >= clipRect.Y2()) {
            return 0;
        }
    }

    if (currentTextDropShadows || currentTextAddOutlines) {
        FontGlyph *glyph = currentFont->GetGlyph(unicodeChar, currentTextAddOutlines ? Font::RenderMode::Border : Font::RenderMode::Normal);
        if (!glyph) {
            return 0;
        }

        float charX = x + glyph->offsetX * currentTextScale;
        float charY = y + glyph->offsetY * currentTextScale;
        float charW = glyph->width * currentTextScale;
        float charH = glyph->height * currentTextScale;

        if (currentTextDropShadows) {
            charX += currentTextShadowOffsetX * currentTextScale;
            charY += currentTextShadowOffsetY * currentTextScale;
        }

        DrawPic(frameThreadData, charX, charY, charW, charH, glyph->s, glyph->t, glyph->s2, glyph->t2, currentTextShadowColor, glyph->texture);
    }

    FontGlyph *glyph = currentFont->GetGlyph(unicodeChar);
    if (!glyph) {
        return 0;
    }

    float charX = x + glyph->offsetX * currentTextScale;
    float charY = y + glyph->offsetY * currentTextScale;
    float charW = glyph->width * currentTextScale;
    float charH = glyph->height * currentTextScale;

    DrawPic(frameThreadData, charX, charY, charW, charH, glyph->s, glyph->t, glyph->s2, glyph->t2, currentTextColor, glyph->texture);

    float pitch = glyph->advanceX * currentTextScale;
    return pitch;
}

void GuiMesh::DrawText2D(RHI::FrameThreadData *frameThreadData, const BE1::Rect &rect, int marginX, int marginY, int lineSpacing, float textScale,
    uint32_t color, uint32_t shadowColor, const BE1::Vec2 &shadowOffset, const Font *font, const BE1::Str &text, DrawTextFlag flags) {
    static const int MaxTextLines = 256;
    int lineOffsets[MaxTextLines];
    int lineLen[MaxTextLines];
    int numLines = 0;
    int currentLineWidth = 0;
    int currentLineLength = 0;
    int offset = 0;
    bool truncated = false;

    assert(coordFrame == CoordFrame::CoordFrame2D);

    currentFont = font;
    currentTextScale = textScale;
    currentTextColor = color;
    currentTextDropShadows = BE1::HasFlag(flags, DrawTextFlag::DropShadows);
    currentTextAddOutlines = BE1::HasFlag(flags, DrawTextFlag::AddOutlines);
    currentTextShadowOffsetX = shadowOffset.x;
    currentTextShadowOffsetY = shadowOffset.y;
    currentTextShadowColor = shadowColor;

    lineOffsets[0] = 0;

    while (offset < text.Length()) {
        char32_t unicodeChar = text.UTF8CharAdvance(offset);
        if (!unicodeChar) {
            break;
        }

        if (unicodeChar == BE1::UC_COLOR_ESCAPE) {
            int prevOffset = offset;
            char32_t nextUnicodeChar = text.UTF8CharAdvance(offset);

            if (nextUnicodeChar != 0 && nextUnicodeChar != BE1::UC_COLOR_ESCAPE) {
                currentLineLength += 2;
                continue;
            } else {
                offset = prevOffset;
            }
        }

        if ((BE1::HasFlag(flags, DrawTextFlag::MultiLines)) && unicodeChar == U'\n') {
            // Save current line length
            lineLen[numLines++] = currentLineLength;

            currentLineLength = 0;
            currentLineWidth = 0;

            // Save next line offset
            lineOffsets[numLines] = offset;
        } else {
            int charWidth = currentFont->GetGlyphAdvanceX(unicodeChar) * currentTextScale;

            if (BE1::HasFlag(flags, DrawTextFlag::WordWrap)) {
                if (currentLineWidth + charWidth > rect.w - marginX) {
                    // Save current line length
                    lineLen[numLines++] = currentLineLength;

                    currentLineLength = 0;
                    currentLineWidth = 0;

                    // Save next line offset
                    lineOffsets[numLines] = offset;
                    continue;
                }
            } else if (currentLineWidth + charWidth > rect.w - marginX) {
                if (BE1::HasFlag(flags, DrawTextFlag::Truncate)) {
                    if (currentLineWidth > 0) {
                        int dotdotdotWidth = currentFont->GetGlyphAdvanceX(U'.') * currentTextScale * 3;

                        do {
                            dotdotdotWidth -= currentFont->GetGlyphAdvanceX(unicodeChar) * currentTextScale;
                            unicodeChar = text.UTF8CharPrevious(offset);
                        } while (dotdotdotWidth > 0 && unicodeChar);

                        truncated = true;
                    }
                }
                // Abandon remaining text
                break;
            }

            currentLineLength++;
            currentLineWidth += charWidth;
        }
    }

    if (currentLineLength > 0) {
        lineLen[numLines++] = currentLineLength;
    }

    // Calculate the y-coordinate.
    int y = rect.y;
    if (BE1::HasAnyFlag(flags, DrawTextFlag::Bottom | DrawTextFlag::VCenter)) {
        int height = currentFont->GetFontHeight() * currentTextScale * numLines + lineSpacing * (numLines - 1);

        if (BE1::HasFlag(flags, DrawTextFlag::Bottom)) {
            y += (rect.h - height) - marginY;
        } else if (BE1::HasFlag(flags, DrawTextFlag::VCenter)) {
            y += (rect.h - height) / 2 + marginY;
        }
    } else {
        y += marginY;
    }

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        int offset = lineOffsets[lineIndex];

        // Calculate the x-coordinate.
        int x = rect.x;
        if (BE1::HasAnyFlag(flags, DrawTextFlag::Right | DrawTextFlag::Center)) {
            int width = currentFont->TextWidth(&text[offset], lineLen[lineIndex], false, true, currentTextScale);

            if (BE1::HasFlag(flags, DrawTextFlag::Right)) {
                x += (rect.w - width) - marginX;
            } else if (BE1::HasFlag(flags, DrawTextFlag::Center)) {
                x += (rect.w - width) / 2 + marginX;
            }
        } else {
            x += marginX;
        }

        for (int lineTextIndex = 0; lineTextIndex < lineLen[lineIndex]; lineTextIndex++) {
            char32_t unicodeChar = text.UTF8CharAdvance(offset);

            if (unicodeChar == BE1::UC_COLOR_ESCAPE) {
                int prevOffset = offset;
                uint32_t nextUnicodeChar = text.UTF8CharAdvance(offset);

                if (nextUnicodeChar == 0 || nextUnicodeChar == BE1::UC_COLOR_ESCAPE) {
                    offset = prevOffset;
                } else {
                    int colorIndex = UC_COLOR_INDEX(nextUnicodeChar) % COUNT_OF(BE1::Vec4Color::table);

                    currentTextColor = (0xFF000000 & currentTextColor) | (0x00FFFFFF & BE1::Vec4Color::table[colorIndex].ToUInt32());

                    lineTextIndex++;
                    continue;
                }
            }

            x += DrawChar(frameThreadData, x, y, unicodeChar);
        }

        if (truncated && lineIndex == numLines - 1) {
            x += DrawChar(frameThreadData, x, y, U'.');
            x += DrawChar(frameThreadData, x, y, U'.');
            x += DrawChar(frameThreadData, x, y, U'.');
        }

        y += (currentFont->GetFontHeight() + lineSpacing) * currentTextScale;
    }
}

void GuiMesh::DrawText3D(RHI::FrameThreadData *frameThreadData, RenderObject::TextDrawMode drawMode, RenderObject::TextAnchor anchor, RenderObject::TextHorzAlignment horzAlignment, float lineSpacing, float textScale,
    uint32_t color, uint32_t shadowColor, const BE1::Vec2 &shadowOffset, const Font *font, const BE1::Str &text) {
    static constexpr int MaxTextLines = 256;
    int lineCharOffsets[MaxTextLines];
    int lineLengths[MaxTextLines];
    int numLines = 0;
    float maxWidth = 0;
    float currentLineWidth = 0;
    int currentLineLength = 0;
    int charOffset = 0;
    char32_t unicodeChar;

    assert(coordFrame == CoordFrame::CoordFrame3D);

    currentFont = font;
    currentTextScale = textScale;
    currentTextColor = color;
    currentTextDropShadows = drawMode == RenderObject::TextDrawMode::DropShadows;
    currentTextAddOutlines = drawMode == RenderObject::TextDrawMode::AddOutlines;
    currentTextShadowOffsetX = shadowOffset.x;
    currentTextShadowOffsetY = shadowOffset.y;
    currentTextShadowColor = shadowColor;

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
            x += DrawChar(frameThreadData, x, y, text.UTF8CharAdvance(offset));
        }

        y += (font->GetFontHeight() + lineSpacing) * textScale;
    }
}

void GuiMesh::DrawText3D(RHI::FrameThreadData *frameThreadData, RenderObject::TextDrawMode drawMode, const BE1::RectF &rect, RenderObject::TextHorzAlignment horzAlignment, RenderObject::TextVertAlignment vertAlignment,
    RenderObject::TextHorzOverflow horzOverflow, RenderObject::TextVertOverflow vertOverflow, float lineSpacing, float textScale,
    uint32_t color, uint32_t shadowColor, const BE1::Vec2 &shadowOffset, const Font *font, const BE1::Str &text) {
    static constexpr int MaxTextLines = 256;
    int lineCharOffsets[MaxTextLines];
    int lineLengths[MaxTextLines];
    int numLines = 0;
    float currentLineWidth = 0;
    int currentLineLength = 0;
    int charOffset = 0;
    char32_t unicodeChar;

    assert(coordFrame == CoordFrame::CoordFrame3D);

    currentFont = font;
    currentTextScale = textScale;
    currentTextColor = color;
    currentTextDropShadows = drawMode == RenderObject::TextDrawMode::DropShadows;
    currentTextAddOutlines = drawMode == RenderObject::TextDrawMode::AddOutlines;
    currentTextShadowOffsetX = shadowOffset.x;
    currentTextShadowOffsetY = shadowOffset.y;
    currentTextShadowColor = shadowColor;

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

    while (unicodeChar = text.UTF8CharAdvance(charOffset)) {
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
            x += DrawChar(frameThreadData, x, y, text.UTF8CharAdvance(offset));
        }

        y += (font->GetFontHeight() + lineSpacing) * textScale;
    }
}

void GuiMesh::CacheIndexes(RHI::FrameThreadData *frameThreadData) {
    constexpr BE1::VertIndex quadTrisIndexes[6] = { 0, 1, 2, 0, 2, 3 };

    // 모든 surf 에 대해서 index buffer 를 채운다.
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        Surface *surf = &surfaces[surfaceIndex];
        assert(surf->numIndexes % 6 == 0);

        if (!surf->indexBuffer) {
            surf->indexBuffer = frameThreadData->AllocIndex(sizeof(quadTrisIndexes[0]), surf->numIndexes);
            BE1::VertIndex *indexPtr = reinterpret_cast<BE1::VertIndex *>(surf->indexBuffer->writePtr);

            // 현재 surf 의 첫번째 버텍스가 위치한 곳을 index 로 나타낸 값
            int baseVertexIndex = 0;

            for (int index = 0; index < surf->numIndexes; index += 6) {
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[0];
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[1];
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[2];
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[3];
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[4];
                *indexPtr++ = baseVertexIndex + quadTrisIndexes[5];

                baseVertexIndex += 4;
            }
        }
    }
}
