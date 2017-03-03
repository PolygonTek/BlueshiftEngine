#include "Precompiled.h"
#include "Render/Render.h"
#include "File/FileSystem.h"
#include "FontFace.h"

BE_NAMESPACE_BEGIN

struct FontFileHeader {
    int         ofsBitmaps;     // byte offset of bitmaps
    int         numBitmaps;     // a number of bitmaps
    int         ofsGlyphs;      // byte offset of glyphs
    int         numGlyphs;      // a number of glyphs
};

struct FontFileBitmap {
    char        name[MaxRelativePath];
};

struct FontFileGlyph {
    int         charCode;
    int         x, y;
    int         width, height;
    float       s, t, s2, t2;
    int         bitmapIndex;
};

void FontFaceBitmap::Purge() {
    for (int i = 0; i < glyphHashMap.Count(); i++) {
        const auto *entry = glyphHashMap.GetByIndex(i);
        FontGlyph *glyph = entry->second;

        materialManager.ReleaseMaterial(glyph->material);
    }

    glyphHashMap.DeleteContents(true);
}

// .font 파일 로딩
bool FontFaceBitmap::Load(const char *filename, int fontSize) {
    Purge();

    byte *data;
    fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        BE_WARNLOG(L"Couldn't open font %hs\n", filename);
        return false;
    }

    FontFileHeader *header = (FontFileHeader *)data;

    Str *bitmapNames = new Str[header->numBitmaps];

    FontFileBitmap *bitmap = (FontFileBitmap *)(data + header->ofsBitmaps);
    for (int i = 0; i < header->numBitmaps; i++, bitmap++) {
        Str basePath = filename;
        basePath.StripFileName();
        basePath += "/";
        basePath += bitmap->name;
        bitmapNames[i] = basePath;
        bitmapNames[i].SetFileExtension(".tga");
    }

    int maxHeight = 0;

    FontFileGlyph *gl = (FontFileGlyph *)(data + header->ofsGlyphs);
    for (int i = 0; i < header->numGlyphs; i++, gl++) {
        FontGlyph *glyph = new FontGlyph;
        glyph->charCode     = gl->charCode;
        glyph->width        = gl->width;
        glyph->height       = gl->height;
        glyph->bearingX     = 0;
        glyph->bearingY     = 0;
        glyph->advance      = gl->width;
        glyph->s            = gl->s;
        glyph->t            = gl->t;
        glyph->s2           = gl->s2;
        glyph->t2           = gl->t2;

        Texture *texture = textureManager.GetTexture(bitmapNames[gl->bitmapIndex], Texture::Clamp | Texture::HighQuality | Texture::NoMipmaps);
        glyph->material = materialManager.GetTextureMaterial(texture, Material::OverlayHint);
        textureManager.ReleaseTexture(texture);

        if (gl->height > maxHeight) {
            maxHeight = gl->height;
        }

        glyphHashMap.Set(gl->charCode, glyph);
    }

    delete [] bitmapNames;

    fileSystem.FreeFile(data);

    this->fontHeight = maxHeight;

    return true;
}

int FontFaceBitmap::GetFontHeight() const {
    return fontHeight;
}

FontGlyph *FontFaceBitmap::GetGlyph(int charCode) {
    const auto *entry = glyphHashMap.Get(charCode);
    if (entry) {
        return entry->second;
    }

    return nullptr;
}

int FontFaceBitmap::GetGlyphAdvance(int charCode) const {
    const auto *entry = glyphHashMap.Get(charCode);
    if (entry) {
        return entry->second->advance;
    }

    return 0;
}

BE_NAMESPACE_END
