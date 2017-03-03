#pragma once

#include "Physics/Physics.h"
#include "Component.h"

BE_NAMESPACE_BEGIN

class ComTransform;

class ComSensor : public Component {
public:
    OBJECT_PROTOTYPE(ComSensor);

    ComSensor();
    virtual ~ComSensor();

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Awake() override;

    virtual void            Enable(bool enable) override;

    virtual void            Update() override;

    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) override;

protected:
    void                    PropertyChanged(const char *classname, const char *propName);
    void                    TransformUpdated(const ComTransform *transform);
    void                    PhysicsUpdated(const PhysRigidBody *body);

    PhysSensor *            sensor;
    Array<PhysCollidable *> oldColliderArray;
    PhysCollidableDesc      physicsDesc;
};

BE_NAMESPACE_END
