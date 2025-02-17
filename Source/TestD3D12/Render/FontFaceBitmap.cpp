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

#include "Precompiled.h"
#include "FontFile.h"
#include "FontFace.h"
#include "Texture.h"

void BitmapFontFace::Purge() {
    for (Texture *texture : textures) {
        textureManager.ReleaseTexture(texture);
    }
    textures.Clear();

    for (const auto &entry : glyphHashMap) {
        FontGlyph *glyph = entry.second;

        textureManager.ReleaseTexture(glyph->texture);
    }
    glyphHashMap.DeleteContents(true);
}

Texture *BitmapFontFace::AddBitmap(const char *filename) {
    Texture *texture = textureManager.GetTexture(filename, Texture::Flag::HighQuality | Texture::Flag::NoMipmaps);
    textures.Append(texture);

    return texture;
}

FontGlyph *BitmapFontFace::AddGlyph(char32_t charCode, int width, int height, int offsetX, int offsetY, int advanceX, int advanceY, float s, float t, float s2, float t2, int materialIndex) {
    FontGlyph *glyph = new FontGlyph;

    glyph->charCode = charCode;
    glyph->width = width;
    glyph->height = height;
    glyph->offsetX = offsetX;
    glyph->offsetY = offsetY;
    glyph->advanceX = advanceX;
    glyph->advanceY = advanceY;
    glyph->s = s;
    glyph->t = t;
    glyph->s2 = s2;
    glyph->t2 = t2;
    glyph->texture = textures[materialIndex];

    glyphHashMap.Set(glyph->charCode, glyph);

    return glyph;
}

bool BitmapFontFace::Load(const char *filename, int fontSize) {
    Purge();

    byte *data;
    BE1::fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        BE_WARNLOG("Couldn't open font %s\n", filename);
        return false;
    }

    const FontFileHeader *header = (FontFileHeader *)data;

    BE1::StrArray bitmapFilenames;
    bitmapFilenames.SetCount(header->numBitmaps);

    const FontFileBitmap *bitmap = (FontFileBitmap *)(data + header->ofsBitmaps);

    bitmapNames.SetCount(header->numBitmaps);

    for (int bitmapIndex = 0; bitmapIndex < header->numBitmaps; bitmapIndex++, bitmap++) {
        bitmapNames[bitmapIndex] = bitmap->name;

        BE1::Str basePath = filename;
        basePath.StripFileName();
        basePath.AppendPath(bitmap->name);

        bitmapFilenames[bitmapIndex] = basePath;
        bitmapFilenames[bitmapIndex].SetFileExtension(".tga");

        AddBitmap(bitmapFilenames[bitmapIndex]);
    }

    int maxHeight = 0;

    glyphHashMap.Init(header->numGlyphs, header->numGlyphs, header->numGlyphs);

    const FontFileGlyph *gl = (FontFileGlyph *)(data + header->ofsGlyphs);

    for (int glyphIndex = 0; glyphIndex < header->numGlyphs; glyphIndex++, gl++) {
        FontGlyph *glyph = AddGlyph(gl->charCode, gl->width, gl->height, gl->offsetX, gl->offsetY, gl->advanceX, gl->advanceY, gl->s, gl->t, gl->s2, gl->t2, gl->bitmapIndex);
        
        if (glyph->height > maxHeight) {
            maxHeight = glyph->height;
        }
    }

    BE1::fileSystem.FreeFile(data);

    this->fontHeight = maxHeight;

    return true;
}

void BitmapFontFace::Write(const char *filename) {
    BE1::File *fp = BE1::fileSystem.OpenFile(filename, BE1::File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("FontFileBitmap::Write: file open error\n");
        return;
    }

    FontFileHeader header;
    header.numBitmaps = textures.Count();
    header.numGlyphs = glyphHashMap.Count();
    header.ofsBitmaps = sizeof(header);
    header.ofsGlyphs = header.ofsBitmaps + header.numBitmaps * sizeof(FontFileBitmap);

    fp->Write(&header, sizeof(header));

    for (int bitmapIndex = 0; bitmapIndex < textures.Count(); bitmapIndex++) {
        FontFileBitmap bitmap;
        BE1::Str::Copynz(bitmap.name, bitmapNames[bitmapIndex].c_str(), sizeof(bitmap.name));

        fp->Write(&bitmap, sizeof(bitmap));
    }

    for (const auto &entry : glyphHashMap) {
        const FontGlyph *gl = entry.second;
        
        FontFileGlyph glyph;
        glyph.charCode      = gl->charCode;
        glyph.width         = gl->width;
        glyph.height        = gl->height;
        glyph.offsetX       = gl->offsetX;
        glyph.offsetY       = gl->offsetY;
        glyph.advanceX      = gl->advanceX;
        glyph.advanceY      = gl->advanceY;
        glyph.s             = gl->s;
        glyph.t             = gl->t;
        glyph.s2            = gl->s2;
        glyph.t2            = gl->t2;
        glyph.bitmapIndex   = textures.FindIndex(gl->texture);

        fp->Write(&glyph, sizeof(glyph));
    }

    BE1::fileSystem.CloseFile(fp);
}

FontGlyph *BitmapFontFace::GetGlyph(char32_t unicodeChar, Font::RenderMode renderMode) const {
    const auto *entry = glyphHashMap.Get(unicodeChar);
    if (entry) {
        return entry->second;
    }
    return nullptr;
}

int32_t BitmapFontFace::GetGlyphAdvanceX(char32_t unicodeChar) const {
    const auto *entry = glyphHashMap.Get(unicodeChar);
    if (entry) {
        return entry->second->advanceX;
    }
    return 0;
}

int32_t BitmapFontFace::GetGlyphAdvanceY(char32_t unicodeChar) const {
    const auto *entry = glyphHashMap.Get(unicodeChar);
    if (entry) {
        return entry->second->advanceY;
    }
    return 0;
}
