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
#include "Core/StrColor.h"
#include "FontFace.h"

BE_NAMESPACE_BEGIN

Font::Font() {
    fontType = FontType::None;
    fontFace = nullptr;
}

void Font::Purge() {
    SAFE_DELETE(fontFace);
}

bool Font::Load(const char *filename) {
    Purge();

    if (Str::CheckExtension(filename, ".font")) {
        fontFace = new FontFaceBitmap;
        if (!fontFace->Load(filename, 0/*fontSize*/)) {
            delete fontFace;
            return false;
        }
        fontType = FontType::Bitmap;
    } else {
        fontFace = new FontFaceFreeType;
        if (!fontFace->Load(filename, fontSize)) {
            delete fontFace;
            return false;
        }
        fontType = FontType::FreeType;
    }

    return true;
}

int Font::GetFontHeight() const {
    if (fontFace) {
        return fontFace->GetFontHeight();
    }
    return 0;
}

FontGlyph *Font::GetGlyph(char32_t unicodeChar) {
    if (fontFace) {
        return fontFace->GetGlyph(unicodeChar);
    }
    return nullptr;
}

int Font::GetGlyphAdvance(char32_t unicodeChar) const {
    if (fontFace) {
        return fontFace->GetGlyphAdvance(unicodeChar);
    }
    return 0;
}

float Font::StringWidth(const Str &text, int maxLength, bool allowLineBreak, bool allowColoredText, float xScale) const {
    float maxWidth = 0;
    float width = 0;
    int offset = 0;
    char32_t unicodeChar;

    while ((unicodeChar = text.UTF8CharAdvance(offset)) && maxLength != 0) {
        if (allowLineBreak && unicodeChar == U'\n') {
            if (width > maxWidth) {
                maxWidth = width;
            }

            maxLength--;
            continue;
        }

        if (allowColoredText) {
            if (unicodeChar == UC_COLOR_ESCAPE) {
                int prevOffset = offset;
                uint32_t nextUnicodeChar = text.UTF8CharAdvance(offset);

                if (nextUnicodeChar != 0 && nextUnicodeChar != UC_COLOR_ESCAPE) {
                    maxLength -= 2;
                    continue;
                } else {
                    offset = prevOffset;
                }
            }
        }

        width += GetGlyphAdvance(unicodeChar) * xScale;
        maxLength--;
    }

    if (width > maxWidth) {
        maxWidth = width;
    }

    return maxWidth;
}

BE_NAMESPACE_END
