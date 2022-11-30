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

/// Swing twist constraint for humanoid ragdolls that allows limited rotation only
class PhysSwingTwistConstraint : public PhysConstraint {
public:
    PhysSwingTwistConstraint(PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB);
    PhysSwingTwistConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB);

    void                SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA);
    void                SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB);

                        /// Gets swing 2 span angle in radian.
    float               GetSwing1SpanLimit() const { return swing1SpanLimit; }
                        /// Gets swing 1 span angle in radian.
    float               GetSwing2SpanLimit() const { return swing2SpanLimit; }
                        /// Gets twist span angle in radian.
    float               GetTwistSpanLimit() const { return twistSpanLimit; }
                        /// Sets span angles in radian.
    void                SetSpanLimits(float swing1SpanLimit, float swing2SpanLimit, float twistSpanLimit);

private:
    float               swing1SpanLimit;
    float               swing2SpanLimit;
    float               twistSpanLimit;
};

BE_NAMESPACE_END
