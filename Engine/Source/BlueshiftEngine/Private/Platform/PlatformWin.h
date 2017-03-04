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

#pragma once

#include "PlatformGeneric.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

class PlatformWin : public PlatformGeneric {
public:
    PlatformWin();
    
    virtual void        Init() override;
    virtual void        Shutdown() override;

    virtual void        EnableMouse(bool enable) override;

    virtual void        SetMainWindowHandle(void *windowHandle) override;

    virtual void        Quit() override;
    virtual void        Log(const wchar_t *msg) override;
    virtual void        Error(const wchar_t *msg) override;

    virtual bool        IsCursorLocked() const override;
    virtual bool        LockCursor(bool lock) override;

    virtual void        GetMousePos(Point &pos) const override;
    virtual void        GenerateMouseDeltaEvent() override;

private:
    bool                cursorLocked;
    Point               rawMouseDeltaOld;

    HWND                hwnd;
};

BE_NAMESPACE_END
