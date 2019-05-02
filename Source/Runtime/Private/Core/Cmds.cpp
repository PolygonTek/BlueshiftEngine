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
#include "Core/Str.h"
#include "Core/Heap.h"
#include "File/FileSystem.h"
#include "Core/CVars.h"
#include "Core/Cmds.h"

BE_NAMESPACE_BEGIN

CmdSystem       cmdSystem;

void CmdSystem::Init() {
    commandBuffer[0] = 0;
    commandBufferSize = 0;
    commandBufferWait = 0;

    cmdList = nullptr;
    
    AddCommand("listCmds", Cmd_ListCmds);
    AddCommand("exec", Cmd_Exec);
    AddCommand("vstr", Cmd_Vstr);
    AddCommand("inc", Cmd_Inc);
    AddCommand("dec", Cmd_Dec);
    AddCommand("echo", Cmd_Echo);
    AddCommand("wait", Cmd_Wait);
    AddCommand("crash", Cmd_Crash);
    AddCommand("date", Cmd_Date);
}

void CmdSystem::Shutdown() {
    Cmd *next;
    for (Cmd *cmd = cmdList; cmd; cmd = next) {
        next = cmd->next;
        Mem_Free(cmd);
    }

    cmdList = nullptr;
}

const char *CmdSystem::CompleteCommand(const char *partial) {
    int len = UTF8::Length(partial);
    if (!len) {
        return nullptr;
    }

    Cmd *cmd;

    // 동일한 것이 있나 먼저 검사
    for (cmd = cmdList; cmd; cmd = cmd->next) {
        if (!Str::Icmp(partial, cmd->name)) {
            return cmd->name;
        }
    }
        
    // 없다면 부분 매치검색
    for (cmd = cmdList; cmd; cmd = cmd->next) {
        if (!Str::Icmpn(partial, cmd->name, len)) {
            return cmd->name;
        }
    }

    return nullptr;
}

Cmd *CmdSystem::Find(const char *name) {
    for (Cmd *cmd = cmdList; cmd; cmd = cmd->next) {
        if (!Str::Icmp(cmd->name, name)) {
            return cmd;
        }
    }

    return nullptr;
}

void CmdSystem::AddCommand(const char *name, cmdFunction_t function, const char *description) {
    if (!name) {
        return;
    }

    if (Find(name)) {
        BE_WARNLOG("CmdSystem::AddCommand: %s already defined\n", name);
        return;
    }
            
    if (cvarSystem.Find(name)) {
        // cvar 와 이름이 중복됐다
        BE_WARNLOG("CmdSystem::AddCommand: %s already defined as a var\n", name);
        return;
    }

    Cmd *cmd = (Cmd *)Mem_Alloc(sizeof(Cmd));
    cmd->name           = name;
    cmd->description    = description;
    cmd->function       = function;
    cmd->next           = cmdList;
    cmdList             = cmd;
}

void CmdSystem::RemoveCommand(const char *name) {
    if (!cmdList) {
        return;
    }

    Cmd *cmd;
    for (Cmd **back = &cmdList; ; back = &cmd->next) {
        cmd = *back;

        if (!cmd) {
            BE_WARNLOG("CmdSystem::RemoveCommand: %s not added\n", name);
            return;
        }

        if (!Str::Icmp(name, cmd->name)) {
            *back = cmd->next;
            Mem_Free(cmd);
            return;
        }
    }
}

void CmdSystem::BufferCommandText(Execution::Enum exec, const char *text) {
    if (exec == Execution::Now) {
        BufferCommandText(Execution::Insert, text);
        ExecuteCommandBuffer();
    } else if (exec == Execution::Insert) {
        // copy off any commands still remaining in the exec buffer
        char *temp;
        int tempSize = commandBufferSize;

        if (tempSize) {
            temp = (char *)Mem_Alloc(sizeof(char) * tempSize);
            memcpy(temp, commandBuffer, sizeof(char) * tempSize);
            commandBufferSize = 0;
        } else {
            temp = nullptr; // shut up compiler
        }
            
        // add the entire text of the file
        BufferCommandText(Execution::Append, text);
        
        // add the copied off data
        if (tempSize) {
            if ((commandBufferSize + tempSize + 1) <= MaxBufferLength) {
                memcpy(commandBuffer + commandBufferSize, temp, sizeof(char) * tempSize);
            } else {
                BE_WARNLOG("CmdSystem::BufferCommandText: overflowed\n");
            }

            Mem_Free(temp);
            commandBufferSize += tempSize;
        }
    } else {
        int l = Str::Length(text);
        if (commandBufferSize + l >= MaxBufferLength) {
            BE_WARNLOG("CmdSystem::BufferCommandText: overflow\n");
            return;
        }

        memcpy(&commandBuffer[commandBufferSize], text, sizeof(char) * l);
        commandBufferSize += l;
    }
}

