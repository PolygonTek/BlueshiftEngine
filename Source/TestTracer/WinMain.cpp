#include "Precompiled.h"
#include "WinRes/WinResource.h"
#include "App.h"
static constexpr int MaxLoadString = 100;

//static constexpr int ScreenWidth = 1280;
//static constexpr int ScreenHeight = 960;
static constexpr int ScreenWidth = 1024;
static constexpr int ScreenHeight = 768;

static HINSTANCE hInst; // current instance
static HWND hWnd;
static HWND hWndStatus;
static TCHAR szTitle[MaxLoadString];       // The title bar text
static TCHAR szWindowClass[MaxLoadString]; // the main window class name

static App app;
static Options options;

void GetRealClientRect(HWND hWnd, RECT *rect) {
    RECT clrect;
    GetClientRect(hWnd, &clrect);

    HWND hChildWnd = GetWindow(hWnd, GW_CHILD);
    if (hChildWnd) {
        hChildWnd = GetWindow(hChildWnd, GW_HWNDFIRST);
        do {
            RECT cr;
            TCHAR classname[100];
            GetClassName(hChildWnd, classname, 100);

            if (!_tcscmp(classname, TOOLBARCLASSNAME)) {
                if (IsWindowVisible(hChildWnd)) {
                    GetClientRect(hChildWnd, &cr);
                    clrect.top += cr.bottom + 2;
                }
            }

            if (!_tcscmp(classname, STATUSCLASSNAME)) {
                if (IsWindowVisible(hChildWnd)) {
                    GetClientRect(hChildWnd, &cr);
                    clrect.bottom -= cr.bottom;
                }
            }

            hChildWnd = GetWindow(hChildWnd, GW_HWNDNEXT);
        } while (GetWindow(hChildWnd, GW_HWNDLAST) != hChildWnd);
    }

    *rect = clrect;
}

void ResizeClient(HWND hWnd, int width, int height, DWORD dwStyle, BOOL bMenu, BOOL bRepaint) {
    RECT wrect, crect;
    GetWindowRect(hWnd, &wrect);
    GetRealClientRect(hWnd, &crect);
    int diffx = crect.right = (wrect.right - wrect.left) - crect.right;
    int diffy = (wrect.bottom - wrect.top) - crect.bottom;
    AdjustWindowRect(&crect, dwStyle, bMenu);
    MoveWindow(hWnd, wrect.left, wrect.top, width + diffx, height + diffy, bRepaint);
}

void SystemLog(const int logLevel, const char *msg) {
    OutputDebugStringA(msg);
}

void SystemError(const int errLevel, const char *msg) {
    MessageBoxA(hWnd, msg, "Error", MB_OK);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance; // Store instance handle in our global variable

    unsigned long style = WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_VISIBLE;
    RECT rect = {0, 0, ScreenWidth, ScreenHeight};
    AdjustWindowRect(&rect, style, TRUE);

    wchar_t windowTitle[256];
    swprintf_s(windowTitle, L"%s - %hs %d Processors", szTitle, BE1::PlatformProcess::PlatformName(), BE1::PlatformSystem::NumCPUCoresIncludingHyperthreads());
    hWnd = CreateWindow(szWindowClass, windowTitle, style, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) {
        return FALSE;
    }

    int parts[256] = {
        160,
    };
    int idx = 1;
    parts[idx] = parts[idx - 1] + 96;
    ++idx;
    parts[idx] = parts[idx - 1] + 90;
    ++idx;
    parts[idx] = parts[idx - 1] + 64;
    ++idx;
    parts[idx++] = -1;
    int numParts = idx;

    hWndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE, _T(""), hWnd, 0);
    SendMessage(hWndStatus, SB_SETPARTS, (WPARAM)numParts, (LPARAM)parts);
    SendMessage(hWndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM) _T("Ready"));

    static TCHAR sz[64];
    _stprintf(sz, _T("%ix%i"), ScreenWidth, ScreenHeight);
    SendMessage(hWndStatus, SB_SETTEXT, (WPARAM)3, (LPARAM)sz);

    ResizeClient(hWnd, ScreenWidth, ScreenHeight, style, TRUE, TRUE);

    BE1::Str path = BE1::PlatformFile::ExecutablePath();
    path.AppendPath("../../..");
    path.CleanPath();
    BE1::PlatformFile::SetCwd(path);
    BE1::Engine::InitBase(4096, path, SystemLog, SystemError);
    app.Init(*BE1::Engine::taskManager, ScreenWidth, ScreenHeight, options);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    PostMessage(hWnd, WM_COMMAND, MAKELONG(IDM_START_AND_STOP, 0), (LPARAM)0);

    return TRUE;
}

void ShutdownInstance() {
    app.Shutdown();
    BE1::Engine::ShutdownBase();
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

static void UpdateRenderStatus() {
    TCHAR sz[64];
    const RenderStats &stats = app.GetStats();
    const uint64_t rayCount = stats.tracedRays;

    int elapsedMsec = stats.elapsedMsec;
    float seconds = elapsedMsec / 1000.0f;
    int hours = (int)seconds / (60 * 60);
    seconds -= hours * (60 * 60);
    int minutes = (int)seconds / 60;
    seconds -= minutes * 60;

    _stprintf(sz, _T("%ih %im %.1fs (%ims)"), hours, minutes, seconds, elapsedMsec);
    SendMessage(hWndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)sz);

    _stprintf(sz, _T("%i/%i SPP"), stats.completedSamples, stats.targetSamples);
    SendMessage(hWndStatus, SB_SETTEXT, (WPARAM)1, (LPARAM)sz);

    if (rayCount > 1024 * 1024 * 1024) {
        _stprintf(sz, _T("%.2fG Rays"), (float)rayCount / (1024 * 1024 * 1024));
    } else if (rayCount > 1024 * 1024) {
        _stprintf(sz, _T("%.2fM Rays"), (float)rayCount / (1024 * 1024));
    } else if (rayCount > 1024) {
        _stprintf(sz, _T("%.2fK Rays"), (float)rayCount / (1024));
    } else {
        _stprintf(sz, _T("%llu Rays"), static_cast<unsigned long long>(rayCount));
    }
    SendMessage(hWndStatus, SB_SETTEXT, (WPARAM)2, (LPARAM)sz);
}

