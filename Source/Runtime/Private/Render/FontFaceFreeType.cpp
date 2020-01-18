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
#include "RenderInternal.h"
#include "Simd/Simd.h"
#include "Core/Heap.h"
#include "File/FileSystem.h"
#include "FreeTypeFont.h"
#include "FontFace.h"
#include "FontFile.h"

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

BE_NAMESPACE_BEGIN

// NOTE: LCD 모드로 렌더링이 안되는 font 도 존재하기 때문에,
// 나중에 RGBA 텍스쳐로 통합해서, 되는 폰트만 LCD 로 하는 편이 나을 수도 있겠다
//#define LCD_MODE_RENDERING
#ifdef LCD_MODE_RENDERING
#define GLYPH_CACHE_TEXTURE_FORMAT  Image::Format::RGBA_8_8_8_8
#else
#define GLYPH_CACHE_TEXTURE_FORMAT  Image::Format::A_8
#endif

#define GLYPH_CACHE_TEXTURE_SIZE    2048
#define GLYPH_COORD_OFFSET          1

struct GlyphAtlas {
    stbrp_context                   context;
    Array<stbrp_node>               nodes;
    Texture *                       texture;
};

static Array<GlyphAtlas *>          atlasArray;

static void Atlas_Add() {
    GlyphAtlas *atlas = new GlyphAtlas;
    atlasArray.Append(atlas);

    // 대략 16x16 조각의 glyph 들을 하나의 텍스쳐에 packing 했을 경우 개수 만큼 할당..
    // 2048*2048 / 16*16 = 16384
    atlas->nodes.SetCount(GLYPH_CACHE_TEXTURE_SIZE * GLYPH_CACHE_TEXTURE_SIZE / (16 * 16));
    stbrp_init_target(&atlas->context, GLYPH_CACHE_TEXTURE_SIZE, GLYPH_CACHE_TEXTURE_SIZE, atlas->nodes.Ptr(), atlas->nodes.Count());

    Image image;
    image.Create2D(GLYPH_CACHE_TEXTURE_SIZE, GLYPH_CACHE_TEXTURE_SIZE, 1, GLYPH_CACHE_TEXTURE_FORMAT, nullptr, 0);
    memset(image.GetPixels(), 0, image.GetSize());

    atlas->texture = textureManager.AllocTexture(va("_glyph_cache_%i", atlasArray.Count() - 1));
    atlas->texture->Create(RHI::TextureType::Texture2D, image, Texture::Flag::Clamp | Texture::Flag::HighQuality | Texture::Flag::NoMipmaps);
}

static Texture *Atlas_AddRect(int inWidth, int inHeight, int &outX, int &outY) {
    stbrp_rect rect;
    rect.w = (uint16_t)inWidth;
    rect.h = (uint16_t)inHeight;

    int atlasIndex = 0;
    do {
        if (atlasIndex == atlasArray.Count()) {
            Atlas_Add();
        }
        if (stbrp_pack_rects(&atlasArray[atlasIndex]->context, &rect, 1) != 0) {
            break;
        }
    } while (atlasIndex++);

    outX = rect.x;
    outY = rect.w;

    return atlasArray[atlasIndex]->texture;
}

void FontFaceFreeType::InitAtlas() {
    Atlas_Add();
}

void FontFaceFreeType::FreeAtlas() {
    for (int atlasIndex = 0; atlasIndex < atlasArray.Count(); atlasIndex++) {
        textureManager.DestroyTexture(atlasArray[atlasIndex]->texture);
    }

    atlasArray.DeleteContents(true);
}

void FontFaceFreeType::Purge() {
    SAFE_DELETE(freeTypeFont);

    if (glyphBuffer) {
        Mem_AlignedFree(glyphBuffer);
        glyphBuffer = nullptr;
    }

    for (int i = 0; i < glyphHashMap.Count(); i++) {
        const auto *entry = glyphHashMap.GetByIndex(i);
        FontGlyph *glyph = entry->second;

        materialManager.ReleaseMaterial(glyph->material);
    }

    glyphHashMap.DeleteContents(true);
}

bool FontFaceFreeType::Load(const char *filename, int fontSize) {
    Purge();

    freeTypeFont = new FreeTypeFont;
    if (!freeTypeFont->Create(filename, fontSize)) {
        return false;
    }

    // Calcualte font height in pixels.
    fontHeight = ((freeTypeFont->GetFace()->size->metrics.height + 63) & ~63) >> 6;

    // Allocate temporary buffer for drawing glyphs.
    // FT_Set_Pixel_Sizes 와는 다르게 fontHeight * fontHeight 를 넘어가는 비트맵이 나올수도 있어서 넉넉하게 가로 세로 두배씩 더 할당
    glyphBuffer = (byte *)Mem_Alloc16(Image::BytesPerPixel(GLYPH_CACHE_TEXTURE_FORMAT) * fontSize * fontSize * 4);

    // Pad with zeros.
    simdProcessor->Memset(glyphBuffer, 0, fontSize * fontSize * Image::BytesPerPixel(GLYPH_CACHE_TEXTURE_FORMAT));

    return true;
}