void CmdSystem::ExecuteString(const char *text) {
    CmdArgs args(text, true);

    if (!args.Argc()) {
        return;
    }

    // HACK!
    Str cmdName;
    const char *cmd0 = args.Argv(0);
    cmdName.Append(cmd0);

    if (cmd0[0] == '+' || cmd0[0] == '-') {
        cmdName.Append(args.Argv(1));
    }

    // Execute a command
    for (Cmd *cmd = cmdList; cmd; cmd = cmd->next) {
        if (!Str::Icmp(cmdName.c_str(), cmd->name)) {
            cmd->function(args);
            return;
        }
    }
    
    // Process a cvar command
    if (cvarSystem.Command(args)) {
        return;
    }

    BE_LOG("Unknown command \"%s\"\n", args.Argv(0));
}

void CmdSystem::ExecuteCommandBuffer() {
    static const int MaxCommandLine = 1024;
    char commandLine[MaxCommandLine];

    while (commandBufferSize) {
        char *text = (char *)commandBuffer;
        int quotes = 0;
        int i;

        // '\n' or ';' 인 라인 구분자를 찾는다 
        for (i = 0; i < commandBufferSize; i++) {
            if (text[i] == '"') {
                quotes++;
            }

            // don't break if inside a quoted string
            if (!(quotes & 1) && text[i] == ';') {
                i++;
                break;
            }

            if (text[i] == '\n' || text[i] == '\r') {
                i++;
                break;
            }
        }

        if (i > MaxCommandLine - 1) {
            i = MaxCommandLine - 1;
        }
        
        Str::Copynz(commandLine, text, i + 1);
        
        if (i == commandBufferSize - 1) {
            commandBufferSize = 0;
        } else {
            commandBufferSize -= i;
            memmove(text, &text[i], sizeof(char) * commandBufferSize);
        }

        // 커맨드 라인 실행
        ExecuteString(commandLine);
        
        // commandBufferWait 명령이 켜져 있다면 나머지 커맨드들을 다음프레임으로 미룬다
        if (commandBufferWait) {
            commandBufferWait--;
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
//
//	cmd command
//
//--------------------------------------------------------------------------------------------------

void CmdSystem::Cmd_ListCmds(const CmdArgs &args) {
    const char *partial;
    int	len;

    if (args.Argc() > 1) {
        partial = args.Argv(1);
        len = Str::Length(partial);
    } else {
        partial = nullptr;
        len = 0;
    }
    
    int num = 0;
    for (Cmd *cmd = cmdSystem.cmdList; cmd; cmd = cmd->next) {
        if (partial && Str::Icmpn(partial, cmd->name, len)) {
            continue;
        }

        BE_LOG("%s\n", cmd->name);

        num++;
    }

    BE_LOG("%i commands", num);

    if (partial) {
        BE_LOG(" beginning with \"%s\"", partial);
    }
    BE_LOG("\n");
}

void CmdSystem::Cmd_Exec(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG("exec <filename> : execute a script file\n");
        return;
    }

    Str arg = args.Argv(1);
    Str filename(arg.c_str());
    filename.DefaultFileExtension("cfg");

    void *data;
    fileSystem.LoadFile(filename.c_str(), true, &data);

    if (!data) {
        BE_WARNLOG("Couldn't execute %s\n", arg.c_str());
        return;
    }

    BE_LOG("executing %s...\n", arg.c_str());
    
    cmdSystem.BufferCommandText(Execution::Insert, (const char *)data);

    fileSystem.FreeFile(data);
}

void CmdSystem::Cmd_Vstr(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG("vstr <variable-string> : execute variable command\n");
        return;
    }

    cmdSystem.BufferCommandText(Execution::Now, cvarSystem.GetCVarString(args.Argv(1)));
}

void CmdSystem::Cmd_Inc(const CmdArgs &args) {
    if (args.Argc() != 3) {
        BE_LOG("inc <variable-string> <amount> : increase variable\n");
        return;
    }

    cvarSystem.SetCVarFloat(args.Argv(1), cvarSystem.GetCVarFloat(args.Argv(1)) + atof(args.Argv(2)));
}

void CmdSystem::Cmd_Dec(const CmdArgs &args) {
    if (args.Argc() != 3) {
        BE_LOG("dec <variable-string> <amount> : decrease variable\n");
        return;
    }

    cvarSystem.SetCVarFloat(args.Argv(1), cvarSystem.GetCVarFloat(args.Argv(1)) - atof(args.Argv(2)));
}

void CmdSystem::Cmd_Echo(const CmdArgs &args) {
    for (int i = 1; i < args.Argc(); i++) {
        BE_LOG("%s ", args.Argv(i));
    }
    BE_LOG("\n");
}

void CmdSystem::Cmd_Wait(const CmdArgs &args) {
    if (args.Argc() == 2) {
        cmdSystem.commandBufferWait = (unsigned int)Str::ToUI32(args.Argv(1));
    } else {
        cmdSystem.commandBufferWait = 1;
    }
}

void CmdSystem::Cmd_Crash(const CmdArgs &args) {
    abort();
}

void CmdSystem::Cmd_Date(const CmdArgs &args) {
    time_t tm;
    time(&tm);
    struct tm *newtm = localtime(&tm);
    BE_LOG("%s", asctime(newtm));
}

BE_NAMESPACE_END
