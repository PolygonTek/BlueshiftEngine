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
#ifdef USE_ADMOB_REWARD_BASED_VIDEO
#include "AndroidAdMob.h"
#endif

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

static void DisplayContext(BE1::RHI::Handle context, void *dataPtr) {
    app.Draw();
}

// 0: full, 1: medium, 2: low
static int DetermineRenderQuality() {
    BE1::AndroidGPUInfo gpuInfo = BE1::AndroidGPUInfo::GetFromOpenGLRendererString(BE1::rhi.GetGPUString());
    int renderQuality = 0;

    switch (gpuInfo.processor) {
    case BE1::AndroidGPUInfo::Processor::Qualcomm_Adreno:
        if (gpuInfo.model >= 500) {
            if (gpuInfo.model < 510) {
                renderQuality = 2;
            } else if (gpuInfo.model < 530) {
                renderQuality = 1;
            }
        } else if (gpuInfo.model >= 400) {
            if (gpuInfo.model < 420) {
                renderQuality = 2;
            } else if (gpuInfo.model <= 430) {
                renderQuality = 1;
            }
        } else {
            renderQuality = 2;
        }
        break;
    case BE1::AndroidGPUInfo::Processor::ARM_MaliG:
        break;
    case BE1::AndroidGPUInfo::Processor::ARM_MaliT:
        if (gpuInfo.model >= 800) {
            if (gpuInfo.model < 880) {
                renderQuality = 2;
            } else {
                renderQuality = 1;
            }
        } else if (gpuInfo.model >= 700) {
            if (gpuInfo.model < 760) {
                renderQuality = 2;
            } else {
                renderQuality = 1;
            }
        } else {
            renderQuality = 2;
        }
        break;
    case BE1::AndroidGPUInfo::Processor::ARM_Mali:
        renderQuality = 2;
        break;
    case BE1::AndroidGPUInfo::Processor::PowerVR_RogueG:
        if (gpuInfo.model >= 6000) {
            if (gpuInfo.model <= 6430) {
                renderQuality = 2;
            } else {
                renderQuality = 1;
            }
        }
        break;
    case BE1::AndroidGPUInfo::Processor::PowerVR_RogueGX:
        if (gpuInfo.model >= 6000) {
            if (gpuInfo.model < 6450) {
                renderQuality = 2;
            } else {
                renderQuality = 1;
            }
        } else {
            renderQuality = 2;
        }
        break;
    case BE1::AndroidGPUInfo::Processor::PowerVR_RogueGT:
        if (gpuInfo.model >= 7000) {
            if (gpuInfo.model < 7400) {
                renderQuality = 2;
            } else if (gpuInfo.model < 7600) {
                renderQuality = 1;
            }
        } else {
            renderQuality = 2;
        }
        break;
    case BE1::AndroidGPUInfo::Processor::PowerVR_RogueGE:
        break;
    default:
        break;
    }
    return renderQuality;
}

