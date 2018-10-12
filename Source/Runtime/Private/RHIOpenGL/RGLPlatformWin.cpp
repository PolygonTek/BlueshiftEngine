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
#include "Platform/PlatformProcess.h"
#include <tchar.h>

BE_NAMESPACE_BEGIN

#define MAX_PIXEL_FORMAT        1024
#define MAX_ATTRIB_SIZE         32

#define FAKE_WINDOW_CLASSNAME   _T("BLUESHIFT_FAKE_WINDOW")

enum GLContextProfile {
    CompatibilityProfile,
    CoreProfile,
    ES2Profile      // ES2 profile including ES3
};

static GLContextProfile contextProfile = CoreProfile;
static int          contextMajorVersion = 3;
static int          contextMinorVersion = 2;

static int          majorVersion;
static int          minorVersion;

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

static int ChooseBestPixelFormat(HDC hDC, int inColorBits, int inAlphaBits, int inDepthBits, int inStencilBits, int inMultiSamples) { 
    int	best = 0;

    BE_LOG(L"ChoosePixelFormat(%i, %i, %i, %i)\n", inColorBits, inAlphaBits, inDepthBits, inStencilBits);

    if (!gwglChoosePixelFormatARB) {
        // DC 에서 지원하는 모든 pixel format 개수를 얻어온다
        unsigned int numFormats = DescribePixelFormat(hDC, 0, 0, nullptr);
        if (numFormats > MAX_PIXEL_FORMAT) {
            BE_WARNLOG(L"numFormats > MAX_PIXEL_FORMAT\n");
            numFormats = MAX_PIXEL_FORMAT;
        } else if (numFormats < 1) {
            BE_FATALERROR(L"no pixel formats found");
        }

        BE_LOG(L"%i pixel formats found\n", numFormats);

        DWORD dwTargetFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

        PIXELFORMATDESCRIPTOR pfdList[MAX_PIXEL_FORMAT];
        PIXELFORMATDESCRIPTOR *pfd = pfdList;

        // positive one-based integer indexes
        for (int i = 1; i <= numFormats; i++, pfd++) {
            DescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), pfd);
        }

        pfd = pfdList;
        for (int i = 1; i <= numFormats; i++, pfd++) {
            // is supported by GDI software implementation ?
            if (pfd->dwFlags & PFD_GENERIC_FORMAT) { 
                BE_DLOG(L"PF %i rejected, software implementation\n", i);
                continue;
            }

            if ((pfd->dwFlags & dwTargetFlags) != dwTargetFlags) {
                BE_DLOG(L"PF %i rejected, improper flags (%x instead of %x)\n", i, pfd->dwFlags, dwTargetFlags);
                continue;
            }

            // is color index pixel type ?
            if (pfd->iPixelType != PFD_TYPE_RGBA) { 
                BE_DLOG(L"PF %i rejected, not RGBA\n", i);
                continue;
            }

            BE_DLOG(L"PF %3i: color(%2i-bits) alpha(%2i-bits), depth(%2i-bits) stencil(%2i-bits)\n", i, pfd->cColorBits, pfd->cAlphaBits, pfd->cDepthBits, pfd->cStencilBits);

            if (pfd->cDepthBits < 16 && inDepthBits > 0) {
                continue;
            }

            if (pfd->cStencilBits < 4 && inStencilBits > 0) {
                continue;
            }

            if (!best) {
                best = i;
                continue;
            }

            // check color bits
            if (pfdList[best-1].cColorBits != inColorBits) {
                if (pfd->cColorBits == inColorBits || pfd->cColorBits > pfdList[best-1].cColorBits) {
                    best = i;
                    continue;
                }
            }

            // check alpha bits
            if (pfdList[best-1].cAlphaBits != inAlphaBits) {
                if (pfd->cAlphaBits == inAlphaBits || pfd->cAlphaBits > pfdList[best-1].cAlphaBits) {
                    best = i;
                    continue;
                }
            }

            // check depth bits
            if (pfdList[best-1].cDepthBits != inDepthBits) {
                if (pfd->cDepthBits == inDepthBits || pfd->cDepthBits > pfdList[best-1].cDepthBits) {
                    best = i;
                    continue;
                }
            }

            // check stencil bits
            if (pfdList[best-1].cStencilBits != inStencilBits) {
                if (pfd->cStencilBits == inStencilBits || (pfd->cStencilBits > inStencilBits && inStencilBits > 0)) {
                    best = i;
                    continue;
                }
            }
        }

        // best PFD choosed !!
        if (!(best = ::ChoosePixelFormat(hDC, &pfdList[best-1]))) {
            BE_FATALERROR(L"ChoosePixelFormat: failed");
        }
    } else {
        int results[MAX_ATTRIB_SIZE];
        int attribs[MAX_ATTRIB_SIZE];
        attribs[0] = WGL_NUMBER_PIXEL_FORMATS_ARB;
        gwglGetPixelFormatAttribivARB(hDC, 0, 0, 1, attribs, results);
        unsigned int numFormats = results[0];

        if (numFormats > MAX_PIXEL_FORMAT) {
            BE_WARNLOG(L"numFormats > MAX_PIXEL_FORMAT\n");
            numFormats = MAX_PIXEL_FORMAT;
        } else if (numFormats < 1) {
            BE_FATALERROR(L"no pixel formats found");
        }

        BE_LOG(L"%i pixel formats found\n", numFormats);

        int numAttribs = 0;
        attribs[numAttribs++] = WGL_SUPPORT_OPENGL_ARB;
        attribs[numAttribs++] = WGL_ACCELERATION_ARB;
        attribs[numAttribs++] = WGL_DOUBLE_BUFFER_ARB;
        attribs[numAttribs++] = WGL_DRAW_TO_WINDOW_ARB;
        attribs[numAttribs++] = WGL_PIXEL_TYPE_ARB;
        attribs[numAttribs++] = WGL_COLOR_BITS_ARB;
        attribs[numAttribs++] = WGL_ALPHA_BITS_ARB;
        attribs[numAttribs++] = WGL_DEPTH_BITS_ARB;
        attribs[numAttribs++] = WGL_STENCIL_BITS_ARB;
        attribs[numAttribs++] = WGL_SAMPLE_BUFFERS_ARB;
        attribs[numAttribs++] = WGL_SAMPLES_ARB;
        attribs[numAttribs++] = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;
        attribs[numAttribs++] = 0;

        int attribList[MAX_PIXEL_FORMAT][MAX_ATTRIB_SIZE];
        for (int i = 1; i <= numFormats; i++) {
            gwglGetPixelFormatAttribivARB(hDC, i, 0, numAttribs, attribs, attribList[i-1]);
        }

        for (int i = 1; i <= numFormats; i++) {
            int *attr = attribList[i-1];

            // WGL_SUPPORT_OPENGL_ARB
            if (attr[0] != GL_TRUE) {
                BE_DLOG(L"PF %i rejected, software implementation\n", i);
                continue;
            }

            // WGL_ACCELERATION_ARB
            if (attr[1] != WGL_FULL_ACCELERATION_ARB) {
                BE_DLOG(L"PF %i rejected, full hw-acceleration required\n", i);
                continue;
            }

            // WGL_DOUBLE_BUFFER_ARB
            if (attr[2] != GL_TRUE) {
                BE_DLOG(L"PF %i rejected, double buffer required\n", i);
                continue;
            }

            // WGL_DRAW_TO_WINDOW_ARB
            if (attr[3] != GL_TRUE) {
                BE_DLOG(L"PF %i rejected, draw to windows required\n", i);
                continue;
            }

            // WGL_PIXEL_TYPE_ARB
            if (attr[4] != WGL_TYPE_RGBA_ARB) {
                BE_DLOG(L"PF %i rejected, not RGBA\n", i);
                continue;
            }

            BE_DLOG(L"PF %3i: color(%2i-bits) alpha(%2i-bits) depth(%2i-bits), stencil(%2i-bits), multisamples(%2ix)\n", i, attr[5], attr[6], attr[7], attr[8], attr[10]);

            // WGL_ALPHA_BITS_ARB
            if (attr[6] <= 0 && inAlphaBits > 0) {
                continue;
            }

            // WGL_DEPTH_BITS_ARB
            if (attr[7] < 16 && inDepthBits > 0) {
                continue;
            }

            // WGL_STENCIL_BITS_ARB
            if (attr[8] < 4 && inStencilBits > 0) {
                continue;
            }

            // WGL_SAMPLE_BUFFERS_ARB
            if (attr[9] == GL_FALSE && inMultiSamples > 0) {
                continue;
            }

            // WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB
            if (attr[11] == GL_FALSE) {
                continue;
            }

            if (!best) {
                best = i;
                continue;
            }

            // check color bits
            if (attribList[best-1][5] != inColorBits) {
                if (attr[5] == inColorBits || attr[5] > attribList[best-1][5]) {
                    best = i;
                    continue;
                }
            }

            // check alpha bits
            if (attribList[best-1][6] != inAlphaBits) {
                if (attr[6] == inAlphaBits || attr[6] > attribList[best-1][6]) {
                    best = i;
                    continue;
                }
            }

            // check depth bits
            if (attribList[best-1][7] != inDepthBits) {
                if (attr[7] == inDepthBits || attr[7] > attribList[best-1][7]) {
                    best = i;
                    continue;
                }
            }

            // check stencil bits
            if (attribList[best-1][8] != inStencilBits) {
                if (attr[8] == inStencilBits || (attr[8] > inStencilBits && inStencilBits > 0)) {
                    best = i;
                    continue;
                }
            }

            // check multi samples
            if (attribList[best-1][10] != inMultiSamples) {
                if (attr[10] == inMultiSamples || attr[10] > attribList[best-1][10]) {
                    best = i;
                    continue;
                }
            }
        }

        numAttribs = 0;
        attribs[numAttribs++] = WGL_SUPPORT_OPENGL_ARB;
        attribs[numAttribs++] = attribList[best-1][0];
        attribs[numAttribs++] = WGL_ACCELERATION_ARB;
        attribs[numAttribs++] = attribList[best-1][1];
        attribs[numAttribs++] = WGL_DOUBLE_BUFFER_ARB;
        attribs[numAttribs++] = attribList[best-1][2];
        attribs[numAttribs++] = WGL_DRAW_TO_WINDOW_ARB;
        attribs[numAttribs++] = attribList[best-1][3];
        attribs[numAttribs++] = WGL_PIXEL_TYPE_ARB;
        attribs[numAttribs++] = attribList[best-1][4];
        attribs[numAttribs++] = WGL_COLOR_BITS_ARB;
        attribs[numAttribs++] = attribList[best-1][5];
        attribs[numAttribs++] = WGL_ALPHA_BITS_ARB;
        attribs[numAttribs++] = attribList[best-1][6];
        attribs[numAttribs++] = WGL_DEPTH_BITS_ARB;
        attribs[numAttribs++] = attribList[best-1][7];
        attribs[numAttribs++] = WGL_STENCIL_BITS_ARB;
        attribs[numAttribs++] = attribList[best-1][8];
        attribs[numAttribs++] = WGL_SAMPLE_BUFFERS_ARB;
        attribs[numAttribs++] = attribList[best-1][9];
        attribs[numAttribs++] = WGL_SAMPLES_ARB;
        attribs[numAttribs++] = attribList[best-1][10];
        attribs[numAttribs++] = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;
        attribs[numAttribs++] = attribList[best - 1][11];

        attribs[numAttribs++] = 0;
        attribs[numAttribs++] = 0;

        if (!gwglChoosePixelFormatARB(hDC, attribs, nullptr, 1, &best, &numFormats)) {
            BE_FATALERROR(L"gwglChoosePixelFormatARB: failed");
        }
    }

    BE_LOG(L"PIXELFORMAT %i choosed\n", best);

    return best;
}

