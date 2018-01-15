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
#include "Platform/Platform.h"
#include "Input/KeyCmd.h"
#include "Input/InputSystem.h"

BE_NAMESPACE_BEGIN

InputSystem inputSystem;

InputSystem::Touch InputSystem::nullTouch = { 0, };

InputSystem::InputSystem() {
    inputUpdated = false;
}

void InputSystem::Init() {
    axisDelta.Set(0, 0);
}

void InputSystem::Shutdown() {
    keyEventAllocator.FreeAllBlocks();
    touchEventAllocator.FreeAllBlocks();
}

void InputSystem::EndFrame() {
    ClearKeyEvents();

    ClearTouches();

    axisDelta.Set(0, 0);

    inputUpdated = false;
}

void InputSystem::ClearKeyEvents() {
    while (1) {
        KeyEv *keyEvent = keyEventQueue.RemoveFirst();
        if (!keyEvent) {
            break;
        }
        keyEventAllocator.Free(keyEvent);
    }
}

void InputSystem::KeyEvent(KeyCode::Enum key, bool down) {
    keyCmdSystem.KeyEvent(key, down);

    KeyEv *newEvent = keyEventAllocator.Alloc();
    newEvent->keynum = key;
    newEvent->down = down;
    newEvent->node.SetOwner(newEvent);
    keyEventQueue.Add(newEvent->node);

    inputUpdated = true;
}

void InputSystem::MouseMoveEvent(int x, int y, int time) {
    if (x == mousePos.x && y == mousePos.y) {
        return;
    }

    mousePos.x = x;
    mousePos.y = y;

    inputUpdated = true;
}

void InputSystem::MouseDeltaEvent(int dx, int dy, int time) {
    axisDelta.x += dx;
    axisDelta.y += dy;
}

void InputSystem::JoyAxisEvent(int dx, int dy, int time) {
    axisDelta.x += dx;
    axisDelta.y += dy;
}

void InputSystem::TouchEvent(InputSystem::Touch::Phase phase, uint64_t id, int x, int y) {
    TouchEv *newEvent = touchEventAllocator.Alloc();
    newEvent->touch.id = (int32_t)id;
    newEvent->touch.phase = phase;
    newEvent->touch.position = Point(x, y);
    newEvent->node.SetOwner(newEvent);
    touchEventQueue.Add(newEvent->node);
}

bool InputSystem::IsUpdated() const {
    return inputUpdated;
}

bool InputSystem::IsKeyDown(KeyCode::Enum keynum) const {
    int count = 0;
    for (Queue<KeyEv> *node = keyEventQueue.GetFirst(); node; node = node->GetNext()) {
        const KeyEv *keyEvent = node->Owner();
        if (keyEvent->keynum == keynum && keyEvent->down) {
            count++;
        }
    }
    return count > 0;
}

bool InputSystem::IsKeyUp(KeyCode::Enum keynum) const {
    int count = 0;
    for (Queue<KeyEv> *node = keyEventQueue.GetFirst(); node; node = node->GetNext()) {
        const KeyEv *keyEvent = node->Owner();
        if (keyEvent->keynum == keynum && !keyEvent->down) {
            count++;
        }
    }
    return count > 0;
}

void InputSystem::ClearTouches() {
    while (1) {
        TouchEv *touchEvent = touchEventQueue.RemoveFirst();
        if (!touchEvent) {
            break;
        }
        touchEventAllocator.Free(touchEvent);
    }
}

int InputSystem::GetTouchCount() const {
    return touchEventQueue.Count();
}

const InputSystem::Touch InputSystem::GetTouch(int touchIndex) const {
    int count = 0;
    for (Queue<TouchEv> *node = touchEventQueue.GetFirst(); node; node = node->GetNext()) {
        if (count == touchIndex) {
            const TouchEv *touchEvent = node->Owner();
            return touchEvent->touch;
        }
        count++;
    }

    return nullTouch;
}

bool InputSystem::IsKeyPressed(KeyCode::Enum keynum) const {
    return keyCmdSystem.IsPressed(keynum);//
}

bool InputSystem::LockCursor(bool lock) {
    return platform->LockCursor(lock);
}

Point InputSystem::GetMousePos() const {
    return mousePos;
}

Point InputSystem::GetAxisDelta() const {
    return axisDelta;
}

BE_NAMESPACE_END
