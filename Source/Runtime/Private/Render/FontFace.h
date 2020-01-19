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

BE_NAMESPACE_BEGIN

class Material;
class FreeTypeFont;

/*
-------------------------------------------------------------------------------

    Abstract class for Font

-------------------------------------------------------------------------------
*/

class FontFace {
public:
    virtual ~FontFace() {}

    virtual FontGlyph *     GetGlyph(char32_t unicodeChar, Font::RenderMode::Enum renderMode) = 0;

                            /// Returns a offset for the next character.
    virtual int             GetGlyphAdvanceX(char32_t unicodeChar) const = 0;
    virtual int             GetGlyphAdvanceY(char32_t unicodeChar) const = 0;

    virtual int             GetFontHeight() const = 0;

    virtual bool            Load(const char *filename, int fontSize) = 0;
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

    virtual FontGlyph *     GetGlyph(char32_t unicodeChar, Font::RenderMode::Enum renderMode) override;

    virtual int             GetGlyphAdvanceX(char32_t unicodeChar) const override;
    virtual int             GetGlyphAdvanceY(char32_t unicodeChar) const override;

    virtual int             GetFontHeight() const override;

    virtual bool            Load(const char *filename, int fontSize) override;

    void                    Write(const char *filename);

private:
    void                    Purge();
    Texture *               AddBitmap(const char *filename);
    FontGlyph *             AddGlyph(char32_t charCode, int width, int height, int offsetX, int offsetY, int advanceX, int advanceY, float s, float t, float s2, float t2, int textureIndex);

    using GlyphHashMap = HashMap<char32_t, FontGlyph *>;
    GlyphHashMap            glyphHashMap;

    Array<Str>              bitmapNames;
    Array<Material *>       materialArray;

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

                            /// Caches a glyph in the texture with the given character code.
    virtual FontGlyph *     GetGlyph(char32_t unicodeChar, Font::RenderMode::Enum renderMode) override;

    virtual int             GetGlyphAdvanceX(char32_t unicodeChar) const override;
    virtual int             GetGlyphAdvanceY(char32_t unicodeChar) const override;

    virtual int             GetFontHeight() const override { return fontHeight; }

    virtual bool            Load(const char *filename, int fontSize) override;

    void                    ClearGlyphCaches();

                            /// Writes font file with bitmaps.
    bool                    Write(const char *filename);

    static void             InitAtlas();
    static void             FreeAtlas();

private:
    void                    Purge();

    FontGlyph *             CacheGlyph(char32_t unicodeChar, Font::RenderMode::Enum renderMode, int atlasPadding);
    Texture *               RenderGlyphToAtlasTexture(char32_t unicodeChar, Font::RenderMode::Enum renderMode, int atlasPadding, int &bitmapLeft, int &bitmapTop, int &glyphX, int &glyphY, int &glyphWidth, int &glyphHeight);

    void                    WriteBitmapFiles(const char *fontFilename);

    using GlyphHashMap = HashMap<int64_t, FontGlyph *>;
    GlyphHashMap            glyphHashMap;

    FreeTypeFont *          freeTypeFont = nullptr;

    int                     fontHeight;

    byte *                  glyphBuffer = nullptr;          ///< Intermediate glyph buffer to upload texture
};

BE_INLINE FontFaceFreeType::~FontFaceFreeType() {
    Purge();
}

BE_NAMESPACE_END
