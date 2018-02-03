/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <jni.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gles3jni.h"
#if _ENGINE 
#	include "Precompiled.h"
#	include "RHI/EGLUtil.h"
#	include <android\asset_manager_jni.h>

#include "Application.h"
#include "Android/window.h"
#include "AndroidAdmob.h"

EGLUtil mEgl;
bool	Initialized;

#endif


#if !_ENGINE
const Vertex QUAD[4] = {
    // Square with diagonal < 2 so that it fits in a [-1 .. 1]^2 square
    // regardless of rotation.
    {{-0.7f, -0.7f}, {0x00, 0xFF, 0x00}},
    {{ 0.7f, -0.7f}, {0x00, 0x00, 0xFF}},
    {{-0.7f,  0.7f}, {0xFF, 0x00, 0x00}},
    {{ 0.7f,  0.7f}, {0xFF, 0xFF, 0xFF}},
};

bool checkGlError(const char* funcName) {
    GLint err = glGetError();
    if (err != GL_NO_ERROR) {
        ALOGE("GL error after %s(): 0x%08x\n", funcName, err);
        return true;
    }
    return false;
}

GLuint createShader(GLenum shaderType, const char* src) {
    GLuint shader = glCreateShader(shaderType);
    if (!shader) {
        checkGlError("glCreateShader");
        return 0;
    }
    glShaderSource(shader, 1, &src, NULL);

    GLint compiled = GL_FALSE;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLogLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen > 0) {
            GLchar* infoLog = (GLchar*)malloc(infoLogLen);
            if (infoLog) {
                glGetShaderInfoLog(shader, infoLogLen, NULL, infoLog);
                ALOGE("Could not compile %s shader:\n%s\n",
                        shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment",
                        infoLog);
                free(infoLog);
            }
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint createProgram(const char* vtxSrc, const char* fragSrc) {
    GLuint vtxShader = 0;
    GLuint fragShader = 0;
    GLuint program = 0;
    GLint linked = GL_FALSE;

    vtxShader = createShader(GL_VERTEX_SHADER, vtxSrc);
    if (!vtxShader)
        goto exit;

    fragShader = createShader(GL_FRAGMENT_SHADER, fragSrc);
    if (!fragShader)
        goto exit;

    program = glCreateProgram();
    if (!program) {
        checkGlError("glCreateProgram");
        goto exit;
    }
    glAttachShader(program, vtxShader);
    glAttachShader(program, fragShader);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        ALOGE("Could not link program");
        GLint infoLogLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);
        if (infoLogLen) {
            GLchar* infoLog = (GLchar*)malloc(infoLogLen);
            if (infoLog) {
                glGetProgramInfoLog(program, infoLogLen, NULL, infoLog);
                ALOGE("Could not link program:\n%s\n", infoLog);
                free(infoLog);
            }
        }
        glDeleteProgram(program);
        program = 0;
    }

exit:
    glDeleteShader(vtxShader);
    glDeleteShader(fragShader);
    return program;
}

static void printGlString(const char* name, GLenum s) {
    const char* v = (const char*)glGetString(s);
    ALOGV("GL %s: %s\n", name, v);
}

// ----------------------------------------------------------------------------

Renderer::Renderer()
:   mNumInstances(0),
    mLastFrameNs(0)
{
    memset(mScale, 0, sizeof(mScale));
    memset(mAngularVelocity, 0, sizeof(mAngularVelocity));
    memset(mAngles, 0, sizeof(mAngles));
}

Renderer::~Renderer() {
}

void Renderer::resize(int w, int h) {
    auto offsets = mapOffsetBuf();
    calcSceneParams(w, h, offsets);
    unmapOffsetBuf();

    // Auto gives a signed int :-(
    for (auto i = (unsigned)0; i < mNumInstances; i++) {
        mAngles[i] = drand48() * TWO_PI;
        mAngularVelocity[i] = MAX_ROT_SPEED * (2.0*drand48() - 1.0);
    }

    mLastFrameNs = 0;

    glViewport(0, 0, w, h);
}

void Renderer::calcSceneParams(unsigned int w, unsigned int h,
        float* offsets) {
    // number of cells along the larger screen dimension
    const float NCELLS_MAJOR = MAX_INSTANCES_PER_SIDE;
    // cell size in scene space
    const float CELL_SIZE = 2.0f / NCELLS_MAJOR;

    // Calculations are done in "landscape", i.e. assuming dim[0] >= dim[1].
    // Only at the end are values put in the opposite order if h > w.
    const float dim[2] = {fmaxf(w,h), fminf(w,h)};
    const float aspect[2] = {dim[0] / dim[1], dim[1] / dim[0]};
    const float scene2clip[2] = {1.0f, aspect[0]};
    const int ncells[2] = {
            static_cast<int>(NCELLS_MAJOR),
            (int)floorf(NCELLS_MAJOR * aspect[1])
    };

    float centers[2][MAX_INSTANCES_PER_SIDE];
    for (int d = 0; d < 2; d++) {
        auto offset = -ncells[d] / NCELLS_MAJOR; // -1.0 for d=0
        for (auto i = 0; i < ncells[d]; i++) {
            centers[d][i] = scene2clip[d] * (CELL_SIZE*(i + 0.5f) + offset);
        }
    }

    int major = w >= h ? 0 : 1;
    int minor = w >= h ? 1 : 0;
    // outer product of centers[0] and centers[1]
    for (int i = 0; i < ncells[0]; i++) {
        for (int j = 0; j < ncells[1]; j++) {
            int idx = i*ncells[1] + j;
            offsets[2*idx + major] = centers[0][i];
            offsets[2*idx + minor] = centers[1][j];
        }
    }

    mNumInstances = ncells[0] * ncells[1];
    mScale[major] = 0.5f * CELL_SIZE * scene2clip[0];
    mScale[minor] = 0.5f * CELL_SIZE * scene2clip[1];
}

