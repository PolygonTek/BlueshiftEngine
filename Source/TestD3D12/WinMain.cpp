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
#include "D3D12App.h"
#include <tchar.h>

static const TCHAR*         mainWindowClassName  = _T("BLUESHIFT_MAIN_WINDOW");

static TCHAR                szTitle[100];    // The title bar text

static HWND                 hwndMain;
static HACCEL               hAccelTable;

LRESULT CALLBACK            MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static void SystemLog(int logLevel, const char* text) {
    int len = BE1::PlatformWinUtils::UTF8ToUCS2(text, nullptr, 0);
    wchar_t* wText = (wchar_t *)alloca(sizeof(wchar_t) * len);
    BE1::PlatformWinUtils::UTF8ToUCS2(text, wText, len);

    OutputDebugString(wText);
}

static void SystemError(int errLevel, const char* text) {
    int len = BE1::PlatformWinUtils::UTF8ToUCS2(text, nullptr, 0);
    wchar_t* wText = (wchar_t *)alloca(sizeof(wchar_t) * len);
    BE1::PlatformWinUtils::UTF8ToUCS2(text, wText, len);

    HWND hwnd = FindWindow(mainWindowClassName, nullptr);
    MessageBox(hwnd, wText, L"Error", MB_OK);
    if (errLevel == BE1::ErrorLevel::Fatal) {
        exit(0);
    }
}

static HWND CreateRenderWindow(const TCHAR* title, const TCHAR* classname, int width, int height, bool fullscreen) {
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

static HWND CreateMainWindow(const TCHAR* title, int width, int height) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wcex;
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.style              = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc        = MainWndProc;
    wcex.hInstance          = hInstance;
    wcex.hIcon              = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTD3D12));
    wcex.hIconSm            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcex.hCursor            = LoadCursor(nullptr, IDC_ARROW);
    //wcex.hbrBackground    = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName       = MAKEINTRESOURCE(IDC_TESTD3D12);
    wcex.lpszClassName      = mainWindowClassName;
    RegisterClassEx(&wcex);

    HWND hwnd = CreateRenderWindow(title, mainWindowClassName, width, height, false);
    return hwnd;
}

static BOOL InitInstance(int nCmdShow) {
    BE1::Str basePath = BE1::PlatformFile::ExecutablePath();
    basePath.AppendPath("../../..");
    basePath.CleanPath();
    BE1::Engine::InitBase(basePath.c_str(), SystemLog, SystemError);

    char temp[128];
    BE1::Str::snPrintf(temp, sizeof(temp), "%ls %s %s %s", szTitle, BE1::PlatformProcess::PlatformName(), __DATE__, __TIME__);

    wchar_t szFullTitle[128];
    BE1::PlatformWinUtils::UTF8ToUCS2(temp, szFullTitle, COUNT_OF(szFullTitle));

    hwndMain = CreateMainWindow(szFullTitle, 1024, 768);

    ::ShowWindow(hwndMain, nCmdShow);

    app.Init(hwndMain);

    return TRUE;
}

static void ShutdownInstance() {
    app.Shutdown();

    BE1::Engine::ShutdownBase();
}

static bool ProcessEventLoop() {
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return true;
}

static bool RunFrameInstance(int elapsedMsec) {
    static int fpsElapsedMsec = 0;
    static int fpsFrames = 0;
    static int fps = 0;

    fpsElapsedMsec += elapsedMsec;
    fpsFrames++;

    if (fpsElapsedMsec >= 1000) {
        fps = fpsFrames / MILLI2SEC(fpsElapsedMsec);
        fpsFrames = 0;
        fpsElapsedMsec = 0;
    }

    if (!ProcessEventLoop()) {
        return false;
    }

    app.RunFrame(elapsedMsec);
    app.Draw(elapsedMsec);

    WCHAR windowText[64];
    swprintf_s(windowText, L"FPS: %u", fps);
    SetWindowText(hwndMain, windowText);

    return true;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Disable automatic DPI scaling.
    SetProcessDPIAware();

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, COUNT_OF(szTitle));

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTD3D12));

    // Perform application initialization
    if (!InitInstance(nCmdShow)) {
        return FALSE;
    }

    ::SetFocus(hwndMain);

    int t0 = BE1::PlatformTime::Milliseconds();

    while (1) {
        int t = BE1::PlatformTime::Milliseconds();
        int elapsedMsec = t - t0;
        BE1::Clamp(elapsedMsec, 0, 1000);

        t0 = t;

        if (!RunFrameInstance(elapsedMsec)) {
            break;
        }
    }

    ShutdownInstance();

    return 0;
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
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            if (renderer.IsInitialized()) {
                renderer.OnResize(LOWORD(lParam), HIWORD(lParam));
            }
        }
        return 0;
    case WM_TIMER:
        break;
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
        case IDM_TOGGLE_FULLSCREEN:
            //ToggleFullscreen(mainContext);
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
