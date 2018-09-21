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
#include "Render/RenderView.h"

BE_NAMESPACE_BEGIN

class Entity;
class GameWorld;
class Component;

using ComponentPtr = Component*;
using ComponentPtrArray = Array<ComponentPtr>;
  
class Component : public Object {
    friend class Entity;

public:
    ABSTRACT_PROTOTYPE(Component);

    Component();
    virtual ~Component() = 0;

                            /// Components share the same name with the entity.
    virtual Str             ToString() const override;
    
                            /// Get the entity that own this component.
    Entity *                GetEntity() const { return entity; }

    void                    SetEntity(Entity *entity) { this->entity = entity; }
    
                            /// Get the game world object.
    GameWorld *             GetGameWorld() const;

                            /// Returns true if the same component is allowed.
    virtual bool            AllowSameComponent() const { return false; }
                            /// Returns true if this component conflicts with the given component.
    virtual bool            IsConflictComponent(const MetaObject &componentClass) const { return false; }

                            /// Returns true if this component have render entity by checking renderEntityHandle.
    virtual bool            HasRenderEntity(int renderEntityHandle) const { return false; }

                            /// Is initialized ? (initialized should be set to true after calling Init function)
    bool                    IsInitialized() const { return initialized; }

                            /// Can disable ?
    virtual bool            CanDisable() const { return true; }
                            /// Is enabled ?
    bool                    IsEnabled() const { return enabled; }
                            /// Set enabled/disabled this component.
    void                    SetEnabled(bool enable);

    bool                    IsActiveInHierarchy() const;

                            /// Purges all the resources of the component, chainPurge for parent class
    virtual void            Purge(bool chainPurge = true);

                            /// Initializes this component. Called after deserialization.
    virtual void            Init();

                            /// Called once when game started before Start().
                            /// When game already started, called immediately after spawned.
    virtual void            Awake() {}

                            /// Called on scene update, variable timestep.
    virtual void            Update() {}

                            ///
    virtual const AABB      GetAABB() { return AABB::zero; }

                            ///
    virtual bool            RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const { return false; }

                            /// Visualize the component in editor.
    virtual void            DrawGizmos(const RenderView::State &sceneView, bool selected) {}

protected:
    virtual void            OnActive() {}
    virtual void            OnInactive() {}

    virtual void            Event_ImmediateDestroy() override;

    void                    SetInitialized(bool init) { initialized = init; }

    Entity *                entity;
    bool                    enabled;
    bool                    initialized;
};

BE_NAMESPACE_END
