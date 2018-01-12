//----------------------------------------------------------------------------------
// File:        native_basic/jni/engine.cpp
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
#define _SAMPLE 0
#define _ENGINE 1 

#if _ENGINE || 0
#include "PreCompiled.h"
#include "Application.h"
#include "Android/window.h"
#include <sys/stat.h>
#endif

#include <wchar.h>
#include "engine.h"

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl3.h>

#include <android/log.h>
#include <nv_and_util/nv_native_app_glue.h>
#include <nv_egl_util/nv_egl_util.h>
#include <nv_bitfont/nv_bitfont.h>
#include <nv_shader/nv_shader.h>

#if _ENGINE || 0

extern "C" {

	//To work around http://code.google.com/p/android/issues/detail?id=23203
	//we don't link with the crt objects. In some configurations, this means
	//a lack of the __dso_handle symbol because it is defined there, and
	//depending on the android platform and ndk versions used, it may or may
	//not be defined in libc.so. In the latter case, we fail to link. Defining
	//it here as weak makes us provide the symbol when it's not provided by
	//the crt objects, making the change transparent for future NDKs that
	//would fix the original problem. On older NDKs, it is not a problem
	//either because the way __dso_handle was used was already broken (and
	//the custom linker works around it).

	__attribute__((weak)) void *__dso_handle;
}

static void DisplayContext(BE1::RHI::Handle context, void *dataPtr) {
	static int t0 = 0;

	if (t0 == 0) {
		t0 = BE1::PlatformTime::Milliseconds();
	}

	int t = BE1::PlatformTime::Milliseconds();
	int elapsedMsec = t - t0;
	if (elapsedMsec > 100)
		elapsedMsec = 100;

	BE1::Engine::RunFrame(elapsedMsec);

	BE1::gameClient.RunFrame();

	app.Update();

	BE1::gameClient.EndFrame();

	app.Draw();

	t0 = t;
}

#endif // _SAMPLE


