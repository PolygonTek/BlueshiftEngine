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
#include "Input/KeyCmd.h"
#include "Core/Str.h"
#include "Core/Heap.h"
#include "Core/Cmds.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

struct KeyName {
    KeyCode::Enum   keynum;
    const char *    name;
};

#define NAMEKEY(code) { KeyCode::code, #code }
#define ALIASKEY(alias, code) { KeyCode::code, alias }

static const KeyName keynames[] = {
    NAMEKEY(Escape),
    NAMEKEY(Num1),
    NAMEKEY(Num2),
    NAMEKEY(Num3),
    NAMEKEY(Num4),
    NAMEKEY(Num5),
    NAMEKEY(Num6),
    NAMEKEY(Num7),
    NAMEKEY(Num8),
    NAMEKEY(Num9),
    NAMEKEY(Num0),
    NAMEKEY(Minus),
    NAMEKEY(Equals),
    NAMEKEY(Backspace),
    NAMEKEY(Tab),
    NAMEKEY(Q),
    NAMEKEY(W),
    NAMEKEY(E),
    NAMEKEY(R),
    NAMEKEY(T),
    NAMEKEY(Y),
    NAMEKEY(U),
    NAMEKEY(I),
    NAMEKEY(O),
    NAMEKEY(P),
    NAMEKEY(LeftBracket),
    NAMEKEY(RightBracket),
    NAMEKEY(Enter),
    NAMEKEY(LeftControl),
    NAMEKEY(A),
    NAMEKEY(S),
    NAMEKEY(D),
    NAMEKEY(F),
    NAMEKEY(G),
    NAMEKEY(H),
    NAMEKEY(J),
    NAMEKEY(K),
    NAMEKEY(L),
    NAMEKEY(SemiColon),
    NAMEKEY(Apostrophe),
    NAMEKEY(Grave),
    NAMEKEY(LeftShift),
    NAMEKEY(BackSlash),
    NAMEKEY(Z),
    NAMEKEY(X),
    NAMEKEY(C),
    NAMEKEY(V),
    NAMEKEY(B),
    NAMEKEY(N),
    NAMEKEY(M),
    NAMEKEY(Comma),
    NAMEKEY(Period),
    NAMEKEY(Slash),
    NAMEKEY(RightShift),
    NAMEKEY(KeyPadStar),
    NAMEKEY(LeftAlt),
    NAMEKEY(Space),
    NAMEKEY(CapsLock),
    NAMEKEY(F1),
    NAMEKEY(F2),
    NAMEKEY(F3),
    NAMEKEY(F4),
    NAMEKEY(F5),
    NAMEKEY(F6),
    NAMEKEY(F7),
    NAMEKEY(F8),
    NAMEKEY(F9),
    NAMEKEY(F10),
    NAMEKEY(NumLock),
    NAMEKEY(ScrollLock),
    NAMEKEY(KeyPad7),
    NAMEKEY(KeyPad8),
    NAMEKEY(KeyPad9),
    NAMEKEY(KeyPadMinus),
    NAMEKEY(KeyPad4),
    NAMEKEY(KeyPad5),
    NAMEKEY(KeyPad6),
    NAMEKEY(KeyPadPlus),
    NAMEKEY(KeyPad1),
    NAMEKEY(KeyPad2),
    NAMEKEY(KeyPad3),
    NAMEKEY(KeyPad0),
    NAMEKEY(KeyPadDot),
    NAMEKEY(F11),
    NAMEKEY(F12),
    NAMEKEY(F13),
    NAMEKEY(F14),
    NAMEKEY(F15),
    NAMEKEY(Kana),
    NAMEKEY(Convert),
    NAMEKEY(NoConvert),
    NAMEKEY(Yen),
    NAMEKEY(KeyPadEquals),
    NAMEKEY(Circumflex),
    NAMEKEY(At),
    NAMEKEY(Colon),
    NAMEKEY(UnderLine),
    NAMEKEY(Kanji),
    NAMEKEY(Stop),
    NAMEKEY(Ax),
    NAMEKEY(Unlabeled),
    NAMEKEY(KeyPadEnter),
    NAMEKEY(RightControl),
    NAMEKEY(KeyPadComma),
    NAMEKEY(KeyPadSlash),
    NAMEKEY(PrintScreen),
    NAMEKEY(RightAlt),
    NAMEKEY(Pause),
    NAMEKEY(Home),
    NAMEKEY(UpArrow),
    NAMEKEY(PageUp),
    NAMEKEY(LeftArrow),
    NAMEKEY(RightArrow),
    NAMEKEY(End),
    NAMEKEY(DownArrow),
    NAMEKEY(PageDown),
    NAMEKEY(Insert),
    NAMEKEY(Delete),
    NAMEKEY(LeftWin),
    NAMEKEY(RightWin),
    NAMEKEY(Apps),
    NAMEKEY(Power),
    NAMEKEY(Sleep),

    NAMEKEY(Joy1),
    NAMEKEY(Joy2),
    NAMEKEY(Joy3),
    NAMEKEY(Joy4),
    NAMEKEY(Joy5),
    NAMEKEY(Joy6),
    NAMEKEY(Joy7),
    NAMEKEY(Joy8),
    NAMEKEY(Joy9),
    NAMEKEY(Joy10),
    NAMEKEY(Joy11),
    NAMEKEY(Joy12),
    NAMEKEY(Joy13),
    NAMEKEY(Joy14),
    NAMEKEY(Joy15),
    NAMEKEY(Joy16),

    NAMEKEY(Joystick1Up),
    NAMEKEY(Joystick1Down),
    NAMEKEY(Joystick1Left),
    NAMEKEY(Joystick1Right),

    NAMEKEY(Joystick2Up),
    NAMEKEY(Joystick2Down),
    NAMEKEY(Joystick2Left),
    NAMEKEY(Joystick2Right),

    NAMEKEY(Mouse1),
    NAMEKEY(Mouse2),
    NAMEKEY(Mouse3),
    NAMEKEY(Mouse4),
    NAMEKEY(Mouse5),
    NAMEKEY(Mouse6),
    NAMEKEY(Mouse7),
    NAMEKEY(Mouse8),

    NAMEKEY(MouseWheelDown),
    NAMEKEY(MouseWheelUp),

    //------------------------
    // Aliases to make it easier to bind or to support old configs
    //------------------------
    ALIASKEY("Alt", LeftAlt),
    ALIASKEY("RAlt", RightAlt),
    ALIASKEY("Control", LeftControl),
    ALIASKEY("Shift", LeftShift),
    ALIASKEY("Menu", Apps),
    ALIASKEY("Command", LeftAlt),

    ALIASKEY("KeyPadHome", KeyPad7),
    ALIASKEY("KeyPadUpArrow", KeyPad8),
    ALIASKEY("KeyPadPageUp", KeyPad9),
    ALIASKEY("KeyPadLeftArrow", KeyPad4),
    ALIASKEY("KeyPadRightArrow", KeyPad6),
    ALIASKEY("KeyPadEnd", KeyPad1),
    ALIASKEY("KeyPadDownArrow", KeyPad2),
    ALIASKEY("KeyPadPageDown", KeyPad3),
    ALIASKEY("KeyPadInsert", KeyPad0),
    ALIASKEY("KeyPadDelete", KeyPadDot),
    ALIASKEY("KeyPadNumLock", NumLock),

    ALIASKEY("-", Minus),
    ALIASKEY("=", Equals),
    ALIASKEY("[", LeftBracket),
    ALIASKEY("]", RightBracket),
    ALIASKEY("\\", BackSlash),
    ALIASKEY("/", Slash),
    ALIASKEY(",", Comma),
    ALIASKEY(".", Period),
    
    { KeyCode::None, nullptr }
};

