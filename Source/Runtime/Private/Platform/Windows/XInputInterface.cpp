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
#include "Platform/Windows/XInputInterface.h"
#include "Input/KeyCodes.h"

BE_NAMESPACE_BEGIN

XInputInterface::XInputInterface() {
    for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++) {
        ControllerState &controllerState = controllerStates[controllerIndex];
        memset(controllerState.buttonStates, 0, sizeof(controllerState.buttonStates));

        controllerState.controllerId = controllerIndex;
    }

    joyKeys[GamePadButton::A] = KeyCode::Joy1;
    joyKeys[GamePadButton::B] = KeyCode::Joy2;
    joyKeys[GamePadButton::X] = KeyCode::Joy3;
    joyKeys[GamePadButton::Y] = KeyCode::Joy4;
    joyKeys[GamePadButton::L1] = KeyCode::Joy5;
    joyKeys[GamePadButton::R1] = KeyCode::Joy6;
    joyKeys[GamePadButton::L2] = KeyCode::Joy7;
    joyKeys[GamePadButton::R2] = KeyCode::Joy8;
    joyKeys[GamePadButton::Back] = KeyCode::Joy9;
    joyKeys[GamePadButton::Start] = KeyCode::Joy10;
    joyKeys[GamePadButton::LeftThumb] = KeyCode::Joy11;
    joyKeys[GamePadButton::RightThumb] = KeyCode::Joy12;
    joyKeys[GamePadButton::DPadUp] = KeyCode::Joy13;
    joyKeys[GamePadButton::DPadDown] = KeyCode::Joy14;
    joyKeys[GamePadButton::DPadLeft] = KeyCode::Joy15;
    joyKeys[GamePadButton::DPadRight] = KeyCode::Joy16;
    joyKeys[GamePadButton::LeftStickUp] = KeyCode::Joystick1Up;
    joyKeys[GamePadButton::LeftStickDown] = KeyCode::Joystick1Down;
    joyKeys[GamePadButton::LeftStickLeft] = KeyCode::Joystick1Left;
    joyKeys[GamePadButton::LeftStickRight] = KeyCode::Joystick1Right;
    joyKeys[GamePadButton::RightStickUp] = KeyCode::Joystick2Up;
    joyKeys[GamePadButton::RightStickDown] = KeyCode::Joystick2Down;
    joyKeys[GamePadButton::RightStickLeft] = KeyCode::Joystick2Left;
    joyKeys[GamePadButton::RightStickRight] = KeyCode::Joystick2Right;
}

void XInputInterface::SendControllerEvents() {
    XINPUT_STATE states[XUSER_MAX_COUNT];

    for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++) {
        ControllerState &controllerState = controllerStates[controllerIndex];

        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        controllerState.connected = XInputGetState(controllerIndex, &states[controllerIndex]) == ERROR_SUCCESS ? true : false;
    }

    for (DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; controllerIndex++) {
        ControllerState &controllerState = controllerStates[controllerIndex];

        if (!controllerState.connected) {
            continue;
        }

        const XINPUT_STATE &state = states[controllerIndex];

        bool newButtonStates[GamePadButton::Count];
        newButtonStates[GamePadButton::A] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_A);
        newButtonStates[GamePadButton::B] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_B);
        newButtonStates[GamePadButton::X] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_X);
        newButtonStates[GamePadButton::Y] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_Y);
        newButtonStates[GamePadButton::L1] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
        newButtonStates[GamePadButton::R1] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
        newButtonStates[GamePadButton::L2] = !!(state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
        newButtonStates[GamePadButton::R2] = !!(state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
        newButtonStates[GamePadButton::Back] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK);
        newButtonStates[GamePadButton::Start] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_START);
        newButtonStates[GamePadButton::LeftThumb] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
        newButtonStates[GamePadButton::RightThumb] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
        newButtonStates[GamePadButton::DPadUp] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP);
        newButtonStates[GamePadButton::DPadDown] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
        newButtonStates[GamePadButton::DPadLeft] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
        newButtonStates[GamePadButton::DPadRight] = !!(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
        newButtonStates[GamePadButton::LeftStickUp] = !!(state.Gamepad.sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        newButtonStates[GamePadButton::LeftStickDown] = !!(state.Gamepad.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        newButtonStates[GamePadButton::LeftStickLeft] = !!(state.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        newButtonStates[GamePadButton::LeftStickRight] = !!(state.Gamepad.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        newButtonStates[GamePadButton::RightStickUp] = !!(state.Gamepad.sThumbLY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        newButtonStates[GamePadButton::RightStickDown] = !!(state.Gamepad.sThumbLY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        newButtonStates[GamePadButton::RightStickLeft] = !!(state.Gamepad.sThumbLX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        newButtonStates[GamePadButton::RightStickRight] = !!(state.Gamepad.sThumbLX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

        for (int buttonIndex = 0; buttonIndex < GamePadButton::Count; buttonIndex++) {
            if (newButtonStates[buttonIndex] != controllerState.buttonStates[buttonIndex]) {
                platform->QueEvent(Platform::EventType::Key, joyKeys[buttonIndex], newButtonStates[buttonIndex], controllerIndex, nullptr);
            }

            controllerState.buttonStates[buttonIndex] = newButtonStates[buttonIndex];
        }

        controllerState.leftTriggerAnalog = (float)state.Gamepad.bLeftTrigger / 255;
        controllerState.rightTriggerAnalog = (float)state.Gamepad.bRightTrigger / 255;

        controllerState.leftXAnalog = Max((float)state.Gamepad.sThumbLX / 32767.0f, -1.0f);
        controllerState.leftYAnalog = Max((float)state.Gamepad.sThumbLY / 32767.0f, -1.0f);
        controllerState.rightXAnalog = Max((float)state.Gamepad.sThumbRX / 32767.0f, -1.0f);
        controllerState.rightYAnalog = Max((float)state.Gamepad.sThumbRY / 32767.0f, -1.0f);

        // Apply force feedback
        XINPUT_VIBRATION vibration;

        vibration.wLeftMotorSpeed = (WORD)(controllerState.forceFeedback.leftValue * 65535.0f);
        vibration.wRightMotorSpeed = (WORD)(controllerState.forceFeedback.rightValue * 65535.0f);

        XInputSetState(controllerState.controllerId, &vibration);
    }
}

void XInputInterface::SetForceFeedbackValues(int controllerIndex, float leftValue, float rightValue) {
    if (controllerIndex < 0 || controllerIndex >= XUSER_MAX_COUNT) {
        return;
    }

    ControllerState &controllerState = controllerStates[controllerIndex];
    if (!controllerState.connected) {
        return;
    }

    controllerState.forceFeedback.leftValue = leftValue;
    controllerState.forceFeedback.rightValue = rightValue;
}

BE_NAMESPACE_END
