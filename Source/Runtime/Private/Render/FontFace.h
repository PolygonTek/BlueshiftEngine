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

#include "freetype/include/ft2build.h"
#include "freetype/include/freetype.h"

BE_NAMESPACE_BEGIN

class Material;

/*
-------------------------------------------------------------------------------

    Abstract class for Font Face

-------------------------------------------------------------------------------
*/

class FontFace {
public:
    virtual ~FontFace() {}

    virtual FontGlyph *     GetGlyph(char32_t unicodeChar) = 0;

                            /// Returns width of given character code for the next character in string.
    virtual int             GetGlyphAdvance(char32_t unicodeChar) const = 0;

    virtual int             GetFontHeight() const = 0;

    virtual bool            Load(const char *filename, int fontSize) = 0;

protected:
    using GlyphHashMap      = HashMap<char32_t, FontGlyph *>;
    GlyphHashMap            glyphHashMap;
};

/*
-------------------------------------------------------------------------------

    Bitmap Font Face

-------------------------------------------------------------------------------
*/

class FontFaceBitmap : public FontFace {
public:
    FontFaceBitmap();
    virtual ~FontFaceBitmap();

    virtual FontGlyph *     GetGlyph(char32_t unicodeChar) override;

                            /// Returns width of given character code for the next character in string.
    virtual int             GetGlyphAdvance(char32_t unicodeChar) const override;

    virtual int             GetFontHeight() const override;

    virtual bool            Load(const char *filename, int fontSize) override;

private:
    void                    Purge();

    int                     fontHeight;
};

/*
-------------------------------------------------------------------------------

    FreeType Font Face

-------------------------------------------------------------------------------
*/

class FontFaceFreeType : public FontFace {
public:
    FontFaceFreeType() = default;
    virtual ~FontFaceFreeType();

    virtual FontGlyph *     GetGlyph(char32_t unicodeChar) override;

                            /// Returns width of given character code for the next character in string.
    virtual int             GetGlyphAdvance(char32_t unicodeChar) const override;

    virtual int             GetFontHeight() const override { return fontHeight; }

    virtual bool            Load(const char *filename, int fontSize) override;

    static void             Init();
    static void             Shutdown();
    
private:
    void                    Purge();

    bool                    LoadFTGlyph(char32_t unicodeChar) const;
    void                    DrawGlyphBufferFromFTBitmap(const FT_Bitmap *bitmap) const;

    int                     fontHeight;

    byte *                  ftFontFileData = nullptr;       ///< FreeType font flie data
    FT_Face                 ftFace = nullptr;               ///< FreeType font face object
    FT_Long                 ftFaceIndex = 0;

    mutable char32_t        lastLoadedChar;                 ///< Last loaded character code
    byte *                  glyphBuffer = nullptr;          ///< Intermediate glyph buffer to upload texture
};

BE_NAMESPACE_END
