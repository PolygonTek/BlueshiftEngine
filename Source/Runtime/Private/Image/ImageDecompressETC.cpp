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
#include "Image/Image.h"
#include "ImageInternal.h"
#include "libpvrt/PVRTTexture.h"
#include "libpvrt/PVRTGlobal.h"
#include "ETCPACK/etcpack_lib.h"

BE_NAMESPACE_BEGIN

/*****************************************************************************
Macros
*****************************************************************************/
#define _CLAMP_(X,Xmin,Xmax) (  (X)<(Xmax) ?  (  (X)<(Xmin)?(Xmin):(X)  )  : (Xmax)    )

/*****************************************************************************
Constants
******************************************************************************/
unsigned int ETC_FLIP = 0x01000000;
unsigned int ETC_DIFF = 0x02000000;
const int mod[8][4] = { { 2, 8, -2, -8 },
{ 5, 17, -5, -17 },
{ 9, 29, -9, -29 },
{ 13, 42, -13, -42 },
{ 18, 60, -18, -60 },
{ 24, 80, -24, -80 },
{ 33, 106, -33, -106 },
{ 47, 183, -47, -183 } };

/*!***********************************************************************
@Function		modifyPixel
@Input			red		Red value of pixel
@Input			green	Green value of pixel
@Input			blue	Blue value of pixel
@Input			x	Pixel x position in block
@Input			y	Pixel y position in block
@Input			modBlock	Values for the current block
@Input			modTable	Modulation values
@Returns		Returns actual pixel colour
@Description	Used by ETCTextureDecompress
*************************************************************************/
static unsigned int modifyPixel(int red, int green, int blue, int x, int y, unsigned int modBlock, int modTable)
{
    int index = x * 4 + y, pixelMod;
    unsigned int mostSig = modBlock << 1;

    if (index<8)
        pixelMod = mod[modTable][((modBlock >> (index + 24)) & 0x1) + ((mostSig >> (index + 8)) & 0x2)];
    else
        pixelMod = mod[modTable][((modBlock >> (index + 8)) & 0x1) + ((mostSig >> (index - 8)) & 0x2)];

    red = _CLAMP_(red + pixelMod, 0, 255);
    green = _CLAMP_(green + pixelMod, 0, 255);
    blue = _CLAMP_(blue + pixelMod, 0, 255);

    return ((red << 16) + (green << 8) + blue) | 0xff000000;
}

/*!***********************************************************************
@Function		ETCTextureDecompress
@Input			pSrcData The ETC texture data to decompress
@Input			x X dimension of the texture
@Input			y Y dimension of the texture
@Modified		pDestData The decompressed texture data
@Input			nMode The format of the data
@Returns		The number of bytes of ETC data decompressed
@Description	Decompresses ETC to RGBA 8888
*************************************************************************/
static int ETCTextureDecompress(const void * const pSrcData, const int &x, const int &y, const void *pDestData, const int &/*nMode*/)
{
    unsigned int blockTop, blockBot, *input = (unsigned int*)pSrcData, *output;
    unsigned char red1, green1, blue1, red2, green2, blue2;
    bool bFlip, bDiff;
    int modtable1, modtable2;

    for (int i = 0; i<y; i += 4)
    {
        for (int m = 0; m<x; m += 4)
        {
            blockTop = *(input++);
            blockBot = *(input++);

            output = (unsigned int*)pDestData + i*x + m;

            // check flipbit
            bFlip = (blockTop & ETC_FLIP) != 0;
            bDiff = (blockTop & ETC_DIFF) != 0;

            if (bDiff)
            {	// differential mode 5 colour bits + 3 difference bits
                // get base colour for subblock 1
                blue1 = (unsigned char)((blockTop & 0xf80000) >> 16);
                green1 = (unsigned char)((blockTop & 0xf800) >> 8);
                red1 = (unsigned char)(blockTop & 0xf8);

                // get differential colour for subblock 2
                signed char blues = (signed char)(blue1 >> 3) + ((signed char)((blockTop & 0x70000) >> 11) >> 5);
                signed char greens = (signed char)(green1 >> 3) + ((signed char)((blockTop & 0x700) >> 3) >> 5);
                signed char reds = (signed char)(red1 >> 3) + ((signed char)((blockTop & 0x7) << 5) >> 5);

                blue2 = (unsigned char)blues;
                green2 = (unsigned char)greens;
                red2 = (unsigned char)reds;

                red1 = red1 + (red1 >> 5);	// copy bits to lower sig
                green1 = green1 + (green1 >> 5);	// copy bits to lower sig
                blue1 = blue1 + (blue1 >> 5);	// copy bits to lower sig

                red2 = (red2 << 3) + (red2 >> 2);	// copy bits to lower sig
                green2 = (green2 << 3) + (green2 >> 2);	// copy bits to lower sig
                blue2 = (blue2 << 3) + (blue2 >> 2);	// copy bits to lower sig
            } else
            {	// individual mode 4 + 4 colour bits
                // get base colour for subblock 1
                blue1 = (unsigned char)((blockTop & 0xf00000) >> 16);
                blue1 = blue1 + (blue1 >> 4);	// copy bits to lower sig
                green1 = (unsigned char)((blockTop & 0xf000) >> 8);
                green1 = green1 + (green1 >> 4);	// copy bits to lower sig
                red1 = (unsigned char)(blockTop & 0xf0);
                red1 = red1 + (red1 >> 4);	// copy bits to lower sig

                // get base colour for subblock 2
                blue2 = (unsigned char)((blockTop & 0xf0000) >> 12);
                blue2 = blue2 + (blue2 >> 4);	// copy bits to lower sig
                green2 = (unsigned char)((blockTop & 0xf00) >> 4);
                green2 = green2 + (green2 >> 4);	// copy bits to lower sig
                red2 = (unsigned char)((blockTop & 0xf) << 4);
                red2 = red2 + (red2 >> 4);	// copy bits to lower sig
            }
            // get the modtables for each subblock
            modtable1 = (blockTop >> 29) & 0x7;
            modtable2 = (blockTop >> 26) & 0x7;

            if (!bFlip)
            {	// 2 2x4 blocks side by side

                for (int j = 0; j<4; j++)	// vertical
                {
                    for (int k = 0; k<2; k++)	// horizontal
                    {
                        *(output + j*x + k) = modifyPixel(red1, green1, blue1, k, j, blockBot, modtable1);
                        *(output + j*x + k + 2) = modifyPixel(red2, green2, blue2, k + 2, j, blockBot, modtable2);
                    }
                }

            } else
            {	// 2 4x2 blocks on top of each other
                for (int j = 0; j<2; j++)
                {
                    for (int k = 0; k<4; k++)
                    {
                        *(output + j*x + k) = modifyPixel(red1, green1, blue1, k, j, blockBot, modtable1);
                        *(output + (j + 2)*x + k) = modifyPixel(red2, green2, blue2, k, j + 2, blockBot, modtable2);
                    }
                }
            }
        }
    }

    return x*y / 2;
}

