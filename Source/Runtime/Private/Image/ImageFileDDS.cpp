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
#include "Core/Heap.h"
#include "SIMD/SIMD.h"
#include "Math/Math.h"
#include "IO/FileSystem.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

#define DDS_MAGIC                   ((' '<<24) + ('S'<<16) + ('D'<<8) + ('D'))

// surface description flags
#define DDSD_CAPS                   0x1
#define DDSD_HEIGHT                 0x2
#define DDSD_WIDTH                  0x4
#define DDSD_PITCH                  0x8
#define DDSD_PIXELFORMAT            0x1000
#define DDSD_MIPMAPCOUNT            0x20000
#define DDSD_LINEARSIZE             0x80000
#define DDSD_DEPTH                  0x800000

// pixel format flags
#define DDSPF_ALPHAPIXELS           0x1
#define DDSPF_ALPHA                 0x2
#define DDSPF_FOURCC                0x4
#define DDSPF_RGB                   0x40
#define DDSPF_RGBA                  0x41
#define DDSPF_LUMINANCE             0x20000
#define DDSPF_BUMPDUDV              0x80000

// our extended flags
#define DDSF_ID_INDEXCOLOR          0x10000000
#define DDSF_ID_MONOCHROME          0x20000000

// caps1
#define DDSCAPS_COMPLEX             0x8
#define DDSCAPS_TEXTURE             0x1000
#define DDSCAPS_MIPMAP              0x400000

// caps2
#define DDSCAPS2_CUBEMAP            0x200
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x400
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x800
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x1000
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x2000
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x4000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x8000
#define DDSCAPS2_CUBEMAP_ALL_FACES  (DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX | DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY | DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ)
#define DDSCAPS2_VOLUME             0x200000

// DDS legacy formats
enum DDS_FORMAT {
    DDS_FORMAT_UNKNOWN = 0,

    DDS_FORMAT_R8G8B8 = 20,
    DDS_FORMAT_A8R8G8B8 = 21,
    DDS_FORMAT_X8R8G8B8 = 22,
    DDS_FORMAT_R5G6B5 = 23,
    DDS_FORMAT_X1R5G5B5 = 24,
    DDS_FORMAT_A1R5G5B5 = 25,
    DDS_FORMAT_A4R4G4B4 = 26,
    DDS_FORMAT_R3G3B2 = 27,
    DDS_FORMAT_A8 = 28,
    DDS_FORMAT_A8R3G3B2 = 29,
    DDS_FORMAT_X4R4G4B4 = 30,
    DDS_FORMAT_A2B10G10R10 = 31,
    DDS_FORMAT_A8B8G8R8 = 32,
    DDS_FORMAT_X8B8G8R8 = 33,
    DDS_FORMAT_G16R16 = 34,
    DDS_FORMAT_A2R10G10B10 = 35,
    DDS_FORMAT_A16B16G16R16 = 36,

    DDS_FORMAT_A8P8 = 40,
    DDS_FORMAT_P8 = 41,

    DDS_FORMAT_L8 = 50,
    DDS_FORMAT_A8L8 = 51,
    DDS_FORMAT_A4L4 = 52,

    DDS_FORMAT_V8U8 = 60,
    DDS_FORMAT_L6V5U5 = 61,
    DDS_FORMAT_X8L8V8U8 = 62,
    DDS_FORMAT_Q8W8V8U8 = 63,
    DDS_FORMAT_V16U16 = 64,
    DDS_FORMAT_A2W10V10U10 = 67,

    DDS_FORMAT_UYVY = MAKE_FOURCC('U', 'Y', 'V', 'Y'),
    DDS_FORMAT_R8G8_B8G8 = MAKE_FOURCC('R', 'G', 'B', 'G'),
    DDS_FORMAT_YUY2 = MAKE_FOURCC('Y', 'U', 'Y', '2'),
    DDS_FORMAT_G8R8_G8B8 = MAKE_FOURCC('G', 'R', 'G', 'B'),
    DDS_FORMAT_DXT1 = MAKE_FOURCC('D', 'X', 'T', '1'),
    DDS_FORMAT_DXT2 = MAKE_FOURCC('D', 'X', 'T', '2'),
    DDS_FORMAT_DXT3 = MAKE_FOURCC('D', 'X', 'T', '3'),
    DDS_FORMAT_DXT4 = MAKE_FOURCC('D', 'X', 'T', '4'),
    DDS_FORMAT_DXT5 = MAKE_FOURCC('D', 'X', 'T', '5'),

    DDS_FORMAT_D16_LOCKABLE = 70,
    DDS_FORMAT_D32 = 71,
    DDS_FORMAT_D15S1 = 73,
    DDS_FORMAT_D24S8 = 75,
    DDS_FORMAT_D24X8 = 77,
    DDS_FORMAT_D24X4S4 = 79,
    DDS_FORMAT_D16 = 80,

    DDS_FORMAT_D32F_LOCKABLE = 82,
    DDS_FORMAT_D24FS8 = 83,

#if !defined(D3D_DISABLE_9EX)
    DDS_FORMAT_D32_LOCKABLE = 84,
    DDS_FORMAT_S8_LOCKABLE = 85,
#endif // !D3D_DISABLE_9EX

    DDS_FORMAT_L16 = 81,

    DDS_FORMAT_VERTEXDATA = 100,
    DDS_FORMAT_INDEX16 = 101,
    DDS_FORMAT_INDEX32 = 102,

    DDS_FORMAT_Q16W16V16U16 = 110,

    DDS_FORMAT_MULTI2_ARGB8 = MAKE_FOURCC('M', 'E', 'T', '1'),

    DDS_FORMAT_R16F = 111,
    DDS_FORMAT_G16R16F = 112,
    DDS_FORMAT_A16B16G16R16F = 113,

    DDS_FORMAT_R32F = 114,
    DDS_FORMAT_G32R32F = 115,
    DDS_FORMAT_A32B32G32R32F = 116,

    DDS_FORMAT_CxV8U8 = 117,

#if !defined(D3D_DISABLE_9EX)
    DDS_FORMAT_A1 = 118,
    DDS_FORMAT_A2B10G10R10_XR_BIAS = 119,
    DDS_FORMAT_BINARYBUFFER = 199,
#endif // !D3D_DISABLE_9EX

    DDS_FORMAT_FORCE_DWORD = 0x7fffffff
};

