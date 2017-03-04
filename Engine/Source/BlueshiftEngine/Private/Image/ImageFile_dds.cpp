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
#include "Core/Str.h"
#include "Core/WStr.h"
#include "Core/Heap.h"
#include "SIMD/Simd.h"
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

// dwCaps
#define DDSCAPS_COMPLEX             0x8
#define DDSCAPS_TEXTURE             0x1000
#define DDSCAPS_MIPMAP              0x400000

// dwCaps2
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

struct DdsFileHeader {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    struct {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwFourCC;
        uint32_t dwRGBBitCount;
        uint32_t dwRBitMask;
        uint32_t dwGBitMask;
        uint32_t dwBBitMask;
        uint32_t dwABitMask;
    } ddsPixelFormat;    
    struct {
        uint32_t dwCaps1;
        uint32_t dwCaps2;
        uint32_t dwCaps3;
        uint32_t dwCaps4;
    } ddsCaps;
    uint32_t dwReserved2;
};

struct DdsFileHeaderDX10 {
    uint32_t dwDxgiFormat;
    uint32_t dwResourceDimension;
    uint32_t dwMiscFlag;
    uint32_t dwArraySize;
    uint32_t dwReserved;
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
    
    if (header->dwSize != sizeof(DdsFileHeader) || (header->dwFlags & essentialFlags) != essentialFlags) {
        BE_WARNLOG(L"Image::LoadDDSFromMemory: bad DDS format %hs\n", name);
        return false;
    }

