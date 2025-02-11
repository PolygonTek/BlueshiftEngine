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
#include "libpvrt/PVRTTexture.h"

BE_NAMESPACE_BEGIN

bool Image::LoadPVR2FromMemory(const char *name, const byte *data, size_t fileSize) {
    const byte *ptr = data;
    
    PVR_Texture_Header *header = (PVR_Texture_Header *)ptr;
    if (header->dwPVR != PVRTEX_IDENTIFIER) {
        BE_WARNLOG("bad PVR identifier %s\n", name);
        return false;
    }
    
    ptr += sizeof(PVR_Texture_Header);
    
    uint32_t pixelType = header->dwpfFlags & PVRTEX_PIXELTYPE;
    switch (pixelType) {
        case OGL_RGBA_4444:
            this->format = Format::A4B4G4R4;
            break;
        case OGL_RGBA_5551:
            this->format = Format::A1B5G5R5;
            break;
        case OGL_RGBA_8888:
            this->format = Format::R8G8B8A8;
            break;
        case OGL_RGB_565:
            this->format = Format::B5G6R5;
            break;
        case OGL_RGB_555:
            this->format = Format::B5G5R5X1;
            break;
        case OGL_RGB_888:
            this->format = Format::R8G8B8;
            break;
        case OGL_I_8:
            this->format = Format::L8;
            break;
        case OGL_AI_88:
            this->format = Format::L8A8;
            break;
        case OGL_BGRA_8888:
            this->format = Format::B8G8R8A8;
            break;
        case OGL_A_8:
            this->format = Format::A8;
            break;
        case OGL_PVRTC2:
            this->format = Format::PVRTC12A;
            break;
        case OGL_PVRTC4:
            this->format = Format::PVRTC14A;
            break;
        default:
            BE_WARNLOG("invalid PVR pixel format %i in %s\n", pixelType, name);
            return false;
    }
    
    this->width = header->dwWidth;
    this->height = header->dwHeight;
    this->depth = 1;
    this->numMipLevels = Max(1, (int)header->dwMipMapCount);
    this->arraySize = Max(1, (int)header->dwNumSurfs);
    
    this->pic = (byte *)Mem_Alloc256(header->dwTextureDataSize);
    simdProcessor->Memcpy(this->pic, ptr, header->dwTextureDataSize);
    this->alloced = true;
    
    return true;
}

static bool IsSignedChannelType(uint32_t channelType) {
    if (channelType == ePVRTVarTypeSignedInteger || channelType == ePVRTVarTypeSignedIntegerNorm ||
        channelType == ePVRTVarTypeSignedShort || channelType == ePVRTVarTypeSignedShortNorm ||
        channelType == ePVRTVarTypeSignedByte || channelType == ePVRTVarTypeSignedByteNorm ||
        channelType == ePVRTVarTypeSignedFloat) {
        return true;
    }
    return false;
}