// DX10
#define DX10_RESOURCE_DIMENSION_BUFFER      1
#define DX10_RESOURCE_DIMENSION_TEXTURE1D   2
#define DX10_RESOURCE_DIMENSION_TEXTURE2D   3
#define DX10_RESOURCE_DIMENSION_TEXTURE3D   4

#define DX10_RESOURCE_MISC_TEXTURECUBE      0x4

#define DX10_RESOURCE_MISC2_UNKNOWN         0
#define DX10_RESOURCE_MISC2_STRAIGHT        1
#define DX10_RESOURCE_MISC2_PREMULTIPLIED   2
#define DX10_RESOURCE_MISC2_OPAQUE          3
#define DX10_RESOURCE_MISC2_CUSTOM          4

// DX10 formats
enum DX10_FORMAT {
    DX10_FORMAT_UNKNOWN = 0,
    DX10_FORMAT_R32G32B32A32_TYPELESS = 1,
    DX10_FORMAT_R32G32B32A32_FLOAT = 2,
    DX10_FORMAT_R32G32B32A32_UINT = 3,
    DX10_FORMAT_R32G32B32A32_SINT = 4,
    DX10_FORMAT_R32G32B32_TYPELESS = 5,
    DX10_FORMAT_R32G32B32_FLOAT = 6,
    DX10_FORMAT_R32G32B32_UINT = 7,
    DX10_FORMAT_R32G32B32_SINT = 8,
    DX10_FORMAT_R16G16B16A16_TYPELESS = 9,
    DX10_FORMAT_R16G16B16A16_FLOAT = 10,
    DX10_FORMAT_R16G16B16A16_UNORM = 11,
    DX10_FORMAT_R16G16B16A16_UINT = 12,
    DX10_FORMAT_R16G16B16A16_SNORM = 13,
    DX10_FORMAT_R16G16B16A16_SINT = 14,
    DX10_FORMAT_R32G32_TYPELESS = 15,
    DX10_FORMAT_R32G32_FLOAT = 16,
    DX10_FORMAT_R32G32_UINT = 17,
    DX10_FORMAT_R32G32_SINT = 18,
    DX10_FORMAT_R32G8X24_TYPELESS = 19,
    DX10_FORMAT_D32_FLOAT_S8X24_UINT = 20,
    DX10_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
    DX10_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
    DX10_FORMAT_R10G10B10A2_TYPELESS = 23,
    DX10_FORMAT_R10G10B10A2_UNORM = 24,
    DX10_FORMAT_R10G10B10A2_UINT = 25,
    DX10_FORMAT_R11G11B10_FLOAT = 26,
    DX10_FORMAT_R8G8B8A8_TYPELESS = 27,
    DX10_FORMAT_R8G8B8A8_UNORM = 28,
    DX10_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    DX10_FORMAT_R8G8B8A8_UINT = 30,
    DX10_FORMAT_R8G8B8A8_SNORM = 31,
    DX10_FORMAT_R8G8B8A8_SINT = 32,
    DX10_FORMAT_R16G16_TYPELESS = 33,
    DX10_FORMAT_R16G16_FLOAT = 34,
    DX10_FORMAT_R16G16_UNORM = 35,
    DX10_FORMAT_R16G16_UINT = 36,
    DX10_FORMAT_R16G16_SNORM = 37,
    DX10_FORMAT_R16G16_SINT = 38,
    DX10_FORMAT_R32_TYPELESS = 39,
    DX10_FORMAT_D32_FLOAT = 40,
    DX10_FORMAT_R32_FLOAT = 41,
    DX10_FORMAT_R32_UINT = 42,
    DX10_FORMAT_R32_SINT = 43,
    DX10_FORMAT_R24G8_TYPELESS = 44,
    DX10_FORMAT_D24_UNORM_S8_UINT = 45,
    DX10_FORMAT_R24_UNORM_X8_TYPELESS = 46,
    DX10_FORMAT_X24_TYPELESS_G8_UINT = 47,
    DX10_FORMAT_R8G8_TYPELESS = 48,
    DX10_FORMAT_R8G8_UNORM = 49,
    DX10_FORMAT_R8G8_UINT = 50,
    DX10_FORMAT_R8G8_SNORM = 51,
    DX10_FORMAT_R8G8_SINT = 52,
    DX10_FORMAT_R16_TYPELESS = 53,
    DX10_FORMAT_R16_FLOAT = 54,
    DX10_FORMAT_D16_UNORM = 55,
    DX10_FORMAT_R16_UNORM = 56,
    DX10_FORMAT_R16_UINT = 57,
    DX10_FORMAT_R16_SNORM = 58,
    DX10_FORMAT_R16_SINT = 59,
    DX10_FORMAT_R8_TYPELESS = 60,
    DX10_FORMAT_R8_UNORM = 61,
    DX10_FORMAT_R8_UINT = 62,
    DX10_FORMAT_R8_SNORM = 63,
    DX10_FORMAT_R8_SINT = 64,
    DX10_FORMAT_A8_UNORM = 65,
    DX10_FORMAT_R1_UNORM = 66,
    DX10_FORMAT_R9G9B9E5_SHAREDEXP = 67,
    DX10_FORMAT_R8G8_B8G8_UNORM = 68,
    DX10_FORMAT_G8R8_G8B8_UNORM = 69,
    DX10_FORMAT_BC1_TYPELESS = 70,
    DX10_FORMAT_BC1_UNORM = 71,
    DX10_FORMAT_BC1_UNORM_SRGB = 72,
    DX10_FORMAT_BC2_TYPELESS = 73,
    DX10_FORMAT_BC2_UNORM = 74,
    DX10_FORMAT_BC2_UNORM_SRGB = 75,
    DX10_FORMAT_BC3_TYPELESS = 76,
    DX10_FORMAT_BC3_UNORM = 77,
    DX10_FORMAT_BC3_UNORM_SRGB = 78,
    DX10_FORMAT_BC4_TYPELESS = 79,
    DX10_FORMAT_BC4_UNORM = 80,
    DX10_FORMAT_BC4_SNORM = 81,
    DX10_FORMAT_BC5_TYPELESS = 82,
    DX10_FORMAT_BC5_UNORM = 83,
    DX10_FORMAT_BC5_SNORM = 84,
    DX10_FORMAT_B5G6R5_UNORM = 85,
    DX10_FORMAT_B5G5R5A1_UNORM = 86,
    DX10_FORMAT_B8G8R8A8_UNORM = 87,
    DX10_FORMAT_B8G8R8X8_UNORM = 88,
    DX10_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
    DX10_FORMAT_B8G8R8A8_TYPELESS = 90,
    DX10_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
    DX10_FORMAT_B8G8R8X8_TYPELESS = 92,
    DX10_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
    DX10_FORMAT_BC6H_TYPELESS = 94,
    DX10_FORMAT_BC6H_UF16 = 95,
    DX10_FORMAT_BC6H_SF16 = 96,
    DX10_FORMAT_BC7_TYPELESS = 97,
    DX10_FORMAT_BC7_UNORM = 98,
    DX10_FORMAT_BC7_UNORM_SRGB = 99,
    DX10_FORMAT_AYUV = 100,
    DX10_FORMAT_Y410 = 101,
    DX10_FORMAT_Y416 = 102,
    DX10_FORMAT_NV12 = 103,
    DX10_FORMAT_P010 = 104,
    DX10_FORMAT_P016 = 105,
    DX10_FORMAT_420_OPAQUE = 106,
    DX10_FORMAT_YUY2 = 107,
    DX10_FORMAT_Y210 = 108,
    DX10_FORMAT_Y216 = 109,
    DX10_FORMAT_NV11 = 110,
    DX10_FORMAT_AI44 = 111,
    DX10_FORMAT_IA44 = 112,
    DX10_FORMAT_P8 = 113,
    DX10_FORMAT_A8P8 = 114,
    DX10_FORMAT_B4G4R4A4_UNORM = 115
};

