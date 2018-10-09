// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Application.h"
#include "android_native_app_glue.h"
#include <dlfcn.h>
#include <android/sensor.h>
#include <android/log.h>

static bool                 appInitialized = false;
static int                  currentWindowWidth = 0;
static int                  currentWindowHeight = 0;
static bool                 suspended = false;
static bool                 surfaceCreated = false;
static bool                 hasFocus = false;

static ASensorManager *     sensorManager = nullptr;
static const ASensor *      accelerometerSensor = nullptr;
static const ASensor *      gyroscopeSensor = nullptr;
static ASensorEventQueue *  sensorEventQueue = nullptr;

static BE1::RHI::Handle     mainContext = BE1::RHI::NullContext;
static BE1::RHI::Handle     mainRenderTarget = BE1::RHI::NullRenderTarget;

static void SystemLog(int logLevel, const wchar_t *msg) {
    if (logLevel == BE1::DevLog) {
        __android_log_print(ANDROID_LOG_VERBOSE, "TestRenderer", "%ls", msg);
    } else if (logLevel == BE1::WarningLog) {
        __android_log_print(ANDROID_LOG_WARN, "TestRenderer", "%ls", msg);
    } else if (logLevel == BE1::ErrorLog) {
        __android_log_print(ANDROID_LOG_ERROR, "TestRenderer", "%ls", msg);
    } else {
        __android_log_print(ANDROID_LOG_INFO, "TestRenderer", "%ls", msg);
    }
}

static void SystemError(int errLevel, const wchar_t *msg) {
    JNIEnv *env = BE1::AndroidJNI::GetJavaEnv();

    jstring javaMsg = BE1::WStr(msg).ToJavaString(env);
    BE1::AndroidJNI::CallVoidMethod(env, BE1::AndroidJNI::activity->clazz, BE1::AndroidJNI::javaMethod_showAlert, javaMsg);

    env->DeleteLocalRef(javaMsg);

    // FIXME: wait until finishing java UI thread
    exit(EXIT_SUCCESS);
}

static void DisplayMainContext(BE1::RHI::Handle context, void *dataPtr) {
    static float t0 = BE1::PlatformTime::Milliseconds() / 1000.0f;
    float t = BE1::PlatformTime::Milliseconds() / 1000.0f - t0;

    ::app.Draw(context, mainRenderTarget, t);
}

static void InitDisplay(ANativeWindow *window) {
    if (!appInitialized) {
        appInitialized = true;

        currentWindowWidth = ANativeWindow_getWidth(window);
        currentWindowHeight = ANativeWindow_getHeight(window);

        ::app.Init(window);

        ::app.LoadResources();

        mainContext = BE1::rhi.CreateContext(window, false);

        mainRenderTarget = ::app.CreateRenderTarget(mainContext);

        BE1::rhi.SetContextDisplayFunc(mainContext, DisplayMainContext, nullptr, false);
    } else {
        BE1::rhi.ActivateSurface(mainContext, window);
    }
}

/*
 * AcquireASensorManagerInstance(android_app* app)
 *    Workaround ASensorManager_getInstance() deprecation false alarm
 *    for Android-N and before, when compiling with NDK-r15
 */
static ASensorManager *AcquireASensorManagerInstance(android_app *app) {
    typedef ASensorManager *(*PF_GETINSTANCEFORPACKAGE)(const char *name);
    void *androidHandle = dlopen("libandroid.so", RTLD_NOW);
    PF_GETINSTANCEFORPACKAGE getInstanceForPackageFunc = (PF_GETINSTANCEFORPACKAGE)dlsym(androidHandle, "ASensorManager_getInstanceForPackage");
    if (getInstanceForPackageFunc) {
        JNIEnv *env = nullptr;
        app->activity->vm->AttachCurrentThread(&env, nullptr);

        jclass android_content_Context = env->GetObjectClass(app->activity->clazz);
        jmethodID midGetPackageName = env->GetMethodID(android_content_Context, "getPackageName", "()Ljava/lang/String;");
        jstring packageName= (jstring)env->CallObjectMethod(app->activity->clazz, midGetPackageName);

        const char *nativePackageName = env->GetStringUTFChars(packageName, 0);
        ASensorManager *mgr = getInstanceForPackageFunc(nativePackageName);
        env->ReleaseStringUTFChars(packageName, nativePackageName);
        app->activity->vm->DetachCurrentThread();
        if (mgr) {
            dlclose(androidHandle);
            return mgr;
        }
    }

    typedef ASensorManager *(*PF_GETINSTANCE)();
    PF_GETINSTANCE getInstanceFunc = (PF_GETINSTANCE) dlsym(androidHandle, "ASensorManager_getInstance");
    // by all means at this point, ASensorManager_getInstance should be available
    assert(getInstanceFunc);
    dlclose(androidHandle);

    return getInstanceFunc();
}

