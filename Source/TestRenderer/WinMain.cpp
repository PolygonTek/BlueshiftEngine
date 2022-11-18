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
#include "Platform/Windows/PlatformWinUtils.h"
#include "WinResource.h"
#include "Application.h"
#include <tchar.h>

//#define CREATE_SUB_WINDOW
#define USE_SHARED_CONTEXT  false

static const TCHAR *        mainWindowClassName  = _T("BLUESHIFT_MAIN_WINDOW");
static const TCHAR *        subWindowClassName   = _T("BLUESHIFT_SUB_WINDOW");

static TCHAR                szTitle[100];    // The title bar text

static BE1::RHI::Handle     mainContext = BE1::RHI::NullContext;
static BE1::RHI::Handle     subContext = BE1::RHI::NullContext;

static BE1::RHI::Handle     mainRenderTarget = BE1::RHI::NullRenderTarget;
static BE1::RHI::Handle     subRenderTarget = BE1::RHI::NullRenderTarget;

LRESULT CALLBACK            MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK            SubWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void SystemLog(int logLevel, const char *text) {
    int len = BE1::PlatformWinUtils::UTF8ToUCS2(text, nullptr, 0);
    wchar_t *wText = (wchar_t *)alloca(sizeof(wchar_t) * len);
    BE1::PlatformWinUtils::UTF8ToUCS2(text, wText, len);

    OutputDebugString(wText);
}

static void SystemError(int errLevel, const char *text) {
    int len = BE1::PlatformWinUtils::UTF8ToUCS2(text, nullptr, 0);
    wchar_t *wText = (wchar_t *)alloca(sizeof(wchar_t) * len);
    BE1::PlatformWinUtils::UTF8ToUCS2(text, wText, len);

    HWND hwnd = FindWindow(mainWindowClassName, nullptr);
    MessageBox(hwnd, wText, L"Error", MB_OK);
    if (errLevel == BE1::ErrorLevel::Fatal) {
        exit(0);
    }
}

static HWND CreateRenderWindow(const TCHAR *title, const TCHAR *classname, int width, int height, bool fullscreen) {
    int style = WS_VISIBLE;
    int styleEx;
    BE1::Rect windowRect;

    if (fullscreen) {
        styleEx = WS_EX_TOPMOST;
        style |= WS_POPUP;
    } else {
        styleEx = 0;
        style |= WS_OVERLAPPEDWINDOW;
    }

    RECT rect;
    ::SetRect(&rect, 0, 0, width, height);
    ::AdjustWindowRect(&rect, style, 0);

    if (fullscreen) {
        windowRect.x = 0;
        windowRect.y = 0;
    } else {
        windowRect.x = CW_USEDEFAULT;
        windowRect.y = CW_USEDEFAULT;
    }

    windowRect.w = rect.right - rect.left;
    windowRect.h = rect.bottom - rect.top;

    HWND hwnd = ::CreateWindowEx(
        styleEx,
        classname,
        title,
        style,
        windowRect.x, windowRect.y, windowRect.w, windowRect.h, 
        nullptr, nullptr, (HINSTANCE)GetModuleHandle(nullptr), nullptr);

    //::SetTimer(hwnd, 0, 100, nullptr);

    ::SetForegroundWindow(hwnd);
    ::SetFocus(hwnd);

    return hwnd;
}

static void ChangeRenderWindow(HWND hwnd, int width, int height, bool fullscreen) {
    int style = GetWindowLong(hwnd, GWL_STYLE);

    if (fullscreen) {
        style &= ~WS_OVERLAPPEDWINDOW;
        style |= WS_POPUP;
        SetWindowLong(hwnd, GWL_STYLE, style);
        SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST);
    } else {
        style &= ~WS_POPUP;
        style |= WS_OVERLAPPEDWINDOW;
        SetWindowLong(hwnd, GWL_STYLE, style);
        SetWindowLong(hwnd, GWL_EXSTYLE, 0);
        ShowWindow(hwnd, SW_RESTORE);
    }

    RECT rect;
    ::SetRect(&rect, 0, 0, width, height);
    ::AdjustWindowRect(&rect, style, 0);
    
    BE1::Rect windowRect;
    windowRect.w = rect.right - rect.left;
    windowRect.h = rect.bottom - rect.top;

    if (fullscreen) {
        windowRect.x = 0;
        windowRect.y = 0;
    } else {
        windowRect.x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.w) >> 1;
        windowRect.y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.h) >> 1;
    }
    
    ::MoveWindow(hwnd, windowRect.x, windowRect.y, windowRect.w, windowRect.h, FALSE);	

    ::SetForegroundWindow(hwnd);
    ::SetFocus(hwnd);
}

static void ToggleFullscreen(BE1::RHI::Handle ctx) {
    int width = 1024;
    int height = 768;

    HWND hwnd = (HWND)BE1::rhi.GetWindowHandleFromContext(ctx);

    if (!BE1::rhi.IsFullscreen()) {
        ChangeRenderWindow(hwnd, width, height, true);
        BE1::rhi.SetFullscreen(ctx, width, height);
    } else {
        BE1::rhi.ResetFullscreen(ctx);
        ChangeRenderWindow(hwnd, width, height, false);
    }
}

