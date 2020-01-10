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
#include "Core/Heap.h"
#include "File/FileSystem.h"
#include "FontFace.h"
#include "FontFile.h"
#include "Simd/Simd.h"

// FreeType2 reference:
// http://freetype.sourceforge.net/freetype2/docs/reference/ft2-base_interface.html

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
#define GLYPH_CACHE_TEXTURE_COUNT   1
#define GLYPH_PADDING               2
#define GLYPH_COORD_OFFSET          1

Array<FontFaceFreeType::GlyphAtlas *> FontFaceFreeType::atlasArray;
FT_Library                          FontFaceFreeType::ftLibrary;

FontFaceFreeType::~FontFaceFreeType() {
    Purge();
}

void FontFaceFreeType::Init() {
    // Initialize FreeType library.
    if (FT_Init_FreeType(&ftLibrary) != 0) {
        BE_FATALERROR("FT_Init_FreeType() failed");
    }

    atlasArray.Resize(GLYPH_CACHE_TEXTURE_COUNT);

    Image image;
    image.Create2D(GLYPH_CACHE_TEXTURE_SIZE, GLYPH_CACHE_TEXTURE_SIZE, 1, GLYPH_CACHE_TEXTURE_FORMAT, nullptr, 0);
    memset(image.GetPixels(), 0, image.GetSize());

    for (int atlasIndex = 0; atlasIndex < GLYPH_CACHE_TEXTURE_COUNT; atlasIndex++) {
        GlyphAtlas *atlas = new GlyphAtlas;
        atlasArray.Append(atlas);

        atlas->texture = textureManager.AllocTexture(va("_glyph_cache_%i", atlasIndex));
        atlas->texture->Create(RHI::TextureType::Texture2D, image, Texture::Flag::Clamp | Texture::Flag::HighQuality | Texture::Flag::NoMipmaps);

        // 대략 16x16 조각의 glyph 들을 하나의 텍스쳐에 packing 했을 경우 개수 만큼 할당..
        // 2048*2048 / 16*16 = 16384
        atlas->chunks.Resize(GLYPH_CACHE_TEXTURE_SIZE * GLYPH_CACHE_TEXTURE_SIZE / (16*16));
    }
}

void FontFaceFreeType::Shutdown() {
    for (int atlasIndex = 0; atlasIndex < atlasArray.Count(); atlasIndex++) {
        atlasArray[atlasIndex]->chunks.Clear();

        textureManager.DestroyTexture(atlasArray[atlasIndex]->texture);
    }

    atlasArray.DeleteContents(true);

    // Destroy FreeType library object.
    FT_Done_FreeType(ftLibrary);
}

void FontFaceFreeType::Purge() {
    if (ftFace) {
        // Font file data should be released after calling FT_Done_Face().
        FT_Done_Face(ftFace);
        ftFace = nullptr;

        fileSystem.FreeFile(ftFontFileData);
        ftFontFileData = nullptr;
    }
    
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

    size_t dataSize = fileSystem.LoadFile(filename, true, (void **)&ftFontFileData);
    if (!ftFontFileData) {
        BE_WARNLOG("Couldn't open FreeType font %s\n", filename);
        return false;
    }
    
    // Certain font formats allow several font faces to be embedded in a single file.
    // faceIndex tells which face you want to load.
    if (FT_New_Memory_Face(ftLibrary, ftFontFileData, dataSize, ftFaceIndex, &ftFace) != 0) {
        BE_ERRLOG("FontFaceFreeType::Create: FT_New_Memory_Face failed\n");
        fileSystem.FreeFile(ftFontFileData);
        ftFontFileData = nullptr;
        return false;
    }

    // Print how many faces are exist.
    BE_DLOG("%i faces embedded in font file '%s'\n", ftFace->num_faces, filename);

    // We use only unicode charmap.
    if (FT_Select_Charmap(ftFace, FT_ENCODING_UNICODE) != 0) {
        BE_ERRLOG("FontFaceFreeType::Create: %s font file doesn't contain unicode charmap\n", filename);
        FT_Done_Face(ftFace);
        ftFace = nullptr;
        fileSystem.FreeFile(ftFontFileData);
        ftFontFileData = nullptr;
        return false;
    }

    // NOTE: fontSize means EM. Not the bitmap size of the actual font.
    if (FT_Set_Pixel_Sizes(ftFace, fontSize, fontSize) != 0) {
        BE_ERRLOG("FontFaceFreeType::Create: FT_Set_Pixel_Sizes failed\n");
        FT_Done_Face(ftFace);
        ftFace = nullptr;
        fileSystem.FreeFile(ftFontFileData);
        ftFontFileData = nullptr;
        return false;
    }

    // Calcualte font height in pixels.
    fontHeight = ((ftFace->size->metrics.height + 63) & ~63) >> 6;

    // Allocate temporary buffer for drawing glyphs.
    // FT_Set_Pixel_Sizes 와는 다르게 fontHeight * fontHeight 를 넘어가는 비트맵이 나올수도 있어서 넉넉하게 가로 세로 두배씩 더 할당
    glyphBuffer = (byte *)Mem_Alloc16(Image::BytesPerPixel(GLYPH_CACHE_TEXTURE_FORMAT) * fontSize * fontSize * 4);

    // Pad with zeros.
    simdProcessor->Memset(glyphBuffer, 0, fontSize * fontSize * Image::BytesPerPixel(GLYPH_CACHE_TEXTURE_FORMAT));

    lastLoadedChar = 0;

    return true;
}

