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

BE_NAMESPACE_BEGIN

// BC1(DXT1): Three-component color and alpha   - Color(5:6:5), Alpha(1) or no alpha
// BC2(DXT3): Three-component color and alpha   - Color(5:6:5), Alpha(4)
// BC3(DXT5): Three-component color and alpha   - Color(5:6:5), Alpha(8)
// BC4(DXN1): One-component color               - One component(8)
// BC5(DXN2): Two-component color               - Two components(8:8)

union DXTBlock {
    struct ColorBlock {
        uint16_t            color0;         ///< RGB565 color key 0
        uint16_t            color1;         ///< RGB565 color key 1
        uint32_t            indexes;        ///< 2 bit index per color pixel
    };
    
    struct AlphaBlock {
        byte                alpha0;         ///< alpha key 0
        byte                alpha1;         ///< alpha key 1
        byte                indexes[6];     ///< 3 bit index per alpha pixel
    };
        
    struct AlphaExplicitBlock {
        uint16_t            row[4];         ///< 4 bit explicit alpha value per alpha pixel
    };
    
    ColorBlock              colorBlock;
    AlphaBlock              alphaBlock;
    AlphaExplicitBlock      alphaExplicitBlock;
};

class BE_API DXTCodec {
public:
                            /// Convert BGR565 to RGB888
    static void             RGB888From565(const uint16_t c565, byte *rgb);
                            /// Convert RGB888 to BGR565
    static const uint16_t   RGB888To565(const byte *rgb);
                            /// Convert Vec3 to RGB888
    static void             RGB888FromVec3(const Vec3 &v, byte *rgb);
                            /// Convert RGB888 to Vec3
    static const Vec3       RGB888ToVec3(const byte *rgb);
};

BE_INLINE void DXTCodec::RGB888From565(const uint16_t c565, byte *rgb) {
    // Shift and then or with the most significant bits
    rgb[0] = byte(((c565 >> 8) & (((1 << (8 - 3)) - 1) << 3)) | ((c565 >> 13) & ((1 << 3) - 1)));   // r
    rgb[1] = byte(((c565 >> 3) & (((1 << (8 - 2)) - 1) << 2)) | ((c565 >> 9) & ((1 << 2) - 1)));    // g
    rgb[2] = byte(((c565 << 3) & (((1 << (8 - 3)) - 1) << 3)) | ((c565 >> 2) & ((1 << 3) - 1)));    // b
}

BE_INLINE const uint16_t DXTCodec::RGB888To565(const byte *rgb) {
    return ((rgb[0] >> 3) << 11) | ((rgb[1] >> 2) << 5) | (rgb[2] >> 3);
}

BE_INLINE void DXTCodec::RGB888FromVec3(const Vec3 &v, byte *rgb) {
    rgb[0] = (byte)(v.x == 1.0 ? 255 : v.x * 256.0f);
    rgb[1] = (byte)(v.y == 1.0 ? 255 : v.y * 256.0f);
    rgb[2] = (byte)(v.z == 1.0 ? 255 : v.z * 256.0f);
}

BE_INLINE const Vec3 DXTCodec::RGB888ToVec3(const byte *rgb) {
    return Vec3(rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f);
}

BE_NAMESPACE_END
