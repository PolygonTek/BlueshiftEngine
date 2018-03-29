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
#include "Math/Math.h"
#include "File/FileSystem.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

#pragma pack(1)

struct PcxHeader {
    byte        ident;          // 0x0a
    byte        version;        // version 5
    byte        encoding;       // when 1, it's RLE encoding
    byte        bpp;            // bits per pixel
    int16_t     xmin, ymin;
    int16_t     xmax, ymax;
    int16_t     hres, vres;
    byte        palette[48];    // 16-color palette
    byte        mode;
    byte        planes;         // number of color planes
    int16_t     bpl;            // number of bytes per line
    int16_t     palette_info;   // 1 = color, 2 = grayscale
    byte        extra[58];   
};

#pragma pack()

bool Image::LoadPCXFromMemory(const char *name, const byte *data, size_t size) {
    const byte *ptr = data;
    PcxHeader *header = (PcxHeader *)ptr;
    ptr += sizeof(PcxHeader);
    
    if (header->bpp != 8 || (header->planes != 1 && header->planes != 3)) {
        BE_WARNLOG(L"Image::LoadPCXFromMemory: bad PCX format %hs\n", name);
        return false;
    }
    
    Create2D(header->xmax - header->xmin + 1, header->ymax - header->ymin + 1, 1, RGB_8_8_8, nullptr, 0);

    if (header->planes == 1) { // 8 bits paletted color
        const byte *palette = data + size - 768;

        for (int y = 0; y < this->height; y++) {
            byte *dstptr = this->pic + this->height * this->width * 3;
            for (int x = 0; x < this->width;) {
                byte packet = *ptr++;
                if ((packet & 0xc0) == 0xc0) {
                    int length = (packet & 0x3f);
                    int index = *ptr++;
                    while (length--) {
                        *dstptr++ = palette[index*3];
                        *dstptr++ = palette[index*3+1];
                        *dstptr++ = palette[index*3+2];
                        x++;
                    }
                } else {
                    *dstptr++ = palette[packet*3];
                    *dstptr++ = palette[packet*3+1];
                    *dstptr++ = palette[packet*3+2];
                    x++;
                }
            }
        }
    } else { // 24 bits color
        for (int y = 0; y < this->height; y++) {
            for (int c = 0; c <= 2; c++) {
                byte *dstptr = this->pic + this->height * this->width * 3 + c;
                for (int x = 0; x < this->width;) {
                    byte packet = *ptr++;
                    if ((packet & 0xc0) == 0xc0) {
                        int length = (packet & 0x3f);
                        int color = *ptr++;
                        while (length--) {
                            *dstptr = color;
                            dstptr += 3;
                            x++;
                        }
                    } else {
                        *dstptr = packet;
                        dstptr += 3;
                        x++;
                    }
                }
            }
        }
    }

    return true;
}

bool Image::WritePCX(const char *filename) const {	
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Image::WritePCX: file open error\n");
        return false;
    }

    Image convertedImage;
    byte *src = pic;
    if (format != BGR_8_8_8) {
        if (!ConvertFormat(BGR_8_8_8, convertedImage)) {
            fileSystem.CloseFile(fp);
            return false;
        }
        
        src = convertedImage.pic;
    }

    PcxHeader pcxfh;
    pcxfh.ident     = 10;
    pcxfh.version   = 5;
    pcxfh.encoding  = 1;
    pcxfh.bpp       = 8;
    pcxfh.xmin      = 0;
    pcxfh.ymin      = 0;
    pcxfh.xmax      = width - 1;
    pcxfh.ymax      = height - 1;
    pcxfh.hres      = width;
    pcxfh.vres      = height;
    pcxfh.mode      = 1;
    pcxfh.planes    = 3;
    pcxfh.bpl       = width;
    memset(pcxfh.palette, 0, 48);
    memset(pcxfh.extra, 0, 58);

    fp->Write(&pcxfh, sizeof(pcxfh));

    // FIXME: fwrite 를 한번에 하자
    for (int y = 0; y < height; y++) {
        for (int c = 2; c >= 0; c--) {
            byte run_n = 0;
            byte run_c = 0;
            byte temp;

            for (int x = 0; x < width; x++) {
                byte dot = *(src + y * width * 3 + x * 3 + c);
                if (run_n == 0) {
                    run_n = 1;
                    run_c = dot;
                } else {
                    if (dot != run_c || run_n >= 0x3f) {
                        if (run_n > 1 || (run_c & 0xc0) == 0xc0) {
                            temp = 0xc0 | run_n;
                            fp->Write(&temp, 1);
                        }
                        fp->Write(&run_c, 1);
                        run_n = 1;
                        run_c = dot;
                    } else {
                        run_n++;
                    }
                }
            }

            if (run_n > 1 || (run_c & 0xc0) == 0xc0) {
                temp = 0xc0 | run_n;
                fp->Write(&temp, 1);
            }
            fp->Write(&run_c, 1);
        }
    }

    fileSystem.CloseFile(fp);

    return true;
}

BE_NAMESPACE_END
