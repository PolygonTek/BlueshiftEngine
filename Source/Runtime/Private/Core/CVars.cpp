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
#include "File/File.h"
#include "Core/Cmds.h"
#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
//
//	CVar
//
//--------------------------------------------------------------------------------------------------

CVar *      CVar::staticVars = nullptr;

void CVar::Init(const wchar_t *name, const wchar_t *value, int flags, const wchar_t *description, float valueMin, float valueMax) {
    this->name          = name;
    this->defaultValue  = value;
    this->desc          = description;
    this->flags         = flags;
    this->valueMin      = valueMin;
    this->valueMax      = valueMax;
    this->valueString   = nullptr;
    this->valueFloat    = 0.0f;
    this->valueInteger  = 0;

    if (CVar::staticVars != (CVar *)(-1)) {
        this->flags |= Static;
        this->nextStatic = CVar::staticVars;
        CVar::staticVars = this;
    } else {
        cvarSystem.Register(this);
    }
}

void CVar::SetString(const wchar_t *value) {
    cvarSystem.SetCVarString(name, value);
}

//--------------------------------------------------------------------------------------------------
//
//	CVarSystem
//
//--------------------------------------------------------------------------------------------------

CVarSystem      cvarSystem;

void CVarSystem::Init() {
    initialized = true;

    cvars.Resize(1024);

    CVar::RegisterStaticCVars();

    cmdSystem.AddCommand(L"listCvars", Cmd_ListCVars);
    cmdSystem.AddCommand(L"cvar_restart", Cmd_Restart);
    cmdSystem.AddCommand(L"reset", Cmd_Reset);
    cmdSystem.AddCommand(L"set", Cmd_Set);
    cmdSystem.AddCommand(L"toggle", Cmd_Toggle);
}

void CVarSystem::Shutdown() {
    initialized = false;

    cmdSystem.RemoveCommand(L"listCvars");
    cmdSystem.RemoveCommand(L"cvar_restart");
    cmdSystem.RemoveCommand(L"reset");
    cmdSystem.RemoveCommand(L"set");
    cmdSystem.RemoveCommand(L"toggle");

    for (int i = 0; i < cvars.Count(); i++) {
        Mem_Free(cvars[i]->valueString);

        if (!(cvars[i]->flags & CVar::Static)) {
            Mem_Free((void *)cvars[i]->name);
            Mem_Free((void *)cvars[i]->defaultValue);
            delete cvars[i];
        }
    }

    cvars.Clear();
    cvarHash.Free();
}

const wchar_t *CVarSystem::CompleteVariable(const wchar_t *partial) {	
    int len = WStr::Length(partial);
    if (!len) {
        return nullptr;
    }

    int i;

    // Check full name 
    for (i = 0; i < cvars.Count(); i++) {
        if (!WStr::Icmp(partial, cvars[i]->name)) {
            return cvars[i]->name;
        }
    }

    // Check partial name
    for (i = 0; i < cvars.Count(); i++) {
        if (!WStr::Icmpn(partial, cvars[i]->name, len)) {
            return cvars[i]->name;
        }
    }

    return nullptr;
}

CVar *CVarSystem::Find(const wchar_t *name) const {
    if (!name || WStr::FindChar(name, L'\\') != -1 || WStr::FindChar(name, L'\"') != -1 || WStr::FindChar(name, L';') != -1) {
        BE_WARNLOG(L"CVarSystem::Find: invalid cvar name string: %ls", name);
        return nullptr;
    }

    int hash = cvarHash.GenerateHash(name, false);
    for (int i = cvarHash.First(hash); i != -1; i = cvarHash.Next(i)) {
        if (!WStr::Icmp(cvars[i]->name, name)) {
            return cvars[i];
        }
    }

    return nullptr;
}

void CVarSystem::Register(CVar *cvar) {
    int hash = cvarHash.GenerateHash(cvar->name, false);
    int i;
    for (i = cvarHash.First(hash); i != -1; i = cvarHash.Next(i)) {
        if (!WStr::Icmp(cvars[i]->name, cvar->name)) {
            break;
        }
    }
    
    if (i == -1) {
        cvar->valueString   = (wchar_t *)Mem_AllocWideString(cvar->defaultValue);
        cvar->valueFloat    = (float)wcstof(cvar->valueString, nullptr);
        cvar->valueInteger  = (int)wcstoul(cvar->valueString, nullptr, 0);

        i = cvars.Append(cvar);
        cvarHash.Add(hash, i);
    }	
}

