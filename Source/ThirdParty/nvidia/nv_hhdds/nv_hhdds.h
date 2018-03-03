//----------------------------------------------------------------------------------
// File:        jni/nv_hhdds/nv_hhdds.h
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

#ifndef _NV_HHDDS_H_INCLUDED
#define _NV_HHDDS_H_INCLUDED

#include <nv_global.h>

#ifdef __cplusplus
extern "C" {
#endif

    /* GL_EXT_texture_compression_dxt1 */
#ifndef GL_EXT_texture_compression_dxt1
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#endif

/* GL_EXT_texture_compression_s3tc */
#ifndef GL_EXT_texture_compression_s3tc
/* GL_COMPRESSED_RGB_S3TC_DXT1_EXT defined in GL_EXT_texture_compression_dxt1 already. */
/* GL_COMPRESSED_RGBA_S3TC_DXT1_EXT defined in GL_EXT_texture_compression_dxt1 already. */
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

/** \file nv_hhdds.h
    Support for DDS-file texture loading/saving and DXT compression
    
    Support for DXT texture loading/saving and compression.  Supports:
    <ul>
        <li> DDS 2D texture loading with mipmaps
        <li> DDS cube map texture loading with mipmaps
        <li> Mipmap pyramid generation
        <li> DDS 2D and cube map texture saving
        <li> On-the-fly conpression to DXT formats
    </ul>

    The library can load DDS files of the following formats: 
    <ul>
        <li> DXT1, DXT3, DXT5
        <li> A8B8G8R8
        <li> A8R8G8B8
        <li> R5G6B5
        <li> A8, L8, A8L8
    </ul>
 */
    
/** The maximum number of mipmap levels (per texture or per cubemap face) */
#define NVHHDDS_MAX_MIPMAPS       (16)

/** The number of cubemap faces that must exist in a cubemap-bearing DDS file */
#define NVHHDDS_NUM_CUBEMAP_FACES (6)

/** The master DDS structure for loading and saving

    This is the master DDS structure.  It shouldn't be allocated by hand, 
    always use NVHHDDSAlloc/NVHHDDSAllocData/NVHHDDSFree to manage them properly.

    @see NVHHDDSAlloc()
    @see NVHHDDSAllocData()
    @see NVHHDDSFree()
*/
typedef struct 
{
    /** Width of the overall texture in texels */
    NvS32 width;
    /** Height of the overall texture in texels */
    NvS32 height;
    /** Number of color/alpha components per texel 1-4 */
    NvS32 components;
    /** The GL type of each color component (noncompressed textures only) */
    NvS32 componentFormat;
    /** The number of bytes per pixel (noncompressed textures only) */
    NvS32 bytesPerPixel;
    /** Nonzero if the format is DXT-compressed */
    NvS32 compressed;
    /** The number of levels in the mipmap pyramid (including the base level) */
    NvS32 numMipmaps;
    /** If nonzero, then the file contains 6 cubemap faces */
    NvS32 cubemap;
    /** The GL format of the loaded texture data */
    NvS32 format;
    /** Nonzero if the texture data includes alpha */
    NvS32 alpha;
    /** Base of the allocated block of all texel data */
    void *dataBlock;
    /** Pointers to the mipmap levels for the texture or each cubemap face */
    void *data[NVHHDDS_MAX_MIPMAPS * NVHHDDS_NUM_CUBEMAP_FACES];
    /** Array of sizes of the mipmap levels for the texture or each cubemap face */
    NvS32   size[NVHHDDS_MAX_MIPMAPS * NVHHDDS_NUM_CUBEMAP_FACES];
    /** Array of widths of the mipmap levels for the texture or each cubemap face */
    NvS32 mipwidth[NVHHDDS_MAX_MIPMAPS * NVHHDDS_NUM_CUBEMAP_FACES];
    /** Array of heights of the mipmap levels for the texture or each cubemap face */
    NvS32 mipheight[NVHHDDS_MAX_MIPMAPS * NVHHDDS_NUM_CUBEMAP_FACES];
} NVHHDDSImage;

/** The main DDS file loading function
    
    @return A pointer to the allocated NVHHDDSImage object representing the 
        texture file, or NULL if the file cannot be found or contains an
        unsupported DDS image format.  This result should be freed with
        a call to NVHHDDSFree when the application is finished with it.
    @param filename The path of the DDS file to be loaded
    @param flipVertical If nonzero, flips the texels across the vertical axis.
        Normally, D3D and OpenGL have opposite ideas of "up", so in most cases,
        flipVertical should be nonzero for OpenGL ES applications.  Note that
        if the image file being loaded was saved via NVHHDDSSave with the
        flagAsFlipped parameter nonzero, then this flag is ignored and no
        flipping occurs.
    @see NVHHDDSFree()
*/
NVHHDDSImage *NVHHDDSLoad(const char* filename, NvS32 flipVertical);

/** Saves the given DDS texture data to a file

    This is a quick-solve for prototype apps that need to be able to 'cache'
    dxt-compressed files, for faster re-loads.  since the runtime compression
    isn't great, this is meant for early proto use, NOT FOR SHIPPING an app on 
    top of.  the format isn't fully specified, just enough for NVHHDDSLoad to 
    read it back in.    
    
    @return 0 on success, -2 if the incoming data is not a saveable format, and
    -1 if the requested file cannot be opened for writing.
    
    @param image A pointer to the allocated NVHHDDSImage object representing the 
        texture file to be written.
    @param filename The path of the DDS file to be saved
    @param flagAsFlipped If nonzero, indicates that the texture data has already 
        been flipped to match OpenGL ES "up" orientation.  During saving, this 
        flag is written to the file.  NVHHDDSLoad will read this flag and avoid
        the computationally-expensive vertical flipping operation.
    @see NVHHDDSLoad()
*/
// Currently, NvFile does not write
//NvS32 NVHHDDSSave(const NVHHDDSImage *image, const char *filename, NvS32 flagAsFlipped);

/** Allocates an empty NVHHDDSImage object

    No image data arrays are allocated.
    Use NVHHDDSAllocData to allocate texel data.
    
    @return A pointer to a valid (empty) NVHHDDSImage object on success, NULL
    on failure.
    
    @see NVHHDDSAllocData()
    @see NVHHDDSFree()
*/
NVHHDDSImage *NVHHDDSAlloc(void);

/** Allocates the pixel data arrays in an initialized NVHHDDSImage object

    The following NVHHDDSImage members MUST be set prior to this call in order for
    the function to succeed:
    <ul>
        <li> width
        <li> height
        <li> numMipmaps
        <li> compressed
        <li> format
        <li> bytesPerPixel (if not compressed)
        <li> cubemap
    </ul>
    
    @param image A pointer to the previously-allocated and initialized 
        NVHHDDSImage object whose pixel data arrays are to be allocated
    @return A pointer to the base of the data block that was attached to the given
        NVHHDDSImage object, or NULL on failure.
    
    @see NVHHDDSAllocData()
    @see NVHHDDSFree()
*/
void         *NVHHDDSAllocData(NVHHDDSImage *image);

/** Frees an NVHHDDSImage object and the pixel data arrays it contains 

    Assumes that the object was allocated using NVHHDDSAlloc and if the pixel
    arrays have been allocated, they were allocated using NVHHDDSAllocData.

    @param image A pointer to the previously-allocated and initialized 
        NVHHDDSImage object to be deleted.
    
    @see NVHHDDSAlloc()
    @see NVHHDDSAllocData()
*/
void          NVHHDDSFree(NVHHDDSImage *image);

/** Generates a compressed texture from uncompressed data 

    This function call takes an uncompressed texture in a DDS structure, and
    compresses it into a DXT format (1, 1a, 3, or 5).  Optionally, it can 
    auto-generate mipmaps.  Note that in general, compression at run-time is
    not recommended for the following reasons:
    <ul>
        <li> Pre-compression reduces data footprint over-the-air and on the
            device filesystem
        <li> Image quality of compressed textures is significantly better 
            when generated by offline tools
        <li> Run-time compression increases load time
        <li> Run-time compression consumes additional power
    </ul>

    The currently supported texture formats are:
    <ul>
        <li> GL_RGBA
        <li> GL_RGB
        <li> GL_LUMINANCE_ALPHA
        <li> GL_LUMINANCE
        <li> GL_ALPHA
    </ul>
    
    The currently supported types are:
    <ul>
        <li> GL_UNSIGNED_BYTE
        <li> GL_UNSIGNED_SHORT_5_6_5
        <li> GL_UNSIGNED_SHORT_4_4_4_4
        <li> GL_UNSIGNED_SHORT_5_5_5_1
    </ul>

    @param srcImage A pointer to the previously-allocated and initialized 
        NVHHDDSImage object to be compressed.
    @param toFormat The desired destination format for compression 
        (GL_COMPRESSED_RGBA_S3TC_DXT[1,3,5]_EXT).
    @param bMakeMIPMaps If nonzero, generates a full mipmap pyramid prior to
        compression.
    
    @see NVHHDDSAlloc()
    @see NVHHDDSAllocData()
*/
NVHHDDSImage *NVCompressToDXT(NVHHDDSImage *srcImage, NvS32 toFormat, NvS32 bMakeMIPMaps);

/** Frees any static temporary data allocated by NVCompressToDXT

    This helper function should be called when you are done processing textures, to tell
    the library it can free any temporary buffers it has kept around in memory.

    @see NVCompressToDXT()
*/
void          NVCompressFlush(void);


#ifdef __cplusplus
}
#endif

#endif // _NVHHDDS_H_INCLUDED