void Renderer::step() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    auto nowNs = now.tv_sec*1000000000ull + now.tv_nsec;

    if (mLastFrameNs > 0) {
        float dt = float(nowNs - mLastFrameNs) * 0.000000001f;

        for (unsigned int i = 0; i < mNumInstances; i++) {
            mAngles[i] += mAngularVelocity[i] * dt;
            if (mAngles[i] >= TWO_PI) {
                mAngles[i] -= TWO_PI;
            } else if (mAngles[i] <= -TWO_PI) {
                mAngles[i] += TWO_PI;
            }
        }

        float* transforms = mapTransformBuf();
        for (unsigned int i = 0; i < mNumInstances; i++) {
            float s = sinf(mAngles[i]);
            float c = cosf(mAngles[i]);
            transforms[4*i + 0] =  c * mScale[0];
            transforms[4*i + 1] =  s * mScale[1];
            transforms[4*i + 2] = -s * mScale[0];
            transforms[4*i + 3] =  c * mScale[1];
        }
        unmapTransformBuf();
    }

    mLastFrameNs = nowNs;
}

void Renderer::render() {
    step();

    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw(mNumInstances);
    checkGlError("Renderer::render");
}
#endif

// ----------------------------------------------------------------------------

#if !_ENGINE
static Renderer* g_renderer = NULL;
#endif
#if _ENGINE


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


void appInit()
{
	if (!Initialized) {
		Initialized = true;

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

		//app.Init();
		app.Init();

#if USE_ADMOB
		RewardBasedVideoAd::RegisterLuaModule(&app.gameWorld->GetLuaVM().State());
#endif
		app.LoadAppScript("Application");

		app.StartAppScript();

		////BE1::cmdSystem.BufferCommandText(BE1::CmdSystem::Append, L"exec \"autoexec.cfg\"\n");


	}
}

void appDeinit()
{
	if (Initialized)
	{
		Initialized = false;

		app.OnApplicationTerminate();

		app.Shutdown();

		app.mainRenderContext->Shutdown();
		BE1::renderSystem.FreeRenderContext(app.mainRenderContext);

		//DestroyRenderWindow(mainWnd);

		BE1::gameClient.Shutdown();

		BE1::Engine::Shutdown();
	}
}


#endif