struct DdsFileHeader {
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipmapCount;
    uint32_t reserved1[11];
    struct {
        uint32_t size;
        uint32_t flags;
        uint32_t fourCC;
        uint32_t RGBBitCount;
        uint32_t RBitMask;
        uint32_t GBitMask;
        uint32_t BBitMask;
        uint32_t ABitMask;
    } ddsPixelFormat;
    struct {
        uint32_t caps1;
        uint32_t caps2;
        uint32_t caps3;
        uint32_t caps4;
    } ddsCaps;
    uint32_t reserved2;
};

struct DdsFileHeaderDX10 {
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlag2;
};

static bool ParseDDSFormat(const DdsFileHeader &ddsHeader, Image::Format &format) {
    switch (ddsHeader.ddsPixelFormat.fourCC) {
    case 0:
        switch (ddsHeader.ddsPixelFormat.RGBBitCount >> 3) {
        case 4:
            if (ddsHeader.ddsPixelFormat.ABitMask == 0x000000ff) {
                if (ddsHeader.ddsPixelFormat.RBitMask == 0xff000000) {
                    format = Image::Format::A8B8G8R8;
                } else {
                    format = Image::Format::A8R8G8B8;
                }
            } else if (ddsHeader.ddsPixelFormat.ABitMask == 0xff00000) {
                if (ddsHeader.ddsPixelFormat.RBitMask == 0x00ff0000) {
                    format = Image::Format::B8G8R8A8;
                } else {
                    format = Image::Format::R8G8B8A8;
                }
            } else {
                if (ddsHeader.ddsPixelFormat.RBitMask == 0x00ff0000) {
                    format = Image::Format::B8G8R8X8;
                } else {
                    format = Image::Format::R8G8B8X8;
                }
            }
            break;
        case 3:
            if (ddsHeader.ddsPixelFormat.RBitMask == 0x00ff0000) {
                format = Image::Format::B8G8R8;
            } else {
                format = Image::Format::R8G8B8;
            }
            break;
        case 2:
            if (ddsHeader.ddsPixelFormat.ABitMask == 0x0000000f) {
                if (ddsHeader.ddsPixelFormat.RBitMask == 0x0000f000) {
                    format = Image::Format::A4B4G4R4;
                } else {
                    format = Image::Format::A4R4G4B4;
                }
            } else if (ddsHeader.ddsPixelFormat.ABitMask == 0x0000f000) {
                if (ddsHeader.ddsPixelFormat.RBitMask == 0x00000f00) {
                    format = Image::Format::B4G4R4A4;
                } else {
                    format = Image::Format::R4G4B4A4;
                }
            } else if (ddsHeader.ddsPixelFormat.ABitMask == 0x00008000) {
                if (ddsHeader.ddsPixelFormat.RBitMask == 0x00007c00) {
                    format = Image::Format::B5G5R5A1;
                } else {
                    format = Image::Format::R5G5B5A1;
                }
            } else if (ddsHeader.ddsPixelFormat.ABitMask == 0x00000001) {
                if (ddsHeader.ddsPixelFormat.RBitMask == 0x0000f800) {
                    format = Image::Format::A1B5G5R5;
                } else {
                    format = Image::Format::A1R5G5B5;
                }
            } else if (ddsHeader.ddsPixelFormat.ABitMask == 0x0000ff00) {
                format = Image::Format::L8A8;
            } else {
                if (ddsHeader.ddsPixelFormat.GBitMask == 0x000000f0) {
                    if (ddsHeader.ddsPixelFormat.RBitMask == 0x00000f00) {
                        format = Image::Format::B4G4R4X4;
                    } else {
                        format = Image::Format::R4G4B4X4;
                    }
                } else if (ddsHeader.ddsPixelFormat.GBitMask == 0x000003e0) {
                    if (ddsHeader.ddsPixelFormat.RBitMask == 0x00007c00) {
                        format = Image::Format::B5G5R5X1;
                    } else {
                        format = Image::Format::R5G5B5X1;
                    }
                } else if (ddsHeader.ddsPixelFormat.GBitMask == 0x000007e0) {
                    if (ddsHeader.ddsPixelFormat.RBitMask == 0x0000f800) {
                        format = Image::Format::B5G6R5;
                    } else {
                        format = Image::Format::R5G6B5;
                    }
                } else {
                    //format = Image::Format::L_16;
                    assert(0);
                }
            }
            break;
        case 1:
            if (ddsHeader.ddsPixelFormat.ABitMask == 0x000000ff) {
                format = Image::Format::A8;
            } else {
                format = Image::Format::L8;
            }
            break;
        default:
            format = Image::Format::Unknown;
            return false;
        }
        break;
    case DDS_FORMAT_R8G8B8:
        format = Image::Format::B8G8R8;
        break;
    case DDS_FORMAT_A8R8G8B8:
        format = Image::Format::B8G8R8A8;
        break;
    case DDS_FORMAT_A2B10G10R10:
        format = Image::Format::R10G10B10A2;
        break;
    case DDS_FORMAT_X8R8G8B8:
        format = Image::Format::B8G8R8X8;
        break;
    case DDS_FORMAT_R5G6B5:
        format = Image::Format::B5G6R5;
        break;
    case DDS_FORMAT_X1R5G5B5:
        format = Image::Format::B5G5R5X1;
        break;
    case DDS_FORMAT_A1R5G5B5:
        format = Image::Format::B5G5R5A1;
        break;
    case DDS_FORMAT_A4R4G4B4:
        format = Image::Format::B4G4R4A4;
        break;
    case DDS_FORMAT_X4R4G4B4:
        format = Image::Format::B4G4R4X4;
        break;
    case DDS_FORMAT_A8B8G8R8:
        format = Image::Format::R8G8B8A8;
        break;
    case DDS_FORMAT_X8B8G8R8:
        format = Image::Format::R8G8B8X8;
        break;
    case DDS_FORMAT_L8:
        format = Image::Format::L8;
        break;
    case DDS_FORMAT_A8:
        format = Image::Format::A8;
        break;
    case DDS_FORMAT_A8L8:
        format = Image::Format::L8A8;
        break;
    case DDS_FORMAT_R16F:
        format = Image::Format::R16_FLOAT;
        break;
    case DDS_FORMAT_G16R16F:
        format = Image::Format::R16G16_FLOAT;
        break;
    case DDS_FORMAT_A16B16G16R16F:
        format = Image::Format::R16G16B16A16_FLOAT;
        break;
    case DDS_FORMAT_R32F:
        format = Image::Format::R32_FLOAT;
        break;
    case DDS_FORMAT_G32R32F:
        format = Image::Format::R32G32_FLOAT;
        break;
    case DDS_FORMAT_A32B32G32R32F:
        format = Image::Format::R32G32B32A32_FLOAT;
        break;
    case DDS_FORMAT_DXT1:
        format = Image::Format::BC1;
        break;
    case DDS_FORMAT_DXT3:
        format = Image::Format::BC2;
        break;
    case DDS_FORMAT_DXT5:
        if (ddsHeader.ddsPixelFormat.RGBBitCount == MAKE_FOURCC('x', 'G', 'B', 'R')) {
            format = Image::Format::BC3XGBR;
        } else {
            format = Image::Format::BC3;
        }
        break;
    case MAKE_FOURCC('R', 'X', 'G', 'B'):   // doom3 RXGB
        format = Image::Format::BC3XGBR;
        break;
    case MAKE_FOURCC('A', 'T', 'I', '1'):
        format = Image::Format::BC4;
        break;
    case MAKE_FOURCC('A', 'T', 'I', '2'):   // ATI 3DTc
        format = Image::Format::BC5;
        break;
    default:
        format = Image::Format::Unknown;
        return false;
    }
    return true;
}

