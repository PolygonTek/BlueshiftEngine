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

static constexpr int    TextLineSpacing = 4;

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

void GuiMesh::SetTextStyle(const Font *font, float scaleX, float scaleY, uint32_t color, float shadowOffsetX, float shadowOffsetY, uint32_t shadowColor) {
    currentFont = font;
    currentTextScaleX = scaleX;
    currentTextScaleY = scaleY;
    currentTextColor = color;
    currentTextShadowOffsetX = shadowOffsetX;
    currentTextShadowOffsetY = shadowOffsetY;
    currentTextShadowColor = shadowColor;
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
    if (!currentSurf || texture != currentSurf->texture) {
        PrepareNewSurf();
    } else {
        assert(currentSurf->vertexBuffer);
        if (frameThreadData->AppendVertex(currentSurf->vertexBuffer, sizeof(BE1::VertexGeneric), 4, verts)) {
            currentSurf->numVerts += 4;
            currentSurf->numIndexes += 6;
            return;
        }
        PrepareNewSurf();
    }

    currentSurf->texture = texture;
    currentSurf->vertexBuffer = frameThreadData->AllocVertex(sizeof(BE1::VertexGeneric), 4, verts);
    currentSurf->numVerts += 4;
    currentSurf->numIndexes += 6;
}

float GuiMesh::DrawChar(RHI::FrameThreadData *frameThreadData, float x, float y, char32_t unicodeChar, DrawTextFlag flags) {
    if (unicodeChar == U' ') {
        return currentFont->GetGlyphAdvanceX(unicodeChar) * currentTextScaleX;
    }

    // Clip away before accessing to the font glyph.
    if (!clipRect.IsEmpty()) {
        if (x >= clipRect.X2() || y >= clipRect.Y2()) {
            return 0;
        }
    }

    if (BE1::HasAnyFlag(flags, DrawTextFlag::DrawShadow | DrawTextFlag::DrawBorder)) {
        FontGlyph *glyph = currentFont->GetGlyph(unicodeChar, BE1::HasFlag(flags, DrawTextFlag::DrawBorder) ? Font::RenderMode::Border : Font::RenderMode::Normal);
        if (!glyph) {
            return 0;
        }

        float charX = x + glyph->offsetX * currentTextScaleX;
        float charY = y + glyph->offsetY * currentTextScaleY;
        float charW = glyph->width * currentTextScaleX;
        float charH = glyph->height * currentTextScaleY;

        if (BE1::HasFlag(flags, DrawTextFlag::DrawShadow)) {
            charX += currentTextShadowOffsetX * currentTextScaleX;
            charY += currentTextShadowOffsetY * currentTextScaleY;
        }

        DrawPic(frameThreadData, charX, charY, charW, charH, glyph->s, glyph->t, glyph->s2, glyph->t2, currentTextShadowColor, glyph->texture);
    }

    FontGlyph *glyph = currentFont->GetGlyph(unicodeChar);
    if (!glyph) {
        return 0;
    }

    float charX = x + glyph->offsetX * currentTextScaleX;
    float charY = y + glyph->offsetY * currentTextScaleY;
    float charW = glyph->width * currentTextScaleX;
    float charH = glyph->height * currentTextScaleY;

    DrawPic(frameThreadData, charX, charY, charW, charH, glyph->s, glyph->t, glyph->s2, glyph->t2, currentTextColor, glyph->texture);

    float pitch = glyph->advanceX * currentTextScaleX;
    return pitch;
}

