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
#include "Core/Heap.h"
#include "Simd/Simd.h"
#include "Math/Math.h"
#include "File/FileSystem.h"
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
    uint32_t mipMapCount;
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

bool Image::LoadDDSFromMemory(const char *name, const byte *data, size_t size) {
    const byte *ptr = data;
    uint32_t fourcc = *(uint32_t *)ptr;
    ptr += sizeof(uint32_t);
    
    if (fourcc != DDS_MAGIC) {
        BE_WARNLOG(L"Image::LoadDDSFromMemory: bad DDS format %hs\n", name);
        return false;
    }

    DdsFileHeader *header = (DdsFileHeader *)ptr;
    ptr += sizeof(DdsFileHeader);

    int essentialFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
    
    if (header->size != sizeof(DdsFileHeader) || (header->flags & essentialFlags) != essentialFlags) {
        BE_WARNLOG(L"Image::LoadDDSFromMemory: bad DDS format %hs\n", name);
        return false;
    }

    if (header->ddsPixelFormat.fourCC == MAKE_FOURCC('D', 'X', '1', '0')) {
        DdsFileHeaderDX10 *dx10Header = (DdsFileHeaderDX10 *)ptr;
        ptr += sizeof(DdsFileHeaderDX10);

        switch (dx10Header->dxgiFormat){
        case DX10_FORMAT_R8_UNORM: format = R_8; break;
        case DX10_FORMAT_R8G8_UNORM: format = RG_8_8; break;
        case DX10_FORMAT_R8G8B8A8_UNORM: format = RGBA_8_8_8_8; break;
        //case DX10_FORMAT_R16_UNORM: format = R_16; break;
        //case DX10_FORMAT_R16G16_UNORM: format = RG_16_16; break;
        //case DX10_FORMAT_R16G16B16A16_UNORM: format = RGBA_16_16_16_16; break;
        case DX10_FORMAT_R16_FLOAT: format = R_16F; break;
        case DX10_FORMAT_R16G16_FLOAT: format = RG_16F_16F; break;
        case DX10_FORMAT_R16G16B16A16_FLOAT: format = RGBA_16F_16F_16F_16F; break;
        case DX10_FORMAT_R32_FLOAT: format = R_32F; break;
        case DX10_FORMAT_R32G32_FLOAT: format = RG_32F_32F; break;
        case DX10_FORMAT_R32G32B32_FLOAT: format = RGB_32F_32F_32F; break;
        case DX10_FORMAT_R32G32B32A32_FLOAT: format = RGBA_32F_32F_32F_32F; break;
        case DX10_FORMAT_R9G9B9E5_SHAREDEXP: format = RGBE_9_9_9_5; break;
        case DX10_FORMAT_R11G11B10_FLOAT: format = RGB_11F_11F_10F; break;
        //case DX10_FORMAT_R10G10B10A2_UNORM: format = RGBA_10_10_10_2; break;
        case DX10_FORMAT_BC1_UNORM: format = RGBA_DXT1; break;
        case DX10_FORMAT_BC2_UNORM: format = RGBA_DXT3; break;
        case DX10_FORMAT_BC3_UNORM: format = RGBA_DXT5; break;
        case DX10_FORMAT_BC4_UNORM: format = DXN1; break;
        case DX10_FORMAT_BC5_UNORM: format = DXN2; break;
        default:
            BE_WARNLOG(L"Image::LoadDDSFromMemory: Unsupported pixel format %hs\n", name);
            return false;
        }
    } else {
        switch (header->ddsPixelFormat.fourCC) {
        case 0:
            switch (header->ddsPixelFormat.RGBBitCount >> 3) {
            case 4:
                if (header->ddsPixelFormat.ABitMask == 0x000000ff) {
                    if (header->ddsPixelFormat.RBitMask == 0xff000000) {
                        this->format = ABGR_8_8_8_8;
                    } else {
                        this->format = ARGB_8_8_8_8;
                    }
                } else if (header->ddsPixelFormat.ABitMask == 0xff00000) {
                    if (header->ddsPixelFormat.RBitMask == 0x00ff0000) {
                        this->format = BGRA_8_8_8_8;
                    } else {
                        this->format = RGBA_8_8_8_8;
                    }
                } else {
                    if (header->ddsPixelFormat.RBitMask == 0x00ff0000) {
                        this->format = BGRX_8_8_8_8;
                    } else {
                        this->format = RGBX_8_8_8_8;
                    }
                }
                break;
            case 3:
                if (header->ddsPixelFormat.RBitMask == 0x00ff0000) {
                    this->format = BGR_8_8_8;
                } else {
                    this->format = RGB_8_8_8;
                }
                break;
            case 2:
                if (header->ddsPixelFormat.ABitMask == 0x0000000f) {
                    if (header->ddsPixelFormat.RBitMask == 0x0000f000) {
                        this->format = ABGR_4_4_4_4;
                    } else {
                        this->format = ARGB_4_4_4_4;
                    }
                } else if (header->ddsPixelFormat.ABitMask == 0x0000f000) {
                    if (header->ddsPixelFormat.RBitMask == 0x00000f00) {
                        this->format = BGRA_4_4_4_4;
                    } else {
                        this->format = RGBA_4_4_4_4;
                    }
                } else if (header->ddsPixelFormat.ABitMask == 0x00008000) {
                    if (header->ddsPixelFormat.RBitMask == 0x00007c00) {
                        this->format = BGRA_5_5_5_1;
                    } else {
                        this->format = RGBA_5_5_5_1;
                    }
                } else if (header->ddsPixelFormat.ABitMask == 0x00000001) {
                    if (header->ddsPixelFormat.RBitMask == 0x0000f800) {
                        this->format = ABGR_1_5_5_5;
                    } else {
                        this->format = ARGB_1_5_5_5;
                    }
                } else if (header->ddsPixelFormat.ABitMask == 0x0000ff00) {
                    this->format = LA_8_8;
                } else {
                    if (header->ddsPixelFormat.GBitMask == 0x000000f0) {
                        if (header->ddsPixelFormat.RBitMask == 0x00000f00) {
                            this->format = BGRX_4_4_4_4;
                        } else {
                            this->format = RGBX_4_4_4_4;
                        }
                    } else if (header->ddsPixelFormat.GBitMask == 0x000003e0) {
                        if (header->ddsPixelFormat.RBitMask == 0x00007c00) {
                            this->format = BGRX_5_5_5_1;
                        } else {
                            this->format = RGBX_5_5_5_1;
                        }
                    } else if (header->ddsPixelFormat.GBitMask == 0x000007e0) {
                        if (header->ddsPixelFormat.RBitMask == 0x0000f800) {
                            this->format = BGR_5_6_5;
                        } else {
                            this->format = RGB_5_6_5;
                        }
                    } else {
                        //this->format = L_16;
                        assert(0);
                    }
                }
                break;
            case 1:
                if (header->ddsPixelFormat.ABitMask == 0x000000ff) {
                    this->format = A_8;
                } else {
                    this->format = L_8;
                }
                break;
            default:
                BE_WARNLOG(L"Image::LoadDDSFromMemory: Unsupported pixel format %hs\n", name);
                return false;
            }
            break;
        case DDS_FORMAT_R8G8B8:
            this->format = BGR_8_8_8;
            break;
        case DDS_FORMAT_A8R8G8B8:
            this->format = BGRA_8_8_8_8;
            break;
        case DDS_FORMAT_X8R8G8B8:
            this->format = BGRX_8_8_8_8;
            break;
        case DDS_FORMAT_R5G6B5:
            this->format = BGR_5_6_5;
            break;
        case DDS_FORMAT_X1R5G5B5:
            this->format = BGRX_5_5_5_1;
            break;
        case DDS_FORMAT_A1R5G5B5:
            this->format = BGRA_5_5_5_1;
            break;
        case DDS_FORMAT_A4R4G4B4:
            this->format = BGRA_4_4_4_4;
            break;
        case DDS_FORMAT_X4R4G4B4:
            this->format = BGRX_4_4_4_4;
            break;
        case DDS_FORMAT_A8B8G8R8:
            this->format = RGBA_8_8_8_8;
            break;
        case DDS_FORMAT_X8B8G8R8:
            this->format = RGBX_8_8_8_8;
            break;
        case DDS_FORMAT_L8:
            this->format = L_8;
            break;
        case DDS_FORMAT_A8:
            this->format = A_8;
            break;
        case DDS_FORMAT_A8L8:
            this->format = LA_8_8;
            break;
        case DDS_FORMAT_R16F:
            this->format = R_16F;
            break;
        case DDS_FORMAT_G16R16F:
            this->format = RG_16F_16F;
            break;
        case DDS_FORMAT_A16B16G16R16F:
            this->format = RGBA_16F_16F_16F_16F;
            break;
        case DDS_FORMAT_R32F:
            this->format = R_32F;
            break;
        case DDS_FORMAT_G32R32F:
            this->format = RG_32F_32F;
            break;
        case DDS_FORMAT_A32B32G32R32F:
            this->format = RGBA_32F_32F_32F_32F;
            break;
        case DDS_FORMAT_DXT1:
            this->format = RGBA_DXT1;
            break;
        case DDS_FORMAT_DXT3:
            this->format = RGBA_DXT3;
            break;
        case DDS_FORMAT_DXT5:
            if (header->ddsPixelFormat.RGBBitCount == MAKE_FOURCC('x', 'G', 'B', 'R')) {
                this->format = XGBR_DXT5;
            } else {
                this->format = RGBA_DXT5;
            }
            break;
        case MAKE_FOURCC('R', 'X', 'G', 'B'):   // doom3 RXGB
            this->format = XGBR_DXT5;
            break;
        case MAKE_FOURCC('A', 'T', 'I', '1'):
            this->format = DXN1;
            break;
        case MAKE_FOURCC('A', 'T', 'I', '2'):   // ATI 3DTc
            this->format = DXN2;
            break;
        default:
            BE_WARNLOG(L"Image::LoadDDSFromMemory: Unsupported pixel format %hs\n", name);
            return false;
        }
    }

    bool isCube = header->ddsCaps.caps2 & DDSCAPS2_CUBEMAP ? true : false;

    this->width = header->width;
    this->height = header->height;
    this->depth = Max((int)header->depth, 1);
    this->numMipmaps = Max((int)header->mipMapCount, 1);
    this->numSlices = isCube ? 6 : 1;
    this->flags = isCube ? CubeMapFlag : 0;

    if (IsFloatFormat() || format == DXN1 || format == DXN2) {
        this->flags |= LinearSpaceFlag;
    }

    int bufSize = GetSize(0, numMipmaps);
    this->pic = (byte *)Mem_Alloc16(bufSize);
    this->alloced = true;

    if (isCube) {
        for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
            for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                int faceSize = GetSliceSize(mipLevel);
                byte *dest = GetPixels(mipLevel) + faceIndex * faceSize;

                simdProcessor->Memcpy(dest, ptr, faceSize);
                ptr += faceSize;
            }
        }
    } else {
        simdProcessor->Memcpy(this->pic, ptr, bufSize);
    }
    
    return true;
}