/*!***********************************************************************
@Function		PVRTDecompressETC
@Input			pSrcData The ETC texture data to decompress
@Input			x X dimension of the texture
@Input			y Y dimension of the texture
@Modified		pDestData The decompressed texture data
@Input			nMode The format of the data
@Returns		The number of bytes of ETC data decompressed
@Description	Decompresses ETC to RGBA 8888
*************************************************************************/
void DecompressETC1(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::RGBA_8_8_8_8);
    assert(dstImage.GetPixels());

    int x = srcImage.GetWidth();
    int y = srcImage.GetHeight();
    int i32read;
    byte *pDecompressedData = (byte *)malloc(x * y * 4);

    const byte *pSrcData = srcImage.GetPixels();

    if (x<ETC_MIN_TEXWIDTH || y<ETC_MIN_TEXHEIGHT) {
        // decompress into a buffer big enough to take the minimum size
        char* pTempBuffer = (char*)malloc(PVRT_MAX(x, ETC_MIN_TEXWIDTH)*PVRT_MAX(y, ETC_MIN_TEXHEIGHT) * 4);
        i32read = ETCTextureDecompress(pSrcData, PVRT_MAX(x, ETC_MIN_TEXWIDTH), PVRT_MAX(y, ETC_MIN_TEXHEIGHT), pTempBuffer, 0);

        for (unsigned int i = 0; i<y; i++) {
            // copy from larger temp buffer to output data
            memcpy((char*)(pDecompressedData)+i*x * 4, pTempBuffer + PVRT_MAX(x, ETC_MIN_TEXWIDTH) * 4 * i, x * 4);
        }

        if (pTempBuffer) {
            free(pTempBuffer);
        }
    } else {
        // decompress larger MIP levels straight into the output data
        i32read = ETCTextureDecompress(pSrcData, x, y, pDecompressedData, 0);
    }

    // swap r and b channels
    unsigned char* pSwap = (unsigned char*)pDecompressedData, swap;

    for (unsigned int i = 0; i < y; i++) {
        for (unsigned int j = 0; j < x; j++) {
            swap = pSwap[0];
            pSwap[0] = pSwap[2];
            pSwap[2] = swap;
            pSwap += 4;
        }
    }

    const ImageFormatInfo *dstFormatInfo = GetImageFormatInfo(dstImage.GetFormat());
    byte *dstPtr = dstImage.GetPixels();

    memcpy(dstPtr, (const byte *)pDecompressedData, x * y * 4);

    if (dstImage.NumMipmaps() > 1) {
        dstImage.GenerateMipmaps();
    }

    free(pDecompressedData);
}

// read color block from data stream
static void ReadColorBlockETC2(const byte *data, uint32_t &block1, uint32_t &block2) {
    block1 = data[0];
    block1 = block1 << 8; block1 |= data[1];
    block1 = block1 << 8; block1 |= data[2];
    block1 = block1 << 8; block1 |= data[3];
    block2 = data[4];
    block2 = block2 << 8; block2 |= data[5];
    block2 = block2 << 8; block2 |= data[6];
    block2 = block2 << 8; block2 |= data[7];
}

//-------------------------------------------------------------------------------------------------
// nvtt
//-------------------------------------------------------------------------------------------------

/// 32 bit color stored as BGRA.
class Color32 {
public:
    Color32() {}
    Color32(const Color32 & c) : u(c.u) {}
    Color32(uint8 R, uint8 G, uint8 B) { setRGBA(R, G, B, 0xFF); }
    Color32(uint8 R, uint8 G, uint8 B, uint8 A) { setRGBA(R, G, B, A); }
    explicit Color32(uint32_t U) : u(U) {}

    void setRGBA(uint8 R, uint8 G, uint8 B, uint8 A) {
        r = R;
        g = G;
        b = B;
        a = A;
    }

    void setBGRA(uint8 B, uint8 G, uint8 R, uint8 A = 0xFF) {
        r = R;
        g = G;
        b = B;
        a = A;
    }

    union {
        struct {
            uint8_t r, g, b, a;
        };
        uint8_t component[4];
        uint32_t u;
    };
};

union BlockETC {
    // Definitions from EtcLib/EtcBlock4x4EncodingBits.h
    struct Individual {
        unsigned red2 : 4;      // byte 0
        unsigned red1 : 4;
        unsigned green2 : 4;    // byte 1
        unsigned green1 : 4;
        unsigned blue2 : 4;     // byte 2
        unsigned blue1 : 4;
        unsigned flip : 1;      // byte 3
        unsigned diff : 1;
        unsigned cw2 : 3;
        unsigned cw1 : 3;
        unsigned selectors;     // bytes 4-7
    };