KeyCmdSystem    keyCmdSystem;

KeyCode::Enum KeyCmdSystem::StringToKeynum(const char *str) {
    if (!str || !str[0]) {
        return KeyCode::None;
    }

    for (const KeyName *kn = keynames; kn->name; kn++) {
        if (!Str::Icmp(str, kn->name)) {
            return kn->keynum;
        }
    }
    return KeyCode::None;
}

const char *KeyCmdSystem::KeynumToString(KeyCode::Enum keynum) {
    if (keynum < KeyCode::None || keynum >= KeyCode::LastKey) {
        return "KEY_NOT_FOUND";
    }
    
    for (const KeyName *kn = keynames; kn->name; kn++) {
        if (keynum == kn->keynum) {
            return kn->name;
        }
    }

    return "UNKNOWN_KEYNUM";
}

void KeyCmdSystem::Init() {
    for (int i = 0; i < 256; i++) {
        keyList[i].binding = nullptr;
        keyList[i].is_down = false;
        keyList[i].count = 0;
    }

    cmdSystem.AddCommand(L"listBinds", Cmd_ListBinds);
    cmdSystem.AddCommand(L"bind", Cmd_Bind);
    cmdSystem.AddCommand(L"unbind", Cmd_Unbind);
    cmdSystem.AddCommand(L"unbindall", Cmd_UnbindAll);
}

void KeyCmdSystem::Shutdown() {
    cmdSystem.RemoveCommand(L"listBinds");
    cmdSystem.RemoveCommand(L"bind");
    cmdSystem.RemoveCommand(L"unbind");
    cmdSystem.RemoveCommand(L"unbindall");

    for (int i = 0; i < COUNT_OF(keyList); i++) {
        if (keyList[i].binding) {
            Mem_Free(keyList[i].binding);
            keyList[i].binding = nullptr;
        }
    }
}

void KeyCmdSystem::ClearStates() {
    for (int i = 0; i < COUNT_OF(keyList); i++) {
        if (keyList[i].is_down || keyList[i].count) {
            KeyEvent((KeyCode::Enum)i, false);
        }

        keyList[i].is_down = false;
        keyList[i].count = 0;
    }
}

const wchar_t *KeyCmdSystem::GetBinding(KeyCode::Enum keynum) const {
    if (keynum < KeyCode::None || keynum >= KeyCode::LastKey) {
        return L"";
    }

    return keyList[(int)keynum].binding;
}

