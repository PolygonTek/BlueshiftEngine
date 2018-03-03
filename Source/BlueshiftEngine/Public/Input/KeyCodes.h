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

BE_NAMESPACE_BEGIN

struct KeyCode {
    // Key scan code
    enum Enum {
        None,
        Escape,
        Num1,           // 1 !
        Num2,           // 2 @
        Num3,           // 3 #
        Num4,           // 4 $
        Num5,           // 5 %
        Num6,           // 6 ^
        Num7,           // 7 &
        Num8,           // 8 *
        Num9,           // 9 (
        Num0,           // 0 )
        Minus,          // - _
        Equals,         // = +
        Backspace,
        Tab,
        Q,
        W,
        E,
        R,
        T,
        Y,
        U,
        I,
        O,
        P,
        LeftBracket,    // [ {
        RightBracket,   // ] }
        Enter,
        LeftControl,
        A,
        S,
        D,
        F,
        G,
        H,
        J,
        K,
        L,
        SemiColon,      // ; :
        Apostrophe,     // ' "
        Grave,          // ` ~
        LeftShift,
        BackSlash,      // \ |
        Z,
        X,
        C,
        V,
        B,
        N,
        M,
        Comma,          // , <
        Period,         // . >
        Slash,          // / ?
        RightShift,
        KeyPadStar,
        LeftAlt,
        Space,
        CapsLock,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        NumLock,
        ScrollLock,
        KeyPad7,
        KeyPad8,
        KeyPad9,
        KeyPadMinus,
        KeyPad4,
        KeyPad5,
        KeyPad6,
        KeyPadPlus,
        KeyPad1,
        KeyPad2,
        KeyPad3,
        KeyPad0,
        KeyPadDot,
        F11 = 0x57,
        F12 = 0x58,
        F13 = 0x64,
        F14 = 0x65,
        F15 = 0x66,
        Kana = 0x70,
        Convert = 0x79,
        NoConvert = 0x7B,
        Yen = 0x7D,
        KeyPadEquals = 0x8D,
        Circumflex = 0x90,
        At = 0x91,
        Colon = 0x92,
        UnderLine = 0x93,
        Kanji = 0x94,
        Stop = 0x95,
        Ax = 0x96,
        Unlabeled = 0x97,
        KeyPadEnter = 0x9C,
        RightControl = 0x9D,
        KeyPadComma = 0xB3,
        KeyPadSlash = 0xB5,
        PrintScreen = 0xB7,
        RightAlt = 0xB8,
        Pause = 0xC5,
        Home = 0xC7,
        UpArrow = 0xC8,
        PageUp = 0xC9,
        LeftArrow = 0xCB,
        RightArrow = 0xCD,
        End = 0xCF,
        DownArrow = 0xD0,
        PageDown = 0xD1,
        Insert = 0xD2,
        Delete = 0xD3,
        LeftWin = 0xDB,
        RightWin = 0xDC,
        Apps = 0xDD,
        Power = 0xDE,
        Sleep = 0xDF,

        //------------------------
        // K_Joy codes must be contiguous, too
        //------------------------

        Joy1 = 256,
        Joy2,
        Joy3,
        Joy4,
        Joy5,
        Joy6,
        Joy7,
        Joy8,
        Joy9,
        Joy10,
        Joy11,
        Joy12,
        Joy13,
        Joy14,
        Joy15,
        Joy16,

        Joystick1Up,
        Joystick1Down,
        Joystick1Left,
        Joystick1Right,

        Joystick2Up,
        Joystick2Down,
        Joystick2Left,
        Joystick2Right,

        //------------------------
        // K_MOUSE enums must be contiguous (no char codes in the middle)
        //------------------------

        Mouse1,
        Mouse2,
        Mouse3,
        Mouse4,
        Mouse5,
        Mouse6,
        Mouse7,
        Mouse8,

        MouseWheelDown,
        MouseWheelUp,

        LastKey
    };
};

BE_NAMESPACE_END