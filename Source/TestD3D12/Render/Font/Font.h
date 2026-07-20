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

/*
---------------------------------------------------------------------------------------------------

    Font

---------------------------------------------------------------------------------------------------
*/

class FontManager;
class Material;
class FontFace;
struct GlyphAtlas;

// Font glyph information.
struct FontGlyph {
    char32_t                charCode;
    int32_t                 width, height;
    int32_t                 offsetX, offsetY;
    int32_t                 advanceX, advanceY;
    float                   s, t, s2, t2;
    SharedPtr<Material>     material;
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
    ~Font();

    const char *            GetName() const { return name.c_str(); }
    const char *            GetHashName() const { return hashName.c_str(); }

    uint64_t                GetRetainedFrameSerial() const { return retainedFrameSerial; }
    void                    SetRetainedFrameSerial(uint64_t frameSerial) { retainedFrameSerial = frameSerial; }

                            /// Returns font type.
    Type                    GetFontType() const { return fontType; }

                            /// Returns pointer to the glyph structure corresponding to a character. Return nullptr if no glyphs are found.
    FontGlyph *             GetGlyph(char32_t unicodeChar, RenderMode renderMode = RenderMode::Normal) const;

                            /// Returns an offset for the next character.
    int                     GetGlyphAdvanceX(char32_t unicodeChar) const;
    int                     GetGlyphAdvanceY(char32_t unicodeChar) const;

                            /// Returns font height in pixels.
    int                     GetFontHeight() const;

                            /// Calculates text width.
    float                   TextWidth(const BE1::Str &text, int maxLen, bool allowLineBreak = false, bool allowColoredText = false, float xScale = 1.0f, float charSpacing = 0.0f) const;

    void                    Purge();

private:
    bool                    Load(const char *filename);

    BE1::Str                hashName;
    BE1::Str                name;

    Type                    fontType = Type::None;
    uint32_t                fontSize = 0;
    uint64_t                retainedFrameSerial = 0;
    UniquePtr<FontFace>     fontFace;
};
