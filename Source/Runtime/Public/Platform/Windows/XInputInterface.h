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

#include <XInput.h>

BE_NAMESPACE_BEGIN

class XInputInterface {
public:
    struct GamePadButton {
        enum Enum {
            A,
            B,
            X,
            Y,
            L1,
            R1,
            L2,
            R2,
            Back,
            Start,
            LeftThumb,
            RightThumb,
            DPadUp,
            DPadDown,
            DPadLeft,
            DPadRight,
            LeftStickUp,
            LeftStickDown,
            LeftStickLeft,
            LeftStickRight,
            RightStickUp,
            RightStickDown,
            RightStickLeft,
            RightStickRight,
            Count
        };
    };

    XInputInterface();
    ~XInputInterface() = default;

    void                SendControllerEvents();

    void                SetForceFeedbackValues(int controllerIndex, float leftValue, float rightValue);

private:
    struct ForceFeedback {
        float           leftValue = 0.0f;
        float           rightValue = 0.0f;
    };

    struct ControllerState {
        int             controllerId = 0;
        float           leftTriggerAnalog = 0;
        float           rightTriggerAnalog = 0;
        float           leftXAnalog = 0;
        float           leftYAnalog = 0;
        float           rightXAnalog = 0;
        float           rightYAnalog = 0;
        ForceFeedback   forceFeedback;
        bool            connected = false;
        bool            buttonStates[GamePadButton::Count];
    };

    ControllerState     controllerStates[XUSER_MAX_COUNT];
    int                 joyKeys[GamePadButton::Count];
};

BE_NAMESPACE_END
