#pragma once

#include "freetype/include/ft2build.h"
#include "freetype/include/freetype.h"

BE_NAMESPACE_BEGIN

class Material;

class FontFace {
public:
    virtual ~FontFace() {}

    virtual int             GetFontHeight() const = 0;

    virtual FontGlyph *     GetGlyph(int charCode) = 0;

    // 다음 문자를 위한 charCode 문자 간격
    virtual int             GetGlyphAdvance(int charCode) const = 0;

    virtual bool            Load(const char *filename, int fontSize) = 0;

protected:
    // 2 byte 문자 코드에 대한 해시키 생성
    struct HashGeneratorCharCode {
        template <typename Type>
        static int Hash(const HashIndex &hasher, const Type &value) {
            return (((value >> 8) & 0xFF) * 119 + (value & 0xFF)) & (hasher.GetHashSize() - 1);
        }
    };

    using GlyphHashMap      = HashMap<int, FontGlyph *, HashCompareDefault, HashGeneratorCharCode>;
    GlyphHashMap            glyphHashMap;
};

class FontFaceBitmap : public FontFace {
public:
    FontFaceBitmap();
    virtual ~FontFaceBitmap();

    virtual int             GetFontHeight() const;

    virtual FontGlyph *     GetGlyph(int charcode);
    virtual int             GetGlyphAdvance(int charcode) const;

    virtual bool            Load(const char *filename, int fontSize);

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

    virtual int             GetFontHeight() const;

    virtual FontGlyph *     GetGlyph(int charcode);
    virtual int             GetGlyphAdvance(int charcode) const;

    virtual bool            Load(const char *filename, int fontSize);

                            // FreeType libary 초기화/종료 함수
    static void             Init();
    static void             Shutdown();
    
private:
    void                    Purge();

    bool                    LoadFTGlyph(unsigned short charcode) const;
    void                    DrawGlyphBufferFromFTBitmap(const FT_Bitmap *bitmap) const;	

    int                     faceIndex;
    int                     fontHeight;

    byte *                  ftFontFileData;             // FreeType font flie data
    FT_Face                 ftFace;
    mutable unsigned short  ftLastLoadedCharCode;
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
