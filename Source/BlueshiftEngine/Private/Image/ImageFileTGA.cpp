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
#include "Core/Heap.h"
#include "Math/Math.h"
#include "File/FileSystem.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

#define MAXCOLORS       16384

// image types
#define TGA_NULL        0
#define TGA_MAP         1
#define TGA_RGB         2
#define TGA_MONO        3
#define TGA_RLEMAP      9
#define TGA_RLERGB      10
#define TGA_RLEMONO     11
#define TGA_COMPMAP     32
#define TGA_COMPMAP4    33

// interleave flag
#define TGA_IL_NONE     0
#define TGA_IL_TWO      1
#define TGA_IL_FOUR     2

#pragma pack(1)

struct TargaHeader {
    byte            idLength;
    byte            colormap_type;
    byte            image_type;
    uint16_t        colormap_index;
    uint16_t        colormap_length;
    byte            colormap_bpp;
    uint16_t        x_origin;
    uint16_t        y_origin;
    uint16_t        width;
    uint16_t        height;
    byte            bpp;
    byte            attributes;
};

#pragma pack()

bool Image::LoadTGAFromMemory(const char *name, const byte *data, size_t size) {
    uint16_t packed;
    byte r, g, b, a;
    int	i;

    const byte *ptr = data;
    TargaHeader *header = (TargaHeader *)ptr;
    ptr += sizeof(TargaHeader) + header->idLength;

    switch (header->image_type) {
    case TGA_MAP:
    case TGA_RGB:
    case TGA_MONO:
    case TGA_RLEMAP:
    case TGA_RLERGB:
    case TGA_RLEMONO:
        break;
    default:
        BE_LOG(L"Image::LoadTGAFromMemory: %hs has bad image_type %i\n", name, header->image_type);
        return false;
    }

    switch (header->bpp) {
    case 8:
    case 16:
    case 24:
    case 32:
        break;
    default:
        BE_WARNLOG(L"Image::LoadTGAFromMemory: %hs has bad bpp %i\n", name, header->bpp);
        return false;
    }

    byte *colormap = nullptr;
    bool has_colormap = (header->image_type == TGA_MAP || 
        header->image_type == TGA_RLEMAP || 
        header->image_type == TGA_COMPMAP || 
        header->image_type == TGA_COMPMAP4) && header->colormap_type == 1;	

    // 칼라맵 정보가 있다면 읽어들인다
    if (has_colormap) {
        switch (header->colormap_bpp) {
        case 8:
        case 16:
        case 24:
        case 32:
            break;
        default:
            BE_WARNLOG(L"Image::LoadTGAFromMemory: %hs has bad %i bpp colormaps\n", name, header->bpp);
            return false;
        }

        if ((header->colormap_index + header->colormap_length + 1) >= MAXCOLORS) {
            return false;
        }

        colormap = (byte *)Mem_Alloc(MAXCOLORS * header->colormap_bpp / 8);

        for (i = header->colormap_index; i < header->colormap_index + header->colormap_length; i++)	{
            // read appropriate number of bytes, break into rgb & put in map
            switch (header->colormap_bpp) {
            case 8:         // grey scale, read and triplicate
                r = g = b = *ptr++;
                break;
            case 16:        // 5 bits each of red green and blue
            case 15:        // watch for byte order
                packed = *(uint16_t *)ptr++;
                b = ((packed) & 0x1f) << 3;
                g = ((packed>>5) & 0x1f) << 3;
                r = ((packed>>10) & 0x1f) << 3;
                break;
            case 24:        // 8 bits each of blue green and red
                b = *ptr++;
                g = *ptr++;
                r = *ptr++;
                break;
            case 32:
                b = *ptr++;
                g = *ptr++;
                r = *ptr++;
                a = *ptr++; // read alpha byte
                break;
            }

            unsigned int colormap_idx = i * header->colormap_bpp / 8;
            colormap[colormap_idx + 0] = b;
            colormap[colormap_idx + 1] = g;
            colormap[colormap_idx + 2] = r;
            if (header->colormap_bpp == 32) {
                colormap[colormap_idx + 3] = a;
            }
        }
    }

    Create2D(header->width, header->height, 1, header->bpp == 32 ? BGRA_8_8_8_8 : BGR_8_8_8, nullptr, 0);

    int byte_per_pixel = BytesPerPixel();
        
    byte *dest = this->pic;

    // RLE check
    bool rlencoded = header->image_type == TGA_RLEMAP || header->image_type == TGA_RLERGB || header->image_type == TGA_RLEMONO;

    unsigned int truerow = 0;
    unsigned int baserow = 0;

    int RLE_count = 0;
    int RLE_flag = 0;

    for (unsigned int y = 0; y < height; y++) {
        unsigned int realrow = truerow;
        if (!((header->attributes & 0x20) >> 5)) {
            realrow = height - realrow - 1;
        }

        for (unsigned int x = 0; x < width; x++) {
            if (rlencoded) {
                if (!RLE_count) { // have to restart run
                    i = *ptr++;
                    RLE_flag = (i & 0x80);
                    if (!RLE_flag) {
                        RLE_count = i + 1;      // stream of unencoded pixels
                    } else {
                        RLE_count = i - 127;    // single pixel replicated
                    }
                                        
                    RLE_count--;                // decrement count & get pixel
                } else {                        // have already read count & (at least) first pixel
                    RLE_count--;
                    if (RLE_flag) {
                        goto PixEncode;         // replicated pixels
                    }
                }
            }

            switch (header->bpp) {
            case 8:
                r = g = b = i = *ptr++;
                break;
            case 16:
                packed = i = *(uint16_t *)ptr++;
                b = ((packed) & 0x1f) << 3;
                g = ((packed>>5) & 0x1f) << 3;
                r = ((packed>>10) & 0x1f) << 3;
                break;
            case 24:
                i = 0;
                b = *ptr++;
                g = *ptr++;
                r = *ptr++;
                break;
            case 32:
                i = 0;
                b = *ptr++;
                g = *ptr++;
                r = *ptr++;
                a = *ptr++;
                break;
            }
PixEncode:
            int idx = (realrow * width + x) * byte_per_pixel;
            if (has_colormap) {
                unsigned int colormap_idx = i * header->colormap_bpp / 8;
                dest[idx + 0] = colormap[colormap_idx + 0];
                dest[idx + 1] = colormap[colormap_idx + 1];
                dest[idx + 2] = colormap[colormap_idx + 2];
                if (header->bpp == 32) {
                    dest[idx + 3] = colormap[colormap_idx + 3];
                }
            } else {
                dest[idx + 0] = b;
                dest[idx + 1] = g;
                dest[idx + 2] = r;
                if (header->bpp == 32) {
                    dest[idx + 3] = a;
                }
            }
        }

        if ((header->attributes & 0xc0) >> 6 == TGA_IL_FOUR) {
            truerow += 4;
        } else if ((header->attributes & 0xc0) >> 6 == TGA_IL_TWO) {
            truerow += 2;
        } else {
            truerow++;
        }
        
        if (truerow >= height) {
            truerow = ++baserow;
        }
    }

    if (has_colormap) {
        Mem_Free(colormap);
    }

    return true;
}

