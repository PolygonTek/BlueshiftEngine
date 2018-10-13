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

class FontFace {
public:
    virtual ~FontFace() {}

    virtual int             GetFontHeight() const = 0;

    virtual FontGlyph *     GetGlyph(wchar_t charCode) = 0;

                            // Returns width of charCode character for the next character in string.
    virtual int             GetGlyphAdvance(wchar_t charCode) const = 0;

    virtual bool            Load(const char *filename, int fontSize) = 0;

protected:
    // Hash generator for the wide character
    struct HashGeneratorCharCode {
        template <typename Type>
        static int Hash(const HashIndex &hasher, const Type &value) {
            return (((value >> 8) & 0xFF) * 119 + (value & 0xFF)) & (hasher.GetHashSize() - 1);
        }
    };

    using GlyphHashMap      = HashMap<wchar_t, FontGlyph *, HashCompareDefault, HashGeneratorCharCode>;
    GlyphHashMap            glyphHashMap;
};

class FontFaceBitmap : public FontFace {
public:
    FontFaceBitmap();
    virtual ~FontFaceBitmap();

    virtual int             GetFontHeight() const override;

    virtual FontGlyph *     GetGlyph(wchar_t charcode) override;
    virtual int             GetGlyphAdvance(wchar_t charcode) const override;

    virtual bool            Load(const char *filename, int fontSize) override;

private:
    void                    Purge();

    int                     fontHeight;
};

BE_INLINE FontFaceBitmap::FontFaceBitmap() {
    glyphHashMap.Init(1024, 1024, 1024);
}

BE_INLINE FontFaceBitmap::~FontFaceBitmap() {
    Purge();
}

class FontFaceFreeType : public FontFace {
public:
    FontFaceFreeType();
    virtual ~FontFaceFreeType();

    virtual int             GetFontHeight() const override;

    virtual FontGlyph *     GetGlyph(wchar_t charcode) override;
    virtual int             GetGlyphAdvance(wchar_t charcode) const override;

    virtual bool            Load(const char *filename, int fontSize) override;

                            // Init/Shutdown function for FreeType libary.
    static void             Init();
    static void             Shutdown();
    
private:
    void                    Purge();

    bool                    LoadFTGlyph(wchar_t charcode) const;
    void                    DrawGlyphBufferFromFTBitmap(const FT_Bitmap *bitmap) const;

    int                     faceIndex;
    int                     fontHeight;

    byte *                  ftFontFileData;             // FreeType font flie data
    FT_Face                 ftFace;
    mutable wchar_t         ftLastLoadedCharCode;
    byte *                  glyphBuffer;
};

BE_INLINE FontFaceFreeType::FontFaceFreeType() {
    ftFontFileData = nullptr;
    ftFace = nullptr;
    glyphBuffer = nullptr;
}

BE_INLINE FontFaceFreeType::~FontFaceFreeType() {
    Purge();
}

BE_NAMESPACE_END
