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
#include "WinResource.h"
#include <windowsx.h>
#include <tchar.h>

static const TCHAR *        mainWindowClassName  = _T("BLUESHIFT_PLAYER_MAIN_WINDOW");

static HWND                 mainWnd;

static HMENU                hMenu;
static TCHAR                szTitle[100];    // The title bar text

LRESULT CALLBACK            WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static BE1::CVar            disp_width(L"disp_width", L"1280", BE1::CVar::Integer | BE1::CVar::Archive, L"");
static BE1::CVar            disp_height(L"disp_height", L"720", BE1::CVar::Integer | BE1::CVar::Archive, L"");
static BE1::CVar            disp_fullscreen(L"disp_fullscreen", L"0", BE1::CVar::Bool | BE1::CVar::Archive, L"");
static BE1::CVar            disp_bpp(L"disp_bpp", L"0", BE1::CVar::Integer | BE1::CVar::Archive, L"");
static BE1::CVar            disp_frequency(L"disp_frequency", L"0", BE1::CVar::Integer | BE1::CVar::Archive, L"");

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
    ::AdjustWindowRect(&rect, style, fullscreen ? FALSE : TRUE);

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
        NULL, NULL, (HINSTANCE)GetModuleHandle(NULL), NULL);

    if (fullscreen) {
        hMenu = GetMenu(hwnd);
        if (hMenu) {
            SetMenu(hwnd, NULL);
        }
    }

    ::SetForegroundWindow(hwnd);
    ::SetFocus(hwnd);

    return hwnd;
}

static void ChangeRenderWindow(HWND hwnd, int width, int height, bool fullscreen) {
    int style = ::GetWindowLong(hwnd, GWL_STYLE);

    if (fullscreen) {
        style &= ~WS_OVERLAPPEDWINDOW;
        style |= WS_POPUP;
        SetWindowLong(hwnd, GWL_STYLE, style);
        SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST);

        HMENU hMenu = GetMenu(hwnd);
        if (hMenu) {
            SetMenu(hwnd, NULL);
        }
    } else {
        style &= ~WS_POPUP;
        style |= WS_OVERLAPPEDWINDOW;
        SetWindowLong(hwnd, GWL_STYLE, style);
        SetWindowLong(hwnd, GWL_EXSTYLE, 0);
        ShowWindow(hwnd, SW_RESTORE);

        if (hMenu) {
            SetMenu(hwnd, hMenu);
        }
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

static void DestroyRenderWindow(HWND hwnd) {
    ::DestroyWindow(hwnd);
}

static void ToggleFullscreen(HWND hwnd) {
    if (!BE1::rhi.IsFullscreen()) {
        ChangeRenderWindow(hwnd, disp_width.GetInteger(), disp_height.GetInteger(), true);

        BE1::rhi.SetFullscreen(app.mainRenderContext->GetContextHandle(), disp_width.GetInteger(), disp_height.GetInteger());
    } else {
        BE1::rhi.ResetFullscreen(app.mainRenderContext->GetContextHandle());
        
        ChangeRenderWindow(hwnd, disp_width.GetInteger(), disp_height.GetInteger(), false);
    }
}

static void DisplayContext(BE1::RHI::Handle context, void *dataPtr) {
    app.Draw();
}

static void InitInstance(HINSTANCE hInstance, LPCTSTR lpCmdLine, int nCmdShow) {
    BE1::Engine::InitParms initParms;
//  initParms.args.TokenizeString(lpCmdLine, false);

    BE1::Str playerDir;
#if 0
    playerDir = BE1::PlatformFile::Cwd();
#else
    playerDir = BE1::PlatformFile::ExecutablePath();
    playerDir.AppendPath("..\\..\\..");
#endif
    playerDir.CleanPath(PATHSEPERATOR_CHAR);
    
    if (lpCmdLine[0]) {
        playerDir = lpCmdLine;
    }

    initParms.baseDir = playerDir;

    BE1::Str dataDir = playerDir + "\\Data";
    initParms.searchPath = dataDir;

    BE1::Engine::Init(&initParms);

    if (!BE1::resourceGuidMapper.Read("Data\\guidmap")) {
        BE_FATALERROR(L"Couldn't open guidmap !");
    }

    WNDCLASSEX wcex;
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize             = sizeof(WNDCLASSEX);
    wcex.style              = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc        = WndProc;
    wcex.hInstance          = hInstance;
    wcex.hIcon              = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAME));
    wcex.hIconSm            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAME));
    wcex.hbrBackground      = (HBRUSH)(COLOR_GRAYTEXT + 1);
    wcex.hCursor            = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszMenuName       = MAKEINTRESOURCE(IDC_GAME);
    wcex.lpszClassName      = mainWindowClassName;
    RegisterClassEx(&wcex);

    const wchar_t *title = BE1::wva(L"%s %hs %hs %hs", szTitle, BE1::PlatformProcess::PlatformName(), __DATE__, __TIME__);

    mainWnd = CreateRenderWindow(title, mainWindowClassName, disp_width.GetInteger(), disp_height.GetInteger(), disp_fullscreen.GetBool());

    BE1::gameClient.Init(mainWnd, true);

    app.mainRenderContext = BE1::renderSystem.AllocRenderContext(true);
    app.mainRenderContext->Init(mainWnd, 1280, 720, DisplayContext, NULL);

    if (disp_fullscreen.GetBool()) {
        BE1::rhi.SetFullscreen(app.mainRenderContext->GetContextHandle(), disp_width.GetInteger(), disp_height.GetInteger());
    }

    app.Init();

    app.LoadAppScript("Application");
    
    app.StartAppScript();
}