    struct Differential {
        unsigned dred2 : 3;     // byte 0
        unsigned red1 : 5;
        unsigned dgreen2 : 3;   // byte 1
        unsigned green1 : 5;
        unsigned dblue2 : 3;    // byte 2
        unsigned blue1 : 5;
        unsigned flip : 1;      // byte 3
        unsigned diff : 1;
        unsigned cw2 : 3;
        unsigned cw1 : 3;
        unsigned selectors;     // bytes 4-7
    };

    struct T {
        unsigned red1b : 2;     // byte 0
        unsigned detect2 : 1;
        unsigned red1a : 2;
        unsigned detect1 : 3;
        unsigned blue1 : 4;     // byte 1
        unsigned green1 : 4;
        unsigned green2 : 4;    // byte 2
        unsigned red2 : 4;
        unsigned db : 1;        // byte 3
        unsigned diff : 1;
        unsigned da : 2;
        unsigned blue2 : 4;
        unsigned selectors;     // bytes 4-7
    };

    struct H {
        unsigned green1a : 3;   // byte 0
        unsigned red1 : 4;
        unsigned detect1 : 1;
        unsigned blue1b : 2;    // byte 1
        unsigned detect3 : 1;
        unsigned blue1a : 1;
        unsigned green1b : 1;
        unsigned detect2 : 3;
        unsigned green2a : 3;   // byte 2
        unsigned red2 : 4;
        unsigned blue1c : 1;
        unsigned db : 1;        // byte 3
        unsigned diff : 1;
        unsigned da : 1;
        unsigned blue2 : 4;
        unsigned green2b : 1;
        unsigned selectors;     // bytes 4-7
    };

    struct Planar {
        unsigned originGreen1 : 1;  // byte 0
        unsigned originRed : 6;
        unsigned detect1 : 1;
        unsigned originBlue1 : 1;   // byte 1
        unsigned originGreen2 : 6;
        unsigned detect2 : 1;
        unsigned originBlue3 : 2;   // byte 2
        unsigned detect4 : 1;
        unsigned originBlue2 : 2;
        unsigned detect3 : 3;
        unsigned horizRed2 : 1;     // byte 3
        unsigned diff : 1;
        unsigned horizRed1 : 5;
        unsigned originBlue4 : 1;
        unsigned horizBlue1 : 1;     // byte 4
        unsigned horizGreen : 7;
        unsigned vertRed1 : 3;      // byte 5
        unsigned horizBlue2 : 5;
        unsigned vertGreen1 : 5;    // byte 6
        unsigned vertRed2 : 3;
        unsigned vertBlue : 6;      // byte 7
        unsigned vertGreen2 : 2;
    };

    uint64_t data64;
    uint32_t data32[2];
    uint8 data8[8];
    Individual individual;
    Differential differential;
    T t;
    H h;
    Planar planar;
};

struct BlockEAC {
    unsigned base : 8;
    unsigned table : 4;
    unsigned multiplier : 4;
    unsigned selectors0 : 8;
    unsigned selectors1 : 8;

    unsigned selectors2 : 8;
    unsigned selectors3 : 8;
    unsigned selectors4 : 8;
    unsigned selectors5 : 8;
};

// ETC2 Modes:
// - ETC1:
//  - two partitions (flip modes): 2*(4x2, 2x4)
//  - two base colors sotred as 444+444 or 555+333
//  - two 3 bit intensity modifiers
// - T Mode. 2 colors 444, 3 bit intensity modifiers, 2 bit indices.
// - H Mode. 2 colors 444, 3 bit intensity modifiers, 2 bit indices.
// - Planar mode: 3 colors 676

struct ETC_Data {
    enum Mode {
        Mode_ETC1,
        Mode_T,
        Mode_H,
        Mode_Planar,
    } mode;

    // @@ It may make more sense to store bit-expanded or even float colors here.
    union {
        struct {
            uint16_t color0;        // 444 or 555
            uint16_t color1;        // 444 or 333
            uint8_t table0;         // 3 bits
            uint8_t table1;         // 3 bits
            bool flip;              // partition mode
            bool diff;              // color encoding
        } etc;
        struct {
            uint16_t color0;        // 444
            uint16_t color1;        // 444
            uint8_t table;          // 3 bits
        } t, h;
        struct {
            uint8_t ro, go, bo;     // 676
            uint8_t rh, gh, bh;     // 676
            uint8_t rv, gv, bv;     // 676
        } planar;
    };

    uint8_t selector[16];           // 2 bit indices (32 bits)
};

// EAC:
// 8 bit base code word
// 4 bit multiplier
// 4 bit table index
// 16 * 3 bit indices.

struct EAC_Data {
    uint8_t alpha;        // 8 bits
    uint8_t multiplier;   // 4 bits
    uint8_t table_index;  // 4 bits
    uint8_t selector[16]; // 3 bit indices
};

static const int etc_th_distances[8] = { 3, 6, 11, 16, 23, 32, 41, 64 };

static const uint8_t etc_selector_scramble[] = { 3, 2, 0, 1 };
static const uint8_t etc_selector_unscramble[] = { 2, 3, 1, 0 };

static const int etc_intensity_modifiers[8][4] = {
    { -8,   -2,  2,   8 },
    { -17,  -5,  5,  17 },
    { -29,  -9,  9,  29 },
    { -42, -13, 13,  42 },
    { -60, -18, 18,  60 },
    { -80, -24, 24,  80 },
    { -106, -33, 33, 106 },
    { -183, -47, 47, 183 }
};

