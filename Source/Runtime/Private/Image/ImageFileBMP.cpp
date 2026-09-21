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
#include "Core/Heap.h"
#include "Math/Math.h"
#include "IO/FileSystem.h"
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
    if (size < sizeof(BmpHeader) + sizeof(BmpInfoHeader)) {
        return false;
    }

    const byte *ptr = data;
    BmpHeader *bmfh = (BmpHeader *)ptr;
    ptr += sizeof(BmpHeader);

    if (bmfh->type != TYPE_BM) {
        BE_WARNLOG("Image::LoadBMPFromMemory: bad BMP format %s\n", name);
        return false;
    }

    BmpInfoHeader *bmih = (BmpInfoHeader *)ptr;

    if (bmih->size < sizeof(BmpInfoHeader) || bmih->size > size - sizeof(BmpHeader)) {
        return false;
    }

    if (bmih->bpp != 8 && bmih->bpp != 16 && bmih->bpp != 24 && bmih->bpp != 32) {
        BE_WARNLOG("Image::LoadBMPFromMemory: unsupported color depth %s\n", name);
        return false;
    }

    if (bmih->compression != 0) {
        BE_WARNLOG("Image::LoadBMPFromMemory: unsupported compression %s\n", name);
        return false;
    }

    const size_t infoEnd = sizeof(BmpHeader) + bmih->size;
    if (bmfh->ofsBits < infoEnd || bmfh->ofsBits > size) {
        return false;
    }

    int w = bmih->width;
    int h = bmih->height;
    bool flipped = false;

    if (h < 0) {
        flipped = true;
        h = -h;
    }

    int padbytes = (((bmih->bpp * w + 31) & ~31) - (bmih->bpp * w)) >> 3;
    const byte *palette = nullptr;

    // 8 bits paletted color
    if (bmih->bpp == 8) {
        size_t paletteSize;
        if (bmih->colorUsed > 0) {
            paletteSize = bmih->colorUsed * 4;
        } else {
            paletteSize = 256 * 4;
        }

        if (paletteSize > bmfh->ofsBits - infoEnd) {
            return false;
        }

        palette = data + infoEnd;
    }

    Create2D(w, h, 1, Image::Format::B8G8R8, Image::GammaSpace::sRGB, nullptr);

    ptr = data + bmfh->ofsBits;

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
                    packed = *reinterpret_cast<const uint16_t *>(ptr);
                    ptr += sizeof(packed);
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
                    packed = *reinterpret_cast<const uint16_t *>(ptr);
                    ptr += sizeof(packed);
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

bool Image::WriteBMP(File *fp) const {
    if (!fp) {
        return false;
    }

    Image convertedImage;
    byte *src = pic;
    if (format != Image::Format::B8G8R8) {
        if (!ConvertFormat(Image::Format::B8G8R8, convertedImage)) {
            return false;
        }
        src = convertedImage.pic;
    }

    int pitch           = ((width * 24 + 31) / 32) << 2;
    int pad             = pitch - width * 3;
    int bits_size       = pitch * height;
    
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

    return true;
}

bool Image::WriteBMP(const char *filename) const {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Image::WriteBMP: failed to open file '%s' for writing\n", filename);
        return false;
    }

    bool ret = WriteBMP(fp);

    fileSystem.CloseFile(fp);

    return ret;
}

BE_NAMESPACE_END
