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

BE_NAMESPACE_BEGIN

#define FAKE_WINDOW_CLASSNAME   _T("BLUESHIFT_FAKE_WINDOW")

static Str          eglVersion;
static Str          eglVendor;
static Str          eglClientAPIs;
static Str          eglExtensions;

static int          majorVersion = 0;
static int          minorVersion = 0;

static bool         deviceFullscreen = false;
static int          deviceBpp = 0;
static int          deviceHz = 0;
static HGLRC        hrcMain;

static CVar         gl_debug(L"gl_debug", L"1", CVar::Bool, L"");
static CVar         gl_debugLevel(L"gl_debugLevel", L"1", CVar::Integer, L"");
static CVar         gl_ignoreGLError(L"gl_ignoreGLError", L"0", CVar::Bool, L"");
static CVar         gl_finish(L"gl_finish", L"0", CVar::Bool, L"");

static LRESULT CALLBACK FakeWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static HWND CreateFakeWindow() {
    WNDCLASS wc;
    HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(nullptr);

    if (!GetClassInfo(hInstance, FAKE_WINDOW_CLASSNAME, &wc)) {
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = FakeWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIcon(wc.hInstance, IDI_WINLOGO);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszMenuName = _T("");
        wc.lpszClassName = FAKE_WINDOW_CLASSNAME;
        if (!RegisterClass(&wc)) {
            BE_FATALERROR(L"Couldn't register fake window class");
        }
    }

    HWND hwndFake = CreateWindowEx(0, FAKE_WINDOW_CLASSNAME, 
        _T("FAKE"), WS_POPUP, 0, 0, 0, 0, 
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwndFake) {
        BE_FATALERROR(L"Couldn't create fake window");
    }

    // hide fake window
    ShowWindow(hwndFake, SW_HIDE);

    return hwndFake;
}