bool Image::LoadPVR3FromMemory(const char *name, const byte *data, size_t fileSize) {
    const byte *ptr = data;
    
    PVRTextureHeaderV3 *header = (PVRTextureHeaderV3 *)ptr;
    if (header->u32Version != PVRTEX3_IDENT) {
        return false;
    }
    
    ptr += sizeof(PVRTextureHeaderV3);
    
    uint64_t pixelFormatPartHigh = header->u64PixelFormat & PVRTEX_PFHIGHMASK;
    
    if (pixelFormatPartHigh == 0) {
        switch (header->u64PixelFormat) {
        case ePVRTPF_PVRTCI_2bpp_RGB:
            this->format = Format::PVRTC12;
            break;
        case ePVRTPF_PVRTCI_2bpp_RGBA:
            this->format = Format::PVRTC12A;
            break;
        case ePVRTPF_PVRTCI_4bpp_RGB:
            this->format = Format::PVRTC14;
            break;
        case ePVRTPF_PVRTCI_4bpp_RGBA:
            this->format = Format::PVRTC14A;
            break;
        case ePVRTPF_PVRTCII_2bpp:
            this->format = Format::PVRTC22A;
            break;
        case ePVRTPF_PVRTCII_4bpp:
            this->format = Format::PVRTC24A;
            break;
        case ePVRTPF_ETC1:
            this->format = Format::ETC1;
            break;
        case ePVRTPF_ETC2_RGB:
            this->format = Format::ETC2;
            break;
        case ePVRTPF_ETC2_RGBA:
            this->format = Format::ETC2A;
            break;
        case ePVRTPF_ETC2_RGB_A1:
            this->format = Format::ETC2A1;
            break;
        case ePVRTPF_EAC_R11:
            this->format = IsSignedChannelType(header->u32ChannelType) ? Format::EACR11_SNORM : Format::EACR11;
            break;
        case ePVRTPF_EAC_RG11:
            this->format = IsSignedChannelType(header->u32ChannelType) ? Format::EACRG11_SNORM : Format::EACRG11;
            break;
        case ePVRTPF_SharedExponentR9G9B9E5:
            this->format = Format::R9G9B9E5_FLOAT;
            break;
        default:
            BE_WARNLOG("Image::LoadPVR3FromMemory: Unsupported pixel format %s\n", name);
            break;
        }
    } else {
        switch (header->u64PixelFormat) {
        case PVRTGENPIXELID4('r', 'g', 'b', 'a', 8, 8, 8, 8):
            this->format = Format::R8G8B8A8;
            break;
        case PVRTGENPIXELID4('b', 'g', 'r', 'a', 8, 8, 8, 8):
            this->format = Format::B8G8R8A8;
            break;
        case PVRTGENPIXELID4('a', 'r', 'g', 'b', 8, 8, 8, 8):
            this->format = Format::A8R8G8B8;
            break;
        case PVRTGENPIXELID4('a', 'b', 'g', 'r', 8, 8, 8, 8):
            this->format = Format::A8B8G8R8;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'x', 8, 8, 8, 8):
            this->format = Format::R8G8B8X8;
            break;
        case PVRTGENPIXELID4('b', 'g', 'r', 'x', 8, 8, 8, 8):
            this->format = Format::B8G8R8X8;
            break;
        case PVRTGENPIXELID3('r', 'g', 'b', 8, 8, 8):
            this->format = Format::R8G8B8;
            break;
        case PVRTGENPIXELID2('r', 'g', 8, 8):
            this->format = Format::R8G8;
            break;
        case PVRTGENPIXELID1('r', 8):
            this->format = Format::R8;
            break;
        case PVRTGENPIXELID2('l', 'a', 8, 8):
            this->format = Format::L8A8;
            break;
        case PVRTGENPIXELID1('l', 8):
            this->format = Format::L8;
            break;
        case PVRTGENPIXELID1('a', 8):
            this->format = Format::A8;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'a', 4, 4, 4, 4):
            this->format = Format::R4G4B4A4;
            break;
        case PVRTGENPIXELID4('b', 'g', 'r', 'a', 4, 4, 4, 4):
            this->format = Format::B4G4R4A4;
            break;
        case PVRTGENPIXELID4('a', 'r', 'g', 'b', 4, 4, 4, 4):
            this->format = Format::A4R4G4B4;
            break;
        case PVRTGENPIXELID4('a', 'b', 'g', 'r', 4, 4, 4, 4):
            this->format = Format::A4B4G4R4;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'x', 4, 4, 4, 4):
            this->format = Format::R4G4B4X4;
            break;
        case PVRTGENPIXELID4('b', 'g', 'r', 'x', 4, 4, 4, 4):
            this->format = Format::B4G4R4X4;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'a', 5, 5, 5, 1):
            this->format = Format::R5G5B5A1;
            break;
        case PVRTGENPIXELID4('b', 'g', 'r', 'a', 5, 5, 5, 1):
            this->format = Format::B5G5R5A1;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'x', 5, 5, 5, 1):
            this->format = Format::R5G5B5X1;
            break;
        case PVRTGENPIXELID4('a', 'r', 'g', 'b', 1, 5, 5, 5):
            this->format = Format::A1R5G5B5;
            break;
        case PVRTGENPIXELID4('a', 'b', 'g', 'r', 1, 5, 5, 5):
            this->format = Format::A1B5G5R5;
            break;
        case PVRTGENPIXELID3('r', 'g', 'b', 5, 6, 5):
            this->format = Format::R5G6B5;
            break;
        case PVRTGENPIXELID3('b', 'g', 'r', 5, 6, 5):
            this->format = Format::B5G6R5;
            break;
        case PVRTGENPIXELID3('r', 'g', 'b', 11, 11, 10):
            this->format = Format::R11G11B10_FLOAT;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'a', 16, 16, 16, 16):
            this->format = Format::R16G16B16A16_FLOAT;
            break;
        case PVRTGENPIXELID3('r', 'g', 'b', 16, 16, 16):
            this->format = Format::R16G16B16_FLOAT;
            break;
        case PVRTGENPIXELID2('r', 'g', 16, 16):
            this->format = Format::R16G16_FLOAT;
            break;
        case PVRTGENPIXELID1('r', 16):
            this->format = Format::R16_FLOAT;
            break;
        case PVRTGENPIXELID2('l', 'a', 16, 16):
            this->format = Format::L16A16_FLOAT;
            break;
        case PVRTGENPIXELID1('l', 16):
            this->format = Format::L16_FLOAT;
            break;
        case PVRTGENPIXELID1('a', 16):
            this->format = Format::A16_FLOAT;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'a', 32, 32, 32, 32):
            this->format = Format::R32G32B32A32_FLOAT;
            break;
        case PVRTGENPIXELID3('r', 'g', 'b', 32, 32, 32):
            this->format = Format::R32G32B32_FLOAT;
            break;
        case PVRTGENPIXELID2('r', 'g', 32, 32):
            this->format = Format::R32G32_FLOAT;
            break;
        case PVRTGENPIXELID1('r', 32):
            this->format = Format::R32_FLOAT;
            break;
        case PVRTGENPIXELID2('l', 'a', 32, 32):
            this->format = Format::L32A32_FLOAT;
            break;
        case PVRTGENPIXELID1('l', 32):
            this->format = Format::L32_FLOAT;
            break;
        case PVRTGENPIXELID1('a', 32):
            this->format = Format::A32_FLOAT;
            break;
        default:
            BE_WARNLOG("Image::LoadPVR3FromMemory: Unsupported pixel format %s\n", name);
            break;
        }
    }
        
    ptr += header->u32MetaDataSize;
    
    this->width = header->u32Width;
    this->height = header->u32Height;
    this->depth = header->u32Depth;
    this->gammaSpace = header->u32ColourSpace == ePVRTCSpacelRGB ? GammaSpace::Linear : GammaSpace::sRGB;
    this->numMipLevels = Max(1, (int)header->u32MIPMapCount);
    this->arraySize = header->u32NumFaces * Max(1, (int)header->u32NumSurfaces);
    this->flags = header->u32NumFaces == 6 ? Flag::CubeMap : Flag::None;
    
    size_t dataSize = fileSize - (ptr - data);
    
    this->pic = (byte *)Mem_Alloc256(dataSize);
    simdProcessor->Memcpy(this->pic, ptr, (int)dataSize);
    this->alloced = true;
    
    return false;
}

