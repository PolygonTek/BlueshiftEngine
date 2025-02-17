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

#pragma once

#include "Containers/HashMap.h"
#include "Font.h"

class Texture;
class Material;
class TrueTypeFont;

/*
-------------------------------------------------------------------------------

    Abstract font face class

-------------------------------------------------------------------------------
*/

class FontFace {
public:
    virtual ~FontFace() = 0 {}

    virtual FontGlyph *     GetGlyph(char32_t unicodeChar, Font::RenderMode renderMode) const = 0;

                            /// Returns a offset for the next character.
    virtual int32_t         GetGlyphAdvanceX(char32_t unicodeChar) const = 0;
    virtual int32_t         GetGlyphAdvanceY(char32_t unicodeChar) const = 0;

    virtual uint32_t        GetFontHeight() const = 0;

    virtual bool            Load(const char *filename, int fontSize) = 0;
};

/*
-------------------------------------------------------------------------------

    Bitmap font face

-------------------------------------------------------------------------------
*/

class BitmapFontFace : public FontFace {
public:
    BitmapFontFace() = default;
    virtual ~BitmapFontFace() { Purge(); }

    virtual FontGlyph *     GetGlyph(char32_t unicodeChar, Font::RenderMode renderMode) const override;

    virtual int32_t         GetGlyphAdvanceX(char32_t unicodeChar) const override;
    virtual int32_t         GetGlyphAdvanceY(char32_t unicodeChar) const override;

    virtual uint32_t        GetFontHeight() const override { return fontHeight; }

    virtual bool            Load(const char *filename, int fontSize) override;

    void                    Write(const char *filename);

private:
    void                    Purge();
    Texture *               AddBitmap(const char *filename);
    FontGlyph *             AddGlyph(char32_t charCode, int width, int height, int offsetX, int offsetY, int advanceX, int advanceY, float s, float t, float s2, float t2, int textureIndex);

    using GlyphHashMap      = BE1::HashMap<char32_t, FontGlyph *>;
    GlyphHashMap            glyphHashMap;

    BE1::Array<BE1::Str>    bitmapNames;
    BE1::Array<Texture *>   textures;
    uint32_t                fontHeight;
};

/*
-------------------------------------------------------------------------------

    TrueType font face

-------------------------------------------------------------------------------
*/

class TrueTypeFontFace : public FontFace {
public:
    TrueTypeFontFace() = default;
    virtual ~TrueTypeFontFace() { Purge(); }

                            /// Caches a glyph in the texture with the given character code.
    virtual FontGlyph *     GetGlyph(char32_t unicodeChar, Font::RenderMode renderMode) const override;

    virtual int32_t         GetGlyphAdvanceX(char32_t unicodeChar) const override;
    virtual int32_t         GetGlyphAdvanceY(char32_t unicodeChar) const override;

    virtual uint32_t        GetFontHeight() const override { return fontHeight; }

    virtual bool            Load(const char *filename, int fontSize) override;

    void                    ClearGlyphCaches();

                            /// Writes font file with bitmaps.
    bool                    Write(const char *filename);

    static void             InitAtlas();
    static void             FreeAtlas();

private:
    void                    Purge();

    FontGlyph *             CacheGlyph(char32_t unicodeChar, Font::RenderMode renderMode, int atlasPadding) const;
    Texture *               RenderGlyphToAtlasTexture(char32_t unicodeChar, Font::RenderMode renderMode, int atlasPadding, int &bitmapLeft, int &bitmapTop, int &glyphX, int &glyphY, int &glyphWidth, int &glyphHeight) const;

    void                    WriteBitmapFiles(const char *fontFilename);

    using GlyphHashMap      = BE1::HashMap<int64_t, FontGlyph *>;
    mutable GlyphHashMap    glyphHashMap;

    TrueTypeFont *          trueTypeFont = nullptr;
    uint32_t                fontHeight = 0;

    mutable byte *          glyphBuffer = nullptr;          ///< Intermediate glyph buffer to upload texture
};