static bool ParseDX10Format(const DdsFileHeaderDX10 &dx10Header, Image::Format &format, Image::GammaSpace &gammaSpace) {
    switch (dx10Header.dxgiFormat) {
    case DX10_FORMAT_R8_UNORM: format = Image::Format::R8; break;
    case DX10_FORMAT_R8_SNORM: format = Image::Format::R8_SNORM; break;
    case DX10_FORMAT_R8_UINT: format = Image::Format::R8_UINT; break;
    case DX10_FORMAT_R8_SINT: format = Image::Format::R8_SINT; break;
    case DX10_FORMAT_R8G8_UNORM: format = Image::Format::R8G8; break;
    case DX10_FORMAT_R8G8_SNORM: format = Image::Format::R8G8_SNORM; break;
    case DX10_FORMAT_R8G8_UINT: format = Image::Format::R8G8_UINT; break;
    case DX10_FORMAT_R8G8_SINT: format = Image::Format::R8G8_SINT; break;
    case DX10_FORMAT_R8G8B8A8_UNORM:
    case DX10_FORMAT_R8G8B8A8_UNORM_SRGB: format = Image::Format::R8G8B8A8; break;
    case DX10_FORMAT_R8G8B8A8_SNORM: format = Image::Format::R8G8B8A8_SNORM; break;
    case DX10_FORMAT_R8G8B8A8_UINT: format = Image::Format::R8G8B8A8_UINT; break;
    case DX10_FORMAT_R8G8B8A8_SINT: format = Image::Format::R8G8B8A8_SINT; break;
    case DX10_FORMAT_B8G8R8A8_UNORM:
    case DX10_FORMAT_B8G8R8A8_UNORM_SRGB: format = Image::Format::B8G8R8A8; break;
    case DX10_FORMAT_B8G8R8X8_UNORM:
    case DX10_FORMAT_B8G8R8X8_UNORM_SRGB: format = Image::Format::B8G8R8X8; break;
    case DX10_FORMAT_R16_FLOAT: format = Image::Format::R16_FLOAT; break;
    case DX10_FORMAT_R16G16_FLOAT: format = Image::Format::R16G16_FLOAT; break;
    case DX10_FORMAT_R16G16B16A16_FLOAT: format = Image::Format::R16G16B16A16_FLOAT; break;
    case DX10_FORMAT_R32_FLOAT: format = Image::Format::R32_FLOAT; break;
    case DX10_FORMAT_R32_SINT: format = Image::Format::R32_SINT; break;
    case DX10_FORMAT_R32_UINT: format = Image::Format::R32_UINT; break;
    case DX10_FORMAT_R32G32_FLOAT: format = Image::Format::R32G32_FLOAT; break;
    case DX10_FORMAT_R32G32_UINT: format = Image::Format::R32G32_UINT; break;
    case DX10_FORMAT_R32G32_SINT: format = Image::Format::R32G32_SINT; break;
    case DX10_FORMAT_R32G32B32_FLOAT: format = Image::Format::R32G32B32_FLOAT; break;
    case DX10_FORMAT_R32G32B32_UINT: format = Image::Format::R32G32B32_UINT; break;
    case DX10_FORMAT_R32G32B32_SINT: format = Image::Format::R32G32B32_SINT; break;
    case DX10_FORMAT_R32G32B32A32_FLOAT: format = Image::Format::R32G32B32A32_FLOAT; break;
    case DX10_FORMAT_R32G32B32A32_UINT: format = Image::Format::R32G32B32A32_UINT; break;
    case DX10_FORMAT_R32G32B32A32_SINT: format = Image::Format::R32G32B32A32_SINT; break;
    case DX10_FORMAT_R9G9B9E5_SHAREDEXP: format = Image::Format::R9G9B9E5_FLOAT; break;
    case DX10_FORMAT_R11G11B10_FLOAT: format = Image::Format::R11G11B10_FLOAT; break;
    case DX10_FORMAT_R10G10B10A2_UNORM: format = Image::Format::R10G10B10A2; break;
    case DX10_FORMAT_BC1_UNORM:
    case DX10_FORMAT_BC1_UNORM_SRGB: format = Image::Format::BC1; break;
    case DX10_FORMAT_BC2_UNORM:
    case DX10_FORMAT_BC2_UNORM_SRGB: format = Image::Format::BC2; break;
    case DX10_FORMAT_BC3_UNORM:
    case DX10_FORMAT_BC3_UNORM_SRGB: format = Image::Format::BC3; break;
    case DX10_FORMAT_BC4_UNORM: format = Image::Format::BC4; break;
    case DX10_FORMAT_BC5_UNORM: format = Image::Format::BC5; break;
    case DX10_FORMAT_BC6H_UF16: format = Image::Format::BC6H_UF16; break;
    case DX10_FORMAT_BC6H_SF16: format = Image::Format::BC6H_SF16; break;
    default:
        format = Image::Format::Unknown;
        return false;
    }

    if (dx10Header.dxgiFormat == DX10_FORMAT_R8G8B8A8_UNORM_SRGB ||
        dx10Header.dxgiFormat == DX10_FORMAT_B8G8R8A8_UNORM_SRGB ||
        dx10Header.dxgiFormat == DX10_FORMAT_B8G8R8X8_UNORM_SRGB ||
        dx10Header.dxgiFormat == DX10_FORMAT_BC1_UNORM_SRGB ||
        dx10Header.dxgiFormat == DX10_FORMAT_BC2_UNORM_SRGB ||
        dx10Header.dxgiFormat == DX10_FORMAT_BC3_UNORM_SRGB) {
        gammaSpace = Image::GammaSpace::sRGB;
    } else if (dx10Header.dxgiFormat == DX10_FORMAT_BC6H_UF16 ||
        dx10Header.dxgiFormat == DX10_FORMAT_BC6H_SF16) {
        gammaSpace = Image::GammaSpace::Linear;
    } else if (
        dx10Header.dxgiFormat == DX10_FORMAT_R8_UNORM ||
        dx10Header.dxgiFormat == DX10_FORMAT_R8G8_UNORM ||
        dx10Header.dxgiFormat == DX10_FORMAT_R8G8B8A8_UNORM ||
        dx10Header.dxgiFormat == DX10_FORMAT_B8G8R8A8_UNORM ||
        dx10Header.dxgiFormat == DX10_FORMAT_B8G8R8X8_UNORM ||
        dx10Header.dxgiFormat == DX10_FORMAT_BC1_UNORM ||
        dx10Header.dxgiFormat == DX10_FORMAT_BC2_UNORM ||
        dx10Header.dxgiFormat == DX10_FORMAT_BC3_UNORM) {
        gammaSpace = Image::GammaSpace::Linear;
    }
    return true;
}

