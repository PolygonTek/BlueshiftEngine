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

#include "LuaCpp/LuaCpp.h"

BE_NAMESPACE_BEGIN

class GameWorld;

class LuaVM {
public:
    static void             Init();
    static void             Shutdown();

    static void             InitEngineModule(const GameWorld *gameWorld);

    static LuaCpp::State &  State() { return *state; }

    static void             EnableDebug();

private:

    static void             RegisterMath(LuaCpp::Module &module);
    static void             RegisterComplex(LuaCpp::Module &module);
    static void             RegisterVec2(LuaCpp::Module &module);
    static void             RegisterVec3(LuaCpp::Module &module);
    static void             RegisterVec4(LuaCpp::Module &module);
    static void             RegisterColor3(LuaCpp::Module &module);
    static void             RegisterColor4(LuaCpp::Module &module);
    static void             RegisterMat2(LuaCpp::Module &module);
    static void             RegisterMat3(LuaCpp::Module &module);
    static void             RegisterMat4(LuaCpp::Module &module);
    static void             RegisterQuaternion(LuaCpp::Module &module);
    static void             RegisterAngles(LuaCpp::Module &module);
    static void             RegisterRotation(LuaCpp::Module &module);
    static void             RegisterPlane(LuaCpp::Module &module);
    static void             RegisterSphere(LuaCpp::Module &module);
    static void             RegisterCylinder(LuaCpp::Module &module);
    static void             RegisterAABB(LuaCpp::Module &module);
    static void             RegisterOBB(LuaCpp::Module &module);
    static void             RegisterFrustum(LuaCpp::Module &module);
    static void             RegisterRay(LuaCpp::Module &module);
    static void             RegisterPoint(LuaCpp::Module &module);
    static void             RegisterRect(LuaCpp::Module &module);

    static void             RegisterCommon(LuaCpp::Module &module);

    static void             RegisterInput(LuaCpp::Module &module);

    static void             RegisterScreen(LuaCpp::Module &module);

    static void             RegisterPhysics(LuaCpp::Module &module);

    static void             RegisterStr(LuaCpp::Module &module);

    static void             RegisterObject(LuaCpp::Module &module);

    static void             RegisterAsset(LuaCpp::Module &module);
    static void             RegisterTextureAsset(LuaCpp::Module &module);
    static void             RegisterShaderAsset(LuaCpp::Module &module);
    static void             RegisterMaterialAsset(LuaCpp::Module &module);
    static void             RegisterSkeletonAsset(LuaCpp::Module &module);
    static void             RegisterMeshAsset(LuaCpp::Module &module);
    static void             RegisterAnimAsset(LuaCpp::Module &module);
    static void             RegisterAnimControllerAsset(LuaCpp::Module &module);
    static void             RegisterSoundAsset(LuaCpp::Module &module);
    static void             RegisterMapAsset(LuaCpp::Module &module);
    static void             RegisterPrefabAsset(LuaCpp::Module &module);

    static void             RegisterComponent(LuaCpp::Module &module);
    static void             RegisterTransformComponent(LuaCpp::Module &module);
    static void             RegisterColliderComponent(LuaCpp::Module &module);
    static void             RegisterBoxColliderComponent(LuaCpp::Module &module);
    static void             RegisterSphereColliderComponent(LuaCpp::Module &module);
    static void             RegisterCylinderColliderComponent(LuaCpp::Module &module);
    static void             RegisterCapsuleColliderComponent(LuaCpp::Module &module);
    static void             RegisterMeshColliderComponent(LuaCpp::Module &module);
    static void             RegisterRigidBodyComponent(LuaCpp::Module &module);
    static void             RegisterSensorComponent(LuaCpp::Module &module);
    static void             RegisterJointComponent(LuaCpp::Module &module);
    static void             RegisterFixedJointComponent(LuaCpp::Module &module);
    static void             RegisterHingeJointComponent(LuaCpp::Module &module);
    static void             RegisterSocketJointComponent(LuaCpp::Module &module);
    static void             RegisterSpringJointComponent(LuaCpp::Module &module);
    static void             RegisterCharacterJointComponent(LuaCpp::Module &module);
    static void             RegisterConstantForceComponent(LuaCpp::Module &module);
    static void             RegisterCharacterControllerComponent(LuaCpp::Module &module);
    static void             RegisterRenderableComponent(LuaCpp::Module &module);
    static void             RegisterMeshRendererComponent(LuaCpp::Module &module);
    static void             RegisterStaticMeshRendererComponent(LuaCpp::Module &module);
    static void             RegisterSkinnedMeshRendererComponent(LuaCpp::Module &module);
    static void             RegisterTextRendererComponent(LuaCpp::Module &module);
    static void             RegisterParticleSystemComponent(LuaCpp::Module &module);
    static void             RegisterCameraComponent(LuaCpp::Module &module);
    static void             RegisterLightComponent(LuaCpp::Module &module);
    static void             RegisterAudioListenerComponent(LuaCpp::Module &module);
    static void             RegisterAudioSourceComponent(LuaCpp::Module &module);
    static void             RegisterSplineComponent(LuaCpp::Module &module);
    static void             RegisterScriptComponent(LuaCpp::Module &module);

    static void             RegisterEntity(LuaCpp::Module &module);
    static void             RegisterGameWorld(LuaCpp::Module &module);

    static LuaCpp::State *  state;

    static const GameWorld *gameWorld;
};

BE_NAMESPACE_END
