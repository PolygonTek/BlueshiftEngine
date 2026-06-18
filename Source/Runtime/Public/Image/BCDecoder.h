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
// BC Decoder
//
//--------------------------------------------------------------------------------

class BE_API BCDecoder : public BCCodec {
public:
    static void             DecompressImageBC1(const byte *blocks, const int width, const int height, const int depth, byte *out);
    static void             DecompressImageBC2(const byte *blocks, const int width, const int height, const int depth, byte *out);
    static void             DecompressImageBC3(const byte *blocks, const int width, const int height, const int depth, byte *out);
    static void             DecompressImageBC4(const byte *blocks, const int width, const int height, const int depth, byte *out);
    static void             DecompressImageBC5(const byte *blocks, const int width, const int height, const int depth, byte *out);
    static void             DecompressImageBC6H(const byte *blocks, const int width, const int height, const int depth, bool signedFormat, float *out);
    static void             DecompressImageBC6HU(const byte *blocks, const int width, const int height, const int depth, float *out);
    static void             DecompressImageBC6HS(const byte *blocks, const int width, const int height, const int depth, float *out);

private:
                            /// Decode 64 bits color block to RGBA8888
    static void	            DecodeColorBlock(const BCCodec::ColorBlock565 *block, byte *out, bool writeAlpha);

                            /// Decode 64 bits alpha block to 8-bit alpha
    static void	            DecodeAlphaBlock(const BCCodec::AlphaBlock3Bit *block, byte *out);

                            /// Decode 64 bits alpha block (4 bits per pixel) to 8-bit alpha
    static void	            DecodeAlphaExplicitBlock(const BCCodec::AlphaBlock4Bit *block, byte *out);

                            /// Decode one unsigned BC6H block to 16 RGBA32F texels. Alpha is set to 1.0.
    static void             DecodeBC6HUBlock(const BCCodec::BC6HBlock *block, float *out);

                            /// Decode one signed BC6H block to 16 RGBA32F texels. Alpha is set to 1.0.
    static void             DecodeBC6HSBlock(const BCCodec::BC6HBlock *block, float *out);
};

BE_INLINE void BCDecoder::DecompressImageBC6HU(const byte *blocks, const int width, const int height, const int depth, float *out) {
    DecompressImageBC6H(blocks, width, height, depth, false, out);
}

BE_INLINE void BCDecoder::DecompressImageBC6HS(const byte *blocks, const int width, const int height, const int depth, float *out) {
    DecompressImageBC6H(blocks, width, height, depth, true, out);
}

BE_NAMESPACE_END
