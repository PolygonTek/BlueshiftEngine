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

// TODO: stiffness, damping 값으로는 시뮬레이션이 불안정할 수 있음. soft constraint 개념을 도입해야함
class PhysGenericSpringConstraint : public PhysGenericConstraint {
public:
    PhysGenericSpringConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA);
    PhysGenericSpringConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB);

    const Vec3          GetAngularStiffness() const { return angularStiffness; }
    void                SetAngularStiffness(const Vec3 &stiffness);

    const Vec3          GetAngularDamping() const { return angularDamping; }
    void                SetAngularDamping(const Vec3 &damping);

    const Vec3          GetLinearStiffness() const { return linearStiffness; }
    void                SetLinearStiffness(const Vec3 &stiffness);
    
    const Vec3          GetLinearDamping() const { return linearDamping; }
    void                SetLinearDamping(const Vec3 &damping);

protected:
    Vec3                angularStiffness;
    Vec3                angularDamping;
    Vec3                linearStiffness;
    Vec3                linearDamping;
};

BE_NAMESPACE_END
