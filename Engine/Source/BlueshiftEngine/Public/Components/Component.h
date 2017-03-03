#pragma once

#include "Core/Object.h"
#include "Render/SceneView.h"

BE_NAMESPACE_BEGIN

class Entity;
class GameWorld;
class Component;

using ComponentPtr          = Component*;
using ComponentPtrArray     = Array<ComponentPtr>;
  
class Component : public Object {
    friend class Entity;

public:
    ABSTRACT_PROTOTYPE(Component);

    Component();
    virtual ~Component() = 0;

                            /// Components share the same name with the entity
    virtual const Str       ToString() const override;
    
                            /// Get the entity that own this component
    Entity *                GetEntity() const { return entity; }
    
                            /// Get the object GameWorld
    GameWorld *             GetGameWorld() const;

                            /// Same component is allowed in a entity ?
    virtual bool            AllowSameComponent() const { return false; }

                            /// Is conflict with specific component ?
    virtual bool            IsConflictComponent(const MetaObject &componentClass) const { return false; }

                            //
    virtual bool            HasRenderEntity(int renderEntityHandle) const { return false; }

                            //
    bool                    IsInitalized() const { return initialized; }

                            /// Can disable ?
    virtual bool            CanDisable() const { return true; }

                            /// Is enabled ?
    bool                    IsEnabled() const { return enabled; }

                            /// Enable/Disable this component
    virtual void            Enable(bool enable) { enabled = enable; }

                            /// Purge all the resources in this component    
    virtual void            Purge(bool chainPurge = true);

                            /// Called in initialization
    virtual void            Init();

                            //
    virtual void            Awake() {}
    
                            //
    virtual void            Start() {}

                            //
    virtual void            Update() {}

                            //
    virtual void            LateUpdate() {}

                            //
    virtual const AABB      GetAABB() { return AABB::zero; }

                            //
    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const { return false; }

                            //
    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) {}

protected:
    virtual void            Event_ImmediateDestroy() override;

    void                    SetInitialized(bool init) { initialized = init; }
    void                    SetEntity(Entity *entity) { this->entity = entity; }

    void                    Reload();
    void                    PropertyChanged(const char *classname, const char *propName);

    Entity *                entity;
    bool                    enabled;
    bool                    initialized;
};

extern const SignalDef      SIG_UpdateUI;

BE_NAMESPACE_END


