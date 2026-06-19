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

// Reference: http ://paulbourke.net/dataformats/pic/
struct RgbeHeaderInfo {
    char        signature[11];      // #?RADIANCE
    float       gamma;
    float       exposure;
    int         width;
    int         height;
};

// standard conversion from float pixels to rgbe pixels
static void Float2RGBE(float red, float green, float blue, byte *rgbe) {
    float v;
    int e;

    red = red > 0.0f ? (red > 1.0e38f ? 1.0e38f : red) : 0.0f;
    green = green > 0.0f ? (green > 1.0e38f ? 1.0e38f : green) : 0.0f;
    blue = blue > 0.0f ? (blue > 1.0e38f ? 1.0e38f : blue) : 0.0f;

    v = Max3(red, green, blue);
    if (v < 1e-32) {
        rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
    } else {
        v = (float)(frexp(v, &e) * 256.0 / v);
        rgbe[0] = (byte)(red * v);
        rgbe[1] = (byte)(green * v);
        rgbe[2] = (byte)(blue * v);
        rgbe[3] = (byte)(e + 128);
    }
}

static bool CanWriteHDRDirect(Image::Format format, Image::GammaSpace gammaSpace) {
    if (gammaSpace != Image::GammaSpace::Linear) {
        return false;
    }

    switch (format) {
    case Image::Format::R16G16B16_FLOAT:
    case Image::Format::R16G16B16A16_FLOAT:
    case Image::Format::R32G32B32_FLOAT:
    case Image::Format::R32G32B32A32_FLOAT:
        return true;
    default:
        return false;
    }
}

static void ReadHDRPixelAsFloat(Image::Format format, const byte *src, float &r, float &g, float &b) {
    switch (format) {
    case Image::Format::R16G16B16_FLOAT: {
        const float16_t *p = reinterpret_cast<const float16_t *>(src);
        r = F16Converter::ToF32(p[0]);
        g = F16Converter::ToF32(p[1]);
        b = F16Converter::ToF32(p[2]);
        break;
    }
    case Image::Format::R16G16B16A16_FLOAT: {
        const float16_t *p = reinterpret_cast<const float16_t *>(src);
        r = F16Converter::ToF32(p[0]);
        g = F16Converter::ToF32(p[1]);
        b = F16Converter::ToF32(p[2]);
        break;
    }
    case Image::Format::R32G32B32_FLOAT: {
        const float *p = reinterpret_cast<const float *>(src);
        r = p[0];
        g = p[1];
        b = p[2];
        break;
    }
    case Image::Format::R32G32B32A32_FLOAT: {
        const float *p = reinterpret_cast<const float *>(src);
        r = p[0];
        g = p[1];
        b = p[2];
        break;
    }
    default:
        assert(0);
        r = g = b = 0.0f;
        break;
    }
}

static bool WriteHDRRLEChannel(File *fp, const byte *channel, int width) {
    int x = 0;
    while (x < width) {
        const int count = Min(128, width - x);
        const byte countByte = static_cast<byte>(count);

        if (!fp->Write(&countByte, 1) || !fp->Write(channel + x, count)) {
            return false;
        }

        x += count;
    }

    return true;
}

static bool WriteHDRScanline(File *fp, const byte *scanline, int width) {
    if (width < 8 || width > 0x7fff) {
        return fp->Write(scanline, static_cast<size_t>(width) * 4);
    }

    const byte header[4] = {
        2,
        2,
        static_cast<byte>(width >> 8),
        static_cast<byte>(width & 0xff)
    };

    if (!fp->Write(header, sizeof(header))) {
        return false;
    }

    for (int i = 0; i < 4; i++) {
        if (!WriteHDRRLEChannel(fp, scanline + i * width, width)) {
            return false;
        }
    }

    return true;
}

// standard conversion from rgbe to float pixels 
// note: Ward uses ldexp(col + 0.5, exp - (128 + 8)).  However we wanted pixels
//       in the range [0,1] to map back into the range [0, 1].
static void RGBE2Float(const byte *rgbe, float *red, float *green, float *blue) {
    float f;

    if (rgbe[3]) { // nonzero pixel
        f = (float)(ldexp(1.0, rgbe[3] - (int)(128 + 8)));
        *red   = rgbe[0] * f;
        *green = rgbe[1] * f;
        *blue  = rgbe[2] * f;
    } else {
        *red = *green = *blue = 0.0;
    }
}

static int ParseRGBEHeaderInfo(char **ptr, RgbeHeaderInfo *info) {
    info->gamma = 1.0f;
    info->exposure = 1.0f;

    char *data = *ptr;

    char text[128];
    char c;
    int len = 0;

    while ((c = *data++) != '\n') {
        text[len++] = c;
    }
    text[len] = 0;

    strcpy(info->signature, text);
    
    while (1) {
        if (*data == '\n') {
            data++;
            break;
        }

        len = 0;

        while ((c = *data++) != '\n') {
            text[len++] = c;
        }
        text[len] = 0;

        if (text[0] == '#') {
            continue;
        }
            
        if (!Str::Cmpn(text, "GAMMA", 5)) {
            (void)sscanf(text, "GAMMA=%g", &info->gamma);
        } else if (!Str::Cmpn(text, "EXPOSURE", 8)) {
            (void)sscanf(text, "EXPOSURE=%g", &info->exposure);
        } else if (!Str::Cmpn(text, "FORMAT", 6)) {
            Str::Cmpn(text, "FORMAT=32-bit_rle_rgbe", 22);
        } else {
            continue;
        }
    }

    len = 0;

    while ((c = *data++) != '\n') {
        text[len++] = c;
    }
    text[len] = 0;

    if (!Str::Cmpn(text, "-Y", strlen("-Y"))) {
        (void)sscanf(text, "-Y %i +X %i", &info->height, &info->width);
    } else {
        return false;
    }

    *ptr = data;

    return true;
}