Texture *FontFaceFreeType::RenderGlyphToAtlasTexture(char32_t unicodeChar, int32_t renderMode, int atlasPadding, int &bitmapLeft, int &bitmapTop, int &glyphX, int &glyphY, int &glyphWidth, int &glyphHeight) {
    FT_Glyph ftGlyph = nullptr;
    const FT_Bitmap *ftBitmap;

    if (!freeTypeFont->LoadGlyph(unicodeChar)) {
        return nullptr;
    }

    if (renderMode == 1) {
        ftGlyph = freeTypeFont->RenderGlyphWithBorder(FT_RENDER_MODE_NORMAL, 1);

        const FT_BitmapGlyph ftBitmapGlyph = (FT_BitmapGlyph)ftGlyph;

        ftBitmap = &ftBitmapGlyph->bitmap;

        bitmapLeft = ftBitmapGlyph->left;
        bitmapTop = ftBitmapGlyph->top;
    } else {
        const FT_GlyphSlot ftGlyphSlot = freeTypeFont->RenderGlyph(FT_RENDER_MODE_NORMAL);

        ftBitmap = &ftGlyphSlot->bitmap;

        bitmapLeft = ftGlyphSlot->bitmap_left;
        bitmapTop = ftGlyphSlot->bitmap_top;
    }

    int fxPaddingX = 0;
    int fxPaddingY = 0;

    glyphWidth = ftBitmap->width + (fxPaddingX << 1);
    glyphHeight = ftBitmap->rows + (fxPaddingY << 1);

    freeTypeFont->BakeGlyphBitmap(ftBitmap, fxPaddingX, fxPaddingY, glyphBuffer);

    //Image image = Image(glyphWidth, glyphHeight, 1, 1, 1, Image::Format::A_8, glyphBuffer, 0).MakeSDF(8);
    //memcpy(glyphBuffer, image.GetPixels(), image.GetSize());

    int actualWidth = glyphWidth + (atlasPadding << 1);
    int actualHeight = glyphHeight + (atlasPadding << 1);
    int x, y;

    Texture *texture = Atlas_AddRect(actualWidth, actualHeight, x, y);

    glyphX = x + atlasPadding;
    glyphY = y + atlasPadding;

    if (ftGlyph) {
        FT_Done_Glyph(ftGlyph);
    }

    return texture;
}

FontGlyph *FontFaceFreeType::CacheGlyph(char32_t unicodeChar, int32_t renderMode, int atlasPadding) {
    const auto *entry = glyphHashMap.Get(((int64_t)renderMode << 32) | unicodeChar);
    if (entry) {
        return entry->second;
    }

    int bitmapLeft;
    int bitmapTop;
    int glyphX;
    int glyphY;
    int glyphWidth;
    int glyphHeight;

    Texture *texture = RenderGlyphToAtlasTexture(unicodeChar, renderMode, atlasPadding, bitmapLeft, bitmapTop, glyphX, glyphY, glyphWidth, glyphHeight);
    if (!texture) {
        return nullptr;
    }

    rhi.SelectTextureUnit(0);

    texture->Bind();
    texture->Update2D(0, glyphX, glyphY, glyphWidth, glyphHeight, GLYPH_CACHE_TEXTURE_FORMAT, glyphBuffer);

    // NOTE: ascender 의 의미가 폰트 포맷마다 해석이 좀 다양하다
    // (base line 에서부터 위쪽으로 top bearing 을 포함해서 그 위쪽까지의 거리가 필요함)
    // The ascender is the vertical distance from the horizontal baseline to 
    // the highest ‘character’ coordinate in a font face. 
    // Unfortunately, font formats define the ascender differently. For some, 
    // it represents the ascent of all capital latin characters (without accents), 
    // for others it is the ascent of the highest accented character, and finally, 
    // other formats define it as being equal to global_bbox.yMax.
    int ascender;
    FT_Face ftFace = freeTypeFont->GetFace();

    if (FT_IS_SCALABLE(ftFace)) {
        ascender = (int)FT_MulFix(ftFace->ascender, ftFace->size->metrics.y_scale);
        ascender = ((ascender + 63) & ~63) >> 6;
    } else {
        ascender = int(ftFace->size->metrics.ascender / 64.0f);
    }

    FontGlyph *gl = new FontGlyph;
    gl->charCode    = unicodeChar;
    gl->width       = glyphWidth;
    gl->height      = glyphHeight;
    gl->offsetX     = bitmapLeft - GLYPH_COORD_OFFSET;
    gl->offsetY     = ascender - bitmapTop - GLYPH_COORD_OFFSET;
    gl->advanceX    = (((int)ftFace->glyph->advance.x) >> 6) - (GLYPH_COORD_OFFSET << 1);
    gl->advanceY    = (((int)ftFace->glyph->advance.y) >> 6) - (GLYPH_COORD_OFFSET << 1);
    gl->s           = (float)(glyphX - GLYPH_COORD_OFFSET) / texture->GetWidth();
    gl->t           = (float)(glyphY - GLYPH_COORD_OFFSET) / texture->GetHeight();
    gl->s2          = (float)(glyphX + glyphWidth + GLYPH_COORD_OFFSET) / texture->GetWidth();
    gl->t2          = (float)(glyphY + glyphHeight + GLYPH_COORD_OFFSET) / texture->GetHeight();

    gl->material = materialManager.GetSingleTextureMaterial(texture, Material::TextureHint::Overlay);

    glyphHashMap.Set(unicodeChar, gl);

    return gl;
}

