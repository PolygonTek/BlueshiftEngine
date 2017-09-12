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

#define TYPE_BM 0x4d42

#pragma pack(1)

struct BmpHeader {
    uint16_t    type;   // 'BM'
    uint32_t    size;
    uint16_t    reserved1;
    uint16_t    reserved2;
    uint32_t    ofsBits;
};

struct BmpInfoHeader {
    uint32_t    size;
    int32_t     width;
    int32_t     height;
    uint16_t    planes;
    uint16_t    bpp;
    uint32_t    compression;
    uint32_t    imageSize;
    int32_t     temp1;
    int32_t     temp2;
    uint32_t    colorUsed;
    uint32_t    colorImportant;
};

#pragma pack()

bool Image::LoadBMPFromMemory(const char *name, const byte *data, size_t size) {
    const byte *ptr = data;
    BmpHeader *bmfh = (BmpHeader *)ptr;
    ptr += sizeof(BmpHeader);

    if (bmfh->type != TYPE_BM) {
        BE_WARNLOG(L"Image::LoadBMPFromMemory: bad BMP format %hs\n", name);
        return false;
    }
    
    BmpInfoHeader *bmih = (BmpInfoHeader *)ptr;
    ptr += sizeof(BmpInfoHeader);

    if (bmih->bpp != 8 && bmih->bpp != 16 && bmih->bpp != 24 && bmih->bpp != 32) {
        BE_WARNLOG(L"Image::LoadBMPFromMemory: unsupported color depth %hs\n", name);
        return false;
    }

    int w = bmih->width;
    int h = bmih->height;
    bool flipped = false;
    
    if (h < 0) {
        flipped = true;
        h = -h;
    }

    Create2D(w, h, 1, Image::BGR_8_8_8, nullptr, 0);
        
    int padbytes = (((bmih->bpp * w + 31) & ~31) - (bmih->bpp * w)) >> 3;
    const byte *palette;

    // 8 bits paletted color
    if (bmih->bpp == 8) {
        int paletteSize;
        if (bmih->colorUsed > 0) {
            paletteSize = bmih->colorUsed * 4;
        } else {
            paletteSize = 256 * 4;
        }
        
        palette = ptr;
        ptr += paletteSize;
    }

    uint16_t packed;
    byte r, g, b;
    byte index;

    if (flipped) {
        for (int y = 0; y < h; y++) {
            byte *dstptr = pic + y * w * 3;
            for (int x = 0; x < w; x++) {
                switch (bmih->bpp) {
                case 8:
                    index = *ptr++;
                    b = palette[4 * index];
                    g = palette[4 * index + 1];
                    r = palette[4 * index + 2];
                    break;
                case 16:
                    packed = *(uint16_t *)ptr++;
                    b = ((packed) & 0x1f) << 3;
                    g = ((packed>>5) & 0x1f) << 3;
                    r = ((packed>>10) & 0x1f) << 3;
                    break;
                case 24:
                    b = *ptr++;
                    g = *ptr++;
                    r = *ptr++;
                    break;
                case 32:
                    b = *ptr++;
                    g = *ptr++;
                    r = *ptr++;
                    ptr++;
                    break;
                }

                *dstptr++ = b;
                *dstptr++ = g;
                *dstptr++ = r;
            }
            ptr += padbytes;
        }
    } else {
        for (int y = h - 1; y >= 0; y--) {
            byte *dstptr = pic + y * w * 3;
            for (int x = 0; x < w; x++) {
                switch (bmih->bpp) {
                case 8:
                    index = *ptr++;
                    b = palette[4 * index];
                    g = palette[4 * index + 1];
                    r = palette[4 * index + 2];
                    break;
                case 16:
                    packed = *(uint16_t *)ptr++;
                    b = ((packed) & 0x1f) << 3;
                    g = ((packed>>5) & 0x1f) << 3;
                    r = ((packed>>10) & 0x1f) << 3;
                    break;
                case 24:
                    b = *ptr++;
                    g = *ptr++;
                    r = *ptr++;
                    break;
                case 32:
                    b = *ptr++;
                    g = *ptr++;
                    r = *ptr++;
                    ptr++;
                    break;
                }

                *dstptr++ = b;
                *dstptr++ = g;
                *dstptr++ = r;
            }
            ptr += padbytes;
        }
    }

    return true;
}

bool Image::WriteBMP(const char *filename) const {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Image::WriteBMP: file open error\n");
        return false;
    }

    Image convertedImage;
    byte *src = pic;
    if (format != Image::BGR_8_8_8) {
        if (!ConvertFormat(Image::BGR_8_8_8, convertedImage)) {
            fileSystem.CloseFile(fp);
            return false;
        }
        src = convertedImage.pic;
    }

    int pitch           = ((width * 24 + 31) / 32) << 2;
    int pad             = pitch - width * 3;
    int bits_size       = pitch * height * 3;
    
    BmpHeader bmfh;
    BmpInfoHeader bmih;
    bmfh.type           = TYPE_BM;
    bmfh.size           = sizeof(bmfh) + sizeof(bmih) + bits_size;
    bmfh.reserved1      = 0;
    bmfh.reserved2      = 0;
    bmfh.ofsBits        = sizeof(bmfh) + sizeof(bmih);
    fp->Write(&bmfh, sizeof(bmfh));
    
    bmih.size           = sizeof(bmih);
    bmih.width          = width;
    bmih.height         = height;
    bmih.planes         = 1;
    bmih.bpp            = 24;
    bmih.compression    = 0;
    bmih.imageSize      = 0;
    bmih.temp1          = 0;
    bmih.temp2          = 0;
    bmih.colorUsed      = 0;
    bmih.colorImportant = 0;
    fp->Write(&bmih, sizeof(bmih));

    byte temp[4] = { 0, };
    
    for (int y = height - 1; y >= 0; y--) {
        byte *ptr = (byte *)src + y * width * 3;
        fp->Write(ptr, 3 * width);
        fp->Write(temp, pad);
    }

    fileSystem.CloseFile(fp);	

    return true;
}

BE_NAMESPACE_END
