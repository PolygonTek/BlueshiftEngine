//----------------------------------------------------------------------------------
// File:        jni/nv_hhdds/nv_hhdds.cpp
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


#include <nv_global.h>

#include "nv_hhdds.h"
#include <nv_file/nv_file.h>
#include <nv_log/nv_log.h>

// to clean up this source file, moved lots of internal defs to a header...
#include "nv_hhdds_internal.h"

#include "GLES3/gl3.h"
#include "GLES3/gl3ext.h"

#include <stdlib.h>

#ifndef ANDROID
#include <memory.h>
#endif

//================================================================================
// helper macros.
static inline void NvSwapChar(NvU8& a, NvU8& b)
{
    NvU8 tmp;
    tmp = a;
    a = b;
    b = tmp;
}

static inline void NvSwapShort(NvU16& a, NvU16& b)
{
    NvU16 tmp;
    tmp = a;
    a = b;
    b = tmp;
}

//================================================================================
// ========== THE MAIN DDS-HANDLING CODE ==========
//================================================================================


//================================================================================
//================================================================================
static NvS32 size_image(NvS32 width, NvS32 height, const NVHHDDSImage* image)
{
    if (image->compressed) 
    {
        return ((width+3)/4)*((height+3)/4) 
            * (image->format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16);   
    } 
    else
    {
        return width * height * image->bytesPerPixel;
    }
}


//================================================================================
//================================================================================
static NvS32 total_image_data_size(NVHHDDSImage* image)
{
    NvS32 i, j, index = 0, size=0, w, h;
    NvS32 cubeCount = image->cubemap ? 6 : 1;

    for (j = 0; j < cubeCount; j++)
    {
        w = image->width;
        h = image->height;

        for (i=0; i<image->numMipmaps; i++) // account for base plus each mip
        {
            image->size[index] = size_image(w, h, image);
            image->mipwidth[index] = w;
            image->mipheight[index] = h;
            size += image->size[index];
            if(w != 1)
            {
                w >>= 1;
            }
            if(h != 1)
            {
                h >>= 1;
            }
           
            index++;
        }
    }

    return(size);
}


//================================================================================
//================================================================================
static void flip_blocks_dxtc1(DXTColBlock *line, NvS32 numBlocks)
{
    DXTColBlock *curblock = line;
    NvS32 i;

    for (i = 0; i < numBlocks; i++) 
    {
        NvSwapChar(curblock->row[0], curblock->row[3]);
        NvSwapChar(curblock->row[1], curblock->row[2]);
        curblock++;
    }
}

//================================================================================
//================================================================================
static void flip_blocks_dxtc3(DXTColBlock *line, NvS32 numBlocks)
{
    DXTColBlock *curblock = line;
    DXT3AlphaBlock *alphablock;
    NvS32 i;

    for (i = 0; i < numBlocks; i++) 
    {
        alphablock = (DXT3AlphaBlock*)curblock;

        NvSwapShort(alphablock->row[0], alphablock->row[3]);
        NvSwapShort(alphablock->row[1], alphablock->row[2]);
        curblock++;

        NvSwapChar(curblock->row[0], curblock->row[3]);
        NvSwapChar(curblock->row[1], curblock->row[2]);
        curblock++;
    }
}

//================================================================================
//================================================================================
static void flip_dxt5_alpha(DXT5AlphaBlock *block)
{
    NvS8 gBits[4][4];
    
    const NvU32 mask = 0x00000007;          // bits = 00 00 01 11
    NvU32 bits = 0;
    memcpy(&bits, &block->row[0], sizeof(NvS8) * 3);

    gBits[0][0] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[0][1] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[0][2] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[0][3] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[1][0] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[1][1] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[1][2] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[1][3] = (NvS8)(bits & mask);

    bits = 0;
    memcpy(&bits, &block->row[3], sizeof(NvS8) * 3);

    gBits[2][0] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[2][1] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[2][2] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[2][3] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[3][0] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[3][1] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[3][2] = (NvS8)(bits & mask);
    bits >>= 3;
    gBits[3][3] = (NvS8)(bits & mask);

    bits = (gBits[3][0] << 0) | (gBits[3][1] << 3) | (gBits[3][2] << 6) |
        (gBits[3][3] << 9) | (gBits[2][0] << 12) | (gBits[2][1] << 15) |
        (gBits[2][2] << 18) | (gBits[2][3] << 21);
    memcpy(&block->row[0], &bits, 3);

    bits = (gBits[1][0] << 0) | (gBits[1][1] << 3) | (gBits[1][2] << 6) |
        (gBits[1][3] << 9) | (gBits[0][0] << 12) | (gBits[0][1] << 15) |
        (gBits[0][2] << 18) | (gBits[0][3] << 21);
    memcpy(&block->row[3], &bits, 3);
}


//================================================================================
//================================================================================
static void flip_blocks_dxtc5(DXTColBlock *line, NvS32 numBlocks)
{
    DXTColBlock *curblock = line;
    DXT5AlphaBlock *alphablock;
    NvS32 i;

    for (i = 0; i < numBlocks; i++) 
    {
        alphablock = (DXT5AlphaBlock*)curblock;
        
        flip_dxt5_alpha(alphablock);
        curblock++;

        NvSwapChar(curblock->row[0], curblock->row[3]);
        NvSwapChar(curblock->row[1], curblock->row[2]);
        curblock++;
    }
}

//================================================================================
//================================================================================
static int flip_data_vertical(char *image, NvS32 width, NvS32 height, 
                               NVHHDDSImage* info)
{
    if (info->compressed) 
    {
        NvS32 linesize, j;
        DXTColBlock *top;
        DXTColBlock *bottom;
        char *tmp;
        void (*flipblocks)(DXTColBlock*, NvS32) = NV_NULL;
        NvS32 xblocks = width / 4;
        NvS32 yblocks = height / 4;
        NvS32 blocksize;

        switch (info->format) 
        {
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: 
                blocksize = 8;
                flipblocks = &flip_blocks_dxtc1; 
                break;
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: 
                blocksize = 16;
                flipblocks = &flip_blocks_dxtc3; 
                break;
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: 
                blocksize = 16;
                flipblocks = &flip_blocks_dxtc5; 
                break;
            default:
                return NvError_NotSupported;
        }

        linesize = xblocks * blocksize;
        tmp = new char[linesize];
        if (!tmp)
            return NvError_InsufficientMemory;

        for (j = 0; j < (yblocks >> 1); j++) 
        {
            top    = (DXTColBlock*) (void*) (image + j * linesize);
            bottom = (DXTColBlock*) (void*) (image + (((yblocks-j)-1) * linesize));

            (*flipblocks)(top, xblocks);
            (*flipblocks)(bottom, xblocks);

            memcpy(tmp,    bottom, linesize);
            memcpy(bottom, top,    linesize);
            memcpy(top,    tmp,    linesize);
        }

        // Catch the middle row of blocks if there is one
        // The loop above will skip the middle row
        if (yblocks & 0x01) 
        {
            DXTColBlock *middle 
                = (DXTColBlock*) (void*) (image + (yblocks >> 1) * linesize);
            (*flipblocks)(middle, xblocks);
        }

        delete[] tmp;
    } 
    else
    {
        NvS32 linesize = width * info->bytesPerPixel;
        NvS32 j;
        NvS8 *top;
        NvS8 *bottom;
        char *tmp;

        // much simpler - just compute the line length and swap each row
        tmp = new char[linesize];
        if (!tmp)
            return NvError_InsufficientMemory;

        for (j = 0; j < (height >> 1); j++) 
        {
            top    = (NvS8*)(image + j * linesize);
            bottom = (NvS8*)(image + (((height-j)-1) * linesize));

            memcpy(tmp,    bottom, linesize);
            memcpy(bottom, top,    linesize);
            memcpy(top,    tmp,    linesize);
        }

        delete[] tmp;
    }
    return NvError_Success;
}    