bool Image::LoadDDSFromMemory(const char *name, const byte *data, size_t size) {
    const byte *ptr = data;
    const uint32_t fourcc = *(uint32_t *)ptr;
    ptr += sizeof(uint32_t);
    
    if (fourcc != DDS_MAGIC) {
        BE_WARNLOG("Image::LoadDDSFromMemory: bad DDS format %s\n", name);
        return false;
    }

    DdsFileHeader *header = (DdsFileHeader *)ptr;
    ptr += sizeof(DdsFileHeader);

    constexpr int essentialFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
    
    if (header->size != sizeof(DdsFileHeader) || (header->flags & essentialFlags) != essentialFlags) {
        BE_WARNLOG("Image::LoadDDSFromMemory: bad DDS format %s\n", name);
        return false;
    }

    gammaSpace = Image::GammaSpace::DontCare;

    int ddsArraySize = 1;

    if (header->ddsPixelFormat.fourCC == MAKE_FOURCC('D', 'X', '1', '0')) {
        DdsFileHeaderDX10 *dx10Header = (DdsFileHeaderDX10 *)ptr;
        ptr += sizeof(DdsFileHeaderDX10);

        // NOTE: arraySize is independent of the cubemap face count.
        // each cubemap always has 6 faces. dx10Header->arraySize specifies the number of cubemaps in the array.
        ddsArraySize = dx10Header->arraySize;

        ParseDX10Format(*dx10Header, format, gammaSpace);
    } else {
        ParseDDSFormat(*header, format);
    }

    if (format == Format::Unknown) {
        BE_WARNLOG("Image::LoadDDSFromMemory: Unknown pixel format %s\n", name);
        return false;
    }

    if (this->gammaSpace == Image::GammaSpace::DontCare) {
        if (Image::NeedFloatConversion(format) || format == Format::BC4 || format == Format::BC5) {
            this->gammaSpace = Image::GammaSpace::Linear;
        } else {
            this->gammaSpace = Image::GammaSpace::sRGB;
        }
    }

    bool isCube = header->ddsCaps.caps2 & DDSCAPS2_CUBEMAP ? true : false;

    this->width = header->width;
    this->height = header->height;
    this->depth = BE1::Max((int)header->depth, 1);
    this->numMipLevels = BE1::Max((int)header->mipmapCount, 1);
    this->arraySize = ddsArraySize * (isCube ? 6 : 1);
    this->flags = isCube ? Flag::CubeMap : Flag::None;

    size_t bufSize = SizeInBytes(0, numMipLevels);
    this->pic = (byte *)Mem_Alloc256(bufSize);
    this->alloced = true;

    if (isCube) {
        // DDS cubemap slices use Image::CubeMapFace order and engine-axis orientation.
        // Copy face texels verbatim without remapping, rotating, or flipping them.
        for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
            size_t sliceSize = SizeInBytesForSlice(mipLevel);

            for (int sliceIndex = 0; sliceIndex < arraySize; sliceIndex++) {
                byte *dest = GetPixels(mipLevel, sliceIndex);

                simdProcessor->Memcpy(dest, ptr, static_cast<int>(sliceSize));
                ptr += sliceSize;
            }
        }
    } else {
        simdProcessor->Memcpy(this->pic, ptr, static_cast<int>(bufSize));
    }

    return true;
}

