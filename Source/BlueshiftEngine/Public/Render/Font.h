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

/*
-------------------------------------------------------------------------------

    Font
    
-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class FontFace;

// Freetype / bitmap font 에 공통으로 쓰이는 glyph 정보 구조체
struct FontGlyph {
    int                     charCode;
    int                     width, height;
    int                     bearingX, bearingY;
    int                     advance;
    float                   s, t, s2, t2;
    Material *              material;
};

class Font {
    friend class FontManager;

public:
    enum FontType {
        None,
        BitmapFont,
        FreeTypeFont
    };

    Font();
    ~Font();

    const char *            GetName() const { return name; }
    const char *            GetHashName() const { return hashName; }

    int                     GetFontHeight() const;

                            /// Returns pointer to the glyph structure corresponding to a character. Return null if glyph not found.
    FontGlyph *             GetGlyph(int charCode);

                            /// Returns a offset for a next character 
    int                     GetGlyphAdvance(int charCode) const;

    int                     StringWidth(const wchar_t *text, int maxLen, bool allowLineBreak = false, bool allowColoredText = false, float xScale = 1.0f) const;

    void                    Purge();
    bool                    Load(const char *filename);

private:
    Str                     hashName;
    Str                     name;
    mutable int             refCount;
    bool                    permanence;

    FontType                fontType;
    int                     fontSize;
    FontFace *              fontFace;
};

BE_INLINE Font::~Font() {
    Purge();
}

class FontManager {
public:
    void                    Init();
    void                    Shutdown();

    Font *                  AllocFont(const char *name, int fontSize);
    Font *                  FindFont(const char *name, int fontSize) const;
    Font *                  GetFont(const char *name, int fontSize);

    void                    ReleaseFont(Font *font, bool immediateDestroy = false);
    void                    DestroyFont(Font *font);
    void                    DestroyUnusedFonts();

    static const char *     defaultFontFilename;
    static Font *           defaultFont;

private:
    struct FontHashKey {
        FontHashKey() {}
        FontHashKey(const Str &name, int fontSize) : name(name), fontSize(fontSize) {}

        Str                 name;
        int                 fontSize;
    };

    struct FontHashCompare {
        template <typename Type1, typename Type2>
        static bool Compare(const Type1 &lhs, const Type2 &rhs) {
            return (lhs.fontSize == rhs.fontSize && Str::Icmp(lhs.name, rhs.name) == 0);
        }
    };

    struct FontHashGenerator {
        template <typename Type>
        static int Hash(const HashIndex &hasher, const Type &value) {
            return hasher.GenerateHash(value.name, false) & value.fontSize;
        }
    };

    using FontHashMap       = HashMap<FontHashKey, Font *, FontHashCompare, FontHashGenerator>;
    FontHashMap             fontHashMap;
};

extern FontManager          fontManager;

BE_NAMESPACE_END
