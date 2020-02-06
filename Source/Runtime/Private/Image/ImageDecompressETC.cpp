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

static void DecodeETC2_RGB8ToRGBA8(const byte *src, const int width, const int height, const int depth, byte *out) {
    ALIGN_AS16 byte unpackedBlock[64];
    uint32_t block1;
    uint32_t block2;

    // Fill alpha channel first
    memset(unpackedBlock, 255, sizeof(unpackedBlock));

    for (int y = 0; y < height; y += 4) {
        byte *dstPtr = out + 4 * width * y;

        int dstBlockHeight = Min(4, height - y);

        for (int x = 0; x < width; x += 4) {
            // ETC2 RGB block
            ReadColorBlockETC2(src, block1, block2);
            src += 8;
            etcpack::decompressBlockETC2c(block1, block2, unpackedBlock, 4, 4, 0, 0, 4);

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
    uint32_t block1;
    uint32_t block2;

    for (int y = 0; y < height; y += 4) {
        byte *dstPtr = out + 4 * width * y;

        int dstBlockHeight = Min(4, height - y);

        for (int x = 0; x < width; x += 4) {
            // ETC2 RGB/punchthrough alpha block 
            ReadColorBlockETC2(src, block1, block2);
            src += 8;
            etcpack::decompressBlockETC21BitAlphaC(block1, block2, unpackedBlock, nullptr, 4, 4, 0, 0, 4);

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
    uint32_t block1;
    uint32_t block2;

    for (int y = 0; y < height; y += 4) {
        byte *dstPtr = out + 4 * width * y;

        int dstBlockHeight = Min(4, height - y);

        for (int x = 0; x < width; x += 4) {
            // EAC block
            etcpack::decompressBlockAlphaC(const_cast<byte *>(src), unpackedBlock + 3, 4, 4, 0, 0, 4);
            src += 8;
            // ETC2 RGB block
            ReadColorBlockETC2(src, block1, block2);
            src += 8;
            etcpack::decompressBlockETC2c(block1, block2, unpackedBlock, 4, 4, 0, 0, 4);

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
            // EAC block
            etcpack::decompressBlockAlpha16bitC(const_cast<byte *>(src), unpackedBlock, 4, 4, 0, 0, 1);
            src += 8;

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
            // FIXME: a bug !
            // EAC block
            etcpack::decompressBlockAlpha16bitC(const_cast<byte *>(src), unpackedBlock, 4, 4, 0, 0, 2);
            src += 8;
            // EAC block
            etcpack::decompressBlockAlpha16bitC(const_cast<byte *>(src), unpackedBlock + 2, 4, 4, 0, 0, 2);
            src += 8;

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
