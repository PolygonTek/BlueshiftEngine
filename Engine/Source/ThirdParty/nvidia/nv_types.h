//----------------------------------------------------------------------------------
// File:        jni/nv_types.h
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
#ifndef __INCLUDED_NV_TYPES_H
#define __INCLUDED_NV_TYPES_H

typedef unsigned char      NvU8;  // 0 to 255
typedef unsigned short     NvU16; // 0 to 65535
typedef unsigned int       NvU32; // 0 to 4294967295
typedef unsigned long long NvU64; // 0 to 18446744073709551615
typedef signed char        NvS8;  // -128 to 127
typedef signed short       NvS16; // -32768 to 32767
typedef signed int         NvS32; // -2147483648 to 2147483647
typedef signed long long   NvS64; // 2^-63 to 2^63-1

// Explicitly sized floats
typedef float              NvF32; // IEEE Single Precision (S1E8M23)
typedef double             NvF64; // IEEE Double Precision (S1E11M52)

// Boolean type
#define NV_FALSE    0
#define NV_TRUE     1
typedef NvU8 NvBool;

// Result of sizeof operator 
typedef unsigned long NvSize;

// Type large enough to hold a relative file offset
typedef long NvOffset;

// Base NULL type.
#define NV_NULL 0

// Error related
typedef enum {
    NvError_Success = 0,
    NvError_NotSupported,
    NvError_NotInitialized,
    NvError_BadParameter,
    NvError_InsufficientMemory,
    NvError_NoEntries,
    NvError_UnknownError,
} NvError;

#if 1 //defined(_DEBUG) // !!!!TBD TODO
#define NvAssert(c)
#else
#define NvAssert(c) ((void)( (c) ? 0 : (NvHandleAssertion(#c, __FILE__, __LINE__), 0)))
#endif


// Other standardized typedefs
typedef NvU64               NvUST; // U64 unadjusted system time value


#endif
