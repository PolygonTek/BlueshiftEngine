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
    bool                    IsMouseExist() const;

    void                    ClearKeyEvents();
    void                    ClearTouches();

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

    bool                    mouseExist;
    bool                    inputUpdated;
};

extern InputSystem          inputSystem;

BE_NAMESPACE_END