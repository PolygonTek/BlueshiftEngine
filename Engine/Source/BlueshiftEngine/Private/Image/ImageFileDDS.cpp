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

// 
#define D3D10_RESOURCE_MISC_TEXTURECUBE     0x4
#define D3D10_RESOURCE_DIMENSION_BUFFER     1
#define D3D10_RESOURCE_DIMENSION_TEXTURE1D  2
#define D3D10_RESOURCE_DIMENSION_TEXTURE2D  3
#define D3D10_RESOURCE_DIMENSION_TEXTURE3D  4

enum DDS_FORMAT {
    DDS_FORMAT_UNKNOWN = 0,
    DDS_FORMAT_R32G32B32A32_TYPELESS = 1,
    DDS_FORMAT_R32G32B32A32_FLOAT = 2,
    DDS_FORMAT_R32G32B32A32_UINT = 3,
    DDS_FORMAT_R32G32B32A32_SINT = 4,
    DDS_FORMAT_R32G32B32_TYPELESS = 5,
    DDS_FORMAT_R32G32B32_FLOAT = 6,
    DDS_FORMAT_R32G32B32_UINT = 7,
    DDS_FORMAT_R32G32B32_SINT = 8,
    DDS_FORMAT_R16G16B16A16_TYPELESS = 9,
    DDS_FORMAT_R16G16B16A16_FLOAT = 10,
    DDS_FORMAT_R16G16B16A16_UNORM = 11,
    DDS_FORMAT_R16G16B16A16_UINT = 12,
    DDS_FORMAT_R16G16B16A16_SNORM = 13,
    DDS_FORMAT_R16G16B16A16_SINT = 14,
    DDS_FORMAT_R32G32_TYPELESS = 15,
    DDS_FORMAT_R32G32_FLOAT = 16,
    DDS_FORMAT_R32G32_UINT = 17,
    DDS_FORMAT_R32G32_SINT = 18,
    DDS_FORMAT_R32G8X24_TYPELESS = 19,
    DDS_FORMAT_D32_FLOAT_S8X24_UINT = 20,
    DDS_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
    DDS_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
    DDS_FORMAT_R10G10B10A2_TYPELESS = 23,
    DDS_FORMAT_R10G10B10A2_UNORM = 24,
    DDS_FORMAT_R10G10B10A2_UINT = 25,
    DDS_FORMAT_R11G11B10_FLOAT = 26,
    DDS_FORMAT_R8G8B8A8_TYPELESS = 27,
    DDS_FORMAT_R8G8B8A8_UNORM = 28,
    DDS_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    DDS_FORMAT_R8G8B8A8_UINT = 30,
    DDS_FORMAT_R8G8B8A8_SNORM = 31,
    DDS_FORMAT_R8G8B8A8_SINT = 32,
    DDS_FORMAT_R16G16_TYPELESS = 33,
    DDS_FORMAT_R16G16_FLOAT = 34,
    DDS_FORMAT_R16G16_UNORM = 35,
    DDS_FORMAT_R16G16_UINT = 36,
    DDS_FORMAT_R16G16_SNORM = 37,
    DDS_FORMAT_R16G16_SINT = 38,
    DDS_FORMAT_R32_TYPELESS = 39,
    DDS_FORMAT_D32_FLOAT = 40,
    DDS_FORMAT_R32_FLOAT = 41,
    DDS_FORMAT_R32_UINT = 42,
    DDS_FORMAT_R32_SINT = 43,
    DDS_FORMAT_R24G8_TYPELESS = 44,
    DDS_FORMAT_D24_UNORM_S8_UINT = 45,
    DDS_FORMAT_R24_UNORM_X8_TYPELESS = 46,
    DDS_FORMAT_X24_TYPELESS_G8_UINT = 47,
    DDS_FORMAT_R8G8_TYPELESS = 48,
    DDS_FORMAT_R8G8_UNORM = 49,
    DDS_FORMAT_R8G8_UINT = 50,
    DDS_FORMAT_R8G8_SNORM = 51,
    DDS_FORMAT_R8G8_SINT = 52,
    DDS_FORMAT_R16_TYPELESS = 53,
    DDS_FORMAT_R16_FLOAT = 54,
    DDS_FORMAT_D16_UNORM = 55,
    DDS_FORMAT_R16_UNORM = 56,
    DDS_FORMAT_R16_UINT = 57,
    DDS_FORMAT_R16_SNORM = 58,
    DDS_FORMAT_R16_SINT = 59,
    DDS_FORMAT_R8_TYPELESS = 60,
    DDS_FORMAT_R8_UNORM = 61,
    DDS_FORMAT_R8_UINT = 62,
    DDS_FORMAT_R8_SNORM = 63,
    DDS_FORMAT_R8_SINT = 64,
    DDS_FORMAT_A8_UNORM = 65,
    DDS_FORMAT_R1_UNORM = 66,
    DDS_FORMAT_R9G9B9E5_SHAREDEXP = 67,
    DDS_FORMAT_R8G8_B8G8_UNORM = 68,
    DDS_FORMAT_G8R8_G8B8_UNORM = 69,
    DDS_FORMAT_BC1_TYPELESS = 70,
    DDS_FORMAT_BC1_UNORM = 71,
    DDS_FORMAT_BC1_UNORM_SRGB = 72,
    DDS_FORMAT_BC2_TYPELESS = 73,
    DDS_FORMAT_BC2_UNORM = 74,
    DDS_FORMAT_BC2_UNORM_SRGB = 75,
    DDS_FORMAT_BC3_TYPELESS = 76,
    DDS_FORMAT_BC3_UNORM = 77,
    DDS_FORMAT_BC3_UNORM_SRGB = 78,
    DDS_FORMAT_BC4_TYPELESS = 79,
    DDS_FORMAT_BC4_UNORM = 80,
    DDS_FORMAT_BC4_SNORM = 81,
    DDS_FORMAT_BC5_TYPELESS = 82,
    DDS_FORMAT_BC5_UNORM = 83,
    DDS_FORMAT_BC5_SNORM = 84,
    DDS_FORMAT_B5G6R5_UNORM = 85,
    DDS_FORMAT_B5G5R5A1_UNORM = 86,
    DDS_FORMAT_B8G8R8A8_UNORM = 87,
    DDS_FORMAT_B8G8R8X8_UNORM = 88,
    DDS_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
    DDS_FORMAT_B8G8R8A8_TYPELESS = 90,
    DDS_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
    DDS_FORMAT_B8G8R8X8_TYPELESS = 92,
    DDS_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
    DDS_FORMAT_BC6H_TYPELESS = 94,
    DDS_FORMAT_BC6H_UF16 = 95,
    DDS_FORMAT_BC6H_SF16 = 96,
    DDS_FORMAT_BC7_TYPELESS = 97,
    DDS_FORMAT_BC7_UNORM = 98,
    DDS_FORMAT_BC7_UNORM_SRGB = 99,
    DDS_FORMAT_AYUV = 100,
    DDS_FORMAT_Y410 = 101,
    DDS_FORMAT_Y416 = 102,
    DDS_FORMAT_NV12 = 103,
    DDS_FORMAT_P010 = 104,
    DDS_FORMAT_P016 = 105,
    DDS_FORMAT_420_OPAQUE = 106,
    DDS_FORMAT_YUY2 = 107,
    DDS_FORMAT_Y210 = 108,
    DDS_FORMAT_Y216 = 109,
    DDS_FORMAT_NV11 = 110,
    DDS_FORMAT_AI44 = 111,
    DDS_FORMAT_IA44 = 112,
    DDS_FORMAT_P8 = 113,
    DDS_FORMAT_A8P8 = 114,
    DDS_FORMAT_B4G4R4A4_UNORM = 115
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
    uint32_t reserved;
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
        case DDS_FORMAT_R8_UNORM: format = R_8; break;
        case DDS_FORMAT_R8G8_UNORM: format = RG_8_8; break;
        case DDS_FORMAT_R8G8B8A8_UNORM: format = RGBA_8_8_8_8; break;
        //case DDS_FORMAT_R16_UNORM: format = R_16; break;
        //case DDS_FORMAT_R16G16_UNORM: format = RG_16_16; break;
        //case DDS_FORMAT_R16G16B16A16_UNORM: format = RGBA_16_16_16_16; break;
        case DDS_FORMAT_R16_FLOAT: format = R_16F; break;
        case DDS_FORMAT_R16G16_FLOAT: format = RG_16F_16F; break;
        case DDS_FORMAT_R16G16B16A16_FLOAT: format = RGBA_16F_16F_16F_16F; break;
        case DDS_FORMAT_R32_FLOAT: format = R_32F; break;
        case DDS_FORMAT_R32G32_FLOAT: format = RG_32F_32F; break;
        case DDS_FORMAT_R32G32B32_FLOAT: format = RGB_32F_32F_32F; break;
        case DDS_FORMAT_R32G32B32A32_FLOAT: format = RGBA_32F_32F_32F_32F; break;
        case DDS_FORMAT_R9G9B9E5_SHAREDEXP: format = RGBE_9_9_9_5; break;
        case DDS_FORMAT_R11G11B10_FLOAT: format = RGB_11F_11F_10F; break;
        //case DDS_FORMAT_R10G10B10A2_UNORM: format = RGBA_10_10_10_2; break;
        case DDS_FORMAT_BC1_UNORM: format = RGBA_DXT1; break;
        case DDS_FORMAT_BC2_UNORM: format = RGBA_DXT3; break;
        case DDS_FORMAT_BC3_UNORM: format = RGBA_DXT5; break;
        case DDS_FORMAT_BC4_UNORM: format = DXN1; break;
        case DDS_FORMAT_BC5_UNORM: format = DXN2; break;
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
        //case 0x24:
        //  this->format = FORMAT_RGBA_16_16_16_16;
        //  break;
        case 0x71:
            this->format = RGBA_16F_16F_16F_16F;
            break;
        case 0x74:
            this->format = RGBA_32F_32F_32F_32F;
            break;
        case MAKE_FOURCC('D', 'X', 'T', '1'):
            this->format = RGBA_DXT1;
            break;
        case MAKE_FOURCC('D', 'X', 'T', '3'):
            this->format = RGBA_DXT3;
            break;
        case MAKE_FOURCC('D', 'X', 'T', '5'):
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

