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

#pragma once

#include "BCCodec.h"

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------
//
// BC Encoder
// http://mrelusive.com/publications/papers/Real-Time-Dxt-Compression.pdf
//
//--------------------------------------------------------------------------------

class BE_API BCEncoder : public BCCodec {
public:
    static void             CompressImageBC1Fast(const byte *src, const int width, const int height, const int depth, byte *dst);
    static void             CompressImageBC1HQ(const byte *src, const int width, const int height, const int depth, byte *dst);
    
    static void             CompressImageBC2Fast(const byte *src, const int width, const int height, const int depth, byte *dst);
    static void             CompressImageBC2HQ(const byte *src, const int width, const int height, const int depth, byte *dst);
    
    static void             CompressImageBC3Fast(const byte *src, const int width, const int height, const int depth, byte *dst);
    static void             CompressImageBC3HQ(const byte *src, const int width, const int height, const int depth, byte *dst);
    
    static void             CompressImageBC4Fast(const byte *src, const int width, const int height, const int depth, byte *dst);
    static void             CompressImageBC4HQ(const byte *src, const int width, const int height, const int depth, byte *dst);

    static void             CompressImageBC5Fast(const byte *src, const int width, const int height, const int depth, byte *dst);
    static void             CompressImageBC5HQ(const byte *src, const int width, const int height, const int depth, byte *dst);

private:
                            /// Extracts a 4x4 block from the texture and stores it in a fixed size buffer.
    static void             ExtractBlock(const byte *src, int srcPitch, int blockWidth, int blockHeight, byte *colorBlock);

                            /// Takes the extents of the bounding box of the colors in the 4x4 block in RGB space. 
                            /// Also finds the minimum and maximum alpha values.
    static void             GetMinMaxBBox(const byte *colorBlock, byte *minColor, byte *maxColor);
    static void             GetMinMaxAlpha(const byte *colorBlock, const int alphaOffset, byte *minColor, byte *maxColor);
    static void             GetMinMaxColorHQ(const byte *colorBlock, byte *minColor, byte *maxColor, const bool ignoreTransparentPixels = false);

    static void             InsetColorsBBox(byte *minColor, byte *maxColor);
    static void             InsetNormalsBBoxBC5(byte *minNormal, byte *maxNormal);

                            /// Finds the two RGB colors in a 4x4 block furthest apart. Also finds the two alpha values furthest apart.
    static void             GetMinMaxColorsMaxDist(const byte *colorBlock, byte *minColor, byte *maxColor);

    static unsigned int     AlphaDistance(const byte a1, const byte a2);
    static unsigned int     ColorDistance(const byte *c1, const byte *c2);
    static unsigned int     ColorDistanceWeighted(const byte *c1, const byte *c2);

    static void             ComputeAlphaIndicesFast(const byte *colorBlock, const int alphaOffset, const byte maxAlpha, const byte minAlpha, byte *out);
    static int              ComputeAlphaIndices(const byte *colorBlock, const int alphaOffset, const byte alpha0, const byte alpha1, byte *rindexes);
    static void             Compute4BitsAlpha(const byte *colorBlock, const int alphaOffset, uint16_t *out);

    static void             ComputeColorIndicesFast(const byte *colorBlock, const byte *maxColor, const byte *minColor, uint32_t *out);
    static int              ComputeColorIndices(const byte *colorBlock, const uint16_t color0, const uint16_t color1, uint32_t &result, const bool usePunchThroughAlpha = false);

    static void             EncodeBC1BlockFast(const byte *src, byte **dstPtr);
    static void             EncodeBC2BlockFast(const byte *src, byte **dstPtr);
    static void             EncodeBC3BlockFast(const byte *src, byte **dstPtr);
    static void             EncodeBC4BlockFast(const byte *src, byte **dstPtr);
    static void             EncodeBC5BlockFast(const byte *src, byte **dstPtr);

    static void             EncodeBC1BlockHQ(const byte *src, byte **dstPtr);
    static void             EncodeBC2BlockHQ(const byte *src, byte **dstPtr);
    static void             EncodeBC3BlockHQ(const byte *src, byte **dstPtr);
    static void             EncodeBC4BlockHQ(const byte *src, byte **dstPtr);
    static void             EncodeBC5BlockHQ(const byte *src, byte **dstPtr);
};

BE_INLINE unsigned int BCEncoder::AlphaDistance(const byte a1, const byte a2) {
    int d = a1 - a2;
    return d * d;
}

BE_INLINE unsigned int BCEncoder::ColorDistance(const byte *c1, const byte *c2) {
    int d0 = c1[0] - c2[0];
    int d1 = c1[1] - c2[1];
    int d2 = c1[2] - c2[2];
    return d0 * d0 + d1 * d1 + d2 * d2;
}

BE_INLINE unsigned int BCEncoder::ColorDistanceWeighted(const byte *c1, const byte *c2) {
    int r, g, b;
    int rmean;

    // http://www.compuphase.com/cmetric.htm
    rmean = ((int)c1[0] + (int)c2[0]) / 2;
    r = (int)c1[0] - (int)c2[0];
    g = (int)c1[1] - (int)c2[1];
    b = (int)c1[2] - (int)c2[2];
    return (((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8);
}

BE_NAMESPACE_END