//================================================================================
//================================================================================
NVHHDDSImage *NVHHDDSLoad(const char* filename, NvS32 flipVertical)
{
    DDS_HEADER ddsh;
    char filecode[4];
    NvFile *fp;
    NVHHDDSImage *image = NV_NULL;
    NvS32 i;
    NvBool needsBGRASwap = NV_FALSE;
    NvBool isAllreadyFlipped = NV_FALSE;

    NVLogDebug(__FUNCTION__, "NVHHDDSLoad file: %s", filename);

    fp = NvFOpen(filename);
    if (fp == NV_NULL) 
    {
        NVLogWarn(__FUNCTION__, "Could not open file: %s", filename);
        return NV_NULL;
    }

    // read in file marker, make sure its a DDS file
    NvFRead(filecode, 1, 4, fp);
    if (memcmp(filecode, "DDS ", 4) != 0)
    {
        NVLogError(__FUNCTION__, "!> No DDS marker in file header: %s", filename);
        NvFClose(fp);
        return NV_NULL;
    }

    // allocate our image base structure
    image = NVHHDDSAlloc();
    if (!image)
    {
        NVLogError(__FUNCTION__, "Failed to allocate DDS image memory block for %s", filename);
        NvFClose(fp);
        return NV_NULL;
    }

    // read in DDS header
    NvFRead(&ddsh, sizeof(ddsh), 1, fp);

    // check if image is a cubempap
    if (ddsh.dwCaps2 & DDS_CUBEMAP)
    {
        const NvS32 allFaces = 
            DDS_CUBEMAP_POSITIVEX |
            DDS_CUBEMAP_POSITIVEY |
            DDS_CUBEMAP_POSITIVEZ |
            DDS_CUBEMAP_NEGATIVEX |
            DDS_CUBEMAP_NEGATIVEY |
            DDS_CUBEMAP_NEGATIVEZ;

        if ((ddsh.dwCaps2 & allFaces) != allFaces)
        {
            NVLogError(__FUNCTION__, "Attempt to load cubemap that doesn't contain all faces (unsupported..) from: %s", filename);
            NvFClose(fp);
            NVHHDDSFree(image);
            return NV_NULL;
        }

        image->cubemap = 1;
        isAllreadyFlipped=NV_TRUE;
    }
    else
    {
        image->cubemap = 0;
    }

    // check if image is a volume texture
    if ((ddsh.dwCaps2 & DDS_VOLUME) && (ddsh.dwDepth > 0))
    {
        NVLogError(__FUNCTION__, "Attempt to load a volume image (currently not supported..) from: %s", filename);
        NvFClose(fp);
        NVHHDDSFree(image);
        return NV_NULL;
    }

    // figure out what the image format is
    if (ddsh.ddspf.dwFlags & DDS_FOURCC) 
    {
        switch(ddsh.ddspf.dwFourCC) 
        {
            case FOURCC_DXT1:
                image->format     = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                image->components = 3;
                image->compressed = 1;
                image->alpha = 0; // Ugh - for backwards compatibility
                break;
            case FOURCC_DXT3:
                image->format     = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                image->components = 4;
                image->compressed = 1;
                image->alpha = 1;
                break;
            case FOURCC_DXT5:
                image->format     = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                image->components = 4;
                image->compressed = 1;
                image->alpha = 1;
                break;
            default:
                NVLogError(__FUNCTION__, "Unsupported FOURCC code = [0x%x], from file: %s", ddsh.ddspf.dwFourCC, filename);
                NvFClose(fp);
                NVHHDDSFree(image);
                return NV_NULL;
        }
    }
    else
    {
        // Check for a supported pixel format

        if ((ddsh.ddspf.dwRGBBitCount == 32) &&
            (ddsh.ddspf.dwRBitMask == 0x000000FF) &&
            (ddsh.ddspf.dwGBitMask == 0x0000FF00) &&
            (ddsh.ddspf.dwBBitMask == 0x00FF0000) &&
            (ddsh.ddspf.dwABitMask == 0xFF000000))
        {
            // We support D3D's A8B8G8R8, which is actually RGBA in linear
            // memory, equivalent to GL's RGBA
            image->format     = GL_RGBA;
            image->components = 4;
            image->componentFormat = GL_UNSIGNED_BYTE;
            image->bytesPerPixel = 4;
            image->alpha = 1;
            image->compressed = 0;
        }
        else if ((ddsh.ddspf.dwRGBBitCount == 32) &&
            (ddsh.ddspf.dwRBitMask == 0x00FF0000) &&
            (ddsh.ddspf.dwGBitMask == 0x0000FF00) &&
            (ddsh.ddspf.dwBBitMask == 0x000000FF) &&
            (ddsh.ddspf.dwABitMask == 0xFF000000)) 
        {
            // We support D3D's A8R8G8B8, which is actually BGRA in linear
            // memory, need to be 
            image->format     = GL_RGBA;
            image->components = 4;
            image->componentFormat = GL_UNSIGNED_BYTE;
            image->bytesPerPixel = 4;
            image->alpha = 1;
            image->compressed = 0;
            needsBGRASwap = NV_TRUE;
        } 
        else if ((ddsh.ddspf.dwRGBBitCount == 16) &&
            (ddsh.ddspf.dwRBitMask == 0x0000F800) &&
            (ddsh.ddspf.dwGBitMask == 0x000007E0) &&
            (ddsh.ddspf.dwBBitMask == 0x0000001F) &&
            (ddsh.ddspf.dwABitMask == 0x00000000))
        {
            // We support D3D's R5G6B5, which is actually RGB in linear
            // memory.  It is equivalent to GL's GL_UNSIGNED_SHORT_5_6_5
            image->format     = GL_RGB;
            image->components = 3;
            image->alpha = 0;
            image->componentFormat = GL_UNSIGNED_SHORT_5_6_5;
            image->bytesPerPixel = 2;
            image->compressed = 0;
        } 
        else if ((ddsh.ddspf.dwRGBBitCount == 8) &&
            (ddsh.ddspf.dwRBitMask == 0x00000000) &&
            (ddsh.ddspf.dwGBitMask == 0x00000000) &&
            (ddsh.ddspf.dwBBitMask == 0x00000000) &&
            (ddsh.ddspf.dwABitMask == 0x000000FF))
        {
            // We support D3D's A8
            image->format     = GL_ALPHA;
            image->components = 1;
            image->alpha = 1;
            image->componentFormat = GL_UNSIGNED_BYTE;
            image->bytesPerPixel = 1;
            image->compressed = 0;
        }
        else if ((ddsh.ddspf.dwRGBBitCount == 8) &&
            (ddsh.ddspf.dwRBitMask == 0x000000FF) &&
            (ddsh.ddspf.dwGBitMask == 0x00000000) &&
            (ddsh.ddspf.dwBBitMask == 0x00000000) &&
            (ddsh.ddspf.dwABitMask == 0x00000000))
        {
            // We support D3D's L8 (flagged as 8 bits of red only)
            image->format     = GL_LUMINANCE;
            image->components = 1;
            image->alpha = 0;
            image->componentFormat = GL_UNSIGNED_BYTE;
            image->bytesPerPixel = 1;
            image->compressed = 0;
        } 
        else if ((ddsh.ddspf.dwRGBBitCount == 16) &&
            (((ddsh.ddspf.dwRBitMask == 0x000000FF) &&
              (ddsh.ddspf.dwGBitMask == 0x00000000) &&
              (ddsh.ddspf.dwBBitMask == 0x00000000) &&
              (ddsh.ddspf.dwABitMask == 0x0000FF00)) ||
             ((ddsh.ddspf.dwRBitMask == 0x000000FF) && // GIMP header for L8A8
              (ddsh.ddspf.dwGBitMask == 0x000000FF) &&  // Ugh
              (ddsh.ddspf.dwBBitMask == 0x000000FF) &&
              (ddsh.ddspf.dwABitMask == 0x0000FF00)))
            )
        {
            // We support D3D's A8L8 (flagged as 8 bits of red and 8 bits of alpha)
            image->format     = GL_LUMINANCE_ALPHA;
            image->components = 2;
            image->alpha = 1;
            image->componentFormat = GL_UNSIGNED_BYTE;
            image->bytesPerPixel = 2;
            image->compressed = 0;
        } 
        else
        {
            NVLogError(__FUNCTION__, "Image data is not DXTC or supported RGB(A) format: %s", filename);
            NvFClose(fp);
            NVHHDDSFree(image);
            return NV_NULL;
        }
    }
    
    // detect flagging to indicate this texture was stored in a y-inverted fashion
    if(!(ddsh.dwFlags&DDS_LINEARSIZE))
    {
        if(ddsh.dwPitchOrLinearSize == DDS_MAGIC_FLIPPED)
        {
            isAllreadyFlipped = NV_TRUE;
        }
    }

    flipVertical = (isAllreadyFlipped != (flipVertical ? NV_TRUE : NV_FALSE)) 
        ? 1 : 0;

    // store primary surface width/height/numMipmaps
    image->width      = ddsh.dwWidth;
    image->height     = ddsh.dwHeight;
    image->numMipmaps = ddsh.dwFlags&DDS_MIPMAPCOUNT ? ddsh.dwMipMapCount : 1;

    if (image->numMipmaps > NVHHDDS_MAX_MIPMAPS) 
    {
        NVLogError(__FUNCTION__, "Too many mipmaps (image->numMipmaps = %d) in file: %s\nIncrease NVHHDDS_MAX_MIPMAPS (== %d) in nvhhdds.h", image->numMipmaps, filename, NVHHDDS_MAX_MIPMAPS);
        NvFClose(fp);
        NVHHDDSFree(image);
        return NV_NULL;
    }

    // allocate the meta datablock for all mip storage.    
    NVHHDDSAllocData(image);
    if (image->dataBlock == NV_NULL) 
    {
        NVLogError(__FUNCTION__, "Failed to allocate memory block for image data storage: %s", filename);
        NvFClose(fp);
        NVHHDDSFree(image);
        return NV_NULL;
    }

    NvS32 faces = image->cubemap ? 6 : 1;

    NvS32 index = 0;

    NvS32 j;
    for (j = 0; j < faces; j++)
    {
        // load all surfaces for the image
        NvS32 width  = image->width;
        NvS32 height = image->height;

        for (i = 0; i < image->numMipmaps; i++) 
        {
            // Get the size, read in the data.

            NvFRead(image->data[index], image->size[index], 1, fp);

            // Flip in Y for OpenGL if needed
            if (flipVertical)
            {
                if (flip_data_vertical((char*)image->data[index], width, height, image))
                {
                    NVLogError(__FUNCTION__, "Vertical flip of image data failed: %s", filename);
                    NvFClose(fp);
                    NVHHDDSFree(image);
                    return NV_NULL;
                }
            }

            // shrink to next power of 2
            width  >>= 1;
            height >>= 1;

            if (!width)
                width = 1;

            if (!height)
                height = 1;

            // make sure DXT isn't <4 on a side...
            if (image->compressed) 
            {
                if (width < 4) 
                    width = 4;
                if (height < 4) 
                    height = 4;
            }

            index++;
        }
    }

    if (needsBGRASwap)
    {
        NvS32 index = 0;
        NvS32 k;

        for (k = 0; k < faces; k++)
        {
            NvS32 width  = image->width;
            NvS32 height = image->height;

            for (i = 0; i < image->numMipmaps; i++)
            {
                char* data = (char*)(image->data[index]);
                NvS32 pixels = width * height;
                NvS32 j;

                for (j = 0; j < pixels; j++) 
                {
                    char temp = data[0];
                    data[0] = data[2];
                    data[2] = temp;

                    data += 4;
                }

                // shrink to next power of 2
                width  >>= 1;
                height >>= 1;

                if (!width)
                    width = 1;

                if (!height)
                    height = 1;

                index++;
            }
        }
    }

    NvFClose(fp);

//#ifdef _DEBUG
//    if( flipVertical )
//    {
//        char fnm[1024];
//        NvFSFindOnPath(filename,fnm,1024);
//
//        NVLog(NVLOGLEVEL_WARN, "NVHHDDS", "Run time flip occured, use DDSMOD to preflip", filename);
//    }
//#endif

    return image;
}

