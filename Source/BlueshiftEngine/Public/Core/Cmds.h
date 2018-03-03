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

    Console command system

-------------------------------------------------------------------------------
*/

#include "CmdArgs.h"

BE_NAMESPACE_BEGIN

// command function
typedef void (*cmdFunction_t)(const CmdArgs &args);

struct Cmd {
    const wchar_t *         name;
    const wchar_t *         description;
    cmdFunction_t           function;
    Cmd *                   next;
};

class BE_API CmdSystem {
public:
    enum {
        MaxBufferLength     = 8192
    };

    enum Execution {
        ExecuteNow,         ///< don't return until completed
        Insert,             ///< insert at current position, but don't run yet
        Append              ///< add to end of the command buffer (normal case)
    };

    void                    Init();
    void                    Shutdown();

    const wchar_t *         CompleteCommand(const wchar_t *partial);

    Cmd *                   GetList() const { return cmdList; }

    Cmd *                   Find(const wchar_t *name);

    void                    AddCommand(const wchar_t *name, cmdFunction_t function, const wchar_t *description = nullptr);
    void                    RemoveCommand(const wchar_t *name);

    void                    BufferCommandText(Execution exec, const wchar_t *text);

                            // buffer 에 쌓여있는 command 실행
    void                    ExecuteCommandBuffer();

private:
    wchar_t                 commandBuffer[MaxBufferLength]; ///< space for commands and script files
    unsigned int            commandBufferLen;
    unsigned int            commandBufferWait;
    Cmd *                   cmdList;

    void                    ExecuteString(const wchar_t *text);

    static void             Cmd_ListCmds(const CmdArgs &args);
    static void             Cmd_Exec(const CmdArgs &args);
    static void             Cmd_Vstr(const CmdArgs &args);
    static void             Cmd_Inc(const CmdArgs &args);
    static void             Cmd_Dec(const CmdArgs &args);
    static void             Cmd_Echo(const CmdArgs &args);
    static void             Cmd_Wait(const CmdArgs &args);
    static void             Cmd_Crash(const CmdArgs &args);
    static void             Cmd_Date(const CmdArgs &args);
};

extern CmdSystem            cmdSystem;

BE_NAMESPACE_END
