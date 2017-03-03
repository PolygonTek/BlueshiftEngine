//----------------------------------------------------------------------------------
// File:        jni/nv_file/nv_file.c
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

#include "nv_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "nv_apk_file.h"

typedef struct
{
    int     type;
    void   *handle;
} NvFileHandle;

enum
{
    APK_FILE,
    STD_FILE
};

void  NvFInit(AAssetManager* assetManager)
{
    NvAPKInit(assetManager);
}

NvFile* NvFOpen(char const* path)
{
    NvFileHandle *handle = NULL;

    // First, try the given path with no mods...
    FILE *fp = fopen(path, "rb");

    // No luck...  Try prepending /data...
    if (!fp)
    {
        char buf[512];
        sprintf(buf, "/data/%s", path); //TODO: use storage card path in the future?
        fp = fopen(buf, "rb");
    }

    if (fp)
    {
        handle = (NvFileHandle*) malloc(sizeof(NvFileHandle));
        handle->type = STD_FILE;
        handle->handle = fp;
    }
    else
    {
        NvAPKFile* apk = NvAPKOpen(path);
        if (apk)
        {
            handle = (NvFileHandle*) malloc(sizeof(NvFileHandle));
            handle->type = APK_FILE;
            handle->handle = apk;
        }
    }
    return (NvFile*) handle;
}

NvFile* _NvFOpen(char const* path, char const* mode)
{
	NvFileHandle *handle = NULL;

	// First, try the given path with no mods...
	FILE *fp = fopen(path, mode);

	if (fp)
	{
		handle = (NvFileHandle*)malloc(sizeof(NvFileHandle));
		handle->type = STD_FILE;
		handle->handle = fp;
	}
	return (NvFile*)handle;
}

void NvFClose(NvFile* file)
{
    NvFileHandle *handle = (NvFileHandle*) file;
    if (!handle)
        return;

    if (handle->type != STD_FILE)
        NvAPKClose(handle->handle);
    else
        fclose(handle->handle);
    free(handle);
}

void NvFChdir(const char* dir)
{
    (void)chdir(dir);
}

int NvFGetc(NvFile *stream)
{
    NvFileHandle *handle = (NvFileHandle*) stream;

    if (!handle)
        return -1;

    if (handle->type != STD_FILE)
        return NvAPKGetc(handle->handle);
    else
        return fgetc(handle->handle);
}

char* NvFGets(char* s, int size, NvFile* stream)
{
    NvFileHandle *handle = (NvFileHandle*) stream;

    if (!handle)
        return NULL;

    if (handle->type != STD_FILE)
        return NvAPKGets(s, size, handle->handle);
    else
        return fgets(s, size, handle->handle);
}

size_t NvFSize(NvFile* stream)
{
    NvFileHandle *handle = (NvFileHandle*) stream;

    if (!handle)
        return 0;

    if (handle->type != STD_FILE)
    {
        return NvAPKSize(handle->handle);
    }
    else
    {
        int pos = ftell(handle->handle);
        int size = 0;
        fseek(handle->handle, 0, SEEK_END);
        size = ftell(handle->handle);
        fseek(handle->handle, pos, SEEK_SET);
        return size;
    }
}

size_t _NvFSize(NvFile* stream)
{
	NvFileHandle *handle = (NvFileHandle*)stream;

	if (!handle)
		return 0;

	if (handle->type != STD_FILE)
	{
		return NvAPKSize(handle->handle);
	}
	else
	{
		struct stat fileInfo;
		fileInfo.st_size = -1;
		if (fstat(fileno(handle->handle), &fileInfo) < 0)
			return -1;

		return fileInfo.st_size;
	}
}


long NvFSeek(NvFile* stream, long offset, int type)
{
    NvFileHandle *handle = (NvFileHandle*) stream;

    if (!handle)
        return -1;

    if (handle->type != STD_FILE)
        return NvAPKSeek(handle->handle, offset, type);
    else
        return fseek(handle->handle, offset, type);
}

long NvFTell(NvFile* stream)
{
    NvFileHandle *handle = (NvFileHandle*) stream;

    if (!handle)
        return -1;

    if (handle->type != STD_FILE)
        return NvAPKTell(handle->handle);
    else
        return ftell(handle->handle);
}

size_t NvFRead(void* ptr, size_t size, size_t nmemb, NvFile* stream)
{
    NvFileHandle *handle = (NvFileHandle*) stream;

    if (!handle)
        return 0;

    if (handle->type != STD_FILE)
        return NvAPKRead(ptr, size, nmemb, handle->handle);
    else
        return fread(ptr, size, nmemb, handle->handle);
}

size_t _NvFWrite(void* ptr, size_t size, size_t nmemb, NvFile* stream)
{
	NvFileHandle *handle = (NvFileHandle*)stream;

	if (!handle)
		return 0;

	if (handle->type != STD_FILE)
		return -1;
	else
		return fwrite(ptr, size, nmemb, handle->handle);
}

int NvFEOF(NvFile *stream)
{
    NvFileHandle *handle = (NvFileHandle*) stream;

    if (!handle)
        return 0;

    if (handle->type != STD_FILE)
        return NvAPKEOF(handle->handle);
    else
        return feof(((FILE*)handle->handle));
}


int _NvFFlush(NvFile* stream)
{
	NvFileHandle *handle = (NvFileHandle*)stream;

	if (!handle)
		return 0;

	if (handle->type != STD_FILE)
		return -1;
	else
		return fflush(handle->handle);
}
