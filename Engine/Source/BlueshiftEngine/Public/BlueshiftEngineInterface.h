#pragma once

#include "Core/Str.h"
#include "Core/CmdArgs.h"

BE_NAMESPACE_BEGIN

class Engine {
public:
    struct InitParms {
        CmdArgs             args;
        Str                 baseDir;
        Str                 searchPath;
    };

    static void             Init(const InitParms *initParms);
    static void             Shutdown();

    static void             InitBase(const char *path, bool forceGenericSIMD, const streamOutFunc_t logFunc, const streamOutFunc_t errorFunc);
    static void             ShutdownBase();

    static void             RunFrame(int elapsedMsec);

    static CmdArgs          args;
    static Str              baseDir;
    static Str              searchPath;
};

BE_NAMESPACE_END