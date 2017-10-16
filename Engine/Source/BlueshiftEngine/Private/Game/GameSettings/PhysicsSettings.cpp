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
#include "Physics/Physics.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"
#include "Game/GameSettings/PhysicsSettings.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Physics Settings", PhysicsSettings, GameSettings)
BEGIN_EVENTS(PhysicsSettings)
END_EVENTS
BEGIN_PROPERTIES(PhysicsSettings)
    PROPERTY_VEC3("gravity", "Gravity", "gravity", "0 0 -9.8", PropertySpec::ReadWrite),
    PROPERTY_INT("filterMasks", "Filter Mask", "", "-1", PropertySpec::ReadWrite | PropertySpec::IsArray),
END_PROPERTIES

void PhysicsSettings::Init() {
    GameSettings::Init();

    Vec3 gravity = props->Get("gravity").As<Vec3>();
    GetGameWorld()->GetPhysicsWorld()->SetGravity(Vec3(MeterToUnit(gravity.x), MeterToUnit(gravity.y), MeterToUnit(gravity.z)));

    int numFilterMasks = props->NumElements("filterMasks");
    for (int filterIndex = 0; filterIndex < numFilterMasks; filterIndex++) {
        Str propName = va("filterMasks[%i]", filterIndex);

        int filterMask = props->Get(propName).As<int>();
        GetGameWorld()->GetPhysicsWorld()->SetCollisionFilterMask(filterIndex, filterMask);
    }
}

void PhysicsSettings::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitialized()) {
        return;
    }

    if (!Str::Cmp(propName, "gravity")) {
        Vec3 gravity = props->Get("gravity").As<Vec3>();
        GetGameWorld()->GetPhysicsWorld()->SetGravity(Vec3(MeterToUnit(gravity.x), MeterToUnit(gravity.y), MeterToUnit(gravity.z)));
        return;
    }

    GameSettings::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END