static const int eac_intensity_modifiers[16][8] = {
    {-3, -6, -9, -15, 2, 5, 8, 14}, // 0
    {-3, -7,-10, -13, 2, 6, 9, 12}, // 1
    {-2, -5, -8, -13, 1, 4, 7, 12}, // 2
    {-2, -4, -6, -13, 1, 3, 5, 12}, // 3
    {-3, -6, -8, -12, 2, 5, 7, 11}, // 4
    {-3, -7, -9, -11, 2, 6, 8, 10}, // 5
    {-4, -7, -8, -11, 3, 6, 7, 10}, // 6
    {-3, -5, -8, -11, 2, 4, 7, 10}, // 7
    {-2, -6, -8, -10, 1, 5, 7, 9 }, // 8
    {-2, -5, -8, -10, 1, 4, 7, 9 }, // 9
    {-2, -4, -8, -10, 1, 3, 7, 9 }, // 10
    {-2, -5, -7, -10, 1, 4, 6, 9 }, // 11
    {-3, -4, -7, -10, 2, 3, 6, 9 }, // 12
    {-1, -2, -3, -10, 0, 1, 2, 9 }, // 13
    {-4, -6, -8,  -9, 3, 5, 7, 8 }, // 14
    {-3, -5, -7,  -9, 2, 4, 6, 8 }  // 15
};

/// Return the minimum of two values.
template <typename T>
inline T min(const T & a, const T & b) {
    return (a < b) ? a : b;
}

/// Return the maximum of the two arguments. For floating point values, it returns the second value if the first is NaN.
template <typename T>
inline T max(const T & a, const T & b) {
    return (b < a) ? a : b;
}

/// Clamp between two values.
template <typename T>
inline T clamp(const T & x, const T & a, const T & b) {
    return min(max(x, a), b);
}

static void unpack_etc2_block(const BlockETC * input_block, ETC_Data * data) {
    // @@ input_block is big endian, byte swap first:
    BlockETC block = *input_block;

    // Assume ETC1 for now.
    data->mode = ETC_Data::Mode_ETC1;

    // These are the same for individual and differential blocks.
    data->etc.diff = block.individual.diff != 0;
    data->etc.flip = block.individual.flip != 0;
    data->etc.table0 = block.individual.cw1;
    data->etc.table1 = block.individual.cw2;

    // Decode colors.
    if (data->etc.diff) {
        data->etc.color0 = uint16_t((block.differential.red1 << 10) | (block.differential.green1 << 5) | block.differential.blue1);
        data->etc.color1 = uint16_t((block.differential.dred2 << 6) | (block.differential.dgreen2 << 3) | block.differential.dblue2);

        // @@ Clean this up.
        int dr = block.differential.dred2;
        int dg = block.differential.dgreen2;
        int db = block.differential.dblue2;
        if (dr >= 4) dr -= 8;
        if (dg >= 4) dg -= 8;
        if (db >= 4) db -= 8;
        int r = block.differential.red1 + dr;
        int g = block.differential.green1 + dg;
        int b = block.differential.blue1 + db;

        // Detect ETC2 modes (invalid combinations).
        if (r < 0 || r > 31) {
            data->mode = ETC_Data::Mode_T;
        } else if (g < 0 || g > 31) {
            data->mode = ETC_Data::Mode_H;
        } else if (b < 0 || b > 31) {
            data->mode = ETC_Data::Mode_Planar;
        }
    } else {
        data->etc.color0 = uint16_t((block.individual.red1 << 8) | (block.individual.green1 << 4) | block.individual.blue1);
        data->etc.color1 = uint16_t((block.individual.red2 << 8) | (block.individual.green2 << 4) | block.individual.blue2);
    }

    if (data->mode == ETC_Data::Mode_T) {
        uint16_t r0 = uint16_t((block.t.red1a << 2) | block.t.red1b);
        uint16_t g0 = uint16_t(block.t.green1);
        uint16_t b0 = uint16_t(block.t.blue1);
        data->t.color0 = uint16_t(r0 << 8) | uint16_t(g0 << 4) | b0;

        uint16_t r1 = uint16_t(block.t.red2);
        uint16_t g1 = uint16_t(block.t.green2);
        uint16_t b1 = uint16_t(block.t.blue2);
        data->t.color1 = uint16_t(r1 << 8) | uint16_t(g1 << 4) | b1;

        data->t.table = uint8_t((block.t.da << 1) | block.t.db);
    } else if (data->mode == ETC_Data::Mode_H) {
        uint16_t r0 = uint16_t(block.h.red1);
        uint16_t g0 = uint16_t((block.h.green1a << 1) | block.h.green1b);
        uint16_t b0 = uint16_t((block.h.blue1a << 3) | (block.h.blue1b << 1) | block.h.blue1c);
        data->h.color0 = uint16_t(r0 << 8) | uint16_t(g0 << 4) | b0;

        uint16_t r1 = uint16_t(block.h.red2);
        uint16_t g1 = uint16_t((block.h.green2a << 1) | block.h.green2b);
        uint16_t b1 = uint16_t(block.h.blue2);
        data->h.color1 = uint16_t(r1 << 8) | uint16_t(g1 << 4) | b1;

        data->h.table = uint8_t((block.h.da << 2) | (block.h.db << 1));

        if (data->h.color0 >= data->h.color1) {
            data->h.table++;
        }
    }

    if (data->mode == ETC_Data::Mode_Planar) {
        data->planar.ro = uint8_t(block.planar.originRed);
        data->planar.go = uint8_t((block.planar.originGreen1 << 6) + block.planar.originGreen2);
        data->planar.bo = uint8_t((block.planar.originBlue1 << 5) + (block.planar.originBlue2 << 3) + (block.planar.originBlue3 << 1) + block.planar.originBlue4);

        data->planar.rh = uint8_t((block.planar.horizRed1 << 1) + block.planar.horizRed2);
        data->planar.gh = uint8_t(block.planar.horizGreen);
        data->planar.bh = uint8_t((block.planar.horizBlue1 << 5) + block.planar.horizBlue2);

        data->planar.rv = uint8_t((block.planar.vertRed1 << 3) + block.planar.vertRed2);
        data->planar.gv = uint8_t((block.planar.vertGreen1 << 2) + block.planar.vertGreen2);
        data->planar.bv = uint8_t(block.planar.vertBlue);
    } else {
        // Note, selectors are arranged in columns, keep that order.
        unsigned char * selectors = (uint8 *)&block.individual.selectors;
        for (int i = 0; i < 16; i++) {
            int byte_msb = (1 - (i / 8));
            int byte_lsb = (3 - (i / 8));
            int shift = (i & 7);

            unsigned msb = (selectors[byte_msb] >> shift) & 1;
            unsigned lsb = (selectors[byte_lsb] >> shift) & 1;

            unsigned index = (msb << 1) | lsb;

            if (data->mode == ETC_Data::Mode_ETC1) {
                data->selector[i] = etc_selector_unscramble[index];
            } else {
                // No scrambling in T & H modes.
                data->selector[i] = index;
            }
        }
    }
}

