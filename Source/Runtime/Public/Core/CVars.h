// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#if defined(_DEBUG)
#define BE_CVAR_ASSERT_MAIN_THREAD() AssertMainThreadAccess()
#else
#define BE_CVAR_ASSERT_MAIN_THREAD() ((void)0)
#endif

/*
-------------------------------------------------------------------------------

    Console Variables

-------------------------------------------------------------------------------
*/

#define CVAR(name, value, flags, description) \
    BE1::CVar name(#name, value, flags, description)

#define CVAR_MINMAX(name, value, flags, description, minimum, maximum) \
    BE1::CVar name(#name, value, flags, description, minimum, maximum)

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

    CVar() = delete;
    CVar(const char *name, const char *value, int flags, const char *description);
    CVar(const char *name, const char *value, int flags, const char *description, float valueMin, float valueMax);
    ~CVar() {}

    const char *            GetName() const { return name; }
    const char *            GetDescription() const { return desc; }
    int                     GetFlags() const { BE_CVAR_ASSERT_MAIN_THREAD(); return flags; }
    float                   GetMinValue() const { return valueMin; }
    float                   GetMaxValue() const { return valueMax; }
    
    bool                    IsModified() const { BE_CVAR_ASSERT_MAIN_THREAD(); return flags & Flag::Modified; }
    void                    SetModified() { BE_CVAR_ASSERT_MAIN_THREAD(); flags |= Flag::Modified; }
    void                    ClearModified() { BE_CVAR_ASSERT_MAIN_THREAD(); flags &= ~Flag::Modified; }

    const char *            GetString() const { BE_CVAR_ASSERT_MAIN_THREAD(); return valueString; }
    float                   GetFloat() const { BE_CVAR_ASSERT_MAIN_THREAD(); return valueFloat; }
    int                     GetInteger() const { BE_CVAR_ASSERT_MAIN_THREAD(); return valueInteger; }
    bool                    GetBool() const { BE_CVAR_ASSERT_MAIN_THREAD(); return (valueInteger != 0); }

    void                    SetString(const char *value);
    void                    SetFloat(const float value) { SetString(va("%f", value)); }
    void                    SetInteger(const int value) { SetString(va("%i", value)); }
    void                    SetBool(const bool value) { SetString(value ? "1" : "0"); }

    static void             RegisterStaticCVars();
    
protected:
#if defined(_DEBUG)
    // CVar 값과 modified 상태는 CVarSystem::Init() 을 호출한 스레드에서만 접근한다.
    static void             AssertMainThreadAccess();
#endif

    const char *            name;           ///< name
    const char *            defaultValue;   ///< default value
    const char *            desc;           ///< description
    int                     flags;          ///< CVAR_? flags
    float                   valueMin;       ///< minimum value
    float                   valueMax;       ///< maximum value
    char *                  valueString;    ///< valid string value
    float                   valueFloat;     ///< atof(value)
    int                     valueInteger;   ///< atoi(value)
    CVar *                  nextStatic;

    static CVar *           staticVars;
    
    void                    Init(const char *name, const char *value, int flags, const char *description, float valueMin, float valueMax);
};

#undef BE_CVAR_ASSERT_MAIN_THREAD

class CVarSystem {
public:
    friend class CVar;
    friend class CmdSystem;

    CVarSystem();
    
    void                    Init();
    void                    Shutdown();

    bool                    IsInitialized() const { return initialized; }

    const char *            CompleteVariable(const char *partial);

    void                    Register(CVar *cvar);

    const Array<CVar *> &   GetArray() const { return cvars; }

    CVar *                  Find(const char *name) const;

    CVar *                  SetCVarString(const char *name, const char *value);
    CVar *                  SetCVarBool(const char *name, const bool value);
    CVar *                  SetCVarFloat(const char *name, const float value);
    CVar *                  SetCVarInteger(const char *name, const int value);

    const char *            GetCVarString(const char *name) const;
    bool                    GetCVarBool(const char *name) const;
    float                   GetCVarFloat(const char *name) const;
    int                     GetCVarInteger(const char *name) const;

    void                    ClearModified();

    void                    WriteVariables(File *fp) const;

private:
#if defined(_DEBUG)
    void                    AssertMainThreadAccess() const;
    uint64_t                mainThreadId = 0;
#endif

    Array<CVar *>           cvars;
    HashIndex               cvarHash;
    bool                    initialized;
    
    bool                    Command(const CmdArgs &args);

    CVar *                  Set(const char *name, const char *value);

    void                    ValidateMinMax(CVar *cvar);
    
    static void             Cmd_ListCVars(const CmdArgs &args);
    static void             Cmd_Restart(const CmdArgs &args);
    static void             Cmd_Reset(const CmdArgs &args);
    static void             Cmd_Set(const CmdArgs &args);
    static void             Cmd_Toggle(const CmdArgs &args);
};

extern CVarSystem           cvarSystem;

BE_INLINE CVar::CVar(const char *name, const char *value, int flags, const char *description) {
    Init(name, value, flags, description, static_cast<float>(INT_MAX), static_cast<float>(INT_MIN));
}

BE_INLINE CVar::CVar(const char *name, const char *value, int flags, const char *description, float valueMin, float valueMax) {
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