extern "C" {
    JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_init(JNIEnv* env, jobject obj);
    JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_resize(JNIEnv* env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_step(JNIEnv* env, jobject obj);
#if _ENGINE
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_done(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_SetAssetManager(JNIEnv* env, jobject obj, jobject activity, jobject asset, jstring path);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_TouchBegin(JNIEnv* env, jobject obj, jint touchId, jint locationX, jint locationY);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_TouchMove(JNIEnv* env, jobject obj, jint touchId, jint locationX, jint locationY);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_TouchEnd(JNIEnv* env, jobject obj, jint touchId, jint locationX, jint locationY);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_TouchCancel(JNIEnv* env, jobject obj, jint touchId);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_DidRewardUser(JNIEnv* env, jobject obj, jstring type, jint amount);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_DidReceiveAd(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_DidOpen(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_DidStartPlaying(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_DidClose(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_WillLeaveApplication(JNIEnv* env, jobject obj);
	JNIEXPORT void JNICALL Java_com_AndroidPlayer_GLES3JNILib_DidFailToLoad(JNIEnv* env, jobject obj, jint errorCode);

	__thread JNIEnv *g_env;
#endif
};

#if !defined(DYNAMIC_ES3)
static GLboolean gl3stubInit() {
    return GL_TRUE;
}
#endif

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_init(JNIEnv* env, jobject obj) {
#if !_ENGINE
    if (g_renderer) {
        delete g_renderer;
        g_renderer = NULL;
    }

    printGlString("Version", GL_VERSION);
    printGlString("Vendor", GL_VENDOR);
    printGlString("Renderer", GL_RENDERER);
    printGlString("Extensions", GL_EXTENSIONS);

    const char* versionStr = (const char*)glGetString(GL_VERSION);
    if (strstr(versionStr, "OpenGL ES 3.") && gl3stubInit()) {
        g_renderer = createES3Renderer();
    } else if (strstr(versionStr, "OpenGL ES 2.")) {
        g_renderer = createES2Renderer();
    } else {
        ALOGE("Unsupported OpenGL ES version");
    }
#endif
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_resize(JNIEnv* env, jobject obj, jint width, jint height) {
#if !_ENGINE
	if (g_renderer) {
        g_renderer->resize(width, height);
    }
#endif

#if _ENGINE
	JNIEnv *old_env = g_env;
	g_env = env;

	extern EGLUtil mEgl;
	appDeinit();
	mEgl.m_width = mEgl.m_windowWidth = width;
	mEgl.m_height = mEgl.m_windowHeight = height;
	appInit();
	
	g_env = old_env;
#endif
}


JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_step(JNIEnv* env, jobject obj) {
#if !_ENGINE
	if (g_renderer) {
        g_renderer->render();
    }
#endif
#if _ENGINE
	JNIEnv *old_env = g_env;
	g_env = env;

	DisplayContext(BE1::RHI::NullContext, 0);

	g_env = old_env;
#endif
}

#if _ENGINE
jobject gActivity;

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_SetAssetManager(JNIEnv* env, jobject obj, jobject activity, jobject asset, jstring path) {
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
	const char *_path = env->GetStringUTFChars(path, NULL);//Java String to C Style string
	BE1::PlatformFile::SetExecutablePath(_path);
	env->ReleaseStringUTFChars(path, _path);

	if (gActivity) {
		env->DeleteGlobalRef(gActivity);
	}
	gActivity = env->NewGlobalRef(activity);
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_TouchBegin(JNIEnv* env, jobject obj, jint touchId, jint locationX, jint locationY)
{
	uint64_t locationQword = BE1::MakeQWord((int)locationX, (int)locationY);

	BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, true, 0, NULL);
	BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, locationX, locationY, 0, NULL);
	BE1::platform->QueEvent(BE1::Platform::TouchBeganEvent, touchId, locationQword, 0, NULL);
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_TouchMove(JNIEnv* env, jobject obj, jint touchId, jint locationX, jint locationY)
{
	uint64_t locationQword = BE1::MakeQWord((int)locationX, (int)locationY);

	BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, locationX, locationY, 0, NULL);
	BE1::platform->QueEvent(BE1::Platform::TouchMovedEvent, touchId, locationQword, 0, NULL);
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_TouchEnd(JNIEnv* env, jobject obj, jint touchId, jint locationX, jint locationY)
{
	uint64_t locationQword = BE1::MakeQWord((int)locationX, (int)locationY);

	BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, NULL);
	BE1::platform->QueEvent(BE1::Platform::TouchEndedEvent, touchId, locationQword, 0, NULL);
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_TouchCancel(JNIEnv* env, jobject obj, jint touchId, jint locationX, jint locationY)
{
	BE1::platform->QueEvent(BE1::Platform::KeyEvent, BE1::KeyCode::Mouse1, false, 0, NULL);
	BE1::platform->QueEvent(BE1::Platform::TouchCanceledEvent, touchId, 0, 0, NULL);
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_DidRewardUser(JNIEnv* env, jobject obj, jstring type, jint amount)
{
	JNIEnv *old_env = g_env;
	g_env = env;

	const char *rewardType = env->GetStringUTFChars(type, NULL);
	int rewardAmount = amount;
	LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_reward_user"];
	if (function.IsFunction()) {
		function(rewardType, rewardAmount);
	}
	env->ReleaseStringUTFChars(type, rewardType);

	g_env = old_env;
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_DidReceiveAd(JNIEnv* env, jobject obj)
{
	JNIEnv *old_env = g_env;
	g_env = env;

	LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_receive_ad"];
	if (function.IsFunction()) {
		function();
	}

	g_env = old_env;
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_DidOpen(JNIEnv* env, jobject obj)
{
	JNIEnv *old_env = g_env;
	g_env = env;

	LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_open"];
	if (function.IsFunction()) {
		function();
	}

	g_env = old_env;
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_DidStartPlaying(JNIEnv* env, jobject obj)
{
	JNIEnv *old_env = g_env;
	g_env = env;

	LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_start_playing"];
	if (function.IsFunction()) {
		function();
	}

	g_env = old_env;
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_DidClose(JNIEnv* env, jobject obj)
{
	JNIEnv *old_env = g_env;
	g_env = env;

	LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_close"];
	if (function.IsFunction()) {
		function();
	}

	g_env = old_env;
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_WillLeaveApplication(JNIEnv* env, jobject obj)
{
	JNIEnv *old_env = g_env;
	g_env = env;

	LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["will_leave_application"];
	if (function.IsFunction()) {
		function();
	}

	g_env = old_env;
}

JNIEXPORT void JNICALL
Java_com_AndroidPlayer_GLES3JNILib_DidFailToLoad(JNIEnv* env, jobject obj, jint errorCode)
{
	JNIEnv *old_env = g_env;
	g_env = env;

	const char *errorDescription = "";
	LuaCpp::Selector function = (*app.state)["package"]["loaded"]["admob"]["RewardBasedVideoAd"]["did_fail_to_load"];
	if (function.IsFunction()) {
		function(errorDescription);
	}

	g_env = old_env;
}


#endif

