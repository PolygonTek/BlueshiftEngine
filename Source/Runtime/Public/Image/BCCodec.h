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

BE_NAMESPACE_BEGIN

// BC1: Three-component color and alpha     - Color(5:6:5), Alpha(1) or no alpha
// BC2: Three-component color and alpha     - Color(5:6:5), Alpha(4)
// BC3: Three-component color and alpha     - Color(5:6:5), Alpha(8)
// BC4: One-component color                 - One component(8)
// BC5: Two-component color                 - Two components(8:8)
// BC6H: Three-component HDR color          - Three half float components, unsigned or signed
// BC7: Three-component color and alpha     - Variable precision color and alpha

class BE_API BCCodec {
public:
    struct ColorBlock565 {
        uint16_t            color0;         ///< RGB565 color key 0
        uint16_t            color1;         ///< RGB565 color key 1
        uint32_t            indexes;        ///< 2 bit index per color pixel
    };
    
    struct AlphaBlock3Bit {
        byte                alpha0;         ///< alpha key 0
        byte                alpha1;         ///< alpha key 1
        byte                indexes[6];     ///< 3 bit index per alpha pixel
    };
        
    struct AlphaBlock4Bit {
        uint16_t            row[4];         ///< 4 bit explicit alpha value per alpha pixel
    };

    struct BC6HBlock {
        byte                data[16];       ///< Mode-dependent 128-bit BC6H payload
    };

    struct BC7Block {
        byte                data[16];       ///< Mode-dependent 128-bit BC7 payload
    };

                            /// Convert BGR565 to RGB888
    static void             RGB888From565(const uint16_t c565, byte *rgb);
                            /// Convert RGB888 to BGR565
    static const uint16_t   RGB888To565(const byte *rgb);
                            /// Convert Vec3 to RGB888
    static void             RGB888FromVec3(const Vec3 &v, byte *rgb);
                            /// Convert RGB888 to Vec3
    static const Vec3       RGB888ToVec3(const byte *rgb);

                            /// Build the four-entry RGB palette. Alpha components are left unchanged.
                            /// BC1 selects four-color mode when color0 > color1; BC2/BC3 always use it.
    static void             BuildColorPalette565(const uint16_t color0, const uint16_t color1, const bool fourColorMode, byte colors[4][4]);
                            /// Build the eight-entry alpha palette used by BC3, BC4, and BC5.
    static void             BuildAlphaPalette(const byte alpha0, const byte alpha1, byte alphas[8]);
};

static_assert(sizeof(BCCodec::ColorBlock565) == 8, "BCCodec::ColorBlock565 must be 8 bytes");
static_assert(sizeof(BCCodec::AlphaBlock3Bit) == 8, "BCCodec::AlphaBlock3Bit must be 8 bytes");
static_assert(sizeof(BCCodec::AlphaBlock4Bit) == 8, "BCCodec::AlphaBlock4Bit must be 8 bytes");
static_assert(sizeof(BCCodec::BC6HBlock) == 16, "BCCodec::BC6HBlock must be 16 bytes");
static_assert(sizeof(BCCodec::BC7Block) == 16, "BCCodec::BC7Block must be 16 bytes");

BE_INLINE void BCCodec::RGB888From565(const uint16_t c565, byte *rgb) {
    // Shift and then or with the most significant bits
    rgb[0] = byte(((c565 >> 8) & (((1 << (8 - 3)) - 1) << 3)) | ((c565 >> 13) & ((1 << 3) - 1)));   // r
    rgb[1] = byte(((c565 >> 3) & (((1 << (8 - 2)) - 1) << 2)) | ((c565 >> 9) & ((1 << 2) - 1)));    // g
    rgb[2] = byte(((c565 << 3) & (((1 << (8 - 3)) - 1) << 3)) | ((c565 >> 2) & ((1 << 3) - 1)));    // b
}

BE_INLINE const uint16_t BCCodec::RGB888To565(const byte *rgb) {
    return ((rgb[0] >> 3) << 11) | ((rgb[1] >> 2) << 5) | (rgb[2] >> 3);
}

BE_INLINE void BCCodec::RGB888FromVec3(const Vec3 &v, byte *rgb) {
    rgb[0] = (byte)(v.x == 1.0 ? 255 : v.x * 256.0f);
    rgb[1] = (byte)(v.y == 1.0 ? 255 : v.y * 256.0f);
    rgb[2] = (byte)(v.z == 1.0 ? 255 : v.z * 256.0f);
}

BE_INLINE const Vec3 BCCodec::RGB888ToVec3(const byte *rgb) {
    return Vec3(rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f);
}

BE_INLINE void BCCodec::BuildColorPalette565(const uint16_t color0, const uint16_t color1, const bool fourColorMode, byte colors[4][4]) {
    RGB888From565(color0, colors[0]);
    RGB888From565(color1, colors[1]);

    if (fourColorMode) {
        // Four-color block: derive the other two colors
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
        colors[2][0] = (2 * colors[0][0] + 1 * colors[1][0]) / 3;
        colors[2][1] = (2 * colors[0][1] + 1 * colors[1][1]) / 3;
        colors[2][2] = (2 * colors[0][2] + 1 * colors[1][2]) / 3;

        colors[3][0] = (1 * colors[0][0] + 2 * colors[1][0]) / 3;
        colors[3][1] = (1 * colors[0][1] + 2 * colors[1][1]) / 3;
        colors[3][2] = (1 * colors[0][2] + 2 * colors[1][2]) / 3;
    } else {
        // Three-color block: derive the other colors
        // 00 = color_0, 01 = color_1, 10 = color_2, 11 = transparent
        colors[2][0] = (colors[0][0] + colors[1][0]) / 2;
        colors[2][1] = (colors[0][1] + colors[1][1]) / 2;
        colors[2][2] = (colors[0][2] + colors[1][2]) / 2;

        // Black RGB for the transparent entry; alpha is set by the caller.
        colors[3][0] = 0x00;
        colors[3][1] = 0x00;
        colors[3][2] = 0x00;
    }
}

BE_INLINE void BCCodec::BuildAlphaPalette(const byte alpha0, const byte alpha1, byte alphas[8]) {
    alphas[0] = alpha0;
    alphas[1] = alpha1;

    // 8-alpha or 6-alpha block?
    if (alpha0 > alpha1) {
        // 8-alpha block: derive the other 6 alphas.
        // 000 = alpha_0, 001 = alpha_1, others are interpolated
        alphas[2] = (6 * alpha0 + 1 * alpha1) / 7;  // 010
        alphas[3] = (5 * alpha0 + 2 * alpha1) / 7;  // 011
        alphas[4] = (4 * alpha0 + 3 * alpha1) / 7;  // 100
        alphas[5] = (3 * alpha0 + 4 * alpha1) / 7;  // 101
        alphas[6] = (2 * alpha0 + 5 * alpha1) / 7;  // 110
        alphas[7] = (1 * alpha0 + 6 * alpha1) / 7;  // 111
    } else {
        // 6-alpha block: derive the other alphas.
        // 000 = alpha_0, 001 = alpha_1, others are interpolated
        alphas[2] = (4 * alpha0 + 1 * alpha1) / 5;  // 010
        alphas[3] = (3 * alpha0 + 2 * alpha1) / 5;  // 011
        alphas[4] = (2 * alpha0 + 3 * alpha1) / 5;  // 100
        alphas[5] = (1 * alpha0 + 4 * alpha1) / 5;  // 101
        alphas[6] = 0;                              // 110
        alphas[7] = 255;                            // 111
    }
}

BE_NAMESPACE_END
