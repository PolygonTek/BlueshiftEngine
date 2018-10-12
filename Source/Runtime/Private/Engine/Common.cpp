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
#include "Core/Object.h"
#include "Input/KeyCmd.h"
#include "Input/InputSystem.h"
#include "Engine/Engine.h"
#include "Engine/GameClient.h"
#include "Engine/Console.h"
#include "Engine/Common.h"
#include "Platform/PlatformProcess.h"
#include "Simd/Simd.h"
#include "Core/StrColor.h"
#include "Core/CVars.h"
#include "Core/Cmds.h"
#include "File/FileSystem.h"
#include "Game/GameSettings.h"

BE_NAMESPACE_BEGIN

static CVAR(developer, L"0", CVar::Bool, L"");
static CVAR(logFile, L"0", CVar::Bool, L"");
static CVAR(forceGenericSIMD, L"0", CVar::Bool, L"");

static File *   consoleLogFile;

Common          common;

static void Common_Log(const int logLevel, const wchar_t *msg) {
    wchar_t buffer[16384];
    const wchar_t *bufptr;

    if (logLevel == DevLog && !developer.GetBool()) {
        return;
    }
    
    if (logLevel == DevLog) {
        WStr::snPrintf(buffer, COUNT_OF(buffer), WS_COLOR_CYAN L"%ls", msg);
        bufptr = buffer;
    } else if (logLevel == WarningLog) {
        WStr::snPrintf(buffer, COUNT_OF(buffer), WS_COLOR_YELLOW L"WARNING: %ls", msg);
        bufptr = buffer;
    } else if (logLevel == ErrorLog) {
        WStr::snPrintf(buffer, COUNT_OF(buffer), WS_COLOR_RED L"ERROR: %ls", msg);
        bufptr = buffer;
    } else {
        bufptr = msg;
    }

    if (logFile.GetBool()) {
        if (!consoleLogFile) {
            consoleLogFile = fileSystem.OpenFile("console.log", File::WriteMode);
            if (consoleLogFile) {
                time_t t;
                time(&t);
                tm *local_time = localtime(&t);
                BE_LOG(L"logFile opened on %hs\n", asctime(local_time));
            } else {
                logFile.SetBool(false);
                BE_WARNLOG(L"can't open the log file\n");
            }
        } else {
            consoleLogFile->Write(bufptr, WStr::Length(bufptr));
        }
    }

    platform->Log(bufptr); //
    
    console.Print(bufptr);
}

static void Common_Error(const int errLevel, const wchar_t *msg) {
    static bool errorEntered = false;

    if (errLevel == FatalErr) {
        if (errorEntered) {
            platform->Error(wva(L"Recursive error after: %ls", msg));
        }

        errorEntered = true;

        if (consoleLogFile) {
            fileSystem.CloseFile(consoleLogFile);
        }

        cmdSystem.BufferCommandText(CmdSystem::ExecuteNow, L"condump \"Log/log\"\n");

        //common.Shutdown();

        platform->Error(msg);
    }
}

void Common::Init(const char *baseDir) {
    Engine::InitBase(baseDir, forceGenericSIMD.GetBool(), (const streamOutFunc_t)Common_Log, (const streamOutFunc_t)Common_Error);

    EventSystem::Init();
    
    SignalSystem::Init();
    
    Object::Init();
    
    console.Init();

    keyCmdSystem.Init();

    cmdSystem.AddCommand(L"version", Cmd_Version);
    cmdSystem.AddCommand(L"error", Cmd_Error);
    cmdSystem.AddCommand(L"quit", Cmd_Quit);

    cmdSystem.BufferCommandText(CmdSystem::ExecuteNow, L"exec \"Config/config.cfg\"\n");
    cvarSystem.ClearModified();

    random.SetSeed(0);

    srand((unsigned int)time(nullptr));

    realTime = 0;
    frameTime = 0;
    frameSec = 0;
}

void Common::Shutdown() {
    cmdSystem.BufferCommandText(CmdSystem::ExecuteNow, L"condump \"Log/log\"\n");

    SaveConfig("Config/config.cfg");

    cmdSystem.RemoveCommand(L"version");
    cmdSystem.RemoveCommand(L"quit");
    cmdSystem.RemoveCommand(L"error");

    keyCmdSystem.Shutdown();

    console.Shutdown();

    Object::Shutdown();

    EventSystem::Shutdown();
    
    SignalSystem::Shutdown();

    Engine::ShutdownBase();
}