static void ShutdownInstance() {
    app.Shutdown();

    app.mainRenderContext->Shutdown();
    BE1::renderSystem.FreeRenderContext(app.mainRenderContext);

    DestroyRenderWindow(mainWnd);

    BE1::gameClient.Shutdown();

    BE1::Engine::Shutdown();
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    MSG msg;

    // Disable automatic DPI scaling.
    //SetProcessDPIAware();

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, COUNT_OF(szTitle));

    InitInstance(hInstance, lpCmdLine, nCmdShow);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAME));

    int t0 = BE1::PlatformTime::Milliseconds();

    while (1) {
        int t = BE1::PlatformTime::Milliseconds();
        int elapsedMsec = t - t0;
        if (elapsedMsec > 1000) {
            elapsedMsec = 1000;
        }

        t0 = t;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                goto QUIT;
            }

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        
        BE1::Engine::RunFrame(elapsedMsec);

        BE1::gameClient.RunFrame();

        app.Update();

        app.mainRenderContext->Display();

        BE1::gameClient.EndFrame();
    }

QUIT:
    app.OnApplicationTerminate();
    
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int wmId, wmEvent;
    HIMC hImmContext;
    WORD fActive;
    BOOL fMinimize;
    wchar_t compStr[3];
    int key;

    switch (message) {
    case WM_CREATE:
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    /*case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        return 0; 
    }*/
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
        }

        if (app.mainRenderContext) {
            app.mainRenderContext->OnResize(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    case WM_DPICHANGED:
        break;
    case WM_ACTIVATE:
        fActive = LOWORD(wParam);
        fMinimize = (BOOL)HIWORD(wParam);
        BE1::platform->AppActivate(fActive != WA_INACTIVE, fMinimize ? true : false);
        return 0;
    case WM_SYSCOMMAND:
        if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER) {
            return 0;
        }
        break;
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
        case IDM_TOGGLE_FULLSCREEN:
            ToggleFullscreen(mainWnd);
            break;
        case IDM_ABOUT:
            DialogBox((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam != VK_PROCESSKEY) {
            // higher 16 bit of lParam represent scancode
            // use lower 8 bit and set extended bit to it's MSB
            key = ((lParam >> 16) & 0xFF ) | (((lParam >> 24) & 1) << 7);

            BE1::platform->QueEvent(BE1::Platform::KeyEvent, key, true, 0, NULL);
        }
        return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key = ((lParam >> 16) & 0xFF ) | (((lParam >> 24) & 1) << 7);

        BE1::platform->QueEvent(BE1::Platform::KeyEvent, key, false, 0, NULL);
        return 0;
    case WM_SYSCHAR:
    case WM_CHAR: // WM_CHAR message uses Unicode Transformation Format (UTF)-16
    case WM_IME_CHAR:
        BE1::platform->QueEvent(BE1::Platform::CharEvent, wParam, 0, 0, NULL);
        return 0;
    case WM_IME_KEYDOWN:
    case WM_IME_KEYUP:
        break;
    case WM_IME_STARTCOMPOSITION:
        return 0;
    case WM_IME_ENDCOMPOSITION:
        return 0;
    case WM_IME_COMPOSITION:
        hImmContext = ImmGetContext(hWnd);
        if (lParam & GCS_COMPSTR) {
            int bytes = ImmGetCompositionString(hImmContext, GCS_COMPSTR, NULL, 0);
            if (bytes > 0) {
                ImmGetCompositionString(hImmContext, GCS_COMPSTR, compStr, bytes);
                BE1::platform->QueEvent(BE1::Platform::CompositionEvent, compStr[0], 0, 0, NULL);
            } else {
                BE1::platform->QueEvent(BE1::Platform::CompositionEvent, L'\b', 0, 0, NULL);
            }
        } else if (lParam & GCS_RESULTSTR) {
            int bytes = ImmGetCompositionString(hImmContext, GCS_RESULTSTR, NULL, 0);
            if (bytes > 0) {
                ImmGetCompositionString(hImmContext, GCS_RESULTSTR, compStr, bytes);
                BE1::platform->QueEvent(BE1::Platform::CharEvent, compStr[0], 0, 0, NULL);
            }
        }
                
        ImmReleaseContext(hWnd, hImmContext);
        return 0;
    case WM_LBUTTONDOWN:
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::Mouse1, true, 0, NULL);
        return 0;
    case WM_RBUTTONDOWN:
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::Mouse2, true, 0, NULL);
        return 0;
    case WM_MBUTTONDOWN:
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::Mouse3, true, 0, NULL);
        return 0;
    case WM_XBUTTONDOWN: {
        int button = GET_XBUTTON_WPARAM(wParam);
        if (button == 1) {
            BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::Mouse4, true, 0, NULL);
        } else if (button == 2) {
            BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::Mouse5, true, 0, NULL);
        }
        return 0; 
    }
    case WM_LBUTTONUP:
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::Mouse1, false, 0, NULL);
        return 0;
    case WM_RBUTTONUP:
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::Mouse2, false, 0, NULL);
        return 0;
    case WM_MBUTTONUP:
        BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::Mouse3, false, 0, NULL);
        return 0;
    case WM_XBUTTONUP: {
        int button = GET_XBUTTON_WPARAM(wParam);
        if (button == 1) {
            BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::Mouse4, false, 0, NULL);
        } else if (button == 2) {
            BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::Mouse5, false, 0, NULL);
        }
        return 0; 
    }
    case WM_MOUSEMOVE: {
        const int x = GET_X_LPARAM(lParam);
        const int y = GET_Y_LPARAM(lParam);

        BE1::platform->QueEvent(BE1::Platform::MouseMoveEvent, x, y, 0, NULL);
        return 0; 
    }
    case WM_MOUSEWHEEL:
        if ((short)HIWORD(wParam) > 0) {
            BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::MouseWheelUp, true, 0, NULL);
            BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::MouseWheelUp, false, 0, NULL);
        } else {
            BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::MouseWheelDown, true, 0, NULL);
            BE1::platform->QueEvent(BE1::Platform::KeyEvent, (int64_t)BE1::KeyCode::MouseWheelDown, false, 0, NULL);
        }
        return 0;
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}
