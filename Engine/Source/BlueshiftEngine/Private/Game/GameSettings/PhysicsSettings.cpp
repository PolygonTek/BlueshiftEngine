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
#include "Game/GameSettings/PhysicsSettings.h"
#include "Game/GameWorld.h"
#include "Physics/Physics.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Physics Settings", PhysicsSettings, GameSettings)
BEGIN_EVENTS(PhysicsSettings)
END_EVENTS

void PhysicsSettings::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("gravity", "Gravity", Vec3, GetGravity, SetGravity, Vec3(0, 0, -9.8f), "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_ARRAY_PROPERTY("filterMasks", "Filter Mask", int, GetFilterMaskElement, SetFilterMaskElement, GetFilterMaskCount, SetFilterMaskCount, -1, "", PropertyInfo::EditorFlag);
}

PhysicsSettings::PhysicsSettings() {
    numFilterMasks = 0;
}

void PhysicsSettings::Init() {
    GameSettings::Init();
}

Vec3 PhysicsSettings::GetGravity() const {
    Vec3 gravity = GetGameWorld()->GetPhysicsWorld()->GetGravity();
    return Vec3(UnitToMeter(gravity.x), UnitToMeter(gravity.y), UnitToMeter(gravity.z));
}

void PhysicsSettings::SetGravity(const Vec3 &gravity) {
    GetGameWorld()->GetPhysicsWorld()->SetGravity(Vec3(MeterToUnit(gravity.x), MeterToUnit(gravity.y), MeterToUnit(gravity.z)));
}

int PhysicsSettings::GetFilterMaskElement(int index) const {
    return GetGameWorld()->GetPhysicsWorld()->GetCollisionFilterMask(index);
}

void PhysicsSettings::SetFilterMaskElement(int index, int mask) {
    GetGameWorld()->GetPhysicsWorld()->SetCollisionFilterMask(index, mask);
}

int PhysicsSettings::GetFilterMaskCount() const {
    return numFilterMasks;
}

void PhysicsSettings::SetFilterMaskCount(int count) {
    int oldCount = numFilterMasks;

    numFilterMasks = count;

    if (count > oldCount) {
        for (int index = oldCount; index < count; index++) {
            GetGameWorld()->GetPhysicsWorld()->SetCollisionFilterMask(index, -1);
        }
    }
}

BE_NAMESPACE_END