void Common::SaveConfig(const char *filename) {
    Str configFilename = filename;
    configFilename.DefaultFileExtension("cfg");

    File *file = fileSystem.OpenFile(configFilename, File::WriteMode);
    if (file) {
        file->Printf("unbindall\n");
        keyCmdSystem.WriteBindings(file);
        cvarSystem.WriteVariables(file);
        fileSystem.CloseFile(file);
    }
}

void Common::RunFrame(int frameMsec) {
    frameTime = frameMsec;
    frameSec = MS2SEC(frameMsec);
    realTime += frameMsec;

    ProcessPlatformEvent();

    cmdSystem.ExecuteCommandBuffer();
}

Common::PlatformId Common::GetPlatformId() const {
#if defined(__IOS__)
return PlatformId::IOSPlatform;
#elif defined(__ANDROID__)
return PlatformId::AndroidPlatform;
#elif defined(__WIN32__)
    return PlatformId::WindowsPlatform;
#elif defined(__MACOSX__)
    return PlatformId::MacOSPlatform;
#elif defined(__LINUX__)
    return PlatformId::LinuxPlatform;
#endif
}

Str Common::GetAppPreferenceDir() const {
    Str companyName = GameSettings::playerSettings->GetProperty("companyName").As<Str>();
    Str productName = GameSettings::playerSettings->GetProperty("productName").As<Str>();
    return fileSystem.GetAppDataDir(companyName, productName);
}

int Common::ProcessPlatformEvent() {
    Platform::Event	ev;

    platform->GenerateMouseDeltaEvent();

    while (1) {
        GetPlatformEvent(&ev);

        if (ev.type == Platform::NoEvent) {
            break;
        }

        switch (ev.type) {
        case Platform::KeyEvent:
            gameClient.KeyEvent((KeyCode::Enum)ev.value, ev.value2 ? true : false);
            break;
        case Platform::CharEvent:
            gameClient.CharEvent((wchar_t)ev.value);
            break;
        case Platform::CompositionEvent:
            gameClient.CompositionEvent((int)ev.value);
            break;
        case Platform::MouseDeltaEvent:
            gameClient.MouseDeltaEvent((int)ev.value, (int)ev.value2, ev.time);
            break;
        case Platform::MouseMoveEvent:
            gameClient.MouseMoveEvent((int)ev.value, (int)ev.value2, ev.time);
            break;
        case Platform::JoyAxisEvent:
            gameClient.JoyAxisEvent((int)ev.value, (int)ev.value2, ev.time);
            break;
        case Platform::TouchBeganEvent:
            gameClient.TouchEvent(InputSystem::Touch::Started, ev.value, LowDWord(ev.value2), HighDWord(ev.value2), ev.time);
            break;
        case Platform::TouchMovedEvent:
            gameClient.TouchEvent(InputSystem::Touch::Moved, ev.value, LowDWord(ev.value2), HighDWord(ev.value2), ev.time);
            break;
        case Platform::TouchEndedEvent:
            gameClient.TouchEvent(InputSystem::Touch::Ended, ev.value, LowDWord(ev.value2), HighDWord(ev.value2), ev.time);
            break;
        case Platform::TouchCanceledEvent:
            gameClient.TouchEvent(InputSystem::Touch::Canceled, ev.value, 0, 0, ev.time);
            break;
        case Platform::ConsoleEvent:
            cmdSystem.BufferCommandText(CmdSystem::Append, (const wchar_t *)ev.ptr);
            cmdSystem.BufferCommandText(CmdSystem::Append, L"\n");
            break;
        case Platform::PacketEvent:
            break;
        default:
            BE_FATALERROR(L"Common::ProcessPlatformEvent: bad event type %i", ev.type);
        }

        if (ev.ptr) {
            Mem_Free(ev.ptr);
        }
    }

    return ev.time;
}

void Common::GetPlatformEvent(Platform::Event *ev) {
    platform->GetEvent(ev);
}

void Common::Cmd_Version(const CmdArgs &args) {
    BE_LOG(L"%hs-%hs %i.%i.%i %hs %hs\n", B_ENGINE_NAME, PlatformProcess::PlatformName(),
        B_ENGINE_VERSION_MAJOR, B_ENGINE_VERSION_MINOR, B_ENGINE_VERSION_PATCH, __DATE__, __TIME__);
}

void Common::Cmd_Error(const CmdArgs &args) {
    if (args.Argv(1)) {
        BE_FATALERROR(L"%ls", args.Argv(1));
    }
}

void Common::Cmd_Quit(const CmdArgs &args) {
    gameClient.Shutdown();
    common.Shutdown();
    exit(0);
}

BE_NAMESPACE_END
