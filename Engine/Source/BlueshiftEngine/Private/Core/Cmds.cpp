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
#include "Core/WStr.h"
#include "Core/Heap.h"
#include "File/FileSystem.h"
#include "Core/CVars.h"
#include "Core/Cmds.h"

BE_NAMESPACE_BEGIN

CmdSystem       cmdSystem;

void CmdSystem::Init() {
    commandBuffer[0] = 0;
    commandBufferLen = 0;
    commandBufferWait = 0;

    cmdList = nullptr;
    
    AddCommand(L"listCmds", Cmd_ListCmds);
    AddCommand(L"exec", Cmd_Exec);
    AddCommand(L"vstr", Cmd_Vstr);
    AddCommand(L"inc", Cmd_Inc);
    AddCommand(L"dec", Cmd_Dec);
    AddCommand(L"echo", Cmd_Echo);
    AddCommand(L"wait", Cmd_Wait);
    AddCommand(L"crash", Cmd_Crash);
    AddCommand(L"date", Cmd_Date);
}

void CmdSystem::Shutdown() {
    Cmd *next;
    for (Cmd *cmd = cmdList; cmd; cmd = next) {
        next = cmd->next;
        Mem_Free(cmd);
    }

    cmdList = nullptr;
}

const wchar_t *CmdSystem::CompleteCommand(const wchar_t *partial) {	
    int len = WStr::Length(partial);
    if (!len) {
        return nullptr;
    }

    Cmd *cmd;

    // 동일한 것이 있나 먼저 검사
    for (cmd = cmdList; cmd; cmd = cmd->next) {
        if (!WStr::Icmp(partial, cmd->name)) {
            return cmd->name;
        }
    }
        
    // 없다면 부분 매치검색
    for (cmd = cmdList; cmd; cmd = cmd->next) {
        if (!WStr::Icmpn(partial, cmd->name, len)) {
            return cmd->name;
        }
    }

    return nullptr;
}

Cmd *CmdSystem::Find(const wchar_t *name) {
    for (Cmd *cmd = cmdList; cmd; cmd = cmd->next) {
        if (!WStr::Icmp(cmd->name, name)) {
            return cmd;
        }
    }

    return nullptr;
}

void CmdSystem::AddCommand(const wchar_t *name, cmdFunction_t function, const wchar_t *description) {
    if (!name) {
        return;
    }

    if (Find(name)) {
        BE_WARNLOG(L"CmdSystem::AddCommand: %ls already defined\n", name);
        return;
    }
            
    if (cvarSystem.Find(name)) {
        // cvar 와 이름이 중복됐다
        BE_WARNLOG(L"CmdSystem::AddCommand: %ls already defined as a var\n", name);
        return;
    }

    Cmd *cmd = (Cmd *)Mem_Alloc(sizeof(Cmd));
    cmd->name           = name;
    cmd->description    = description;
    cmd->function       = function;
    cmd->next           = cmdList;
    cmdList             = cmd;
}

void CmdSystem::RemoveCommand(const wchar_t *name) {
    if (!cmdList) {
        return;
    }

    Cmd *cmd;
    for (Cmd **back = &cmdList; ; back = &cmd->next) {
        cmd = *back;

        if (!cmd) {
            BE_WARNLOG(L"CmdSystem::RemoveCommand: %ls not added\n", name);
            return;
        }

        if (!WStr::Icmp(name, cmd->name)) {
            *back = cmd->next;
            Mem_Free(cmd);
            return;
        }
    }
}

void CmdSystem::BufferCommandText(Execution exec, const wchar_t *text) {
    if (exec == ExecuteNow) {
        BufferCommandText(Insert, text);
        ExecuteCommandBuffer();
    } else if (exec == Insert) {
        // copy off any commands still remaining in the exec buffer
        int templen = commandBufferLen;
        wchar_t *temp;
        if (templen) {
            temp = (wchar_t *)Mem_Alloc(sizeof(wchar_t) * templen);
            memcpy(temp, commandBuffer, sizeof(wchar_t) * templen);
            commandBufferLen = 0;
        } else {
            temp = nullptr; // shut up compiler
        }
            
        // add the entire text of the file
        BufferCommandText(Append, text);
        
        // add the copied off data
        if (templen) {
            if ((commandBufferLen + templen + 1) <= MaxBufferLength) {
                memcpy(commandBuffer + commandBufferLen, temp, sizeof(wchar_t) * templen);
            } else {
                BE_WARNLOG(L"CmdSystem::BufferCommandText: overflowed\n");
            }

            Mem_Free(temp);
            commandBufferLen += templen;
        }
    } else {
        int l = WStr::Length(text);
        if (commandBufferLen + l >= MaxBufferLength) {
            BE_WARNLOG(L"CmdSystem::BufferCommandText: overflow\n");
            return;
        }

        memcpy(&commandBuffer[commandBufferLen], text, sizeof(wchar_t) * l);
        commandBufferLen += l;
    }	
}