#if 0
//================================================================================
// DDSSave currently only supports writing out basic compressed formats for now.
//================================================================================
int NVHHDDSSave(const NVHHDDSImage *image, const char* filename, NvS32 flagAsFlipped)
{
    DDS_HEADER ddsh;
    char filecode[8] = "DDS ";
    NvFile *fp;
    int i, n;
    NvS32 faces = 1;

    // setup DDS header
    // this is highly customized, as we know our reader is
    // the only thing that needs to be able to handle it! ;)
    memset(&ddsh, 0, sizeof(ddsh));

    ddsh.dwSize = sizeof(DDS_HEADER);
    ddsh.dwHeight = image->height;
    ddsh.dwWidth = image->width;
    //ddsh.dwPitchOrLinearSize;
    //ddsh.dwDepth; // set below...
    //ddsh.dwMipMapCount; // set below...
    //ddsh.dwReserved1[11];
    ddsh.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);
    //ddsh.ddspf.dwFlags; // set below...
    //ddsh.ddspf.dwFourCC; // set below...
    //ddsh.dwCaps1;
    //ddsh.dwCaps2;
    //ddsh.dwReserved2[3];

    ddsh.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT;

    // store primary surface width/height/depth/numMipmaps
    ddsh.dwWidth = image->width;
    ddsh.dwHeight = image->height;
    if (image->numMipmaps > 1)
    {
        ddsh.dwMipMapCount = image->numMipmaps;
        ddsh.dwFlags |= DDS_MIPMAPCOUNT;
    }

    if (image->cubemap)
    {
        faces = 6;
        ddsh.dwCaps2 |= DDS_CUBEMAP |
            DDS_CUBEMAP_POSITIVEX |
            DDS_CUBEMAP_POSITIVEY |
            DDS_CUBEMAP_POSITIVEZ |
            DDS_CUBEMAP_NEGATIVEX |
            DDS_CUBEMAP_NEGATIVEY |
            DDS_CUBEMAP_NEGATIVEZ;
    }

    if(flagAsFlipped)
    {
        ddsh.dwPitchOrLinearSize = DDS_MAGIC_FLIPPED;
    }

    // figure out what the image format is
    switch(image->format) 
    {
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            ddsh.ddspf.dwFlags |= DDS_FOURCC;
            ddsh.ddspf.dwFourCC = FOURCC_DXT1;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            ddsh.ddspf.dwFlags |= DDS_FOURCC;
            ddsh.ddspf.dwFourCC = FOURCC_DXT3;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            ddsh.ddspf.dwFlags |= DDS_FOURCC;
            ddsh.ddspf.dwFourCC = FOURCC_DXT5;
            break;
        case GL_RGBA:
            if((image->components==4)&&
                (image->componentFormat==GL_UNSIGNED_BYTE)&&
                (image->bytesPerPixel==4))
            {
                ddsh.ddspf.dwFlags |= DDS_RGB|DDS_ALPHAPIXELS;
                ddsh.ddspf.dwRGBBitCount = 32;
                ddsh.ddspf.dwRBitMask = 0x000000ff;
                ddsh.ddspf.dwGBitMask = 0x0000ff00;
                ddsh.ddspf.dwBBitMask = 0x00ff0000;
                ddsh.ddspf.dwABitMask = 0xff000000;
            }
            else
            {
//                NVLog(NVLOGLEVEL_ERROR, "NVHHDDS", "Unsupported compressed format for saving file", filename);
                return -2;
            }
            break;
        case GL_RGB:
            if((image->components==3)&&
                (image->componentFormat==GL_UNSIGNED_SHORT_5_6_5)&&
                (image->bytesPerPixel==2))
            {
                ddsh.ddspf.dwFlags |= DDS_RGB;
                ddsh.ddspf.dwRGBBitCount = 16;
                ddsh.ddspf.dwRBitMask = 0x0000f800;
                ddsh.ddspf.dwGBitMask = 0x000007e0;
                ddsh.ddspf.dwBBitMask = 0x0000001f;
                ddsh.ddspf.dwABitMask = 0x00000000;
            }
            else
            {
//                NVLog(NVLOGLEVEL_ERROR, "NVHHDDS", "Unsupported compressed format for saving file", filename);
                return -2;
            }
            break;
        default:
//            NVLog(NVLOGLEVEL_ERROR, "NVHHDDS", "Unsupported compressed format for saving file", filename);
            return -2;
    }

    fp = NvFOpen(filename, "wb");
    if (fp == NV_NULL)
    {
//        NVLog(NVLOGLEVEL_ERROR, "NVHHDDS", "Could not open dds file for writing", filename);
        return -1;
    }

    // write out DSS file marker
    NvFWrite(filecode, 1, 4, fp);

    NvFWrite(&ddsh, sizeof(ddsh), 1, fp);

    // write out all surfaces for the image (6 surfaces for cubemaps)
    int ioff = 0;
    for (n=0; n < faces; n++)
    {
        for (i = 0; i < image->numMipmaps; i++) 
        {
            NvFWrite(image->data[ioff], image->size[ioff], 1, fp);
            ioff++;
        }
    }

    NvFClose(fp);

    return 0;
}
#endif

