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
    const char *            name;
    const char *            description;
    cmdFunction_t           function;
    Cmd *                   next;
};

class BE_API CmdSystem {
public:
    static constexpr int MaxBufferLength = 8192;

    struct Execution {
        enum Enum {
            Now,            ///< don't return until completed
            Insert,         ///< insert at current position, but don't run yet
            Append          ///< add to end of the command buffer (normal case)
        };
    };

    void                    Init();
    void                    Shutdown();

    const char *            CompleteCommand(const char *partial);

    Cmd *                   GetList() const { return cmdList; }

    Cmd *                   Find(const char *name);

    void                    AddCommand(const char *name, cmdFunction_t function, const char *description = nullptr);
    void                    RemoveCommand(const char *name);

    void                    BufferCommandText(Execution::Enum exec, const char *text);

                            /// Executes commands pending in the buffer.
    void                    ExecuteCommandBuffer();

private:
    char                    commandBuffer[MaxBufferLength];     ///< space for commands and script files
    unsigned int            commandBufferSize;
    unsigned int            commandBufferWait;
    Cmd *                   cmdList;

    void                    ExecuteString(const char *text);

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
