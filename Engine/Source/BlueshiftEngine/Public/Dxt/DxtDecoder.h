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