static void InitSensors(android_app *appState) {
    sensorManager = AcquireASensorManagerInstance(appState);
    accelerometerSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    gyroscopeSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_GYROSCOPE);
    sensorEventQueue = ASensorManager_createEventQueue(sensorManager, appState->looper, LOOPER_ID_USER, nullptr, nullptr);
}

static void ProcessSensors(int32_t id) {
    // If a sensor has data, process it now.
    if (id == LOOPER_ID_USER) {
        if (accelerometerSensor) {
            ASensorEvent event;
            while (ASensorEventQueue_getEvents(sensorEventQueue, &event, 1) > 0) {
                /*float maginitude = event.acceleration.x * event.acceleration.x + event.acceleration.y * event.acceleration.y;
                if (maginitude * 4 >= event.acceleration.z * event.acceleration.z) {
                    int32_t orientation = ORIENTATION_REVERSE_LANDSCAPE;
                    float angle = atan2f(-event.acceleration.y, event.acceleration.x);
                    if (angle <= -M_PI_2 - M_PI_4) {
                        orientation = ORIENTATION_REVERSE_LANDSCAPE;
                    } else if (angle <= -M_PI_4) {
                        orientation = ORIENTATION_PORTRAIT;
                    } else if (angle <= M_PI_4) {
                        orientation = ORIENTATION_LANDSCAPE;
                    } else if (angle <= M_PI_2 + M_PI_4) {
                        orientation = ORIENTATION_REVERSE_PORTRAIT;
                    }
                    LOGI( "orientation %f %d", angle, orientation);
                }*/
            }
        }
    }
}

static void ResumeSensors() {
    // When our app gains focus, we start monitoring the accelerometer.
    if (accelerometerSensor) {
        ASensorEventQueue_enableSensor(sensorEventQueue, accelerometerSensor);
        // We'd like to get 60 events per second (in us).
        ASensorEventQueue_setEventRate(sensorEventQueue, accelerometerSensor, (1000L / 60) * 1000);
    }
    if (gyroscopeSensor) {
        ASensorEventQueue_enableSensor(sensorEventQueue, gyroscopeSensor);
        // We'd like to get 60 events per second (in microseconds).
        ASensorEventQueue_setEventRate(sensorEventQueue, gyroscopeSensor, (1000L / 60) * 1000);
    }
}

static void SuspendSensors() {
    // When our app loses focus, we stop monitoring the accelerometer.
    // This is to avoid consuming battery while not being used.
    if (accelerometerSensor) {
        ASensorEventQueue_disableSensor(sensorEventQueue, accelerometerSensor);
    }
    if (gyroscopeSensor) {
        ASensorEventQueue_disableSensor(sensorEventQueue, gyroscopeSensor);
    }
}

static void WindowSizeChanged(int w, int h) {
    currentWindowWidth = w;
    currentWindowHeight = h;
}

// Process the next main command.
static void HandleCmd(android_app *appState, int32_t cmd) {
    switch (cmd) {
    case APP_CMD_DESTROY:
        /**
        * Command from main thread: the app's activity is being destroyed,
        * and waiting for the app thread to clean up and exit before proceeding.
        */
        break;
    case APP_CMD_PAUSE:
        /**
         * Command from main thread: the app's activity has been paused.
         */
        suspended = true;
        break;
    case APP_CMD_RESUME:
        /**
         * Command from main thread: the app's activity has been resumed.
         */
        suspended = false;
        break;
    case APP_CMD_INIT_WINDOW:
        /**
		 * Command from main thread: a new ANativeWindow is ready for use.  Upon
		 * receiving this command, android_app->window will contain the new window
		 * surface.
		 */
        if (appState->window) {
            surfaceCreated = true;
            InitDisplay(appState->window);
        }        
        break;
    case APP_CMD_TERM_WINDOW:
        /**
		 * Command from main thread: the existing ANativeWindow needs to be
		 * terminated.  Upon receiving this command, android_app->window still
		 * contains the existing window; after calling android_app_exec_cmd
		 * it will be set to NULL.
		 */
        if (surfaceCreated) {
            BE1::rhi.DeactivateSurface(mainContext);
            surfaceCreated = false;
        }
        break;
    case APP_CMD_LOST_FOCUS:
        /**
         * Command from main thread: the app's activity window has lost
         * input focus.
         */
        SuspendSensors();
        hasFocus = false;
        break;
    case APP_CMD_GAINED_FOCUS:
        /**
         * Command from main thread: the app's activity window has gained
         * input focus.
         */
        ResumeSensors();
        hasFocus = true;
        break;
    case APP_CMD_WINDOW_RESIZED:
        /**
         * Command from main thread: the current ANativeWindow has been resized.
         * Please redraw with its new size.
         */
        break;
    case APP_CMD_CONFIG_CHANGED:
        /**
		 * Command from main thread: the current device configuration has changed.
		 */
        break;
    }
}

