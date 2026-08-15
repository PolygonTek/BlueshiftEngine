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

#include "Precompiled.h"
#include "Image/Image.h"
#include "ImageInternal.h"

#include "ImageFormatUnpackRGBA8888.inl"
#include "ImageFormatPackRGBA8888.inl"
#include "ImageFormatUnpackRGBA32F.inl"
#include "ImageFormatPackRGBA32F.inl"

BE_NAMESPACE_BEGIN

static const ImageFormatInfo imageFormatInfo[] = {
    // +------------------------------------------------------------------------------------------------------------ name
    // |                        +----------------------------------------------------------------------------------- size (bytes per pixel or bytes per block)
    // |                        |   +------------------------------------------------------------------------------- number of components
    // |                        |   |   +--------------------------------------------------------------------------- r bits
    // |                        |   |   |   +----------------------------------------------------------------------- g bits
    // |                        |   |   |   |   +------------------------------------------------------------------- b bits
    // |                        |   |   |   |   |   +--------------------------------------------------------------- a bits
    // |                        |   |   |   |   |   |   +----------------------------------------------------------- type
    // |                        |   |   |   |   |   |   |                        +---------------------------------- unpackRGBA8888
    // |                        |   |   |   |   |   |   |                        |        +------------------------- packRGBA8888
    // |                        |   |   |   |   |   |   |                        |        |        +---------------- unpackRGBA32F
    // |                        |   |   |   |   |   |   |                        |        |        |        +------- packRGBA32F
    // |                        |   |   |   |   |   |   |                        |        |        |        |
    { "Unknown",                0,  0,  0,  0,  0,  0,  Image::FormatType::None, nullptr, nullptr, nullptr, nullptr },
    { "R32_FLOAT",              4,  1,  32, 0,  0,  0,  Image::FormatType::Float, nullptr, nullptr, R32FToRGBA32F, RGBA32FToR32F },
    { "R32_SINT",               4,  1,  32, 0,  0,  0,  Image::FormatType::SInt, nullptr, nullptr, R32SIntToRGBA32F, RGBA32FToR32SInt },
    { "R32_UINT",               4,  1,  32, 0,  0,  0,  Image::FormatType::UInt, nullptr, nullptr, R32UIntToRGBA32F, RGBA32FToR32UInt },
    { "R32G32_FLOAT",           8,  2,  32, 32, 0,  0,  Image::FormatType::Float, nullptr, nullptr, RG32FToRGBA32F, RGBA32FToRG32F },
    { "R32G32_SINT",            8,  2,  32, 32, 0,  0,  Image::FormatType::SInt, nullptr, nullptr, RG32SIntToRGBA32F, RGBA32FToRG32SInt },
    { "R32G32_UINT",            8,  2,  32, 32, 0,  0,  Image::FormatType::UInt, nullptr, nullptr, RG32UIntToRGBA32F, RGBA32FToRG32UInt },
    { "R32G32B32_FLOAT",        12, 3,  32, 32, 32, 0,  Image::FormatType::Float, nullptr, nullptr, RGB32FToRGBA32F, RGBA32FToRGB32F },
    { "R32G32B32_SINT",         12, 3,  32, 32, 32, 0,  Image::FormatType::SInt, nullptr, nullptr, RGB32SIntToRGBA32F, RGBA32FToRGB32SInt },
    { "R32G32B32_UINT",         12, 3,  32, 32, 32, 0,  Image::FormatType::UInt, nullptr, nullptr, RGB32UIntToRGBA32F, RGBA32FToRGB32UInt },
    { "R32G32B32A32_FLOAT",     16, 4,  32, 32, 32, 32, Image::FormatType::Float, nullptr, nullptr, RGBA32FToRGBA32F, RGBA32FToRGBA32F },
    { "R32G32B32A32_SINT",      16, 4,  32, 32, 32, 32, Image::FormatType::SInt, nullptr, nullptr, RGBA32SIntToRGBA32F, RGBA32FToRGBA32SInt },
    { "R32G32B32A32_UINT",      16, 4,  32, 32, 32, 32, Image::FormatType::UInt, nullptr, nullptr, RGBA32UIntToRGBA32F, RGBA32FToRGBA32UInt },
    { "R16_FLOAT",              2,  1,  16, 0,  0,  0,  Image::FormatType::Float, nullptr, nullptr, R16FToRGBA32F, RGBA32FToR16F },
    { "R16",                    2,  1,  16, 0,  0,  0,  Image::FormatType::UNorm, nullptr, nullptr, R16ToRGBA32F, RGBA32FToR16 },
    { "R16_SNORM",              2,  1,  16, 0,  0,  0,  Image::FormatType::SNorm, nullptr, nullptr, R16SNormToRGBA32F, RGBA32FToR16SNorm },
    { "R16_SINT",               2,  1,  16, 0,  0,  0,  Image::FormatType::SInt, nullptr, nullptr, R16SIntToRGBA32F, RGBA32FToR16SInt },
    { "R16_UINT",               2,  1,  16, 0,  0,  0,  Image::FormatType::UInt, nullptr, nullptr, R16UIntToRGBA32F, RGBA32FToR16UInt },
    { "R16G16_FLOAT",           4,  2,  16, 16, 0,  0,  Image::FormatType::Float, nullptr, nullptr, RG16FToRGBA32F, RGBA32FToRG16F },
    { "R16G16",                 4,  2,  16, 16, 0,  0,  Image::FormatType::UNorm, nullptr, nullptr, RG16ToRGBA32F, RGBA32FToRG16 },
    { "R16G16_SNORM",           4,  2,  16, 16, 0,  0,  Image::FormatType::SNorm, nullptr, nullptr, RG16SNormToRGBA32F, RGBA32FToRG16SNorm },
    { "R16G16_SINT",            4,  2,  16, 16, 0,  0,  Image::FormatType::SInt, nullptr, nullptr, RG16SIntToRGBA32F, RGBA32FToRG16SInt },
    { "R16G16_UINT",            4,  2,  16, 16, 0,  0,  Image::FormatType::UInt, nullptr, nullptr, RG16UIntToRGBA32F, RGBA32FToRG16UInt },
    { "R16G16B16_FLOAT",        6,  3,  16, 16, 16, 0,  Image::FormatType::Float, nullptr, nullptr, RGB16FToRGBA32F, RGBA32FToRGB16F },
    { "R16G16B16",              6,  3,  16, 16, 16, 0,  Image::FormatType::UNorm, nullptr, nullptr, RGB16ToRGBA32F, RGBA32FToRGB16 },
    { "R16G16B16_SNORM",        6,  3,  16, 16, 16, 0,  Image::FormatType::SNorm, nullptr, nullptr, RGB16SNormToRGBA32F, RGBA32FToRGB16SNorm },
    { "R16G16B16_SINT",         6,  3,  16, 16, 16, 0,  Image::FormatType::SInt, nullptr, nullptr, RGB16SIntToRGBA32F, RGBA32FToRGB16SInt },
    { "R16G16B16_UINT",         6,  3,  16, 16, 16, 0,  Image::FormatType::UInt, nullptr, nullptr, RGB16UIntToRGBA32F, RGBA32FToRGB16UInt },
    { "R16G16B16A16_FLOAT",     8,  4,  16, 16, 16, 16, Image::FormatType::Float, nullptr, nullptr, RGBA16FToRGBA32F, RGBA32FToRGBA16F },
    { "R16G16B16A16",           8,  4,  16, 16, 16, 16, Image::FormatType::UNorm, nullptr, nullptr, RGBA16ToRGBA32F, RGBA32FToRGBA16 },
    { "R16G16B16A16_SNORM",     8,  4,  16, 16, 16, 16, Image::FormatType::SNorm, nullptr, nullptr, RGBA16SNormToRGBA32F, RGBA32FToRGBA16SNorm },
    { "R16G16B16A16_SINT",      8,  4,  16, 16, 16, 16, Image::FormatType::SInt, nullptr, nullptr, RGBA16SIntToRGBA32F, RGBA32FToRGBA16SInt },
    { "R16G16B16A16_UINT",      8,  4,  16, 16, 16, 16, Image::FormatType::UInt, nullptr, nullptr, RGBA16UIntToRGBA32F, RGBA32FToRGBA16UInt },
    { "R9G9B9E5_FLOAT",         4,  3,  9,  9,  9,  0,  Image::FormatType::Float | Image::FormatType::Packed, nullptr, nullptr, RGBE9995ToRGBA32F, RGBA32FToRGBE9995 },
    { "R11G11B10_FLOAT",        4,  3,  11, 11, 10, 0,  Image::FormatType::Float | Image::FormatType::Packed, nullptr, nullptr, RGB11F11F10FToRGBA32F, RGBA32FToRGB11F11F10F },
    { "R8",                     1,  1,  8,  0,  0,  0,  Image::FormatType::UNorm, R8ToRGBA8888, RGBA8888ToR8, R8ToRGBA32F, RGBA32FToR8 },
    { "R8_SNORM",               1,  1,  8,  0,  0,  0,  Image::FormatType::SNorm, nullptr, nullptr, R8SNormToRGBA32F, RGBA32FToR8SNorm },
    { "R8_SINT",                1,  1,  8,  0,  0,  0,  Image::FormatType::SInt, nullptr, nullptr, R8SIntToRGBA32F, RGBA32FToR8SInt },
    { "R8_UINT",                1,  1,  8,  0,  0,  0,  Image::FormatType::UInt, nullptr, nullptr, R8UIntToRGBA32F, RGBA32FToR8UInt },
    { "R8G8",                   2,  2,  8,  8,  0,  0,  Image::FormatType::UNorm, RG88ToRGBA8888, RGBA8888ToRG88, RG88ToRGBA32F, RGBA32FToRG88 },
    { "R8G8_SNORM",             2,  2,  8,  8,  0,  0,  Image::FormatType::SNorm, nullptr, nullptr, RG88SNormToRGBA32F, RGBA32FToRG88SNorm },
    { "R8G8_SINT",              2,  2,  8,  8,  0,  0,  Image::FormatType::SInt, nullptr, nullptr, RG88SIntToRGBA32F, RGBA32FToRG88SInt },
    { "R8G8_UINT",              2,  2,  8,  8,  0,  0,  Image::FormatType::UInt, nullptr, nullptr, RG88UIntToRGBA32F, RGBA32FToRG88UInt },
    { "R8G8B8"   ,              3,  3,  8,  8,  8,  0,  Image::FormatType::UNorm, RGB888ToRGBA8888, RGBA8888ToRGB888, RGB888ToRGBA32F, RGBA32FToRGB888 },
    { "R8G8B8_SNORM",           3,  3,  8,  8,  8,  0,  Image::FormatType::SNorm, nullptr, nullptr, RGB888SNormToRGBA32F, RGBA32FToRGB888SNorm },
    { "R8G8B8_SINT",            3,  3,  8,  8,  8,  0,  Image::FormatType::SInt, nullptr, nullptr, RGB888SIntToRGBA32F, RGBA32FToRGB888SInt },
    { "R8G8B8_UINT",            3,  3,  8,  8,  8,  0,  Image::FormatType::UInt, nullptr, nullptr, RGB888UIntToRGBA32F, RGBA32FToRGB888UInt },
    { "B8G8R8",                 3,  3,  8,  8,  8,  0,  Image::FormatType::UNorm, BGR888ToRGBA8888, RGBA8888ToBGR888, BGR888ToRGBA32F, RGBA32FToBGR888 },
    { "R8G8B8A8",               4,  4,  8,  8,  8,  8,  Image::FormatType::UNorm, RGBA8888ToRGBA8888, RGBA8888ToRGBA8888, RGBA8888ToRGBA32F, RGBA32FToRGBA8888 },
    { "R8G8B8A8_SNORM",         4,  4,  8,  8,  8,  8,  Image::FormatType::SNorm, nullptr, nullptr, RGBA8888SNormToRGBA32F, RGBA32FToRGBA8888SNorm },
    { "R8G8B8A8_SINT",          4,  4,  8,  8,  8,  8,  Image::FormatType::SInt, nullptr, nullptr, RGBA8888SIntToRGBA32F, RGBA32FToRGBA8888SInt },
    { "R8G8B8A8_UINT",          4,  4,  8,  8,  8,  8,  Image::FormatType::UInt, nullptr, nullptr, RGBA8888UIntToRGBA32F, RGBA32FToRGBA8888UInt },
    { "B8G8R8A8",               4,  4,  8,  8,  8,  8,  Image::FormatType::UNorm, BGRA8888ToRGBA8888, RGBA8888ToBGRA8888, BGRA8888ToRGBA32F, RGBA32FToBGRA8888 },
    { "A8B8G8R8",               4,  4,  8,  8,  8,  8,  Image::FormatType::UNorm, ABGR8888ToRGBA8888, RGBA8888ToABGR8888, ABGR8888ToRGBA32F, RGBA32FToABGR8888 },
    { "A8R8G8B8",               4,  4,  8,  8,  8,  8,  Image::FormatType::UNorm, ARGB8888ToRGBA8888, RGBA8888ToARGB8888, ARGB8888ToRGBA32F, RGBA32FToARGB8888 },
    { "R8G8B8X8",               4,  4,  8,  8,  8,  0,  Image::FormatType::UNorm, RGBX8888ToRGBA8888, RGBA8888ToRGBX8888, RGBX8888ToRGBA32F, RGBA32FToRGBX8888 },
    { "B8G8R8X8",               4,  4,  8,  8,  8,  0,  Image::FormatType::UNorm, BGRX8888ToRGBA8888, RGBA8888ToBGRX8888, BGRX8888ToRGBA32F, RGBA32FToBGRX8888 },

    { "R10G10B10A2",            4,  4, 10, 10, 10,  2,  Image::FormatType::UNorm | Image::FormatType::Packed, nullptr, nullptr, RGBA1010102ToRGBA32F, RGBA32FToRGBA1010102 },
    { "R10G10B10A2_UINT",       4,  4, 10, 10, 10,  2,  Image::FormatType::UInt | Image::FormatType::Packed, nullptr, nullptr, RGBA1010102UIntToRGBA32F, RGBA32FToRGBA1010102UInt },
    { "R4G4B4X4",               2,  4,  4,  4,  4,  0,  Image::FormatType::UNorm | Image::FormatType::Packed, RGBX4444ToRGBA8888, RGBA8888ToRGBX4444, RGBX4444ToRGBA32F, RGBA32FToRGBX4444 },
    { "B4G4R4X4",               2,  4,  4,  4,  4,  0,  Image::FormatType::UNorm | Image::FormatType::Packed, BGRX4444ToRGBA8888, RGBA8888ToBGRX4444, BGRX4444ToRGBA32F, RGBA32FToBGRX4444 },
    { "R4G4B4A4",               2,  4,  4,  4,  4,  4,  Image::FormatType::UNorm | Image::FormatType::Packed, RGBA4444ToRGBA8888, RGBA8888ToRGBA4444, RGBA4444ToRGBA32F, RGBA32FToRGBA4444 },
    { "B4G4R4A4",               2,  4,  4,  4,  4,  4,  Image::FormatType::UNorm | Image::FormatType::Packed, BGRA4444ToRGBA8888, RGBA8888ToBGRA4444, BGRA4444ToRGBA32F, RGBA32FToBGRA4444 },
    { "A4B4G4R4",               2,  4,  4,  4,  4,  4,  Image::FormatType::UNorm | Image::FormatType::Packed, ABGR4444ToRGBA8888, RGBA8888ToABGR4444, ABGR4444ToRGBA32F, RGBA32FToABGR4444 },
    { "A4R4G4B4",               2,  4,  4,  4,  4,  4,  Image::FormatType::UNorm | Image::FormatType::Packed, ARGB4444ToRGBA8888, RGBA8888ToARGB4444, ARGB4444ToRGBA32F, RGBA32FToARGB4444 },
    { "R5G5B5X1",               2,  4,  5,  5,  5,  0,  Image::FormatType::UNorm | Image::FormatType::Packed, RGBX5551ToRGBA8888, RGBA8888ToRGBX5551, RGBX5551ToRGBA32F, RGBA32FToRGBX5551 },
    { "B5G5R5X1",               2,  4,  5,  5,  5,  0,  Image::FormatType::UNorm | Image::FormatType::Packed, BGRX5551ToRGBA8888, RGBA8888ToBGRX5551, BGRX5551ToRGBA32F, RGBA32FToBGRX5551 },
    { "R5G5B5A1",               2,  4,  5,  5,  5,  1,  Image::FormatType::UNorm | Image::FormatType::Packed, RGBA5551ToRGBA8888, RGBA8888ToRGBA5551, RGBA5551ToRGBA32F, RGBA32FToRGBA5551 },
    { "B5G5R5A1",               2,  4,  5,  5,  5,  1,  Image::FormatType::UNorm | Image::FormatType::Packed, BGRA5551ToRGBA8888, RGBA8888ToBGRA5551, BGRA5551ToRGBA32F, RGBA32FToBGRA5551 },
    { "A1B5G5R5",               2,  4,  5,  5,  5,  1,  Image::FormatType::UNorm | Image::FormatType::Packed, ABGR1555ToRGBA8888, RGBA8888ToABGR1555, ABGR1555ToRGBA32F, RGBA32FToABGR1555 },
    { "A1R5G5B5",               2,  4,  5,  5,  5,  1,  Image::FormatType::UNorm | Image::FormatType::Packed, ARGB1555ToRGBA8888, RGBA8888ToARGB1555, ARGB1555ToRGBA32F, RGBA32FToARGB1555 },
    { "R5G6B5",                 2,  3,  5,  6,  5,  0,  Image::FormatType::UNorm | Image::FormatType::Packed, RGB565ToRGBA8888, RGBA8888ToRGB565, RGB565ToRGBA32F, RGBA32FToRGB565 },
    { "B5G6R5",                 2,  3,  5,  6,  5,  0,  Image::FormatType::UNorm | Image::FormatType::Packed, BGR565ToRGBA8888, RGBA8888ToBGR565, BGR565ToRGBA32F, RGBA32FToBGR565 },

    // depth/stencil ------------------------------------------------------------------------------
    { "D16",                    2,  1,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Depth, nullptr, nullptr, L16ToRGBA32F, RGBA32FToL16 },
    { "D24X8",                  4,  1,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Depth, nullptr, nullptr, L24X8ToRGBA32F, RGBA32FToL24X8 },
    { "D24S8",                  4,  2,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::DepthStencil, nullptr, nullptr, L24X8ToRGBA32F, RGBA32FToL24X8 },
    { "D32_FLOAT",              4,  1,  0,  0,  0,  0,  Image::FormatType::Float | Image::FormatType::Depth, nullptr, nullptr, L32FToRGBA32F, RGBA32FToL32F },
    { "D32_FLOAT_S8X24",        8,  3,  0,  0,  0,  0,  Image::FormatType::Float | Image::FormatType::DepthStencil, nullptr, nullptr, L32FX32ToRGBA32F, RGBA32FToL32FX32 },

    // Luminance/Alpha
    { "L32_FLOAT",              4,  1,  32, 0,  0,  0,  Image::FormatType::Float, nullptr, nullptr, L32FToRGBA32F, RGBA32FToL32F },
    { "A32_FLOAT",              4,  1,  0,  0,  0,  32, Image::FormatType::Float, nullptr, nullptr, A32FToRGBA32F, RGBA32FToA32F },
    { "L32A32_FLOAT",           8,  2,  32, 0,  0,  32, Image::FormatType::Float, nullptr, nullptr, LA32FToRGBA32F, RGBA32FToLA32F },
    { "L16_FLOAT",              2,  1,  16, 0,  0,  0,  Image::FormatType::Float, nullptr, nullptr, L16FToRGBA32F, RGBA32FToL16F },
    { "A16_FLOAT",              2,  1,  0,  0,  0,  16, Image::FormatType::Float, nullptr, nullptr, A16FToRGBA32F, RGBA32FToA16F },
    { "L16A16_FLOAT",           4,  2,  16, 0,  0,  16, Image::FormatType::Float, nullptr, nullptr, LA16FToRGBA32F, RGBA32FToLA16F },
    { "L8",                     1,  1,  8,  0,  0,  0,  Image::FormatType::UNorm, L8ToRGBA8888, RGBA8888ToL8, L8ToRGBA32F, RGBA32FToL8 },
    { "A8" ,                    1,  1,  0,  0,  0,  8,  Image::FormatType::UNorm, A8ToRGBA8888, RGBA8888ToA8, A8ToRGBA32F, RGBA32FToA8 },
    { "L8A8"  ,                 2,  2,  8,  0,  0,  8,  Image::FormatType::UNorm, LA88ToRGBA8888, RGBA8888ToLA88, LA88ToRGBA32F, RGBA32FToLA88 },

    // BC -----------------------------------------------------------------------------------------
    { "BC1",                    8,  4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC2",                    16, 4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC3",                    16, 4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC3_XGBR",               16, 4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC4",                    8,  1,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC5",                    16, 2,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC6H_UF16",              16, 3,  0,  0,  0,  0,  Image::FormatType::Float | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC6H_SF16",              16, 3,  0,  0,  0,  0,  Image::FormatType::Float | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "BC7",                    16, 4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },

    // PVRTC --------------------------------------------------------------------------------------
    { "PVRTC12",                8,  3,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "PVRTC14",                8,  3,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "PVRTC12A",               8,  4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "PVRTC14A",               8,  4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "PVRTC22A",               8,  4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "PVRTC24A",               8,  4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },

    // ETC ----------------------------------------------------------------------------------------
    { "ETC1",                   8,  3,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "ETC2",                   8,  3,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "ETC2A1",                 8,  4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "ETC2A",                  16, 4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "EACR11",                 8,  1,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "EACRG11",                16, 2,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "EACR11_SNORM",           8,  1,  0,  0,  0,  0,  Image::FormatType::SNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "EACRG11_SNORM",          16, 2,  0,  0,  0,  0,  Image::FormatType::SNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },

    // ATC ----------------------------------------------------------------------------------------
    { "ATC",                    8,  3,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "ATCE",                   16, 4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
    { "ATCI",                   16, 4,  0,  0,  0,  0,  Image::FormatType::UNorm | Image::FormatType::Compressed, nullptr, nullptr, nullptr, nullptr },
};

static_assert(sizeof(imageFormatInfo) / sizeof(imageFormatInfo[0]) == to_int(Image::Format::Count), "Unexpected image format table size");

const ImageFormatInfo *GetImageFormatInfo(Image::Format imageFormat) {
    assert(imageFormat < Image::Format::Count);
    return &imageFormatInfo[to_int(imageFormat)];
}

bool CompressedFormatMinDimensions(Image::Format imageFormat, int &minWidth, int &minHeight) {
    switch (imageFormat) {
    case Image::Format::BC1:
    case Image::Format::BC2:
    case Image::Format::BC3:
    case Image::Format::BC3XGBR:
    case Image::Format::BC4:
    case Image::Format::BC5:
    case Image::Format::BC6H_UF16:
    case Image::Format::BC6H_SF16:
    case Image::Format::BC7:
        minWidth = 4;
        minHeight = 4;
        return true;
    case Image::Format::PVRTC12:
    case Image::Format::PVRTC12A:
        minWidth = 16;
        minHeight = 8;
        return true;
    case Image::Format::PVRTC14:
    case Image::Format::PVRTC14A:
        minWidth = 8;
        minHeight = 8;
        return true;
    case Image::Format::PVRTC22A:
        minWidth = 8;
        minHeight = 4;
        return true;
    case Image::Format::PVRTC24A:
        minWidth = 4;
        minHeight = 4;
        return true;
    case Image::Format::ETC1:
    case Image::Format::ETC2:
    case Image::Format::ETC2A1:
    case Image::Format::ETC2A:
    case Image::Format::EACR11:
    case Image::Format::EACR11_SNORM:
    case Image::Format::EACRG11:
    case Image::Format::EACRG11_SNORM:
        minWidth = 4;
        minHeight = 4;
        return true;
    case Image::Format::ATC:
    case Image::Format::ATCE: // Explicit alpha
    case Image::Format::ATCI: // Interpolated alpha
        minWidth = 4;
        minHeight = 4;
        return true;
    default:
        return false;
    }
    
    return false;
}

bool CompressedFormatBlockDimensions(Image::Format imageFormat, int &blockWidth, int &blockHeight) {
    switch (imageFormat) {
    case Image::Format::BC1:
    case Image::Format::BC2:
    case Image::Format::BC3:
    case Image::Format::BC3XGBR:
    case Image::Format::BC4:
    case Image::Format::BC5:
    case Image::Format::BC6H_UF16:
    case Image::Format::BC6H_SF16:
    case Image::Format::BC7:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::Format::PVRTC12:
    case Image::Format::PVRTC12A:
        blockWidth = 8;
        blockHeight = 4;
        return true;
    case Image::Format::PVRTC14:
    case Image::Format::PVRTC14A:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::Format::PVRTC22A:
        blockWidth = 8;
        blockHeight = 4;
        return true;
    case Image::Format::PVRTC24A:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::Format::ETC1:
    case Image::Format::ETC2:
    case Image::Format::ETC2A1:
    case Image::Format::ETC2A:
    case Image::Format::EACR11:
    case Image::Format::EACR11_SNORM:
    case Image::Format::EACRG11:
    case Image::Format::EACRG11_SNORM:
        blockWidth = 4;
        blockHeight = 4;
        return true;
    case Image::Format::ATC:
    case Image::Format::ATCE: // Explicit alpha
    case Image::Format::ATCI: // Interpolated alpha
        blockWidth = 4;
        blockHeight = 4;
        return true;
    default:
        return false;
    }
    
    return false;
}

BE_NAMESPACE_END
