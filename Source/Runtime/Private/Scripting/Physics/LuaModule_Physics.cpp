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

    LuaCpp::Selector _Physics_CastResult = _Physics["CastResult"];
    _Physics_CastResult.SetClass<CastResult>();
    _Physics_CastResult.AddClassMembers<CastResult>(
        "point", &CastResult::point,
        "normal", &CastResult::normal,
        "end_pos", &CastResult::endPos,
        "fraction", &CastResult::fraction,
        "surface_flags", &CastResult::surfaceFlags);

    _Physics["gravity"].SetFunc([physicsWorld]() { return physicsWorld->GetGravity(); });
    _Physics["set_gravity"].SetFunc([physicsWorld](const Vec3 &gravity) { return physicsWorld->SetGravity(gravity); });
    _Physics["ray_cast"].SetFunc([physicsWorld](const Vec3 &start, const Vec3 &end, int mask, CastResult &castResult) {
        return physicsWorld->RayCast(start, end, mask, castResult);
    });
    _Physics["box_cast"].SetFunc([physicsWorld](const Vec3 &boxCenter, const Vec3 &boxExtents, const Mat3 &axis, const Vec3 &end, int mask, CastResult &castResult) {
        return physicsWorld->BoxCast(boxCenter, boxExtents, axis, end, mask, castResult);
    });
    _Physics["sphere_cast"].SetFunc([physicsWorld](const Vec3 &sphereCenter, float sphereRadius, const Vec3 &end, int mask, CastResult &castResult) {
        return physicsWorld->SphereCast(sphereCenter, sphereRadius, end, mask, castResult);
    });
}

BE_NAMESPACE_END