//================================================================================
//================================================================================
NVHHDDSImage *NVHHDDSAlloc(void)
{
    NVHHDDSImage *image = new NVHHDDSImage;
    if (image)
        memset(image, 0, sizeof(NVHHDDSImage));
    return(image);
}


//================================================================================
//================================================================================
void *NVHHDDSAllocData(NVHHDDSImage *image)
{
    if (image)
    {
        NvS32 i;
        NvS32 size = total_image_data_size(image);
        image->dataBlock = new NvU8[size]; // no need to calloc, as we fill every bit...
        if (image->dataBlock == NV_NULL)
        {
            NVLogError(__FUNCTION__, "Failed to allocate (%d KB) memory block for image data storage.", size/1024);
            return NV_NULL;
        }

        image->data[0] = image->dataBlock;

        NvS32 planes = image->numMipmaps * (image->cubemap ? 6 : 1);

        for (i = 1; i < planes; i++) // account for base plus each mip
        {
            image->data[i] = (void*)(((NvS32)(image->data[i - 1]))
                + image->size[i - 1]);
        }

        return(image->dataBlock); // in case caller wants to sanity check...
    }
    return NV_NULL;
}


//================================================================================
//================================================================================
void NVHHDDSFree(NVHHDDSImage *image)
{
    if (image)
    {
        if (image->dataBlock)
            delete[] (NvU8*)(image->dataBlock);
        image->dataBlock = NV_NULL;
        delete image;
        image = NV_NULL;
    }
}


//================================================================================
// ========== THE MAIN DXT COMPRESSION CODE ==========
//================================================================================

// INTERNAL STATIC VARIABLES for DXT processing.
static NvS8 *s_tempStorage = NV_NULL;
static NvS32 s_tempStorageSize = 0;
static NvS32 s_srcFormat = 0;
static NvS32 s_srcType = 0;

//================================================================================
// we use this to set up our processing format/type ONCE, then use statics/globals to
// reference for the rest of the time -- rather than passing as params to each function.
//================================================================================
static void SetCurrentFormat(NvS32 format, NvS32 type)
{
    s_srcFormat = format;
    s_srcType = type;
}