static EGLConfig ChooseBestConfig(EGLDisplay eglDisplay, int inColorBits, int inAlphaBits, int inDepthBits, int inStencilBits, int inMultiSamples) {
    int best = -1;

    struct ConfigAttribValues {
        EGLint redSize;
        EGLint greenSize;
        EGLint blueSize;
        EGLint alphaSize;
        EGLint depthSize;
        EGLint stencilSize;
        EGLint samples;
        EGLint sampleBuffers;
    };

    static const EGLint configAttribs[] = {
        EGL_BUFFER_SIZE, 0,
        EGL_RENDERABLE_TYPE, geglext._EGL_KHR_create_context ? EGL_OPENGL_ES3_BIT_KHR : EGL_OPENGL_ES2_BIT,
        EGL_CONFIG_CAVEAT, EGL_NONE,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
        EGL_TRANSPARENT_TYPE, EGL_NONE,
        EGL_RED_SIZE, EGL_DONT_CARE,
        EGL_GREEN_SIZE, EGL_DONT_CARE,
        EGL_BLUE_SIZE, EGL_DONT_CARE,
        EGL_ALPHA_SIZE, EGL_DONT_CARE,
        EGL_DEPTH_SIZE, EGL_DONT_CARE,
        EGL_STENCIL_SIZE, EGL_DONT_CARE,
        EGL_LEVEL, 0,
        EGL_NONE // end marker
    };

    EGLint maxConfigs;
    eglChooseConfig(eglDisplay, configAttribs, nullptr, 0, &maxConfigs);
    
    EGLConfig *configs = (EGLConfig *)_alloca(sizeof(EGLConfig) * maxConfigs);
    eglChooseConfig(eglDisplay, configAttribs, configs, maxConfigs, &maxConfigs);

    ConfigAttribValues *attribValues = (ConfigAttribValues *)_alloca(sizeof(ConfigAttribValues) * maxConfigs);

    for (int i = 0; i < maxConfigs; i++) {
        ConfigAttribValues *attr = &attribValues[i];

        eglGetConfigAttrib(eglDisplay, configs[i], EGL_RED_SIZE, &attr->redSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_GREEN_SIZE, &attr->greenSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_BLUE_SIZE, &attr->blueSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_ALPHA_SIZE, &attr->alphaSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_DEPTH_SIZE, &attr->depthSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_STENCIL_SIZE, &attr->stencilSize);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_SAMPLES, &attr->samples);
        eglGetConfigAttrib(eglDisplay, configs[i], EGL_SAMPLE_BUFFERS, &attr->sampleBuffers);
    }

    for (int i = 0; i < maxConfigs; i++) {
        ConfigAttribValues *attr = &attribValues[i];

        if (inAlphaBits > 0) {
            if (attr->alphaSize <= 0) {
                continue;
            }
        }

        if (inDepthBits > 0) {
            if (attr->depthSize < 16) {
                continue;
            }
        }

        if (inStencilBits > 0) {
            if (attr->stencilSize < 4) {
                continue;
            }
        }

        if (inMultiSamples > 0) {
            if (attr->samples == 0) {
                continue;
            }
        }

        if (best == -1) {
            best = i;
            continue;
        }

        int colorBits = attr->redSize + attr->greenSize + attr->blueSize;
        int bestColorBits = attribValues[best].redSize + attribValues[best].greenSize + attribValues[best].blueSize;
        if (bestColorBits != inColorBits) {
            if (colorBits == inColorBits || colorBits > bestColorBits) {
                best = i;
                continue;
            }
        }

        if (attribValues[best].alphaSize != inAlphaBits) {
            if (attr->alphaSize == inAlphaBits || attr->alphaSize > attribValues[best].alphaSize) {
                best = i;
                continue;
            }
        }

        if (attribValues[best].depthSize != inDepthBits) {
            if (attr->depthSize == inDepthBits || attr->depthSize > attribValues[best].depthSize) {
                best = i;
                continue;
            }
        }

        if (attribValues[best].stencilSize != inStencilBits) {
            if (attr->stencilSize == inStencilBits || attr->stencilSize > attribValues[best].stencilSize) {
                best = i;
                continue;
            }
        }

        if (attribValues[best].sampleBuffers != inMultiSamples) {
            if (attr->sampleBuffers == inMultiSamples || attr->sampleBuffers > attribValues[best].sampleBuffers) {
                best = i;
                continue;
            }
        }
    }

    return configs[best];
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
    mainContext->hwnd = CreateFakeWindow(); // Create fake window for global RC
    mainContext->hdc = GetDC(mainContext->hwnd);

    // Create EGL display connection
    //mainContext->eglDisplay = eglGetDisplay(mainContext->hdc);
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
    eglBindAPI(1 ? EGL_OPENGL_ES_API : EGL_OPENGL_API);

    gegl_init(mainContext->eglDisplay, false);

    mainContext->eglConfig = ChooseBestConfig(mainContext->eglDisplay, settings->colorBits, settings->alphaBits, settings->depthBits, settings->stencilBits, settings->multiSamples);

    // Create a main EGL rendering context
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    mainContext->eglContext = eglCreateContext(mainContext->eglDisplay, mainContext->eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (mainContext->eglContext == EGL_NO_CONTEXT) {
        BE_FATALERROR(L"Couldn't create EGL context");
    }

    // Once we've got a valid configuration we can create a window surface that'll be used for rendering
    EGLint surfaceAttribs[] = { EGL_NONE };
    mainContext->eglSurface = eglCreateWindowSurface(mainContext->eglDisplay, mainContext->eglConfig, mainContext->hwnd, surfaceAttribs);
    if (mainContext->eglSurface == EGL_NO_SURFACE) {
        BE_FATALERROR(L"Couldn't create EGL window surface");
    }

    // Associate the EGL context with the EGL surface
    if (!eglMakeCurrent(mainContext->eglDisplay, mainContext->eglSurface, mainContext->eglSurface, mainContext->eglContext)) {
        BE_FATALERROR(L"Couldn't make current context");
    }

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

    if (!eglDestroyContext(mainContext->eglDisplay, mainContext->eglContext)) {
        BE_FATALERROR(L"destroying main context EGL context: failed");
    }

    if (!eglDestroySurface(mainContext->eglDisplay, mainContext->eglSurface)) {
        BE_FATALERROR(L"destroying main context EGL surface: failed");
    }

    if (!eglTerminate(mainContext->eglDisplay)) {
        BE_FATALERROR(L"terminating main context EGL display: failed");
    }

    if (!ReleaseDC(mainContext->hwnd, mainContext->hdc)) {
        BE_FATALERROR(L"releasing main context DC: failed");
    }

    if (!DestroyWindow(mainContext->hwnd)) {
        BE_FATALERROR(L"destroying main context window: failed");
    }

    SAFE_DELETE(mainContext->state);
    SAFE_DELETE(mainContext);
}

