#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Input/KeyCmd.h"
#include "Input/InputSystem.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterInput(LuaCpp::Module &module) {
    LuaCpp::Selector _Input = module["Input"];

    _Input.SetObj(inputSystem);
    _Input.AddObjMembers(inputSystem,
        "is_key_down", &InputSystem::IsKeyDown,
        "is_key_up", &InputSystem::IsKeyUp,
        "is_key_pressed", &InputSystem::IsKeyPressed,
        "touch_count", &InputSystem::GetTouchCount,
        "touch", &InputSystem::GetTouch,
        "lock_cursor", &InputSystem::LockCursor,
        "mouse_pos", &InputSystem::GetMousePos,
        "axis_delta", &InputSystem::GetAxisDelta);

    LuaCpp::Selector _Input_KeyCode = _Input["KeyCode"];
    _Input_KeyCode.SetClass<KeyCode>();
    for (int i = (int)KeyCode::None + 1; i < (int)KeyCode::LastKey; i++) {
        _Input_KeyCode[keyCmdSystem.KeynumToString((KeyCode::Enum)i)] = i;
    }

    LuaCpp::Selector _Input_Touch = _Input["Touch"];
    _Input_Touch.SetClass<InputSystem::Touch>();
    _Input_Touch["NullPhase"] = InputSystem::Touch::NullPhase;
    _Input_Touch["Started"] = InputSystem::Touch::Started;
    _Input_Touch["Moved"] = InputSystem::Touch::Moved;
    _Input_Touch["Ended"] = InputSystem::Touch::Ended;
    _Input_Touch["Canceled"] = InputSystem::Touch::Canceled;
    _Input_Touch.AddClassMembers<InputSystem::Touch>(
        "id", &InputSystem::Touch::id,
        "phase", &InputSystem::Touch::phase,
        "position", &InputSystem::Touch::position);    
}

BE_NAMESPACE_END