bool Image::WriteTGA(const char *filename) const {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Image::WriteTGA: file open error\n");
        return false;
    }

    int bpp;
    Image convertedImage;
    byte *src = this->pic;
    
    if (format == BGR_8_8_8) {
        bpp = 24;
    } else if (format == BGRA_8_8_8_8) {
        bpp = 32;
    } else {
        bpp = 24;
        if (!ConvertFormat(BGR_8_8_8, convertedImage)) {
            fileSystem.CloseFile(fp);
            return false;
        }

        src = convertedImage.pic;
    }

    TargaHeader tgafh;
    tgafh.idLength          = 0;
    tgafh.colormap_type     = 0;
    tgafh.image_type        = 2;
    tgafh.colormap_index    = 0;
    tgafh.colormap_length   = 0;
    tgafh.colormap_bpp      = 0;
    tgafh.x_origin          = 0;
    tgafh.y_origin          = 0;
    tgafh.width             = this->width;
    tgafh.height            = this->height;
    tgafh.bpp               = bpp;
    tgafh.attributes        = 8;

    fp->Write(&tgafh, sizeof(tgafh));

    for (int y = height - 1; y >= 0; y--) {
        byte *ptr = (byte *)src + y * width * (bpp >> 3);
        fp->Write(ptr, (bpp >> 3) * width);
    }

    fileSystem.CloseFile(fp);

    return true;
}

BE_NAMESPACE_END
