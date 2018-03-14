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
#include "Simd/Simd.h"

// FreeType2 reference:
// http://freetype.sourceforge.net/freetype2/docs/reference/ft2-base_interface.html

BE_NAMESPACE_BEGIN

// NOTE: LCD 모드로 렌더링이 안되는 font 도 존재하기 때문에,
// 나중에 RGBA 텍스쳐로 통합해서, 되는 폰트만 LCD 로 하는 편이 나을 수도 있겠다
//#define LCD_MODE_RENDERING
#ifdef LCD_MODE_RENDERING
#define GLYPH_CACHE_TEXTURE_FORMAT  Image::RGBA_8_8_8_8
#else
#define GLYPH_CACHE_TEXTURE_FORMAT  Image::LA_8_8
#endif

#define GLYPH_CACHE_TEXTURE_SIZE    1024
#define GLYPH_CACHE_TEXTURE_COUNT   1
#define GLYPH_BORDER_PIXELS         2

// glyph atlas texture 의 사용중인 공간을 덩어리 단위로 표현
struct Chunk {
    int width;
    int height;
};

struct GlyphAtlas {
    Texture *       texture;
    Array<Chunk>    chunks;
};

static Array<GlyphAtlas *>  atlasArray;
static FT_Library           ftLibrary;

void FontFaceFreeType::Init() {
    // initialize FreeType library
    if (FT_Init_FreeType(&ftLibrary) != 0) {
        BE_FATALERROR(L"FT_Init_FreeType() failed");
    }

    atlasArray.Resize(GLYPH_CACHE_TEXTURE_COUNT);

    Image image;
    image.Create2D(GLYPH_CACHE_TEXTURE_SIZE, GLYPH_CACHE_TEXTURE_SIZE, 1, GLYPH_CACHE_TEXTURE_FORMAT, nullptr, 0);
    memset(image.GetPixels(), 0, image.GetSize());

    for (int i = 0; i < GLYPH_CACHE_TEXTURE_COUNT; i++) {
        GlyphAtlas *atlas = new GlyphAtlas;
        atlasArray.Append(atlas);
        // 대략 8x8 조각의 glyph 들을 하나의 텍스쳐에 packing 했을 경우 개수 만큼 할당..
        atlas->chunks.Resize(GLYPH_CACHE_TEXTURE_SIZE * GLYPH_CACHE_TEXTURE_SIZE / 64);
        atlas->texture = textureManager.AllocTexture(va("_glyph_cache_%i", i));
        atlas->texture->Create(RHI::Texture2D, image, Texture::Clamp | Texture::HighQuality | Texture::NoMipmaps);
    }
}

void FontFaceFreeType::Shutdown() {
    for (int i = 0; i < atlasArray.Count(); i++) {
        atlasArray[i]->chunks.Clear();
        textureManager.DestroyTexture(atlasArray[i]->texture);
    }

    atlasArray.DeleteContents(true);

    FT_Done_FreeType(ftLibrary);
}

