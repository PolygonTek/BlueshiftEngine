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
#include "Scripting/LuaVM.h"
#include "Game/GameWorld.h"
#include "File/FileSystem.h"
#include "File/File.h"
#include "Core/CVars.h"
#include "Core/Cmds.h"

extern "C" {
#include "luasocket/luasocket.h"
}

BE_NAMESPACE_BEGIN

// VS Code Lua Debugger by devCAT: https://marketplace.visualstudio.com/items?itemName=devCAT.lua-debug
// Set lua_debuggeeController to "vscode_debuggee_controller"

// MobDebug in ZeroBrane Studio https://github.com/pkulchenko/MobDebug
// Set lua_debuggeeController to "mobdebug_controller"

static CVAR(lua_debug, L"0", CVar::Bool | CVar::Archive, L"Enable Lua debugging");
static CVAR(lua_debuggerServer, L"localhost", CVar::Archive, L"Lua debugger server address for remote debugging");
static CVAR(lua_debuggeeController, L"mobdebug_controller", 0, L"Lua debuggee controller script name");

static int engine_print(lua_State *L) {
    int nargs = lua_gettop(L);
    for (int i = 1; i <= nargs; ++i) {
        const char *s = lua_tostring(L, i);
        BE_LOG(L"%hs", s);
    }
    BE_LOG(L"\n");
    return 0;
}

static const struct luaL_Reg printlib[] = {
    { "print", engine_print },
    { nullptr, nullptr }
};

void LuaVM::Init() {
    if (state) {
        Shutdown();
    }

    state = new LuaCpp::State(true);

    //BE_LOG(L"Lua version %.1f\n", state->Version());

    // Redirect global print function
    state->RegisterLib(printlib, nullptr);

    state->HandleExceptionsWith([](int status, std::string msg, std::exception_ptr exception) {
        const char *statusStr = "";
        switch (status) {
        case LUA_ERRRUN:
            statusStr = "LUA_ERRRUN";
            break;
        case LUA_ERRSYNTAX:
            statusStr = "LUA_ERRSYNTAX";
            break;
        case LUA_ERRMEM:
            statusStr = "LUA_ERRMEM";
            break;
#if LUA_VERSION_NUM >= 502
        case LUA_ERRGCMM:
            statusStr = "LUA_ERRGCMM";
            break;
#endif
        case LUA_ERRERR:
            statusStr = "LUA_ERRERR";
            break;
        }
        BE_ERRLOG(L"%hs - %hs\n", statusStr, msg.c_str());
    });

    state->RegisterSearcher([this](const std::string &name) {
        Str filename = name.c_str();
        filename.DefaultFileExtension(".lua");

        char *data;
        size_t size = fileSystem.LoadFile(filename, true, (void **)&data);
        if (!data) {
            size = fileSystem.LoadFile("Scripts/" + filename, true, (void **)&data);
            if (!data) {
                return false;
            }
        }

        if (!state->RunBuffer(name, data, size, name.c_str())) {
            BE_ERRLOG(L"Failed to run %hs\n", filename.c_str());
        }

        fileSystem.FreeFile(data);
        return true;
    });

    LoadTween();
    LoadWaitSupport();
}

void LuaVM::LoadWaitSupport() {
    const char *filename = "Scripts/wait_support.lua";
    char *data;

    size_t size = fileSystem.LoadFile(filename, true, (void **)&data);
    if (data) {
        if (state->RunBuffer(filename, data, size)) {
            LuaCpp::Selector waitSupport = (*state)["wait_support"];

            clearWatingThreads = waitSupport["clear_waiting_threads"];
            if (!clearWatingThreads.IsFunction()) {
                clearWatingThreads = LuaCpp::Selector();
            }

            wakeUpWatingThreads = waitSupport["wake_up_waiting_threads"];
            if (!wakeUpWatingThreads.IsFunction()) {
                wakeUpWatingThreads = LuaCpp::Selector();
            }
        }
    }
}