static void DisplayMainContext(BE1::RHI::Handle context, void *dataPtr) {
    static uint32_t t0 = BE1::PlatformTime::Milliseconds();

    uint32_t t = BE1::PlatformTime::Milliseconds() - t0;

    ::app.Draw(context, mainRenderTarget, MILLI2SEC(t));
}

static void DisplaySubContext(BE1::RHI::Handle context, void *dataPtr) {
    static uint32_t t0 = BE1::PlatformTime::Milliseconds();

    uint32_t t = BE1::PlatformTime::Milliseconds() - t0;

    ::app.Draw(context, subRenderTarget, MILLI2SEC(t));
}

static HWND CreateMainWindow(const TCHAR *title, int width, int height) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wcex;
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.style              = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc        = MainWndProc;
    wcex.hInstance          = hInstance;
    wcex.hIcon              = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTRENDERER));
    wcex.hIconSm            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcex.hCursor            = LoadCursor(nullptr, IDC_ARROW);
    //wcex.hbrBackground    = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName       = MAKEINTRESOURCE(IDC_TESTRENDERER);
    wcex.lpszClassName      = mainWindowClassName;
    RegisterClassEx(&wcex);

    HWND hwnd = CreateRenderWindow(title, mainWindowClassName, width, height, false);
    return hwnd;
}

static HWND CreateSubWindow(const TCHAR *title, int width, int height) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wcex;
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.style              = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc        = SubWndProc;
    wcex.hInstance          = hInstance;
    wcex.hIcon              = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTRENDERER));
    wcex.hIconSm            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcex.hCursor            = LoadCursor(nullptr, IDC_ARROW);
    //wcex.hbrBackground    = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszClassName      = subWindowClassName;
    RegisterClassEx(&wcex);

    HWND hwnd = CreateRenderWindow(title, subWindowClassName, width, height, false);
    return hwnd;
}

BOOL InitInstance(int nCmdShow) {
    BE1::Str basePath = BE1::PlatformFile::ExecutablePath();
    basePath.AppendPath("../../..");
    basePath.CleanPath();
    BE1::Engine::InitBase(basePath.c_str(), SystemLog, SystemError);

    char temp[128];
    BE1::Str::snPrintf(temp, sizeof(temp), "%ls %s %s %s", szTitle, BE1::PlatformProcess::PlatformName(), __DATE__, __TIME__);

    wchar_t szFullTitle[128];
    BE1::PlatformWinUtils::UTF8ToUCS2(temp, szFullTitle, COUNT_OF(szFullTitle));

    HWND hwndMain = CreateMainWindow(szFullTitle, 1024, 768);

    app.Init(hwndMain);

    app.LoadResources();

    mainContext = BE1::rhi.CreateContext(hwndMain, USE_SHARED_CONTEXT);

    BE1::rhi.SetContextDisplayFunc(mainContext, DisplayMainContext, nullptr, false);

    // FBO cannot be shared, so we should create FBO for each context
    mainRenderTarget = app.CreateRenderTarget(mainContext);

#ifdef CREATE_SUB_WINDOW
    HWND hwndSub = CreateSubWindow(_T("sub window"), 512, 384);

    subContext = BE1::rhi.CreateContext(hwndSub, USE_SHARED_CONTEXT);

    BE1::rhi.SetContextDisplayFunc(subContext, DisplaySubContext, nullptr, false);

    subRenderTarget = app.CreateRenderTarget(subContext);
#endif

    return TRUE;
}

void ShutdownInstance() {
    app.FreeResources();
    
#ifdef CREATE_SUB_WINDOW
    if (subContext) {
        BE1::rhi.DestroyRenderTarget(subRenderTarget);

        HWND hwnd = (HWND)BE1::rhi.GetWindowHandleFromContext(subContext);
        BE1::rhi.DestroyContext(subContext);

        ::DestroyWindow(hwnd);
    }
#endif

    if (mainContext) {
        BE1::rhi.DestroyRenderTarget(mainRenderTarget);

        HWND hwnd = (HWND)BE1::rhi.GetWindowHandleFromContext(mainContext);
        BE1::rhi.DestroyContext(mainContext);

        ::DestroyWindow(hwnd);
    }

    app.Shutdown();

    BE1::Engine::ShutdownBase();
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Disable automatic DPI scaling.
    SetProcessDPIAware();

    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, COUNT_OF(szTitle));

    // Perform application initialization
    if (!InitInstance(nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTRENDERER));

    while (1) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                goto QUIT;
            }

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        app.RunFrame();

        if (mainContext) {
            BE1::rhi.DisplayContext(mainContext);
        }
#ifdef CREATE_SUB_WINDOW
        if (subContext) {
            BE1::rhi.DisplayContext(subContext);
        }
#endif
    }

QUIT:
    ShutdownInstance();

    return (int)msg.wParam;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CLOSE:
        break;
    case WM_TIMER:
        if (mainContext) {
            //BE1::rhi.DisplayContext(mainContext);
        }
        break;
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
        case IDM_TOGGLE_FULLSCREEN:
            ToggleFullscreen(mainContext);
            return 0;
        case IDM_ABOUT:
            DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
            return 0;
        case IDM_EXIT:
            DestroyWindow(hwnd);
            return 0;
        }
        break; 
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK SubWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CLOSE:
        return 0; // prevent to close sub window
    case WM_TIMER:
        if (subContext) {
            //BE1::rhi.DisplayContext(subContext);
        }
        break;
    case WM_SIZE:
        break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}
