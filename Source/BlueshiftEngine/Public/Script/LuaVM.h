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

#include "Core/CmdArgs.h"
#include "Containers/Array.h"
#include "LuaCpp/LuaCpp.h"

BE_NAMESPACE_BEGIN

class GameWorld;

using EngineModuleCallback = void (*)(LuaCpp::Module &);

class LuaVM {
public:
    LuaVM();

    void                    Init();
    void                    Shutdown();

    LuaCpp::State &         State() { return *state; }

    const char *            GetLuaVersion() const;
    const char *            GetLuaJitVersion() const;
    int                     GetLuaMemory() const { return state->GetGCKb() * 1024; }

    void                    EnableJIT(bool enabled);

    void                    StartDebuggee();
    void                    StopDebuggee();
    void                    PollDebuggee();

    void                    RegisterEngineModuleCallback(EngineModuleCallback callback);

    void                    InitEngineModule(const GameWorld *gameWorld);

private:
    void                    RegisterMath(LuaCpp::Module &module);
    void                    RegisterComplex(LuaCpp::Module &module);
    void                    RegisterVec2(LuaCpp::Module &module);
    void                    RegisterVec3(LuaCpp::Module &module);
    void                    RegisterVec4(LuaCpp::Module &module);
    void                    RegisterColor3(LuaCpp::Module &module);
    void                    RegisterColor4(LuaCpp::Module &module);
    void                    RegisterMat2(LuaCpp::Module &module);
    void                    RegisterMat3(LuaCpp::Module &module);
    void                    RegisterMat3x4(LuaCpp::Module &module);
    void                    RegisterMat4(LuaCpp::Module &module);
    void                    RegisterQuaternion(LuaCpp::Module &module);
    void                    RegisterAngles(LuaCpp::Module &module);
    void                    RegisterRotation(LuaCpp::Module &module);
    void                    RegisterPlane(LuaCpp::Module &module);
    void                    RegisterSphere(LuaCpp::Module &module);
    void                    RegisterCylinder(LuaCpp::Module &module);
    void                    RegisterAABB(LuaCpp::Module &module);
    void                    RegisterOBB(LuaCpp::Module &module);
    void                    RegisterFrustum(LuaCpp::Module &module);
    void                    RegisterRay(LuaCpp::Module &module);
    void                    RegisterPoint(LuaCpp::Module &module);
    void                    RegisterRect(LuaCpp::Module &module);

    void                    RegisterCommon(LuaCpp::Module &module);

    void                    RegisterInput(LuaCpp::Module &module);

    void                    RegisterScreen(LuaCpp::Module &module);

    void                    RegisterPhysics(LuaCpp::Module &module);

    void                    RegisterStr(LuaCpp::Module &module);

    void                    RegisterFile(LuaCpp::Module &module);
    void                    RegisterFileSystem(LuaCpp::Module &module);

    void                    RegisterObject(LuaCpp::Module &module);

    void                    RegisterAsset(LuaCpp::Module &module);
    void                    RegisterTextureAsset(LuaCpp::Module &module);
    void                    RegisterShaderAsset(LuaCpp::Module &module);
    void                    RegisterMaterialAsset(LuaCpp::Module &module);
    void                    RegisterSkeletonAsset(LuaCpp::Module &module);
    void                    RegisterMeshAsset(LuaCpp::Module &module);
    void                    RegisterAnimAsset(LuaCpp::Module &module);
    void                    RegisterAnimControllerAsset(LuaCpp::Module &module);
    void                    RegisterSoundAsset(LuaCpp::Module &module);
    void                    RegisterMapAsset(LuaCpp::Module &module);
    void                    RegisterPrefabAsset(LuaCpp::Module &module);

    void                    RegisterComponent(LuaCpp::Module &module);
    void                    RegisterTransformComponent(LuaCpp::Module &module);
    void                    RegisterColliderComponent(LuaCpp::Module &module);
    void                    RegisterBoxColliderComponent(LuaCpp::Module &module);
    void                    RegisterSphereColliderComponent(LuaCpp::Module &module);
    void                    RegisterCylinderColliderComponent(LuaCpp::Module &module);
    void                    RegisterCapsuleColliderComponent(LuaCpp::Module &module);
    void                    RegisterMeshColliderComponent(LuaCpp::Module &module);
    void                    RegisterRigidBodyComponent(LuaCpp::Module &module);
    void                    RegisterSensorComponent(LuaCpp::Module &module);
    void                    RegisterJointComponent(LuaCpp::Module &module);
    void                    RegisterFixedJointComponent(LuaCpp::Module &module);
    void                    RegisterHingeJointComponent(LuaCpp::Module &module);
    void                    RegisterSocketJointComponent(LuaCpp::Module &module);
    void                    RegisterSpringJointComponent(LuaCpp::Module &module);
    void                    RegisterCharacterJointComponent(LuaCpp::Module &module);
    void                    RegisterConstantForceComponent(LuaCpp::Module &module);
    void                    RegisterCharacterControllerComponent(LuaCpp::Module &module);
    void                    RegisterRenderableComponent(LuaCpp::Module &module);
    void                    RegisterMeshRendererComponent(LuaCpp::Module &module);
    void                    RegisterStaticMeshRendererComponent(LuaCpp::Module &module);
    void                    RegisterSkinnedMeshRendererComponent(LuaCpp::Module &module);
    void                    RegisterAnimatorComponent(LuaCpp::Module &module);
    void                    RegisterTextRendererComponent(LuaCpp::Module &module);
    void                    RegisterParticleSystemComponent(LuaCpp::Module &module);
    void                    RegisterCameraComponent(LuaCpp::Module &module);
    void                    RegisterLightComponent(LuaCpp::Module &module);
    void                    RegisterAudioListenerComponent(LuaCpp::Module &module);
    void                    RegisterAudioSourceComponent(LuaCpp::Module &module);
    void                    RegisterSplineComponent(LuaCpp::Module &module);
    void                    RegisterScriptComponent(LuaCpp::Module &module);

    void                    RegisterEntity(LuaCpp::Module &module);
    void                    RegisterGameWorld(LuaCpp::Module &module);

    LuaCpp::State *         state;
    LuaCpp::Selector        startDebuggee;
    LuaCpp::Selector        stopDebuggee;
    LuaCpp::Selector        pollDebuggee;
    bool                    debuggeeStarted = false;

    Array<EngineModuleCallback> engineModuleCallbacks;

    const GameWorld *       gameWorld;
};

BE_INLINE LuaVM::LuaVM() {
    state = nullptr;
    gameWorld = nullptr;
}

BE_NAMESPACE_END
