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

#include "Precompiled.h"
#include "Render/Render.h"
#include "File/FileSystem.h"
#include "FontFace.h"
#include "FontFile.h"

BE_NAMESPACE_BEGIN

FontFaceBitmap::FontFaceBitmap() {
}

FontFaceBitmap::~FontFaceBitmap() {
    Purge();
}

void FontFaceBitmap::Purge() {
    for (int i = 0; i < glyphHashMap.Count(); i++) {
        const auto *entry = glyphHashMap.GetByIndex(i);
        FontGlyph *glyph = entry->second;

        materialManager.ReleaseMaterial(glyph->material);
    }

    glyphHashMap.DeleteContents(true);
}

bool FontFaceBitmap::Load(const char *filename, int fontSize) {
    Purge();

    byte *data;
    fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        BE_WARNLOG("Couldn't open font %s\n", filename);
        return false;
    }

    const FontFileHeader *header = (FontFileHeader *)data;

    StrArray bitmapFilenames;
    bitmapFilenames.SetCount(header->numBitmaps);

    const FontFileBitmap *bitmap = (FontFileBitmap *)(data + header->ofsBitmaps);

    for (int i = 0; i < header->numBitmaps; i++, bitmap++) {
        Str basePath = filename;
        basePath.StripFileName();
        basePath.AppendPath(bitmap->name);

        bitmapFilenames[i] = basePath;
        bitmapFilenames[i].SetFileExtension(".tga");
    }

    int maxHeight = 0;

    glyphHashMap.Init(header->numGlyphs, header->numGlyphs, 0);

    const FontFileGlyph *gl = (FontFileGlyph *)(data + header->ofsGlyphs);

    for (int glyphIndex = 0; glyphIndex < header->numGlyphs; glyphIndex++, gl++) {
        FontGlyph *glyph = new FontGlyph;
        glyph->charCode = gl->charCode;
        glyph->width    = gl->width;
        glyph->height   = gl->height;
        glyph->offsetX  = gl->offsetX;
        glyph->offsetY  = gl->offsetY;
        glyph->advanceX = gl->advanceX;
        glyph->advanceY = gl->advanceY;
        glyph->s        = gl->s;
        glyph->t        = gl->t;
        glyph->s2       = gl->s2;
        glyph->t2       = gl->t2;

        Texture *texture = textureManager.GetTextureWithoutTextureInfo(bitmapFilenames[gl->bitmapIndex], Texture::Flag::Clamp | Texture::Flag::HighQuality | Texture::Flag::NoMipmaps);
        glyph->material = materialManager.GetSingleTextureMaterial(texture, Material::TextureHint::Overlay);
        textureManager.ReleaseTexture(texture);

        if (glyph->height > maxHeight) {
            maxHeight = glyph->height;
        }

        glyphHashMap.Set(glyph->charCode, glyph);
    }

    fileSystem.FreeFile(data);

    this->fontHeight = maxHeight;

    return true;
}

int FontFaceBitmap::GetFontHeight() const {
    return fontHeight;
}

FontGlyph *FontFaceBitmap::GetGlyph(char32_t unicodeChar) {
    const auto *entry = glyphHashMap.Get(unicodeChar);
    if (entry) {
        return entry->second;
    }
    return nullptr;
}

int FontFaceBitmap::GetGlyphAdvanceX(char32_t unicodeChar) const {
    const auto *entry = glyphHashMap.Get(unicodeChar);
    if (entry) {
        return entry->second->advanceX;
    }
    return 0;
}

int FontFaceBitmap::GetGlyphAdvanceY(char32_t unicodeChar) const {
    const auto *entry = glyphHashMap.Get(unicodeChar);
    if (entry) {
        return entry->second->advanceY;
    }
    return 0;
}

BE_NAMESPACE_END
