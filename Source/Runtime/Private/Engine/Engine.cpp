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
#include "BlueshiftEngine.h"
#include "Platform/PlatformSystem.h"
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

TaskManager *       Engine::taskManager = nullptr;
CmdArgs             Engine::args;
Str                 Engine::baseDir;
Str                 Engine::searchPath;
thread_local bool   Engine::isMainThread = false;

static streamOutFunc_t logFuncPtr = nullptr;
static streamOutFunc_t errFuncPtr = nullptr;

static CVAR(forceGenericSIMD, "0", CVar::Flag::Bool, "Disable SIMD-optimized code path, but individual SIMD-optimized code will be applied");
static CVAR(forceGenericSIMDForDebug, "1", CVar::Flag::Bool, "");

void Engine::InitBase(uint32_t maxTaskCount, const char *baseDir, const streamOutFunc_t logFunc, const streamOutFunc_t errFunc) {
    // Set user-default ANSI code page obtained from the operating system
    setlocale(LC_ALL, "");

    logFuncPtr = logFunc;
    errFuncPtr = errFunc;

#if defined(__WIN32__) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);

    // NOTE: can be replaced by setting '({,,ucrtbased.dll}_crtBreakAlloc)' (msvc2015) in debug watch window
    //_CrtSetBreakAlloc(286008);
#endif

    ByteOrder::Init();

    cmdSystem.Init();

    cvarSystem.Init();

    fileSystem.Init(baseDir);

    DetectCpu();

    bool genericSIMD = forceGenericSIMD.GetBool();
#ifdef _DEBUG
    if (!genericSIMD) {
        genericSIMD = forceGenericSIMDForDebug.GetBool();
    }
#endif

    SIMD::Init(genericSIMD);

    PlatformTime::Init();

    Math::Init();

#ifdef USE_TASK_MANAGER
    // Create the task manager with the given max task count.
    taskManager = new TaskManager(maxTaskCount);

    // Start as many task threads as there are physical CPU cores.
    int numCores = PlatformSystem::NumCPUCores();
    taskManager->Start(numCores);

    BE_LOG("Task threads (%i) started\n", taskManager->NumThreads());
#endif
}

void Engine::ShutdownBase() {
#ifdef USE_TASK_MANAGER
    // Stop all tasks.
    if (taskManager) {
        taskManager->Stop();
        delete taskManager;
        taskManager = nullptr;
    }
#endif

    PlatformTime::Shutdown();
    
    SIMD::Shutdown();

    fileSystem.Shutdown();

    cvarSystem.Shutdown();

    cmdSystem.Shutdown();

#if defined(__WIN32__) && defined(_DEBUG)
    _ASSERT(_CrtCheckMemory());
#endif
}

void Engine::Init(const InitParms *initParms) {
    Engine::baseDir = initParms->baseDir;
    Engine::searchPath = initParms->searchPath;
    Engine::args = initParms->args;

    Platform::Init();

    common.Init(initParms->maxTasks, Engine::baseDir);

    if (!Engine::searchPath.IsEmpty()) {
        fileSystem.SetSearchPath(Engine::searchPath);
    }
}

void Engine::Shutdown() {
    common.Shutdown();

    Platform::Shutdown();
}

void Engine::RunFrame(int elapsedMsec) {
    BE_PROFILE_SYNC_FRAME();

    common.RunFrame(elapsedMsec);
}

void Log(int logLevel, const char *fmt, ...) {
    char buffer[16384];
    va_list args;

    va_start(args, fmt);
    Str::vsnPrintf(buffer, COUNT_OF(buffer), fmt, args);
    va_end(args);

    (*logFuncPtr)(logLevel, buffer);
}

void Error(int errLevel, const char *fmt, ...) {
    char buffer[16384];
    va_list args;

    va_start(args, fmt);
    Str::vsnPrintf(buffer, COUNT_OF(buffer), fmt, args);
    va_end(args);

    (*errFuncPtr)(errLevel, buffer);
}

void Assert(bool expr) {
    if (!expr) {
        BE_ERRLOG("Assert Failed\n");
        assert(0);
    }
}

BE_NAMESPACE_END
