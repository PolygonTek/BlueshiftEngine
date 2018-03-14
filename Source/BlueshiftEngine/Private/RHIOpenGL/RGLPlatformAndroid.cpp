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
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"
#include <android/native_window.h>
#include <android/native_window_jni.h>

BE_NAMESPACE_BEGIN

static Str          eglVersion;
static Str          eglVendor;
static Str          eglClientAPIs;
static Str          eglExtensions;

static int          majorVersion = 0;
static int          minorVersion = 0;

static CVar         gl_debug(L"gl_debug", L"1", CVar::Bool, L"");
static CVar         gl_debugLevel(L"gl_debugLevel", L"1", CVar::Integer, L"");
static CVar         gl_ignoreGLError(L"gl_ignoreGLError", L"0", CVar::Bool, L"");
static CVar         gl_finish(L"gl_finish", L"0", CVar::Bool, L"");

static EGLConfig ChooseBestConfig(EGLDisplay eglDisplay, int inColorBits, int inAlphaBits, int inDepthBits, int inStencilBits, int inMultiSamples) {
    EGLint minAttribs[32];

    int numMinAttribs = 0;
    minAttribs[numMinAttribs++] = EGL_RENDERABLE_TYPE;
    minAttribs[numMinAttribs++] = EGL_OPENGL_ES3_BIT_KHR; // EGL_KHR_create_context

    minAttribs[numMinAttribs++] = EGL_SURFACE_TYPE;
    minAttribs[numMinAttribs++] = EGL_WINDOW_BIT;

    minAttribs[numMinAttribs++] = EGL_COLOR_BUFFER_TYPE;
    minAttribs[numMinAttribs++] = EGL_RGB_BUFFER;

    minAttribs[numMinAttribs++] = EGL_RED_SIZE;
    minAttribs[numMinAttribs++] = 5;
    minAttribs[numMinAttribs++] = EGL_GREEN_SIZE;
    minAttribs[numMinAttribs++] = 6;
    minAttribs[numMinAttribs++] = EGL_BLUE_SIZE;
    minAttribs[numMinAttribs++] = 5;
    minAttribs[numMinAttribs++] = EGL_ALPHA_SIZE;
    minAttribs[numMinAttribs++] = 0;
    minAttribs[numMinAttribs++] = EGL_DEPTH_SIZE;
    minAttribs[numMinAttribs++] = 16;
    minAttribs[numMinAttribs++] = EGL_STENCIL_SIZE;
    minAttribs[numMinAttribs++] = 0;
    minAttribs[numMinAttribs++] = EGL_SAMPLE_BUFFERS;
    minAttribs[numMinAttribs++] = 0;
    minAttribs[numMinAttribs++] = EGL_SAMPLES;
    minAttribs[numMinAttribs++] = 0;

    minAttribs[numMinAttribs++] = EGL_CONFIG_CAVEAT;
    minAttribs[numMinAttribs++] = EGL_NONE;

    minAttribs[numMinAttribs++] = EGL_NONE;

    EGLint maxConfigs;
    if (!eglChooseConfig(eglDisplay, minAttribs, nullptr, 0, &maxConfigs)) {
        BE_FATALERROR(L"Cannot query count of minimum matched EGL configs");
    }

    EGLConfig *configs = new EGLConfig[maxConfigs];
    //if (!eglGetConfigs(eglDisplay, configs, maxConfigs, &maxConfigs)) {
    if (!eglChooseConfig(eglDisplay, minAttribs, configs, maxConfigs, &maxConfigs)) {
        BE_FATALERROR(L"Cannot query all minimum matched EGL configs");
    }

    EGLConfig bestConfig = nullptr;
    int64_t bestScore = LONG_MAX; // smaller score is better

    for (int i = 0; i < maxConfigs; i++) {
        EGLint renderableType;
        EGLint surfaceType;
        EGLint colorSize;
        EGLint redSize;
        EGLint greenSize;
        EGLint blueSize;
        EGLint alphaSize;
        EGLint depthSize;
        EGLint stencilSize;
        EGLint samples;
        EGLint sampleBuffers;
        EGLint depthEncoding;

        eglGetConfigAttrib(eglDisplay, configs[i], EGL_RENDERABLE_TYPE, &renderableType);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_SURFACE_TYPE, &surfaceType);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_BUFFER_SIZE, &colorSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_RED_SIZE, &redSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_GREEN_SIZE, &greenSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_BLUE_SIZE, &blueSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_ALPHA_SIZE, &alphaSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_DEPTH_SIZE, &depthSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_STENCIL_SIZE, &stencilSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_SAMPLES, &samples);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_SAMPLE_BUFFERS, &sampleBuffers);

        // Optional, Tegra-specific non-linear depth buffer, which allows for much better
        // effective depth range in relatively limited bit-depths (e.g. 16-bit)
        int nonLinearDepth = 0;
		if (geglext._EGL_NV_depth_nonlinear) {
        	if (eglGetConfigAttrib(eglDisplay, configs[i], EGL_DEPTH_ENCODING_NV, &depthEncoding)) {
            	nonLinearDepth = (depthEncoding == EGL_DEPTH_ENCODING_NONLINEAR_NV) ? 1 : 0;
        	}
    	}

        if (!(renderableType & (EGL_OPENGL_ES3_BIT_KHR | EGL_OPENGL_ES2_BIT))) {
            continue;
        }

        if (!(surfaceType & EGL_WINDOW_BIT)) {
            continue;
        }

        if (colorSize < 16) {
            continue;
        }

        if (inDepthBits > 0) {
            if (depthSize < 16) {
                continue;
            }
        }

        if (inStencilBits > 0) {
            if (stencilSize < 4) {
                continue;
            }
        }

        if (inMultiSamples > 0) {
            if (samples == 0) {
                continue;
            }
        }

        int colorBits = redSize + greenSize + blueSize;

        int64_t score = 0;
        score |= ((int64_t)Min(Math::Abs(sampleBuffers - (inMultiSamples > 0 ? 1 : 0)), 15)) << 29;
        score |= ((int64_t)Min(Math::Abs(samples - inMultiSamples), 31)) << 24;
        score |= Min(Math::Abs(colorBits - inColorBits), 127) << 17;
        score |= Min(Math::Abs(depthSize - inDepthBits), 63) << 11;
        score |= Min(Math::Abs(1 - nonLinearDepth), 1) << 10;
        score |= Min(Math::Abs(stencilSize - inStencilBits), 31) << 6;
        score |= Min(Math::Abs(alphaSize - inAlphaBits), 31) << 0;

        if (score < bestScore || !bestConfig) {
            BE_LOG(L"Best config: renderableType(%i), surfaceType(%i), r(%i), g(%i), b(%i), a(%i), d(%i), s(%i)\n", renderableType, surfaceType, redSize, greenSize, blueSize, alphaSize, depthSize, stencilSize);

            bestConfig = configs[i];
            bestScore = score;
        }
    }

    delete[] configs;

    return bestConfig;
}

