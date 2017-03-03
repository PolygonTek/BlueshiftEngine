#pragma once

#include "Platform/Platform.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

class CmdArgs;

class Common {
public:
    void            Init(const char *baseDir);
    void            Shutdown();

    void            RunFrame(int frameMsec);

    Random          random;

    int             realTime;       // absolute time in milliseconds
    int             frameTime;      // frame time in milliseconds
    float           frameSec;       // frame time in seconds

private:
    void            SaveConfig(const char *filename);

    int             ProcessPlatformEvent();
    void            GetPlatformEvent(Platform::Event *ev);

    static void     Cmd_Version(const CmdArgs &args);
    static void     Cmd_Error(const CmdArgs &args);
    static void     Cmd_Quit(const CmdArgs &args);
};

extern Common       common;

BE_NAMESPACE_END