    if (header->ddsPixelFormat.dwFourCC == MAKE_FOURCC('D', 'X', '1', '0')) {
        DdsFileHeaderDX10 *dx10Header = (DdsFileHeaderDX10 *)ptr;
        ptr += sizeof(DdsFileHeaderDX10);

        switch (dx10Header->dwDxgiFormat){
        case 61: format = R_8; break;
        case 49: format = RG_8_8; break;
        case 28: format = RGBA_8_8_8_8; break;
        //case 56: format = R_16; break;
        //case 35: format = RG_16_16; break;
        //case 11: format = RGBA_16_16_16_16; break;
        case 54: format = R_16F; break;
        case 34: format = RG_16F_16F; break;
        case 10: format = RGBA_16F_16F_16F_16F; break;
        case 41: format = R_32F; break;
        case 16: format = RG_32F_32F; break;
        case 6:  format = RGB_32F_32F_32F; break;
        case 2:  format = RGBA_32F_32F_32F_32F; break;
        case 67: format = RGBE_9_9_9_5; break;
        case 26: format = RGB_11F_11F_10F; break;
        //case 24: format = RGBA_10_10_10_2; break;
        case 71: format = RGBA_DXT1; break;
        case 74: format = RGBA_DXT3; break;
        case 77: format = RGBA_DXT5; break;
        case 80: format = DXN1; break;
        case 83: format = DXN2; break;
        default:
            BE_WARNLOG(L"Image::LoadDDSFromMemory: Unsupported pixel format %hs\n", name);
            return false;
        }
    } else {
        switch (header->ddsPixelFormat.dwFourCC) {
        case 0:
            switch (header->ddsPixelFormat.dwRGBBitCount >> 3) {
            case 4:
                if (header->ddsPixelFormat.dwABitMask == 0x000000ff) {
                    if (header->ddsPixelFormat.dwRBitMask == 0xff000000) {
                        this->format = ABGR_8_8_8_8;
                    } else {
                        this->format = ARGB_8_8_8_8;
                    }
                } else if (header->ddsPixelFormat.dwABitMask == 0xff00000) {
                    if (header->ddsPixelFormat.dwRBitMask == 0x00ff0000) {
                        this->format = BGRA_8_8_8_8;
                    } else {
                        this->format = RGBA_8_8_8_8;
                    }
                } else {
                    if (header->ddsPixelFormat.dwRBitMask == 0x00ff0000) {
                        this->format = BGRX_8_8_8_8;
                    } else {
                        this->format = RGBX_8_8_8_8;
                    }
                }
                break;
            case 3:
                if (header->ddsPixelFormat.dwRBitMask == 0x00ff0000) {
                    this->format = BGR_8_8_8;
                } else {
                    this->format = RGB_8_8_8;
                }
                break;
            case 2:
                if (header->ddsPixelFormat.dwABitMask == 0x0000000f) {
                    if (header->ddsPixelFormat.dwRBitMask == 0x0000f000) {
                        this->format = ABGR_4_4_4_4;
                    } else {
                        this->format = ARGB_4_4_4_4;
                    }
                } else if (header->ddsPixelFormat.dwABitMask == 0x0000f000) {
                    if (header->ddsPixelFormat.dwRBitMask == 0x00000f00) {
                        this->format = BGRA_4_4_4_4;
                    } else {
                        this->format = RGBA_4_4_4_4;
                    }
                } else if (header->ddsPixelFormat.dwABitMask == 0x00008000) {
                    if (header->ddsPixelFormat.dwRBitMask == 0x00007c00) {
                        this->format = BGRA_5_5_5_1;
                    } else {
                        this->format = RGBA_5_5_5_1;
                    }
                } else if (header->ddsPixelFormat.dwABitMask == 0x00000001) {
                    if (header->ddsPixelFormat.dwRBitMask == 0x0000f800) {
                        this->format = ABGR_1_5_5_5;
                    } else {
                        this->format = ARGB_1_5_5_5;
                    }
                } else if (header->ddsPixelFormat.dwABitMask == 0x0000ff00) {
                    this->format = LA_8_8;
                } else {
                    if (header->ddsPixelFormat.dwGBitMask == 0x000000f0) {
                        if (header->ddsPixelFormat.dwRBitMask == 0x00000f00) {
                            this->format = BGRX_4_4_4_4;
                        } else {
                            this->format = RGBX_4_4_4_4;
                        }
                    } else if (header->ddsPixelFormat.dwGBitMask == 0x000003e0) {
                        if (header->ddsPixelFormat.dwRBitMask == 0x00007c00) {
                            this->format = BGRX_5_5_5_1;
                        } else {
                            this->format = RGBX_5_5_5_1;
                        }
                    } else if (header->ddsPixelFormat.dwGBitMask == 0x000007e0) {
                        if (header->ddsPixelFormat.dwRBitMask == 0x0000f800) {
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
                if (header->ddsPixelFormat.dwABitMask == 0x000000ff) {
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
            //	case 0x24:
            //		this->format = FORMAT_RGBA_16_16_16_16;
            //		break;
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
            if (header->ddsPixelFormat.dwRGBBitCount == MAKE_FOURCC('x', 'G', 'B', 'R')) {
                this->format = XGBR_DXT5;
            } else {
                this->format = RGBA_DXT5;
            }
            break;
        case MAKE_FOURCC('R', 'X', 'G', 'B'):	// doom3 RXGB
            this->format = XGBR_DXT5;
            break;
        case MAKE_FOURCC('A', 'T', 'I', '1'):
            this->format = DXN1;
            break;
        case MAKE_FOURCC('A', 'T', 'I', '2'):	// ATI 3DTc
            this->format = DXN2;
            break;
        default:
            BE_WARNLOG(L"Image::LoadDDSFromMemory: Unsupported pixel format %hs\n", name);
            return false;
        }
    }

    bool isCube = header->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP ? true : false;

    this->width = header->dwWidth;
    this->height = header->dwHeight;
    this->depth = Max((int)header->dwDepth, 1);
    this->numMipmaps = Max((int)header->dwMipMapCount, 1);
    this->numSlices = isCube ? 6 : 1;

    int bufSize = GetSize(0, numMipmaps);
    this->pic = (byte *)Mem_Alloc16(bufSize);
    this->allocated = true;

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
    const Image *srcImage = this;
    Image convertedImage;

    if (format != RGBA_DXT1 && format != RGBA_DXT3 && format != RGBA_DXT5 && format != DXN1 && format != DXN2) {
        Image::Format convertFormat;
        if (GetFlags() & NormalMapFlag) {
            convertFormat = DXN2;
        } else {
            int alphaBits;
            GetBits(nullptr, nullptr, nullptr, &alphaBits);

            if (alphaBits <= 1) {
                convertFormat = RGBA_DXT1;
            } else {
                convertFormat = RGBA_DXT5;
            }
        }

        if (convertFormat != format) {
            ConvertFormat(convertFormat, convertedImage);
            srcImage = &convertedImage;
        }
    }

    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Image::WriteDDS: file open error\n");
        return false;
    }

    fp->WriteUInt32(DDS_MAGIC);

    DdsFileHeader header;
    memset(&header, 0, sizeof(header));
    header.dwSize = sizeof(header);
    header.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | (numMipmaps > 1 ? DDSD_MIPMAPCOUNT : 0) | (depth > 1 ? DDSD_DEPTH : 0);
    header.dwHeight = height;
    header.dwWidth = width;
    header.dwPitchOrLinearSize = 0;
    header.dwDepth = depth > 1 ? depth : 0;
    header.dwMipMapCount = numMipmaps > 1 ? numMipmaps : 0;

    header.ddsPixelFormat.dwSize = sizeof(header.ddsPixelFormat);
    header.ddsPixelFormat.dwFlags = DDSPF_RGB;
    header.ddsCaps.dwCaps1 = DDSCAPS_TEXTURE | (numMipmaps > 1 ? DDSCAPS_MIPMAP | DDSCAPS_COMPLEX : 0) | (depth > 1 ? DDSCAPS_COMPLEX : 0);
    header.ddsCaps.dwCaps2 = (depth > 1) ? DDSCAPS2_VOLUME : (numSlices == 6) ? DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES : 0;

    header.ddsPixelFormat.dwFlags = DDSPF_FOURCC;

    switch (srcImage->format) {
    case RGBA_DXT1:
        header.ddsPixelFormat.dwFourCC = MAKE_FOURCC('D', 'X', 'T', '1');
        break;
    case RGBA_DXT3:
        header.ddsPixelFormat.dwFourCC = MAKE_FOURCC('D', 'X', 'T', '3');
        break;
    case RGBA_DXT5:
        header.ddsPixelFormat.dwFourCC = MAKE_FOURCC('D', 'X', 'T', '5');
        break;
    case DXN1:
        header.ddsPixelFormat.dwFourCC = MAKE_FOURCC('A', 'T', 'I', '1');
        break;
    case DXN2:
        header.ddsPixelFormat.dwFourCC = MAKE_FOURCC('A', 'T', 'I', '2'); // ATI 3DTc
        break;
    default:
        break;
    }

    fp->Write(&header, sizeof(header));

    fp->Write(srcImage->pic, srcImage->GetSize(0, numMipmaps));

    fileSystem.CloseFile(fp);

    return true;
}

BE_NAMESPACE_END
