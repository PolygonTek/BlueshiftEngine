//----------------------------------------------------------------------------------
// File:        jni/nv_file/nv_apk_file.h
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
#ifndef __INCLUDED_NV_APKAM_FILE_H
#define __INCLUDED_NV_APKAM_FILE_H

#include <android/asset_manager.h>

#if defined(__cplusplus) 
extern "C"
{
#endif

typedef void NvAPKFile;


void        NvAPKInit(AAssetManager* assetManager);

NvAPKFile*  NvAPKOpen(char const* path);
void        NvAPKClose(NvAPKFile* file);
int         NvAPKGetc(NvAPKFile *stream);
char*       NvAPKGets(char* s, int size, NvAPKFile* stream);
size_t      NvAPKSize(NvAPKFile* stream);
long        NvAPKSeek(NvAPKFile* stream, long offset, int type);
long        NvAPKTell(NvAPKFile* stream);
size_t      NvAPKRead(void* ptr, size_t size, size_t nmemb, NvAPKFile* stream);
int         NvAPKEOF(NvAPKFile *stream);

AAssetManager* _NvAPKManager();


#if defined(__cplusplus)
}
#endif

#endif
