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
#include "Platform/PlatformTime.h"
#include "Profiler/Profiler.h"
#include <tchar.h>

BE_NAMESPACE_BEGIN

#define MAX_PIXEL_FORMAT        1200
#define MAX_ATTRIB_SIZE         32

#define FAKE_WINDOW_CLASSNAME   _T("BLUESHIFT_FAKE_WINDOW")

enum GLContextProfile {
    CompatibilityProfile,
    CoreProfile,
    ES2Profile      // ES2 profile including ES3
};

static int          majorVersion;
static int          minorVersion;

static bool         deviceFullscreen = false;
static int          deviceBpp = 0;
static int          deviceHz = 0;
static HGLRC        hrcMain;

static CVar         gl_debug("gl_debug", "0", CVar::Flag::Bool, "");
static CVar         gl_debugLevel("gl_debugLevel", "3", CVar::Flag::Integer, "");
static CVar         gl_ignoreError("gl_ignoreError", "0", CVar::Flag::Bool, "");
static CVar         gl_finish("gl_finish", "0", CVar::Flag::Bool, "");

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
            BE_FATALERROR("Couldn't register fake window class");
        }
    }

    HWND hwndFake = CreateWindowEx(0, FAKE_WINDOW_CLASSNAME, 
        _T("FAKE"), WS_POPUP, 0, 0, 0, 0, 
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwndFake) {
        BE_FATALERROR("Couldn't create fake window");
    }

    // hide fake window
    ShowWindow(hwndFake, SW_HIDE);

    return hwndFake;
}

static int ChooseBestPixelFormat(HDC hDC, int inColorBits, int inAlphaBits, int inDepthBits, int inStencilBits, int inMultiSamples) { 
    int	best = 0;

    BE_LOG("ChoosePixelFormat(%i, %i, %i, %i)\n", inColorBits, inAlphaBits, inDepthBits, inStencilBits);

    if (!gwglChoosePixelFormatARB) {
        // Get the number of pixel formats supported by this DC.
        unsigned int numFormats = DescribePixelFormat(hDC, 0, 0, nullptr);
        if (numFormats > MAX_PIXEL_FORMAT) {
            BE_WARNLOG("numFormats(%i) > MAX_PIXEL_FORMAT(%i)\n", numFormats, MAX_PIXEL_FORMAT);
            numFormats = MAX_PIXEL_FORMAT;
        } else if (numFormats < 1) {
            BE_FATALERROR("no pixel formats found");
        }

        BE_LOG("%i pixel formats found\n", numFormats);

        DWORD dwTargetFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

        PIXELFORMATDESCRIPTOR pfdList[MAX_PIXEL_FORMAT];
        PIXELFORMATDESCRIPTOR *pfd = pfdList;

        // Use positive one-based integer indexes
        for (int i = 1; i <= numFormats; i++, pfd++) {
            DescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), pfd);
        }

        pfd = pfdList;
        for (int i = 1; i <= numFormats; i++, pfd++) {
            // Is supported by GDI software implementation ?
            if (pfd->dwFlags & PFD_GENERIC_FORMAT) { 
                BE_DLOG("PF %i rejected, software implementation\n", i);
                continue;
            }

            if ((pfd->dwFlags & dwTargetFlags) != dwTargetFlags) {
                BE_DLOG("PF %i rejected, improper flags (%x instead of %x)\n", i, pfd->dwFlags, dwTargetFlags);
                continue;
            }

            // Is color index pixel type ?
            if (pfd->iPixelType != PFD_TYPE_RGBA) { 
                BE_DLOG("PF %i rejected, not RGBA\n", i);
                continue;
            }

            BE_DLOG("PF %3i: color(%2i-bits) alpha(%2i-bits), depth(%2i-bits) stencil(%2i-bits)\n", i, pfd->cColorBits, pfd->cAlphaBits, pfd->cDepthBits, pfd->cStencilBits);

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

            // Check color bits
            if (pfdList[best-1].cColorBits != inColorBits) {
                if (pfd->cColorBits == inColorBits || pfd->cColorBits > pfdList[best-1].cColorBits) {
                    best = i;
                    continue;
                }
            }

            // Check alpha bits
            if (pfdList[best-1].cAlphaBits != inAlphaBits) {
                if (pfd->cAlphaBits == inAlphaBits || pfd->cAlphaBits > pfdList[best-1].cAlphaBits) {
                    best = i;
                    continue;
                }
            }

            // Check depth bits
            if (pfdList[best-1].cDepthBits != inDepthBits) {
                if (pfd->cDepthBits == inDepthBits || pfd->cDepthBits > pfdList[best-1].cDepthBits) {
                    best = i;
                    continue;
                }
            }

            // Check stencil bits
            if (pfdList[best-1].cStencilBits != inStencilBits) {
                if (pfd->cStencilBits == inStencilBits || (pfd->cStencilBits > inStencilBits && inStencilBits > 0)) {
                    best = i;
                    continue;
                }
            }
        }

        // Best PFD choosed !!
        if (!(best = ::ChoosePixelFormat(hDC, &pfdList[best-1]))) {
            BE_FATALERROR("ChoosePixelFormat: failed");
        }
    } else {
        int results[MAX_ATTRIB_SIZE];
        int attribs[MAX_ATTRIB_SIZE];
        attribs[0] = WGL_NUMBER_PIXEL_FORMATS_ARB;
        gwglGetPixelFormatAttribivARB(hDC, 0, 0, 1, attribs, results);
        unsigned int numFormats = results[0];

        if (numFormats > MAX_PIXEL_FORMAT) {
            BE_WARNLOG("numFormats(%i) > MAX_PIXEL_FORMAT(%i)\n", numFormats, MAX_PIXEL_FORMAT);
            numFormats = MAX_PIXEL_FORMAT;
        } else if (numFormats < 1) {
            BE_FATALERROR("no pixel formats found");
        }

        BE_LOG("%i pixel formats found\n", numFormats);

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
                BE_DLOG("PF %i rejected, software implementation\n", i);
                continue;
            }

            // WGL_ACCELERATION_ARB
            if (attr[1] != WGL_FULL_ACCELERATION_ARB) {
                BE_DLOG("PF %i rejected, full hw-acceleration required\n", i);
                continue;
            }

            // WGL_DOUBLE_BUFFER_ARB
            if (attr[2] != GL_TRUE) {
                BE_DLOG("PF %i rejected, double buffer required\n", i);
                continue;
            }

            // WGL_DRAW_TO_WINDOW_ARB
            if (attr[3] != GL_TRUE) {
                BE_DLOG("PF %i rejected, draw to windows required\n", i);
                continue;
            }

            // WGL_PIXEL_TYPE_ARB
            if (attr[4] != WGL_TYPE_RGBA_ARB) {
                BE_DLOG("PF %i rejected, not RGBA\n", i);
                continue;
            }

            BE_DLOG("PF %3i: color(%2i-bits) alpha(%2i-bits) depth(%2i-bits), stencil(%2i-bits), multisamples(%2ix)\n", i, attr[5], attr[6], attr[7], attr[8], attr[10]);

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
            BE_FATALERROR("gwglChoosePixelFormatARB: failed");
        }
    }

    BE_LOG("PIXELFORMAT %i choosed\n", best);

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
    // GL_MAJOR_VERSION and GL_MINOR_VERSION queries are supported from 3.0 or higher core context.
    glGetIntegerv(GL_MAJOR_VERSION, major);
    glGetIntegerv(GL_MINOR_VERSION, minor);
