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