static void unpack_eac_block(const BlockEAC * input_block, EAC_Data * data) {
    data->alpha = input_block->base;
    data->table_index = input_block->table;
    data->multiplier = input_block->multiplier;

    uint64_t selector_bits = 0;
    selector_bits |= uint64_t(input_block->selectors0) << 40;
    selector_bits |= uint64_t(input_block->selectors1) << 32;
    selector_bits |= uint64_t(input_block->selectors2) << 24;
    selector_bits |= uint64_t(input_block->selectors3) << 16;
    selector_bits |= uint64_t(input_block->selectors4) << 8;
    selector_bits |= uint64_t(input_block->selectors5) << 0;
    for (unsigned i = 0; i < 16; i++) {
        unsigned shift = 45 - (3 * i);
        data->selector[i] = (selector_bits >> shift) & 0x7;
    }
}

static float get_alpha8(int base, int table, int mul, int index) {
    int value = ClampByte(base + eac_intensity_modifiers[table][index] * mul);
    return value;
}

static uint16_t get_alpha11(int base, int table, int mul, int index) {
    int elevenbase = base * 8 + 4;
    int tabVal = eac_intensity_modifiers[table][index];
    int elevenTabVal = tabVal * 8;

    if (mul != 0) elevenTabVal *= mul;
    else elevenTabVal /= 8;

    //calculate sum
    int elevenbits = elevenbase + elevenTabVal;

    //clamp..
    if (elevenbits >= 256 * 8) elevenbits = 256 * 8 - 1;
    else if (elevenbits < 0) elevenbits = 0;
    //elevenbits now contains the 11 bit alpha value as defined in the spec.

    //extend to 16 bits before returning, since we don't have any good 11-bit file formats.
    uint16_t sixteenbits = (elevenbits << 5) + (elevenbits >> 6);

    return sixteenbits;
}

// This assumes nin > nout-nin
inline int bitexpand(uint32_t bits, unsigned nin, unsigned nout) {
    assert(nout > nin);
    //assert(nout - nin > nin);
    return (bits << unsigned(nout - nin)) | (bits >> unsigned(2U * nin - nout));
}

// Returns signed r,g,b without bit expansion.
static void unpack_delta_333(uint32_t packed_delta, int * r, int * g, int * b) {
    *r = (packed_delta >> 6) & 7;
    *g = (packed_delta >> 3) & 7;
    *b = packed_delta & 7;
    if (*r >= 4) *r -= 8;
    if (*g >= 4) *g -= 8;
    if (*b >= 4) *b -= 8;
}

// Integer color unpacking for decompressor.
static void unpack_color_444(uint32_t packed_color, int * r, int * g, int * b) {
    int r4 = (packed_color >> 8) & 0xF;
    int g4 = (packed_color >> 4) & 0xF;
    int b4 = packed_color & 0xF;
    *r = r4 << 4 | r4;  // bitexpand(r4, 4, 8);
    *g = g4 << 4 | g4;  // bitexpand(g4, 4, 8);
    *b = b4 << 4 | b4;  // bitexpand(b4, 4, 8);
}

static void unpack_color_555(uint32_t packed_color, int * r, int * g, int * b) {
    int r5 = (packed_color >> 10) & 0x1F;
    int g5 = (packed_color >> 5) & 0x1F;
    int b5 = packed_color & 0x1F;
    *r = (r5 << 3) | (r5 >> 2); // bitexpand(r5, 5, 8);
    *g = (g5 << 3) | (g5 >> 2); // bitexpand(g5, 5, 8);
    *b = (b5 << 3) | (b5 >> 2); // bitexpand(b5, 5, 8);
}

static bool unpack_color_555(uint32_t packed_color, uint32_t packed_delta, int * r, int * g, int * b) {
    int dc_r, dc_g, dc_b;
    unpack_delta_333(packed_delta, &dc_r, &dc_g, &dc_b);

    int r5 = int((packed_color >> 10U) & 0x1F) + dc_r;
    int g5 = int((packed_color >> 5U) & 0x1F) + dc_g;
    int b5 = int(packed_color & 0x1F) + dc_b;

    bool success = true;
    if (static_cast<unsigned>(r5 | g5 | b5) > 31U) {
        success = false;
        r5 = clamp(r5, 0, 31);
        g5 = clamp(g5, 0, 31);
        b5 = clamp(b5, 0, 31);
    }

    *r = (r5 << 3) | (r5 >> 2); // bitexpand(r5, 5, 8);
    *g = (g5 << 3) | (g5 >> 2); // bitexpand(g5, 5, 8);
    *b = (b5 << 3) | (b5 >> 2); // bitexpand(b5, 5, 8);

    return success;
}

static void unpack_color_676(uint32_t packed_color, int * r, int * g, int * b) {
    int r6 = (packed_color >> 13) & 0x3F;
    int g7 = (packed_color >> 6) & 0x7F;
    int b6 = packed_color & 0x3F;

    *r = bitexpand(r6, 6, 8);   // r << 2 | r >> 4
    *g = bitexpand(g7, 7, 8);   // g << 1 | g >> 6
    *b = bitexpand(b6, 6, 8);   // b << 2 | b >> 4
}

static Color32 saturate_color(int R, int G, int B) {
    Color32 c;
    c.r = uint8_t(clamp(R, 0, 255));
    c.g = uint8_t(clamp(G, 0, 255));
    c.b = uint8_t(clamp(B, 0, 255));
    c.a = 255;
    return c;
}