void GuiMesh::DrawTextInRect(RHI::FrameThreadData *frameThreadData, const BE1::Rect &rect, int marginX, int marginY, const BE1::Str &text, int textLength, DrawTextFlag flags) {
    static const int MaxTextLines = 256;
    int lineOffsets[MaxTextLines];
    int lineLen[MaxTextLines];
    int numLines = 0;
    int currentLineWidth = 0;
    int currentLineLength = 0;
    int offset = 0;
    bool truncated = false;

    if (textLength == -1) {
        textLength = text.Length();
    }

    lineOffsets[0] = 0;

    while (offset < textLength) {
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
            int charWidth = currentFont->GetGlyphAdvanceX(unicodeChar) * currentTextScaleX;

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
                        int dotdotdotWidth = currentFont->GetGlyphAdvanceX(U'.') * currentTextScaleX * 3;

                        do {
                            dotdotdotWidth -= currentFont->GetGlyphAdvanceX(unicodeChar) * currentTextScaleX;
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

    // Calculate the y-coordinate
    int y;
    if (BE1::HasAnyFlag(flags, DrawTextFlag::Bottom | DrawTextFlag::VCenter)) {
        int height = currentFont->GetFontHeight() * currentTextScaleY * numLines + TextLineSpacing * (numLines - 1);

        if (BE1::HasFlag(flags, DrawTextFlag::Bottom)) {
            y = rect.y + rect.h - height - marginY;
        } else if (BE1::HasFlag(flags, DrawTextFlag::VCenter)) {
            y = rect.y + (rect.h - height) / 2 + marginY;
        }
    } else {
        y = rect.y + marginY;
    }

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        offset = lineOffsets[lineIndex];

        // Calculate the x-coordinate
        int x;
        if (BE1::HasAnyFlag(flags, DrawTextFlag::Right | DrawTextFlag::Center)) {
            int width = currentFont->TextWidth(&text[offset], lineLen[lineIndex], false, true, currentTextScaleX);

            if (BE1::HasFlag(flags, DrawTextFlag::Right)) {
                x = rect.x + rect.w - width - marginX;
            } else if (BE1::HasFlag(flags, DrawTextFlag::Center)) {
                x = rect.x + (rect.w - width) / 2 + marginX;
            }
        } else {
            x = rect.x + marginX;
        }

        for (int lineTextIndex = 0; lineTextIndex < lineLen[lineIndex]; lineTextIndex++) {
            char32_t unicodeChar = text.UTF8CharAdvance(offset);

            int colorIndex = -1;

            if (unicodeChar == BE1::UC_COLOR_ESCAPE) {
                int prevOffset = offset;
                uint32_t nextUnicodeChar = text.UTF8CharAdvance(offset);

                if (nextUnicodeChar != 0 && nextUnicodeChar != BE1::UC_COLOR_ESCAPE) {
                    colorIndex = UC_COLOR_INDEX(nextUnicodeChar);
                } else {
                    offset = prevOffset;
                }
            }

            if (colorIndex >= 0) {
                BE1::Clamp(colorIndex, 0, (int)COUNT_OF(BE1::Vec4Color::table) - 1);
                currentTextColor = (0xFF000000 & currentTextColor) | (0x00FFFFFF & BE1::Vec4Color::table[colorIndex].ToUInt32());

                lineTextIndex++;
                continue;
            }

            x += DrawChar(frameThreadData, x, y, unicodeChar, flags);
        }

        if (truncated && lineIndex == numLines - 1) {
            x += DrawChar(frameThreadData, x, y, U'.', flags);
            x += DrawChar(frameThreadData, x, y, U'.', flags);
            x += DrawChar(frameThreadData, x, y, U'.', flags);
        }

        y += (currentFont->GetFontHeight() + TextLineSpacing) * currentTextScaleY;
    }
}

void GuiMesh::CacheIndexes(RHI::FrameThreadData *frameThreadData) {
    if (surfaces.Count() == 0) {
        return;
    }

    constexpr BE1::VertIndex quadTrisIndexes[6] = { 0, 1, 2, 0, 2, 3 };

    // 모든 surf 에 대해서 index buffer 를 채운다.
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        Surface *surf = &surfaces[surfaceIndex];
        assert(surf->numIndexes % 6 == 0);

        if (!surf->indexBuffer) {
            surf->indexBuffer = frameThreadData->AllocIndex(sizeof(quadTrisIndexes[0]), surf->numIndexes);
            BE1::VertIndex *indexPtr = reinterpret_cast<BE1::VertIndex *>(surf->indexBuffer->writePtr);

            // 현재 surf 의 첫번째 버텍스가 위치한 곳을 index 로 나타낸 값
            int baseVertexIndex = 0;//surf->vertexBuffer->GetOffset() / sizeof(BE1::VertexGeneric);

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
