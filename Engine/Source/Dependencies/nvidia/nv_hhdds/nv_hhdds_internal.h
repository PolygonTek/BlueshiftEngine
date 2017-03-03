//----------------------------------------------------------------------------------
// File:        jni/nv_hhdds/nv_hhdds_internal.h
// SDK Version: v10.14 
// Email:       tegradev@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2007-2012, NVIDIA CORPORATION.  All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA OR ITS SUPPLIERS
// BE  LIABLE  FOR  ANY  SPECIAL,  INCIDENTAL,  INDIRECT,  OR  CONSEQUENTIAL DAMAGES
// WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS OF BUSINESS PROFITS,
// BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
// ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
//
//----------------------------------------------------------------------------------

#ifndef __INCLUDED_NV_HHDDS_INTERNAL_H
#define __INCLUDED_NV_HHDDS_INTERNAL_H

#include <nv_global.h>

//================================================================================
// Various DDS file defines

#define DDSD_CAPS             0x00000001l
#define DDSD_HEIGHT           0x00000002l
#define DDSD_WIDTH            0x00000004l
#define DDSD_PIXELFORMAT      0x00001000l
#define DDS_ALPHAPIXELS       0x00000001l
#define DDS_FOURCC            0x00000004l
#define DDS_PITCH             0x00000008l
#define DDS_COMPLEX           0x00000008l
#define DDS_RGB               0x00000040l
#define DDS_TEXTURE           0x00001000l
#define DDS_MIPMAPCOUNT       0x00020000l
#define DDS_LINEARSIZE        0x00080000l
#define DDS_VOLUME            0x00200000l
#define DDS_MIPMAP            0x00400000l
#define DDS_DEPTH             0x00800000l

#define DDS_CUBEMAP           0x00000200L
#define DDS_CUBEMAP_POSITIVEX 0x00000400L
#define DDS_CUBEMAP_NEGATIVEX 0x00000800L
#define DDS_CUBEMAP_POSITIVEY 0x00001000L
#define DDS_CUBEMAP_NEGATIVEY 0x00002000L
#define DDS_CUBEMAP_POSITIVEZ 0x00004000L
#define DDS_CUBEMAP_NEGATIVEZ 0x00008000L

#define FOURCC_DXT1 0x31545844 //(MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT3 0x33545844 //(MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT5 0x35545844 //(MAKEFOURCC('D','X','T','5'))

#define DDS_MAGIC_FLIPPED     0x0F7166ED

//================================================================================
// DXT data-layout structure definitions.
typedef struct 
{
    NvU16 col0; // 16-bit 565 interpolant endpoints
    NvU16 col1;
    NvU8  row[4]; // 4x4 * 2bpp color-index == 4 bytes.
} DXTColBlock;

typedef struct 
{
    NvU16 row[4]; // 4x4 * 4bpp alpha == 8 bytes.  (pure 4-bit alpha values)
} DXT3AlphaBlock;

typedef struct 
{
    NvU8 alpha0; // 8-bit alpha interpolant endpoints
    NvU8 alpha1;
    NvU8 row[6]; // 4x4 * 3bpp alpha-index == 48bits == 6 bytes.
} DXT5AlphaBlock;

typedef struct
{
    NvU8 red;
    NvU8 green;
    NvU8 blue;
    NvU8 alpha;
} Color8888;


//================================================================================
// DDS file format structures.
typedef struct _DDS_PIXELFORMAT 
{
    NvU32 dwSize;
    NvU32 dwFlags;
    NvU32 dwFourCC;
    NvU32 dwRGBBitCount;
    NvU32 dwRBitMask;
    NvU32 dwGBitMask;
    NvU32 dwBBitMask;
    NvU32 dwABitMask;
} DDS_PIXELFORMAT;

typedef struct _DDS_HEADER 
{
    NvU32 dwSize;
    NvU32 dwFlags;
    NvU32 dwHeight;
    NvU32 dwWidth;
    NvU32 dwPitchOrLinearSize;
    NvU32 dwDepth;
    NvU32 dwMipMapCount;
    NvU32 dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    NvU32 dwCaps1;
    NvU32 dwCaps2;
    NvU32 dwReserved2[3];
} DDS_HEADER;
#endif /* __INCLUDED_NV_HHDDS_INTERNAL_H */