FontGlyph *FontFaceFreeType::GetGlyph(char32_t unicodeChar) {
    return CacheGlyph(unicodeChar, 0, 2);
}

int FontFaceFreeType::GetGlyphAdvanceX(char32_t unicodeChar) const {
    // Return previously obtained advanceY if it is in the glyph cache.
    const auto *entry = glyphHashMap.Get((int64_t)unicodeChar);
    if (entry) {
        return entry->second->advanceX;
    }

    // If glyph is not in cache, load glyph to compute advance.
    if (freeTypeFont->LoadGlyph(unicodeChar)) {
        return (((int)freeTypeFont->GetFace()->glyph->advance.x) >> 6) - (GLYPH_COORD_OFFSET << 1);
    }
    return 0;
}

int FontFaceFreeType::GetGlyphAdvanceY(char32_t unicodeChar) const {
    // Return previously obtained advanceX if it is in the glyph cache.
    const auto *entry = glyphHashMap.Get((int64_t)unicodeChar);
    if (entry) {
        return entry->second->advanceY;
    }

    // If glyph is not in cache, load glyph to compute advance.
    if (freeTypeFont->LoadGlyph(unicodeChar)) {
        return (((int)freeTypeFont->GetFace()->glyph->advance.y) >> 6) - (GLYPH_COORD_OFFSET << 1);
    }
    return 0;
}

bool FontFaceFreeType::Write(const char *filename) {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("FontFaceFreeType::Save: file open error\n");
        return false;
    }

    FontFileHeader header;
    header.ofsBitmaps = sizeof(FontFileHeader);
    header.numBitmaps = 1;
    header.ofsGlyphs = sizeof(FontFileHeader) + sizeof(FontFileBitmap) * header.numBitmaps;
    header.numGlyphs = glyphHashMap.Count();

    fp->Write(&header, sizeof(header));

    FontFileBitmap bitmap;

    Str bitmapName = filename;
    bitmapName.StripFileExtension();

    assert(bitmapName.Length() < sizeof(bitmap.name));

    memset(bitmap.name, 0, sizeof(bitmap.name));
    strcpy(bitmap.name, bitmapName.c_str());

    fp->Write(&bitmap, sizeof(bitmap));

    for (int glyphIndex = 0; glyphIndex < glyphHashMap.Count(); glyphIndex++) {
        const auto *entry = glyphHashMap.GetByIndex(glyphIndex);
        const FontGlyph *glyph = entry->second;

        fp->WriteInt32(glyph->charCode);
        fp->WriteInt32(glyph->width);
        fp->WriteInt32(glyph->height);
        fp->WriteInt32(glyph->offsetX);
        fp->WriteInt32(glyph->offsetY);
        fp->WriteInt32(glyph->advanceX);
        fp->WriteInt32(glyph->advanceY);
        fp->WriteFloat(glyph->s);
        fp->WriteFloat(glyph->t);
        fp->WriteFloat(glyph->s2);
        fp->WriteFloat(glyph->t2);
        fp->WriteInt32(0);
    }

    fileSystem.CloseFile(fp);

    WriteBitmapFiles(filename);

    return true;
}

void FontFaceFreeType::WriteBitmapFiles(const char *fontFilename) {
    Str bitmapBasename = fontFilename;
    bitmapBasename.StripFileExtension();

    for (int i = 0; i < atlasArray.Count(); i++) {
        const Texture *texture = atlasArray[i]->texture;

        Image bitmapImage;
        bitmapImage.Create2D(texture->GetWidth(), texture->GetHeight(), 1, texture->GetFormat(), nullptr, 0);

        texture->Bind();
        texture->GetTexels2D(0, texture->GetFormat(), bitmapImage.GetPixels(0));

        Str filename = bitmapBasename;
        filename += i;
        filename.Append(".png");
        bitmapImage.WritePNG(filename);
    }
}

BE_NAMESPACE_END
