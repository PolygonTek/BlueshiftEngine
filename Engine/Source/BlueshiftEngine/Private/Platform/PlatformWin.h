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
