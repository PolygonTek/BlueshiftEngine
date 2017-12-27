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
#include "Script/LuaVM.h"
#include "Game/GameWorld.h"
#include "File/FileSystem.h"
#include "File/File.h"
#include "Core/CVars.h"
#include "Core/Cmds.h"

extern int luaopen_file(lua_State *L);

BE_NAMESPACE_BEGIN

LuaCpp::State *     LuaVM::state = nullptr;
const GameWorld *   LuaVM::gameWorld = nullptr;

void LuaVM::Init() {
    cmdSystem.AddCommand(L"lua_version", Cmd_LuaVersion);
    cmdSystem.AddCommand(L"lua_mem", Cmd_LuaMemory);

    state = new LuaCpp::State(true);

    BE_LOG(L"Lua version %.1f\n", state->Version());

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

    state->RegisterSearcher([](const std::string &name) {
        Str filename = name.c_str();
        filename.DefaultFileExtension(".lua");

        char *data;
        size_t size = fileSystem.LoadFile(filename.c_str(), true, (void **)&data);
        if (!data) {
            return false;
        }

        state->RunBuffer(filename.c_str(), data, size);

        fileSystem.FreeFile(data);
        return true;
    });
   state->Require("blueshift.io", luaopen_file);
#if defined __IOS__ || defined __ANDROID__
    EnableDebug();
#endif
}

extern "C" int luaopen_socket_core(lua_State *L);
static CVar lua_server(L"lua_server", L"127.0.0.1", CVar::Archive, L"lua server for debugging");

void LuaVM::EnableDebug() {
    char *server = tombs(lua_server.GetString());
    if (server[0] == 0)
        return;
    
    File *f = fileSystem.OpenFileRead("Scripts/debug/debug.lua", true);
    if (!f)
        return;
    fileSystem.CloseFile(f);
    state->Require("socket.core", luaopen_socket_core);
    char *cmd = va("assert(load(_G['blueshift.io'].open('Scripts/debug/debug.lua', 'rb'):read('*a'), '@Scripts/debug/debug.lua'))('%s')", server);
    (*state)(cmd);
}

void LuaVM::InitEngineModule(const GameWorld *gameWorld) {
    LuaVM::gameWorld = gameWorld;

    state->RegisterModule("blueshift", [](LuaCpp::Module &module) {
        module["log"].SetFunc([](const std::string &msg) {
            BE_LOG(L"%hs\n", msg.c_str());
        });
        module["unit_to_centi"].SetFunc(&UnitToCenti);
        module["unit_to_meter"].SetFunc(&UnitToMeter);
        module["centi_to_unit"].SetFunc(&CentiToUnit);
        module["meter_to_unit"].SetFunc(&MeterToUnit);

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
        RegisterJointComponent(module);
        RegisterFixedJointComponent(module);
        RegisterHingeJointComponent(module);
        RegisterSocketJointComponent(module);
        RegisterSpringJointComponent(module);
        RegisterCharacterJointComponent(module);
        RegisterConstantForceComponent(module);
        RegisterCharacterControllerComponent(module);
        RegisterRenderableComponent(module);
        RegisterMeshRendererComponent(module);
        RegisterStaticMeshRendererComponent(module);
        RegisterSkinnedMeshRendererComponent(module);
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
    });

    state->Require("blueshift");
}

void LuaVM::Shutdown() {
    cmdSystem.RemoveCommand(L"lua_version");
    cmdSystem.RemoveCommand(L"lua_mem");

    SAFE_DELETE(state);
}

void LuaVM::Cmd_LuaVersion(const CmdArgs &args) {
    float version = state->Version();

    BE_LOG(L"%.1f\n", version);
}

void LuaVM::Cmd_LuaMemory(const CmdArgs &args) {
    Str strBytes = Str::FormatBytes(state->GetGCKb() * 1024);

    BE_LOG(L"%hs used by Lua\n", strBytes.c_str());
}

BE_NAMESPACE_END