void LuaVM::LoadTween() {
    const char *filename = "Scripts/tween.lua";
    char *data;

    size_t size = fileSystem.LoadFile(filename, true, (void **)&data);
    if (data) {
        if (state->RunBuffer(filename, data, size)) {
            LuaCpp::Selector tween = (*state)["tween"];

            clearTweeners = tween["clear_tweeners"];
            if (!clearTweeners.IsFunction()) {
                clearTweeners = LuaCpp::Selector();
            }

            updateTweeners = tween["update_tweeners"];
            if (!updateTweeners.IsFunction()) {
                updateTweeners = LuaCpp::Selector();
            }
        }
    }
}

void LuaVM::InitEngineModule(const GameWorld *gameWorld) {
    LuaVM::gameWorld = gameWorld;

    state->RegisterModule("blueshift", [this](LuaCpp::Module &module) {
        module["log"].SetFunc([](const std::string &msg) {
            BE_LOG(L"%hs\n", msg.c_str());
        });
        module["unit_to_centi"].SetFunc(static_cast<float(*)(float)>(&UnitToCenti));
        module["unit_to_meter"].SetFunc(static_cast<float(*)(float)>(&UnitToMeter));
        module["centi_to_unit"].SetFunc(static_cast<float(*)(float)>(&CentiToUnit));
        module["meter_to_unit"].SetFunc(static_cast<float(*)(float)>(&MeterToUnit));

        // Math
        RegisterMath(module);
        RegisterComplex(module);
        RegisterVec2(module);
        RegisterVec3(module);
        RegisterVec4(module);
        RegisterColor3(module);
        RegisterColor4(module);
        RegisterMat2(module);
        RegisterMat3(module);
        RegisterMat3x4(module);
        RegisterMat4(module);
        RegisterQuaternion(module);
        RegisterAngles(module);
        RegisterRotation(module);
        RegisterPlane(module);
        RegisterSphere(module);
        RegisterCylinder(module);
        RegisterAABB(module);
        RegisterOBB(module);
        RegisterFrustum(module);
        RegisterRay(module);
        RegisterPoint(module);
        RegisterRect(module);
        // Common
        RegisterCommon(module);
        // Input
        RegisterInput(module);
        // Screen
        RegisterScreen(module);
        // Physics
        RegisterPhysics(module);
        // Str
        RegisterStr(module);
        // Guid
        RegisterGuid(module);
        // File
        RegisterFile(module);
        RegisterFileSystem(module);
        // Object
        RegisterObject(module);
        // Asset
        RegisterAsset(module);
        RegisterTextureAsset(module);
        RegisterShaderAsset(module);
        RegisterMaterialAsset(module);
        RegisterSkeletonAsset(module);
        RegisterMeshAsset(module);
        RegisterAnimAsset(module);
        RegisterAnimControllerAsset(module);
        RegisterSoundAsset(module);
        RegisterMapAsset(module);
        RegisterPrefabAsset(module);
        // Component
        RegisterComponent(module);
        RegisterTransformComponent(module);
        RegisterColliderComponent(module);
        RegisterBoxColliderComponent(module);
        RegisterSphereColliderComponent(module);
        RegisterCylinderColliderComponent(module);
        RegisterCapsuleColliderComponent(module);
        RegisterMeshColliderComponent(module);
        RegisterRigidBodyComponent(module);
        RegisterSensorComponent(module);
        RegisterVehicleWheelComponent(module);
        RegisterJointComponent(module);
        RegisterFixedJointComponent(module);
        RegisterHingeJointComponent(module);
        RegisterSocketJointComponent(module);
        RegisterSliderJointComponent(module);
        RegisterSpringJointComponent(module);
        RegisterWheelJointComponent(module);
        RegisterCharacterJointComponent(module);
        RegisterConstantForceComponent(module);
        RegisterCharacterControllerComponent(module);
        RegisterRenderableComponent(module);
        RegisterMeshRendererComponent(module);
        RegisterStaticMeshRendererComponent(module);
        RegisterSkinnedMeshRendererComponent(module);
        RegisterAnimationComponent(module);
        RegisterAnimatorComponent(module);
        RegisterTextRendererComponent(module);
        RegisterParticleSystemComponent(module);
        RegisterCameraComponent(module);
        RegisterLightComponent(module);
        RegisterAudioListenerComponent(module);
        RegisterAudioSourceComponent(module);
        RegisterSplineComponent(module);
        RegisterScriptComponent(module);
        // Game World
        RegisterEntity(module);
        RegisterGameWorld(module);

        for (int i = 0; i < engineModuleCallbacks.Count(); i++) {
            engineModuleCallbacks[i](module);
        }
    });

    //state->Require("blueshift");
}