static bool LoadDDSInfo(File *fp, DdsFileHeader &header, Image::Format &format, Image::GammaSpace &gammaSpace, int &arraySize) {
    uint32_t fourcc;
    fp->ReadUInt32(fourcc);
    if (fourcc != DDS_MAGIC) {
        BE_WARNLOG("LoadDDSInfo: bad DDS format %s\n", fp->GetFilename());
        return false;
    }

    fp->Read(&header, sizeof(header));

    constexpr int essentialFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;

    if (header.size != sizeof(DdsFileHeader) || (header.flags & essentialFlags) != essentialFlags) {
        BE_WARNLOG("LoadDDSInfo: bad DDS format %s\n", fp->GetFilename());
        return false;
    }

    gammaSpace = Image::GammaSpace::DontCare;

    arraySize = 1;

    if (header.ddsPixelFormat.fourCC == MAKE_FOURCC('D', 'X', '1', '0')) {
        DdsFileHeaderDX10 dx10Header;
        fp->Read(&dx10Header, sizeof(dx10Header));

        // NOTE: arraySize is independent of the cubemap face count.
        // each cubemap always has 6 faces. dx10Header->arraySize specifies the number of cubemaps in the array.
        arraySize = dx10Header.arraySize;

        ParseDX10Format(dx10Header, format, gammaSpace);
    } else {
        ParseDDSFormat(header, format);
    }

    if (format == Image::Format::Unknown) {
        BE_WARNLOG("LoadDDSInfo: Unknown pixel format %s\n", fp->GetFilename());
        return false;
    }

    if (gammaSpace == Image::GammaSpace::DontCare) {
        if (Image::NeedFloatConversion(format) || format == Image::Format::BC4 || format == Image::Format::BC5) {
            gammaSpace = Image::GammaSpace::Linear;
        } else {
            gammaSpace = Image::GammaSpace::sRGB;
        }
    }

    return true;
}

bool Image::LoadDDSMipRangeData(File *fp, int32_t originalWidth, int32_t originalHeight, int32_t originalDepth, int32_t ddsArraySize, int32_t originalMipLevels, bool isCubemap, int32_t firstMip, int32_t mipCount, MipRange *outInfo) {
    // DDS depth 와 mip count 값은 0 일 수 있기 때문에 여기서 최소 1 로 변경해준다.
    originalDepth = Max(1, originalDepth);
    originalMipLevels = Max(1, originalMipLevels);

    Clamp(firstMip, 0, originalMipLevels - 1);
    Clamp(mipCount, 1, originalMipLevels - firstMip);

    this->width = BE1::Max(1, originalWidth >> firstMip);
    this->height = BE1::Max(1, originalHeight >> firstMip);
    this->depth = BE1::Max(1, originalDepth >> firstMip);
    this->numMipLevels = mipCount;
    this->arraySize = ddsArraySize * (isCubemap ? 6 : 1);
    this->flags = isCubemap ? Flag::CubeMap : Flag::None;

    size_t bufSize = SizeInBytes(0, numMipLevels);
    this->pic = (byte *)Mem_Alloc256(bufSize);
    this->alloced = true;

    int seekStart = fp->Tell();
    uint64_t seekOffset = Image::MemRequired(originalWidth, originalHeight, originalDepth, firstMip, arraySize, format);

    // NOTE: fp 가 zip 으로 압축된 파일인 경우 Seek 을 사용할 수 없음에 주의
    fp->Seek(seekStart + static_cast<int64_t>(seekOffset));

    // NOTE: DDS 는 mip-major order 로 저장되고, Image 는 slice-major order 로 저장된다.
    // Cubemap face 순서와 방향은 동일하므로 texel 을 변환하지 않고 그대로 읽는다.
    for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
        size_t sliceSize = SizeInBytesForSlice(mipLevel);

        for (int sliceIndex = 0; sliceIndex < arraySize; sliceIndex++) {
            byte *dest = GetPixels(mipLevel, sliceIndex);
            size_t readSize = fp->Read(dest, sliceSize);
            if (readSize != sliceSize) {
                Mem_AlignedFree(this->pic);
                this->pic = nullptr;
                this->alloced = false;
                return false;
            }
        }
    }

    if (outInfo) {
        outInfo->originalWidth = originalWidth;
        outInfo->originalHeight = originalHeight;
        outInfo->originalDepth = originalDepth;
        outInfo->originalMipLevels = originalMipLevels;
        outInfo->firstMip = firstMip;
        outInfo->mipCount = mipCount;
    }
    return true;
}

bool Image::LoadDDSMipRange(const char *filename, int32_t firstMip, int32_t mipCount, MipRange *outInfo) {
    if (!filename || filename[0] == 0) {
        return false;
    }

    File *fp = fileSystem.OpenFileRead(filename, false);
    if (!fp) {
        return false;
    }

    bool ret = LoadDDSMipRange(fp, firstMip, mipCount, outInfo);

    fileSystem.CloseFile(fp);
    return ret;
}

bool Image::LoadDDSMipRange(File *fp, int32_t firstMip, int32_t mipCount, MipRange *outInfo) {
    if (!fp) {
        return false;
    }

    DdsFileHeader header;
    int ddsArraySize;
    if (!LoadDDSInfo(fp, header, format, gammaSpace, ddsArraySize)) {
        return false;
    }

    bool isCubemap = (header.ddsCaps.caps2 & DDSCAPS2_CUBEMAP) ? true : false;

    if (!LoadDDSMipRangeData(fp, header.width, header.height, header.depth, ddsArraySize, header.mipmapCount, isCubemap, firstMip, mipCount, outInfo)) {
        return false;
    }

    return true;
}

