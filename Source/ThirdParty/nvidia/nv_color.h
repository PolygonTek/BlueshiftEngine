//----------------------------------------------------------------------------------
// File:        jni/nv_color.h
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

#ifndef _NV_COLOR_H
#define _NV_COLOR_H

//#include "../nv_global.h"

/** @file nv_color.h
    Simple abstraction for RGBA colors as parameters to various libraries/functions.
*/

/** Base type for a single color channel (8-bit unsigned). */
typedef NvU8 NvPCColor8;

//#define NV_COLOR_BREAKOLD
#ifdef NV_COLOR_BREAKOLD // uses a struct to break backwards-compat and accidental uses

typedef struct
{
    NvPCColor8 r;
    NvPCColor8 g;
    NvPCColor8 b;
    NvPCColor8 a;
} NvPackedColor;


static const NvPackedColor gnvpcwhite = {255,255,255,255};
static const NvPackedColor gnvpcblack = {0,0,0,255};

#define NV_PACKED_COLOR(r, g, b, a)     {(NvPCColor8)(r),(NvPCColor8)(g),(NvPCColor8)(b),(NvPCColor8)(a)}

#define NV_PC_PREDEF_WHITE    gnvpcwhite
#define NV_PC_PREDEF_BLACK    gnvpcblack

#define NV_PC_RED(c)        ( c.r )
#define NV_PC_GREEN(c)      ( c.g )
#define NV_PC_BLUE(c)       ( c.b )
#define NV_PC_ALPHA(c)      ( c.a )

#define NV_PC_PACK_UINT(c)  ( *(unsigned int *)(&(c)) )

#define NV_PC_EQUAL(x,y)      ( (x).r==(y).r && (x).g==(y).g && (x).b==(y).b && (x).a==(y).a )

#else /* code that doesn't break old pass-as-uint stuff */

/** Main type declaration for a packed 4-color construct. */
typedef NvU32 NvPackedColor;

/** A macro to build a packed color, passing in RGBA as four 8-bit integer values. */
#define NV_PACKED_COLOR(r,g,b,a)    ((NvPackedColor)( ( (((int)(a))&0xFF)<<24 ) | ( (((int)(b))&0xFF)<<16 ) | ( (((int)(g))&0xFF)<<8 ) | ( (((int)(r))&0xFF) ) ))

/** A predefined constant for WHITE. */
#define NV_PC_PREDEF_WHITE    NV_PACKED_COLOR(0xFF, 0xFF, 0xFF, 0xFF)
/** A predefined constant for BLACK. */
#define NV_PC_PREDEF_BLACK    NV_PACKED_COLOR(0x00, 0x00, 0x00, 0xFF)

/** A macro for 'extracting' the red value from an NvPackedColor. */
#define NV_PC_RED(c)        ( ( c>>0 ) & 0xff )
/** A macro for 'extracting' the green value from an NvPackedColor. */
#define NV_PC_GREEN(c)      ( ( c>>8 ) & 0xff )
/** A macro for 'extracting' the blue value from an NvPackedColor. */
#define NV_PC_BLUE(c)       ( ( c>>16 ) & 0xff )
/** A macro for 'extracting' the alpha value from an NvPackedColor. */
#define NV_PC_ALPHA(c)      ( ( c>>24 ) & 0xff )

/** A macro requesting the packed color repacked into a 32-bit unsigned int.
    This is a no-op for the color-as-uint approach.
*/
#define NV_PC_PACK_UINT(c)  (c)
/** A macro for testing the equality of two NvPackedColors. */
#define NV_PC_EQUAL(x,y)    (x==y)
#endif

/** A macro for mapping a single packed color channel into its floating point [0,1] rep. */
#define NV_PC_TO_FLOAT(c)   ( c / 255.0f )

#endif /*_NV_COLOR_H*/
