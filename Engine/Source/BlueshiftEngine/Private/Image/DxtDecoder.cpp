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
#include "Image/DxtDecoder.h"

BE_NAMESPACE_BEGIN

// Decode 64 bits color block to RGBA8888
void DXTDecoder::DecodeColorBlock(const DXTBlock::ColorBlock *block, byte *out, bool writeAlpha) {
    byte colors[4][4];

    RGB888From565(block->color0, colors[0]);
    RGB888From565(block->color1, colors[1]);

    colors[0][3] = 255;
    colors[1][3] = 255;

    // 4-color or 3-color & 1-transparent color block ?
    if (block->color0 > block->color1) {
        // Four-color block: derive the other two colors
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
        colors[2][0] = (2 * colors[0][0] + 1 * colors[1][0]) / 3;
        colors[2][1] = (2 * colors[0][1] + 1 * colors[1][1]) / 3;
        colors[2][2] = (2 * colors[0][2] + 1 * colors[1][2]) / 3;
        colors[2][3] = 255;

        colors[3][0] = (1 * colors[0][0] + 2 * colors[1][0]) / 3;
        colors[3][1] = (1 * colors[0][1] + 2 * colors[1][1]) / 3;
        colors[3][2] = (1 * colors[0][2] + 2 * colors[1][2]) / 3;
        colors[3][3] = 255;
    } else {
        // Three-color block: derive the other colors
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = transparent
        colors[2][0] = (colors[0][0] + colors[1][0]) / 2;
        colors[2][1] = (colors[0][1] + colors[1][1]) / 2;
        colors[2][2] = (colors[0][2] + colors[1][2]) / 2;
        colors[2][3] = 255;

        // random color to indicate alphas
        colors[3][0] = 0x00;
        colors[3][1] = 255;
        colors[3][2] = 255;
        colors[3][3] = 0x00;
    }

    uint32_t indexes = block->indexes;

    for (int i = 0; i < 16; i++) {
        out[i * 4 + 0] = colors[indexes & 3][0];
        out[i * 4 + 1] = colors[indexes & 3][1];
        out[i * 4 + 2] = colors[indexes & 3][2];
        if (writeAlpha) {
            out[i * 4 + 3] = colors[indexes & 3][3];
        }
        indexes >>= 2;
    }
}

// Decode 64 bits alpha block to 8-bit alphas
void DXTDecoder::DecodeAlphaBlock(const DXTBlock::AlphaBlock *block, byte *out) {
    byte alphas[8];

    alphas[0] = block->alpha0;
    alphas[1] = block->alpha1;

    // 8-alpha or 6-alpha block?
    if (block->alpha0 > block->alpha1) {
        // 8-alpha block: derive the other 6 alphas.
        // 000 = alpha_0, 001 = alpha_1, others are interpolated
        alphas[2] = (6 * alphas[0] + 1 * alphas[1]) / 7;    // 010
        alphas[3] = (5 * alphas[0] + 2 * alphas[1]) / 7;    // 011
        alphas[4] = (4 * alphas[0] + 3 * alphas[1]) / 7;    // 100
        alphas[5] = (3 * alphas[0] + 4 * alphas[1]) / 7;    // 101
        alphas[6] = (2 * alphas[0] + 5 * alphas[1]) / 7;    // 110
        alphas[7] = (1 * alphas[0] + 6 * alphas[1]) / 7;    // 111
    } else {
        // 6-alpha block: derive the other alphas.
        // 000 = alpha_0, 001 = alpha_1, others are interpolated
        alphas[2] = (4 * alphas[0] + 1 * alphas[1]) / 5;    // 010
        alphas[3] = (3 * alphas[0] + 2 * alphas[1]) / 5;    // 011
        alphas[4] = (2 * alphas[0] + 3 * alphas[1]) / 5;    // 100
        alphas[5] = (1 * alphas[0] + 4 * alphas[1]) / 5;    // 101
        alphas[6] = 0;                                      // 110
        alphas[7] = 255;                                    // 111
    }

    unsigned int idx01 = (int)block->indexes[0] | ((int)block->indexes[1] << 8) | ((int)block->indexes[2] << 16);
    unsigned int idx23 = (int)block->indexes[3] | ((int)block->indexes[4] << 8) | ((int)block->indexes[5] << 16);

    for (int i = 0; i < 8; i++) {
        out[i * 4] = alphas[idx01 & 7];
        idx01 >>= 3;
    }

    for (int i = 8; i < 16; i++) {
        out[i * 4] = alphas[idx23 & 7];
        idx23 >>= 3;
    }
}

// Decode 64 bits alpha block (4 bits per pixel) to 8-bit alphas
void DXTDecoder::DecodeAlphaExplicitBlock(const DXTBlock::AlphaExplicitBlock *block, byte *out) {
    uint16_t    bits;
    byte        alpha;

    for (int y = 0; y < 4; y++) {
        bits = block->row[y];

        for (int x = 0; x < 4; x++, bits >>= 4) {
            alpha = (bits & 0x000F);
            *out = (alpha << 4) | alpha;
            out += 4;
        }
    }
}