//================================================================================
//================================================================================
static void *GetTexelAddress( NvS32 texelindex, void *ptex )
{
    void *result;

    switch (s_srcType)
    {
        case GL_UNSIGNED_BYTE:
            /* depends on the format */
            switch (s_srcFormat)
            {
                case GL_RGBA:
                    result = &(((NvU8 *)(ptex))[4*texelindex]);
                    break;
                case GL_RGB:
                    result = &(((NvU8 *)(ptex))[3*texelindex]);
                    break;
                case GL_LUMINANCE_ALPHA:
                    result = &(((NvU8 *)(ptex))[2*texelindex]);
                    break;
                case GL_LUMINANCE:
                    result = &(((NvU8 *)(ptex))[1*texelindex]);
                    break;
                case GL_ALPHA:
                    result = &(((NvU8 *)(ptex))[1*texelindex]);
                    break;
                default:
                    NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
                    return NV_NULL;
                    break;
            }
            break;

        /* all of these are 1 unsigned short per texel */
        case GL_UNSIGNED_SHORT_5_6_5:
        case GL_UNSIGNED_SHORT_4_4_4_4:
        case GL_UNSIGNED_SHORT_5_5_5_1:
            result = &(((NvU16 *)(ptex))[texelindex]);
            break;

        default:
            NVLogError(__FUNCTION__, "Unsupported type [0x%x]", (NvU32)s_srcType);
            return NV_NULL;
    }

    return result;
}


//================================================================================
//================================================================================
static Color8888 TexelToColor8888(void *texel)
{
    Color8888 result = {0,0,0,0};
    NvU8  *ubptr = (NvU8 *) texel;
    NvU16 *usptr = (NvU16 *) texel;

    switch (s_srcType)
    {
        case GL_UNSIGNED_BYTE:
            switch (s_srcFormat)
            {
                case GL_RGBA:
                    result.red   = ubptr[0];
                    result.green = ubptr[1];
                    result.blue  = ubptr[2];
                    result.alpha = ubptr[3];
                    break;
                case GL_RGB:
                    result.red   = ubptr[0];
                    result.green = ubptr[1];
                    result.blue  = ubptr[2];
                    result.alpha = 255;
                    break;
                case GL_LUMINANCE:
                    result.red   = ubptr[0];
                    result.green = ubptr[0];
                    result.blue  = ubptr[0];
                    result.alpha = 255;
                    break;
                case GL_LUMINANCE_ALPHA:
                    result.red   = ubptr[0];
                    result.green = ubptr[0];
                    result.blue  = ubptr[0];
                    result.alpha = ubptr[1];
                    break;
                case GL_ALPHA:
                    result.red   = 0;
                    result.green = 0;
                    result.blue  = 0;
                    result.alpha = ubptr[0];
                    break;
                default:
                    NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
                    break;
            }
            break;

        case GL_UNSIGNED_SHORT_5_6_5:
            if (s_srcFormat != GL_RGB)
            {
                NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
                break;
            }
            result.red   = (NvU8) ((((usptr[0] & 0xF800) >> 11) & 0x0000001F) << 3);
            result.green = (NvU8) ((((usptr[0] & 0x07E0) >>  5) & 0x0000003F) << 2);
            result.blue  = (NvU8) ((((usptr[0] & 0x001F) >>  0) & 0x0000001F) << 3);
            result.alpha = 255;
            break;
        case GL_UNSIGNED_SHORT_4_4_4_4:
            if (s_srcFormat != GL_RGBA)
            {
                NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
                break;
            }
            result.red   = (NvU8) ((((usptr[0] & 0xF000) >> 12) & 0x0000000F) << 4);
            result.green = (NvU8) ((((usptr[0] & 0x0F00) >>  8) & 0x0000000F) << 4);
            result.blue  = (NvU8) ((((usptr[0] & 0x00F0) >>  4) & 0x0000000F) << 4);
            result.alpha = (NvU8) ((((usptr[0] & 0x000F) >>  0) & 0x0000000F) << 4);
            break;
        case GL_UNSIGNED_SHORT_5_5_5_1:
            if (s_srcFormat != GL_RGBA)
            {
                NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
                break;
            }
            result.red   = (NvU8) ((((usptr[0] & 0xF800) >> 11) & 0x0000001F) << 3);
            result.green = (NvU8) ((((usptr[0] & 0x07C0) >>  6) & 0x0000001F) << 3);
            result.blue  = (NvU8) ((((usptr[0] & 0x003E) >>  1) & 0x0000001F) << 3);
            result.alpha = (NvU8) (255 * (usptr[0] & 0x00000001));
            break;
        default:
            NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
            break;
    }

    return result;
}


//================================================================================
//================================================================================
static void Color8888ToTexel(Color8888 input, void *texel)
{
    NvU8  *ubptr = (NvU8 *) texel;
    NvU16 *usptr = (NvU16 *) texel;

    switch (s_srcType)
    {
        case GL_UNSIGNED_BYTE:
            switch (s_srcFormat)
            {
                case GL_RGBA:
                    ubptr[0] = input.red;
                    ubptr[1] = input.green;
                    ubptr[2] = input.blue;
                    ubptr[3] = input.alpha;
                    break;
                case GL_RGB:
                    ubptr[0] = input.red;
                    ubptr[1] = input.green;
                    ubptr[2] = input.blue;
                    break;
                case GL_LUMINANCE:
                    ubptr[0] = input.red;
                    break;
                case GL_LUMINANCE_ALPHA:
                    ubptr[0] = input.red;
                    ubptr[1] = input.alpha;
                    break;
                case GL_ALPHA:
                    ubptr[0] = input.alpha;
                    break;
                default:
                    NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
                    break;
            }
            break;

        case GL_UNSIGNED_SHORT_5_6_5:
            if (s_srcFormat != GL_RGB)
            {
                NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
                break;
            }
            usptr[0] = ((input.red >> 3) << 11) | ((input.green >> 2) << 5) | (input.blue >> 3);
            break;
        case GL_UNSIGNED_SHORT_4_4_4_4:
            if (s_srcFormat != GL_RGBA)
            {
                NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
                break;
            }
            usptr[0] = ((input.red >> 4) << 12) | ((input.green >> 4) << 8) | ((input.blue >> 4) << 4) | (input.alpha >> 4);
            break;
        case GL_UNSIGNED_SHORT_5_5_5_1:
            if (s_srcFormat != GL_RGBA)
            {
                NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
                break;
            }
            usptr[0] = ((input.red >> 3) << 11) | ((input.green >> 3) << 6) | ((input.blue >> 3) << 1) | ((input.alpha > 0) ? 1 : 0);
            break;
        default:
            NVLogError(__FUNCTION__, "Unsupported format/type pair [0x%x 0x%x]", (NvU32)s_srcFormat, (NvU32)s_srcType);
            break;
    }
}


//================================================================================
//================================================================================
static void SetTexelColor(Color8888 color, NvS32 texelindex, void *ptex)
{
    void *ptr;
    ptr = GetTexelAddress(texelindex, ptex);
    Color8888ToTexel(color, ptr);
}