// Load glyph into glyph slot to get the bitmap.
bool FontFaceFreeType::LoadFTGlyph(char32_t unicodeChar) const {
    if (lastLoadedChar != unicodeChar) {
        unsigned int glyphIndex = FT_Get_Char_Index(ftFace, unicodeChar);

        if (glyphIndex == 0) {
            // There is no glyph image for this character.
            return false;
        }

        // Load glyph into glyph slot.
        if (FT_Load_Glyph(ftFace, glyphIndex, FT_LOAD_TARGET_NORMAL | FT_LOAD_NO_BITMAP) != 0) {
            return false;
        }

        lastLoadedChar = unicodeChar;
    }

    return true;
}

// FT_Bitmap 으로 부터 glyphBuffer 에 비트맵 데이터를 그린다.
void FontFaceFreeType::CopyFTBitmapToGlyphBuffer(const FT_Bitmap *bitmap) const {        
#ifdef LCD_MODE_RENDERING
    int w = bitmap->width / 3;
    int h = bitmap->rows;
#else
    int w = bitmap->width;
    int h = bitmap->rows;
#endif

    // Draw FreeType bitmap from here.
    const byte *bufferPtr = bitmap->buffer;

    switch (bitmap->pixel_mode) {
    case FT_PIXEL_MODE_MONO:
        for (int y = 0; y < bitmap->rows; y++) {
            for (int x = 0, b = 0; x < bitmap->width; x++, b++) {
                int offset = w * y + x;

                if (bufferPtr[b >> 3] & (0b1000 >> (b & 0b0111))) {
                    glyphBuffer[offset] = 255;
                } else {
                    glyphBuffer[offset] = 0;
                }
            }
            bufferPtr += bitmap->pitch;
        }
        break;
    case FT_PIXEL_MODE_GRAY:
        for (int y = 0; y < bitmap->rows; y++) {
            for (int x = 0; x < bitmap->width; x++) {
                int offset = w * y + x;

                glyphBuffer[offset] = bufferPtr[x];
            }
            bufferPtr += bitmap->pitch;
        }
        break;
    case FT_PIXEL_MODE_LCD:
        for (int y = 0; y < bitmap->rows; y++) {
            for (int x = 0; x < bitmap->width / 3; x++) {
                int offset = (w * y + x) << 2;

                int r = bufferPtr[x * 3 + 0];
                int g = bufferPtr[x * 3 + 1];
                int b = bufferPtr[x * 3 + 2];

                glyphBuffer[offset + 0] = r;
                glyphBuffer[offset + 1] = g;
                glyphBuffer[offset + 2] = b;
                glyphBuffer[offset + 3] = (r + g + b) / 3;
            }
            bufferPtr += bitmap->pitch;
        }
        break;
    default:
        assert(0);
        break;
    }
}

int FontFaceFreeType::AllocGlyphAtlas(int width, int height, int *x, int *y) {
    int fitChunkYOffset;

    if (width == 0 || height == 0) {
        return -1;
    }
    
    for (int atlasIndex = 0; atlasIndex < GLYPH_CACHE_TEXTURE_COUNT; atlasIndex++) {
        GlyphAtlas *atlas = atlasArray[atlasIndex];

        Chunk *fitChunk = nullptr;
        int fitSpaceHeight = GLYPH_CACHE_TEXTURE_SIZE;
        int yoffset = 0;

        for (int j = 0; j < atlas->chunks.Count(); j++) {
            Chunk *chunk = &atlas->chunks[j];
            
            // 가로로 남는 덩어리가 있는지..
            if (chunk->width + width <= atlas->texture->GetWidth()) {
                // 세로 크기가 같은 덩어리를 발견하면 바로 거기다 끼워놓고 리턴
                if (chunk->height == height) {
                    *x = chunk->width;
                    *y = yoffset;

                    chunk->width += width;

                    return atlasIndex;
                } else if (chunk->height > height) {
                    if (chunk->height < fitSpaceHeight) {
                        fitChunk = chunk;
                        fitChunkYOffset = yoffset;
                    }
                }
            }

            yoffset += chunk->height;
        }

        // Create a new chunk if there is space left.
        if (GLYPH_CACHE_TEXTURE_SIZE - yoffset >= height) {
            *x = 0;
            *y = yoffset;

            Chunk chunk;
            chunk.width = width;
            chunk.height = height;
            atlas->chunks.Append(chunk);
        
            return atlasIndex;
        } else {
            if (fitChunk) {
                *x = fitChunk->width;
                *y = fitChunkYOffset;

                fitChunk->width += width;

                return atlasIndex;
            }
        }
    }

    BE_WARNLOG("not enough atlas chunk for cache-able glyph\n");
    return -1;
}