void FontFaceFreeType::Purge() {
    if (ftFace) {
        // FT_Done_Face 이후에 font file data 를 해제 해야 한다
        FT_Done_Face(ftFace);
        fileSystem.FreeFile(ftFontFileData);
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

// 트루 타입 폰트 파일 로딩
bool FontFaceFreeType::Load(const char *filename, int fontSize) {
    Purge();

    byte *data;
    size_t dataSize = fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        BE_WARNLOG(L"Couldn't open FreeType font %hs\n", filename);
        return false;
    }
    
    // Certain font formats allow several font faces to be embedded in a single file.
    // faceIndex tells which face you want to load.
    int faceIndex = 0; // FIXME
    if (FT_New_Memory_Face(ftLibrary, (FT_Byte *)data, dataSize, faceIndex, &ftFace) != 0) {
        BE_ERRLOG(L"FontFaceFreeType::Create: FT_New_Memory_Face failed\n");
        return false;
    }

    // face 개수 몇개인지 출력
    BE_DLOG(L"%i faces embedded in font file '%hs'\n", ftFace->num_faces, filename);

    // unicode charmap 만 사용한다
    if (FT_Select_Charmap(ftFace, FT_ENCODING_UNICODE) != 0) {
        BE_ERRLOG(L"FontFaceFreeType::Create: %hs font file doesn't contain unicode charmap\n", filename);
        return false;
    }

    this->faceIndex = faceIndex;
    this->ftFontFileData = data;
    this->ftLastLoadedCharCode = 0;

    // NOTE: fontSize 는 EM 을 의미한다. 실제 font 의 bitmap size 가 아님
    if (FT_Set_Pixel_Sizes(ftFace, fontSize, fontSize) != 0) {
        BE_ERRLOG(L"FontFaceFreeType::Create: FT_Set_Pixel_Sizes failed\n");
        return false;
    }

    fontHeight = ((ftFace->size->metrics.height + 63) & ~63) >> 6;

    // glyph 을 그리기 위한 임시 버퍼
    // FT_Set_Pixel_Sizes 와는 다르게 fontHeight * fontHeight 를 넘어가는 비트맵이 나올수도 있어서 넉넉하게 가로 세로 두배씩 더 할당
    glyphBuffer = (byte *)Mem_Alloc16(Image::BytesPerPixel(GLYPH_CACHE_TEXTURE_FORMAT) * fontSize * fontSize * 4);

    return true;
}

int FontFaceFreeType::GetFontHeight() const {
    return fontHeight;
}

// glyph bitmap 을 얻기 위해 glyph slot 에 glyph 을 로드
bool FontFaceFreeType::LoadFTGlyph(unsigned short charCode) const {
    if (ftLastLoadedCharCode != charCode) {
        unsigned int glyph_index = FT_Get_Char_Index(ftFace, charCode);
        if (glyph_index == 0) {
            // charCode 에 맞는 glyph image 가 존재하지 않는다.
            return false;
        }

        // glyph slot 에 glyph 을 로드
        if (FT_Load_Glyph(ftFace, glyph_index, FT_LOAD_TARGET_LIGHT | FT_LOAD_NO_BITMAP) != 0) {
            return false;
        }

        ftLastLoadedCharCode = charCode;
    }

    return true;
}

// FT_Bitmap 으로 부터 glyphBuffer 에 비트맵 데이터를 그린다.
void FontFaceFreeType::DrawGlyphBufferFromFTBitmap(const FT_Bitmap *bitmap) const {
    int     offset;
    int     x, y;
    int     red, green, blue;
    int     b;
        
    // 0 으로 채운다
#ifdef LCD_MODE_RENDERING
    int w = bitmap->width / 3 + GLYPH_BORDER_PIXELS * 2;
    int h = bitmap->rows + GLYPH_BORDER_PIXELS * 2;

    uint32_t *ptr = (uint32_t *)glyphBuffer;
#else
    int w = bitmap->width + GLYPH_BORDER_PIXELS * 2;
    int h = bitmap->rows + GLYPH_BORDER_PIXELS * 2;

    byte *ptr = glyphBuffer;
#endif

    simdProcessor->Memset(ptr, 0, w * h * Image::BytesPerPixel(GLYPH_CACHE_TEXTURE_FORMAT));

    // 여기서 부터 FreeType bitmap 그리기
    byte *buffer_ptr = bitmap->buffer;
    
    switch (bitmap->pixel_mode) {
    case FT_PIXEL_MODE_MONO:
        for (y = 0; y < bitmap->rows; y++) {
            for (x = 0, b = 0; x < bitmap->width; x++, b++) {
                offset = w * (GLYPH_BORDER_PIXELS + y) + GLYPH_BORDER_PIXELS + x;

                glyphBuffer[offset * 2] = 255;
                if (buffer_ptr[b >> 3] & (0x80 >> (b & 7))) {
                    glyphBuffer[offset * 2 + 1] = 255;
                } else {
                    glyphBuffer[offset * 2 + 1] = 0;
                }
            }

            buffer_ptr += bitmap->pitch;
        }
        break;
    case FT_PIXEL_MODE_GRAY:
        for (y = 0; y < bitmap->rows; y++) {
            for (x = 0; x < bitmap->width; x++) {
                offset = w * (GLYPH_BORDER_PIXELS + y) + GLYPH_BORDER_PIXELS + x;
                glyphBuffer[offset * 2] = 255;
                glyphBuffer[offset * 2 + 1] = buffer_ptr[x];
            }

            buffer_ptr += bitmap->pitch;
        }
        break;
    case FT_PIXEL_MODE_LCD:
        for (y = 0; y < bitmap->rows; y++) {
            for (x = 0; x < bitmap->width / 3; x++) {
                offset  = (w * (GLYPH_BORDER_PIXELS + y) + GLYPH_BORDER_PIXELS + x) * 4;
                red     = buffer_ptr[x * 3 + 0];
                green   = buffer_ptr[x * 3 + 1];
                blue    = buffer_ptr[x * 3 + 2];

                glyphBuffer[offset + 0] = red;
                glyphBuffer[offset + 1] = green;
                glyphBuffer[offset + 2] = blue;
                glyphBuffer[offset + 3] = (red + green + blue) / 3;
            }

            buffer_ptr += bitmap->pitch;
        }
        break;
    default:
        assert(0);
        break;
    }
}

static Texture *AllocGlyphTexture(int width, int height, int *x, int *y) {
    int fitChunkYOffset;

    if (width == 0 || height == 0) {
        return nullptr;
    }
    
    for (int i = 0; i < GLYPH_CACHE_TEXTURE_COUNT; i++) {
        GlyphAtlas *atlas = atlasArray[i];

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

                    return atlas->texture;
                } else if (chunk->height > height) {
                    if (chunk->height < fitSpaceHeight) {
                        fitChunk = chunk;
                        fitChunkYOffset = yoffset;
                    }
                }
            }

            yoffset += chunk->height;
        }

        // 남는 공간이 있다면 새로운 청크 생성
        if (GLYPH_CACHE_TEXTURE_SIZE - yoffset >= height) {
            *x = 0;
            *y = yoffset;

            Chunk chunk;
            chunk.width = width;
            chunk.height = height;
            atlas->chunks.Append(chunk);
        
            return atlas->texture;
        } else {
            if (fitChunk) {
                *x = fitChunk->width;
                *y = fitChunkYOffset;

                fitChunk->width += width;

                return atlas->texture;
            }
        }
    }

    BE_WARNLOG(L"not enough texture chunk for cache-able glyph\n");
    return nullptr;
}

