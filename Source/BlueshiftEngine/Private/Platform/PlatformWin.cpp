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
#include "PlatformWin.h"

BE_NAMESPACE_BEGIN

static int          originalmouseparms[3];
static int          newmouseparms[3];
static POINT        oldCursorPos;
static Point        windowCenter;

static RECT GetScreenWindowRect(HWND hwnd) {
    int sx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int sy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    RECT window_rect;
    ::GetWindowRect(hwnd, &window_rect);
    if (window_rect.left < sx) {
        window_rect.left = sx;
    }

    if (window_rect.top < sy) {
        window_rect.top = sy;
    }

    if (window_rect.right >= cx - 1) {
        window_rect.right = cx - 1;
    }

    if (window_rect.bottom >= cy - 1) {
        window_rect.bottom = cy - 1;
    }

    return window_rect;
}

PlatformWin::PlatformWin() {
}

void PlatformWin::Init() {
    PlatformGeneric::Init();

    hwnd = nullptr;

    cursorLocked = false;
}

void PlatformWin::Shutdown() {
    PlatformGeneric::Shutdown();
}

void PlatformWin::EnableMouse(bool enable) {
    //SystemParametersInfo(SPI_GETMOUSE, 0, originalmouseparms, 0);
    //newmouseparms[0] = 0;
    //newmouseparms[1] = 0;
    //newmouseparms[2] = 1;

    GetCursorPos(&oldCursorPos);

    rawMouseDeltaOld.x = 0;
    rawMouseDeltaOld.y = 0;

    PlatformGeneric::EnableMouse(enable);
}

void PlatformWin::SetMainWindowHandle(void *windowHandle) {
    hwnd = (HWND)windowHandle;
}

void PlatformWin::Quit() {
    exit(0);
}

void PlatformWin::Log(const wchar_t *msg) {
    OutputDebugString(msg);
}

void PlatformWin::Error(const wchar_t *msg) {
    MessageBox(hwnd ? hwnd : GetDesktopWindow(), msg, L"Error", MB_OK);
    
    Quit();
}

bool PlatformWin::IsCursorLocked() const {
    return cursorLocked;
}

bool PlatformWin::LockCursor(bool lock) {
    if (!hwnd || !mouseEnabled) {
        return false;
    }

    bool oldLocked = cursorLocked;

    if (lock ^ cursorLocked) {
        cursorLocked = lock;

        if (lock) {
            SystemParametersInfo(SPI_SETMOUSE, 0, newmouseparms, 0);

            RECT windowRect = GetScreenWindowRect(hwnd);
            windowCenter.x = (windowRect.left + windowRect.right) / 2;
            windowCenter.y = (windowRect.top + windowRect.bottom) / 2;

            GetCursorPos(&oldCursorPos);
            SetCursorPos(windowCenter.x, windowCenter.y);

            rawMouseDeltaOld.x = 0;
            rawMouseDeltaOld.y = 0;

            SetCapture(hwnd);

            // 커서를 화면에 가둔다
            ClipCursor(&windowRect);
            while (ShowCursor(FALSE) >= 0);
        } else {
            SystemParametersInfo(SPI_SETMOUSE, 0, originalmouseparms, 0);

            SetCursorPos(oldCursorPos.x, oldCursorPos.y);

            ClipCursor(nullptr);
            ReleaseCapture();
            while (ShowCursor(TRUE) < 0);
        }
    }

    return oldLocked;
}

void PlatformWin::GetMousePos(Point &pos) const {
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    ScreenToClient(hwnd, &cursorPos);

    pos.Set(cursorPos.x, cursorPos.y);
}

void PlatformWin::GenerateMouseDeltaEvent() {
    if (!hwnd || !mouseEnabled) {
        return;
    }

    if (!cursorLocked) {
        POINT cursorPos;
        GetCursorPos(&cursorPos);

        Point mouseDelta;
        mouseDelta.x = cursorPos.x - oldCursorPos.x;
        mouseDelta.y = cursorPos.y - oldCursorPos.y;

        QueEvent(Platform::MouseDeltaEvent, mouseDelta.x, mouseDelta.y, 0, nullptr);

        oldCursorPos = cursorPos;
    } else {
        POINT cursorPos;
        GetCursorPos(&cursorPos);

        Point rawMouseDelta;
        rawMouseDelta.x = cursorPos.x - windowCenter.x;
        rawMouseDelta.y = cursorPos.y - windowCenter.y;

        if (rawMouseDelta.x || rawMouseDelta.y) {
            SetCursorPos(windowCenter.x, windowCenter.y);
        }

        Point mouseDelta;
        mouseDelta.x = rawMouseDelta.x;
        mouseDelta.y = rawMouseDelta.y;
        
        rawMouseDeltaOld.x = rawMouseDelta.x;
        rawMouseDeltaOld.y = rawMouseDelta.y;

        if (mouseDelta.x != 0 || mouseDelta.y != 0) {
            QueEvent(Platform::MouseDeltaEvent, mouseDelta.x, mouseDelta.y, 0, nullptr);
        }

        if (rawMouseDelta.x != 0 || rawMouseDelta.y != 0) {
            SetCursorPos(windowCenter.x, windowCenter.y);
        }
    }
}

BE_NAMESPACE_END
