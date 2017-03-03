#include "Precompiled.h"
#include "Components/ComRigidBody.h"
#include "Components/ComSensor.h"
#include "Game/CastResult.h"

BE_NAMESPACE_BEGIN

ComRigidBody *CastResultEx::GetRigidBody() {
    if (!hitObject || !hitObject->GetUserPointer()) {
        return nullptr;
    }
    return (reinterpret_cast<Component *>(hitObject->GetUserPointer()))->Cast<ComRigidBody>();
}

ComSensor *CastResultEx::GetSensor() {
    if (!hitObject || !hitObject->GetUserPointer()) {
        return nullptr;
    }
    return (reinterpret_cast<Component *>(hitObject->GetUserPointer()))->Cast<ComSensor>();
}

BE_NAMESPACE_END