static LRESULT CALLBACK NewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;
    GLContext *ctx = (GLContext *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        if (ctx->onDemandDrawing) {
            rhi.DisplayContext(ctx->handle);
        }
        EndPaint(hwnd, &ps);
        break;
    }

    return ctx->oldWndProc(hwnd, uMsg, wParam, lParam);
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
    ctx->hwnd = (HWND)windowHandle;
    ctx->hdc = GetDC(ctx->hwnd);
    if (!ctx->hdc) {
        BE_FATALERROR(L"get DC: failed");
    }
    
    SetWindowLongPtr(ctx->hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
    ctx->oldWndProc = (WNDPROC)SetWindowLongPtr(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)NewWndProc);

    ctx->eglConfig = mainContext->eglConfig;

    if (!useSharedContext) {
        // main context will be reused
        ctx->state = mainContext->state;

        ctx->eglDisplay = mainContext->eglDisplay;
        ctx->eglSurface = mainContext->eglSurface;
        ctx->eglContext = mainContext->eglContext;
        ctx->defaultVAO = mainContext->defaultVAO;

        EGLint surfaceAttribs[] = { EGL_NONE };
        ctx->eglSurface = eglCreateWindowSurface(ctx->eglDisplay, ctx->eglConfig, ctx->hwnd, surfaceAttribs);
        if (ctx->eglSurface == EGL_NO_SURFACE) {
            BE_FATALERROR(L"Couldn't create EGL window surface");
        }
    } else {
        ctx->state = new GLState;

        ctx->eglDisplay = mainContext->eglDisplay;
        /*ctx->eglDisplay = eglGetDisplay(ctx->hdc);
        if (ctx->eglDisplay == EGL_NO_DISPLAY) {
            BE_FATALERROR(L"Couldn't get EGL display");
        }
        
        // Initialize EGL for this display
        eglInitialize(ctx->eglDisplay, nullptr, nullptr);

        eglBindAPI(EGL_OPENGL_ES_API);*/

        // Create a main EGL rendering context
        EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        ctx->eglContext = eglCreateContext(ctx->eglDisplay, ctx->eglConfig, mainContext->eglContext, contextAttribs);
        if (ctx->eglContext == EGL_NO_CONTEXT) {
            BE_FATALERROR(L"Couldn't create EGL context");
        }

        EGLint surfaceAttribs[] = { EGL_NONE };
        ctx->eglSurface = eglCreateWindowSurface(ctx->eglDisplay, ctx->eglConfig, ctx->hwnd, surfaceAttribs);
        if (ctx->eglSurface == EGL_NO_SURFACE) {
            BE_FATALERROR(L"Couldn't create EGL window surface");
        }
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

        if (!eglDestroyContext(ctx->eglDisplay, ctx->eglContext)) {
            BE_FATALERROR(L"destroying context EGL context: failed");
        }
        BE_DLOG(L"destroying context EGL context: ok\n");

        if (!eglDestroySurface(ctx->eglDisplay, ctx->eglSurface)) {
            BE_FATALERROR(L"destroying context EGL surface: failed");
        }
        BE_DLOG(L"destroying context EGL surface: ok\n");

        delete ctx->state;
    }

    if (currentContext == ctx) {
        currentContext = mainContext;

        eglMakeCurrent(mainContext->eglDisplay, mainContext->eglSurface, mainContext->eglSurface, mainContext->eglContext);
    }
    
    ReleaseDC(ctx->hwnd, ctx->hdc);

    SetWindowLongPtr(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)ctx->oldWndProc);

    delete ctx;
    contextList[ctxHandle] = nullptr; 
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
    return (WindowHandle)ctx->hwnd;
}

