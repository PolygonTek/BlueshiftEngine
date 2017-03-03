//----------------------------------------------------------------------------------
// File:        jni/nv_egl_util/nv_egl_util.cpp
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

#include "nv_egl_util.h"
#include <android/log.h>
#include <unistd.h>

#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG,  \
											 "NvEGLUtil", \
											 __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,  \
											 "NvEGLUtil", \
											 __VA_ARGS__))

#if 1
#define EGL_STATUS_LOG(str) \
	LOGD("Success: %s (%s:%d)", str, __FUNCTION__, __LINE__)

#define EGL_ERROR_LOG(str) \
		LOGE("Failure: %s, error = 0x%08x (%s:%d)", str, eglGetError(), __FUNCTION__, __LINE__)
#else
#define EGL_STATUS_LOG(str)
#define EGL_ERROR_LOG(str)
#endif


bool NvEGLUtil::defaultEGLChooser(EGLDisplay disp, EGLConfig& bestConfig)
{

	EGLint count = 0;
	if (!eglGetConfigs(disp, NULL, 0, &count))
	{
		EGL_ERROR_LOG("defaultEGLChooser cannot query count of all configs");
		return false;
	}

	LOGD("Config count = %d", count);

	EGLConfig* configs = new EGLConfig[count];
	if (!eglGetConfigs(disp, configs, count, &count))
	{
		EGL_ERROR_LOG("defaultEGLChooser cannot query all configs");
		return false;
	}

	int bestMatch = 1<<30;
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
		if ((renderableFlags & EGL_OPENGL_ES2_BIT) == 0)
			continue;
		if (depthBits < 16)
			continue;
		if ((redBits < 5) || (greenBits < 6) || (blueBits < 5))
			continue;

		int penalty = depthBits - 16;
		match += penalty * penalty;
		penalty = redBits - 5;
		match += penalty * penalty;
		penalty = greenBits - 6;
		match += penalty * penalty;
		penalty = blueBits - 5;
		match += penalty * penalty;
		penalty = alphaBits;
		match += penalty * penalty;
		penalty = stencilBits;
		match += penalty * penalty;

		if ((match < bestMatch) || (bestIndex == -1))
		{
			bestMatch = match;
			bestIndex = i;
			LOGD("Config[%d] is the new best config", i, configs[i]);
		}
	}

	if (bestIndex < 0)
	{
		delete[] configs;
		return false;
	}

	bestConfig = configs[bestIndex];
	delete[] configs;

	return true; 
}

NvEGLUtil* NvEGLUtil::create(ConfigChooser chooser)
{
	NvEGLUtil* thiz = new NvEGLUtil;

    thiz->m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (thiz->m_display != EGL_NO_DISPLAY)
	{
		EGL_STATUS_LOG("eglGetDisplay");
	}
	else
	{
		EGL_ERROR_LOG("eglGetDisplay");
		delete thiz;
		return NULL;
	}

    if (eglInitialize(thiz->m_display, 0, 0))
	{
		EGL_STATUS_LOG("eglInitialize");
	}
	else
	{
		EGL_ERROR_LOG("eglInitialize");
		delete thiz;
		return NULL;
	}

	if (chooser(thiz->m_display, thiz->m_config))
	{
		EGL_STATUS_LOG("Config chooser");
	}
	else
	{
		EGL_ERROR_LOG("Config chooser");
		delete thiz;
		return NULL;
	}

    if (eglGetConfigAttrib(thiz->m_display, thiz->m_config, EGL_NATIVE_VISUAL_ID, 
		&(thiz->m_format)))
	{
		EGL_STATUS_LOG("eglGetConfigAttrib");
	}
	else
	{
		EGL_ERROR_LOG("eglGetConfigAttrib");
		delete thiz;
		return NULL;
	}

	EGLint contextAttrs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	
    if (thiz->m_context = eglCreateContext(thiz->m_display, thiz->m_config, NULL, contextAttrs))
	{
		EGL_STATUS_LOG("eglCreateContext");
	}
	else
	{
		EGL_ERROR_LOG("eglCreateContext");
		delete thiz;
		return NULL;
	}

	thiz->m_status = NV_INITIALIZED;

	return thiz;
}