// Process the next input event.
static int32_t HandleInput(android_app *appState, AInputEvent *event) {
    int32_t type = AInputEvent_getType(event);
    int32_t source = AInputEvent_getSource(event);

    if (type == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        uint64_t pointerId;
        size_t pointerIndex;
        size_t pointerCount;
        int x, y;

        if ((source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK) {
            // TODO: Implement this
        } else {
            switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
                pointerId = (uint64_t)AMotionEvent_getPointerId(event, 0);
                x = (int)AMotionEvent_getX(event, 0);
                y = (int)AMotionEvent_getY(event, 0);
                BE_LOG(L"AMOTION_EVENT_ACTION_DOWN: %i %i", x, y);
                break;
            case AMOTION_EVENT_ACTION_UP:
                pointerId = (uint64_t)AMotionEvent_getPointerId(event, 0);
                x = (int)AMotionEvent_getX(event, 0);
                y = (int)AMotionEvent_getY(event, 0);
                BE_LOG(L"AMOTION_EVENT_ACTION_UP: %i %i", x, y);
                break;
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                pointerIndex = (size_t)((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                pointerId = (uint64_t)AMotionEvent_getPointerId(event, pointerIndex);
                x = (int)AMotionEvent_getX(event, pointerIndex);
                y = (int)AMotionEvent_getY(event, pointerIndex);
                BE_LOG(L"AMOTION_EVENT_ACTION_POINTER_DOWN: %i %i", x, y);
                break;
            case AMOTION_EVENT_ACTION_POINTER_UP:
                pointerIndex = (size_t)((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                pointerId = (uint64_t)AMotionEvent_getPointerId(event, pointerIndex);
                x = (int)AMotionEvent_getX(event, pointerIndex);
                y = (int)AMotionEvent_getY(event, pointerIndex);
                BE_LOG(L"AMOTION_EVENT_ACTION_POINTER_UP: %i %i", x, y);
                break;
            case AMOTION_EVENT_ACTION_MOVE:
                // ACTION_MOVE events are batched, unlike the other events.
                pointerCount = AMotionEvent_getPointerCount(event);
                for (size_t i = 0; i < pointerCount; i++) {
                    pointerId = (uint64_t)AMotionEvent_getPointerId(event, i);
                    x = (int)AMotionEvent_getX(event, i);
                    y = (int)AMotionEvent_getY(event, i);
                    BE_LOG(L"AMOTION_EVENT_ACTION_MOVE(%i/%i): %i %i", (int)i, (int)pointerCount, x, y);
                }
                break;
            }
        }
        return 1;
    }
    return 0;
}

static void InitInstance(android_app *appState) {
    BE1::AndroidJNI::Init(appState->activity);

    BE1::Str basePath = appState->activity->externalDataPath;
    BE1::Engine::InitBase(basePath.c_str(), false, SystemLog, SystemError);

    // Set window format to 8888
    ANativeActivity_setWindowFormat(appState->activity, WINDOW_FORMAT_RGBA_8888);

    appState->onAppCmd = HandleCmd; // app command callback
    appState->onInputEvent = HandleInput; // input event callback

#ifdef USE_NDK_PROFILER
    monstartup("libTestRenderer.so");
#endif

    // Prepare to monitor accelerometer
    InitSensors(appState);
}

static void ShutdownInstance() {
    app.FreeResources();

    if (mainContext) {
        BE1::rhi.DestroyRenderTarget(mainRenderTarget);

        BE1::rhi.DestroyContext(mainContext);
    }

    app.Shutdown();

    BE1::Engine::ShutdownBase();
}

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_CURRENT_VERSION) != JNI_OK) {
        return -1;
    }
    return JNI_CURRENT_VERSION;
}

} // extern "C"

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(android_app *appState) {
    InitInstance(appState);

    // loop waiting for stuff to do.
    while (1) {
        // Read all pending events.
        int id;
        int events;
        android_poll_source *source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((id = ALooper_pollAll(!suspended ? 0 : -1, nullptr, &events, (void **)&source)) >= 0) {
            // Process this event.
            if (source) {
                source->process(appState, source);
            }

            ProcessSensors(id);

            // Check if we are exiting.
            if (appState->destroyRequested != 0) {
                ShutdownInstance();
                return;
            }
        }

        if (surfaceCreated && !suspended) {
            BE1::RHI::DisplayMetrics displayMetrics;
            BE1::rhi.GetDisplayMetrics(mainContext, &displayMetrics);

            if (displayMetrics.backingWidth != currentWindowWidth || displayMetrics.backingHeight != currentWindowHeight) {
                WindowSizeChanged(displayMetrics.backingWidth, displayMetrics.backingHeight);
            }

            app.RunFrame();

            BE1::rhi.DisplayContext(mainContext);
        }
    }
}