#else
    // Use glGetString if context has not been created yet.
    const char *verstr = (const char *)glGetString(GL_VERSION);
    if (!verstr || sscanf(verstr, "%d.%d", major, minor) != 2) {
        *major = *minor = 0;
    }
#endif
}

static void GetContextVersionFromCommandLine(int &majorVersion, int &minorVersion) {
    majorVersion = 4;
    minorVersion = 3;
    //majorVersion = 3;
    //minorVersion = 2;
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
        BE_FATALERROR("set pixel format: failed");
    }
    BE_DLOG("set pixel format: ok\n");

    HGLRC hrcFake = wglCreateContext(hdcFake);
    if (!hrcFake) {
        BE_FATALERROR("Couldn't create fake RC");
    }

    wglMakeCurrent(hdcFake, hrcFake);

    // NOTE: To get the WGL extension string, you need a valid DC connected to RC.
    // gwglXXX function bindings & check WGL extensions
    gwgl_init(hdcFake, false);

    // gglXXX function bindings & check GL extensions
    ggl_init(gl_debug.GetBool());

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
        BE_FATALERROR("set pixel format: failed");
    }
    BE_DLOG("set pixel format: ok\n");

    GLContextProfile contextProfile = CoreProfile;
    int contextMajorVersion = 0;
    int contextMinorVersion = 0;
    GetContextVersionFromCommandLine(contextMajorVersion, contextMinorVersion);

    // Create rendering context
    mainContext->hrc = CreateContextAttribs(mainContext->hdc, nullptr, contextProfile, contextMajorVersion, contextMinorVersion);
    if (!mainContext->hrc) {
        BE_FATALERROR("Couldn't create RC");
    }

    if (!wglMakeCurrent(mainContext->hdc, mainContext->hrc)) {
        BE_FATALERROR("Couldn't make current context");
    }

    GetGLVersion(&majorVersion, &minorVersion);

    if (contextProfile == CompatibilityProfile) {
        int decimalVersion = majorVersion * 10 + minorVersion;
        if (decimalVersion < 32) {
            BE_FATALERROR("Minimum OpenGL extensions missing !!\nRequired OpenGL 3.2 or higher graphic card");
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

#ifdef ENABLE_IMGUI
    ImGuiCreateContext(mainContext);
#endif
}

void OpenGLRHI::FreeMainContext() {
#ifdef ENABLE_IMGUI
    ImGuiDestroyContext(mainContext);
#endif

    // Delete default VAO for main context
    gglDeleteVertexArrays(1, &mainContext->defaultVAO);

    wglMakeCurrent(nullptr, nullptr);

    if (!wglDeleteContext(mainContext->hrc)) {
        BE_FATALERROR("deleting main context RC: failed");
    }

    if (!ReleaseDC(mainContext->hwnd, mainContext->hdc)) {
        BE_FATALERROR("releasing main context DC: failed");
    }

    if (!DestroyWindow(mainContext->hwnd)) {
        BE_FATALERROR("destroying main context window: failed");
    }

    SAFE_DELETE(mainContext->state);
    SAFE_DELETE(mainContext);
}

//-------------------------------------------------------------------------------------------------------
// ImGui_ImplWin32_XXXX
//-------------------------------------------------------------------------------------------------------

static ImGuiMouseCursor g_LastMouseCursor = ImGuiMouseCursor_COUNT;

static bool ImGui_ImplWin32_UpdateMouseCursor() {
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
        return false;
    }

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
    } else {
        // Show OS mouse cursor
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor) {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:         win32_cursor = IDC_HAND; break;
        case ImGuiMouseCursor_NotAllowed:   win32_cursor = IDC_NO; break;
        }
        ::SetCursor(::LoadCursor(NULL, win32_cursor));
    }
    return true;
}

