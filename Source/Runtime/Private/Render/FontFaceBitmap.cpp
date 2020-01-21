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
#include "Render/FontFile.h"
#include "IO/FileSystem.h"
#include "FontFace.h"

BE_NAMESPACE_BEGIN

FontFaceBitmap::FontFaceBitmap() {
}

FontFaceBitmap::~FontFaceBitmap() {
    Purge();
}

void FontFaceBitmap::Purge() {
    for (int i = 0; i < materialArray.Count(); i++) {
        materialManager.ReleaseMaterial(materialArray[i]);
    }
    materialArray.Clear();

    for (int i = 0; i < glyphHashMap.Count(); i++) {
        const auto *entry = glyphHashMap.GetByIndex(i);
        FontGlyph *glyph = entry->second;

        materialManager.ReleaseMaterial(glyph->material);
    }
    glyphHashMap.DeleteContents(true);
}

Texture *FontFaceBitmap::AddBitmap(const char *filename) {
    Texture *texture = textureManager.GetTextureWithoutTextureInfo(filename, Texture::Flag::Clamp | Texture::Flag::HighQuality | Texture::Flag::NoMipmaps);

    Material *material = materialManager.GetSingleTextureMaterial(texture, Material::TextureHint::Overlay);

    textureManager.ReleaseTexture(texture);

    materialArray.Append(material);

    return texture;
}

FontGlyph *FontFaceBitmap::AddGlyph(char32_t charCode, int width, int height, int offsetX, int offsetY, int advanceX, int advanceY, float s, float t, float s2, float t2, int materialIndex) {
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

    glyph->material = materialArray[materialIndex];

    glyphHashMap.Set(glyph->charCode, glyph);

    return glyph;
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

    bitmapNames.SetCount(header->numBitmaps);

    for (int bitmapIndex = 0; bitmapIndex < header->numBitmaps; bitmapIndex++, bitmap++) {
        bitmapNames[bitmapIndex] = bitmap->name;

        Str basePath = filename;
        basePath.StripFileName();
        basePath.AppendPath(bitmap->name);

        bitmapFilenames[bitmapIndex] = basePath;
        bitmapFilenames[bitmapIndex].SetFileExtension(".tga");

        AddBitmap(bitmapFilenames[bitmapIndex]);
    }

    int maxHeight = 0;

    glyphHashMap.Init(header->numGlyphs, header->numGlyphs, 0);

    const FontFileGlyph *gl = (FontFileGlyph *)(data + header->ofsGlyphs);

    for (int glyphIndex = 0; glyphIndex < header->numGlyphs; glyphIndex++, gl++) {
        FontGlyph *glyph = AddGlyph(gl->charCode, gl->width, gl->height, gl->offsetX, gl->offsetY, gl->advanceX, gl->advanceY, gl->s, gl->t, gl->s2, gl->t2, gl->bitmapIndex);
        
        if (glyph->height > maxHeight) {
            maxHeight = glyph->height;
        }
    }

    fileSystem.FreeFile(data);

    this->fontHeight = maxHeight;

    return true;
}

void FontFaceBitmap::Write(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("FontFileBitmap::Write: file open error\n");
        return;
    }

    FontFileHeader header;
    header.numBitmaps = materialArray.Count();
    header.numGlyphs = glyphHashMap.Count();
    header.ofsBitmaps = sizeof(header);
    header.ofsGlyphs = header.ofsBitmaps + header.numBitmaps * sizeof(FontFileBitmap);

    fp->Write(&header, sizeof(header));

    for (int bitmapIndex = 0; bitmapIndex < materialArray.Count(); bitmapIndex++) {
        FontFileBitmap bitmap;
        Str::Copynz(bitmap.name, bitmapNames[bitmapIndex].c_str(), sizeof(bitmap.name));

        fp->Write(&bitmap, sizeof(bitmap));
    }

    for (int glyphIndex = 0; glyphIndex < glyphHashMap.Count(); glyphIndex++) {
        const auto *entry = glyphHashMap.GetByIndex(glyphIndex);
        const FontGlyph *gl = entry->second;
        
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
        glyph.bitmapIndex   = materialArray.FindIndex(gl->material);

        fp->Write(&glyph, sizeof(glyph));
    }

    fileSystem.CloseFile(fp);
}

int FontFaceBitmap::GetFontHeight() const {
    return fontHeight;
}

FontGlyph *FontFaceBitmap::GetGlyph(char32_t unicodeChar, Font::RenderMode::Enum renderMode) {
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