static void get_diff_subblock_palette(uint16_t packed_color, unsigned table_idx, Color32 palette[4]) {
    assert(table_idx < 8);

    const int * intensity_table = etc_intensity_modifiers[table_idx];

    int r, g, b;
    unpack_color_555(packed_color, &r, &g, &b);

    for (int i = 0; i < 4; i++) {
        const int y = intensity_table[i];
        palette[i] = saturate_color(r + y, g + y, b + y);
    }
}

static bool get_diff_subblock_palette(uint16_t packed_color, uint16_t packed_delta, unsigned table_idx, Color32 palette[4]) {
    assert(table_idx < 8);

    const int * intensity_table = etc_intensity_modifiers[table_idx];

    int r, g, b;
    bool success = unpack_color_555(packed_color, packed_delta, &r, &g, &b);

    for (int i = 0; i < 4; i++) {
        const int y = intensity_table[i];
        palette[i] = saturate_color(r + y, g + y, b + y);
    }

    return success;
}

static void get_abs_subblock_palette(uint16_t packed_color, unsigned table_idx, Color32 palette[4]) {
    assert(table_idx < 8);

    const int * intensity_table = etc_intensity_modifiers[table_idx];

    int r, g, b;
    unpack_color_444(packed_color, &r, &g, &b);

    for (int i = 0; i < 4; i++) {
        const int y = intensity_table[i];
        palette[i] = saturate_color(r + y, g + y, b + y);
    }
}

static int get_selector(const ETC_Data & data, int x, int y) {
    // Note selectors are arranged in column order.
    return data.selector[x * 4 + y];
}

static int get_partition(const ETC_Data & data, int x, int y) {
    assert(data.mode == ETC_Data::Mode_ETC1);
    return data.etc.flip ? y > 1 : x > 1;
}

static void decode_etc1(const ETC_Data & data, Color32 colors[16]) {
    assert(data.mode == ETC_Data::Mode_ETC1);

    Color32 palette[2][4];

    if (data.etc.diff) {
        // Decode colors in 555+333 mode.
        get_diff_subblock_palette(data.etc.color0, data.etc.table0, palette[0]);
        get_diff_subblock_palette(data.etc.color0, data.etc.color1, data.etc.table1, palette[1]);
    } else {
        // Decode colors in 444,444 mode.
        get_abs_subblock_palette(data.etc.color0, data.etc.table0, palette[0]);
        get_abs_subblock_palette(data.etc.color1, data.etc.table1, palette[1]);
    }

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            colors[y * 4 + x] = palette[get_partition(data, x, y)][get_selector(data, x, y)];
        }
    }
}

static void decode_etc2_t(const ETC_Data & data, Color32 output_colors[16]) {
    assert(data.mode == ETC_Data::Mode_T);

    int r, g, b;
    Color32 palette[4];

    int d = etc_th_distances[data.t.table];

    unpack_color_444(data.t.color0, &r, &g, &b);
    palette[0] = saturate_color(r, g, b);

    unpack_color_444(data.t.color1, &r, &g, &b);
    palette[1] = saturate_color(r + d, g + d, b + d);
    palette[2] = saturate_color(r, g, b);
    palette[3] = saturate_color(r - d, g - d, b - d);

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            output_colors[y * 4 + x] = palette[get_selector(data, x, y)];
        }
    }
}

static void decode_etc2_h(const ETC_Data & data, Color32 output_colors[16]) {
    assert(data.mode == ETC_Data::Mode_H);

    int r, g, b;
    Color32 palette[4];

    int d = etc_th_distances[data.t.table];

    unpack_color_444(data.t.color0, &r, &g, &b);
    palette[0] = saturate_color(r + d, g + d, b + d);
    palette[1] = saturate_color(r - d, g - d, b - d);

    unpack_color_444(data.t.color1, &r, &g, &b);
    palette[2] = saturate_color(r + d, g + d, b + d);
    palette[3] = saturate_color(r - d, g - d, b - d);

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            output_colors[y * 4 + x] = palette[get_selector(data, x, y)];
        }
    }
}

static void decode_etc2_planar(const ETC_Data & data, Color32 output_colors[16]) {
    assert(data.mode == ETC_Data::Mode_Planar);

    int ro, go, bo; // origin color
    int rh, gh, bh; // horizontal color
    int rv, gv, bv; // vertical color

    // Unpack from 676
    ro = bitexpand(data.planar.ro, 6, 8);   // r << 2 | r >> 4
    go = bitexpand(data.planar.go, 7, 8);   // g << 1 | g >> 6
    bo = bitexpand(data.planar.bo, 6, 8);

    rh = bitexpand(data.planar.rh, 6, 8);
    gh = bitexpand(data.planar.gh, 7, 8);
    bh = bitexpand(data.planar.bh, 6, 8);

    rv = bitexpand(data.planar.rv, 6, 8);
    gv = bitexpand(data.planar.gv, 7, 8);
    bv = bitexpand(data.planar.bv, 6, 8);

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            int r = (4 * ro + x * (rh - ro) + y * (rv - ro) + 2) >> 2;
            int g = (4 * go + x * (gh - go) + y * (gv - go) + 2) >> 2;
            int b = (4 * bo + x * (bh - bo) + y * (bv - bo) + 2) >> 2;

            int idx = 4 * y + x;
            output_colors[idx].r = r;
            output_colors[idx].g = g;
            output_colors[idx].b = b;
            output_colors[idx].a = 255;
        }
    }
}

static void decode_etc2(const ETC_Data & data, Color32 colors[16]) {
    if (data.mode == ETC_Data::Mode_ETC1) {
        decode_etc1(data, colors);
    } else if (data.mode == ETC_Data::Mode_T) {
        decode_etc2_t(data, colors);
    } else if (data.mode == ETC_Data::Mode_H) {
        decode_etc2_h(data, colors);
    } else /*if (data.mode == ETC_Data::Mode_Planar)*/ {
        decode_etc2_planar(data, colors);
    }
}

