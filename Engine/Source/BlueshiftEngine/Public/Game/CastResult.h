#pragma once

#include "Physics/Physics.h"

BE_NAMESPACE_BEGIN

class ComRigidBody;
class ComSensor;

struct CastResultEx : public CastResult {
    ComRigidBody *          GetRigidBody();
    ComSensor *             GetSensor();
};

BE_NAMESPACE_END
