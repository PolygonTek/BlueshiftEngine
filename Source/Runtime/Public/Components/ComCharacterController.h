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
#include "Component.h"
#include "Game/CastResult.h"

BE_NAMESPACE_BEGIN

class ComTransform;
class PhysRigidBody;
class PhysSensor;

class ComCharacterController : public Component {
public:
    OBJECT_PROTOTYPE(ComCharacterController);

    ComCharacterController();
    virtual ~ComCharacterController();

                            /// Returns true if this component conflicts with the given component
    virtual bool            IsConflictComponent(const MetaObject &componentClass) const override;

    virtual void            Purge(bool chainPurge = true) override;

                            /// Initializes this component. Called after deserialization.
    virtual void            Init() override;

                            /// Called once when game started before Start()
                            /// When game already started, called immediately after spawned
    virtual void            Awake() override;

                            /// Called on game world update, variable timestep.
    virtual void            Update() override;

#if WITH_EDITOR
                            /// Visualize the component in editor
    virtual void            DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) override;
#endif

    bool                    IsOnGround() const { return onGround; }

    bool                    Move(const Vec3 &moveVector);

    float                   GetMass() const;
    void                    SetMass(const float mass);
    
    float                   GetCapsuleRadius() const;
    void                    SetCapsuleRadius(float capsuleRadius);
    
    float                   GetCapsuleHeight() const;
    void                    SetCapsuleHeight(float capsuleHeight);
    
    float                   GetStepOffset() const;
    void                    SetStepOffset(float stepOffset);
        
    float                   GetSlopeLimit() const;
    void                    SetSlopeLimit(float slopeLimit);

protected:
    virtual void            OnActive() override;
    virtual void            OnInactive() override;

    void                    CreateBodyAndSensor();
    void                    GroundTrace();
    void                    RecoverFromPenetration();
    bool                    SlideMove(const Vec3 &moveVector);

    void                    TransformUpdated(const ComTransform *transform);

    Collider *              collider;
    PhysRigidBody *         body;
    PhysSensor *            correctionSensor;

    Vec3                    origin;
    CastResult              groundTrace;
    bool                    isValidGroundTrace;
    bool                    onGround;

    float                   mass;
    float                   capsuleRadius;
    float                   capsuleHeight;
    float                   stepOffset;
    float                   slopeDotZ;
};

BE_NAMESPACE_END