void CVarSystem::WriteVariables(File *fp) const {
    for (int i = 0; i < cvars.Count(); i++) {
        CVar *cvar = cvars[i];

        if (cvar->flags & CVar::Archive) {
            fp->Printf("set %ls \"%ls\"\n", cvar->name, cvar->valueString);
        }
    }
}

CVar *CVarSystem::Set(const wchar_t *name, const wchar_t *value) {
    if (!name || WStr::FindChar(name, L'\\') != -1 || WStr::FindChar(name, L'\"') != -1 || WStr::FindChar(name, L';') != -1) {
        BE_WARNLOG(L"CVarSystem::Set: invalid cvar name string: %ls", name);
        return nullptr;
    }

    CVar *cvar = nullptr;
    int hash = cvarHash.GenerateHash(name, false);
    int i;
    for (i = cvarHash.First(hash); i != -1; i = cvarHash.Next(i)) {
        if (!WStr::Icmp(cvars[i]->name, name)) {
            cvar = cvars[i];
            break;
        }
    }
    
    if (cvar == nullptr) {
        return nullptr;
    }

    if (cvar->flags & CVar::ReadOnly) {
        BE_LOG(L"%ls is readonly\n", name);
        return cvar;
    }

    if (!value) {
        value = cvar->defaultValue;
    }

    if (!WStr::Cmp(cvar->valueString, value)) {
        return cvar;
    }

    cvar->flags |= CVar::Modified;
    Mem_Free(cvar->valueString);
    cvar->valueString   = Mem_AllocWideString(value);
    cvar->valueFloat    = wcstof(value, nullptr);
    cvar->valueInteger  = (int)wcstoul(value, nullptr, 0);
    ValidateMinMax(cvar);

    return cvar;
}

CVar *CVarSystem::SetCVarString(const wchar_t *name, const wchar_t *value) {
    return Set(name, value);
}

CVar *CVarSystem::SetCVarBool(const wchar_t *name, const bool value) {
    return Set(name, (value ? L"1" : L"0"));
}

CVar *CVarSystem::SetCVarFloat(const wchar_t *name, const float value) {
    return Set(name, wva(L"%f", value));
}

CVar *CVarSystem::SetCVarInteger(const wchar_t *name, const int value) {
    return Set(name, wva(L"%i", value));
}

const wchar_t *CVarSystem::GetCVarString(const wchar_t *name) const {
    CVar *cvar = Find(name);
    if (!cvar) {
        return L"";
    }
    return cvar->valueString;
}

bool CVarSystem::GetCVarBool(const wchar_t *name) const {
    CVar *cvar = Find(name);
    if (!cvar) {
        return false;
    }
    return (cvar->valueInteger ? true : false);
}

float CVarSystem::GetCVarFloat(const wchar_t *name) const {
    CVar *cvar = Find(name);
    if (!cvar) {
        return 0.0f;
    }
    return cvar->valueFloat;
}

int CVarSystem::GetCVarInteger(const wchar_t *name) const {
    CVar *cvar = Find(name);
    if (!cvar) {
        return 0;
    }
    return cvar->valueInteger;
}

void CVarSystem::ClearModified() {
    for (int i = 0; i < cvars.Count(); i++) {
        CVar *cvar = cvars[i];
        cvar->ClearModified();
    }
}

bool CVarSystem::Command(const CmdArgs &args) {
    CVar *cvar = Find(args.Argv(0));
    if (!cvar) {
        return false;
    }

    if (args.Argc() == 1) {
        // Print cvar information if no parameters are specified
        BE_LOG(L"%ls is: \"%ls\" default: \"%ls\"\ndescription: %ls\n", cvar->name, cvar->valueString, cvar->defaultValue, cvar->desc);
        return true;
    }

    Set(cvar->name, args.Argv(1));
    return true;
}

