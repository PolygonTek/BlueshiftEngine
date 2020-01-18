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

#include "Core/Str.h"
#include "Containers/HashIndex.h"
#include "Containers/HashMap.h"

// FreeType headers
#include "freetype/include/ft2build.h"
#include "freetype/include/freetype.h"
#include "freetype/include/ftglyph.h"
#include "freetype/include/ftbitmap.h"
#include "freetype/include/ftstroke.h"

BE_NAMESPACE_BEGIN

class FreeTypeFont {
public:
    ~FreeTypeFont();

    static void             Init();
    static void             Shutdown();

    bool                    Create(const char *filename, int fontSize);
    void                    Purge();

    bool                    LoadGlyph(char32_t unicodeChar) const;

    FT_Face                 GetFace() const { return ftFace; }

    FT_GlyphSlot            RenderGlyph(FT_Render_Mode ftRenderMode);
    FT_Glyph                RenderGlyphWithBorder(FT_Render_Mode ftRenderMode, int borderRadius);

    void                    BakeGlyphBitmap(const FT_Bitmap *bitmap, int paddingX, int paddingY, byte *pixels);

private:
    FT_Byte *               ftFontFileData = nullptr;       ///< FreeType font flie data.
    FT_Face                 ftFace = nullptr;               ///< FreeType font face object.
    FT_Long                 ftFaceIndex = 0;

    mutable char32_t        lastLoadedChar;                 ///< Last loaded character code.
};

BE_INLINE FreeTypeFont::~FreeTypeFont() {
    Purge();
}

BE_NAMESPACE_END
