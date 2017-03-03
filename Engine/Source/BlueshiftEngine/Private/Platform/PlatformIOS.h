#pragma once

#include "PlatformGeneric.h"
//#include <CoreGraphics/CoreGraphics.h>

OBJC_CLASS(UIWindow);

BE_NAMESPACE_BEGIN

class PlatformIOS : public PlatformGeneric {
public:
    PlatformIOS();
    
    virtual void            Init() override;
    virtual void            Shutdown() override;

    virtual void            SetMainWindowHandle(void *windowHandle) override;

    virtual void            Quit() override;
    virtual void            Log(const wchar_t *msg) override;
    virtual void            Error(const wchar_t *msg) override;

private:
    UIWindow *              window;
};

BE_NAMESPACE_END
