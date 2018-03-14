//----------------------------------------------------------------------------------
// File:        jni/nv_file/nv_file.h
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

#ifndef __INCLUDED_NV_FILE_H
#define __INCLUDED_NV_FILE_H

#if defined(__cplusplus)
extern "C"
{
#endif

/** @file nv_file.h
  The NvF(ile) library is an abstraction library that makes it possible to
  automatically find files either in Android's /data tree or in the APK
  itself.  The library provides a unified FILE*-like interface to both of
  the file types.  This makes it possible for native applications to migrate
  data between the APK and the /data filesystem without changing application code
*/
#include <stdio.h>
#include <android/asset_manager.h>

/**
 An opaque handle to a file, either in the APK or in the filesystem
 */
typedef void NvFile;

/**
  Initializes the library.  Calls NvAPKInit() internally.
  @see nv_thread.h
  */
void        NvFInit(AAssetManager* assetManager);

/**
  A wrapper similar to fopen.  Only provides read-access to files, since the
  file returned may be in the read-only APK.  Can be called from any 
  JNI-connected thread.
  @param path The path to the file.  This path is searched within /data and 
  within the APK
  @return A pointer to an open file on success, NULL on failure
  */
NvFile*     NvFOpen(char const* path);

/**
  A wrapper similar to fclose.  Can be called from any JNI-connected thread.
  @param file A pointer to an open file opened via NvFOpen()
  */
void        NvFClose(NvFile* file);

/**
  A wrapper similar to chdir.  Can be called from any thread.
  @param dir String path to be made current
  */
void        NvFChdir(const char* dir);


/**
  A wrapper similar to fgetc.  Can be called from any JNI-connected thread.
  @param stream A pointer to an open file opened via NvFOpen()
  @return The character read from the file
  */
int         NvFGetc(NvFile *stream);


/**
  A wrapper similar to fgets.  Can be called from any JNI-connected thread.
  @param s A char buffer to receive the string
  @param size The size of the buffer pointed to by s
  @param stream A pointer to an open file opened via NvFOpen()
  @return A pointer to s on success or NULL on failure
  */
char*       NvFGets(char* s, int size, NvFile* stream);

/**
  Gets the size of the file in bytes.  Can be called from any JNI-connected thread.
  @param stream A pointer to an open file opened via NvFOpen()
  @return The size of the file in bytes
  */
size_t      NvFSize(NvFile* stream);
size_t      _NvFSize(NvFile* stream);

/**
  A wrapper equivalent to fseek.  Can be called from any JNI-connected thread.
  @param stream A pointer to an open file opened via NvFOpen()
  @param offset The offset from the specified base
  @param type The offset base; same as calls to fseek
  @return Zero on success, nonzero on failure
  */
long        NvFSeek(NvFile* stream, long offset, int type);

/**
  Gets the current file pointer offset.  Can be called from any JNI-connected thread.
  @param stream A pointer to an open file opened via NvFOpen()
  @return The offset of the file pointer in the file in bytes
  */
long        NvFTell(NvFile* stream);

/**
  A wrapper similar to fread.  Can be called from any JNI-connected thread.
  @param ptr A buffer of size size into which data will be read
  @param size size of element to be read
  @param nmemb count of elements to be read
  @param stream A pointer to an open file opened via NvFOpen()
  @return The number of elements read
  */
size_t      NvFRead(void* ptr, size_t size, size_t nmemb, NvFile* stream);

/**
  A wrapper similar to feof.  Can be called from any JNI-connected thread.
  @param stream A pointer to an open file opened via NvFOpen()
  @return Nonzero on EOF, zero otherwise
  */
int         NvFEOF(NvFile *stream);


NvFile* 	_NvFOpen(char const* path, char const *mode);
size_t      _NvFWrite(void* ptr, size_t size, size_t nmemb, NvFile* stream);
int			_NvFFlush(NvFile *stream);

#if defined(__cplusplus)
}
#endif


#endif
