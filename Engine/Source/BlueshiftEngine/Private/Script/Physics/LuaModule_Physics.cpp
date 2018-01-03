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
#include "Components/ComRigidBody.h"
#include "Components/ComSensor.h"
#include "Physics/PhysicsWorld.h"
#include "Game/GameWorld.h"
#include "Game/CastResult.h"

BE_NAMESPACE_BEGIN

struct FilterGroup {};

void LuaVM::RegisterPhysics(LuaCpp::Module &module) {
    LuaCpp::Selector _Physics = module["Physics"];

    PhysicsWorld *physicsWorld = gameWorld->GetPhysicsWorld();

    LuaCpp::Selector _Physics_FilterGroup = _Physics["FilterGroup"];
    _Physics_FilterGroup.SetClass<FilterGroup>();
    _Physics_FilterGroup["DefaultGroup"] = PhysCollidable::CollisionFilterGroup::DefaultGroup;
    _Physics_FilterGroup["StaticGroup"] = PhysCollidable::CollisionFilterGroup::StaticGroup;
    _Physics_FilterGroup["KinematicGroup"] = PhysCollidable::CollisionFilterGroup::KinematicGroup;
    _Physics_FilterGroup["SensorGroup"] = PhysCollidable::CollisionFilterGroup::SensorGroup;
    _Physics_FilterGroup["CharacterGroup"] = PhysCollidable::CollisionFilterGroup::CharacterGroup;
    _Physics_FilterGroup["AllGroup"] = PhysCollidable::CollisionFilterGroup::AllGroup;

    LuaCpp::Selector _Physics_CastResultParent = _Physics["CastResultParent"];
    _Physics_CastResultParent.SetClass<CastResult>();
    _Physics_CastResultParent.AddClassMembers<CastResult>(
        "point", &CastResult::point,
        "normal", &CastResult::normal,
        "end_pos", &CastResult::endpos,
        "fraction", &CastResult::fraction,
        "surface_flags", &CastResult::surfaceFlags);
    
    LuaCpp::Selector _Physics_CastResult = _Physics["CastResult"];
    _Physics_CastResult.SetClass<CastResultEx>(_Physics_CastResultParent);
    _Physics_CastResult.AddClassMembers<CastResultEx>(
        "rigid_body", &CastResultEx::GetRigidBody,
        "sensor", &CastResultEx::GetSensor);

    _Physics["gravity"].SetFunc([physicsWorld]() { return physicsWorld->GetGravity(); });
    _Physics["set_gravity"].SetFunc([physicsWorld](const Vec3 &gravity) { return physicsWorld->SetGravity(gravity); });
    _Physics["ray_cast"].SetFunc([physicsWorld](const Vec3 &start, const Vec3 &end, int filterGroup, int filterMask, CastResultEx &castResult) {
        return physicsWorld->RayCast(nullptr, start, end, filterGroup, filterMask, castResult);
    });
}

BE_NAMESPACE_END
