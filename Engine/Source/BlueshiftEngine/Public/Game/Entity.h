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

/*
-------------------------------------------------------------------------------

    Entity

-------------------------------------------------------------------------------
*/

#include "Containers/HashTable.h"
#include "Core/Object.h"
#include "Render/SceneView.h"
#include "Components/Component.h"

BE_NAMESPACE_BEGIN

class Component;
class ComTransform;
class GameWorld;
class Prefab;
class Entity;

using EntityPtr = Entity*;
using EntityPtrArray = Array<EntityPtr>;

class Entity : public Object {
    friend class GameWorld;
    friend class GameEdit;
    friend class Prefab;
    friend class Component;

public:
    enum WorldPosEnum {
        Pivot,
        Center,
        Minimum,
        Maximum
    };

    OBJECT_PROTOTYPE(Entity);
    
    Entity();
    virtual ~Entity();

    virtual Str                 ToString() const override { return GetName(); }
    
                                /// Returns name.
    Str                         GetName() const { return name; }
                                /// Sets name.
    void                        SetName(const Str &name);

                                /// Returns tag name.
    Str                         GetTag() const { return tag; }
                                /// Sets tag name.
    void                        SetTag(const Str &tag);

                                /// Returns layer index.
    int                         GetLayer() const { return layer; }
                                /// Sets layer index.
    void                        SetLayer(int layer);

                                /// Returns if this entity is frozen. Frozen entity will not be selectable in editor.
    bool                        IsFrozen() const { return frozen; }
                                /// Sets this entity frozen.
    void                        SetFrozen(bool frozen);

    bool                        IsPrefabSource() const { return prefab; }

    Guid                        GetPrefabSourceGuid() const;
    void                        SetPrefabSourceGuid(const Guid &prefabSourceGuid);

    GameWorld *                 GetGameWorld() const { return gameWorld; }

    int                         GetEntityNum() const { return entityNum; }

                                /// Returns hierarchy node.
    const Hierarchy<Entity> &   GetNode() const { return node; }

                                /// Returns root entity.
    Entity *                    GetRoot() const;
                                /// Is root entity ?
    bool                        IsRoot() const { return GetRoot() == this; }

                                /// Returns parent entity.
    Entity *                    GetParent() const;
                                /// Sets parent entity.
    void                        SetParent(Entity *parentEntity);

                                /// Returns parent entity GUID.
    Guid                        GetParentGuid() const;
                                /// Sets parent entity by GUID.
    void                        SetParentGuid(const Guid &parentGuid);

                                /// Returns true if this entity have any children.
    bool                        HasChildren() const;
                                /// Gets the children by depth first order.
    void                        GetChildren(EntityPtrArray &children) const;
                                /// Finds a child entity with the given name.
    Entity *                    FindChild(const char *name) const;
    
                                /// Returns number of components.
    int                         NumComponents() const { return components.Count(); }
                                /// Checks if component exist by the given meta object.
    bool                        HasComponent(const MetaObject *type) const;
                                /// Returns a component pointer that is conflicting with other components.
    Component *                 GetConflictingComponent(const MetaObject *type) const;
                                /// Returns index of the component pointer.
    int                         GetComponentIndex(const Component *component) const;
                                /// Returns a component pointer by the given comopnent index.
    Component *                 GetComponent(int index) const { return components[index]; }
                                /// Returns a component pointer by the given meta object.
    Component *                 GetComponent(const MetaObject *type) const;
                                /// Returns a component pointer by the given type T.
    template <typename T> T *   GetComponent() const;
                                /// Returns a component pointer array of all.
    ComponentPtrArray &         GetComponents() { return components; }
                                /// Returns a component pointer array by the given meta object.
    ComponentPtrArray           GetComponents(const MetaObject *type) const;

                                /// Returns a transform component.
    ComTransform *              GetTransform() const;

                                /// Adds a component to the entity.
    void                        AddComponent(Component *component) { InsertComponent(component, components.Count()); }
                                /// Inserts a component after the index to the entity.
    void                        InsertComponent(Component *component, int index);
                                /// Removes a component.
    bool                        RemoveComponent(Component *component);
                                /// Swap two components.
    bool                        SwapComponent(int fromIndex, int toIndex);

    bool                        HasRenderEntity(int renderEntityHandle) const;

                                /// Purges all of the data.
    void                        Purge();

                                /// Initializes this entity. Always parent entities will be initialized first.
    void                        Init();
                                /// Initializes components.
    void                        InitComponents();

                                /// Called once when game started before Start()
                                /// When game already started, called immediately after spawned
    void                        Awake();
                                /// Called once when game started.
                                /// When game already started, called immediately after spawned
    void                        Start();

