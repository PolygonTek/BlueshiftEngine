//----------------------------------------------------------------------------------
// File:        jni/nv_glesutil/nv_images.h
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
#ifndef _NV_IMAGES_H
#define _NV_IMAGES_H

#include <nv_global.h>

#include <nv_hhdds/nv_hhdds.h>
#include <GLES3/gl3.h>

GLuint NvCreateTextureFromDDSEx(const char* filename, NvBool flipVertical,
                                NvBool useMipmaps, NvS32* width, NvS32* height, NvBool* alpha, NvBool *isCubeMap);

#define NvCreateTextureFromDDS(a,b,c) NvCreateTextureFromDDSEx(a,b,c,0,0,0,0)

GLuint NvCreateTextureFromHHDDSImage(const NVHHDDSImage& image,
                                NvBool useMipmaps, NvS32* width, NvS32* height, NvBool* alpha, NvBool *isCubeMap);

#endif