    int bufSize = GetSize(0, numMipmaps);
    this->pic = (byte *)Mem_Alloc16(bufSize);
    this->alloced = true;

    if (isCube) {
        for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
            for (int mipLevel = 0; mipLevel < numMipmaps; mipLevel++) {
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
    header.flags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | (numMipmaps > 1 ? DDSD_MIPMAPCOUNT : 0) | (depth > 1 ? DDSD_DEPTH : 0);
    header.height = height;
    header.width = width;
    header.pitchOrLinearSize = 0;
    header.depth = depth > 1 ? depth : 0;
    header.mipMapCount = numMipmaps > 1 ? numMipmaps : 0;

    header.ddsPixelFormat.size = sizeof(header.ddsPixelFormat);

    header.ddsCaps.caps1 = DDSCAPS_TEXTURE | (numMipmaps > 1 ? DDSCAPS_MIPMAP | DDSCAPS_COMPLEX : 0) | (depth > 1 ? DDSCAPS_COMPLEX : 0);
    header.ddsCaps.caps2 = (depth > 1) ? DDSCAPS2_VOLUME : (numSlices == 6) ? DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES : 0;

    DdsFileHeaderDX10 dx10Header;
    memset(&dx10Header, 0, sizeof(dx10Header));
    dx10Header.resourceDimension = depth > 1 ? D3D10_RESOURCE_DIMENSION_TEXTURE3D : D3D10_RESOURCE_DIMENSION_TEXTURE2D;
    dx10Header.miscFlag = numSlices == 6 ? D3D10_RESOURCE_MISC_TEXTURECUBE : 0;
    dx10Header.arraySize = numSlices;
    
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
        dx10Header.dxgiFormat = DDS_FORMAT_R8_UNORM;
        break;
    case RG_8_8:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DDS_FORMAT_R8G8_UNORM;
        break;
    case R_16F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DDS_FORMAT_R16_FLOAT;
        break;
    case RG_16F_16F:
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DDS_FORMAT_R16G16_FLOAT;
        break;
    case RGBA_16F_16F_16F_16F:
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DDS_FORMAT_R16G16B16A16_FLOAT;
        break;
    case R_32F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DDS_FORMAT_R32_FLOAT;
        break;
    case RG_32F_32F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DDS_FORMAT_R32G32_FLOAT;
        break;
    case RGB_32F_32F_32F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DDS_FORMAT_R32G32B32_FLOAT;
        break;
    case RGBA_32F_32F_32F_32F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DDS_FORMAT_R32G32B32A32_FLOAT;
        break;
    case RGBE_9_9_9_5:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DDS_FORMAT_R9G9B9E5_SHAREDEXP;
        break;
    case RGB_11F_11F_10F:
        header.ddsPixelFormat.flags = DDSPF_FOURCC;
        header.ddsPixelFormat.fourCC = MAKE_FOURCC('D', 'X', '1', '0');
        dx10Header.dxgiFormat = DDS_FORMAT_R11G11B10_FLOAT;
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
