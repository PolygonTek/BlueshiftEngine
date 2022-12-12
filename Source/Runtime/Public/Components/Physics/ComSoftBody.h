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

#include "Physics/Physics.h"
#include "Components/Component.h"

BE_NAMESPACE_BEGIN

class ComTransform;

class ComSoftBody : public Component {
public:
    OBJECT_PROTOTYPE(ComSoftBody);

    ComSoftBody();
    virtual ~ComSoftBody();

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned.
    virtual void            Awake() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

    float                   GetMass() const;
    void                    SetMass(float mass);

    float                   GetRestitution() const;
    void                    SetRestitution(float restitution);

    float                   GetFriction() const;
    void                    SetFriction(float friction);

    float                   GetStretchingStiffness() const;
    void                    SetStretchingStiffness(float stiffness);

    int                     GetSolverIterationCount() const;
    void                    SetSolverIterationCount(int iterationCount);

    bool                    IsCCDEnabled() const;
    void                    SetCCDEnabled(bool enabled);

    const Vec3              GetGravity() const { return body ? body->GetGravity() : Vec3::zero; }
    void                    SetGravity(const Vec3 &gravityAcceleration) { if (body) body->SetGravity(gravityAcceleration); }

    float                   GetPointWeight(int index) const;
    void                    SetPointWeight(int index, float weight);
    int                     GetPointWeightCount() const { return physicsDesc.pointWeights.Count(); }
    void                    SetPointWeightCount(int count);

#if WITH_EDITOR
                            /// Visualize the component in editor
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

protected:
    void                    CreateBody();
    void                    InitPoints();
    void                    UpdateMeshVertsFromPoints();

    void                    TransformUpdated(const ComTransform *transform);

    PhysSoftBody *          body = nullptr;
    PhysCollidableDesc      physicsDesc;
    int                     positionSolverIterationCount = 5;
    float                   stretchingStiffness = 1.0f;
};

BE_NAMESPACE_END
