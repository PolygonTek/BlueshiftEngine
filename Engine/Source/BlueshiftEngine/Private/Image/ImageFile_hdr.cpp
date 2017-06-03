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
            
        if (!Str::Cmpn(text, "GAMMA", strlen("GAMMA"))) {
            sscanf(text, "GAMMA=%g", &info->gamma);
        } else if (!Str::Cmpn(text, "EXPOSURE", strlen("EXPOSURE"))) {
            sscanf(text, "EXPOSURE=%g", &info->exposure);
        } else if (!Str::Cmpn(text, "FORMAT", strlen("FORMAT"))) {
            Str::Cmpn(text, "FORMAT=32-bit_rle_rgbe", strlen("FORMAT=32-bit_rle_rgbe"));
        } else {
            return false;
        }
    }

    len = 0;

    while ((c = *data++) != '\n') {
        text[len++] = c;
    }
    text[len] = 0;

    if (!Str::Cmpn(text, "-Y", strlen("-Y"))) {
        sscanf(text, "-Y %i +X %i", &info->height, &info->width);
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

    if (Str::Cmp(headerInfo.signature, "#?RADIANCE") != 0) {
        BE_WARNLOG(L"Image::LoadHDRFromMemory: bad RGBE signature '%hs' %hs\n", headerInfo.signature, name);
        return false;
    }

    Create2D(headerInfo.width, headerInfo.height, 1, RGB_16F_16F_16F, nullptr, LinearFlag);

    float16_t *dest = (float16_t *)this->pic;

    float r, g, b;

    if (headerInfo.width < 8 || headerInfo.height > 0x7fff)	{
        for (int i = 0; i < headerInfo.width * headerInfo.height; i++) {
            RGBE2Float(ptr, &r, &g, &b);
            ptr += 4;

            *dest++ = F32toF16(r);
            *dest++ = F32toF16(g);
            *dest++ = F32toF16(b);
        }
    } else {
        byte *line_buffer = (byte *)_alloca(sizeof(byte) * 4 * headerInfo.width);

        byte rgbe[4];
        byte packet[2];
        int count;
        
        for (int j = 0; j < headerInfo.height; j++) {
            rgbe[0] = *ptr++;
            rgbe[1] = *ptr++;
            rgbe[2] = *ptr++;
            rgbe[3] = *ptr++;

            assert(rgbe[0] == 2 && rgbe[1] == 2 && !(rgbe[2] & 0x80));
            assert(((((int)rgbe[2]) << 8) | rgbe[3]) == headerInfo.width);

            byte *line_ptr = line_buffer;

            for (int i = 0; i < 4; i++)	{
                byte *line_ptr_end = &line_buffer[(i + 1) * headerInfo.width];
                while (line_ptr < line_ptr_end)	{
                    packet[0] = *ptr++;

                    if (packet[0] > 128) {
                        count = packet[0] - 128;
                        packet[1] = *ptr++;
                        
                        while (count--) {
                            *line_ptr++ = packet[1];
                        }
                    } else {
                        count = packet[0];
                        while (count--) {
                            *line_ptr++ = *ptr++;;
                        }
                    }
                }
            }

            for (int i = 0; i < headerInfo.width; i++) {
                rgbe[0] = line_buffer[i];
                rgbe[1] = line_buffer[i + headerInfo.width];
                rgbe[2] = line_buffer[i + headerInfo.width * 2];
                rgbe[3] = line_buffer[i + headerInfo.width * 3];

                RGBE2Float(rgbe, &r, &g, &b);

                *dest++ = F32toF16(r);
                *dest++ = F32toF16(g);
                *dest++ = F32toF16(b);
            }
        }
    }

    return true;
}

BE_NAMESPACE_END
