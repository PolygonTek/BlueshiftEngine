//----------------------------------------------------------------------------------
// File:        jni/nv_thread/nv_thread.h
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

#ifndef __INCLUDED_NV_THREAD_H
#define __INCLUDED_NV_THREAD_H

#include <jni.h>
#include <pthread.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C"
{
#endif

/** @file nv_thread.h
  The NVThread library makes it easy to create native threads that can acess
  JNI objects.  By default, pthreads created in the Android NDK are NOT connected
  to the JVM and JNI calls will fail.  This library wraps thread creation in
  such a way that pthreads created using it will connect to and disconnect from
  the JVM as appropriate.  Applications creating all of their threads with these
  interfaces can use the provided NVThreadGetCurrentJNIEnv() function to
  get the current thread's JNI context at any time.

  Note that native-created threads still have JNI limitations over threads
  that are calls down to native from Java.  The JNI function FindClass will
  NOT find application-specific classes when called from native threads.
  Native code that needs to call FindClass and record the indices of Java
  class members for later access must call FindClass and Get*FieldID/Get*MethodID
  in threads calling from Java, such as JNI_OnLoad
 */

/**
  Initializes the nv_thread system by connecting it to the JVM.  This
  function must be called as early as possible in the native code's
  JNI_OnLoad function, so that the thread system is prepared for any
  JNI-dependent library initialization calls.  
  @param vm The VM pointer - should be the JavaVM pointer sent to JNI_OnLoad.
  */
void NVThreadInit(JavaVM* vm);

/**
  Retrieves the JNIEnv object associated with the current thread, allowing
  any thread that was creating with NVThreadSpawnJNIThread() to access the
  JNI at will.  This JNIEnv is NOT usable across multiple calls or threads
  The function should be called in each function that requires a JNIEnv
  @return The current thread's JNIEnv, or NULL if the thread was not created
  by NVThreadSpawnJNIThread
  @see NVThreadSpawnJNIThread
  */
JNIEnv* NVThreadGetCurrentJNIEnv();

/**
  Spwans a new native thread that is registered for use with JNI.  Threads
  created with this function will have access to JNI data via the JNIEnv
  available from NVThreadGetCurrentJNIEnv().
  @param thread is the same as in pthread_create
  @param attr is the same as in pthread_create
  @param start_routine is the same as in pthread_create
  @param arg is the same as in pthread_create
  @return 0 on success, -1 on failure
  @see NVThreadGetCurrentJNIEnv
*/
int NVThreadSpawnJNIThread(pthread_t *thread, pthread_attr_t const * attr,
    void *(*start_routine)(void *), void * arg);

/**
  Sleeps the current thread for the specified number of milliseconds
  @param millisec Sleep time in ms
  @return 0 on success, -1 on failure
*/
int NVThreadSleep(unsigned long millisec);

#if defined(__cplusplus)
}
#endif

#endif
