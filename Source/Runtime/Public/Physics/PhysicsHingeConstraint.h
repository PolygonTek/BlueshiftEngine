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

/// Hinge constraint, also known as revolute joint
class PhysHingeConstraint : public PhysConstraint {
public:
    PhysHingeConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA);
    PhysHingeConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB);

    void                SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA);
    void                SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB);

                        /// Gets minimum rotation angle in radian.
    float               GetAngularLowerLimit() const { return lowerLimit; }
                        /// Gets maximum rotation angle in radian.
    float               GetAngularUpperLimit() const { return upperLimit; }
                        /// Sets rotation limit angles in radian.
    void                SetAngularLimits(float lowerLimit, float upperLimit);
                        /// Enables rotation limits.
    void                EnableAngularLimits(bool enable);

                        /// Sets motor target angular velocity in radian and maximum motor torque.
    void                SetMotor(float motorTargetVelocity, float maxMotorTorque);
                        /// Enables motors.
    void                EnableMotor(bool enable);

private:
    float               lowerLimit;
    float               upperLimit;
};

BE_NAMESPACE_END
