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

#pragma once

#pragma pack(1)

struct FontFileHeader {
    uint32_t        ofsBitmaps;     // byte offset of bitmaps
    uint32_t        numBitmaps;     // a number of bitmaps
    uint32_t        ofsGlyphs;      // byte offset of glyphs
    uint32_t        numGlyphs;      // a number of glyphs
};

struct FontFileBitmap {
    char            name[256];
};

struct FontFileGlyph {
    char32_t        charCode;
    int32_t         width, height;
    int32_t         offsetX, offsetY;
    int32_t         advanceX, advanceY;
    float           s, t, s2, t2;
    uint32_t        bitmapIndex;
};

#pragma pack()