static void UpdateRenderMenu() {
    const bool rendering = app.IsRendering();
    ModifyMenu(GetMenu(hWnd), IDM_START_AND_STOP, MF_BYCOMMAND | MF_STRING, IDM_START_AND_STOP, rendering ? _T("Stop") : _T("Start"));
    EnableMenuItem(GetMenu(hWnd), 2, MF_BYPOSITION | (rendering ? MF_DISABLED : MF_ENABLED));
    DrawMenuBar(hWnd);
}

struct ToggleOption {
    int command;
    bool Options::*value;
};

struct SampleOption {
    int firstCommand;
    int lastCommand;
    int Options::*value;
};

static constexpr ToggleOption ToggleOptions[] = {
    {IDM_OPTIONS_INDIRECT_LIGHTING, &Options::indirectLighting},
    {IDM_OPTIONS_IMPORTANCE_SAMPLING, &Options::importanceSampling},
    {IDM_OPTIONS_AMBIENT_OCCLUSION, &Options::ambientOcclusion},
    {IDM_OPTIONS_DOF, &Options::useDOF},
};

static constexpr SampleOption SampleOptions[] = {
    {IDM_OPTIONS_MAX_SAMPLE_PASSES_1, IDM_OPTIONS_MAX_SAMPLE_PASSES_4096, &Options::maxSamplePasses},
    {IDM_OPTIONS_SPLIT_SAMPLE_COUNT_1, IDM_OPTIONS_SPLIT_SAMPLE_COUNT_4096, &Options::splitSampleCount},
    {IDM_OPTIONS_SHADOW_SAMPLE_COUNT_1, IDM_OPTIONS_SHADOW_SAMPLE_COUNT_256, &Options::shadowSampleCount},
};

static void UpdateOptionsMenu(HMENU menu) {
    for (const ToggleOption &option : ToggleOptions) {
        CheckMenuItem(menu, option.command, options.*option.value ? MF_CHECKED : MF_UNCHECKED);
    }
    for (const SampleOption &option : SampleOptions) {
        for (int command = option.firstCommand; command <= option.lastCommand; ++command) {
            const int count = 1 << (command - option.firstCommand);
            CheckMenuItem(menu, command, options.*option.value == count ? MF_CHECKED : MF_UNCHECKED);
        }
    }
}

static bool HandleOptionCommand(int command) {
    for (const ToggleOption &option : ToggleOptions) {
        if (command == option.command) {
            if (!app.IsRendering()) {
                options.*option.value = !(options.*option.value);
            }
            return true;
        }
    }
    for (const SampleOption &option : SampleOptions) {
        if (command >= option.firstCommand && command <= option.lastCommand) {
            if (!app.IsRendering()) {
                options.*option.value = 1 << (command - option.firstCommand);
            }
            return true;
        }
    }
    return false;
}

static void TakeScreenshot() {
    switch (app.TakeScreenshot()) {
    case ScreenshotResult::WriteFailed:
        MessageBoxW(hWnd, L"Could not save screenshot.", L"TestTracer", MB_OK);
        break;
    case ScreenshotResult::NamesExhausted:
        MessageBoxW(hWnd, L"All screenshot names are in use.", L"TestTracer", MB_OK);
        break;
    default:
        break;
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
    case WM_CREATE:
        SetTimer(hWnd, 0, 100, nullptr);
        break;
    case WM_TIMER:
        if (app.Update()) {
            if (!app.IsRendering()) {
                UpdateRenderMenu();
            }

            UpdateRenderStatus();
            InvalidateRect(hWnd, nullptr, FALSE);
            UpdateWindow(hWnd);
        }
        break;
    case WM_INITMENUPOPUP:
        if (LOWORD(lParam) == 2) {
            UpdateOptionsMenu((HMENU)wParam);
        }
        break;
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        if (HandleOptionCommand(wmId)) {
            return 0;
        }
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_FILE_TAKE_SCREENSHOT:
            TakeScreenshot();
            break;
        case IDM_START_AND_STOP:
            if (app.IsRendering()) { // STOP
                app.Stop();
            } else { // START
                if (!app.Start(options)) {
                    MessageBoxW(hWnd, L"Could not start rendering.", L"TestTracer", MB_OK);
                }
            }
            UpdateRenderMenu();
            UpdateRenderStatus();
            InvalidateRect(hWnd, nullptr, FALSE);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        app.GetImage().Blt(hdc, 0, 0);
        EndPaint(hWnd, &ps);
        break;
    case WM_SIZE:
        SendMessage(hWndStatus, WM_SIZE, 0, 0);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYRAY));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_MYRAY);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_BAR_CLASSES};
    InitCommonControlsEx(&controls);
    MSG msg;
    HACCEL hAccelTable;

    LoadString(hInstance, IDS_APP_TITLE, szTitle, MaxLoadString);
    LoadString(hInstance, IDC_MYRAY, szWindowClass, MaxLoadString);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MYRAY));

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    ShutdownInstance();

    return (int)msg.wParam;
}