static void InitDisplay(ANativeWindow *window) {
    if (!appInitialized) {
        appInitialized = true;

        currentWindowWidth = ANativeWindow_getWidth(window);
        currentWindowHeight = ANativeWindow_getHeight(window);

        BE1::gameClient.Init(window, false);

        int renderQuality = DetermineRenderQuality();
        int renderWidth;
        int renderHeight;

        if (renderQuality > 0) {
            if (currentWindowWidth > currentWindowHeight) {
                // landscape mode
                if (renderQuality == 1) {
                    renderWidth = BE1::Min(1280, currentWindowWidth);
                    renderHeight = BE1::Min(720, currentWindowHeight);
                } else {
                    renderWidth = BE1::Min(1024, currentWindowWidth);
                    renderHeight = BE1::Min(576, currentWindowHeight);
                }
            } else {
                if (renderQuality == 1) {
                    renderWidth = BE1::Min(720, currentWindowWidth);
                    renderHeight = BE1::Min(1280, currentWindowHeight);
                } else {
                    renderWidth = BE1::Min(576, currentWindowWidth);
                    renderHeight = BE1::Min(1024, currentWindowHeight);
                }
            }
        } else {
            renderWidth = currentWindowWidth;
            renderHeight = currentWindowHeight;
        }

        app.mainRenderContext = BE1::renderSystem.AllocRenderContext(true);
        app.mainRenderContext->Init(window, renderWidth, renderHeight, DisplayContext, nullptr);

        app.Init();

#ifdef USE_ADMOB_REWARD_BASED_VIDEO
        RewardBasedVideoAd::RegisterLuaModule(&app.gameWorld->GetLuaVM().State());
#endif

        app.LoadAppScript("Application");

        app.StartAppScript();
    } else {
        BE1::rhi.ActivateSurface(app.mainRenderContext->GetContextHandle(), window);
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

    if (app.mainRenderContext) {
        app.mainRenderContext->OnResize(w, h);
    }
}

// Process the next main command.
static void HandleCmd(android_app *appState, int32_t cmd) {
    switch (cmd) {
    case APP_CMD_DESTROY:
        /**
        * Command from main thread: the app's activity is being destroyed,
        * and waiting for the app thread to clean up and exit before proceeding.
        */
        if (appInitialized) {
            app.OnApplicationTerminate();
        }
        break;
    case APP_CMD_PAUSE:
        /**
         * Command from main thread: the app's activity has been paused.
         */
        suspended = true;
        if (appInitialized) {
            app.OnApplicationPause(true);
        }
        break;
    case APP_CMD_RESUME:
        /**
         * Command from main thread: the app's activity has been resumed.
         */
        if (appInitialized) {
            app.OnApplicationPause(false);
        }
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
            BE1::rhi.DeactivateSurface(app.mainRenderContext->GetContextHandle());
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
        uint64_t locationQword;
        int x, y;

        if ((source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK) {
            // TODO: Implement this
        } else {
            switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
                pointerId = (uint64_t)AMotionEvent_getPointerId(event, 0);
                x = (int)AMotionEvent_getX(event, 0);
                y = (int)AMotionEvent_getY(event, 0);
                locationQword = BE1::MakeQWord(x, y);
                BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, true, 0, NULL);
                BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, x, y, 0, NULL);
                BE1::platform->QueEvent(BE1::Platform::TouchBeganEvent, pointerId, locationQword, 0, NULL);
                break;
            case AMOTION_EVENT_ACTION_UP:
                pointerId = (uint64_t)AMotionEvent_getPointerId(event, 0);
                x = (int)AMotionEvent_getX(event, 0);
                y = (int)AMotionEvent_getY(event, 0);
                locationQword = BE1::MakeQWord(x, y);
                BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, NULL);
                BE1::platform->QueEvent(BE1::Platform::TouchEndedEvent, pointerId, locationQword, 0, NULL);
                break;
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                pointerIndex = (size_t)((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                pointerId = (uint64_t)AMotionEvent_getPointerId(event, pointerIndex);
                x = (int)AMotionEvent_getX(event, pointerIndex);
                y = (int)AMotionEvent_getY(event, pointerIndex);
                locationQword = BE1::MakeQWord(x, y);
                BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, true, 0, NULL);
                BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, x, y, 0, NULL);
                BE1::platform->QueEvent(BE1::Platform::TouchBeganEvent, pointerId, locationQword, 0, NULL);
                break;
            case AMOTION_EVENT_ACTION_POINTER_UP:
                pointerIndex = (size_t)((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                pointerId = (uint64_t)AMotionEvent_getPointerId(event, pointerIndex);
                x = (int)AMotionEvent_getX(event, pointerIndex);
                y = (int)AMotionEvent_getY(event, pointerIndex);
                locationQword = BE1::MakeQWord(x, y);
                BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, NULL);
                BE1::platform->QueEvent(BE1::Platform::TouchEndedEvent, pointerId, locationQword, 0, NULL);
                break;
            case AMOTION_EVENT_ACTION_MOVE:
                // ACTION_MOVE events are batched, unlike the other events.
                pointerCount = AMotionEvent_getPointerCount(event);
                for (size_t i = 0; i < pointerCount; i++) {
                    pointerId = (uint64_t)AMotionEvent_getPointerId(event, i);
                    x = (int)AMotionEvent_getX(event, i);
                    y = (int)AMotionEvent_getY(event, i);
                    locationQword = BE1::MakeQWord(x, y);
                    BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, x, y, 0, NULL);
                    BE1::platform->QueEvent(BE1::Platform::TouchMovedEvent, pointerId, locationQword, 0, NULL);
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

    // ----- Core initialization -----
    BE1::Engine::InitParms initParms;

    BE1::Str appDir = appState->activity->externalDataPath;
    initParms.baseDir = appDir;

    BE1::Str dataDir = appDir + "/Data";
    initParms.searchPath = dataDir;

    BE1::Engine::Init(&initParms);
    // -------------------------------

    BE1::resourceGuidMapper.Read("Data/guidmap");

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
    app.Shutdown();

    app.mainRenderContext->Shutdown();

    BE1::renderSystem.FreeRenderContext(app.mainRenderContext);

    BE1::gameClient.Shutdown();

    BE1::Engine::Shutdown();
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

    int t0 = BE1::PlatformTime::Milliseconds();

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
            int backingWidth, backingHeight;
            BE1::rhi.GetContextSize(app.mainRenderContext->GetContextHandle(), nullptr, nullptr, &backingWidth, &backingHeight);
            if (backingWidth != currentWindowWidth || backingHeight != currentWindowHeight) {
                WindowSizeChanged(backingWidth, backingHeight);
            }

            int t = BE1::PlatformTime::Milliseconds();
            int elapsedMsec = t - t0;
            if (elapsedMsec > 1000) {
                elapsedMsec = 1000;
            }

            t0 = t;

            BE1::Engine::RunFrame(elapsedMsec);

            BE1::gameClient.RunFrame();

#ifdef USE_ADMOB_REWARD_BASED_VIDEO
            rewardBasedVideoAd.ProcessQueue();
#endif

            app.Update();

            app.mainRenderContext->Display();

            BE1::gameClient.EndFrame();
        }
    }
}
