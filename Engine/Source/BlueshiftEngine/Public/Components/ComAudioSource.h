#pragma once

#include "Component.h"

BE_NAMESPACE_BEGIN

class ComTransform;
class PhysRigidBody;
class Sound;

class ComAudioSource : public Component {
public:
    OBJECT_PROTOTYPE(ComAudioSource);

    ComAudioSource();
    virtual ~ComAudioSource();

    virtual void            Purge(bool chainPurge = true) override;

    virtual void            Init() override;

    virtual void            Awake() override;

    virtual void            Enable(bool enable) override;

    virtual void            Update() override;

    void                    Play();
    void                    Stop();

protected:
    const Guid              GetAudioClip() const;
    void                    SetAudioClip(const Guid &guid);

    void                    PropertyChanged(const char *classname, const char *propName);
    void                    TransformUpdated(const ComTransform *transform);
    void                    PhysicsUpdated(const PhysRigidBody *body);

    Sound *                 referenceSound;
    Sound *                 sound;
    Str                     audioClipPath;
    bool                    playOnAwake;
    float                   minDistance;
    float                   maxDistance;
    float                   volume;
    bool                    looping;
};

BE_NAMESPACE_END
