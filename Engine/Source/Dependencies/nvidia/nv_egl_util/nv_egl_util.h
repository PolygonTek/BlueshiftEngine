//----------------------------------------------------------------------------------
// File:        jni/nv_egl_util/nv_egl_util.h
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

#ifndef _NV_EGL_UTIL_H
#define _NV_EGL_UTIL_H

#include <stdlib.h>
#include <time.h>
#include <android/native_window.h>
#include <EGL/eglplatform.h>
#include <EGL/egl.h>

#ifndef EGL_NV_system_time
#define EGL_NV_system_time 1
typedef khronos_uint64_t EGLuint64NV;
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLuint64NV EGLAPIENTRY eglGetSystemTimeFrequencyNV(void);
EGLAPI EGLuint64NV EGLAPIENTRY eglGetSystemTimeNV(void);
#endif
typedef EGLuint64NV (EGLAPIENTRYP PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC)(void);
typedef EGLuint64NV (EGLAPIENTRYP PFNEGLGETSYSTEMTIMENVPROC)(void);
#endif


class NvEGLUtil {
public:
	// typedef for a function pointer that can be used by the app to find their own
	// EGL configuration.  This function, if supplied to construction, must return the
	// app's desired EGL config for the on-screen context
	typedef bool (*ConfigChooser)(EGLDisplay disp, EGLConfig& bestConfig);

	// The default config chooser function, which returns a 565 RGB config with a 16 bit depth buffer
	static bool defaultEGLChooser(EGLDisplay disp, EGLConfig& bestConfig);

	// returns a wrapper with a valid config on success.
	// no surface is created yet
	// A context is created but is not bound
	static NvEGLUtil* create(ConfigChooser chooser = &defaultEGLChooser);
	
	// Shuts down any active EGL resources and deletes
	~NvEGLUtil();

	// valid only when getWindow()!=NULL
	int getWidth() { return m_width; }
	int getHeight() { return m_height; }
	int getFormat() { return m_format; }

	// these map to egl query if surface exists,
	// or m_width/height if not.
	int getSurfaceWidth();
	int getSurfaceHeight();
	
	// This clock will use the NV time extension if available, allowing the
	// time to be slowed, accelerated or stopped.  This function may return
	// the same time from multiple calls, specifically if PerfHUD ES is active
	// Units are miliseconds
	long getSystemTime();

	// used to set or update the native window given to us by Android
	// Must be called with NULL when the window is destroyed
	bool setWindow(ANativeWindow* window);

	// Returns true if the window size has changed since the last call to this
	// function, and indicates that the app needs to re-set its viewport, etc
	bool checkWindowResized();

	// Create the rendering surface.  Will fail if no valid window has been
	// passed using setWindow
	bool createSurface();

	// Unbinds and destroys the rendering surface
	bool destroySurface();

	// Binds the rendering context and on-screen surface; 
	// fails if there is no rendering surface available
	bool bind();

	// Unbinds the current rendering context and surface
	bool unbind();

	// Swaps the current on-screen surface.  Fails if the surface and context
	// are not already bounds
	bool swap();

	// Returns true if there is a bound surface and context.
	// If allocateIfNeeded is true, then the function will 
	// attempt to allocate the rendering surface and bind the
	// surface and context if possible.  Otherwise, the app
	// takes no actions and only returns whether or not the
	// system was already ready for rendering
	bool isReadyToRender(bool allocateIfNeeded = false);

	// Returns true if the rendering surface and context are bound
	// false otherwise
	bool isBound() { return m_status == NV_IS_BOUND; }

	// Returns true if the app has an allocated surface (bound or not)
	// returns false otherwise
	bool hasSurface() { return m_status >= NV_HAS_SURFACE; }

	// Accessors for the low-level EGL/Android objects
	EGLDisplay getDisplay() { return m_display; }
	EGLConfig getConfig() { return m_config; }
	EGLContext getContext() { return m_context; }
	EGLSurface getSurface() { return m_surface; }
	ANativeWindow* getWindow() { return m_window; }

#if !defined __XAMARIN__
protected:
#endif
	enum {
		// Numbering matters here, as we use >/< for state hierarchy
		NV_UNINITIALIZED = 0,
		NV_INITIALIZED = 1,
		NV_HAS_SURFACE = 2,
		NV_IS_BOUND = 3
	};

	enum {
		// the clock timing mode
		NV_TIME_MODE_INIT,
		NV_TIME_MODE_SYSTEM,
		NV_TIME_MODE_EGL
	};

	NvEGLUtil()
	{ 
		m_display = EGL_NO_DISPLAY;
		m_config = NULL;
		m_context = EGL_NO_CONTEXT;
		m_surface = EGL_NO_SURFACE;
		m_window = NULL;
		m_status = NV_UNINITIALIZED;
		m_width = 0;
		m_height = 0;
		mNewWindow = true;
		m_lastQueriedTime = 0;
		m_accumulatedTime = 0;
		gettimeofday(&m_startTime, 0);
		m_eglGetSystemTimeFrequencyNVProc = NULL;
		m_eglGetSystemTimeNVProc = NULL;
		m_eglSystemTimeFrequency = 0;
		m_nvTimeExtensionQueried = false;
		m_timeMode = NV_TIME_MODE_INIT;
	}


	bool queryNVTimeSupport();

	EGLDisplay m_display;
	EGLConfig m_config;
	EGLContext m_context;
	EGLSurface m_surface;
	EGLint m_format;
	int m_width;
	int m_height;

	ANativeWindow* m_window;
	bool mNewWindow;

	unsigned int m_status;

	long m_lastQueriedTime;
	long m_accumulatedTime;
	PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC m_eglGetSystemTimeFrequencyNVProc;
	PFNEGLGETSYSTEMTIMENVPROC m_eglGetSystemTimeNVProc;
	EGLuint64NV m_eglSystemTimeFrequency;
	struct timeval m_startTime;
	bool m_nvTimeExtensionQueried;
	int m_timeMode;
};



#endif

