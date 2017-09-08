/* San Angeles Observation OpenGL ES version example
 * Copyright 2009 The Android Open Source Project
 * All rights reserved.
 *
 * This source is free software; you can redistribute it and/or
 * modify it under the terms of EITHER:
 *   (1) The GNU Lesser General Public License as published by the Free
 *       Software Foundation; either version 2.1 of the License, or (at
 *       your option) any later version. The text of the GNU Lesser
 *       General Public License is included with this source in the
 *       file LICENSE-LGPL.txt.
 *   (2) The BSD-style license that is included with this source in
 *       the file LICENSE-BSD.txt.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the files
 * LICENSE-LGPL.txt and LICENSE-BSD.txt for more details.
 */
#include "pch.h"
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>
#include <stdint.h>
#include "importgl.h"
#include "app.h"
#if _ENGINE || 1 
#	include "Precompiled.h"
#	include "RHI/EGLUtil.h"
#	include <android\asset_manager_jni.h>
#endif


typedef   int(*callback1_t) (int a);
typedef   char* (*callback2_t) (char* a);
static   callback1_t my_callback1;
static   callback2_t my_callback2;


/*
jenv->GetJavaVM(&jvm);
(*jvm)->GetEnv(jvm, (void **)&jenv, JNI_VERSION_1_2);

in New Thread
jvm->AttachCurrentThread(&jenv, NULL);

*/


extern "C" void
SetAssetManager(JNIEnv*  env, jobject asset, char *path)
{
#if _ENGINE
	static jobject object;
	if (object) {
		env->DeleteGlobalRef(object);
	}
	if (asset) {
		object = env->NewGlobalRef(asset);
		AAssetManager *mgr = AAssetManager_fromJava(env, object);
		BE1::PlatformFile::SetManager(mgr);
	}
	else
		BE1::PlatformFile::SetManager(0);

	BE1::PlatformFile::SetExecutablePath(path);
#endif
}

extern "C" void RegisterCallback1(callback1_t cb)
{
	my_callback1 = cb;
}

extern "C" void	RegisterCallback2(callback2_t cb)
{
	my_callback2 = cb;
}

extern "C" int InvokeManagedCode1(int a)
{
	if (my_callback1 == NULL) {
		return 0;
	}
	return (*my_callback1) (a);
}

extern "C" char * InvokeManagedCode2(char * a)
{
	if (my_callback2 == NULL) {
		return 0;
	}
	return (*my_callback2) (a);
}







int   gAppAlive = 1;

static int  sWindowWidth = 320;
static int  sWindowHeight = 480;
static int  sDemoStopped = 0;
static long sTimeOffset = 0;
static int  sTimeOffsetInit = 0;
static long sTimeStopped = 0;

static long
	_getTime(void)
{
	struct timeval  now;

	gettimeofday(&now, NULL);
	return (long)(now.tv_sec * 1000 + now.tv_usec / 1000);
}

/* Call to initialize the graphics state */
extern "C" void
	Java_com_example_XamarinPlayer_DemoRenderer_nativeInit(JNIEnv*  env)
{
	int j = InvokeManagedCode1(10);
	char *b = InvokeManagedCode2("abc");

//#if _SAMPLE
//	importGLInit();
//#endif
//	appInit();

	gAppAlive = 1;
}

extern "C" void
	Java_com_example_XamarinPlayer_DemoRenderer_nativeResize(JNIEnv*  env, jobject  thiz, jint w, jint h)
{
	sWindowWidth = w;
	sWindowHeight = h;
	__android_log_print(ANDROID_LOG_INFO, "XamarinPlayer", "resize w=%d h=%d", w, h);

#if _SAMPLE
	importGLInit();
	appInit();
#endif
#if _ENGINE
	extern EGLUtil mEgl;
	appDeinit();
	mEgl.m_width = mEgl.m_windowWidth = sWindowWidth;
	mEgl.m_height = mEgl.m_windowHeight = sWindowHeight;
	appInit();
#endif
}

/* Call to finalize the graphics state */
extern "C" void
	Java_com_example_XamarinPlayer_DemoRenderer_nativeDone(JNIEnv*  env)
{
	appDeinit();
#if _SAMPLE
	importGLDeinit();
#endif
}

/* This is called to indicate to the render loop that it should
	* stop as soon as possible.
	*/

void _pause()
{
	/* we paused the animation, so store the current
		* time in sTimeStopped for future nativeRender calls */
	sDemoStopped = 1;
	sTimeStopped = _getTime();
}

void _resume()
{
	/* we resumed the animation, so adjust the time offset
		* to take care of the pause interval. */
	sDemoStopped = 0;
	sTimeOffset -= _getTime() - sTimeStopped;
}


extern "C" void
	Java_com_example_XamarinPlayer_DemoGLSurfaceView_nativeTogglePauseResume(JNIEnv*  env)
{
	sDemoStopped = !sDemoStopped;
	if (sDemoStopped)
		_pause();
	else
		_resume();
}

extern "C" void
	Java_com_example_XamarinPlayer_DemoGLSurfaceView_nativePause(JNIEnv*  env)
{
	_pause();
}

extern "C" void
	Java_com_example_XamarinPlayer_DemoGLSurfaceView_nativeResume(JNIEnv*  env)
{
	_resume();
}

/* Call to render the next GL frame */
extern "C" void
	Java_com_example_XamarinPlayer_DemoRenderer_nativeRender(JNIEnv*  env)
{
	long   curTime;

	/* NOTE: if sDemoStopped is TRUE, then we re-render the same frame
		*       on each iteration.
		*/
	if (sDemoStopped) {
		curTime = sTimeStopped + sTimeOffset;
	}
	else {
		curTime = _getTime() + sTimeOffset;
		if (sTimeOffsetInit == 0) {
			sTimeOffsetInit = 1;
			sTimeOffset = -curTime;
			curTime = 0;
		}
	}

	//__android_log_print(ANDROID_LOG_INFO, "XamarinPlayer", "curTime=%ld", curTime);

	appRender(curTime, sWindowWidth, sWindowHeight);
}


