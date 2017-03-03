//----------------------------------------------------------------------------------
// File:        jni/nv_file/nv_apk_file.c
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

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <jni.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <stdio.h>

// The header is the same as the other implementation, so we can support
// both seamlessly
#include "nv_apk_file.h"

static AAssetManager* s_manager;

void NvAPKInit(AAssetManager* assetManager)
{
    s_manager = assetManager;
}

NvAPKFile* NvAPKOpen(char const* path)
{
    return (NvAPKFile *)AAssetManager_open(s_manager, path, AASSET_MODE_UNKNOWN);
}

void NvAPKClose(NvAPKFile* file)
{
    AAsset_close((AAsset*)file);
}

int NvAPKGetc(NvAPKFile *stream)
{
    char buff;
    AAsset_read((AAsset*)stream, &buff, 1);

    return buff;
}

char* NvAPKGets(char* s, int size, NvAPKFile* stream)
{
    int i;
    char *d=s;
    for(i = 0; (size > 1) && (!NvAPKEOF(stream)); i++, size--, d++)
    {
        NvAPKRead(d,1,1,stream);
        if(*d==10)
        {
            size=1;
        }
    }
    *d=0;

    return s;
}

size_t NvAPKSize(NvAPKFile* stream)
{
    return AAsset_getLength((AAsset*)stream);
}

long NvAPKSeek(NvAPKFile* stream, long offset, int type)
{
    // Match fseek: 0 on success, -1 on failure
    // (AAsset_seek returns -1 on failure and the new pos on success)
    return (AAsset_seek((AAsset*)stream, offset, type) == -1) ? -1 : 0;
}

long NvAPKTell(NvAPKFile* stream)
{
    return NvAPKSize(stream) - AAsset_getRemainingLength((AAsset*)stream);
}

size_t NvAPKRead(void* ptr, size_t size, size_t nmemb, NvAPKFile* stream)
{
    jint readLength = size*nmemb;

    int avail = NvAPKSize(stream)-NvAPKTell(stream);
    if(readLength>avail)
    {
        readLength = avail;
        nmemb = readLength/size;
    }

    // read returns the # of bytes.  If size > 0, we need to divide by the size
    return AAsset_read((AAsset*)stream, ptr, readLength) / (size ? size : 1);
}

int NvAPKEOF(NvAPKFile *stream)
{
    int rv = (NvAPKTell(stream) >= NvAPKSize(stream)) ? 1 : 0;
    return rv;
}

AAssetManager* _NvAPKManager()
{
	return s_manager;
}