// Caching glyphs in the texture with the given character code.
FontGlyph *FontFaceFreeType::GetGlyph(char32_t unicodeChar) {
    const auto *entry = glyphHashMap.Get(unicodeChar);
    if (entry) {
        return entry->second;
    }

    if (!LoadFTGlyph(unicodeChar)) {
        return nullptr;
    }

#ifdef LCD_MODE_RENDERING
    // FT_RENDER_MODE_NORMAL: LCD sub-pixel RGB anti-aliasing mode.
    if (FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_LCD) != 0) {
#else
    // FT_RENDER_MODE_NORMAL: normal 8bit anti-aliasing mode.
    if (FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_NORMAL) != 0) {
#endif
        return nullptr;
    }

    const FT_GlyphSlot glyphSlot = ftFace->glyph;
    const FT_Bitmap *bitmap = &glyphSlot->bitmap;

#ifdef LCD_MODE_RENDERING
    int width = bitmap->width / 3;
#else
    int width = bitmap->width;
#endif
    int height = bitmap->rows;

    int allocWidth = width + (GLYPH_PADDING << 1);
    int allocHeight = height + (GLYPH_PADDING << 1);

    int x, y;
    int atlasIndex = AllocGlyphAtlas(allocWidth, allocHeight, &x, &y);
    Texture *texture = atlasArray[atlasIndex]->texture;
    if (!texture) {
        return nullptr;
    }

    CopyFTBitmapToGlyphBuffer(&ftFace->glyph->bitmap);

    rhi.SelectTextureUnit(0);

    texture->Bind();
    texture->Update2D(0, x + GLYPH_PADDING, y + GLYPH_PADDING, width, height, GLYPH_CACHE_TEXTURE_FORMAT, glyphBuffer);

    // NOTE: ascender 의 의미가 폰트 포맷마다 해석이 좀 다양하다
    // (base line 에서부터 위쪽으로 top bearing 을 포함해서 그 위쪽까지의 거리가 필요함)
    // The ascender is the vertical distance from the horizontal baseline to 
    // the highest ‘character’ coordinate in a font face. 
    // Unfortunately, font formats define the ascender differently. For some, 
    // it represents the ascent of all capital latin characters (without accents), 
    // for others it is the ascent of the highest accented character, and finally, 
    // other formats define it as being equal to global_bbox.yMax.
    int ascender;
    if (FT_IS_SCALABLE(ftFace)) {
        ascender = (int)FT_MulFix(ftFace->ascender, ftFace->size->metrics.y_scale);
        ascender = ((ascender + 63) & ~63) >> 6;
    } else {
        ascender = int(ftFace->size->metrics.ascender / 64.0f);
    }

    FontGlyph *glyph = new FontGlyph;
    glyph->charCode     = unicodeChar;
    glyph->width        = width;
    glyph->height       = height;
    glyph->offsetX      = glyphSlot->bitmap_left - GLYPH_COORD_OFFSET;
    glyph->offsetY      = ascender - glyphSlot->bitmap_top - GLYPH_COORD_OFFSET;
    glyph->advance      = (((int)glyphSlot->advance.x) >> 6) - (GLYPH_COORD_OFFSET << 1);
    glyph->s            = (float)(x + (GLYPH_PADDING - GLYPH_COORD_OFFSET)) / texture->GetWidth();
    glyph->t            = (float)(y + (GLYPH_PADDING - GLYPH_COORD_OFFSET)) / texture->GetHeight();
    glyph->s2           = (float)(x + allocWidth - (GLYPH_PADDING - GLYPH_COORD_OFFSET)) / texture->GetWidth();
    glyph->t2           = (float)(y + allocHeight - (GLYPH_PADDING - GLYPH_COORD_OFFSET)) / texture->GetHeight();

    glyph->material = materialManager.GetSingleTextureMaterial(texture, Material::TextureHint::Overlay);

    glyphHashMap.Set(unicodeChar, glyph);

    return glyph;
}

int FontFaceFreeType::GetGlyphAdvance(char32_t unicodeChar) const {
    // Return previously obtained advance if it is in the glyph cache.
    const auto *entry = glyphHashMap.Get(unicodeChar);
    if (entry) {
        return entry->second->advance;
    }

    // If glyph is not in cache, load glyph to compute advance.
    if (LoadFTGlyph(unicodeChar)) {
        return (((int)ftFace->glyph->advance.x) >> 6) - (GLYPH_COORD_OFFSET << 1);
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
        fp->WriteInt32(glyph->advance);
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