static void decode_eac_8(const EAC_Data & data, uint8_t *output_colors, int output_channel = 3) {
    for (int i = 0; i < 16; i++) {
        int s = data.selector[4 * (i % 4) + i / 4];
        output_colors[output_channel * i] = get_alpha8(data.alpha, data.table_index, data.multiplier, s);
    }
}

static void decode_eac_11(const EAC_Data & data, uint16_t *output_colors, int output_channel = 0) {
    for (int i = 0; i < 16; i++) {
        int s = data.selector[4 * (i % 4) + i / 4];
        output_colors[output_channel * i] = get_alpha11(data.alpha, data.table_index, data.multiplier, s);
    }
}

//-------------------------------------------------------------------------------------------------

static void DecodeETC2_RGB8ToRGBA8(const byte *src, const int width, const int height, const int depth, byte *out) {
    ALIGN_AS16 byte unpackedBlock[64];
    
    // Fill alpha channel first
    memset(unpackedBlock, 255, sizeof(unpackedBlock));

    for (int y = 0; y < height; y += 4) {
        byte *dstPtr = out + 4 * width * y;

        int dstBlockHeight = Min(4, height - y);

        for (int x = 0; x < width; x += 4) {
#if 0
            ETC_Data etc;
            unpack_etc2_block((const BlockETC *)src, &etc);
            src += 8;

            decode_etc2(etc, (Color32 *)unpackedBlock);
#else
            // ETC2 RGB block
            uint32_t block1;
            uint32_t block2;
            ReadColorBlockETC2(src, block1, block2);
            etcpack::decompressBlockETC2c(block1, block2, unpackedBlock, 4, 4, 0, 0, 4);
            src += 8;
#endif

            int dstBlockWidth = Min(4, width - x);

            const byte *srcPtr = unpackedBlock;

            for (int by = 0; by < dstBlockHeight; by++) {
                memcpy(dstPtr + by * 4 * width, srcPtr, 4 * dstBlockWidth);

                srcPtr += 4 * 4;
            }

            dstPtr += 4 * dstBlockWidth;
        }
    }
}

static void DecodeETC2_RGB8A1ToRGBA8(const byte *src, const int width, const int height, const int depth, byte *out) {
    ALIGN_AS16 byte unpackedBlock[64];

    for (int y = 0; y < height; y += 4) {
        byte *dstPtr = out + 4 * width * y;

        int dstBlockHeight = Min(4, height - y);

        for (int x = 0; x < width; x += 4) {
#if 0
            ETC_Data etc;
            unpack_etc2_block((const BlockETC *)src, &etc);
            src += 8;

            decode_etc2(etc, (Color32 *)unpackedBlock);
#else
            // ETC2 RGB/punchthrough alpha block 
            uint32_t block1;
            uint32_t block2;
            ReadColorBlockETC2(src, block1, block2);
            etcpack::decompressBlockETC21BitAlphaC(block1, block2, unpackedBlock, nullptr, 4, 4, 0, 0, 4);
            src += 8;
#endif
            int dstBlockWidth = Min(4, width - x);

            const byte *srcPtr = unpackedBlock;

            for (int by = 0; by < dstBlockHeight; by++) {
                memcpy(dstPtr + 4 * width * by, srcPtr, 4 * dstBlockWidth);

                srcPtr += 4 * 4;
            }

            dstPtr += 4 * dstBlockWidth;
        }
    }
}

static void DecodeETC2_RGBA8ToRGBA8(const byte *src, const int width, const int height, const int depth, byte *out) {
    ALIGN_AS16 byte unpackedBlock[64];

    for (int y = 0; y < height; y += 4) {
        byte *dstPtr = out + 4 * width * y;

        int dstBlockHeight = Min(4, height - y);

        for (int x = 0; x < width; x += 4) {
#if 1
            EAC_Data eac;
            unpack_eac_block((const BlockEAC *)src, &eac);
            src += 8;

            ETC_Data etc;
            unpack_etc2_block((const BlockETC *)src, &etc);
            src += 8;

            decode_etc2(etc, (Color32 *)unpackedBlock);
            decode_eac_8(eac, (uint8_t *)unpackedBlock + 3, 4);
#else
            // EAC block
            etcpack::decompressBlockAlphaC(const_cast<byte *>(src), unpackedBlock + 3, 4, 4, 0, 0, 4);
            src += 8;

            // ETC2 RGB block
            uint32_t block1;
            uint32_t block2;
            ReadColorBlockETC2(src, block1, block2);
            etcpack::decompressBlockETC2c(block1, block2, unpackedBlock, 4, 4, 0, 0, 4);
            src += 8;
#endif

            int dstBlockWidth = Min(4, width - x);

            const byte *srcPtr = unpackedBlock;

            for (int by = 0; by < dstBlockHeight; by++) {
                memcpy(dstPtr + 4 * width * by, srcPtr, 4 * dstBlockWidth);

                srcPtr += 4 * 4;
            }

            dstPtr += 4 * dstBlockWidth;
        }
    }
}