bool Image::LoadPVRFromMemory(const char *name, const byte *data, size_t size) {
    bool ret;
    uint32_t v3magic = *reinterpret_cast<const uint32_t *>(data);
    
    if (v3magic == PVRTEX3_IDENT) {
        ret = LoadPVR3FromMemory(name, data, size);
    } else {
        ret = LoadPVR2FromMemory(name, data, size);
    }

    return ret;
}

bool Image::WritePVR(const char *filename) const {
    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Image::WritePVR: file open error\n");
        return false;
    }

    PVRTextureHeaderV3 header;
    header.u32Version = PVRTEX3_IDENT;
    header.u32Flags = 0;
    header.u32Height = height;
    header.u32Width = width;
    header.u32Depth = 1;
    header.u32NumSurfaces = 1;
    header.u32NumFaces = HasFlag(flags, Flag::CubeMap) ?  6 : 1;
    header.u32MIPMapCount = numMipLevels;

    switch (format) {
    case Format::PVRTC12:
        header.u64PixelFormat = ePVRTPF_PVRTCI_2bpp_RGB;
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::PVRTC12A:
        header.u64PixelFormat = ePVRTPF_PVRTCI_2bpp_RGBA;
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::PVRTC14:
        header.u64PixelFormat = ePVRTPF_PVRTCI_4bpp_RGB;
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::PVRTC14A:
        header.u64PixelFormat = ePVRTPF_PVRTCI_4bpp_RGBA;
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::PVRTC22A:
        header.u64PixelFormat = ePVRTPF_PVRTCII_2bpp;
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::PVRTC24A:
        header.u64PixelFormat = ePVRTPF_PVRTCII_4bpp;
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::ETC1:
        header.u64PixelFormat = ePVRTPF_ETC1;
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::ETC2:
        header.u64PixelFormat = ePVRTPF_ETC2_RGB;
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::ETC2A1:
        header.u64PixelFormat = ePVRTPF_ETC2_RGB_A1;
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::ETC2A:
        header.u64PixelFormat = ePVRTPF_ETC2_RGBA;
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::EACRG11:
        header.u64PixelFormat = ePVRTPF_EAC_RG11;
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::EACRG11_SNORM:
        header.u64PixelFormat = ePVRTPF_EAC_RG11;
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedShortNorm;
        break;
    case Format::R8G8B8A8:
        header.u64PixelFormat = PVRTGENPIXELID4('r', 'g', 'b', 'a', 8, 8, 8, 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::B8G8R8A8:
        header.u64PixelFormat = PVRTGENPIXELID4('b', 'g', 'r', 'a', 8, 8, 8, 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::A8R8G8B8:
        header.u64PixelFormat = PVRTGENPIXELID4('a', 'r', 'g', 'b', 8, 8, 8, 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::A8B8G8R8:
        header.u64PixelFormat = PVRTGENPIXELID4('a', 'b', 'g', 'r', 8, 8, 8, 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::R8G8B8X8:
        header.u64PixelFormat = PVRTGENPIXELID4('r', 'g', 'b', 'x', 8, 8, 8, 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::B8G8R8X8:
        header.u64PixelFormat = PVRTGENPIXELID4('b', 'g', 'r', 'x', 8, 8, 8, 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::R8G8B8:
        header.u64PixelFormat = PVRTGENPIXELID3('r', 'g', 'b', 8, 8, 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::R8G8:
        header.u64PixelFormat = PVRTGENPIXELID2('r', 'g', 8, 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::R8:
        header.u64PixelFormat = PVRTGENPIXELID1('r', 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::L8A8:
        header.u64PixelFormat = PVRTGENPIXELID2('l', 'a', 8, 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::L8:
        header.u64PixelFormat = PVRTGENPIXELID1('l', 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::A8:
        header.u64PixelFormat = PVRTGENPIXELID1('a', 8);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
        break;
    case Format::R4G4B4A4:
        header.u64PixelFormat = PVRTGENPIXELID4('r', 'g', 'b', 'a', 4, 4, 4, 4);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::B4G4R4A4:
        header.u64PixelFormat = PVRTGENPIXELID4('b', 'g', 'r', 'a', 4, 4, 4, 4);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::A4R4G4B4:
        header.u64PixelFormat = PVRTGENPIXELID4('a', 'r', 'g', 'b', 4, 4, 4, 4);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::A4B4G4R4:
        header.u64PixelFormat = PVRTGENPIXELID4('a', 'b', 'g', 'r', 4, 4, 4, 4);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::R4G4B4X4:
        header.u64PixelFormat = PVRTGENPIXELID4('b', 'g', 'r', 'x', 4, 4, 4, 4);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::B4G4R4X4:
        header.u64PixelFormat = PVRTGENPIXELID4('b', 'g', 'r', 'x', 4, 4, 4, 4);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::R5G5B5A1:
        header.u64PixelFormat = PVRTGENPIXELID4('r', 'g', 'b', 'a', 5, 5, 5, 1);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::B5G5R5A1:
        header.u64PixelFormat = PVRTGENPIXELID4('b', 'g', 'r', 'a', 5, 5, 5, 1);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;    
    case Format::R5G5B5X1:
        header.u64PixelFormat = PVRTGENPIXELID4('r', 'g', 'b', 'x', 5, 5, 5, 1);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::A1R5G5B5:
        header.u64PixelFormat = PVRTGENPIXELID4('a', 'r', 'g', 'b', 1, 5, 5, 5);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::A1B5G5R5:
        header.u64PixelFormat = PVRTGENPIXELID4('a', 'b', 'g', 'r', 1, 5, 5, 5);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::R5G6B5:
        header.u64PixelFormat = PVRTGENPIXELID3('r', 'g', 'b', 5, 6, 5);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::B5G6R5:
        header.u64PixelFormat = PVRTGENPIXELID3('b', 'g', 'r', 5, 6, 5);
        header.u32ColourSpace = ePVRTCSpacesRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedShortNorm;
        break;
    case Format::R11G11B10_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID3('r', 'g', 'b', 11, 11, 10);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedFloat;
        break;
    case Format::R9G9B9E5_FLOAT:
        header.u64PixelFormat = ePVRTPF_SharedExponentR9G9B9E5;
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeUnsignedFloat;
        break;
    case Format::R16G16B16A16_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID4('r', 'g', 'b', 'a', 16, 16, 16, 16);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::R16G16B16_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID3('r', 'g', 'b', 16, 16, 16);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::R16G16_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID2('r', 'g', 16, 16);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::R16_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID1('r', 16);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::L16A16_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID2('l', 'a', 16, 16);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::L16_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID1('l', 16);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::A16_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID1('a', 16);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::R32G32B32A32_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID4('r', 'g', 'b', 'a', 32, 32, 32, 32);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::R32G32B32_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID3('r', 'g', 'b', 32, 32, 32);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::R32G32_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID2('r', 'g', 32, 32);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::R32_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID1('r', 32);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::L32A32_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID4('r', 'g', 'b', 'a', 32, 32, 32, 32);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::L32_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID2('l', 'a', 32, 32);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    case Format::A32_FLOAT:
        header.u64PixelFormat = PVRTGENPIXELID1('l', 32);
        header.u32ColourSpace = ePVRTCSpacelRGB;
        header.u32ChannelType = ePVRTVarTypeSignedFloat;
        break;
    default:
        fileSystem.CloseFile(fp);
        BE_WARNLOG("Image::WritePVR: invalid format '%s' for PVR\n", Image::FormatName(format));
        return false;
    }

#if 0
    MetaDataBlock metaDataBlock;
    metaDataBlock.DevFOURCC = PVRTEX3_IDENT;
    metaDataBlock.u32Key = ePVRTMetaDataTextureOrientation;
    metaDataBlock.u32DataSize = 3;
    metaDataBlock.Data = new PVRTuint8[metaDataBlock.u32DataSize];
    metaDataBlock.Data[0] = ePVRTOrientRight;
    metaDataBlock.Data[1] = ePVRTOrientUp;
    metaDataBlock.Data[2] = ePVRTOrientIn;

    header.u32MetaDataSize = metaDataBlock.SizeOfBlock();

    fp->Write(&header, sizeof(header));

    fp->Write(&metaDataBlock.DevFOURCC, sizeof(metaDataBlock.DevFOURCC));
    fp->Write(&metaDataBlock.u32Key, sizeof(metaDataBlock.u32Key));
    fp->Write(&metaDataBlock.u32DataSize, sizeof(metaDataBlock.u32DataSize));

    for (int i = 0; i < metaDataBlock.u32DataSize; i++) {
        fp->Write(&metaDataBlock.Data[i], sizeof(metaDataBlock.Data[0]));
    }

#else
    header.u32MetaDataSize = 0;

    fp->Write(&header, sizeof(header));
#endif
    
    fp->Write(pic, SizeInBytes(0, numMipLevels));

    fileSystem.CloseFile(fp);

    return true;
}

BE_NAMESPACE_END
