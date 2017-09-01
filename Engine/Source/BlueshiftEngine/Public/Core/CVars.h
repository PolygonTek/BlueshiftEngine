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

    Console Variables

-------------------------------------------------------------------------------
*/

#define CVAR(name, value, flags, description) \
    BE1::CVar name(L ## #name, value, flags, description)

#define CVAR_MINMAX(name, value, flags, description, minimum, maximum) \
    BE1::CVar name(L ## #name, value, flags, description, minimum, maximum)

#include "Containers/Array.h"
#include "Containers/HashIndex.h"
#include "CmdArgs.h"

BE_NAMESPACE_BEGIN

class File;

class BE_API CVar {
    friend class CVarSystem;

public:
    enum Flag {
        All                 = -1,
        Modified            = BIT(0),
        Static              = BIT(1),
        ReadOnly            = BIT(2),
        Integer             = BIT(3),
        Float               = BIT(4),
        Bool                = BIT(5),
        Archive             = BIT(6),
    };

    // do not use default ctor
    CVar() = delete;
    CVar(const wchar_t *name, const wchar_t *value, int flags, const wchar_t *description);
    CVar(const wchar_t *name, const wchar_t *value, int flags, const wchar_t *description, float valueMin, float valueMax);
    ~CVar() {}

    const wchar_t *         GetName() const { return name; }
    const wchar_t *         GetDescription() const { return desc; }
    int                     GetFlags() const { return flags; }
    float                   GetMinValue() const { return valueMin; }
    float                   GetMaxValue() const { return valueMax; }
    
    bool                    IsModified() const { return flags & Modified; }
    void                    SetModified() { flags |= Modified; }
    void                    ClearModified() { flags &= ~Modified; }

    const wchar_t *         GetString() const { return valueString; }
    float                   GetFloat() const { return valueFloat; }
    int                     GetInteger() const { return valueInteger; }
    bool                    GetBool() const { return (valueInteger != 0); }

    void                    SetString(const wchar_t *value);
    void                    SetFloat(const float value) { SetString(wva(L"%f", value)); }
    void                    SetInteger(const int value) { SetString(wva(L"%i", value)); }
    void                    SetBool(const bool value) { SetString(value ? L"1" : L"0"); }

    static void             RegisterStaticCVars();
    
protected:
    const wchar_t *         name;           ///< name
    const wchar_t *         defaultValue;   ///< default value
    const wchar_t *         desc;           ///< description
    int                     flags;          ///< CVAR_? flags
    float                   valueMin;       ///< minimum value
    float                   valueMax;       ///< maximum value
    wchar_t *               valueString;    ///< valid string value
    float                   valueFloat;     ///< atof(value)
    int                     valueInteger;   ///< atoi(value)
    CVar *                  nextStatic;

    static CVar *           staticVars;
    
    void                    Init(const wchar_t *name, const wchar_t *value, int flags, const wchar_t *description, float valueMin, float valueMax);
};

class CVarSystem {
public:
    friend class CVar;
    friend class CmdSystem;

    CVarSystem();
    
    void                    Init();
    void                    Shutdown();

    bool                    IsInitialized() const { return initialized; }

    const wchar_t *         CompleteVariable(const wchar_t *partial);

    void                    Register(CVar *cvar);

    const Array<CVar *> &   GetArray() const { return cvars; }

    CVar *                  Find(const wchar_t *name) const;

    CVar *                  SetCVarString(const wchar_t *name, const wchar_t *value);
    CVar *                  SetCVarBool(const wchar_t *name, const bool value);
    CVar *                  SetCVarFloat(const wchar_t *name, const float value);
    CVar *                  SetCVarInteger(const wchar_t *name, const int value);

    const wchar_t *         GetCVarString(const wchar_t *name) const;
    bool                    GetCVarBool(const wchar_t *name) const;
    float                   GetCVarFloat(const wchar_t *name) const;
    int                     GetCVarInteger(const wchar_t *name) const;

    void                    ClearModified();

    void                    WriteVariables(File *fp) const;

private:
    Array<CVar *>           cvars;
    HashIndex               cvarHash;
    bool                    initialized;
    
    bool                    Command(const CmdArgs &args);

    CVar *                  Set(const wchar_t *name, const wchar_t *value);

    void                    ValidateMinMax(CVar *cvar);
    
    static void             Cmd_ListCVars(const CmdArgs &args);
    static void             Cmd_Restart(const CmdArgs &args);
    static void             Cmd_Reset(const CmdArgs &args);
    static void             Cmd_Set(const CmdArgs &args);
    static void             Cmd_Toggle(const CmdArgs &args);
};

extern CVarSystem           cvarSystem;

BE_INLINE CVar::CVar(const wchar_t *name, const wchar_t *value, int flags, const wchar_t *description) {
    Init(name, value, flags, description, INT_MAX, INT_MIN);
}

BE_INLINE CVar::CVar(const wchar_t *name, const wchar_t *value, int flags, const wchar_t *description, float valueMin, float valueMax) {
    Init(name, value, flags, description, valueMin, valueMax);
}

BE_INLINE void CVar::RegisterStaticCVars() {
    if (staticVars != reinterpret_cast<CVar *>(-1)) {
        for (CVar *cvar = CVar::staticVars; cvar; cvar = cvar->nextStatic) {
            cvarSystem.Register(cvar);
        }
        CVar::staticVars = reinterpret_cast<CVar *>(-1);
    }
}

BE_INLINE CVarSystem::CVarSystem() {
    initialized = false;
}

BE_NAMESPACE_END
