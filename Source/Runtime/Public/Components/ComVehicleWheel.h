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

#include "Component.h"

BE_NAMESPACE_BEGIN

class PhysVehicle;
class ComRigidBody;

class ComVehicleWheel : public Component {
    friend class ComRigidBody;

public:
    OBJECT_PROTOTYPE(ComVehicleWheel);

    ComVehicleWheel();
    virtual ~ComVehicleWheel();

                            /// Returns true if this component conflicts with the given component.
    virtual bool            IsConflictComponent(const MetaObject &componentClass) const override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

#if WITH_EDITOR
                            /// Visualize the component in editor.
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

    void                    ClearCache();

    const Vec3 &            GetLocalOrigin() const { return localOrigin; }
    void                    SetLocalOrigin(const Vec3 &origin);

    Angles                  GetLocalAngles() const { return localAxis.ToAngles(); }
    void                    SetLocalAngles(const Angles &angles);

    float                   GetRadius() const { return radius; }
    void                    SetRadius(float radius);

    float                   GetSuspensionDistance() const { return suspensionRestLength; }
    void                    SetSuspensionDistance(float susDist);

    float                   GetSuspensionMaxDistance() const { return suspensionMaxDistance; }
    void                    SetSuspensionMaxDistance(float susMaxDist);

    float                   GetSuspensionMaxForce() const { return suspensionMaxForce; }
    void                    SetSuspensionMaxForce(float susMaxForce);

    float                   GetSuspensionStiffness() const { return suspensionStiffness; }
    void                    SetSuspensionStiffness(float susStiffness);

    float                   GetSuspensionDampingRelaxation() const { return suspensionDampingRelaxation; }
    void                    SetSuspensionDampingRelaxation (float susDampingRelaxation);

    float                   GetSuspensionDampingCompression() const { return suspensionDampingCompression; }
    void                    SetSuspensionDampingCompression(float susDampingCompression);

    float                   GetRollingFriction() const { return rollingFriction; }
    void                    SetRollingFriction(float rollingFriction);

    float                   GetRollingInfluence() const { return rollingInfluence; }
    void                    SetRollingInfluence(float rollingInfluence);

    float                   GetSteeringAngle() const;
    void                    SetSteeringAngle(float angle);

    float                   GetTorque() const;
    void                    SetTorque(float torque);

    float                   GetBrakingTorque() const;
    void                    SetBrakingTorque(float brakingTorque);

    float                   GetSkidInfo() const;

    float                   GetSuspensionRelativeVelocity() const;

protected:
    Mat3                    localAxis = Mat3::identity;
    Vec3                    localOrigin = Vec3::origin;
    float                   radius = MeterToUnit(0.5f);
    float                   suspensionRestLength = MeterToUnit(0.0f);
    float                   suspensionMaxDistance = MeterToUnit(0.3f);
    float                   suspensionMaxForce = MeterToUnit(100.0f);
    float                   suspensionStiffness = 60.f;
    float                   suspensionDampingRelaxation = 2.3f;
    float                   suspensionDampingCompression = 4.4f;
    float                   rollingFriction = 2.0f;
    float                   rollingInfluence = 0.1f;
    PhysVehicle *           vehicle = nullptr;
    int                     vehicleWheelIndex = -1;
};

BE_NAMESPACE_END