static void GetGLVersion(int *major, int *minor) {
#if 1
    // GL_MAJOR_VERSION, GL_MINOR_VERSION 쿼리는 3.0 core context 부터 지원된다
    glGetIntegerv(GL_MAJOR_VERSION, major);
    glGetIntegerv(GL_MINOR_VERSION, minor);
#else
    // 아직 context 가 만들어지기 전이라면 glGetString 을 이용
    const char *verstr = (const char *)glGetString(GL_VERSION);
    if (!verstr || sscanf(verstr, "%d.%d", major, minor) != 2) {
        *major = *minor = 0;
    }
#endif
}

void OpenGLRHI::InitMainContext(WindowHandle windowHandle, const Settings *settings) {
    // Create main context
    mainContext = new GLContext;
    mainContext->state = new GLState;
    mainContext->nativeWindow = (ANativeWindow *)windowHandle; // main context has no surface

    // Create EGL display connection
    mainContext->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mainContext->eglDisplay == EGL_NO_DISPLAY) {
        BE_FATALERROR(L"Couldn't get EGL display");
    }

    // Initialize EGL for this display
    eglInitialize(mainContext->eglDisplay, nullptr, nullptr);

    eglVersion = eglQueryString(mainContext->eglDisplay, EGL_VERSION);
    eglVendor = eglQueryString(mainContext->eglDisplay, EGL_VENDOR);
    eglClientAPIs = eglQueryString(mainContext->eglDisplay, EGL_CLIENT_APIS);
    eglExtensions = eglQueryString(mainContext->eglDisplay, EGL_EXTENSIONS);

    BE_LOG(L"EGL version: %hs\n", eglVersion.c_str());
    BE_LOG(L"EGL vendor: %hs\n", eglVendor.c_str());
    BE_LOG(L"EGL client APIs: %hs\n", eglClientAPIs.c_str());
    BE_LOG(L"EGL extensions: %hs\n", eglExtensions.c_str());

    // Set the current rendering API
    eglBindAPI(EGL_OPENGL_ES_API);

    gegl_init(mainContext->eglDisplay, false);

    if (!geglext._EGL_KHR_create_context) {
        BE_FATALERROR(L"This device cannot support OpenGL 3.0 context");
    }

    mainContext->eglConfig = ChooseBestConfig(mainContext->eglDisplay, settings->colorBits, settings->alphaBits, settings->depthBits, settings->stencilBits, settings->multiSamples);

    // Create a main EGL rendering context
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    mainContext->eglContext = eglCreateContext(mainContext->eglDisplay, mainContext->eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (mainContext->eglContext == EGL_NO_CONTEXT) {
        BE_FATALERROR(L"Couldn't create EGL context");
    }

    ActivateSurface(NullContext, windowHandle);
    
    GetGLVersion(&majorVersion, &minorVersion);

    ggl_init(gl_debug.GetBool());
    
    // default FBO 
    mainContext->defaultFramebuffer = 0;

    // Create default VAO for main context
    gglGenVertexArrays(1, &mainContext->defaultVAO);
}