void DXTDecoder::DecompressImageDXT1(const DXTBlock *dxtBlock, const int width, const int height, const int depth, byte *out) {
    ALIGN16(byte unpackedBlock[64]);

    for (int z = 0; z < depth; z++) {
        byte *dst_z = out + 4 * (width * height * z);

        for (int y = 0; y < height; y += 4) {
            byte *dstPtr = dst_z + 4 * width * y;

            int dstBlockHeight = Min(4, height - y);

            for (int x = 0; x < width; x += 4, dstPtr += 4 * 4) {
                DXTDecoder::DecodeColorBlock(&dxtBlock->colorBlock, unpackedBlock, true);
                dxtBlock++;

                byte *srcPtr = unpackedBlock;

                int dstBlockWidth = Min(4, width - x);

                for (int i = 0; i < dstBlockHeight; i++, srcPtr += 4 * 4) {
                    memcpy(dstPtr + i * 4 * width, srcPtr, dstBlockWidth * 4);
                }
            }
        }
    }
}

void DXTDecoder::DecompressImageDXT3(const DXTBlock *dxtBlock, const int width, const int height, const int depth, byte *out) {
    ALIGN16(byte unpackedBlock[64]);

    for (int z = 0; z < depth; z++) {
        byte *dst_z = out + 4 * (width * height * z);

        for (int y = 0; y < height; y += 4) {
            byte *dstPtr = dst_z + 4 * width * y;

            int dstBlockHeight = Min(4, height - y);

            for (int x = 0; x < width; x += 4, dstPtr += 4 * 4) {
                DXTDecoder::DecodeAlphaExplicitBlock(&dxtBlock->alphaExplicitBlock, unpackedBlock + 3);
                dxtBlock++;
                DXTDecoder::DecodeColorBlock(&dxtBlock->colorBlock, unpackedBlock, false);
                dxtBlock++;

                byte *srcPtr = unpackedBlock;

                int dstBlockWidth = Min(4, width - x);

                for (int i = 0; i < dstBlockHeight; i++, srcPtr += 4 * 4) {
                    memcpy(dstPtr + i * 4 * width, srcPtr, dstBlockWidth * 4);
                }
            }
        }
    }
}

void DXTDecoder::DecompressImageDXT5(const DXTBlock *dxtBlock, const int width, const int height, const int depth, byte *out) {
    ALIGN16(byte unpackedBlock[64]);

    for (int z = 0; z < depth; z++) {
        byte *dst_z = out + 4 * (width * height * z);

        for (int y = 0; y < height; y += 4) {
            byte *dstPtr = dst_z + 4 * width * y;

            int dstBlockHeight = Min(4, height - y);

            for (int x = 0; x < width; x += 4, dstPtr += 4 * 4) {
                DXTDecoder::DecodeAlphaBlock(&dxtBlock->alphaBlock, unpackedBlock + 3);
                dxtBlock++;
                DXTDecoder::DecodeColorBlock(&dxtBlock->colorBlock, unpackedBlock, false);
                dxtBlock++;

                byte *srcPtr = unpackedBlock;

                int dstBlockWidth = Min(4, width - x);

                for (int i = 0; i < dstBlockHeight; i++, srcPtr += 4 * 4) {
                    memcpy(dstPtr + i * 4 * width, srcPtr, dstBlockWidth * 4);                    
                }
            }
        }
    }
}

void DXTDecoder::DecompressImageDXN2(const DXTBlock *dxtBlock, const int width, const int height, const int depth, byte *out) {
    ALIGN16(byte unpackedBlock[64]);

    for (int z = 0; z < depth; z++) {
        byte *dst_z = out + 4 * (width * height * z);

        for (int y = 0; y < height; y += 4) {
            byte *dstPtr = dst_z + 4 * width * y;

            int dstBlockHeight = Min(4, height - y);

            for (int x = 0; x < width; x += 4, dstPtr += 4 * 4) {
                DXTDecoder::DecodeAlphaBlock(&dxtBlock->alphaBlock, unpackedBlock);
                dxtBlock++;
                DXTDecoder::DecodeAlphaBlock(&dxtBlock->alphaBlock, unpackedBlock + 1);
                dxtBlock++;

                float normals[16 * 4];
                for (int i = 0; i < 16; i++) {
                    normals[i * 4 + 0] = unpackedBlock[i * 4 + 0] / 255.0f * 2.0f - 1.0f;
                    normals[i * 4 + 1] = unpackedBlock[i * 4 + 1] / 255.0f * 2.0f - 1.0f;
                }

                for (int i = 0; i < 16; i++) {
                    float x = normals[i * 4 + 0];
                    float y = normals[i * 4 + 1];
                    float z = 1.0f - x * x - y * y;
                    if (z < 0.0f) z = 0.0f;
                    normals[i * 4 + 2] = sqrt(z);
                }

                for (int i = 0; i < 16; i++) {
                    unpackedBlock[i * 4 + 0] = Math::Ftob((normals[i * 4 + 0] + 1.0f) / 2.0f * 255.0f);
                    unpackedBlock[i * 4 + 1] = Math::Ftob((normals[i * 4 + 1] + 1.0f) / 2.0f * 255.0f);
                    unpackedBlock[i * 4 + 2] = Math::Ftob((normals[i * 4 + 2] + 1.0f) / 2.0f * 255.0f);
                    unpackedBlock[i * 4 + 3] = 255;
                }

                byte *srcPtr = unpackedBlock;

                int dstBlockWidth = Min(4, width - x); 

                for (int i = 0; i < dstBlockHeight; i++, srcPtr += 4 * 4) {
                    memcpy(dstPtr + i * 4 * width, srcPtr, dstBlockWidth * 4);
                }
            }
        }
    }
}

BE_NAMESPACE_END
