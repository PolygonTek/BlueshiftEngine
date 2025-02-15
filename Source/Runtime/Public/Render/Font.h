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

/*
-------------------------------------------------------------------------------

    Font
    
-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class FontFace;
class Material;

// Font glyph information.
struct FontGlyph {
    char32_t                charCode;
    int32_t                 width, height;
    int32_t                 offsetX, offsetY;
    int32_t                 advanceX, advanceY;
    float                   s, t, s2, t2;
    Material *              material;
};

class Font {
    friend class FontManager;

public:
    enum class Type : uint8_t {
        None,
        Bitmap,
        TrueType
    };

    enum class RenderMode : uint8_t {
        Normal,
        Border
    };

    Font() = default;
    ~Font() { Purge(); }

    const char *            GetName() const { return name; }
    const char *            GetHashName() const { return hashName; }

                            /// Returns font type.
    Type                    GetFontType() const { return fontType; }

                            /// Returns pointer to the glyph structure corresponding to a character. Return nullptr if no glyphs are found.
    FontGlyph *             GetGlyph(char32_t unicodeChar, RenderMode renderMode = RenderMode::Normal);

                            /// Returns a offset for the next character.
    int                     GetGlyphAdvanceX(char32_t unicodeChar) const;
    int                     GetGlyphAdvanceY(char32_t unicodeChar) const;

                            /// Returns font height in pixels.
    int                     GetFontHeight() const;

                            /// Calculates text width.
    float                   TextWidth(const Str &text, int maxLen, bool allowLineBreak = false, bool allowColoredText = false, float xScale = 1.0f) const;

    void                    Purge();
    bool                    Load(const char *filename);

private:
    Str                     hashName;
    Str                     name;
    mutable int32_t         refCount = 0;
    bool                    permanence = false;

    Type                    fontType = Type::None;
    uint32_t                fontSize = 0;
    FontFace *              fontFace = nullptr;
};

class FontManager {
public:
    void                    Init();
    void                    Shutdown();

    Font *                  AllocFont(const char *name, uint32_t fontSize);
    Font *                  FindFont(const char *name, uint32_t fontSize) const;
    Font *                  GetFont(const char *name, uint32_t fontSize);

    void                    ReleaseFont(Font *font, bool immediateDestroy = false);
    void                    DestroyFont(Font *font);
    void                    DestroyUnusedFonts();

    void                    ClearAtlasTextures();

    static const char *     defaultFontFilename;
    static Font *           defaultFont;

private:
    struct FontHashKey {
        FontHashKey() {}
        FontHashKey(const Str &name, uint32_t fontSize) : name(name), fontSize(fontSize) {}

        Str                 name;
        uint32_t            fontSize;
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
            return hasher.GenerateHash(value.name, false) * value.fontSize;
        }
    };

    using FontHashMap       = HashMap<FontHashKey, Font *, FontHashCompare, FontHashGenerator>;
    FontHashMap             fontHashMap;
};

extern FontManager          fontManager;

BE_NAMESPACE_END
