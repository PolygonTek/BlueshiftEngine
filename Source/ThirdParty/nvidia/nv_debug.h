//----------------------------------------------------------------------------------
// File:        jni/nv_debug.h
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
#ifndef __INCLUDED_NV_DEBUG_H
#define __INCLUDED_NV_DEBUG_H

#define CT_ASSERT(tag,cond) \
enum { COMPILE_TIME_ASSERT__ ## tag = 1/(cond) }

#define dimof( x ) ( sizeof(x) / sizeof(x[0]) )
#include <android/log.h>

#define DBG_DETAILED 0

#if 0

	// the detailed prefix can be customised by setting DBG_DETAILED_PREFIX. See
	// below as a reference.
	// NOTE: fmt is the desired format string and must be in the prefix.
	//#ifndef DBG_DETAILED_PREFIX
	//	#define DBG_DETAILED_PREFIX "%s, %s, line %d: " fmt, __FILE__, __FUNCTION__, __LINE__,
	//#endif 
	//#define DEBUG_D_(fmt, args...) 
	//#define DEBUG_D(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, MODULE, (DBG_DETAILED_PREFIX) ## args)

#else

	#ifdef STRINGIFY
		#pragma push_macro("STRINGIFY")
		#undef STRINGIFY
		#define STRINGIFYPUSHED_____
	#endif
	#define STRINGIFY(x) #x

	// debug macro, includes file name function name and line number
	#define TO(x) typeof(x)
	#define DEBUG_D_(file, line, fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, MODULE, file ", %s, line(" STRINGIFY(line) "): " fmt, __FUNCTION__, ## args)
	#define DEBUG_D(fmt, args...) DEBUG_D_( __FILE__ , __LINE__ , fmt, ## args)

	#ifdef STRINGIFYPUSHED_____
		#undef STRINGIFYPUSHED_____
		#pragma pop_macro("STRINGIFY")
	#endif

#endif

// basic debug macro
#define DEBUG_(fmt, args...) (__android_log_print(ANDROID_LOG_DEBUG, MODULE, fmt, ## args))

// Debug macro that can be switched to spew a file name,
// function and line number using DEBUG_DETAILED
#if DBG_DETAILED == 1
	#define DEBUG(fmt, args...) DEBUG_D(fmt, ## args)
#else
	#define DEBUG(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, MODULE, fmt, ## args)
#endif


#endif