void KeyCmdSystem::SetBinding(KeyCode::Enum keynum, const wchar_t *cmd) {
    if (keynum < KeyCode::None || keynum >= KeyCode::LastKey) {
        return;
    }

    // Remove binding for this keynum
    if (keyList[(int)keynum].binding) {
        Mem_Free(keyList[(int)keynum].binding);
        keyList[(int)keynum].binding = nullptr;
    }

    if (!cmd) {
        return;
    }
    
    keyList[(int)keynum].binding = Mem_AllocWideString(cmd);
}

void KeyCmdSystem::WriteBindings(File *fp) const {
    for (int i = 0; i < COUNT_OF(keyList); i++) {
        if (keyList[i].binding && *keyList[i].binding) {
            fp->Printf("bind \"%s\" \"%ls\"\n", KeyCmdSystem::KeynumToString((KeyCode::Enum)i), keyList[i].binding);
        }
    }
}

bool KeyCmdSystem::IsPressed(KeyCode::Enum keynum) const {
    if (keynum < KeyCode::None || keynum >= KeyCode::LastKey) {
        return false;
    }

    if (keyList[(int)keynum].count > 0) {
        return true;
    }
    
    return false;
}

bool KeyCmdSystem::IsPressedAnyKey() const {
    for (int i = 0; i < COUNT_OF(keyList); i++) {
        if (keyList[i].is_down) {
            return true;
        }
    }

    return false;
}

void KeyCmdSystem::KeyEvent(KeyCode::Enum keynum, bool down) {
    wchar_t cmd[1024];
    wchar_t *kb;

    if (keynum == KeyCode::None || keynum >= KeyCode::LastKey) {
        return;
    }

    Key *key = &keyList[(int)keynum];
    key->is_down = down;

    if (down) {
        key->count++;
    } else {
        key->count = 0;
    }

    if (!down) {
        kb = key->binding;
        if (!kb) {
            return;
        }

        if (kb[0] == L'+') {
            WStr::snPrintf(cmd, COUNT_OF(cmd), L"-%ls\n", kb + 1);
            cmdSystem.BufferCommandText(CmdSystem::Append, cmd);
        }
        return;
    }
    
    if (key->count > 1) {
        return; // 키를 누르는 동안 오는 자동 반복 신호 무시
    }
        
    if (keynum >= KeyCode::Joy1 && !key->binding) { // K_JOY1 이상은 키보드외 입력장치의 키 이벤트 신호
        //BE_LOG(L"%s is unbound.\n", KeyCmdSystem::KeynumToString(keynum));
    }
    
    kb = key->binding;
    if (kb && kb[0]) {
        cmdSystem.BufferCommandText(CmdSystem::Append, kb);
        cmdSystem.BufferCommandText(CmdSystem::Append, L"\n");
    }	
}

//--------------------------------------------------------------------------------------------------

void KeyCmdSystem::Cmd_ListBinds(const CmdArgs &args) {
    int count = 0;
    for (int i = 0; i < 256; i++) {
        if (keyCmdSystem.keyList[i].binding) {
            BE_LOG(L"%hs \"%ls\"\n", KeyCmdSystem::KeynumToString((KeyCode::Enum)i), keyCmdSystem.keyList[i].binding);
            count++;
        }
    }
}

void KeyCmdSystem::Cmd_Bind(const CmdArgs &args) {
    int argc = args.Argc();
    KeyCode::Enum keynum;

    if (argc >= 2) {
        keynum = KeyCmdSystem::StringToKeynum(WStr::ToStr(args.Argv(1)).c_str());
        if (keynum < KeyCode::None || keynum >= KeyCode::LastKey) {
            BE_WARNLOG(L"\"%ls\" isn't a valid key\n", args.Argv(1));
            return;
        }
    }

    if (argc > 2) {
        keyCmdSystem.SetBinding(keynum, args.Argv(2));
    } else if (argc == 2) {
        if (keyCmdSystem.keyList[(int)keynum].binding)
            BE_LOG(L"\"%ls\" = \"%ls\"\n", args.Argv(1), keyCmdSystem.keyList[(int)keynum].binding);
        else
            BE_LOG(L"\"%ls\" is not bound\n", args.Argv(1));
    } else {
        BE_LOG(L"bind <key> [command] : attach a command to a key\n");
    }
}

void KeyCmdSystem::Cmd_Unbind(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG(L"unbind <key> : remove commands from a key\n");
        return;
    }
    
    KeyCode::Enum keynum = KeyCmdSystem::StringToKeynum(WStr::ToStr(args.Argv(1)).c_str());
    if (keynum < KeyCode::None || keynum >= KeyCode::LastKey) {
        BE_WARNLOG(L"\"%ls\" isn't a valid key\n", args.Argv(1));
        return;
    }

    keyCmdSystem.SetBinding(keynum, nullptr);
}

void KeyCmdSystem::Cmd_UnbindAll(const CmdArgs &args) {
    for (int i = 0; i < 256; i++) {
        if (keyCmdSystem.keyList[i].binding) {
            keyCmdSystem.SetBinding((KeyCode::Enum)i, nullptr);
        }
    }
}

BE_NAMESPACE_END