static void ImGui_ImplWin32_UpdateMousePos(HWND hwnd) {
    ImGuiIO &io = ImGui::GetIO();

    // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
    if (io.WantSetMousePos) {
        POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
        ::ClientToScreen(hwnd, &pos);
        ::SetCursorPos(pos.x, pos.y);
    }

    // Set mouse position
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    POINT pos;
    if (HWND active_window = ::GetForegroundWindow()) {
        if (active_window == hwnd || ::IsChild(active_window, hwnd)) {
            if (::GetCursorPos(&pos) && ::ScreenToClient(hwnd, &pos)) {
                io.MousePos = ImVec2((float)pos.x, (float)pos.y);
            }
        }
    }
}

// Allow compilation with old Windows SDK. MinGW doesn't have default _WIN32_WINNT/WINVER versions.
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef DBT_DEVNODES_CHANGED
#define DBT_DEVNODES_CHANGED 0x0007
#endif

// Win32 message handler (process Win32 mouse/keyboard inputs, etc.)
// Call from your application's message handler.
// When implementing your own back-end, you can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if Dear ImGui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to Dear ImGui, and hide them from your application based on those two flags.
// PS: In this Win32 handler, we use the capture API (GetCapture/SetCapture/ReleaseCapture) to be able to read mouse coordinates when dragging mouse outside of our window bounds.
// PS: We treat DBLCLK messages as regular mouse down messages, so this code will work on windows classes that have the CS_DBLCLKS flag set. Our own example app code doesn't set this flag.
static IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui::GetCurrentContext() == NULL) {
        return 0;
    }

    ImGuiIO &io = ImGui::GetIO();
    switch (msg) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
    {
        int button = 0;
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
        if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
        if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
        if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL) {
            ::SetCapture(hwnd);
        }
        io.MouseDown[button] = true;
        return io.WantCaptureMouse;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
    {
        int button = 0;
        if (msg == WM_LBUTTONUP) { button = 0; }
        if (msg == WM_RBUTTONUP) { button = 1; }
        if (msg == WM_MBUTTONUP) { button = 2; }
        if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
        io.MouseDown[button] = false;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd) {
            ::ReleaseCapture();
        }
        return io.WantCaptureMouse;
    }
    case WM_MOUSEWHEEL:
        io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        return io.WantCaptureMouse;
    case WM_MOUSEHWHEEL:
        io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        return io.WantCaptureMouse;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam < 256) {
            io.KeysDown[wParam] = 1;
        }
        return io.WantCaptureKeyboard;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wParam < 256) {
            io.KeysDown[wParam] = 0;
        }
        return io.WantCaptureKeyboard;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000) {
            io.AddInputCharacterUTF16((unsigned short)wParam);
        }
        return io.WantCaptureKeyboard;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && ImGui_ImplWin32_UpdateMouseCursor()) {
            return 1;
        }
        return 0;
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------------