void CVarSystem::ValidateMinMax(CVar *cvar) {
    if (cvar->flags & (CVar::Float | CVar::Integer) && (cvar->valueMax - cvar->valueMin) > 0.0f) {
        if (cvar->valueFloat < cvar->valueMin) {
            BE_WARNLOG(L"cvar '%ls' out of range (%.3f < %.3f)\n", cvar->name, cvar->valueFloat, cvar->valueMin);
            cvar->SetFloat(cvar->valueMin);
            return;
        }

        if (cvar->valueFloat > cvar->valueMax) {
            BE_WARNLOG(L"cvar '%ls' out of range (%.3f > %.3f)\n", cvar->name, cvar->valueFloat, cvar->valueMax);
            cvar->SetFloat(cvar->valueMax);
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------------
//
//  CVar command
//
//--------------------------------------------------------------------------------------------------

void CVarSystem::Cmd_ListCVars(const CmdArgs &args) {
    const wchar_t *partialString;
    int len;

    if (args.Argc() > 1) {
        partialString = args.Argv(1);
        len = WStr::Length(partialString);
    } else {
        partialString = nullptr;
        len = 0;
    }

    int num = 0;
    for (int i = 0; i < cvarSystem.cvars.Count(); i++) {
        CVar *cvar = cvarSystem.cvars[i];

        if (partialString && WStr::Icmpn(partialString, cvar->name, len)) {
            continue;
        }

        wchar_t flags[32];
        flags[0] = cvar->flags & CVar::Archive  ? L'A' : L' ';
        flags[1] = cvar->flags & CVar::ReadOnly ? L'R' : L' ';
        flags[2] = 0;
        
        const wchar_t *type;
        if (cvar->flags & CVar::Float) {
            type = L"float";
        } else if (cvar->flags & CVar::Integer) {
            type = L"integer";
        } else if (cvar->flags & CVar::Bool) {
            type = L"bool";
        } else {
            type = L"string";
        }

        BE_LOG(L"%4s %8s %ls = %ls\n", flags, type, cvar->name, cvar->valueString);

        num++;
    }

    BE_LOG(L"%i total cvars", num);
    if (partialString) {
        BE_LOG(L" beginning with \"%ls\"", partialString);
    }
    BE_LOG(L"\n");
}

void CVarSystem::Cmd_Restart(const CmdArgs &args) {
    for (int i = 0; i < cvarSystem.cvars.Count(); i++) {
        CVar *cvar = cvarSystem.cvars[i];
        cvarSystem.Set(cvar->name, cvar->defaultValue);
    }

    BE_LOG(L"all cvars set to default\n");
}

void CVarSystem::Cmd_Reset(const CmdArgs &args) {
    if (args.Argc() > 1) {
        cvarSystem.Set(args.Argv(1), nullptr);
    } else {
        BE_LOG(L"usage: reset <variable>\n");
    }
}

void CVarSystem::Cmd_Set(const CmdArgs &args) {
    if (args.Argc() > 2) {
        cvarSystem.Set(args.Argv(1), args.Argv(2));
    } else {
        BE_LOG(L"usage: set <variable> <value>\n");
    }
}

void CVarSystem::Cmd_Toggle(const CmdArgs &args) {
    if (args.Argc() == 1) {
        BE_LOG(L"usage:\n");
        BE_LOG(L"toggle <variable> - toggles between 0 and 1\n");
        BE_LOG(L"toggle <variable> <value> - toggles between 0 and <value>\n");
        BE_LOG(L"toggle <variable> [string-1] [string-2]...[string-n] - cycles through all strings\n");
        return;
    }

    const wchar_t *arg = args.Argv(1);
    CVar *cvar = cvarSystem.Find(arg);
    if (!cvar) {
        BE_LOG(L"unknown variable\n");
        return;
    }

    if (args.Argc() == 2) {
        if (cvar->GetBool()) {
            cvarSystem.Set(arg, L"0");
        } else {
            cvarSystem.Set(arg, L"1");
        }
    } else if (args.Argc() == 3) {
        const wchar_t *arg2 = args.Argv(2);
        
        if (cvar->GetInteger() == wcstoul(arg2, nullptr, 0)) {
            cvarSystem.Set(arg, L"0");
        } else {
            cvarSystem.Set(arg, arg2);
        }
    } else {
        int i;
        for (i = 0; i < args.Argc() - 2; i++) {
            if (!WStr::Cmp(args.Argv(2 + i), cvar->valueString)) {
                i++;
                break;
            }
        }

        i %= args.Argc() - 2;

        cvarSystem.Set(arg, args.Argv(2 + i));
    }
}

BE_NAMESPACE_END