void CmdSystem::ExecuteString(const wchar_t *text) {
    CmdArgs args(text, true);

    if (!args.Argc()) {
        return;
    }

    // HACK!
    WStr cmdName;
    const wchar_t *cmd0 = args.Argv(0);
    cmdName.Append(cmd0);

    if (cmd0[0] == L'+' || cmd0[0] == L'-') {
        cmdName.Append(args.Argv(1));
    }

    // Execute a command
    for (Cmd *cmd = cmdList; cmd; cmd = cmd->next) {
        if (!WStr::Icmp(cmdName.c_str(), cmd->name)) {
            cmd->function(args);
            return;
        }
    }
    
    // Process a cvar command
    if (cvarSystem.Command(args)) {
        return;
    }

    BE_LOG(L"Unknown command \"%ls\"\n", args.Argv(0));
}

void CmdSystem::ExecuteCommandBuffer() {
    static const int MaxCommandLine = 1024;
    wchar_t commandLine[MaxCommandLine];

    while (commandBufferLen) {
        wchar_t *text = (wchar_t *)commandBuffer;
        int quotes = 0;
        int i;

        // '\n' or ';' 인 라인 구분자를 찾는다 
        for (i = 0; i < commandBufferLen; i++) {
            if (text[i] == L'"') {
                quotes++;
            }

            // don't break if inside a quoted string
            if (!(quotes & 1) && text[i] == L';') {
                i++;
                break;
            }

            if (text[i] == L'\n' || text[i] == L'\r') {
                i++;
                break;
            }
        }

        if (i > MaxCommandLine - 1) {
            i = MaxCommandLine - 1;
        }
        
        WStr::Copynz(commandLine, text, i + 1);
        
        if (i == commandBufferLen - 1) {
            commandBufferLen = 0;
        } else {
            commandBufferLen -= i;
            memmove(text, &text[i], sizeof(wchar_t) * commandBufferLen);
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
    const wchar_t *partial;
    int	len;

    if (args.Argc() > 1) {
        partial = args.Argv(1);
        len = WStr::Length(partial);
    } else {
        partial = nullptr;
        len = 0;
    }
    
    int num = 0;
    for (Cmd *cmd = cmdSystem.cmdList; cmd; cmd = cmd->next) {
        if (partial && WStr::Icmpn(partial, cmd->name, len)) {
            continue;
        }

        BE_LOG(L"%ls\n", cmd->name);

        num++;
    }

    BE_LOG(L"%i commands", num);

    if (partial) {
        BE_LOG(L" beginning with \"%ls\"", partial);
    }
    BE_LOG(L"\n");
}

void CmdSystem::Cmd_Exec(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG(L"exec <filename> : execute a script file\n");
        return;
    }

    WStr arg = args.Argv(1);
    
    Str filename(arg.c_str());
    filename.DefaultFileExtension("cfg");

    void *data;
    fileSystem.LoadFile(filename.c_str(), true, &data);

    if (!data) {
        BE_WARNLOG(L"Couldn't execute %ls\n", arg.c_str());
        return;
    }

    BE_LOG(L"executing %ls...\n", arg.c_str());
    
    cmdSystem.BufferCommandText(Insert, Str::ToWStr((const char *)data));

    fileSystem.FreeFile(data);
}

void CmdSystem::Cmd_Vstr(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG(L"vstr <variable-string> : execute variable command\n");
        return;
    }

    cmdSystem.BufferCommandText(ExecuteNow, cvarSystem.GetCVarString(args.Argv(1)));
}

void CmdSystem::Cmd_Inc(const CmdArgs &args) {
    if (args.Argc() != 3) {
        BE_LOG(L"inc <variable-string> <amount> : increase variable\n");
        return;
    }

    cvarSystem.SetCVarFloat(args.Argv(1), cvarSystem.GetCVarFloat(args.Argv(1)) + wcstof(args.Argv(2), nullptr));
}

void CmdSystem::Cmd_Dec(const CmdArgs &args) {
    if (args.Argc() != 3) {
        BE_LOG(L"dec <variable-string> <amount> : decrease variable\n");
        return;
    }

    cvarSystem.SetCVarFloat(args.Argv(1), cvarSystem.GetCVarFloat(args.Argv(1)) - wcstof(args.Argv(2), nullptr));
}

void CmdSystem::Cmd_Echo(const CmdArgs &args) {
    for (int i = 1; i < args.Argc(); i++) {
        BE_LOG(L"%ls ", args.Argv(i));
    }
    BE_LOG(L"\n");
}

void CmdSystem::Cmd_Wait(const CmdArgs &args) {
    if (args.Argc() == 2) {
        cmdSystem.commandBufferWait = (unsigned int)wcstol(args.Argv(1), nullptr, 10);
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
    BE_LOG(L"%hs", asctime(newtm));
}

BE_NAMESPACE_END