static LRESULT CALLBACK NewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    GLContext *ctx = (GLContext *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

#ifdef ENABLE_IMGUI
    ImGuiContext *oldContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(ctx->imGuiContext);
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) {
        ImGui::SetCurrentContext(oldContext);
        return 0;
    }
    ImGui::SetCurrentContext(oldContext);
#endif

    switch (uMsg) {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (ctx->onDemandDrawing) {
                rhi.DisplayContext(ctx->handle);
            }
            EndPaint(hwnd, &ps);
            break;
        }
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
        BE_FATALERROR("get DC: failed");
    }
    
    SetWindowLongPtr(ctx->hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
    ctx->oldWndProc = (WNDPROC)SetWindowLongPtr(ctx->hwnd, GWLP_WNDPROC, (LONG_PTR)NewWndProc);

    // All rendering contexts use an identical pixel format
    int pixelFormat = GetPixelFormat(mainContext->hdc);
    if (!SetPixelFormat(ctx->hdc, pixelFormat, nullptr)) {
        BE_FATALERROR("set pixel format: failed");
    }
    BE_DLOG("set pixel format: ok\n");

    if (!useSharedContext) {
        // main context will be reused
        ctx->state = mainContext->state;
        ctx->hrc = mainContext->hrc;
        ctx->defaultVAO = mainContext->defaultVAO;
    } else {
        ctx->state = new GLState;

        if (gwglCreateContextAttribsARB) {
            GLContextProfile contextProfile = CoreProfile;
            int contextMajorVersion = 0;
            int contextMinorVersion = 0;
            GetContextVersionFromCommandLine(contextMajorVersion, contextMinorVersion);

            ctx->hrc = CreateContextAttribs(ctx->hdc, mainContext->hrc, contextProfile, contextMajorVersion, contextMinorVersion);
            if (!ctx->hrc) {
                BE_FATALERROR("Couldn't create RC");
            }

            wglMakeCurrent(nullptr, nullptr); 
        } else {
            ctx->hrc = wglCreateContext(ctx->hdc);
            if (!ctx->hrc) {
                BE_FATALERROR("Couldn't create RC");
            }

            // Allow sharing of all display list and texture objects between rendering context
            //
            // NOTE: The best time to call wglShareLists is after creating the GL contexts 
            // you want to share, but before you create any objects in either of the contexts. 
            // If you create objects, then there is a chance that wglShareLists will fail.
            wglShareLists(mainContext->hrc, ctx->hrc);
        }
    }

#ifdef ENABLE_IMGUI
    ctx->imGuiContext = mainContext->imGuiContext;
    ctx->imGuiLastTime = PlatformTime::Seconds();
#endif

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
            BE_FATALERROR("deleting RC: failed");
        }
        BE_DLOG("deleting RC: ok\n");

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

void OpenGLRHI::ActivateSurface(Handle ctxHandle, WindowHandle windowHandle) {
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
        BE_FATALERROR("OpenGLRHI::SetContext: Couldn't make current context");
    }

    this->currentContext = ctx;

#ifdef ENABLE_IMGUI
    ImGui::SetCurrentContext(ctx->imGuiContext);
#endif
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
    float dpiScale = Math::Round(dpi / 96.0f);
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

    BE_LOG("Changing display setting...\n");
    
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

            BE_LOG("using color bits of %i\n", 16);
        } else {
            BE_LOG("using desktop display depth of %i\n", bpp);
        }
    }

    if (hz > 0) {
        dm.dmDisplayFrequency = hz;
        dm.dmFields |= DM_DISPLAYFREQUENCY;
    }
    
    if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
        BE_WARNLOG("Can't change fullscreen mode");
        return false;
    }

    deviceFullscreen = true;

    hdc = GetDC(GetDesktopWindow());
    deviceBpp = GetDeviceCaps(hdc, BITSPIXEL);
    deviceHz = GetDeviceCaps(hdc, VREFRESH);
    ReleaseDC(nullptr, hdc);

    BE_LOG("set fullscreen mode: %ix%i %ibpp %ihz\n", width, height, deviceBpp, deviceHz);
    return true;
}