// Returns the first mip level whose largest dimension is less than or equal to maxDimension.
int32_t Image::ComputeFirstMipForMaxDimension(int32_t width, int32_t height, int32_t depth, int32_t numMipLevels, int32_t maxDimension) {
    if (maxDimension <= 0) {
        return 0;
    }

    for (int mipLevel = 0; mipLevel < numMipLevels; ++mipLevel) {
        int mipWidth = BE1::Max(1, width >> mipLevel);
        int mipHeight = BE1::Max(1, height >> mipLevel);
        int mipDepth = BE1::Max(1, depth >> mipLevel);

        int mipMaxDimension = BE1::Max(mipWidth, BE1::Max(mipHeight, mipDepth));
        if (mipMaxDimension <= maxDimension) {
            return mipLevel;
        }
    }

    return BE1::Max(0, numMipLevels - 1);
}

bool Image::LoadDDSMipRangeForMaxDimension(const char *filename, int32_t maxDimension, MipRange *outInfo) {
    if (!filename || filename[0] == 0) {
        return false;
    }

    File *fp = fileSystem.OpenFileRead(filename, false);
    if (!fp) {
        return false;
    }

    bool ret = LoadDDSMipRangeForMaxDimension(fp, maxDimension, outInfo);

    fileSystem.CloseFile(fp);
    return ret;
}

bool Image::LoadDDSMipRangeForMaxDimension(File *fp, int32_t maxDimension, MipRange *outInfo) {
    if (!fp) {
        return false;
    }

    DdsFileHeader header;
    int ddsArraySize;
    if (!LoadDDSInfo(fp, header, format, gammaSpace, ddsArraySize)) {
        return false;
    }

    bool isCubemap = (header.ddsCaps.caps2 & DDSCAPS2_CUBEMAP) ? true : false;

    int32_t firstMip = Image::ComputeFirstMipForMaxDimension(header.width, header.height, header.depth, header.mipmapCount, maxDimension);
    int32_t mipCount = header.mipmapCount - firstMip;

    if (!LoadDDSMipRangeData(fp, header.width, header.height, header.depth, ddsArraySize, header.mipmapCount, isCubemap, firstMip, mipCount, outInfo)) {
        return false;
    }

    return true;
}

