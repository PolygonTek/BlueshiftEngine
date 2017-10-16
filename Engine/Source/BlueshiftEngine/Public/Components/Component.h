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

                            /// Returns true if this component have render entity by checking renderEntityHandle
    virtual bool            HasRenderEntity(int renderEntityHandle) const { return false; }

                            /// Is initialized ? (initialized should be set to true after calling Init function)
    bool                    IsInitialized() const { return initialized; }

                            /// Can disable ?
    virtual bool            CanDisable() const { return true; }

                            /// Is enabled ?
    bool                    IsEnabled() const { return enabled; }

                            /// Enable/Disable this component
    virtual void            SetEnable(bool enable) { enabled = enable; }

                            /// Purge all the resources in this component
    virtual void            Purge(bool chainPurge = true);

                            /// Called in initialization
    virtual void            Init();

                            ///
    virtual void            Awake() {}
    
                            ///
    virtual void            Start() {}

                            ///
    virtual void            Update() {}

                            ///
    virtual void            LateUpdate() {}

                            ///
    virtual const AABB      GetAABB() { return AABB::zero; }

                            ///
    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const { return false; }

                            ///
    virtual void            DrawGizmos(const SceneView::Parms &sceneView, bool selected) {}

protected:
    virtual void            Event_ImmediateDestroy() override;

    void                    SetInitialized(bool init) { initialized = init; }
    void                    SetEntity(Entity *entity) { this->entity = entity; }

    void                    PropertyChanged(const char *classname, const char *propName);

    Entity *                entity;
    bool                    enabled;
    bool                    initialized;
};

BE_NAMESPACE_END
