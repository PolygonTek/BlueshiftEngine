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
#include "IO/FileSystem.h"
#include "Render/TrueTypeFont.h"
#include "freetype/ftstroke.h"

BE_NAMESPACE_BEGIN

static FT_Library ftLibrary;

void TrueTypeFont::Init() {
    if (FT_Init_FreeType(&ftLibrary) != 0) {
        BE_FATALERROR("FT_Init_FreeType() failed");
    }
}

void TrueTypeFont::Shutdown() {
    FT_Done_FreeType(ftLibrary);
}

void TrueTypeFont::Purge() {
    if (ftFace) {
        FT_Done_Face(ftFace);
        ftFace = nullptr;
    }

    // Font file data should be released after calling FT_Done_Face().
    if (ftFontFileData) {
        fileSystem.FreeFile(ftFontFileData);
        ftFontFileData = nullptr;
    }
}

bool TrueTypeFont::Load(const char *filename, int fontSize) {
    Purge();

    size_t dataSize = fileSystem.LoadFile(filename, true, (void **)&ftFontFileData);
    if (!ftFontFileData) {
        BE_WARNLOG("Couldn't open FreeType font %s\n", filename);
        return false;
    }

    // Certain font formats allow several font faces to be embedded in a single file.
    // faceIndex tells which face you want to load.
    if (FT_New_Memory_Face(ftLibrary, ftFontFileData, dataSize, ftFaceIndex, &ftFace) != 0) {
        BE_ERRLOG("TrueTypeFont::Load: FT_New_Memory_Face failed\n");
        fileSystem.FreeFile(ftFontFileData);
        ftFontFileData = nullptr;
        return false;
    }

    // Print how many faces are exist.
    BE_DLOG("%i faces embedded in font file '%s'\n", ftFace->num_faces, filename);

    // We use only unicode charmap.
    if (FT_Select_Charmap(ftFace, FT_ENCODING_UNICODE) != 0) {
        BE_ERRLOG("TrueTypeFont::Load: %s font file doesn't contain unicode charmap\n", filename);
        FT_Done_Face(ftFace);
        ftFace = nullptr;
        fileSystem.FreeFile(ftFontFileData);
        ftFontFileData = nullptr;
        return false;
    }

    // NOTE: fontSize means EM. Not the bitmap size of the actual font.
    if (FT_Set_Pixel_Sizes(ftFace, fontSize, fontSize) != 0) {
        BE_ERRLOG("TrueTypeFont::Load: FT_Set_Pixel_Sizes failed\n");
        FT_Done_Face(ftFace);
        ftFace = nullptr;
        fileSystem.FreeFile(ftFontFileData);
        ftFontFileData = nullptr;
        return false;
    }

    return true;
}

// Load glyph into glyph slot to get the bitmap.
bool TrueTypeFont::LoadGlyph(char32_t unicodeChar) const {
    unsigned int glyphIndex = FT_Get_Char_Index(ftFace, unicodeChar);

    if (glyphIndex == 0) {
        // There is no glyph image for this character.
        return false;
    }

    // Load glyph into glyph slot.
    if (FT_Load_Glyph(ftFace, glyphIndex, FT_LOAD_TARGET_NORMAL | FT_LOAD_NO_BITMAP) != 0) {
        return false;
    }

    return true;
}

FT_GlyphSlot TrueTypeFont::RenderGlyph(FT_Render_Mode renderMode) {
    FT_Render_Glyph(ftFace->glyph, renderMode);

    return ftFace->glyph;
}

FT_Glyph TrueTypeFont::RenderGlyphWithBorder(FT_Render_Mode renderMode, float borderThickness) {
    FT_Stroker stroker;
    if (FT_Stroker_New(ftLibrary, &stroker) != 0) {
        return nullptr;
    }

    FT_Stroker_Set(stroker, (int)(borderThickness * 64), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

    FT_Glyph glyph;
    if (FT_Get_Glyph(ftFace->glyph, &glyph) != 0) {
        return nullptr;
    }

    if (FT_Glyph_Stroke(&glyph, stroker, 1) != 0) {
        return nullptr;
    }

    if (FT_Glyph_To_Bitmap(&glyph, renderMode, nullptr, false) != 0) {
        return nullptr;
    }

    FT_Stroker_Done(stroker);

    return glyph;
}

// Draw pixels using the FreeType bitmap information.
void TrueTypeFont::BakeGlyphBitmap(const FT_Bitmap *bitmap, int dstPitch, byte *dstPtr) {
    const byte *srcPtr = bitmap->buffer;

    switch (bitmap->pixel_mode) {
    case FT_PIXEL_MODE_MONO:
        for (int y = 0; y < bitmap->rows; y++) {
            for (int x = 0; x < bitmap->width; x++) {
                if (srcPtr[x >> 3] & (0x80 >> (x & 7))) {
                    dstPtr[x] = 255;
                } else {
                    dstPtr[x] = 0;
                }
            }
            srcPtr += bitmap->pitch;
            dstPtr += dstPitch;
        }
        break;
    case FT_PIXEL_MODE_GRAY:
        for (int y = 0; y < bitmap->rows; y++) {
            for (int x = 0; x < bitmap->width; x++) {
                dstPtr[x] = srcPtr[x];
            }
            srcPtr += bitmap->pitch;
            dstPtr += dstPitch;
        }
        break;
    default:
        assert(0);
        break;
    }
}

BE_NAMESPACE_END