bool Image::LoadHDRFromMemory(const char *name, const byte *data, size_t size) {
    const byte *ptr = data;

    RgbeHeaderInfo headerInfo;
    if (!ParseRGBEHeaderInfo((char **)&ptr, &headerInfo)) {
        return false;
    }

    if (Str::Cmp(headerInfo.signature, "#?RADIANCE") != 0 &&
        Str::Cmp(headerInfo.signature, "#?RGBE") != 0) {
        BE_WARNLOG("Image::LoadHDRFromMemory: bad RGBE signature '%s' %s\n", headerInfo.signature, name);
        return false;
    }

    Create2D(headerInfo.width, headerInfo.height, 1, Format::R16G16B16_FLOAT, GammaSpace::Linear, nullptr, Flag::None);

    float16_t *dest = (float16_t *)this->pic;

    float r, g, b;

    if (headerInfo.width < 8 || headerInfo.width > 0x7fff) {
        for (int i = 0; i < headerInfo.width * headerInfo.height; i++) {
            RGBE2Float(ptr, &r, &g, &b);
            ptr += 4;

            *dest++ = F16Converter::FromF32(r);
            *dest++ = F16Converter::FromF32(g);
            *dest++ = F16Converter::FromF32(b);
        }
    } else {
        byte *lineBuffer = (byte *)_alloca(sizeof(byte) * 4 * headerInfo.width);

        byte rgbe[4];
        byte packet[2];
        int count;
        
        for (int y = 0; y < headerInfo.height; y++) {
            rgbe[0] = *ptr++;
            rgbe[1] = *ptr++;
            rgbe[2] = *ptr++;
            rgbe[3] = *ptr++;

            assert(rgbe[0] == 2 && rgbe[1] == 2 && !(rgbe[2] & 0x80));
            assert(((((int)rgbe[2]) << 8) | rgbe[3]) == headerInfo.width);

            byte *linePtr = lineBuffer;

            for (int i = 0; i < 4; i++) {
                byte *lineEndPtr = &lineBuffer[(i + 1) * headerInfo.width];

                while (linePtr < lineEndPtr) {
                    packet[0] = *ptr++;

                    if (packet[0] > 128) {
                        count = packet[0] - 128;
                        packet[1] = *ptr++;
                        
                        while (count--) {
                            *linePtr++ = packet[1];
                        }
                    } else {
                        count = packet[0];
                        while (count--) {
                            *linePtr++ = *ptr++;
                        }
                    }
                }
            }

            for (int i = 0; i < headerInfo.width; i++) {
                rgbe[0] = lineBuffer[i];
                rgbe[1] = lineBuffer[i + headerInfo.width];
                rgbe[2] = lineBuffer[i + headerInfo.width * 2];
                rgbe[3] = lineBuffer[i + headerInfo.width * 3];

                RGBE2Float(rgbe, &r, &g, &b);

                *dest++ = F16Converter::FromF32(r);
                *dest++ = F16Converter::FromF32(g);
                *dest++ = F16Converter::FromF32(b);
            }
        }
    }

    return true;
}

bool Image::WriteHDR(File *fp) const {
    if (!fp || !pic || width <= 0 || height <= 0) {
        return false;
    }
    if (depth != 1 || arraySize != 1) {
        BE_WARNLOG("Image::WriteHDR: HDR only supports 2D images\n");
        return false;
    }

    Image convertedImage;
    const Image *srcImage = this;
    if (!CanWriteHDRDirect(format, gammaSpace)) {
        if (!ConvertFormat(Format::R32G32B32_FLOAT, convertedImage, GammaSpace::Linear)) {
            BE_WARNLOG("Image::WriteHDR: failed to convert '%s' to RGB32F\n", Image::FormatName(format));
            return false;
        }
        srcImage = &convertedImage;
    }

    if (!fp->Printf("#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y %d +X %d\n", height, width)) {
        return false;
    }

    const bool useRLE = width >= 8 && width <= 0x7fff;
    const int bytesPerPixel = srcImage->BytesPerPixel();
    const byte *src = srcImage->GetPixels();
    byte *scanline = static_cast<byte *>(Mem_Alloc16(static_cast<size_t>(width) * 4));

    for (int y = 0; y < height; y++) {
        const byte *srcRow = src + static_cast<size_t>(y) * width * bytesPerPixel;

        for (int x = 0; x < width; x++) {
            float r;
            float g;
            float b;
            byte rgbe[4];

            ReadHDRPixelAsFloat(srcImage->GetFormat(), srcRow + x * bytesPerPixel, r, g, b);
            Float2RGBE(r, g, b, rgbe);

            if (useRLE) {
                scanline[x] = rgbe[0];
                scanline[x + width] = rgbe[1];
                scanline[x + width * 2] = rgbe[2];
                scanline[x + width * 3] = rgbe[3];
            } else {
                byte *dst = scanline + x * 4;
                dst[0] = rgbe[0];
                dst[1] = rgbe[1];
                dst[2] = rgbe[2];
                dst[3] = rgbe[3];
            }
        }

        if (!WriteHDRScanline(fp, scanline, width)) {
            Mem_AlignedFree(scanline);
            return false;
        }
    }

    Mem_AlignedFree(scanline);
    return true;
}

bool Image::WriteHDR(const char *filename) const {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Image::WriteHDR: file open error\n");
        return false;
    }

    bool ret = WriteHDR(fp);

    fileSystem.CloseFile(fp);

    return ret;
}

BE_NAMESPACE_END
