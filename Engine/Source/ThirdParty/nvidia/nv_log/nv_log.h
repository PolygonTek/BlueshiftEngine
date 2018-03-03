//----------------------------------------------------------------------------------
// File:        jni/nv_log/nv_log.h
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
//==============================================================================
//    File:        nv_log.h
//
//    Copyright 2008, NVIDIA Corporation.
//    ALL RIGHTS RESERVED
//==============================================================================
#ifndef _NV_LOG_H_
#define _NV_LOG_H_

#include <nv_global.h>

//=============================================================================
/**
 *    @function            NVLog
 *    @brief                Customizable logger function supporting printf style arguments.
 *    @note                The logger is not re-entrant but could be if we remove the
 *                        static variable in NVDefaultLogMessage.
 *
 *    @param inLevel        Severity level of the debug entry ranging from DEBUG(low) to FATAL(high)
 *    @param inTag        Short string describing the subsystem being logged allowing system level filtering
 *    @param inMessage    Simple string describing the log entry
 *    @param inDetails    Longer detailed string providing more information and printf-style formatting
 *    @param ...            Optional variadic arguments formatted into inDetails.
 *
 *    INFO
 *    This opt-in logger is a replacement for OutputDebugString, printf, fprintf,
 *    cout, cerr, etc.  It supports variadic arguments just printf to allow easy
 *    formatting of program states and variables.  It's written in plain ANSI-C
 *    to allow any library or application to use it.
 *
 *    EXAMPLE
 *    Getting started is just a matter of replacing your printf statements with
 *    any of the convenience functions such as NVLogDebug or NVLogWarn such that:
 *    printf( "Warning: found image '%s' with size %d x %d", inNameString, inWidth, inHeight );
 *     simply becomes
 *    NVLogWarn( "Found image '%s' with size %d x %d, inNameString, inWidth, inHeight );
 *
 *    IMPLEMENTATION NOTES
 *    Implementation is done using function pointers impersonating real functions.
 *    This enables the user to redirect the log calls to perform additional or
 *    different duties such as displaying results to a dialog or sending them
 *    across a network pipe.
 *
 *    USAGE:
 *    The most common usage is redirecting the NVLogMessage function pointer and
 *    not the NVLog function pointer.  The NVLogMessage pointer is called after
 *    the variadic parameters have been parsed into the inDetails parameter by
 *    the default NvLog implementation.  The default implementation of NvLog also
 *    does the filtering of log levels such that NVLogMessage is not called if the
 *    current global g_NVLogMask level is not enough for the entry to pass through.
 *
 *    Redirect or chain the NVLogMessage function pointer at the start of your
 *    application to control log entry flow.
 *    
 *    The full NVLog function use these parameters:
 *     inLevel    Severity of the log entry.  Filtered using the global g_NVLogMask field.
 *     inTag        Sub-system name allowing detailed filtering separate from levels.
 *     inMessage    Short description of log entry.
 *     inDetails    Longer description including printf style formatting
 *     ...        Optional variadic parameters parsed into inDetails
 *
 *    Convenience functions use less parameters and hardwire the level and tag:
 *    @see NVLogDebug, NVLogInfo, NVLogWarn, NVLogError, NVLogFatal
 */

//==============================================================================
//    C Linkage
//==============================================================================
#ifdef __cplusplus
extern "C"
{
#endif

#include <nv_types.h>

//==============================================================================
//    Constants
//==============================================================================
typedef enum 
{
    NVLOGLEVEL_DEBUG =        0x01,    ///< Temporary low-level chatty information
    NVLOGLEVEL_TRACE =        0x02,    ///< Permanent low-level chatty information
    NVLOGLEVEL_INFO    =        0x04,    ///< Performance data, checkpoints or successful operation
    NVLOGLEVEL_WARN =        0x08,    ///< An unideal situation but recoverable
    NVLOGLEVEL_ERROR =        0x10,    ///< System could not complete a task
    NVLOGLEVEL_FATAL =        0x20,    ///< Stability of system compromised -shutdown now
    NVLOGLEVEL_ALL =        0xFF,    ///< All levels
} ENVLogLevel;

//==============================================================================
//    Callback Signatures
//==============================================================================
typedef void    (*TNVLogFunction)            ( const char* inMessage, const char* inDetails, ... );
typedef void    (*TNVLogFunctionEx)            ( ENVLogLevel inLevel, const char* inTag, const char* inMessage, const char* inDetails, ... );
typedef char*    (*TNVMessageFunction)        ( ENVLogLevel inLevel, const char* inTag, const char* inMessage, const char* inDetails );
typedef void    (*TNVOutputFunction)        ( const char* inMessage );

//==============================================================================
//    Function Pointers and Fields
//==============================================================================
extern long                        g_NVLogMask;        ///< Type filter level for masking out less relevant entries
extern void*                    g_NVLogFile;        ///< Optional log file

// Core Functions
extern TNVLogFunctionEx            NVLog;                ///< Full entry point to logger including type, tag and printf style formatting
extern TNVMessageFunction        NVLogMessage;        ///< Exit callback after variadic parameters are parsed
extern TNVOutputFunction        NVLogOutput;        ///< Exit that outputs the final string

// Convenience Functions
extern TNVLogFunction            NVLogDebug;            ///< Easy entry point using level = NVLOGLEVEL_DEBUG and tag = "Log"
extern TNVLogFunction            NVLogTrace;            ///< Easy entry point using level = NVLOGLEVEL_TRACE and tag = "Log"
extern TNVLogFunction            NVLogInfo;            ///< Easy entry point using level = NVLOGLEVEL_INFO and tag = "Log"
extern TNVLogFunction            NVLogWarn;            ///< Easy entry point using level = NVLOGLEVEL_WARN and tag = "Log"
extern TNVLogFunction            NVLogError;            ///< Easy entry point using level = NVLOGLEVEL_ERROR and tag = "Log"
extern TNVLogFunction            NVLogFatal;            ///< Easy entry point using level = NVLOGLEVEL_FATAL and tag = "Log"

//==============================================================================
//    Functions
//==============================================================================
// Configuration
void    NVLogEnable( ENVLogLevel inLevel, NvBool inEnable );
void    NVLogFile( const char* inFileName );

// Core Callbacks
void    NVDefaultLog( ENVLogLevel inLevel, const char* inTag, const char* inMessage, const char* inDetails, ... );
char*    NVDefaultLogMessage( ENVLogLevel inLevel, const char* inTag, const char* inMessage, const char* inDetails );
void    NVDefaultLogOutput( const char* inMessage );

// Convenience Callbacks
void    NVDefaultLogDebug( const char* inMessage, const char* inDetails, ... );
void    NVDefaultLogTrace( const char* inMessage, const char* inDetails, ... );
void    NVDefaultLogInfo( const char* inMessage, const char* inDetails, ... );
void    NVDefaultLogWarn( const char* inMessage, const char* inDetails, ... );
void    NVDefaultLogError( const char* inMessage, const char* inDetails, ... );
void    NVDefaultLogFatal( const char* inMessage, const char* inDetails, ... );

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _NV_LOG_H_
