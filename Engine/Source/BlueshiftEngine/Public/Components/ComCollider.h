#pragma once

#include "Component.h"

BE_NAMESPACE_BEGIN

class Collider;

class ComCollider : public Component {
public:
    ABSTRACT_PROTOTYPE(ComCollider);

    ComCollider();
    virtual ~ComCollider() = 0;

    virtual bool            AllowSameComponent() const override { return true; }

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Enable(bool enable) override;

    virtual const AABB      GetAABB() override;

    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const override;

    Collider *              GetCollider() const { return collider; }

protected:
    void                    PropertyChanged(const char *classname, const char *propName);

    Str                     material;
    Collider *              collider;
};

BE_NAMESPACE_END
