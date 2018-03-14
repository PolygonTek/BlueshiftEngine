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
        fontType = BitmapFont;
    } else {
        fontFace = new FontFaceFreeType;
        if (!fontFace->Load(filename, fontSize)) {
            delete fontFace;
            return false;
        }
        fontType = FreeTypeFont;
    }

    return true;
}

int Font::GetFontHeight() const {
    if (fontFace) {
        return fontFace->GetFontHeight();
    }
    return 0;
}

FontGlyph *Font::GetGlyph(int charCode) {
    if (fontFace) {
        return fontFace->GetGlyph(charCode);
    }
    return nullptr;
}

int Font::GetGlyphAdvance(int charCode) const {
    if (fontFace) {
        return fontFace->GetGlyphAdvance(charCode);
    }
    return 0;
}

int Font::StringWidth(const wchar_t *text, int maxLen, bool allowLineBreak, bool allowColoredText, float xscale) const {
    int maxWidth = 0;
    int width = 0;
    wchar_t *ptr = (wchar_t *)text;

    while (*ptr && maxLen != 0) {
        if (allowLineBreak && *ptr == L'\n') {
            if (width > maxWidth) {
                maxWidth = width;
            }

            ptr++;
            maxLen--;
            continue;
        }

        if (allowColoredText) {
            if (WS_IS_COLOR(ptr)) {
                ptr += 2;
                maxLen -= 2;
                continue;
            }
        }

        width += GetGlyphAdvance(*ptr) * xscale;
        ptr++;
        maxLen--;
    }

    if (width > maxWidth) {
        maxWidth = width;
    }

    return maxWidth;
}
    
BE_NAMESPACE_END