bool Image::WriteDDS(const char *filename) const {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Image::WriteDDS: file open error\n");
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

    if (numMipmaps > 1) {
        header.flags |= DDSD_MIPMAPCOUNT;
        header.ddsCaps.caps1 |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
        header.mipMapCount = numMipmaps;
    }

    if (depth > 1) {
        header.flags |= DDSD_DEPTH;
        header.ddsCaps.caps2 |= DDSCAPS2_VOLUME;
        header.depth = depth;
    }

    if ((flags & CubeMapFlag) && numSlices == 6) {
        header.ddsCaps.caps1 |= DDSCAPS_COMPLEX;
        header.ddsCaps.caps2 |= DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES;
    }

    if (IsCompressed()) {
        header.flags |= DDSD_LINEARSIZE;
        header.pitchOrLinearSize = GetSliceSize();
    } else {
        header.flags |= DDSD_PITCH;
        header.pitchOrLinearSize = (width * BytesPerPixel() + 7) / 8;
    }

    header.ddsPixelFormat.size = sizeof(header.ddsPixelFormat);

    DdsFileHeaderDX10 dx10Header;
    memset(&dx10Header, 0, sizeof(dx10Header));
    dx10Header.resourceDimension = depth > 1 ? DX10_RESOURCE_DIMENSION_TEXTURE3D : DX10_RESOURCE_DIMENSION_TEXTURE2D;
    dx10Header.miscFlag = numSlices == 6 ? DX10_RESOURCE_MISC_TEXTURECUBE : 0;
    dx10Header.arraySize = 1;
    dx10Header.miscFlag2 = DX10_RESOURCE_MISC2_UNKNOWN;

    switch (format) {
    case ABGR_8_8_8_8:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0xFF000000;
        header.ddsPixelFormat.GBitMask = 0x00FF0000;
        header.ddsPixelFormat.BBitMask = 0x0000FF00;
        header.ddsPixelFormat.ABitMask = 0x000000FF;
        break;
    case ARGB_8_8_8_8:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0x0000FF00;
        header.ddsPixelFormat.GBitMask = 0x00FF0000;
        header.ddsPixelFormat.BBitMask = 0xFF000000;
        header.ddsPixelFormat.ABitMask = 0x000000FF;
        break;
    case BGRA_8_8_8_8:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0x00FF0000;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x000000FF;
        header.ddsPixelFormat.ABitMask = 0xFF000000;
        break;
    case RGBA_8_8_8_8:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0x000000FF;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x00FF0000;
        header.ddsPixelFormat.ABitMask = 0xFF000000;
        break;
    case BGRX_8_8_8_8:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0x00FF0000;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x000000FF;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case RGBX_8_8_8_8:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 32;
        header.ddsPixelFormat.RBitMask = 0x000000FF;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x00FF0000;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case BGR_8_8_8:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 24;
        header.ddsPixelFormat.RBitMask = 0x00FF0000;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x000000FF;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case RGB_8_8_8:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 24;
        header.ddsPixelFormat.RBitMask = 0x000000FF;
        header.ddsPixelFormat.GBitMask = 0x0000FF00;
        header.ddsPixelFormat.BBitMask = 0x00FF0000;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case ABGR_4_4_4_4:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000F000;
        header.ddsPixelFormat.GBitMask = 0x00000F00;
        header.ddsPixelFormat.BBitMask = 0x000000F0;
        header.ddsPixelFormat.ABitMask = 0x0000000F;
        break;
    case ARGB_4_4_4_4:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x000000F0;
        header.ddsPixelFormat.GBitMask = 0x00000F00;
        header.ddsPixelFormat.BBitMask = 0x0000F000;
        header.ddsPixelFormat.ABitMask = 0x0000000F;
        break;
    case BGRA_4_4_4_4:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x00000F00;
        header.ddsPixelFormat.GBitMask = 0x000000F0;
        header.ddsPixelFormat.BBitMask = 0x0000000F;
        header.ddsPixelFormat.ABitMask = 0x0000F000;
        break;
    case RGBA_4_4_4_4:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000000F;
        header.ddsPixelFormat.GBitMask = 0x000000F0;
        header.ddsPixelFormat.BBitMask = 0x00000F00;
        header.ddsPixelFormat.ABitMask = 0x0000F000;
        break;
    case BGRA_5_5_5_1:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x00007C00;
        header.ddsPixelFormat.GBitMask = 0x000003E0;
        header.ddsPixelFormat.BBitMask = 0x0000001F;
        header.ddsPixelFormat.ABitMask = 0x00008000;
        break;
    case RGBA_5_5_5_1:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000001F;
        header.ddsPixelFormat.GBitMask = 0x000003E0;
        header.ddsPixelFormat.BBitMask = 0x00007C00;
        header.ddsPixelFormat.ABitMask = 0x00008000;
        break;
    case ABGR_1_5_5_5:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000F800;
        header.ddsPixelFormat.GBitMask = 0x000007C0;
        header.ddsPixelFormat.BBitMask = 0x0000003E;
        header.ddsPixelFormat.ABitMask = 0x00000001;
        break;
    case ARGB_1_5_5_5:
        header.ddsPixelFormat.flags = DDSPF_RGBA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000003E;
        header.ddsPixelFormat.GBitMask = 0x000007C0;
        header.ddsPixelFormat.BBitMask = 0x0000F800;
        header.ddsPixelFormat.ABitMask = 0x00000001;
        break;
    case LA_8_8:
        header.ddsPixelFormat.flags = DDSPF_LUMINANCE | DDSPF_ALPHAPIXELS;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x000000FF;
        header.ddsPixelFormat.GBitMask = 0x00000000;
        header.ddsPixelFormat.BBitMask = 0x00000000;
        header.ddsPixelFormat.ABitMask = 0x0000FF00;
        break;
    case BGRX_4_4_4_4:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x00000F00;
        header.ddsPixelFormat.GBitMask = 0x000000F0;
        header.ddsPixelFormat.BBitMask = 0x0000000F;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case RGBX_4_4_4_4:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000000F;
        header.ddsPixelFormat.GBitMask = 0x000000F0;
        header.ddsPixelFormat.BBitMask = 0x00000F00;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case BGRX_5_5_5_1:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x00007C00;
        header.ddsPixelFormat.GBitMask = 0x000003E0;
        header.ddsPixelFormat.BBitMask = 0x0000001F;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case RGBX_5_5_5_1:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000001F;
        header.ddsPixelFormat.GBitMask = 0x000003E0;
        header.ddsPixelFormat.BBitMask = 0x00007C00;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case BGR_5_6_5:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000F800;
        header.ddsPixelFormat.GBitMask = 0x000007E0;
        header.ddsPixelFormat.BBitMask = 0x0000001F;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case RGB_5_6_5:
        header.ddsPixelFormat.flags = DDSPF_RGB;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 16;
        header.ddsPixelFormat.RBitMask = 0x0000001F;
        header.ddsPixelFormat.GBitMask = 0x000007E0;
        header.ddsPixelFormat.BBitMask = 0x0000F800;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case L_8:
        header.ddsPixelFormat.flags = DDSPF_LUMINANCE;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 8;
        header.ddsPixelFormat.RBitMask = 0x000000FF;
        header.ddsPixelFormat.GBitMask = 0x00000000;
        header.ddsPixelFormat.BBitMask = 0x00000000;
        header.ddsPixelFormat.ABitMask = 0x00000000;
        break;
    case A_8:
        header.ddsPixelFormat.flags = DDSPF_ALPHA;
        header.ddsPixelFormat.fourCC = 0;
        header.ddsPixelFormat.RGBBitCount = 8;
        header.ddsPixelFormat.RBitMask = 0x00000000;
        header.ddsPixelFormat.GBitMask = 0x00000000;
        header.ddsPixelFormat.BBitMask = 0x00000000;
        header.ddsPixelFormat.ABitMask = 0x000000FF;
        break;
    case RGBA_DXT1:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', 'T', '1');
        break;
    case RGBA_DXT3:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', 'T', '3');
        break;
    case RGBA_DXT5:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', 'T', '5');
        break;
    case XGBR_DXT5:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', 'T', '5');
        header.ddsPixelFormat.RGBBitCount = MAKE_FOURCC('x', 'G', 'B', 'R');
        break;
    case DXN1:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('A', 'T', 'I', '1');
        break;
    case DXN2:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('A', 'T', 'I', '2'); // ATI 3DTc
        break;
    case R_8:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R8_UNORM;
        break;
    case RG_8_8:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R8G8_UNORM;
        break;
    case R_16F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R16_FLOAT;
        break;
    case RG_16F_16F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R16G16_FLOAT;
        break;
    case RGBA_16F_16F_16F_16F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R16G16B16A16_FLOAT;
        break;
    case R_32F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R32_FLOAT;
        break;
    case RG_32F_32F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R32G32_FLOAT;
        break;
    case RGB_32F_32F_32F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R32G32B32_FLOAT;
        break;
    case RGBA_32F_32F_32F_32F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R32G32B32A32_FLOAT;
        break;
    case RGBE_9_9_9_5:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R9G9B9E5_SHAREDEXP;
        break;
    case RGB_11F_11F_10F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DX10_FORMAT_R11G11B10_FLOAT;
        break;
    default:
        fileSystem.CloseFile(fp);
        BE_WARNLOG(L"Image::WriteDDS: invalid format '%hs' for DDS\n", Image::FormatName(format));
        return false;
    }

    fp->Write(&header, sizeof(header));

    if (header.ddsPixelFormat.fourCC == MAKE_FOURCC('D', 'X', '1', '0')) {
        fp->Write(&dx10Header, sizeof(dx10Header));
    }

    fp->Write(pic, GetSize(0, numMipmaps));

    fileSystem.CloseFile(fp);

    return true;
}

BE_NAMESPACE_END
