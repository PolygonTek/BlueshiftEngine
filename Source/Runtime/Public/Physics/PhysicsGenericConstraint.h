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

/// Generic 6 DOF (Degree of Freedom) constraint
class PhysGenericConstraint : public PhysConstraint {
public:
    PhysGenericConstraint(PhysRigidBody *bodyA, PhysRigidBody *bodyB) : PhysConstraint(bodyA, bodyB) {}
    PhysGenericConstraint(PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB);
    PhysGenericConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB);

    void                SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA);
    void                SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB);

                        /// Gets minimum translation offsets for each axis.
    const Vec3          GetLinearLowerLimits() const { return linearLowerLimits; }
                        /// Sets minimum translation offsets for each axis.
    void                SetLinearLowerLimits(const Vec3 &lower);
                        /// Gets maximum translation offsets for each axis.
    const Vec3          GetLinearUpperLimits() const { return linearUpperLimits; }
                        /// Sets maximum translation offsets for each axis.
    void                SetLinearUpperLimits(const Vec3 &upper);

                        /// Enable translation limits
    void                EnableLinearLimits(bool enableX, bool enableY, bool enableZ);

                        /// Gets minimum rotation angles for each axis in radian.
    const Vec3          GetAngularLowerLimits() const { return angularLowerLimits; }
                        /// Sets minimum rotation angles for each axis in radian.
    void                SetAngularLowerLimits(const Vec3 &lower);
                        /// Gets maximum rotation angles for each axis in radian.
    const Vec3          GetAngularUpperLimits() const { return angularUpperLimits; }
                        /// Sets maximum rotation angles for each axis in radian.
    void                SetAngularUpperLimits(const Vec3 &upper);

                        /// Enable rotation limits
    void                EnableAngularLimits(bool enableX, bool enableY, bool enableZ);

protected:
    Vec3                linearLowerLimits;
    Vec3                linearUpperLimits;
    Vec3                angularLowerLimits;
    Vec3                angularUpperLimits;
};

BE_NAMESPACE_END