void OpenGLRHI::ResetFullscreen(Handle ctxHandle) {
    if (!deviceFullscreen) {
        return;
    }

    deviceFullscreen = false;

    BE_LOG("resetting display setting: ");
    if (ChangeDisplaySettings(nullptr, 0) == DISP_CHANGE_SUCCESSFUL) {
        BE_LOG("ok\n");
    } else {
        BE_LOG("failed\n");
    }

    HDC hdc = GetDC(nullptr);
    deviceBpp = GetDeviceCaps(hdc, BITSPIXEL);
    deviceHz = GetDeviceCaps(hdc, VREFRESH);
    ReleaseDC(nullptr, hdc);

    BE_LOG("set window mode: %ibpp %ihz\n", deviceBpp, deviceHz);
}

void OpenGLRHI::GetGammaRamp(unsigned short ramp[768]) const {
    ::GetDeviceGammaRamp(currentContext->hdc, ramp);
}

void OpenGLRHI::SetGammaRamp(unsigned short ramp[768]) const {
    ::SetDeviceGammaRamp(currentContext->hdc, ramp);
}

bool OpenGLRHI::SwapBuffers() {
    BE_PROFILE_CPU_SCOPE_STATIC("OpenGLRHI::SwapBuffers");
    BE_PROFILE_GPU_SCOPE_STATIC("OpenGLRHI::SwapBuffers");

    if (!gl_ignoreError.GetBool()) {
        CheckError("OpenGLRHI::SwapBuffers");
    }

    if (gl_finish.GetBool()) {
        gglFinish();
    }

    BOOL succeeded = ::SwapBuffers(currentContext->hdc);
    if (!succeeded) {
        Str lastErrorText = PlatformWinProcess::GetLastErrorText();
        BE_WARNLOG("Failed to SwapBuffers : %s", lastErrorText.c_str());
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

void OpenGLRHI::ImGuiCreateContext(GLContext *ctx) {
    // Setup Dear ImGui context
    ctx->imGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx->imGuiContext);
    ImGuiIO &io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    io.IniFilename = nullptr;

    // Setup back-end capabilities flags
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendPlatformName = "OpenGLRHI-Windows";
    io.ImeWindowHandle = ctx->hwnd;

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Space] = VK_SPACE;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';

    ImGui_ImplOpenGL_Init("#version 130");

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    //io.Fonts->AddFontFromFileTTF("../../../Data/EngineFonts/consola.ttf", 13);
}

void OpenGLRHI::ImGuiDestroyContext(GLContext *ctx) {
    ImGui_ImplOpenGL_Shutdown();

    ImGui::DestroyContext(ctx->imGuiContext);
}

void OpenGLRHI::ImGuiBeginFrame(Handle ctxHandle) {
    BE_PROFILE_CPU_SCOPE_STATIC("OpenGLRHI::ImGuiBeginFrame");

    ImGui_ImplOpenGL_ValidateFrame();

    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    DisplayMetrics dm;
    GetDisplayMetrics(ctxHandle, &dm);

    // Setup display size (every frame to accommodate for window resizing)
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(dm.screenWidth, dm.screenHeight);

    // Setup time step
    double currentTime = PlatformTime::Seconds();
    io.DeltaTime = currentTime - ctx->imGuiLastTime;
    ctx->imGuiLastTime = currentTime;

    // Read keyboard modifiers inputs
    io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
    io.KeySuper = false;
    // io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

    // Update OS mouse position
    ImGui_ImplWin32_UpdateMousePos(ctx->hwnd);

    // Update OS mouse cursor with the cursor requested by imgui
    ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (g_LastMouseCursor != mouse_cursor) {
        g_LastMouseCursor = mouse_cursor;
        ImGui_ImplWin32_UpdateMouseCursor();
    }

    ImGui::NewFrame();
}

void OpenGLRHI::ImGuiRender() {
    BE_PROFILE_CPU_SCOPE_STATIC("OpenGLRHI::ImGuiRender");
    BE_PROFILE_GPU_SCOPE_STATIC("OpenGLRHI::ImGuiRender");

    ImGui::Render();

    bool sRGBWriteEnabled = OpenGL::SupportsFrameBufferSRGB() && IsSRGBWriteEnabled();
    if (sRGBWriteEnabled) {
        SetSRGBWrite(false);
    }

    ImGui_ImplOpenGL_RenderDrawData(ImGui::GetDrawData());

    if (sRGBWriteEnabled) {
        SetSRGBWrite(true);
    }
}

void OpenGLRHI::ImGuiEndFrame() {
    BE_PROFILE_CPU_SCOPE_STATIC("OpenGLRHI::ImGuiEndFrame");

    ImGui::EndFrame();
}

BE_NAMESPACE_END
