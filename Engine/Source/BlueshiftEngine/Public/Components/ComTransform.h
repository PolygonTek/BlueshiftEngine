#pragma once

#include "Component.h"

BE_NAMESPACE_BEGIN

class PhysRigidBody;

class ComTransform : public Component {
    friend class Entity;

public:
    OBJECT_PROTOTYPE(ComTransform);
    
    ComTransform();
    virtual ~ComTransform();

    virtual void            Init() override;

    virtual bool            CanDisable() const override;

    ComTransform *          GetParent() const;

                            // origin/scale/axis in local space 
    const Vec3 &            GetLocalOrigin() const { return localOrigin; }
    const Vec3 &            GetLocalScale() const { return localScale; }
    const Mat3 &            GetLocalAxis() const { return localAxis; }
    const Angles            GetLocalAngles() const { return GetLocalAxis().ToAngles(); }

    void                    SetLocalOrigin(const Vec3 &origin);
    void                    SetLocalScale(const Vec3 &scale);
    void                    SetLocalAxis(const Mat3 &axis);
    void                    SetLocalAngles(const Angles &localAngles) { SetLocalAxis(localAngles.ToMat3()); }
    void                    SetLocalTransform(const Vec3 &origin, const Vec3 &scale, const Mat3 &axis);

                            // origin/scale/axis in world space
    const Vec3              GetOrigin() const;
    const Vec3              GetScale() const;
    const Mat3              GetAxis() const;
    const Angles            GetAngles() const { return GetAxis().ToAngles(); }

    const Vec3              Forward() const { return GetAxis()[0]; }
    const Vec3              Right() const { return GetAxis()[1]; }
    const Vec3              Up() const { return GetAxis()[2]; }

    void                    SetOrigin(const Vec3 &origin);
    void                    SetAxis(const Mat3 &axis);
    void                    SetAngles(const Angles &angles) { SetAxis(angles.ToMat3()); }

    const Mat4 &            GetLocalMatrix() const { return localMatrix; }
    const Mat4 &            GetWorldMatrix() const { return worldMatrix; }

    void                    Translate(const Vec3 &translation);
    void                    Rotate(const Vec3 axis, float angle);

protected:
    void                    RecalcWorldMatrix();
    void                    RecalcLocalMatrix();
    void                    UpdateChildren(bool ignorePhysicsEntity = false);
    void                    PhysicsUpdated(const PhysRigidBody *body);
    void                    PropertyChanged(const char *classname, const char *propName);

    Vec3                    localOrigin;
    Vec3                    localScale;
    Mat3                    localAxis;

    Mat4                    localMatrix;
    Mat4                    worldMatrix;
};

extern const SignalDef      SIG_TransformUpdated;

BE_NAMESPACE_END