void LuaVM::Shutdown() {
    engineModuleCallbacks.Clear();

    clearTweeners = LuaCpp::Selector();
    updateTweeners = LuaCpp::Selector();

    clearWatingThreads = LuaCpp::Selector();
    wakeUpWatingThreads = LuaCpp::Selector();

    startDebuggee = LuaCpp::Selector();
    stopDebuggee = LuaCpp::Selector();
    pollDebuggee = LuaCpp::Selector();
    
    SAFE_DELETE(state);
}

void LuaVM::RegisterEngineModuleCallback(EngineModuleCallback callback) {
    engineModuleCallbacks.Append(callback);
}

const char *LuaVM::GetLuaVersion() const { 
    static char versionString[32] = "";
    int major, minor;
    state->Version(major, minor);
    Str::snPrintf(versionString, sizeof(versionString), "%i.%i", major, minor);
    return versionString;
}

const char *LuaVM::GetLuaJitVersion() const {
    static char versionString[32] = "";
    int major, minor, patch;
    state->JitVersion(major, minor, patch); 
    Str::snPrintf(versionString, sizeof(versionString), "%i.%i.%i", major, minor, patch);
    return versionString;
}

void LuaVM::EnableJIT(bool enabled) {
    state->EnableJIT(enabled);
}

void LuaVM::ClearTweeners() {
    if (clearTweeners.IsFunction()) {
        clearTweeners();
    }
}

void LuaVM::UpdateTweeners(float unscaledDeltaTime, float timeScale) {
    if (updateTweeners.IsFunction()) {
        updateTweeners(unscaledDeltaTime, timeScale);
    }
}

void LuaVM::ClearWatingThreads() {
    if (clearWatingThreads.IsFunction()) {
        clearWatingThreads();
    }
}

void LuaVM::WakeUpWatingThreads(float currentTime) {
    if (wakeUpWatingThreads.IsFunction()) {
        wakeUpWatingThreads(currentTime);
    }
}

void LuaVM::StartDebuggee() {
    if (!lua_debug.GetBool()) {
        return;
    }

    if (!startDebuggee.IsValid()) {
        Str name(lua_debuggeeController.GetString());
        Str filename = "Scripts/" + name;
        filename.DefaultFileExtension(".lua");

        char *data;
        size_t size = fileSystem.LoadFile(filename, true, (void **)&data);
        if (!data) {
            return;
        }

        state->Require("socket.core", luaopen_socket_core);

        if (state->RunBuffer(filename.c_str(), data, size, name)) {
            LuaCpp::Selector sandbox = (*state)[name.c_str()];

            startDebuggee = sandbox["start"];
            if (!startDebuggee.IsFunction()) {
                startDebuggee = LuaCpp::Selector();
            }

            stopDebuggee = sandbox["stop"];
            if (!stopDebuggee.IsFunction()) {
                stopDebuggee = LuaCpp::Selector();
            }

            pollDebuggee = sandbox["poll"];
            if (!pollDebuggee.IsFunction()) {
                pollDebuggee = LuaCpp::Selector();
            }
        }
    }

    Str addr = Str(lua_debuggerServer.GetString());
    startDebuggee(addr.c_str());

    debuggeeStarted = true;
}

void LuaVM::StopDebuggee() {
    if (!debuggeeStarted) {
        return;
    }

    if (stopDebuggee.IsValid()) {
        stopDebuggee();
    }

    debuggeeStarted = false;
}

void LuaVM::PollDebuggee() {
    if (!debuggeeStarted) {
        return;
    }

    if (pollDebuggee.IsValid()) {
        pollDebuggee();
    }
}

BE_NAMESPACE_END
