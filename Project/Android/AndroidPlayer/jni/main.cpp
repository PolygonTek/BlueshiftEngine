//----------------------------------------------------------------------------------
// File:        native_basic/jni/main.cpp
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

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl3.h>

#include <android/log.h>
#include <nv_and_util/nv_native_app_glue.h>
#include <nv_egl_util/nv_egl_util.h>

#include "engine.h"

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x00000040
#endif

bool MyEGLChooser(EGLDisplay disp, EGLConfig& bestConfig)
{

	EGLint count = 0;
	if (!eglGetConfigs(disp, NULL, 0, &count))
	{
		LOGW("defaultEGLChooser cannot query count of all configs");
		return false;
	}

	LOGD("Config count = %d", count);

	EGLConfig* configs = new EGLConfig[count];
	if (!eglGetConfigs(disp, configs, count, &count))
	{
		LOGW("defaultEGLChooser cannot query all configs");
		return false;
	}

	int bestMatch = 1 << 30;
	int bestIndex = -1;

	int i;
	for (i = 0; i < count; i++)
	{
		int match = 0;
		EGLint surfaceType = 0;
		EGLint blueBits = 0;
		EGLint greenBits = 0;
		EGLint redBits = 0;
		EGLint alphaBits = 0;
		EGLint depthBits = 0;
		EGLint stencilBits = 0;
		EGLint renderableFlags = 0;

		eglGetConfigAttrib(disp, configs[i], EGL_SURFACE_TYPE, &surfaceType);
		eglGetConfigAttrib(disp, configs[i], EGL_BLUE_SIZE, &blueBits);
		eglGetConfigAttrib(disp, configs[i], EGL_GREEN_SIZE, &greenBits);
		eglGetConfigAttrib(disp, configs[i], EGL_RED_SIZE, &redBits);
		eglGetConfigAttrib(disp, configs[i], EGL_ALPHA_SIZE, &alphaBits);
		eglGetConfigAttrib(disp, configs[i], EGL_DEPTH_SIZE, &depthBits);
		eglGetConfigAttrib(disp, configs[i], EGL_STENCIL_SIZE, &stencilBits);
		eglGetConfigAttrib(disp, configs[i], EGL_RENDERABLE_TYPE, &renderableFlags);
		LOGD("Config[%d]: R%dG%dB%dA%d D%dS%d Type=%04x Render=%04x",
			i, redBits, greenBits, blueBits, alphaBits, depthBits, stencilBits, surfaceType, renderableFlags);

		if ((surfaceType & EGL_WINDOW_BIT) == 0)
			continue;
		if ((renderableFlags & EGL_OPENGL_ES3_BIT) == 0)
			continue;
		if ((depthBits < 24) || (stencilBits < 8))
			continue;
		if ((redBits < 8) || (greenBits < 8) || (blueBits < 8) || (alphaBits < 8))
			continue;

		int penalty = depthBits - 16;
		match += penalty * penalty;
		penalty = redBits - 8;
		match += penalty * penalty;
		penalty = greenBits - 8;
		match += penalty * penalty;
		penalty = blueBits - 8;
		match += penalty * penalty;
		penalty = alphaBits - 8;
		match += penalty * penalty;
		penalty = depthBits - 24;
		match += penalty * penalty;
		penalty = stencilBits - 8;
		match += penalty * penalty;
		penalty = surfaceType | EGL_PIXMAP_BIT;
		match += penalty * penalty;

		if ((match < bestMatch) || (bestIndex == -1))
		{
			bestMatch = match;
			bestIndex = i;
//			LOGD("Config[%d] is the new best config", i, configs[i]);
		}
	}

	if (bestIndex < 0)
	{
		delete[] configs;
		return false;
	}

	bestConfig = configs[bestIndex];
	LOGD("Config[%d] is best", bestIndex, configs[bestIndex]);
	delete[] configs;

	return true;
}

void android_main(struct android_app* app)
{
    // Make sure glue isn't stripped.
    app_dummy();

	NvEGLUtil* egl = NvEGLUtil::create(&MyEGLChooser);
    if (!egl) 
    {
        // if we have a basic EGL failure, we need to exit immediately; nothing else we can do
        nv_app_force_quit_no_cleanup(app);
        return;
    }

    Engine* engine = new Engine(*egl, app);

	long lastTime = egl->getSystemTime();

    // loop waiting for stuff to do.

	while (nv_app_status_running(app))
    {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        // If not rendering, we will block 250ms waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident = ALooper_pollAll(((nv_app_status_focused(app) &&
										  engine->isGameplayMode()) ? 1 : 250),
        								NULL,
        								&events,
        								(void**)&source)) >= 0)
        {
            // Process this event. 
            if (source != NULL)
                source->process(app, source);

            // Check if we are exiting.  If so, dump out
            if (!nv_app_status_running(app))
				break;
        }

		long currentTime = egl->getSystemTime();

		// clamp time - it must not go backwards, and we don't
		// want it to be more than a half second to avoid huge
		// delays causing issues.  Note that we do not clamp to above
		// zero because some tools will give us zero delta
		long deltaTime = currentTime - lastTime;
		if (deltaTime < 0)
			deltaTime = 0;
		else if (deltaTime > 500)
			deltaTime = 500;

		lastTime = currentTime;

		// Update the frame, which optionally updates time and animations
		// and renders
		engine->updateFrame(nv_app_status_interactable(app), deltaTime);
    }

	delete engine;
    delete egl;
}
