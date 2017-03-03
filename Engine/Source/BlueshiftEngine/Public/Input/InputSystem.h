#pragma once

/*
-------------------------------------------------------------------------------

    Input System

-------------------------------------------------------------------------------
*/

#include "Core/Allocator.h"
#include "Containers/Queue.h"
#include "Math/Math.h"
#include "KeyCodes.h"

BE_NAMESPACE_BEGIN

class InputSystem {
public:
    struct KeyEv {
        KeyCode::Enum       keynum;
        bool                down;
        Queue<KeyEv>        node;
    };
    
    struct Touch {
        enum Phase {
            NullPhase, Started, Moved, Ended, Canceled
        };
        int32_t             id;
        Phase               phase;
        Point               position;
    };
    
    struct TouchEv {
        Touch               touch;
        Queue<TouchEv>      node;
    };

    InputSystem();

    void                    Init();
    void                    Shutdown();

    void                    EndFrame();

    bool                    IsUpdated() const;

    bool                    IsKeyDown(KeyCode::Enum keynum) const;
    bool                    IsKeyUp(KeyCode::Enum keynum) const;
    bool                    IsKeyPressed(KeyCode::Enum keynum) const;
    int                     GetTouchCount() const;
    const Touch             GetTouch(int touchIndex) const;

    bool                    LockCursor(bool lock);

    Point                   GetMousePos() const;
    Point                   GetAxisDelta() const;

    void                    KeyEvent(KeyCode::Enum key, bool down);
    void                    MouseMoveEvent(int x, int y, int time);
    void                    MouseDeltaEvent(int dx, int dy, int time);
    void                    JoyAxisEvent(int dx, int dy, int time);
    void                    TouchEvent(Touch::Phase phase, uint64_t touchId, int x, int y);

private:
    BlockAllocator<KeyEv, 32> keyEventAllocator;
    Queue<KeyEv>            keyEventQueue;
    
    BlockAllocator<TouchEv, 32> touchEventAllocator;
    Queue<TouchEv>          touchEventQueue;

    static Touch            nullTouch;

    Point                   mousePos;
    Point                   axisDelta;

    bool                    inputUpdated;
};

extern InputSystem          inputSystem;

BE_NAMESPACE_END