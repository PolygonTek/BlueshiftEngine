#pragma once

#include "Platform/Platform.h"

BE_NAMESPACE_BEGIN

class PlatformGeneric : public PlatformAbstract {
public:
    PlatformGeneric();
    
    virtual void        Init() override;
    virtual void        Shutdown() override;

    virtual void        EnableMouse(bool enable) override;

    virtual void        GetEvent(Platform::Event *ev) override;
    virtual void        QueEvent(Platform::EventType type, int64_t value, int64_t value2, int ptrLength, void *ptr) override;

    virtual bool        IsActive() const override;
    virtual void        AppActivate(bool active, bool minimized) override;

    virtual bool        IsCursorLocked() const override { return false; }
    virtual bool        LockCursor(bool lock) override { return false; }

    virtual void        GetMousePos(Point &pos) const override {}
    virtual void        GenerateMouseDeltaEvent() override {}

protected:
    bool                active;
    bool                minimized;
    bool                mouseEnabled;

    int                 numEvents;
    int                 numProcessedEvents;
    Platform::Event     eventQueue[Platform::MaxPlatformEvents];
};

BE_NAMESPACE_END