Engine::Engine(NvEGLUtil& egl, struct android_app* app) :
	mEgl(egl)
{
    mApp = app;

	mResizePending = false;

	mGameplayMode = true;

	mForceRender = 4;

    mTimeVal = 0.0;

    app->userData = this;
	app->onAppCmd = &Engine::handleCmdThunk;
    app->onInputEvent = &Engine::handleInputThunk;

	m_uiInitialized = false;

	nv_shader_init(app->activity->assetManager);

#if _SAMPLE
	m_clockText = NULL;
	m_uiText[0] = NULL;
	m_uiText[1] = NULL;
#endif
#if _ENGINE

	bool bDebugAndroid = 0;
#ifdef DEBUG
	bDebugAndroid = true;
#endif
	jclass contextClass = (app->appThreadEnv)->FindClass("android/content/Context");
	jmethodID midGetPackageName = (app->appThreadEnv)->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
	jstring packageName = (jstring)(app->appThreadEnv)->CallObjectMethod(app->activity->clazz, midGetPackageName);
	const char *package = (app->appThreadEnv)->GetStringUTFChars(packageName, JNI_FALSE);
	if (strcmp(package, "com.polygontek.devtech.AndroidPlayer") == 0)
		bDebugAndroid = 1;
	(app->appThreadEnv)->ReleaseStringUTFChars(packageName, package);
	(app->appThreadEnv)->DeleteLocalRef(contextClass);
	(app->appThreadEnv)->DeleteLocalRef(packageName);

	BE1::PlatformFile::SetManager(app->activity->assetManager);
	ANativeActivity_setWindowFlags(app->activity,
		AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
	if (!bDebugAndroid) {
		BE1::PlatformFile::SetExecutablePath(app->activity->externalDataPath);
	}
	else {
		mkdir("/sdcard/blueshift", S_IRWXU | S_IRWXG | S_IRWXO);
		BE1::PlatformFile::SetExecutablePath("/sdcard/blueshift");
	}

#endif // _ENGINE

}

Engine::~Engine()
{
#if _ENGINE
	if (m_uiInitialized) {
		app.OnApplicationTerminate();

		app.Shutdown();

		app.mainRenderContext->Shutdown();
		BE1::renderSystem.FreeRenderContext(app.mainRenderContext);

		//DestroyRenderWindow(mainWnd);

		BE1::gameClient.Shutdown();

		BE1::Engine::Shutdown();

	}
#endif // _ENGINE
#if _SAMPLE
	NVBFTextFree(m_uiText[1]);
	NVBFTextFree(m_uiText[0]);
	NVBFTextFree(m_clockText);
	NVBFCleanup();

#endif // _SAMPLE
}

bool Engine::initUI()
{
	if (m_uiInitialized)
		return true;

#if _SAMPLE
	#define NUM_FONTS	2
	static NvBool fontsSplit[NUM_FONTS] = {1,1}; /* all are split */
	static const char *fontFiles[NUM_FONTS] = {
	    "courier+lucida_256.dds",
	    "utahcond+bold_1024.dds"
	};
	if (NVBFInitialize(NUM_FONTS, (const char**)fontFiles, fontsSplit, 0))
	{
		LOGW("Could not initialize NvBitFont");
		return false;
	}

	m_clockText = NVBFTextAlloc();
	NVBFTextSetFont(m_clockText, 1); // should look up by font file name.
	NVBFTextSetSize(m_clockText, 32);
    NVBFTextSetColor(m_clockText, NV_PC_PREDEF_WHITE);
	NVBFTextSetString(m_clockText, "000:00.00");

	m_uiText[0] = NVBFTextAlloc();
	NVBFTextSetFont(m_uiText[0], 2); // should look up by font file name.
	NVBFTextSetSize(m_uiText[0], 32);
    NVBFTextSetColor(m_uiText[0], NV_PC_PREDEF_WHITE);
	NVBFTextSetString(m_uiText[0],
			NVBF_COLORSTR_RED NVBF_STYLESTR_BOLD "Auto-pause:\n" NVBF_STYLESTR_NORMAL
			NVBF_COLORSTR_BLUE "Press back to quit\nTap window to resume");

	m_uiText[1] = NVBFTextAlloc();
	NVBFTextSetFont(m_uiText[1], 2); // should look up by font file name.
	NVBFTextSetSize(m_uiText[1], 32);
    NVBFTextSetColor(m_uiText[1], NV_PC_PREDEF_WHITE);
	NVBFTextSetString(m_uiText[1], NVBF_COLORSTR_GREEN "Gameplay mode 1 !");
#endif
#if _ENGINE
	//// ----- Core initialization -----
	//BE1::Engine::InitParms initParms;

	//BE1::Str appDir = BE1::PlatformFile::ExecutablePath();
	//initParms.baseDir = appDir;

	//BE1::Str dataDir = appDir + "/Data";
	//initParms.searchPath = dataDir;

	//BE1::Engine::Init(&initParms);

	BE1::Engine::InitParms initParms;
	initParms.baseDir = "";
	initParms.searchPath = "Data";
	BE1::Engine::Init(&initParms);
	//// -------------------------------

	//BE1::resourceGuidMapper.Read("Data/guidmap");
	BE1::resourceGuidMapper.Read("Data/guidmap");

	//// mainWindow(UIWindow) - rootViewController.view(UIView) - eaglView(EAGLView)
	//CGRect screenBounds = [[UIScreen mainScreen] bounds];
	//mainWindow = [[UIWindow alloc] initWithFrame:screenBounds];
	//mainWindow.backgroundColor = [UIColor blackColor];

	//rootViewController = [[RootViewController alloc] init];
	//mainWindow.rootViewController = rootViewController;

	//[mainWindow makeKeyAndVisible];

	//BE1::gameClient.Init((__bridge BE1::Renderer::WindowHandle)mainWindow, true);
	extern NvEGLUtil *g_pEGL;
	g_pEGL = &mEgl;
	BE1::gameClient.Init(0 /*&mEgl*/, true);

	//float retinaScale = [[UIScreen mainScreen] scale];
	//BE1::Vec2 screenScaleFactor(0.75f, 0.75f);
	//int renderWidth = screenBounds.size.width * retinaScale * screenScaleFactor.x;
	//int renderHeight = screenBounds.size.height * retinaScale* screenScaleFactor.y;

	//app.mainRenderContext = BE1::renderSystem.AllocRenderContext(true);
	app.mainRenderContext = BE1::renderSystem.AllocRenderContext(true);
	//app.mainRenderContext->Init((__bridge BE1::Renderer::WindowHandle)[rootViewController view],
	//	renderWidth, renderHeight, DisplayContext, NULL);
	{
		int w = mEgl.getWidth();
		int h = mEgl.getHeight();
		app.mainRenderContext->Init(&mEgl, w, h, DisplayContext, 0);
	}

    app.Init();

    app.LoadAppScript("Application");

    app.StartAppScript();
#endif 
	m_uiInitialized = true;
	return true;
}

void Engine::setGameplayMode(bool running)
{
	if (mGameplayMode != running)
		requestForceRender();

	mGameplayMode = running;
}

bool Engine::checkWindowResized()
{
	if (mEgl.checkWindowResized())
	{
		mResizePending = true;
		requestForceRender();
		LOGI("Window size change %dx%d", mEgl.getWidth(), mEgl.getHeight()); 
		return true;
	}

	return false;
}

bool Engine::resizeIfNeeded()
{
	if (!mResizePending)
		return false;

	int w = mEgl.getWidth();
	int h = mEgl.getHeight();
	int height = (w > h) ? (h / 16) : (w / 16);

#if _SAMPLE
	NVBFSetScreenRes(w, h);
	if (m_clockText)
	{
		NVBFTextSetSize(m_clockText, height);
		NVBFTextCursorAlign(m_clockText, NVBF_ALIGN_LEFT, NVBF_ALIGN_TOP);
		NVBFTextCursorPos(m_clockText, 10, 10);
	}
	if (m_uiText[0])
	{
		NVBFTextSetSize(m_uiText[0], height);
		NVBFTextCursorAlign(m_uiText[0], NVBF_ALIGN_CENTER, NVBF_ALIGN_BOTTOM);
		NVBFTextCursorPos(m_uiText[0], w / 2, h / 2);
	}
	if (m_uiText[1])
	{
		NVBFTextSetSize(m_uiText[1], height);
		NVBFTextCursorAlign(m_uiText[1], NVBF_ALIGN_CENTER, NVBF_ALIGN_CENTER);
		NVBFTextCursorPos(m_uiText[1], w / 2, h / 2);
	}
#endif // _SAMPLE

	mResizePending = false;

	return true;
}

bool Engine::renderFrame(bool allocateIfNeeded)
{
    if (!mEgl.isReadyToRender(allocateIfNeeded))
        return false;

	if (!initUI())
	{
		LOGW("Could not initialize UI - assets may be missing!");
		ANativeActivity_finish(mApp->activity);
		return false;
	}

	resizeIfNeeded();
#if !_ENGINE
	// set up viewport
	glViewport((GLint)0, (GLint)0, 
		(GLsizei)(mEgl.getWidth()), (GLsizei)(mEgl.getHeight()));

	// clear buffers as necessary
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
	// do some game rendering here
	// ...
	
#if _SAMPLE

	// start rendering bitfont text overlaid here.
	NVBFTextRenderPrep();

	if (m_clockText)
	{
		NVBFTextSetMultiplyColor(m_clockText, mGameplayMode ?
			NV_PACKED_COLOR(255, 255, 255, 255) :
			NV_PACKED_COLOR(255, 50, 50, 255));
		NVBFTextRender(m_clockText);

		// we update the clock text >after< drawing so it will change on pause.
		int mins = mTimeVal / 60;
		float secs = (float)mTimeVal - mins * 60;
		char str[32];
		sprintf(str, "%03d:%05.2f", mins, secs);
		NVBFTextSetString(m_clockText, str);
	}

	void* uiText = m_uiText[mGameplayMode ? 1 : 0];
	if (uiText)
		NVBFTextRender(uiText);

	// done rendering overlaid text.
	NVBFTextRenderDone();

#endif // _SAMPLE
#if _ENGINE
	DisplayContext(BE1::RHI::NullContext, 0);
#endif


	if (mForceRender > 0)
		mForceRender--;
#if !_ENGINE
    mEgl.swap();
#endif
    return true;
}

void Engine::updateFrame(bool interactible, long deltaTime)
{
	if (interactible)
	{
		// Each frame, we check to see if the window has resized.  While the
		// various events we get _should_ cover this, in practice, it appears
		// that the safest move across all platforms and OSes is to check at 
		// the top of each frame
		checkWindowResized();

		// Time stands still when we're auto-paused, and we don't
		// automatically render
		if (mGameplayMode)
		{
			advanceTime(deltaTime);

			// This will try to set up EGL if it isn't set up
			// When we first set up EGL completely, we also load our GLES resources
			// If these are already set up or we succeed at setting them all up now, then
			// we go ahead and render.
			renderFrame(true);
		}
		else if (isForcedRenderPending()) // forced rendering when needed for UI, etc
		{
			renderFrame(true);
		}
	}
	else
	{
		// Even if we are not interactible, we may be visible, so we
		// HAVE to do any forced renderings if we can.  We must also
		// check for resize, since that may have been the point of the
		// forced render request in the first place!
		if (isForcedRenderPending() && mEgl.isReadyToRender(false)) 
		{
			checkWindowResized();
			renderFrame(false);
		}
	}
}

int Engine::handleInput(AInputEvent* event)
{
    //We only handle motion events (touchscreen) and key (button/key) events
#if _SAMPLE
	int32_t eventType = AInputEvent_getType(event);

	if (eventType == AINPUT_EVENT_TYPE_MOTION)
	{
		int32_t action = AMOTION_EVENT_ACTION_MASK &
    					 AMotionEvent_getAction((const AInputEvent*)event);

		// A tap on the screen takes us out of autopause into gameplay mode if
		// we were paused.  No other touch processing is done.
		if (action == AMOTION_EVENT_ACTION_DOWN)
		{
			setGameplayMode(true);
			return 0;
		}

		return 1;
	} else if (eventType == AINPUT_EVENT_TYPE_KEY) {
		int32_t code = AKeyEvent_getKeyCode((const AInputEvent*)event);

		// if we are in gameplay mode, we eat the back button and move into
		// pause mode.  If we are already in pause mode, we allow the back
		// button to be handled by the OS, which means we'll be shut down
		if ((code == AKEYCODE_BACK) && mGameplayMode)
		{
			setGameplayMode(false);
			return 1;
		}

	}
#endif // _SAMPLE
#if _ENGINE
	int32_t eventType = AInputEvent_getType(event);

	if (eventType == AINPUT_EVENT_TYPE_MOTION)
	{
		int32_t action = (AMotionEvent_getAction((const AInputEvent*)event) & AMOTION_EVENT_ACTION_MASK);

		// A tap on the screen takes us out of autopause into gameplay mode if
		// we were paused.  No other touch processing is done.
		if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN)
		{
			int32_t index = (AMotionEvent_getAction((const AInputEvent*)event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
			{
				int32_t locationX = AMotionEvent_getX((const AInputEvent*)event, index);
				int32_t locationY = AMotionEvent_getY((const AInputEvent*)event, index);

				uint64_t touchId = AMotionEvent_getPointerId((const AInputEvent*)event, index);
				uint64_t locationQword = BE1::MakeQWord((int)locationX, (int)locationY);

				BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, true, 0, NULL);
				BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, locationX, locationY, 0, NULL);
				BE1::platform->QueEvent(BE1::Platform::TouchBeganEvent, touchId, locationQword, 0, NULL);

			}

			return 1;
		}
		else if (action == AMOTION_EVENT_ACTION_MOVE) {
			for (int32_t index = 0; index < AMotionEvent_getPointerCount((const AInputEvent*)event); index++) {
				int32_t locationX = AMotionEvent_getX((const AInputEvent*)event, index);
				int32_t locationY = AMotionEvent_getY((const AInputEvent*)event, index);

				uint64_t touchId = AMotionEvent_getPointerId((const AInputEvent*)event, index);
				uint64_t locationQword = BE1::MakeQWord((int)locationX, (int)locationY);

				BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, locationX, locationY, 0, NULL);
				BE1::platform->QueEvent(BE1::Platform::TouchMovedEvent, touchId, locationQword, 0, NULL);
			}
		}
		else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP) {
			int32_t index = (AMotionEvent_getAction((const AInputEvent*)event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
			{
				int32_t locationX = AMotionEvent_getX((const AInputEvent*)event, index);
				int32_t locationY = AMotionEvent_getY((const AInputEvent*)event, index);

				uint64_t touchId = AMotionEvent_getPointerId((const AInputEvent*)event, index);
				uint64_t locationQword = BE1::MakeQWord((int)locationX, (int)locationY);

				BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, NULL);
				BE1::platform->QueEvent(BE1::Platform::TouchEndedEvent, touchId, locationQword, 0, NULL);
			}
		}
		else if (action == AMOTION_EVENT_ACTION_CANCEL) {
			for (int32_t index = 0; index < AMotionEvent_getPointerCount((const AInputEvent*)event); index++) {
				uint64_t touchId = AMotionEvent_getPointerId((const AInputEvent*)event, index);

				BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, NULL);
				BE1::platform->QueEvent(BE1::Platform::TouchCanceledEvent, touchId, 0, 0, NULL);
			}
		}
	}