//================================================================================
//================================================================================
static Color8888 GetTexelColor(NvS32 texelindex, void *ptex)
{
    void *ptr;
    ptr = GetTexelAddress(texelindex, ptex);
    return TexelToColor8888(ptr);
}

//================================================================================
//================================================================================
static void BoxFilterImage(NvS32 w, NvS32 h, void *psrc, void *pdst)
{
    NvS32 sw, sh, r, g, b, a;
    Color8888 sample, tmp;

    for (sh = 0; sh < h; sh+=2)
    {
        for (sw = 0; sw < w; sw+=2)
        {
            tmp = GetTexelColor(w*sh + sw, psrc);
            r = tmp.red;
            g = tmp.green;
            b = tmp.blue;
            a = tmp.alpha;
            tmp = GetTexelColor(w*sh + sw+1, psrc);
            r += tmp.red;
            g += tmp.green;
            b += tmp.blue;
            a += tmp.alpha;
            tmp = GetTexelColor(w*(sh+1) + sw, psrc);
            r += tmp.red;
            g += tmp.green;
            b += tmp.blue;
            a += tmp.alpha;
            tmp = GetTexelColor(w*(sh+1) + sw+1, psrc);
            r += tmp.red;
            g += tmp.green;
            b += tmp.blue;
            a += tmp.alpha;

            sample.red = (NvU8) (r >> 2);
            sample.green = (NvU8) (g >> 2);
            sample.blue = (NvU8) (b >> 2);
            sample.alpha = (NvU8) (a >> 2);

            SetTexelColor(sample, ((w>>1) * (sh>>1)) + (sw>>1), pdst);
        }
    }
}


