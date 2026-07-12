// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Platform/PlatformSystem.h"
#include "Platform/Windows/PlatformWinUtils.h"
#include "WinResource.h"
#include "App.h"
#include "RHI/RHI.h"
#include "Render/RenderSystem.h"
#include "Render/RenderContext.h"
#include <tchar.h>

static const TCHAR *        mainWindowClassName  = _T("BLUESHIFT_MAIN_WINDOW");
static const TCHAR *        subWindowClassName = _T("BLUESHIFT_SUB_WINDOW");

static TCHAR                szTitle[100];    // The title bar text

static HWND                 hwndMain;
static HWND                 hwndSub;
static HACCEL               hAccelTable;
static WCHAR                windowTitleString[256];

LRESULT CALLBACK            MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static constexpr const char *appConfigFilename = "Config/TestD3D12.cfg";

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

static bool SetCameraMoveFlagFromKey(WPARAM key, bool enabled) {
    switch (key) {
    case 'W':
        app.SetCameraMoveFlag(App::CameraMoveFlag::Forward, enabled);
        return true;
    case 'S':
        app.SetCameraMoveFlag(App::CameraMoveFlag::Backward, enabled);
        return true;
    case 'A':
        app.SetCameraMoveFlag(App::CameraMoveFlag::Left, enabled);
        return true;
    case 'D':
        app.SetCameraMoveFlag(App::CameraMoveFlag::Right, enabled);
        return true;
    case 'E':
        app.SetCameraMoveFlag(App::CameraMoveFlag::Up, enabled);
        return true;
    case 'Q':
        app.SetCameraMoveFlag(App::CameraMoveFlag::Down, enabled);
        return true;
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
        app.SetCameraMoveFlag(App::CameraMoveFlag::Fast, enabled);
        return true;
    default:
        return false;
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
    ::AdjustWindowRect(&rect, style, FALSE);

    windowRect.w = rect.right - rect.left;
    windowRect.h = rect.bottom - rect.top;

    if (fullscreen) {
        windowRect.x = 0;
        windowRect.y = 0;
    } else {
        RECT workArea;
        if (!::SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0)) {
            const int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
            const int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

            ::SetRect(&workArea, 0, 0, screenWidth, screenHeight);
        }

        const int workAreaWidth = workArea.right - workArea.left;
        const int workAreaHeight = workArea.bottom - workArea.top;

        // Limit the full window size, including the title bar and borders, to the work area.
        windowRect.w = BE1::Min(windowRect.w, workAreaWidth);
        windowRect.h = BE1::Min(windowRect.h, workAreaHeight);

        windowRect.x = workArea.left + ((workAreaWidth - windowRect.w) >> 1);
        windowRect.y = workArea.top + ((workAreaHeight - windowRect.h) >> 1);
    }

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

static BOOL InitInstance(int nCmdShow, LPTSTR lpCmdLine) {
    BE1::Engine::isMainThread = true;
    BE1::Str execPath = BE1::PlatformFile::ExecutablePath();
    BE1::Str basePath = execPath;
    basePath.AppendPath("../../..");
    basePath.CleanPath();
    BE1::Engine::InitBase(4096, basePath, SystemLog, SystemError);

    // search path 설정
    BE1::Str searchPath = basePath;
    searchPath.AppendPath("Data");
    BE1::fileSystem.SetSearchPath(searchPath);

    BE1::CmdArgs startupArgs;
    if (lpCmdLine[0]) {
        BE1::Str startupCommandLine = BE1::Str::UTF8StrFromWCharString(lpCmdLine);
        startupArgs.TokenizeString(startupCommandLine.c_str(), true);
    }
    const BE1::Str startupCommandText = BE1::CmdSystem::BuildStartupCommandText(startupArgs);

    BE1::cmdSystem.BufferCommandText(BE1::CmdSystem::Execution::Now, BE1::va("exec \"%s\"\n", appConfigFilename));
    if (startupCommandText.Length()) {
        BE1::cmdSystem.BufferCommandText(BE1::CmdSystem::Execution::Now, startupCommandText.c_str());
    }
    BE1::cvarSystem.ClearModified();

    // 추가적인 DLL 폴더로 Win64 폴더를 추가
    BE1::Str commonDllPath = execPath;
    commonDllPath.AppendPath("..");
    commonDllPath.CleanPath();
    //PlatformProcess::AddDllDirectory(commonDllPath);
    char pathBuffer[4096];
    BE1::PlatformSystem::GetEnvVar("PATH", pathBuffer, 4096);
    BE1::Str newPath = commonDllPath + ";" + pathBuffer;
    BE1::PlatformSystem::SetEnvVar("PATH", newPath);

    char szFullTitle[128];
    BE1::Str::snPrintf(szFullTitle, sizeof(szFullTitle), "%ls %s %s %s", szTitle, BE1::PlatformProcess::PlatformName(), __DATE__, __TIME__);

    wchar_t title[128];
    BE1::PlatformWinUtils::UTF8ToUCS2(szFullTitle, title, COUNT_OF(title));

    hwndMain = CreateMainWindow(title, 1920, 1080);

    app.Init(hwndMain);

    return TRUE;
}

static void ShutdownInstance() {
    app.Shutdown();

    BE1::File *file = BE1::fileSystem.OpenFile(appConfigFilename, BE1::File::Mode::Write);
    if (file) {
        BE1::cvarSystem.WriteVariables(file);
        BE1::fileSystem.CloseFile(file);
    }

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

static bool RunFrameInstance(uint64_t frameUsec) {
    PROFILER_CPU_SCOPED_EVENT("RunFrameInstance");

    app.UpdateFrameTime(frameUsec);

    if (!ProcessEventLoop()) {
        return false;
    }

    BE1::cmdSystem.ExecuteCommandBuffer();

    app.Update(frameUsec);

    app.Render(frameUsec);

    return true;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    // Disable automatic DPI scaling.
    SetProcessDPIAware();

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, COUNT_OF(szTitle));

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTD3D12));

    // Perform application initialization
    if (!InitInstance(nCmdShow, lpCmdLine)) {
        return FALSE;
    }

    ::SetFocus(hwndMain);

    GetWindowText(hwndMain, windowTitleString, COUNT_OF(windowTitleString));

    uint64_t t0 = BE1::PlatformTime::Microseconds();

    while (1) {
        uint64_t t = BE1::PlatformTime::Microseconds();
        uint64_t elapsedUsec = t - t0;
        BE1::Clamp(elapsedUsec, 0ULL, 1000000ULL);

        t0 = t;

        if (!RunFrameInstance(elapsedUsec)) {
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
    enum class MouseDragMode {
        None,
        Look,
        Pan
    };

    static MouseDragMode mouseDragMode = MouseDragMode::None;
    static int lastMouseX = 0;
    static int lastMouseY = 0;

#ifdef ENABLE_IMGUI
    RenderContext *renderContext = renderSystem ? renderSystem->FindRenderContextByWindowHandle(hwnd) : nullptr;
    const bool imguiHandled = renderContext && renderContext->HandleImGuiWndProc(hwnd, message, static_cast<uintptr_t>(wParam), static_cast<intptr_t>(lParam));
#endif

    switch (message) {
    case WM_CLOSE:
        break;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            app.OnResize(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    case WM_TIMER:
        break;
    case WM_KILLFOCUS:
        app.ClearCameraMoveFlags();
        if (mouseDragMode != MouseDragMode::None) {
            mouseDragMode = MouseDragMode::None;
            if (GetCapture() == hwnd) {
                ReleaseCapture();
            }
        }
        break;
    case WM_KEYDOWN:
        if (wParam == VK_F11) {
            app.TakeScreenshot();
            return 0;
        }

        // 마우스 드래그 모드가 있을 때는 ImGui 키보드 캡쳐보다 먼저 실행한다.
        if (mouseDragMode != MouseDragMode::None && SetCameraMoveFlagFromKey(wParam, true)) {
            return 0;
        }

#ifdef ENABLE_IMGUI
        if (renderContext && renderContext->ImGuiWantsCaptureKeyboard()) {
            return 0;
        }
#endif
        if (SetCameraMoveFlagFromKey(wParam, true)) {
            return 0;
        }
        break;
    case WM_KEYUP:
        if (SetCameraMoveFlagFromKey(wParam, false)) {
            return 0;
        }
#ifdef ENABLE_IMGUI
        if (renderContext && renderContext->ImGuiWantsCaptureKeyboard()) {
            return 0;
        }
#endif
        break;
    case WM_LBUTTONDOWN:
#ifdef ENABLE_IMGUI
        if (renderContext && renderContext->ImGuiWantsCaptureMouse()) {
            return 0;
        }
#endif
        {
            POINTS p = MAKEPOINTS(lParam);
            lastMouseX = p.x;
            lastMouseY = p.y;
            mouseDragMode = MouseDragMode::Look;
            SetCapture(hwnd);
        }
        return 0;
    case WM_LBUTTONUP:
        if (mouseDragMode == MouseDragMode::Look) {
            mouseDragMode = MouseDragMode::None;
            ReleaseCapture();
        }
        return 0;
    case WM_RBUTTONDOWN:
#ifdef ENABLE_IMGUI
        if (renderContext && renderContext->ImGuiWantsCaptureMouse()) {
            return 0;
        }
#endif
        {
            POINTS p = MAKEPOINTS(lParam);
            lastMouseX = p.x;
            lastMouseY = p.y;
            mouseDragMode = MouseDragMode::Pan;
            SetCapture(hwnd);
        }
        return 0;
    case WM_RBUTTONUP:
        if (mouseDragMode == MouseDragMode::Pan) {
            mouseDragMode = MouseDragMode::None;
            ReleaseCapture();
        }
        return 0;
    case WM_MOUSEMOVE:
#ifdef ENABLE_IMGUI
        if (renderContext && renderContext->ImGuiWantsCaptureMouse() && mouseDragMode == MouseDragMode::None) {
            return 0;
        }
#endif
        if (mouseDragMode != MouseDragMode::None) {
            POINTS p = MAKEPOINTS(lParam);
            int deltaX = p.x - lastMouseX;
            int deltaY = p.y - lastMouseY;
            if (mouseDragMode == MouseDragMode::Look || (mouseDragMode == MouseDragMode::Pan && app.GetCameraControlMode() == App::CameraControlMode::Fly)) {
                app.AddCameraMouseDelta(deltaX, deltaY);
            } else {
                app.AddCameraPanDelta(deltaX, deltaY);
            }
            lastMouseX = p.x;
            lastMouseY = p.y;
        }
        return 0;
    case WM_MOUSEWHEEL:
#ifdef ENABLE_IMGUI
        if (renderContext && renderContext->ImGuiWantsCaptureMouse()) {
            return 0;
        }
#endif
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            app.AddCameraDistance(delta);
        }
        return 0;
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
#ifdef ENABLE_IMGUI
    if (imguiHandled) {
        return 0;
    }
#endif
    return DefWindowProc(hwnd, message, wParam, lParam);
}
