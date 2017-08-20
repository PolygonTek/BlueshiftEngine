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

using EntityPtr                 = Entity*;
using EntityPtrArray            = Array<EntityPtr>;

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

    virtual const Str           ToString() const override { return GetName(); }
    
    const char *                GetName() const { return name.c_str(); }

    const char *                GetTag() const { return tag.c_str(); }

    const int                   GetLayer() const { return props->Get("layer").As<int>(); }

    int                         GetEntityNum() const { return entityNum; }
    int                         GetSpawnId() const;

                                // frozen entity is not selectable in the Editor
    bool                        IsFrozen() const { return frozen; }

    bool                        IsPrefabParent() const;
    bool                        IsPrefabInstance() const;
    Entity *                    GetPrefabParent() const;

    GameWorld *                 GetGameWorld() const { return gameWorld; }

                                // hierarchy
    const Hierarchy<Entity> &   GetNode() const { return node; }
    bool                        IsRoot() const { return GetRoot() == this; }
    Entity *                    GetRoot() const;
    Entity *                    GetParent() const;
    void                        SetParent(int parentGuid);

                                /// Get the children by depth first order
    void                        GetChildren(EntityPtrArray &children) const;
    
                                /// Find a child by name
    Entity *                    FindChild(const char *name) const;
    
                                /// Number of components
    int                         NumComponents() const { return components.Count(); }

    bool                        HasComponent(const MetaObject &type) const;

    Component *                 GetConflictingComponent(const MetaObject &type) const;
    int                         GetComponentIndex(const Component *component) const;
    Component *                 GetComponent(int index) const { return components[index]; }
    Component *                 GetComponent(const MetaObject &type) const;
    template <typename T> T *   GetComponent() const;
    ComponentPtrArray           GetComponents(const MetaObject &type) const;
    ComTransform *              GetTransform() const;

                                /// Adds a component to the entity
    void                        AddComponent(Component *component);

                                /// Inserts a component after the index to the entity
    void                        InsertComponent(Component *component, int index);

    bool                        HasRenderEntity(int renderEntityHandle) const;

    void                        PurgeJointComponents();
    void                        Purge();

                                // Create an entity by JSON text.
                                // Just initialize properties of an entity and it's components.
                                // Later this have to be initialized by it's properties.
    static Entity *             CreateEntity(Json::Value &entityValue);

                                // Make copy of a entity's JSON value and replace the GUID of entity/components to new one
    static Json::Value          CloneEntityValue(const Json::Value &entityValue, HashTable<Guid, Guid> &oldToNewGuidMap);

    static void                 RemapGuids(EntityPtrArray &entities, const HashTable<Guid, Guid> &guidMap);

    void                        InitHierarchy();

                                // entity 초기화 함수, 언제나 부모 entity 초기화가 먼저 실행된다.
    void                        Init();

    void                        Awake();

                                // game 이 시작되어 update loop 에 들어가면 처음 한번만 실행된다. 
                                // game 중이라면 spawn 된 후 바로 실행한다.
    void                        Start();

    void                        Update();

    void                        LateUpdate();
    
    void                        OnApplicationTerminate();
    void                        OnApplicationPause(bool pause);

    void                        Serialize(Json::Value &entityValue) const;

    bool                        IsActiveSelf() const;
    void                        SetActive(bool active);

    virtual const AABB          GetAABB() const;
    const AABB                  GetWorldAABB() const;
    const Vec3                  GetWorldPosition(WorldPosEnum pos = Pivot) const;

    virtual void                DrawGizmos(const SceneView::Parms &sceneView, bool selected);

    virtual bool                RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const;

    static void                 Destroy(Entity *entity);

    static const SignalDef      SIG_ComponentInserted;
    static const SignalDef      SIG_ComponentRemoved;
    static const SignalDef      SIG_LayerChanged;
        
protected:
    virtual void                Event_ImmediateDestroy() override;

    void                        PropertyChanged(const char *classname, const char *propName);

    Str                         name;
    int                         nameHash;       // hash key for gameWorld->entityHash
    Str                         tag;
    int                         tagHash;        // hash key for gameWorld->entityTagHash
    int                         entityNum;      // index for gameWorld->entities
    Hierarchy<Entity>           node;

    bool                        initialized;
    bool                        frozen;

    GameWorld *                 gameWorld;

    ComponentPtrArray           components;     ///< 0'th component is always transform component
};

template <typename T>
BE_INLINE T *Entity::GetComponent() const {
    Component *component = GetComponent(T::metaObject);
    if (component) {
        return component->Cast<T>();
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