//================================================================================
//================================================================================
NVHHDDSImage *NVCompressToDXT(NVHHDDSImage *srcImage, NvS32 toFormat, NvS32 bMakeMipmaps)
{
    // VARIABLES -------------------
    NVHHDDSImage *dstImage;
    void *srcdata;
    void *texel;
    NvU32 level;
    NvU32 uMipLevel = 0;
    NvS32 w, h;
    NvS32 i, x, y;

    // variables used in the processing inner loop.
    void *p[4];
    NvU8 dxAlpha[4][4]; // caching for alpha values for a given block.
    NvU8 cx, cy;
    NvU8 xmin = 0, ymin = 0, xmax = 0, ymax = 0;
    NvU16 l[4][4];
    NvU16 lmin = 0, lmax = 0;
    NvU16 lcut[3];
    Color8888 cmin, cmax, rgba;
    NvU16 c[2];
    NvU32 idx = 0, flip = 0;
    GLboolean bHasAlpha = GL_FALSE;
    GLboolean bFirst = GL_TRUE;
    NvU32 code;
    NvU16 t;

    // CODE STARTS HERE -------------------

    if (!srcImage)
        return NV_NULL;

    if (srcImage->cubemap)
    {
        NVLogError(__FUNCTION__, "!> Error: cubemap compression not yet supported.");
        return NV_NULL;
    }

    // hold onto format/type globally, so we don't pass around all the time.
    SetCurrentFormat(srcImage->format, srcImage->componentFormat);

    // make our destination image structure
    dstImage = NVHHDDSAlloc();
    if (!dstImage)
        return NV_NULL;

    dstImage->width = srcImage->width;
    dstImage->height = srcImage->height;
    dstImage->components = 4; // Documented as not set when compressed is true
    dstImage->componentFormat = GL_UNSIGNED_BYTE;
    dstImage->bytesPerPixel = 1; // Documented as not set when compressed is true
    dstImage->compressed = 1;
    dstImage->format = toFormat;

    w = srcImage->width;
    h = srcImage->height;
    level = (NvU32)((w > h) ? w : h);
    if (srcImage->numMipmaps > 1) // use that
    {
        dstImage->numMipmaps = srcImage->numMipmaps;
    }
    else
    {
        if (bMakeMipmaps)
        {
            if (level)
            {
                dstImage->numMipmaps = 0;
                for (i = level; i; i >>= 1)
                    dstImage->numMipmaps++;
            }
            else
            {
                dstImage->numMipmaps = 1;
            }
        }
        else
        {
            dstImage->numMipmaps = 1;
        }
    }

    // set srcdata for first time through loop.  
    // do this as we may generate mip data on-the-fly, 
    // not pulled from srcImage->data[n]...
    if (!srcImage->data || !srcImage->data[0])
    {
        NVHHDDSFree(dstImage);
        return NV_NULL;
    }

    srcdata = srcImage->data[0];

    // let the DDS system allocate the proper-size datablock...
    NVHHDDSAllocData(dstImage);
    if (dstImage->dataBlock == NV_NULL) 
    {
        NVLogError(__FUNCTION__, "!> Error allocating DXT conversion buffer");
        NVHHDDSFree(dstImage);
        return NV_NULL;
    }

    // reset w and h
    w = srcImage->width;
    h = srcImage->height;

    while (1)
    {
        DXTColBlock *pdxtd = (DXTColBlock*)dstImage->data[uMipLevel];

        NVLogDebug(__FUNCTION__, "processing mip level %d, %dx%d", uMipLevel, w, h);

        for (y = 0; y < h; y += 4)
        {
            /* cache pointers to the first entry of the first row of the block. */
            for (i = 0; i < 4; ++i)
            {
                p[i] = GetTexelAddress( w * (y + i), srcdata );
            }

            for (x = 0; x < w; x += 4)
            {
                // reset variables.
                xmin = 0, ymin = 0, xmax = 0, ymax = 0;
                lmin = 0, lmax = 0;
                idx = 0, flip = 0;
                bHasAlpha = GL_FALSE;
                bFirst = GL_TRUE;
            
                // find the min/max luma locations for the 4x4 cell
                for (cy = 0; cy < 4; ++cy)
                {
                    for (cx = 0; cx < 4; ++cx)
                    {
                        texel = GetTexelAddress( (x + cx), p[cy] );
                        rgba = TexelToColor8888( texel );
                        dxAlpha[cy][cx] = rgba.alpha; // hold onto it, in case we are doing DXT3/5 processing.
                        if (toFormat==GL_COMPRESSED_RGBA_S3TC_DXT1_EXT && rgba.alpha < 0x7f)
                        {
                            bHasAlpha = GL_TRUE;
                        }
                        else // opaque or DXT3/5
                        {
                            // calculate approximate luma value
                            l[cy][cx] =    ((NvU16)rgba.red)*3 + ((NvU16)rgba.green)*6 + ((NvU16)rgba.blue);
                            if (bFirst)
                            {
                                bFirst = GL_FALSE;
                                xmin = xmax = cx;
                                ymin = ymax = cy;
                                lmin = lmax = l[cy][cx];
                            }
                            else
                            {
                                if (lmin > l[cy][cx])
                                {
                                    lmin = l[cy][cx];
                                    xmin = cx;
                                    ymin = cy;
                                }

                                if (lmax < l[cy][cx])
                                {
                                    lmax = l[cy][cx];
                                    xmax = cx;
                                    ymax = cy;
                                }
                            }
                        }
                    }
                }

                // pack RGB565 values for endpoints
                texel = GetTexelAddress( (x + xmin), p[ymin] );
                cmin = TexelToColor8888( texel );

                texel = GetTexelAddress( (x + xmax), p[ymax] );
                cmax = TexelToColor8888( texel );
                c[0] = ((cmin.red >> 3) << 11) | ((cmin.green >> 2) << 5) | (cmin.blue >> 3);
                c[1] = ((cmax.red >> 3) << 11) | ((cmax.green >> 2) << 5) | (cmax.blue >> 3);

                if (bHasAlpha || (c[0] == c[1]))
                {
                    // encode numerically smaller endpoint first (c0 <= c1 implies alpha)
                    if (c[1] < c[0])
                    {
                        t = c[0]; c[0] = c[1]; c[1] = t;
                        flip = 1;
                    }

                    // decision points at 1/3 and 2/3
                    lcut[0] = (2 * lmin + lmax + 1) / 3;
                    lcut[1] = (lmin + 2 * lmax + 1) / 3;

                    // generate the index bits
                    for (cy = 0; cy < 4; ++cy)
                    {
                        for (cx = 0; cx < 4; ++cx)
                        {
                            texel = GetTexelAddress((x + cx), p[cy]);
                            if (TexelToColor8888( texel ).alpha < 0x7F)
                            {
                                code = 3;
                            }
                            else if (l[cy][cx] <= lcut[0])
                            {
                                code = 0 ^ flip;
                            }
                            else if (l[cy][cx] <= lcut[1])
                            {
                                code = 2;
                            }
                            else
                            {
                                code = 1 ^ flip;
                            }

                            // build from the end of the codeword down; 0,0 should be in lsbs
                            idx = (idx >> 2) | (code << 30);
                        }
                    }
                }
                else // opaque
                {
                    // encode numerically larger endpoint first (c0 > c1 implies no alpha)
                    if (c[0] < c[1])
                    {
                        t = c[0]; c[0] = c[1]; c[1] = t;
                        flip = 1;
                    }

                    // decision points at 1/4, 1/2 and 3/4
                    lcut[0] = (3 * lmin + lmax + 2) / 4;
                    lcut[1] = (lmin + lmax) / 2;
                    lcut[2] = (lmin + 3 * lmax + 2) / 4;

                    // generate the index bits
                    for (cy = 0; cy < 4; ++cy)
                    {
                        for (cx = 0; cx < 4; ++cx)
                        {
                            if (l[cy][cx] <= lcut[0])
                            {
                                code = 0 ^ flip;
                            }
                            else if (l[cy][cx] <= lcut[1])
                            {
                                code = 2 ^ flip;
                            }
                            else if (l[cy][cx] <= lcut[2])
                            {
                                code = 3 ^ flip;
                            }
                            else
                            {
                                code = 1 ^ flip;
                            }

                            // build from the end of the codeword down; 0,0 should be in lsbs
                            idx = (idx >> 2) | (code << 30);
                        }
                    }
                }

                // pack the alpha cell FIRST, if DXT3 or 5 (it comes BEFORE the color data...)
                if (toFormat==GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
                {
                    // since the alpha block and the color blocks are both 8-bytes,
                    // just increment the ptr 'naturally'.
                    DXT3AlphaBlock *pdxt3h = (DXT3AlphaBlock *)pdxtd++; 
                    for (cy = 0; cy < 4; ++cy) // loop over rows, pack 4 texels into 1 short...
                    {
                        // first value goes in low bits, rest gets shifted up.
                        pdxt3h->row[cy] =    ((dxAlpha[cy][0] >> 4) & 0xF) << 0        
                                        |    ((dxAlpha[cy][1] >> 4) & 0xF) << 4
                                        |    ((dxAlpha[cy][2] >> 4) & 0xF) << 8
                                        |    ((dxAlpha[cy][3] >> 4) & 0xF) << 12;
                    }
                }
                else if (toFormat==GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
                {
                    // since the alpha block and the color blocks are both 8-bytes,
                    // just increment the ptr 'naturally'.
                    DXT5AlphaBlock *pdxt5h = (DXT5AlphaBlock *)pdxtd++;
                    NvU16 alut[8],slut[8];

                    // first, find min/max
                    lmin = 255; lmax = 0; // use l vars for current minmax
                    for (cy = 0; cy < 4; ++cy)
                    {
                        for (cx = 0; cx < 4; ++cx)
                        {
                            if (lmax < dxAlpha[cy][cx])
                                lmax = dxAlpha[cy][cx];

                            if (lmin > dxAlpha[cy][cx])
                                lmin = dxAlpha[cy][cx];
                        }
                    }

                    // !!!!TBD we could optimize here if lmin==lmax and ==0 or ==255
                    // just spit out the block as 0s or FFs, with the endpoint colors both 0 or 255...
                    // hold onto our potential start/end points.
                    alut[0] = lmin;
                    alut[7] = lmax;

                    // !!!!!TBD could improve spanning results here....
                    // if one of the alpha values is 0 or 255 (or NEAR), 
                    // and the remaining values fit in a 'small delta space'
                    // then we should run a simplistic algorithm to determine 
                    // if we might be better off using a 6-point system
                    // !!!!TBD this should decide based on lower average ERROR
                    // from what the interp results will be, rather than size of delta space.
                    NvS32 smallDeltaSpace = 0, smallDeltaVal = 3, smallDeltaTol = 128;
                    // try to detect our small delta space case...
                    // if lmin near 0 or lmax near 255, look at values 
                    // NOT close to 0/255 for a smaller subset range.
                    // if range is large, try looking if we have a small subset range
                    // that'd work better
                    if ((lmax-lmin > smallDeltaTol) &&
                        ((lmin < smallDeltaVal) || (lmax > 255-smallDeltaVal)))
                    {
                        // find the total 'spread' of the other 6 values.  
                        // if it's less than some amount, switch to the 6pt system.                        
                        lmin = 255; lmax = 0; // use l vars for current minmax
                        for (cy = 0; cy < 4; ++cy)
                        {
                            for (cx = 0; cx < 4; ++cx)
                            {
                                if ((dxAlpha[cy][cx] < smallDeltaVal) || 
                                    (dxAlpha[cy][cx] > 255-smallDeltaVal))
                                {
                                    continue; // ignore this for purposes of minmax
                                }

                                if (lmax < dxAlpha[cy][cx]) 
                                    lmax = dxAlpha[cy][cx];
                                
                                if (lmin > dxAlpha[cy][cx])
                                    lmin = dxAlpha[cy][cx];
                            }
                        }

                        if (lmax-lmin < smallDeltaTol) // if new minmax seems a reasonable 'small range'...
                            smallDeltaSpace = lmax-lmin;
                    }

                    // !!!!TBD this should decide based on lower average ERROR from what the
                    // interp results will be, rather than size of delta space.
                    if (smallDeltaSpace) // use 6pt system.
                    {
                        alut[0] = 0; // w/6pt system, we 'hardcode' [0] and [7] to 0 and 255...
                        alut[7] = 255;
                        alut[1] = lmin;
                        alut[6] = lmax;
                        for (i = 2; i < 6; i++)
                        {
                            // should be between 0 and 255...
                            alut[i] = (NvU16) (((6-i)*alut[1] + (i-1)*alut[6]) / 5);
                        }
                        slut[0] = 6;
                        slut[7] = 7;
                        slut[1] = 0;
                        slut[6] = 1;
                        for (i = 2; i < 6; i++) 
                        {
                            slut[i] = (NvU16) i;
                        }
                    }
                    else // otherwise, we use a 8-point system, inclusive of our min/max
                    {
                        // we set [0] and [7] earlier...
                        for (i = 1; i < 7; i++)
                        {
                            // should be between 0 and 255...
                            alut[i] = (NvU16) (((7-i)*alut[7] + i*alut[0]) / 7); 
                        }
                        slut[0] = 1;
                        slut[7] = 0;
                        for (i = 1; i < 7; i++)
                        {
                            slut[i] = (NvU16) i+1;
                        }
                    }

                    // then, loop through each alpha, find the nearest match regardless of 
                    // 6pt/8pt system, and hang onto the 3-bit index value.  we'll encode in second pass.
                    for (cy = 0; cy < 4; ++cy)
                    {
                        for (cx = 0; cx < 4; ++cx)
                        {
                            for (i=0; i<7; i++)
                            {
                                // is it between these two alut values
                                if ((alut[i] <= dxAlpha[cy][cx]) &&
                                    (dxAlpha[cy][cx] <= alut[i+1]))
                                {
                                    // if so, which is it NEAREST to?  replace VALUE with that INDEX.
                                    if ((dxAlpha[cy][cx] - alut[i]) < 
                                        (alut[i+1]-dxAlpha[cy][cx]))
                                    {
                                        dxAlpha[cy][cx] = (NvU8)slut[i];
                                    }
                                    else
                                    {
                                        dxAlpha[cy][cx] = (NvU8)slut[i+1];
                                    }
                                    break; // done with calc for this cx/cy
                                }
                            }
                        }
                    }

                    // now, figure out how to encode the bits.
                    NvU8 *bits, shift = 0, addval;
                    if (smallDeltaSpace) // use 6pt system, set a0 <= a1
                    {
                        // normal values tells it we're using the 6-point system, 
                        // and that [v6]==0 and [v7]==255
                        pdxt5h->alpha0 = (NvU8)alut[1]; 
                        pdxt5h->alpha1 = (NvU8)alut[6];
                    }
                    else // 8pt system, set a0 > a1
                    {
                        // inverting values tells it we're using the 8-point system, 
                        // and that [v6]==0 and [v7]==255
                        pdxt5h->alpha0 = (NvU8)alut[7]; 
                        pdxt5h->alpha1 = (NvU8)alut[0];
                    }

                    for (i = 0; i < 6; i++)
                    {
                        // clear index space
                        pdxt5h->row[i] = 0; 
                    }
                    bits = pdxt5h->row; // starting one.
                    for (cy = 0; cy < 4; ++cy)
                    {
                        for (cx = 0; cx < 4; ++cx)
                        {
                            // try to fit as many bits in as we can
                            addval = dxAlpha[cy][cx];
                            *bits |= (addval<<shift);
                            shift += 3;

                            // didn't all fit.  upper bits overflow. 
                            // go to next short, add in what's left.
                            if (shift >= 8)
                            {
                                bits++; // move to next byte
                                shift -= 8; // adjust shift for moving over byte bound.
                                if (shift) // bits left to add to new short.
                                    *bits = addval>>(3-shift);
                            }
                        }    
                    }                        
                } // done with DXT3/5 handling.

                // Now, pack the color data cell for all DXT types.
                pdxtd->col0 = c[0];
                pdxtd->col1 = c[1];
                pdxtd->row[0] = (NvU8) ((idx>>0)&0xFF);
                pdxtd->row[1] = (NvU8) ((idx>>8)&0xFF);
                pdxtd->row[2] = (NvU8) ((idx>>16)&0xFF);
                pdxtd->row[3] = (NvU8) ((idx>>24)&0xFF);

                // advance to the next DXT-base cell
                ++pdxtd;
            }
        }

        // The number of bytes we consumed in the above loops should match
        // the precomputed size of the mip level!
//        kdAssert ( dstImage->size[uMipLevel] ==
//                    (NvS32)((NvU32)pdxtd - (NvU32)(dstImage->data[uMipLevel])) );

        NVLogDebug(__FUNCTION__, "Finished dxt mipmap %d (finish size %d)", uMipLevel, dstImage->size[uMipLevel]);

        // have we done all the levels yet?
        uMipLevel++;
        if (uMipLevel == (NvU32)dstImage->numMipmaps)
            break; // out of the big while loop.

        // if flag set, make next mip level, assuming it doesn't exist already in source...
        if (bMakeMipmaps)
        {
            if (!s_tempStorage || s_tempStorageSize < w*h)
            {
                if (s_tempStorage)
                    delete[] s_tempStorage; // use our helper.

                s_tempStorageSize = w*h; // bytes == w/2 * h*2 * 4, /2/2*4 cancels out.
                s_tempStorage = new NvS8[s_tempStorageSize];

                if (!s_tempStorage)
                {
                    NVHHDDSFree(dstImage);
                    return NV_NULL;
                }
            }
            BoxFilterImage( w, h, srcdata, s_tempStorage );
            // use the temp storage as the data source for the next mip pass...
            srcdata = s_tempStorage;
        }
        else
        { // see if we HAVE a next mip level, else we need to bail.
            if ((NvU32)srcImage->numMipmaps < uMipLevel)
                break;

            if (srcImage->data[uMipLevel] == NV_NULL)
                break; // out of the big while loop.

            srcdata = srcImage->data[uMipLevel];
        }

        w = (w >> 1) == 0 ? 1 : (w >> 1);
        h = (h >> 1) == 0 ? 1 : (h >> 1);
    }

    NVLogDebug(__FUNCTION__, "Finished DXT compression.");

    // we pass back the dstImage pointer.
    // caller is then responsible for storing the converted data, uploading, and freeing the ptr.
    // as well as caller is still responsible for the srcImage too (we don't dispose of it!)
    return(dstImage);
}

//================================================================================
//================================================================================
void NVCompressFlush(void)
{
    if (s_tempStorage != NV_NULL)
        delete[] s_tempStorage;
    s_tempStorage = NV_NULL;
    s_tempStorageSize = 0;
}