NvEGLUtil::~NvEGLUtil()
{
	LOGD("**** Releasing EGL Manager object and all EGL Resources");
	if (eglDestroyContext(m_display, m_context))
	{
		EGL_STATUS_LOG("eglDestroyContext");
	}
	else
	{
		EGL_ERROR_LOG("eglDestroyContext");
	}

	if (eglTerminate(m_display))
	{
		EGL_STATUS_LOG("eglTerminate");
	}
	else
	{
		EGL_ERROR_LOG("eglTerminate");
	}
}

bool NvEGLUtil::setWindow(ANativeWindow* window)
{
	if (window != m_window)
	{
		LOGD("**** Window has changed!");
		destroySurface();
	}

	m_window = window;

	m_width = m_window ? ANativeWindow_getWidth(m_window) : 0;
	m_height = m_window ? ANativeWindow_getHeight(m_window) : 0;

	return true;
}

bool NvEGLUtil::checkWindowResized()
{
	if (!m_window)
		return false;

	int w = ANativeWindow_getWidth(m_window);
	int h = ANativeWindow_getHeight(m_window);
	if (m_width != w || m_height != h || mNewWindow)
	{
		m_width = w;
		m_height = h;
		mNewWindow = false;
		return true;
	}

	return false;
}

bool NvEGLUtil::createSurface()
{
	if (m_status >= NV_HAS_SURFACE)
		return true;

	if (!m_window)
		return false;

	if (m_status < NV_INITIALIZED)
		return false;

    ANativeWindow_setBuffersGeometry(m_window, 0, 0, m_format);

    m_surface = eglCreateWindowSurface(m_display, m_config, m_window, NULL);
	if (m_surface != EGL_NO_SURFACE)
	{
		EGL_STATUS_LOG("eglCreateWindowSurface");
		int w = ANativeWindow_getWidth(m_window);
		int h = ANativeWindow_getHeight(m_window);
		EGLint sw, sh;
		eglQuerySurface(m_display, m_surface, EGL_WIDTH, &sw);
		eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &sh);
		LOGD("Win=%dx%d, Surf=%dx%d", w, h, sw, sh);
		mNewWindow = true;
	}
	else
	{
		EGL_ERROR_LOG("eglCreateWindowSurface");
		return false;
	}

	m_status = NV_HAS_SURFACE;

	return true;
}

int NvEGLUtil::getSurfaceWidth()
{
	EGLint sw;
	if ((m_status >= NV_HAS_SURFACE) && eglQuerySurface(m_display, m_surface, EGL_WIDTH, &sw))
		return sw;
	else
		return m_width;
}

int NvEGLUtil::getSurfaceHeight()
{
	EGLint sh;
	if ((m_status >= NV_HAS_SURFACE) && eglQuerySurface(m_display, m_surface, EGL_HEIGHT, &sh))
		return sh;
	else
		return m_height;
}