                                /// Called on game world update, variable timestep.
    void                        Update();
                                /// Called on game world late-update, variable timestep.
    void                        LateUpdate();
                                /// Called on physics update, fixed timestep.
    void                        FixedUpdate(float timeStep);
                                /// Called on physics late-update, fixed timestep.
    void                        FixedLateUpdate(float timeStep);
    
    void                        OnApplicationTerminate();
    void                        OnApplicationPause(bool pause);

                                /// Serializes entity to JSON value.
    virtual void                Serialize(Json::Value &data) const override;
                                /// Deserializes entity from JSON value.
    virtual void                Deserialize(const Json::Value &data) override;

    static void                 SerializeHierarchy(const Entity *entity, Json::Value &entitiesValue);

    bool                        IsActiveSelf() const { return activeSelf; }
    bool                        IsActiveInHierarchy() const { return activeInHierarchy; }

    void                        SetActive(bool active);

    virtual const AABB          GetAABB() const;
    const AABB                  GetWorldAABB() const;
    const Vec3                  GetWorldPosition(WorldPosEnum pos = Pivot) const;

                                /// Visualizes the component in editor.
    virtual void                DrawGizmos(const SceneView::Parms &sceneView, bool selected);

    virtual bool                RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const;

                                /// Creates an entity by JSON text.
    static Entity *             CreateEntity(Json::Value &data, GameWorld *gameWorld = nullptr);

                                /// Makes copy of JSON value of an entity and then replace each GUIDs of entity/components to the new ones.
    static Json::Value          CloneEntityValue(const Json::Value &entityValue, HashTable<Guid, Guid> &oldToNewGuidMap);
                                /// Makes copy of JSON value of entities and then replace each GUIDs of entity/components to the new ones.
    static Json::Value          CloneEntitiesValue(const Json::Value &entitiesValue, HashTable<Guid, Guid> &oldToNewGuidMap);

                                /// Replaces GUIDs of an entity (including components) using GUID map.
    static void                 RemapGuids(Entity *entity, const HashTable<Guid, Guid> &remapGuidMap);

    static void                 DestroyInstance(Entity *entity);

                                // FIXME: Don't emit these signals in player mode
    static const SignalDef      SIG_ActiveChanged;
    static const SignalDef      SIG_ActiveInHierarchyChanged;
    static const SignalDef      SIG_NameChanged;
    static const SignalDef      SIG_LayerChanged;
    static const SignalDef      SIG_FrozenChanged;
    static const SignalDef      SIG_ParentChanged;
    static const SignalDef      SIG_ComponentInserted;
    static const SignalDef      SIG_ComponentRemoved;
    static const SignalDef      SIG_ComponentSwapped;

protected:
    void                        SetActiveInHierarchy(bool active);

    virtual void                Event_ImmediateDestroy() override;

    Str                         name;               ///< Entity name
    int                         nameHash;           ///< Hash key for GameWorld::entityHash
    Str                         tag;                ///< Tag name
    int                         tagHash;            ///< Hash key for GameWorld::entityTagHash
    int                         entityNum;          ///< Index for GameWorld::entities
    int                         layer;              ///< Layer number
    Hierarchy<Entity>           node;
    Guid                        prefabSourceGuid;

    bool                        initialized;
    bool                        activeSelf;
    bool                        activeInHierarchy;
    bool                        prefab;
    bool                        frozen;

    GameWorld *                 gameWorld;

    ComponentPtrArray           components;         ///< 0'th component is always transform component
};

template <typename T>
BE_INLINE T *Entity::GetComponent() const {
    Component *component = GetComponent(&T::metaObject);
    if (component) {
        return component->Cast<T>();
    }

    return nullptr;
}

BE_INLINE bool Entity::HasComponent(const MetaObject *type) const {
    if (GetComponent(type)) {
        return true;
    }
    return false;
}

BE_INLINE int Entity::GetComponentIndex(const Component *component) const {
    return components.FindIndex(const_cast<Component *>(component));
}

BE_INLINE Component *Entity::GetComponent(const MetaObject *type) const {
    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component->GetMetaObject()->IsTypeOf(*type)) {
            return component;
        }
    }

    return nullptr;
}

BE_INLINE ComponentPtrArray Entity::GetComponents(const MetaObject *type) const {
    ComponentPtrArray subComponents;

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];

        if (component->GetMetaObject()->IsTypeOf(*type)) {
            subComponents.Append(component);
        }
    }

    return subComponents;
}

BE_INLINE Component *Entity::GetConflictingComponent(const MetaObject *type) const {
    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component->IsConflictComponent(*type)) {
            return component;
        }
    }

    return nullptr;
}

BE_INLINE Entity *Entity::GetRoot() const {
    const Entity *ent = this;
    while (ent->GetParent()) {
        ent = ent->GetParent();
    }
    return const_cast<Entity *>(ent);
}

BE_INLINE Entity *Entity::GetParent() const {
    return node.GetParent();
}

BE_NAMESPACE_END