void OpenGLRHI::GetContextSize(Handle ctxHandle, int *windowWidth, int *windowHeight, int *backingWidth, int *backingHeight) {
    GLContext *ctx = contextList[ctxHandle];

    if (windowWidth || windowHeight || backingWidth || backingHeight) {
#if 0
        //DPI_AWARENESS dpiAwareness = GetAwarenessFromDpiAwarenessContext(GetWindowDpiAwarenessContext(ctx->hwnd));
        // Number of pixels per logical inch along the screen size.
        // In a system with multiple display monitors, this value is the same for all monitors.
        //int dpi = GetDpiForWindow(ctx->hwnd);
        int dpi = GetDeviceCaps(ctx->hdc, LOGPIXELSX);
        float dpiScale = Math::Rint(dpi / 96.0f);
#else
        float dpiScale = 1.0f;
#endif
        RECT rc;
        GetClientRect(ctx->hwnd, &rc);
        if (windowWidth) {
            *windowWidth = rc.right / dpiScale;
        }
        if (windowHeight) {
            *windowHeight = rc.bottom / dpiScale;
        }
        if (backingWidth) {
            *backingWidth = rc.right;
        }
        if (backingHeight) {
            *backingHeight = rc.bottom;
        }
    }
}

bool OpenGLRHI::IsFullscreen() const {
    return deviceFullscreen;
}

bool OpenGLRHI::SetFullscreen(Handle ctxHandle, int width, int height) {
    HDC hdc;

    BE_LOG(L"Changing display setting...\n");
    
    DEVMODE dm;
    memset(&dm, 0, sizeof(dm));
    dm.dmSize       = sizeof(dm);
    dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;	
    dm.dmPelsWidth  = width;
    dm.dmPelsHeight = height;

    int bpp = 0;
    int hz = 0;

    if (bpp > 0) {
        dm.dmFields |= DM_BITSPERPEL;
        dm.dmBitsPerPel = bpp;
    } else {
        hdc = GetDC(GetDesktopWindow());
        bpp = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(nullptr, hdc);
        if (bpp < 16) {
            dm.dmFields |= DM_BITSPERPEL;
            dm.dmBitsPerPel = 16;

            BE_LOG(L"using color bits of %i\n", 16);
        } else {
            BE_LOG(L"using desktop display depth of %i\n", bpp);
        }
    }

    if (hz > 0) {
        dm.dmDisplayFrequency = hz;
        dm.dmFields |= DM_DISPLAYFREQUENCY;
    }
    
    if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
        BE_WARNLOG(L"Can't change fullscreen mode");
        return false;
    }

    deviceFullscreen = true;

    hdc = GetDC(GetDesktopWindow());
    deviceBpp = GetDeviceCaps(hdc, BITSPIXEL);
    deviceHz = GetDeviceCaps(hdc, VREFRESH);
    ReleaseDC(nullptr, hdc);

    BE_LOG(L"set fullscreen mode: %ix%i %ibpp %ihz\n", width, height, deviceBpp, deviceHz);
    return true;
}

void OpenGLRHI::ResetFullscreen(Handle ctxHandle) {
    if (!deviceFullscreen) {
        return;
    }

    deviceFullscreen = false;

    BE_LOG(L"resetting display setting: ");
    if (ChangeDisplaySettings(nullptr, 0) == DISP_CHANGE_SUCCESSFUL) {
        BE_LOG(L"ok\n");
    } else {
        BE_LOG(L"failed\n");
    }

    HDC hdc = GetDC(nullptr);
    deviceBpp = GetDeviceCaps(hdc, BITSPIXEL);
    deviceHz = GetDeviceCaps(hdc, VREFRESH);
    ReleaseDC(nullptr, hdc);

    BE_LOG(L"set window mode: %ibpp %ihz\n", deviceBpp, deviceHz);
}

void OpenGLRHI::GetGammaRamp(unsigned short ramp[768]) const {
    ::GetDeviceGammaRamp(currentContext->hdc, ramp);
}

void OpenGLRHI::SetGammaRamp(unsigned short ramp[768]) const {
    ::SetDeviceGammaRamp(currentContext->hdc, ramp);
}

bool OpenGLRHI::SwapBuffers() {
    if (!gl_ignoreGLError.GetBool()) {
        CheckError("OpenGLRHI::SwapBuffers");
    }

    if (gl_finish.GetBool()) {
        gglFinish();
    }

    //gglFlush();

    eglSwapBuffers(currentContext->eglContext, currentContext->eglSurface);

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