void OpenGLRHI::FreeMainContext() {
    // Delete default VAO for main context
    gglDeleteVertexArrays(1, &mainContext->defaultVAO);

    eglMakeCurrent(mainContext->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (!eglDestroySurface(mainContext->eglDisplay, mainContext->eglSurface)) {
        BE_FATALERROR(L"destroying main context EGL surface: failed");
    }

    if (!eglDestroyContext(mainContext->eglDisplay, mainContext->eglContext)) {
        BE_FATALERROR(L"destroying main context EGL context: failed");
    }

    if (!eglTerminate(mainContext->eglDisplay)) {
        BE_FATALERROR(L"terminating main context EGL display: failed");
    }

    SAFE_DELETE(mainContext->state);
    SAFE_DELETE(mainContext);
}

RHI::Handle OpenGLRHI::CreateContext(RHI::WindowHandle windowHandle, bool useSharedContext) {
    GLContext *ctx = new GLContext;

    int handle = contextList.FindNull();
    if (handle == -1) {
        handle = contextList.Append(ctx);
    } else {
        contextList[handle] = ctx;
    }

    ctx->handle = (Handle)handle;
    ctx->nativeWindow = (ANativeWindow *)windowHandle;
    ctx->eglConfig = mainContext->eglConfig;

    if (!useSharedContext) {
        // main context will be reused
        ctx->state = mainContext->state;

        ctx->eglDisplay = mainContext->eglDisplay;
        ctx->eglContext = mainContext->eglContext;
        ctx->eglSurface = mainContext->eglSurface;
        ctx->defaultVAO = mainContext->defaultVAO;        
    } else {
        ctx->state = new GLState;

        ctx->eglDisplay = mainContext->eglDisplay;

        // Create a main EGL rendering context
        EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        ctx->eglContext = eglCreateContext(ctx->eglDisplay, ctx->eglConfig, mainContext->eglContext, contextAttribs);
        if (ctx->eglContext == EGL_NO_CONTEXT) {
            BE_FATALERROR(L"Couldn't create EGL context");
        }

        ActivateSurface(ctx->handle, windowHandle);
    }

    SetContext((Handle)handle);

    ctx->defaultFramebuffer = 0;

    if (useSharedContext) {
        // Create default VAO for shared context
        gglGenVertexArrays(1, &ctx->defaultVAO);
    }

    SetDefaultState();
    
    return (Handle)handle;
}

void OpenGLRHI::DestroyContext(Handle ctxHandle) {
    GLContext *ctx = contextList[ctxHandle];

    if (ctx->eglContext != mainContext->eglContext) {
        // Delete default VAO for shared context
        gglDeleteVertexArrays(1, &ctx->defaultVAO);

        if (!eglDestroySurface(ctx->eglDisplay, ctx->eglSurface)) {
            BE_FATALERROR(L"destroying context EGL surface: failed");
        }
        BE_DLOG(L"destroying context EGL surface: ok\n");

        if (!eglDestroyContext(ctx->eglDisplay, ctx->eglContext)) {
            BE_FATALERROR(L"destroying context EGL context: failed");
        }
        BE_DLOG(L"destroying context EGL context: ok\n");

        delete ctx->state;
    }

    if (currentContext == ctx) {
        currentContext = mainContext;

        eglMakeCurrent(mainContext->eglDisplay, mainContext->eglSurface, mainContext->eglSurface, mainContext->eglContext);
    }  

    delete ctx;
    contextList[ctxHandle] = nullptr; 
}

void OpenGLRHI::ActivateSurface(Handle ctxHandle, RHI::WindowHandle windowHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    ctx->nativeWindow = (ANativeWindow *)windowHandle;

    // EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
    // guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
    // As soon as we picked a EGLConfig, we can safely reconfigure the
    // ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID.
    EGLint format;
    eglGetConfigAttrib(ctx->eglDisplay, ctx->eglConfig, EGL_NATIVE_VISUAL_ID, &format);

    // Change the format and size of the window buffers. 
    // 0, 0 -> Buffer dimensions = Screen resolution
    ANativeWindow_setBuffersGeometry(ctx->nativeWindow, 0, 0, format);

    // Once we've got a valid configuration we can create a window surface that'll be used for rendering
    EGLint surfaceAttribs[32];
    int numSurfaceAttribs = 0;

    surfaceAttribs[numSurfaceAttribs++] = EGL_RENDER_BUFFER;
    surfaceAttribs[numSurfaceAttribs++] = EGL_BACK_BUFFER;

    if (geglext._EGL_KHR_gl_colorspace) {
        surfaceAttribs[numSurfaceAttribs++] = EGL_GL_COLORSPACE_KHR;
        surfaceAttribs[numSurfaceAttribs++] = EGL_GL_COLORSPACE_SRGB_KHR;
    }
    surfaceAttribs[numSurfaceAttribs++] = EGL_NONE;

    ctx->eglSurface = eglCreateWindowSurface(ctx->eglDisplay, ctx->eglConfig, ctx->nativeWindow, surfaceAttribs);
    if (ctx->eglSurface == EGL_NO_SURFACE) {
        BE_FATALERROR(L"Couldn't create EGL window surface");
    }

    // Associate the EGL context with the EGL surface
    if (!eglMakeCurrent(ctx->eglDisplay, ctx->eglSurface, ctx->eglSurface, ctx->eglContext)) {
        BE_FATALERROR(L"Couldn't make current context");
    }
}

void OpenGLRHI::DeactivateSurface(Handle ctxHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    eglMakeCurrent(ctx->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    
    if (ctx->eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(ctx->eglDisplay, ctx->eglSurface);
        ctx->eglSurface = EGL_NO_SURFACE;
    }
}

void OpenGLRHI::SetContext(Handle ctxHandle) {
    EGLContext eglCurrentContext = eglGetCurrentContext();
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    if (eglCurrentContext != ctx->eglContext) {
        if (eglCurrentContext != EGL_NO_CONTEXT) {
            gglFlush();
        }

        eglMakeCurrent(currentContext->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    if (!eglMakeCurrent(ctx->eglDisplay, ctx->eglSurface, ctx->eglSurface, ctx->eglContext)) {
        BE_FATALERROR(L"OpenGLRHI::SetContext: Couldn't make current context");
    }
    
    this->currentContext = ctx;
}

void OpenGLRHI::SetContextDisplayFunc(Handle ctxHandle, DisplayContextFunc displayFunc, void *displayFuncDataPtr, bool onDemandDrawing) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
    
    ctx->displayFunc = displayFunc;
    ctx->displayFuncDataPtr = displayFuncDataPtr;
    ctx->onDemandDrawing = onDemandDrawing;
}

void OpenGLRHI::DisplayContext(Handle ctxHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
    
    ctx->displayFunc(ctxHandle, ctx->displayFuncDataPtr);
}

RHI::WindowHandle OpenGLRHI::GetWindowHandleFromContext(Handle ctxHandle) {
    const GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
    return (WindowHandle)ctx->nativeWindow;
}

void OpenGLRHI::GetContextSize(Handle ctxHandle, int *windowWidth, int *windowHeight, int *backingWidth, int *backingHeight) const {
    const GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    if (windowWidth || windowHeight || backingWidth || backingHeight) {
        EGLint surfaceWidth;
        EGLint surfaceHeight;

        eglQuerySurface(ctx->eglDisplay, ctx->eglSurface, EGL_WIDTH, &surfaceWidth);
        eglQuerySurface(ctx->eglDisplay, ctx->eglSurface, EGL_HEIGHT, &surfaceHeight);

        if (windowWidth) {
            *windowWidth = surfaceWidth;
        }
        if (windowHeight) {
            *windowHeight = surfaceHeight;
        }
        if (backingWidth) {
            *backingWidth = surfaceWidth;
        }
        if (backingHeight) {
            *backingHeight = surfaceHeight;
        }
    }
}

bool OpenGLRHI::IsFullscreen() const {
    return true;
}

bool OpenGLRHI::SetFullscreen(Handle ctxHandle, int width, int height) {    
    return true;
}

void OpenGLRHI::ResetFullscreen(Handle ctxHandle) {   
}

void OpenGLRHI::GetGammaRamp(unsigned short ramp[768]) const {
}

void OpenGLRHI::SetGammaRamp(unsigned short ramp[768]) const {
}

bool OpenGLRHI::SwapBuffers() {
    if (!gl_ignoreGLError.GetBool()) {
        CheckError("OpenGLRHI::SwapBuffers");
    }

    if (gl_finish.GetBool()) {
        gglFinish();
    }

    //gglFlush();

    EGLBoolean succeeded = eglSwapBuffers(currentContext->eglDisplay, currentContext->eglSurface);
    if (!succeeded) {
        EGLint err = eglGetError();
        if (err == EGL_BAD_SURFACE || err == EGL_BAD_NATIVE_WINDOW) {
            DeactivateSurface(currentContext->handle);
            ActivateSurface(currentContext->handle, currentContext->nativeWindow);
            return true;
        } else if (err == EGL_CONTEXT_LOST || err == EGL_BAD_CONTEXT) {
            return false;
        }
        return false;
    }

    if (gl_debug.IsModified()) {
        gl_debug.ClearModified();

        ggl_rebind(gl_debug.GetBool());
    }

    return true;
}

void OpenGLRHI::SwapInterval(int interval) const {
    eglSwapInterval(currentContext->eglContext, interval);
}

BE_NAMESPACE_END
