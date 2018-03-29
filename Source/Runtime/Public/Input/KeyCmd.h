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

    Key Command System

-------------------------------------------------------------------------------
*/

#include "KeyCodes.h"

BE_NAMESPACE_BEGIN

class File;
class CmdArgs;

class KeyCmdSystem {
public:
    void                    Init();
    void                    Shutdown();

    void                    ClearStates();
    void                    KeyEvent(KeyCode::Enum keynum, bool down);

    const wchar_t *         GetBinding(KeyCode::Enum keynum) const;
    void                    SetBinding(KeyCode::Enum keynum, const wchar_t *cmd);
    void                    WriteBindings(File *fp) const;

    bool                    IsPressed(KeyCode::Enum keynum) const;
    bool                    IsPressedAnyKey() const;

    static KeyCode::Enum    StringToKeynum(const char *str);
    static const char *     KeynumToString(KeyCode::Enum keynum);

private:
    struct Key {
        bool                is_down;
        int                 count;
        wchar_t *           binding;
    };

    Key                     keyList[(int)KeyCode::LastKey];

    static void             Cmd_ListBinds(const CmdArgs &args);
    static void             Cmd_Bind(const CmdArgs &args);
    static void             Cmd_Unbind(const CmdArgs &args);
    static void             Cmd_UnbindAll(const CmdArgs &args);
};

extern KeyCmdSystem         keyCmdSystem;

BE_NAMESPACE_END
