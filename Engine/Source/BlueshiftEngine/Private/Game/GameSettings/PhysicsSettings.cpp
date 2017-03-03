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
END_PROPERTIES

void PhysicsSettings::Init() {
    GameSettings::Init();

    Vec3 gravity = props->Get("gravity").As<Vec3>();
    GetGameWorld()->GetPhysicsWorld()->SetGravity(Vec3(MeterToUnit(gravity.x), MeterToUnit(gravity.y), MeterToUnit(gravity.z)));
}

void PhysicsSettings::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
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