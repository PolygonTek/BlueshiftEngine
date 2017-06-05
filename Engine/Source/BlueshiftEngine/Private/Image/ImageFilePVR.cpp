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
#include "libpvrt/PVRTTexture.h"

BE_NAMESPACE_BEGIN

bool Image::LoadPVR2FromMemory(const char *name, const byte *data, size_t fileSize) {
    const byte *ptr = data;
    
    PVR_Texture_Header *header = (PVR_Texture_Header *)ptr;
    if (header->dwPVR != PVRTEX_IDENTIFIER) {
        BE_WARNLOG(L"bad PVR identifier %hs\n", name);
        return false;
    }
    
    ptr += sizeof(PVR_Texture_Header);
    
    uint32_t pixelType = header->dwpfFlags & PVRTEX_PIXELTYPE;
    switch (pixelType) {
        case OGL_RGBA_4444:
            this->format = ABGR_4_4_4_4;
            break;
        case OGL_RGBA_5551:
            this->format = ABGR_1_5_5_5;
            break;
        case OGL_RGBA_8888:
            this->format = RGBA_8_8_8_8;
            break;
        case OGL_RGB_565:
            this->format = BGR_5_6_5;
            break;
        case OGL_RGB_555:
            this->format = BGRX_5_5_5_1;
            break;
        case OGL_RGB_888:
            this->format = RGB_8_8_8;
            break;
        case OGL_I_8:
            this->format = L_8;
            break;
        case OGL_AI_88:
            this->format = LA_8_8;
            break;
        case OGL_BGRA_8888:
            this->format = BGRA_8_8_8_8;
            break;
        case OGL_A_8:
            this->format = A_8;
            break;
        case OGL_PVRTC2:
            this->format = RGBA_PVRTC_2BPPV1;
            break;
        case OGL_PVRTC4:
            this->format = RGBA_PVRTC_4BPPV1;
            break;
        default:
            BE_WARNLOG(L"invalid PVR pixel format %i in %hs\n", pixelType, name);
            return false;
    }
    
    this->width = header->dwWidth;
    this->height = header->dwHeight;
    this->depth = 1;
    this->numMipmaps = Max(1, (int)header->dwMipMapCount);
    this->numSlices = Max(1, (int)header->dwNumSurfs);
    
    this->pic = (byte *)Mem_Alloc16(header->dwTextureDataSize);
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
            this->format = RGB_PVRTC_2BPPV1;
            break;
        case ePVRTPF_PVRTCI_2bpp_RGBA:
            this->format = RGBA_PVRTC_2BPPV1;
            break;
        case ePVRTPF_PVRTCI_4bpp_RGB:
            this->format = RGB_PVRTC_4BPPV1;
            break;
        case ePVRTPF_PVRTCI_4bpp_RGBA:
            this->format = RGBA_PVRTC_4BPPV1;
            break;
        case ePVRTPF_PVRTCII_2bpp:
            this->format = RGBA_PVRTC_2BPPV2;
            break;
        case ePVRTPF_PVRTCII_4bpp:
            this->format = RGBA_PVRTC_4BPPV2;
            break;
        case ePVRTPF_ETC1:
            this->format = RGB_8_ETC1;
            break;
        case ePVRTPF_ETC2_RGB:
            this->format = RGB_8_ETC2;
            break;
        case ePVRTPF_ETC2_RGBA:
            this->format = RGBA_8_8_ETC2;
            break;
        case ePVRTPF_ETC2_RGB_A1:
            this->format = RGBA_8_1_ETC2;
            break;
        case ePVRTPF_EAC_R11:
            this->format = IsSignedChannelType(header->u32ChannelType) ? SignedR_11_EAC : R_11_EAC;
            break;
        case ePVRTPF_EAC_RG11:
            this->format = IsSignedChannelType(header->u32ChannelType) ? SignedRG_11_11_EAC : RG_11_11_EAC;
            break;
        default:
            BE_WARNLOG(L"Image::LoadPVRFromMemory: Unsupported pixel format %hs\n", name);
            break;
        }
    } else {
        switch (header->u64PixelFormat) {
        case PVRTGENPIXELID4('r', 'g', 'b', 'a', 8, 8, 8, 8):
            this->format = RGBA_8_8_8_8;
            break;
        case PVRTGENPIXELID4('b', 'g', 'r', 'a', 8, 8, 8, 8):
            this->format = BGRA_8_8_8_8;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'a', 4, 4, 4, 4):
            this->format = RGBA_4_4_4_4;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'a', 5, 5, 5, 1):
            this->format = RGBA_5_5_5_1;
            break;
        case PVRTGENPIXELID3('r', 'g', 'b', 5, 6, 5):
            this->format = RGB_5_6_5;
            break;
        case PVRTGENPIXELID3('r', 'g', 'b', 8, 8, 8):
            this->format = RGB_8_8_8;
            break;
        case PVRTGENPIXELID2('r', 'g', 8, 8):
            this->format = RG_8_8;
            break;
        case PVRTGENPIXELID1('r', 8):
            this->format = R_8;
            break;
        case PVRTGENPIXELID2('l', 'a', 8, 8):
            this->format = LA_8_8;
            break;
        case PVRTGENPIXELID1('l', 8):
            this->format = L_8;
            break;
        case PVRTGENPIXELID1('a', 8):
            this->format = A_8;
            break;
        case PVRTGENPIXELID3('r', 'g', 'b', 11, 11, 10):
            this->format = RGB_11F_11F_10F;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'a', 16, 16, 16, 16):
            this->format = RGBA_16F_16F_16F_16F;
            break;
        case PVRTGENPIXELID3('r', 'g', 'b', 16, 16, 16):
            this->format = RGB_16F_16F_16F;
            break;
        case PVRTGENPIXELID2('r', 'g', 16, 16):
            this->format = RG_16F_16F;
            break;
        case PVRTGENPIXELID1('r', 16):
            this->format = R_16F;
            break;
        case PVRTGENPIXELID2('l', 'a', 16, 16):
            this->format = LA_16F_16F;
            break;
        case PVRTGENPIXELID1('l', 16):
            this->format = L_16F;
            break;
        case PVRTGENPIXELID1('a', 16):
            this->format = A_16F;
            break;
        case PVRTGENPIXELID4('r', 'g', 'b', 'a', 32, 32, 32, 32):
            this->format = RGBA_32F_32F_32F_32F;
            break;
        case PVRTGENPIXELID3('r', 'g', 'b', 32, 32, 32):
            this->format = RGB_32F_32F_32F;
            break;
        case PVRTGENPIXELID2('r', 'g', 32, 32):
            this->format = RG_32F_32F;
            break;
        case PVRTGENPIXELID1('r', 32):
            this->format = R_32F;
            break;
        case PVRTGENPIXELID2('l', 'a', 32, 32):
            this->format = LA_32F_32F;
            break;
        case PVRTGENPIXELID1('l', 32):
            this->format = L_32F;
            break;
        case PVRTGENPIXELID1('a', 32):
            this->format = A_32F;
            break;
        default:
            BE_WARNLOG(L"Image::LoadPVRFromMemory: Unsupported pixel format %hs\n", name);
            break;
        }
    }
        
    ptr += header->u32MetaDataSize;
    
    this->width = header->u32Width;
    this->height = header->u32Height;
    this->depth = header->u32Depth;
    this->flags = header->u32ColourSpace == ePVRTCSpacesRGB ? LinearFlag : SRGBFlag;
    this->numMipmaps = Max(1, (int)header->u32MIPMapCount);
    this->numSlices = Max(1, (int)header->u32NumFaces);

    size_t dataSize = fileSize - (ptr - data);
    
    this->pic = (byte *)Mem_Alloc16(dataSize);
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
    const Image *srcImage = this;
    Image convertedImage;

    if (format != RGB_8_ETC1 && format != RGB_8_ETC2 && format != RGBA_8_8_ETC2 && format != RGBA_8_1_ETC2) {
        Image::Format convertFormat;
        int alphaBits;
        GetBits(nullptr, nullptr, nullptr, &alphaBits);

        if (alphaBits >= 8) {
            convertFormat = RGBA_8_8_ETC2;
        } else if (alphaBits == 1) {
            convertFormat = RGBA_8_1_ETC2;
        } else {
            convertFormat = RGB_8_ETC2;
        }

        if (convertFormat != format) {
            ConvertFormat(convertFormat, convertedImage);
            srcImage = &convertedImage;
        }
    }

    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"Image::WritePVR: file open error\n");
        return false;
    }

    PVRTextureHeaderV3 header;
    header.u32Version = PVRTEX3_IDENT;
    header.u32Flags = 0;
    header.u32ColourSpace = (flags & LinearFlag) ? ePVRTCSpacesRGB : ePVRTCSpacelRGB;
    header.u32ChannelType = ePVRTVarTypeUnsignedByteNorm;
    header.u32Height = height;
    header.u32Width = width;
    header.u32Depth = 1;
    header.u32NumSurfaces = 1;
    header.u32NumFaces = 1;
    header.u32MIPMapCount = numMipmaps;

    switch (srcImage->format) {
    case RGB_8_ETC1:
        header.u64PixelFormat = ePVRTPF_ETC1;
        break;
    case RGB_8_ETC2:
        header.u64PixelFormat = ePVRTPF_ETC2_RGB;
        break;
    case RGBA_8_8_ETC2:
        header.u64PixelFormat = ePVRTPF_ETC2_RGBA;
        break;
    case RGBA_8_1_ETC2:
        header.u64PixelFormat = ePVRTPF_ETC2_RGB_A1;
        break;
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
    
    fp->Write(srcImage->pic, srcImage->GetSize(0, numMipmaps));

    fileSystem.CloseFile(fp);

    return true;
}

BE_NAMESPACE_END