static void DecodeEAC_R11ToRGBA32F(const byte *src, const int width, const int height, const int depth, bool signedFormat, float *out) {
    ALIGN_AS16 byte unpackedBlock[64];

    etcpack::formatSigned = signedFormat;

    for (int y = 0; y < height; y += 4) {
        float *dstPtr = out + 4 * width * y;

        int dstBlockHeight = Min(4, height - y);

        for (int x = 0; x < width; x += 4) {
#if 1
            EAC_Data eac;
            unpack_eac_block((const BlockEAC *)src, &eac);
            decode_eac_11(eac, (uint16_t *)unpackedBlock, 2);
            src += 8;
#else
            // EAC block
            etcpack::decompressBlockAlpha16bitC(const_cast<byte *>(src), unpackedBlock, 4, 4, 0, 0, 1);
            src += 8;
#endif

            float fdata[4 * 16];
            if (signedFormat) {
                for (int i = 0; i < 16; i++) {
                    fdata[4 * i + 0] = (((int16_t *)unpackedBlock)[i] + 0.5f) / (32767.0f + 0.5f);
                    fdata[4 * i + 1] = 0;
                    fdata[4 * i + 2] = 0;
                    fdata[4 * i + 3] = 255;
                }
            } else {
                for (int i = 0; i < 16; i++) {
                    fdata[4 * i + 0] = ((uint16_t *)unpackedBlock)[i] / 65535.0f;
                    fdata[4 * i + 1] = 0;
                    fdata[4 * i + 2] = 0;
                    fdata[4 * i + 3] = 255;
                }
            }

            int dstBlockWidth = Min(4, width - x);

            const float *srcPtr = (float *)fdata;

            for (int by = 0; by < dstBlockHeight; by++) {
                memcpy(dstPtr + 4 * width * by, srcPtr, sizeof(float) * 4 * dstBlockWidth);

                srcPtr += 4 * 4;
            }

            dstPtr += 4 * dstBlockWidth;
        }
    }
}

static void DecodeEAC_RG11ToRGBA32F(const byte *src, const int width, const int height, const int depth, bool signedFormat, bool normal, float *out) {
    ALIGN_AS16 byte unpackedBlock[64];

    etcpack::formatSigned = signedFormat;

    for (int y = 0; y < height; y += 4) {
        float *dstPtr = out + 4 * width * y;

        int dstBlockHeight = Min(4, height - y);

        for (int x = 0; x < width; x += 4) {
#if 1
            EAC_Data eac;
            unpack_eac_block((const BlockEAC *)src, &eac);
            decode_eac_11(eac, (uint16_t *)unpackedBlock, 2);
            src += 8;

            unpack_eac_block((const BlockEAC *)src, &eac);
            decode_eac_11(eac, (uint16_t *)unpackedBlock + 1, 2);
            src += 8;
#else
            // EAC block
            etcpack::decompressBlockAlpha16bitC(const_cast<byte *>(src), unpackedBlock, 4, 4, 0, 0, 2);
            src += 8;
            // EAC block
            etcpack::decompressBlockAlpha16bitC(const_cast<byte *>(src), unpackedBlock + 2, 4, 4, 0, 0, 2);
            src += 8;
#endif
            float fdata[4 * 16];
            if (signedFormat) {
                for (int i = 0; i < 16; i++) {
                    fdata[4 * i + 0] = (((int16_t *)unpackedBlock)[2 * i + 0] + 0.5f) / (32767.0f + 0.5f);
                    fdata[4 * i + 1] = (((int16_t *)unpackedBlock)[2 * i + 1] + 0.5f) / (32767.0f + 0.5f);
                    fdata[4 * i + 2] = 0.0f;
                    fdata[4 * i + 3] = 1.0f;
                }
            } else {
                for (int i = 0; i < 16; i++) {
                    fdata[4 * i + 0] = (((uint16_t *)unpackedBlock)[2 * i + 0] / 65535.0f) * 2.0f - 1.0f;
                    fdata[4 * i + 1] = (((uint16_t *)unpackedBlock)[2 * i + 1] / 65535.0f) * 2.0f - 1.0f;
                    fdata[4 * i + 2] = 0.0f;
                    fdata[4 * i + 3] = 1.0f;
                }
            }

            if (normal) {
                for (int i = 0; i < 16; i++) {
                    float x = fdata[4 * i + 0];
                    float y = fdata[4 * i + 1];

                    fdata[4 * i + 2] = Math::Sqrt(Math::Fabs(1.0f - x * x - y * y));
                }
            }

            int dstBlockWidth = Min(4, width - x);

            const float *srcPtr = fdata;

            for (int by = 0; by < dstBlockHeight; by++) {
                memcpy(dstPtr + 4 * width * by, srcPtr, sizeof(float) * 4 * dstBlockWidth);

                srcPtr += 4 * 4;
            }

            dstPtr += 4 * dstBlockWidth;
        }
    }
}

void DecompressETC2_RGB8(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::RGBA_8_8_8_8);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            const byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            DecodeETC2_RGB8ToRGBA8(src, w, h, d, dst);
        }
    }
}

void DecompressETC2_RGB8A1(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::RGBA_8_8_8_8);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            const byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            DecodeETC2_RGB8A1ToRGBA8(src, w, h, d, dst);
        }
    }
}

void DecompressETC2_RGBA8(const Image &srcImage, Image &dstImage) {
    assert(dstImage.GetFormat() == Image::Format::RGBA_8_8_8_8);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            const byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            DecodeETC2_RGBA8ToRGBA8(src, w, h, d, dst);
        }
    }
}

void DecompressEAC_R11(const Image &srcImage, Image &dstImage, bool signedFormat) {
    assert(dstImage.GetFormat() == Image::Format::RGBA_32F_32F_32F_32F);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            const byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            DecodeEAC_R11ToRGBA32F(src, w, h, d, signedFormat, (float *)dst);
        }
    }
}

void DecompressEAC_RG11(const Image &srcImage, Image &dstImage, bool signedFormat, bool normal) {
    assert(dstImage.GetFormat() == Image::Format::RGBA_32F_32F_32F_32F);

    int numMipmaps = srcImage.NumMipmaps();
    int numSlices = srcImage.NumSlices();

    for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
        int w = srcImage.GetWidth(mipLevel);
        int h = srcImage.GetHeight(mipLevel);
        int d = srcImage.GetDepth(mipLevel);

        for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
            const byte *src = srcImage.GetPixels(mipLevel, sliceIndex);
            byte *dst = dstImage.GetPixels(mipLevel, sliceIndex);

            DecodeEAC_RG11ToRGBA32F(src, w, h, d, signedFormat, normal, (float *)dst);
        }
    }
}

BE_NAMESPACE_END
