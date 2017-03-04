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

#pragma once

#include "DxtCodec.h"

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------
//
// DXT Decoder
//
//--------------------------------------------------------------------------------

class BE_API DXTDecoder : public DXTCodec {
public:
    static void             DecompressImageDXT1(const DXTBlock *dxtBlock, const int width, const int height, const int depth, byte *out);
    static void             DecompressImageDXT3(const DXTBlock *dxtBlock, const int width, const int height, const int depth, byte *out);
    static void             DecompressImageDXT5(const DXTBlock *dxtBlock, const int width, const int height, const int depth, byte *out);
    static void             DecompressImageDXN2(const DXTBlock *dxtBlock, const int width, const int height, const int depth, byte *out);

private:
                            /// Decode 64 bits color block to RGBA8888
    static void	            DecodeColorBlock(const DXTBlock::ColorBlock *block, byte *out, bool writeAlpha);
                            /// Decode 64 bits alpha block to 8-bit alpha
    static void	            DecodeAlphaBlock(const DXTBlock::AlphaBlock *block, byte *out);
                            /// Decode 64 bits alpha block (4 bits per pixel) to 8-bit alpha
    static void	            DecodeAlphaExplicitBlock(const DXTBlock::AlphaExplicitBlock *block, byte *out);
};

BE_NAMESPACE_END