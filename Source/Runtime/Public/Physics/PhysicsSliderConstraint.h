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

/// Slider constraint
class PhysSliderConstraint : public PhysConstraint {
public:
    PhysSliderConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA);
    PhysSliderConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB);

    void                SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA);
    void                SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB);

                        /// Gets minimum translation offset in slider axis.
    float               GetLinearLowerLimit() const { return linearLowerLimit; }
                        /// Gets maximum translation offset in slider axis.
    float               GetLinearUpperLimit() const { return linearUpperLimit; }
                        /// Sets translation offset limit.
    void                SetLinearLimits(float lowerLimit, float upperLimit);
                        /// Enables translation limits.
    void                EnableLinearLimits(bool enable);

                        /// Gets minimum rotation angle in radian.
    float               GetAngularLowerLimit() const { return angularLowerLimit; }
                        /// Gets maximum rotation angle in radian.
    float               GetAngularUpperLimit() const { return angularUpperLimit; }
                        /// Sets rotation limit angles in radian.
    void                SetAngularLimits(float lowerLimit, float upperLimit);
                        /// Enables rotation limits.
    void                EnableAngularLimits(bool enable);

                        /// Sets motor target linear velocity and maximum motor impulse.
    void                SetLinearMotor(float motorTargetVelocity, float maxMotorForce);
                        /// Enables linear motor.
    void                EnableLinearMotor(bool enable);

                        /// Sets motor target angular velocity in radian and maximum motor torque.
    void                SetAngularMotor(float motorTargetVelocity, float maxMotorTorque);
                        /// Enables angular motor.
    void                EnableAngularMotor(bool enable);

private:
    float               linearLowerLimit;
    float               linearUpperLimit;
    float               angularLowerLimit;
    float               angularUpperLimit;
};

BE_NAMESPACE_END
