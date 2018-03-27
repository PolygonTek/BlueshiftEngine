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

#include "PhysicsConstraint.h"

BE_NAMESPACE_BEGIN

/// Point-to-point constraint, also known as ball socket joint
class PhysP2PConstraint : public PhysConstraint {
public:
    PhysP2PConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA);
    PhysP2PConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, PhysRigidBody *bodyB, const Vec3 &anchorInB);

    const Vec3          GetAnchorA() const;
    void                SetAnchorA(const Vec3 &anchorInA);

    const Vec3          GetAnchorB() const;
    void                SetAnchorB(const Vec3 &anchorInB);

    float               GetImpulseClamp() const;
    void                SetImpulseClamp(float clamp);
};

BE_NAMESPACE_END
