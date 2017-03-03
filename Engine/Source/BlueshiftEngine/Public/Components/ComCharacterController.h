#pragma once

#include "Physics/Physics.h"
#include "Component.h"
#include "Game/CastResult.h"

BE_NAMESPACE_BEGIN

class LuaVM;
class ComTransform;
class PhysCollidable;
class PhysRigidBody;
class PhysSensor;

class ComCharacterController : public Component {
    friend class LuaVM;

public:
    OBJECT_PROTOTYPE(ComCharacterController);

    ComCharacterController();
    virtual ~ComCharacterController();

    virtual bool            IsConflictComponent(const MetaObject &componentClass) const override;

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Awake() override;

    virtual void            Enable(bool enable) override;

    virtual void            Update() override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

    bool                    IsOnGround() const { return onGround; }

    bool                    Move(const Vec3 &moveVector);

protected:
    void                    GroundTrace();
    void                    RecoverFromPenetration();
    bool                    SlideMove(const Vec3 &moveVector);

    float                   GetMass() const;
    void                    SetMass(const float mass);

    float                   GetCapsuleRadius() const;
    void                    SetCapsuleRadius(const float capsuleRadius);

    float                   GetCapsuleHeight() const;
    void                    SetCapsuleHeight(const float capsuleHeight);

    float                   GetStepOffset() const;
    void                    SetStepOffset(const float stepOffset);

    float                   GetSlopeLimit() const;
    void                    SetSlopeLimit(const float slopeLimit);

    void                    PropertyChanged(const char *classname, const char *propName);
    void                    TransformUpdated(const ComTransform *transform);
    void                    PhysicsUpdated(const PhysRigidBody *body);	

    Collider *              collider;
    PhysRigidBody *         body;
    PhysSensor *            correctionSensor;

    Vec3                    origin;
    CastResultEx            groundTrace;
    bool                    isValidGroundTrace;
    bool                    onGround;

    float                   mass;
    float                   capsuleRadius;
    float                   capsuleHeight;
    float                   stepOffset;
    float                   slopeDotZ;
};

BE_NAMESPACE_END