#endif
    return 0;
}

void Engine::handleCommand(int cmd)
{
    switch (cmd)
    {
		// The window is being shown, get it ready.
		// Note that on ICS, the EGL size will often be correct for the new size here
		// But on HC it will not be.  We need to defer checking the new res until the
		// first render with the new surface!
        case APP_CMD_INIT_WINDOW:
        case APP_CMD_WINDOW_RESIZED:
			mEgl.setWindow(mApp->window);
			requestForceRender();
        	break;

        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
			mEgl.setWindow(NULL);
			break;

        case APP_CMD_GAINED_FOCUS:
#if _ENGINE
			setGameplayMode(true);
#endif
			requestForceRender();
			break;

        case APP_CMD_LOST_FOCUS:
		case APP_CMD_PAUSE:
        	// Move out of gameplay mode if we are in it.  But if we are
			// in another dialog mode, leave it as-is
            if (mGameplayMode)
				setGameplayMode(false);
			requestForceRender();
            break;

		// ICS does not appear to require this, but on GB phones,
		// not having this causes rotation changes to be delayed or
		// ignored when we're in a non-rendering mode like autopause.
		// The forced renders appear to allow GB to process the rotation
		case APP_CMD_CONFIG_CHANGED:
			requestForceRender();
			break;
    }
}

/**
 * Process the next input event.
 */
int32_t Engine::handleInputThunk(struct android_app* app, AInputEvent* event)
{
    Engine* engine = (Engine*)app->userData;
	if (!engine)
		return 0;

	return engine->handleInput(event);
}

/**
 * Process the next main command.
 */
void Engine::handleCmdThunk(struct android_app* app, int32_t cmd)
{
    Engine* engine = (Engine*)app->userData;
	if (!engine)
		return;

	engine->handleCommand(cmd);
}