bool NvEGLUtil::destroySurface()
{
	if (m_status < NV_HAS_SURFACE)
		return true;

	if (m_status >= NV_IS_BOUND)
	{
        if(eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
		{
			EGL_STATUS_LOG("eglMakeCurrent");
		}
		else
		{
			EGL_ERROR_LOG("eglMakeCurrent");
			return false;
		}
	}
	
	if (eglDestroySurface(m_display, m_surface))
	{
		EGL_STATUS_LOG("eglDestroySurface");
	}
	else
	{
		EGL_ERROR_LOG("eglDestroySurface");
		return false;
	}

    m_surface = EGL_NO_SURFACE;
    
	m_status = NV_INITIALIZED;

    return true;
}

bool NvEGLUtil::isReadyToRender(bool allocateIfNeeded)
{
	if (m_status >= NV_IS_BOUND)
		return true;

	if (!allocateIfNeeded)
		return false;

    // If we have not bound the context and surface, do we even _have_ a surface?
    if (m_status < NV_HAS_SURFACE)
    {
		if (m_status < NV_INITIALIZED)
			return false;

		if (!createSurface())
			return false;
    }

    // We have a surface and context, so bind them
    if (!bind())
        return false;

    return true;
}

bool NvEGLUtil::bind()
{
	if (m_status >= NV_IS_BOUND)
		return true;

	if (m_status < NV_HAS_SURFACE)
		return false;

    if (eglMakeCurrent(m_display, m_surface, m_surface, m_context))
	{
		EGL_STATUS_LOG("eglMakeCurrent");
	}
	else
	{
		EGL_ERROR_LOG("eglMakeCurrent");
		return false;
	}

	m_status = NV_IS_BOUND;

	return true;
}

bool NvEGLUtil::unbind()
{
	if (m_status < NV_IS_BOUND)
		return true;

    if (eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
	{
		EGL_STATUS_LOG("eglMakeCurrent");
	}
	else
	{
		EGL_ERROR_LOG("eglMakeCurrent");
		return false;
	}

	m_status = NV_HAS_SURFACE;

	return true;
}

bool NvEGLUtil::swap()
{
	if (m_status < NV_IS_BOUND)
		return false;

	// Because we expect Swap to be called every frame, we only not failure
	if (!eglSwapBuffers(m_display, m_surface))
	{
		EGL_ERROR_LOG("eglSwapBuffers");
		return false;
	}

	return true;
}


bool NvEGLUtil::queryNVTimeSupport()
{
    if (m_nvTimeExtensionQueried)
        return true;

	if (m_status < NV_IS_BOUND)
		return false;

    m_nvTimeExtensionQueried = true;

	m_eglGetSystemTimeFrequencyNVProc = (PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC) eglGetProcAddress("eglGetSystemTimeFrequencyNV");
	m_eglGetSystemTimeNVProc = (PFNEGLGETSYSTEMTIMENVPROC) eglGetProcAddress("eglGetSystemTimeNV");

	// now, we'll proceed through a series of sanity checking.
	// if they all succeed, we'll return.
	// if any fail, we fall out of conditional tests to end of function, null pointers, and return.
	if (m_eglGetSystemTimeFrequencyNVProc &&
		m_eglGetSystemTimeNVProc)
	{
		m_eglSystemTimeFrequency = m_eglGetSystemTimeFrequencyNVProc();
		if (m_eglSystemTimeFrequency>0) // assume okay.  quick-check it works.
		{
			EGLuint64NV time1, time2;
			time1 = m_eglGetSystemTimeNVProc();
			usleep(2000); // 2ms should be MORE than sufficient, right?
			time2 = m_eglGetSystemTimeNVProc();
			if (time1 != time2) // quick sanity only...
			{
				// we've sanity checked:
				// - fn pointers non-null
				// - freq non-zero
				// - two calls to time sep'd by sleep non-equal
				// safe to return now.
				return true;
			}
		}
	}

	// fall back if we've not returned already.
	m_eglGetSystemTimeFrequencyNVProc = (PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC) NULL;
	m_eglGetSystemTimeNVProc = (PFNEGLGETSYSTEMTIMENVPROC) NULL;

	return true;
}


long NvEGLUtil::getSystemTime()
{
	// if the extension is available _and_ EGL is bound, we use the extension
	// If either of these cases is false, we fall back to system time
	// Both of these sources just compute deltas from the last call and accumulate
	// that to the main clock, which is returned.
	// When we change modes, we reset our base and return a fake 1ms step that one time
	queryNVTimeSupport();

	if (m_eglGetSystemTimeFrequencyNVProc && m_eglGetSystemTimeNVProc && 
		(m_status >= NV_IS_BOUND))
	{
		EGLuint64NV egltime = m_eglGetSystemTimeNVProc();

		EGLuint64NV egltimequot = egltime / m_eglSystemTimeFrequency;
		EGLuint64NV egltimerem = egltime - (m_eglSystemTimeFrequency * egltimequot);
		egltimequot *= 1000;
		egltimerem *= 1000;
		egltimerem /= m_eglSystemTimeFrequency;
		egltimequot += egltimerem;

		if (m_timeMode == NV_TIME_MODE_EGL)
		{
			m_accumulatedTime += egltimequot - m_lastQueriedTime;
		}
		else
		{
			LOGD("Switching to NV Extension timer mode");

			m_accumulatedTime++; // don't return zero delta in this case
		}
		
		m_timeMode = NV_TIME_MODE_EGL;
		m_lastQueriedTime = egltimequot;
	}
	else
	{
		// Extension not supported - use system time
		struct timeval endTime;
		gettimeofday(&endTime, 0);
		long currTime = (endTime.tv_sec - m_startTime.tv_sec) * 1000;
		currTime += (endTime.tv_usec - m_startTime.tv_usec) / 1000;


		if (m_timeMode == NV_TIME_MODE_SYSTEM)
		{
			m_accumulatedTime += currTime - m_lastQueriedTime;
		}
		else
		{
			LOGD("Switching to System timer mode");
			m_accumulatedTime++; // don't return zero delta in this case
		}
		
		m_timeMode = NV_TIME_MODE_SYSTEM;
		m_lastQueriedTime = currTime;
	}

	return m_accumulatedTime;
}

