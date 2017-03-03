#pragma once

#include "Component.h"

BE_NAMESPACE_BEGIN

class ComTransform;
class PhysRigidBody;

class ComAudioListener : public Component {
public:
    OBJECT_PROTOTYPE(ComAudioListener);

    ComAudioListener();
    virtual ~ComAudioListener();

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Awake() override;

    virtual void            Enable(bool enable) override;

    virtual void            Update() override;

protected:
    void                    PropertyChanged(const char *classname, const char *propName);
    void                    TransformUpdated(const ComTransform *transform);
    void                    PhysicsUpdated(const PhysRigidBody *body);
};

BE_NAMESPACE_END