// 문자코드에 따른 glyph 을 texture 에 캐싱
FontGlyph *FontFaceFreeType::GetGlyph(int charCode) {
    const auto *entry = glyphHashMap.Get(charCode);
    if (entry) {
        return entry->second;
    }

    if (!LoadFTGlyph(charCode)) {
        return nullptr;
    }

#ifdef LCD_MODE_RENDERING
    // FT_RENDER_MODE_NORMAL: LCD sub-pixel RGB anti-aliasing mode
    if (FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_LCD) != 0) {
#else
    // FT_RENDER_MODE_NORMAL: normal 8bit anti-aliasing mode
    if (FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_NORMAL) != 0) {
#endif
        return nullptr;
    }

    FT_GlyphSlot glyphSlot = ftFace->glyph;
    FT_Bitmap *bitmap = &glyphSlot->bitmap;

#ifdef LCD_MODE_RENDERING
    int width = bitmap->width / 3;
#else
    int width = bitmap->width;
#endif
    int height = bitmap->rows;

    int x, y;
    Texture *texture = AllocGlyphTexture(width + GLYPH_BORDER_PIXELS * 2, height + GLYPH_BORDER_PIXELS * 2, &x, &y);
    if (!texture) {
        return nullptr;
    }

    DrawGlyphBufferFromFTBitmap(bitmap);

    rhi.SelectTextureUnit(0);

    texture->Bind();
    texture->Update2D(x, y, width + GLYPH_BORDER_PIXELS * 2, bitmap->rows + GLYPH_BORDER_PIXELS * 2, GLYPH_CACHE_TEXTURE_FORMAT, glyphBuffer);

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
        ascender = int(ftFace->size->metrics.ascender * 1.0f / 64.0f);
    }

    FontGlyph *glyph = new FontGlyph;
    glyph->charCode     = charCode;
    glyph->width        = width;
    glyph->height       = bitmap->rows;
    glyph->bearingX     = glyphSlot->bitmap_left;
    glyph->bearingY     = ascender - glyphSlot->bitmap_top;
    glyph->advance      = (int)glyphSlot->advance.x >> 6;
    glyph->s            = (float)(x + GLYPH_BORDER_PIXELS) / texture->GetWidth();
    glyph->t            = (float)(y + GLYPH_BORDER_PIXELS) / texture->GetHeight();
    glyph->s2           = (float)(x + GLYPH_BORDER_PIXELS + width) / texture->GetWidth();
    glyph->t2           = (float)(y + GLYPH_BORDER_PIXELS + bitmap->rows) / texture->GetHeight();

    glyph->material = materialManager.GetSingleTextureMaterial(texture, Material::OverlayHint);
                
    glyphHashMap.Set(charCode, glyph);
                
    return glyph;
}

int FontFaceFreeType::GetGlyphAdvance(int charCode) const {
    // glyph 캐시에 있다면 미리 구한 advance 를 리턴
    const auto *entry = glyphHashMap.Get(charCode);
    if (entry) {
        return entry->second->advance;
    }

    // glyph 캐시에 없다면 advance 를 계산하기 위해 glyph 을 로드
    if (LoadFTGlyph(charCode)) {
        return (int)ftFace->glyph->advance.x >> 6;
    }

    return 0;
}

BE_NAMESPACE_END