static HGLRC CreateContextAttribs(HDC hdc, HGLRC hSharedContext, GLContextProfile contextProfile, int majorVersion, int minorVersion) {
    int contextFlags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
    if (gl_debug.GetBool()) {
        if (gglext._GL_ARB_debug_output) {
            contextFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
        }
    }

    int profileMask = 0;
    switch (contextProfile) {
    case CompatibilityProfile:
        profileMask |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        break;
    case CoreProfile:
        profileMask |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
        break;
    case ES2Profile:
        profileMask |= WGL_CONTEXT_ES2_PROFILE_BIT_EXT;
        break;
    }

    int attribs[16];
    int numAttribs = 0;
    if (contextProfile != CompatibilityProfile) {
        attribs[numAttribs++] = WGL_CONTEXT_MAJOR_VERSION_ARB;
        attribs[numAttribs++] = majorVersion;
        attribs[numAttribs++] = WGL_CONTEXT_MINOR_VERSION_ARB;
        attribs[numAttribs++] = minorVersion;
    }
    attribs[numAttribs++] = WGL_CONTEXT_FLAGS_ARB;
    attribs[numAttribs++] = contextFlags;
    attribs[numAttribs++] = WGL_CONTEXT_PROFILE_MASK_ARB;
    attribs[numAttribs++] = profileMask;
    attribs[numAttribs++] = 0;

    return gwglCreateContextAttribsARB(hdc, hSharedContext, attribs);
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

static void InitGLFunctions() {
    HWND hwndFake = CreateFakeWindow();
    HDC hdcFake = GetDC(hwndFake);

    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(hdcFake, &pfd);
    if (!SetPixelFormat(hdcFake, pixelFormat, nullptr)) {
        BE_FATALERROR(L"set pixel format: failed");
    }
    BE_DLOG(L"set pixel format: ok\n");

    HGLRC hrcFake = wglCreateContext(hdcFake);
    if (!hrcFake) {
        BE_FATALERROR(L"Couldn't create fake RC");
    }

    wglMakeCurrent(hdcFake, hrcFake);

    // NOTE: WGL extension string 을 얻기 위해서는 RC 와 연결된 valid DC 가 필요하다
    // gwglXXX function bindings & check WGL extensions
    gwgl_init(hdcFake, false);

    // gglXXX function bindings & check GL extensions
    ggl_init(false);

    wglMakeCurrent(nullptr, nullptr);

    wglDeleteContext(hrcFake);
    ReleaseDC(hwndFake, hdcFake);
    DestroyWindow(hwndFake);
}

void OpenGLRHI::InitMainContext(WindowHandle windowHandle, const Settings *settings) {
    InitGLFunctions();
    
    // Create main context
    mainContext = new GLContext;
    mainContext->state = new GLState;
    mainContext->hwnd = CreateFakeWindow(); // Create fake window for global RC
    mainContext->hdc = GetDC(mainContext->hwnd);

    // Set PF for the main DC
    int pixelFormat = ChooseBestPixelFormat(mainContext->hdc, settings->colorBits, settings->alphaBits, settings->depthBits, settings->stencilBits, settings->multiSamples);
    if (!SetPixelFormat(mainContext->hdc, pixelFormat, nullptr)) {
        BE_FATALERROR(L"set pixel format: failed");
    }
    BE_DLOG(L"set pixel format: ok\n");

    // Create rendering context
    mainContext->hrc = CreateContextAttribs(mainContext->hdc, nullptr, contextProfile, contextMajorVersion, contextMinorVersion);
    if (!mainContext->hrc) {
        BE_FATALERROR(L"Couldn't create RC");
    }

    if (!wglMakeCurrent(mainContext->hdc, mainContext->hrc)) {
        BE_FATALERROR(L"Couldn't make current context");
    }

    GetGLVersion(&majorVersion, &minorVersion);

    if (contextProfile == CompatibilityProfile) {
        int decimalVersion = majorVersion * 10 + minorVersion;
        if (decimalVersion < 32) {
            BE_FATALERROR(L"Minimum OpenGL extensions missing !!\nRequired OpenGL 3.2 or higher graphic card");
        }
    }

    // Enable debug callback
    if (gl_debug.GetBool()) {
        if (gglext._GL_ARB_debug_output) {
            gglDebugMessageCallbackARB(OpenGL::DebugCallback, nullptr);
            gglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        } else {
            ggl_rebind(true);
        }
    }
    
    // default FBO 
    mainContext->defaultFramebuffer = 0;

    // Create default VAO for main context
    gglGenVertexArrays(1, &mainContext->defaultVAO);
}

void OpenGLRHI::FreeMainContext() {
    // Delete default VAO for main context
    gglDeleteVertexArrays(1, &mainContext->defaultVAO);

    wglMakeCurrent(nullptr, nullptr);

    if (!wglDeleteContext(mainContext->hrc)) {
        BE_FATALERROR(L"deleting main context RC: failed");
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

    // All rendering contexts use an identical pixel format
    int pixelFormat = GetPixelFormat(mainContext->hdc);
    if (!SetPixelFormat(ctx->hdc, pixelFormat, nullptr)) {
        BE_FATALERROR(L"set pixel format: failed");	
    }
    BE_DLOG(L"set pixel format: ok\n");

    if (!useSharedContext) {
        // main context will be reused
        ctx->state = mainContext->state;
        ctx->hrc = mainContext->hrc;
        ctx->defaultVAO = mainContext->defaultVAO;
    } else {
        ctx->state = new GLState;

        if (gwglCreateContextAttribsARB) {
            ctx->hrc = CreateContextAttribs(ctx->hdc, mainContext->hrc, contextProfile, contextMajorVersion, contextMinorVersion);
            if (!ctx->hrc) {
                BE_FATALERROR(L"Couldn't create RC");
            }

            wglMakeCurrent(nullptr, nullptr); 
        } else {
            ctx->hrc = wglCreateContext(ctx->hdc);
            if (!ctx->hrc) {
                BE_FATALERROR(L"Couldn't create RC");
            }

            // Allow sharing of all display list and texture objects between rendering context
            //
            // NOTE: The best time to call wglShareLists is after creating the GL contexts 
            // you want to share, but before you create any objects in either of the contexts. 
            // If you create objects, then there is a chance that wglShareLists will fail.
            wglShareLists(mainContext->hrc, ctx->hrc);
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

    if (ctx->hrc != mainContext->hrc) {
        // Delete default VAO for shared context
        gglDeleteVertexArrays(1, &ctx->defaultVAO);

        if (!wglDeleteContext(ctx->hrc)) {
            BE_FATALERROR(L"deleting RC: failed");
        }
        BE_DLOG(L"deleting RC: ok\n");

        delete ctx->state;
    }

    if (currentContext == ctx) {
        currentContext = mainContext;

        wglMakeCurrent(mainContext->hdc, mainContext->hrc);
    }
    
    ReleaseDC(ctx->hwnd, ctx->hdc);

    SetWindowLongPtr(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)ctx->oldWndProc);

    delete ctx;
    contextList[ctxHandle] = nullptr; 
}

void OpenGLRHI::ActivateSurface(Handle ctxHandle, RHI::WindowHandle windowHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
}

void OpenGLRHI::DeactivateSurface(Handle ctxHandle) {
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];
}

void OpenGLRHI::SetContext(Handle ctxHandle) {
    HDC currentDC = wglGetCurrentDC();
    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    if (currentDC != ctx->hdc) {
        if (currentDC) {
            gglFlush();
        }
        wglMakeCurrent(currentContext->hdc, nullptr);
    }

    if (!wglMakeCurrent(ctx->hdc, ctx->hrc)) {
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

void OpenGLRHI::GetDisplayMetrics(Handle ctxHandle, DisplayMetrics *displayMetrics) const {
    const GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

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
    displayMetrics->screenWidth = rc.right / dpiScale;
    displayMetrics->screenHeight = rc.bottom / dpiScale;
    displayMetrics->backingWidth = rc.right;
    displayMetrics->backingHeight = rc.bottom;
    displayMetrics->safeAreaInsets.Set(0, 0, 0, 0);
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

    BOOL succeeded = ::SwapBuffers(currentContext->hdc);
    if (!succeeded) {
        WStr lastErrorText = PlatformWinProcess::GetLastErrorText();
        BE_WARNLOG(L"Failed to SwapBuffers : %ls", lastErrorText.c_str());
        return false;
    }

    if (gl_debug.IsModified()) {
        gl_debug.ClearModified();

        if (gglext._GL_ARB_debug_output) {
            if (gl_debug.GetBool()) {
                gglDebugMessageCallbackARB(OpenGL::DebugCallback, nullptr);
                gglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
            } else {
                gglDebugMessageCallbackARB(nullptr, nullptr);
                gglDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
            }
        } else {
            ggl_rebind(gl_debug.GetBool());
            gwgl_rebind(gl_debug.GetBool());
        }
    }

    return true;
}

void OpenGLRHI::SwapInterval(int interval) const {
    gwglSwapIntervalEXT(interval);
}

BE_NAMESPACE_END