bool Image::WriteDDS(File *fp) const {
    if (!fp) {
        return false;
    }

    const bool isCubemap = HasFlag(flags, Flag::CubeMap);
    if (isCubemap && (arraySize < 6 || arraySize % 6 != 0)) {
        BE_WARNLOG("Image::WriteDDS: cubemap array size must be a positive multiple of 6\n");
        return false;
    }

    fp->WriteUInt32(DDS_MAGIC);

    DdsFileHeader header;
    memset(&header, 0, sizeof(header));
    header.size = sizeof(header);
    header.flags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
    header.height = height;
    header.width = width;
    header.pitchOrLinearSize = 0;
    header.ddsCaps.caps1 = DDSCAPS_TEXTURE;

    if (numMipLevels > 1) {
        header.flags |= DDSD_MIPMAPCOUNT;
        header.ddsCaps.caps1 |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
        header.mipmapCount = numMipLevels;
    }

    if (depth > 1) {
        header.flags |= DDSD_DEPTH;
        header.ddsCaps.caps2 |= DDSCAPS2_VOLUME;
        header.depth = depth;
    }

    if (isCubemap) {
        header.ddsCaps.caps1 |= DDSCAPS_COMPLEX;
        header.ddsCaps.caps2 |= DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES;
    }

    if (IsCompressed()) {
        header.flags |= DDSD_LINEARSIZE;
        header.pitchOrLinearSize = static_cast<uint32_t>(SizeInBytesForSlice());
    } else {
        header.flags |= DDSD_PITCH;
        header.pitchOrLinearSize = (width * BytesPerPixel() + 7) / 8;
    }

    header.ddsPixelFormat.size = sizeof(header.ddsPixelFormat);

    DdsFileHeaderDX10 dx10Header;
    memset(&dx10Header, 0, sizeof(dx10Header));
    dx10Header.resourceDimension = depth > 1 ? DX10_RESOURCE_DIMENSION_TEXTURE3D : DX10_RESOURCE_DIMENSION_TEXTURE2D;
    dx10Header.miscFlag = isCubemap ? DX10_RESOURCE_MISC_TEXTURECUBE : 0;
    dx10Header.arraySize = arraySize / (isCubemap ? 6 : 1);
    dx10Header.miscFlag2 = DX10_RESOURCE_MISC2_UNKNOWN;

    switch (format) {
    case Format::A8B8G8R8:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0xFF000000;
        header.ddsPixelFormat.GBitMask = 0x00FF0000;
        header.ddsPixelFormat.BBitMask = 0x0000FF00;
        header.ddsPixelFormat.ABitMask = 0x000000FF;
        break;
    case Format::A8R8G8B8:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0x0000FF00;
        header.ddsPixelFormat.GBitMask = 0x00FF0000;
        header.ddsPixelFormat.BBitMask = 0xFF000000;
        header.ddsPixelFormat.ABitMask = 0x000000FF;
        break;
    case Format::B8G8R8A8:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0x00FF0000;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x000000FF;
        header.ddsPixelFormat.ABitMask = 0xFF000000;
        break;
    case Format::R8G8B8A8:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0x000000FF;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x00FF0000;
        header.ddsPixelFormat.ABitMask = 0xFF000000;
        break;
    case Format::B8G8R8X8:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0x00FF0000;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x000000FF;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::R8G8B8X8:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0x000000FF;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x00FF0000;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::B8G8R8:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 24;
        header.ddsPixelFormat.RBitMask = 0x00FF0000;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x000000FF;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::R8G8B8:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 24;
        header.ddsPixelFormat.RBitMask = 0x000000FF;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x00FF0000;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::A4B4G4R4:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000F000;
        header.ddsPixelFormat.GBitMask = 0x00000F00;
        header.ddsPixelFormat.BBitMask = 0x000000F0;
        header.ddsPixelFormat.ABitMask = 0x0000000F;
        break;
    case Format::A4R4G4B4:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x000000F0;
        header.ddsPixelFormat.GBitMask = 0x00000F00;
        header.ddsPixelFormat.BBitMask = 0x0000F000;
        header.ddsPixelFormat.ABitMask = 0x0000000F;
        break;
    case Format::B4G4R4A4:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x00000F00;
        header.ddsPixelFormat.GBitMask = 0x000000F0;
        header.ddsPixelFormat.BBitMask = 0x0000000F;
        header.ddsPixelFormat.ABitMask = 0x0000F000;
        break;
    case Format::R4G4B4A4:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000000F;
        header.ddsPixelFormat.GBitMask = 0x000000F0;
        header.ddsPixelFormat.BBitMask = 0x00000F00;
        header.ddsPixelFormat.ABitMask = 0x0000F000;
        break;
    case Format::B5G5R5A1:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x00007C00;
        header.ddsPixelFormat.GBitMask = 0x000003E0;
        header.ddsPixelFormat.BBitMask = 0x0000001F;
        header.ddsPixelFormat.ABitMask = 0x00008000;
        break;
    case Format::R5G5B5A1:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000001F;
        header.ddsPixelFormat.GBitMask = 0x000003E0;
        header.ddsPixelFormat.BBitMask = 0x00007C00;
        header.ddsPixelFormat.ABitMask = 0x00008000;
        break;
    case Format::A1B5G5R5:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000F800;
        header.ddsPixelFormat.GBitMask = 0x000007C0;
        header.ddsPixelFormat.BBitMask = 0x0000003E;
        header.ddsPixelFormat.ABitMask = 0x00000001;
        break;
    case Format::A1R5G5B5:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000003E;
        header.ddsPixelFormat.GBitMask = 0x000007C0;
        header.ddsPixelFormat.BBitMask = 0x0000F800;
        header.ddsPixelFormat.ABitMask = 0x00000001;
        break;
    case Format::L8A8:
        header.ddsPixelFormat.flags = DDSPF_LUMINANCE | DDSPF_ALPHAPIXELS;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x000000FF;
        header.ddsPixelFormat.GBitMask = 0x00000000;
        header.ddsPixelFormat.BBitMask = 0x00000000;
        header.ddsPixelFormat.ABitMask = 0x0000FF00;
        break;
    case Format::B4G4R4X4:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x00000F00;
        header.ddsPixelFormat.GBitMask = 0x000000F0;
        header.ddsPixelFormat.BBitMask = 0x0000000F;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::R4G4B4X4:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000000F;
        header.ddsPixelFormat.GBitMask = 0x000000F0;
        header.ddsPixelFormat.BBitMask = 0x00000F00;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::B5G5R5X1:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x00007C00;
        header.ddsPixelFormat.GBitMask = 0x000003E0;
        header.ddsPixelFormat.BBitMask = 0x0000001F;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::R5G5B5X1:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000001F;
        header.ddsPixelFormat.GBitMask = 0x000003E0;
        header.ddsPixelFormat.BBitMask = 0x00007C00;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::B5G6R5:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000F800;
        header.ddsPixelFormat.GBitMask = 0x000007E0;
        header.ddsPixelFormat.BBitMask = 0x0000001F;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::R5G6B5:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000001F;
        header.ddsPixelFormat.GBitMask = 0x000007E0;
        header.ddsPixelFormat.BBitMask = 0x0000F800;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::L8:
        header.ddsPixelFormat.flags = DDSPF_LUMINANCE;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 8;
        header.ddsPixelFormat.RBitMask = 0x000000FF;
        header.ddsPixelFormat.GBitMask = 0x00000000;
        header.ddsPixelFormat.BBitMask = 0x00000000;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case Format::A8:
        header.ddsPixelFormat.flags = DDSPF_ALPHA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 8;
        header.ddsPixelFormat.RBitMask = 0x00000000;
        header.ddsPixelFormat.GBitMask = 0x00000000;
        header.ddsPixelFormat.BBitMask = 0x00000000;
        header.ddsPixelFormat.ABitMask = 0x000000FF;
        break;
    case Format::BC1:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', 'T', '1');
        break;
    case Format::BC2:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', 'T', '3');
        break;
    case Format::BC3:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', 'T', '5');
        break;
    case Format::BC3XGBR:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', 'T', '5');
        header.ddsPixelFormat.RGBBitCount = MAKE_FOURCC('x', 'G', 'B', 'R');
        break;
    case Format::BC4:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('A', 'T', 'I', '1');
        break;
    case Format::BC5:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('A', 'T', 'I', '2'); // ATI 3DTc
        break;
    case Format::R8:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R8_UNORM;
        break;
    case Format::R8G8:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R8G8_UNORM;
        break;
    case Format::R16_FLOAT:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R16_FLOAT;
        break;
    case Format::R16G16_FLOAT:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R16G16_FLOAT;
        break;
    case Format::R16G16B16A16_FLOAT:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R16G16B16A16_FLOAT;
        break;
    case Format::R32_FLOAT:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R32_FLOAT;
        break;
    case Format::R32G32_FLOAT:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R32G32_FLOAT;
        break;
    case Format::R32G32B32_FLOAT:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R32G32B32_FLOAT;
        break;
    case Format::R32G32B32A32_FLOAT:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R32G32B32A32_FLOAT;
        break;
    case Format::R9G9B9E5_FLOAT:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R9G9B9E5_SHAREDEXP;
        break;
    case Format::R11G11B10_FLOAT:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R11G11B10_FLOAT;
        break;
    default:
        BE_WARNLOG("Image::WriteDDS: invalid format '%s' for DDS\n", Image::FormatName(format));
        return false;
    }

    fp->Write(&header, sizeof(header));

    if (header.ddsPixelFormat.fourCC == MAKE_FOURCC('D', 'X', '1', '0')) {
        fp->Write(&dx10Header, sizeof(dx10Header));
    }

    // NOTE: DDS 파일은 mip-major order 로 저장된다.
    // Mip0: Face0, Face1, Face2, ...
    // Mip1: Face0, Face1, Face2, ...
    // 반면, Image 의 데이터는 slice-major order 로 되어있다.
    // Cubemap 은 Image::CubeMapFace 순서와 engine-axis 방향을 그대로 저장한다.
    if (arraySize > 1) {
        for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
            size_t sliceSize = SizeInBytesForSlice(mipLevel);

            for (int sliceIndex = 0; sliceIndex < arraySize; sliceIndex++) {
                const byte *slicePtr = GetPixels(mipLevel, sliceIndex);

                fp->Write(slicePtr, sliceSize);
            }
        }
    } else {
        fp->Write(pic, SizeInBytes(0, numMipLevels));
    }

    return true;
}

bool Image::WriteDDS(const char *filename) const {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Image::WriteDDS: file open error\n");
        return false;
    }

    bool ret = WriteDDS(fp);

    fileSystem.CloseFile(fp);

    return ret;
}

BE_NAMESPACE_END
