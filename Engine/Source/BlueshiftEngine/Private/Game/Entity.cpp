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

#include "Precompiled.h"
#include "Components/Component.h"
#include "Components/ComTransform.h"
#include "Components/ComJoint.h"
#include "Components/ComRenderable.h"
#include "Components/ComScript.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

const SignalDef Entity::SIG_ComponentInserted("componentInserted", "ai");
const SignalDef Entity::SIG_ComponentRemoved("componentRemoved", "a");
const SignalDef Entity::SIG_LayerChanged("layerChanged", "a");

OBJECT_DECLARATION("Entity", Entity, Object)
BEGIN_EVENTS(Entity)
END_EVENTS
BEGIN_PROPERTIES(Entity)
    PROPERTY_OBJECT("parent", "Parent", "parent entity", Guid::zero, Entity::metaObject, PropertyInfo::ReadWrite),
    PROPERTY_BOOL("prefab", "Prefab", "is prefab ?", false, PropertyInfo::ReadWrite),
    PROPERTY_OBJECT("prefabSource", "Prefab Source", "prefab source entity", Guid::zero, Entity::metaObject, PropertyInfo::ReadWrite),
    PROPERTY_STRING("name", "Name", "entity name", "Entity", PropertyInfo::ReadWrite),
    PROPERTY_STRING("tag", "Tag", "Tag", "Untagged", PropertyInfo::ReadWrite),
    PROPERTY_INT("layer", "Layer", "Layer", 0, PropertyInfo::ReadWrite),
    PROPERTY_BOOL("frozen", "Frozen", "is frozen ?", false, PropertyInfo::ReadWrite),
END_PROPERTIES

#ifdef NEW_PROPERTY_SYSTEM
void Entity::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("Parent", ObjectRef, GetParentRef, SetParentRef, ObjectRef(Entity::metaObject, Guid::zero), "Parent Entity", PropertyInfo::ReadWrite);
    REGISTER_PROPERTY("Prefab", bool, prefab, false, "", PropertyInfo::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Prefab Source", ObjectRef, GetPrefabSourceRef, SetPrefabSourceRef, ObjectRef(Entity::metaObject, Guid::zero), "", PropertyInfo::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Name", Str, GetName, SetName, "Entity", "", PropertyInfo::ReadWrite);
    REGISTER_MIXED_ACCESSOR_PROPERTY("Tag", Str, GetTag, SetTag, "Untagged", "", PropertyInfo::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Layer", int, GetLayer, SetLayer, 0, "", PropertyInfo::ReadWrite);
    REGISTER_ACCESSOR_PROPERTY("Frozen", bool, IsFrozen, SetFrozen, false, "", PropertyInfo::ReadWrite);
}
#endif

Entity::Entity() {
    gameWorld = nullptr;
    entityNum = GameWorld::BadEntityNum;
    node.SetOwner(this);
    layer = 0;
    frozen = false;
    prefab = false;
    prefabSourceGuid = Guid::zero;
    initialized = false;

#ifndef NEW_PROPERTY_SYSTEM
    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&Entity::PropertyChanged);
#endif
}

Entity::~Entity() {
    Purge();
}

void Entity::Purge() {
    // Purge all the components in opposite order
    for (int componentIndex = components.Count() - 1; componentIndex >= 0; componentIndex--) {
        Component *component = components[componentIndex];
        if (component) {
            component->Purge();
        }
    }

    initialized = false;
}

void Entity::Event_ImmediateDestroy() {
    if (gameWorld) {
        if (gameWorld->IsRegisteredEntity(this)) {
            gameWorld->UnregisterEntity(this);
        }
    }

    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        component->entity = nullptr;
        Component::DestroyInstanceImmediate(component);
    }

    components.Clear();

    Object::Event_ImmediateDestroy();
}

void Entity::Init() {
#ifndef NEW_PROPERTY_SYSTEM
    name = props->Get("name").As<Str>();
    tag = props->Get("tag").As<Str>();
    layer = props->Get("layer").As<int>();
    frozen = props->Get("frozen").As<bool>();

    const Guid parentGuid = props->Get("parent").As<Guid>();
    if (!parentGuid.IsZero()) {
        Object *parentObject = Entity::FindInstance(parentGuid);
        Entity *parent = parentObject ? parentObject->Cast<Entity>() : nullptr;
        if (!parent) {
            BE_WARNLOG(L"Couldn't find parent entity %hs of %hs\n", parentGuid.ToString(), name.c_str());
        } else {
            node.SetParent(parent->node);
        }
    }

    prefab = props->Get("prefab").As<bool>();
    prefabSourceGuid = props->Get("prefabSource").As<Guid>();
#endif

    initialized = true;
}

void Entity::InitComponents() {
    assert(gameWorld);

    // Initialize components
    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component) {
            if (!component->IsInitialized()) {
                component->Init();
            }
        }
    }

    ComRenderable *renderable = GetComponent<ComRenderable>();
    if (renderable) {
        renderable->props->Set("skipSelection", frozen);
    }
}

void Entity::Awake() {
    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component) {
            component->Awake();
        }
    }
}

void Entity::Start() {
    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component) {
            component->Start();
        }
    }
}

void Entity::Update() {
    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component && component->IsEnabled()) {
            component->Update();
        }
    }
}

void Entity::LateUpdate() {
    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component && component->IsEnabled()) {
            component->LateUpdate();
        }
    }
}

int Entity::GetSpawnId() const {
    assert(gameWorld);
    return gameWorld->GetEntitySpawnId(this);
}

ComTransform *Entity::GetTransform() const {
    ComTransform *transform = static_cast<ComTransform *>(GetComponent(0));
    assert(transform);
    return transform;
}

void Entity::InsertComponent(Component *component, int index) {
    component->SetEntity(this);

    components.Insert(component, index);

    EmitSignal(&SIG_ComponentInserted, component, index);
}

void Entity::GetChildren(EntityPtrArray &children) const {
    for (Entity *child = node.GetChild(); child; child = child->node.GetNextSibling()) {
        children.Append(child);
        child->GetChildren(children);
    }
}

Entity *Entity::FindChild(const char *name) const {
    for (Entity *child = node.GetChild(); child; child = child->node.GetNextSibling()) {
        if (!Str::Cmp(child->GetName(), name)) {
            return child;
        }
    }
    return nullptr;
}

bool Entity::HasRenderEntity(int renderEntityHandle) const {
    for (int componentIndex = 1; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component) {
            if (component->HasRenderEntity(renderEntityHandle)) {
                return true;
            }
        }
    }
    return false;
}

void Entity::OnApplicationTerminate() {
    ComponentPtrArray scriptComponents = GetComponents(ComScript::metaObject);
    for (int i = 0; i < scriptComponents.Count(); i++) {
        ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();
        
        scriptComponent->OnApplicationTerminate();
    }
}

void Entity::OnApplicationPause(bool pause) {
    ComponentPtrArray scriptComponents = GetComponents(ComScript::metaObject);
    for (int i = 0; i < scriptComponents.Count(); i++) {
        ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();
        
        scriptComponent->OnApplicationPause(pause);
    }
}

void Entity::Serialize(Json::Value &value) const {
    Json::Value componentsValue;

    props->Serialize(value);

    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component) {
            Json::Value componentValue;
            component->props->Serialize(componentValue);

            componentsValue.append(componentValue);
        }
    }

    value["components"] = componentsValue;
}

bool Entity::IsActiveSelf() const {
    for (int componentIndex = 1; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component) {
            if (component->IsEnabled()) {
                return true;
            }
        }
    }

    return false;
}

void Entity::SetActive(bool active) {
    for (int componentIndex = 1; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component) {
            component->SetEnable(active);
        }
    }

    for (Entity *childEnt = node.GetChild(); childEnt; childEnt = childEnt->node.GetNextSibling()) {
        childEnt->SetActive(active);
    }
}

const AABB Entity::GetAABB() const {
    AABB aabb;
    aabb.Clear();

    for (int componentIndex = 1; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component) {
            aabb.AddAABB(component->GetAABB());
        }
    }

    return aabb;
}

const AABB Entity::GetWorldAABB() const {
    const ComTransform *transform = GetTransform();

    AABB worldAabb;
    worldAabb.SetFromTransformedAABB(GetAABB(), transform->GetOrigin(), transform->GetAxis());
    return worldAabb;
}

const Vec3 Entity::GetWorldPosition(WorldPosEnum pos) const {
    Vec3 vec;
    
    if (pos == Pivot) {
        vec = GetTransform()->GetOrigin();
    } else {
        AABB aabb = GetWorldAABB();
            
        if (pos == Minimum) {
            vec = aabb.b[0];
        } else if (pos == Maximum) {
            vec = aabb.b[1];
        } else if (pos == Center) {
            vec = aabb.Center();
        } else {
            assert(0);
        }
    }

    return vec;
}

void Entity::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    for (int componentIndex = 1; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component && component->IsEnabled()) {
            component->DrawGizmos(sceneView, selected);
        }
    }
}

bool Entity::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    float s = lastScale;

    for (int componentIndex = 1; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component && component->IsEnabled()) {
            component->RayIntersection(start, dir, backFaceCull, s);
        }
    }

    if (s < lastScale) {
        lastScale = s;
        return true;
    }

    return false;
}

void Entity::DestroyInstance(Entity *entity) {
    EntityPtrArray children;
    entity->GetChildren(children);

    for (int i = children.Count() - 1; i >= 0; i--) {
        Object::DestroyInstance(children[i]);
    }
    Object::DestroyInstance(entity);
}

void Entity::PropertyChanged(const char *classname, const char *propName) {
    if (!initialized) {
        return;
    }

    if (!Str::Cmp(propName, "name")) {
        SetName(props->Get("name").As<Str>());
        return;
    }

    if (!Str::Cmp(propName, "tag")) {
        SetTag(props->Get("tag").As<Str>());
        return;
    }

    if (!Str::Cmp(propName, "layer")) {
        SetLayer(props->Get("tag").As<int>());
        return;
    }

    if (!Str::Cmp(propName, "parent")) {
        SetParentGuid(props->Get("parent").As<Guid>());
        return;
    }

    if (!Str::Cmp(propName, "prefab")) {
        prefab = props->Get("prefab").As<bool>();
        return;
    }

    if (!Str::Cmp(propName, "prefabSource")) {
        SetPrefabSource(props->Get("prefabSource").As<Guid>());
        return;
    }

    if (!Str::Cmp(propName, "frozen")) {
        SetFrozen(props->Get("frozen").As<bool>());
        return;
    }
}

void Entity::SetName(const Str &name) {
    this->name = name;

    GetGameWorld()->OnEntityNameChanged(this);
}

void Entity::SetTag(const Str &tag) {
    this->tag = tag;

    GetGameWorld()->OnEntityTagChanged(this);
}

void Entity::SetLayer(int layer) {
    this->layer = layer;

    EmitSignal(&SIG_LayerChanged, this);
}

void Entity::SetParent(Entity *parentEntity) {
    SetParentGuid(parentEntity->GetGuid());
}

Guid Entity::GetParentGuid() const {
    Entity *parentEntity = node.GetParent();
    if (parentEntity) {
        return parentEntity->GetGuid();
    }
    return Guid::zero;
}

void Entity::SetParentGuid(const Guid &parentGuid) {
    Object *parentObject = Entity::FindInstance(parentGuid);
    Entity *parent = parentObject ? parentObject->Cast<Entity>() : nullptr;
    ComTransform *transform = GetTransform();
    Mat4 localMatrix;

    if (parent) {
        node.SetParent(parent->node);

        localMatrix = parent->GetTransform()->GetWorldMatrix().AffineInverse() * transform->GetWorldMatrix();
    } else {
        node.SetParent(gameWorld->GetEntityHierarchy());

        localMatrix = transform->GetWorldMatrix();
    }

    Mat3 axis = localMatrix.ToMat3();
    Vec3 scale;
    scale.x = axis[0].Length();
    scale.y = axis[1].Length();
    scale.z = axis[2].Length();
    axis.OrthoNormalizeSelf();

    transform->props->Set("origin", localMatrix.ToTranslationVec3());
    transform->props->Set("scale", scale);
    transform->props->Set("angles", axis.ToAngles());
}

ObjectRef Entity::GetParentRef() const {
    Entity *parentEntity = node.GetParent();
    Guid parentEntityGuid = parentEntity ? parentEntity->GetGuid() : Guid::zero;
    return ObjectRef(Entity::metaObject, parentEntityGuid);
}

void Entity::SetParentRef(const ObjectRef &parenteRef) {
    SetParentGuid(parenteRef.objectGuid);
}

Guid Entity::GetPrefabSource() const {
    return prefabSourceGuid;
}

void Entity::SetPrefabSource(const Guid &prefabSourceGuid) {
    this->prefabSourceGuid = prefabSourceGuid;
}

ObjectRef Entity::GetPrefabSourceRef() const {
    return ObjectRef(Entity::metaObject, prefabSourceGuid);
}

void Entity::SetPrefabSourceRef(const ObjectRef &prefabSourceRef) {
    prefabSourceGuid = prefabSourceRef.objectGuid;
}

void Entity::SetFrozen(bool frozen) {
    this->frozen = frozen;

    ComRenderable *renderable = GetComponent<ComRenderable>();
    if (renderable) {
        renderable->props->Set("skipSelection", frozen);
    }
}

Entity *Entity::CreateEntity(Json::Value &entityValue) {
    Guid entityGuid = Guid::FromString(entityValue.get("guid", Guid::zero.ToString()).asCString());
    if (entityGuid.IsZero()) {
        entityValue["guid"] = Guid::CreateGuid().ToString();
    }

    entityGuid = Guid::FromString(entityValue["guid"].asCString());

    Entity *entity = static_cast<Entity *>(Entity::metaObject.CreateInstance(entityGuid));
    entity->props->Init(entityValue);

    Json::Value &componentsValue = entityValue["components"];

    for (int i = 0; i < componentsValue.size(); i++) {
        Json::Value &componentValue = componentsValue[i];

        const char *classname = componentValue["classname"].asCString();
        MetaObject *metaComponent = Object::GetMetaObject(classname);

        if (metaComponent) {
            if (metaComponent->IsTypeOf(Component::metaObject)) {
                Guid componentGuid = Guid::FromString(componentValue.get("guid", Guid::zero.ToString()).asCString());
                if (componentGuid.IsZero()) {
                    componentValue["guid"] = Guid::CreateGuid().ToString();
                }

                componentGuid = Guid::FromString(componentValue["guid"].asCString());

                Component *component = static_cast<Component *>(metaComponent->CreateInstance(componentGuid));

                // Initialize property infos for script a component
                if (metaComponent->IsTypeOf(ComScript::metaObject)) {
                    ComScript *scriptComponent = component->Cast<ComScript>();
                    scriptComponent->InitPropertyInfo(componentValue);
                }

                component->props->Init(componentValue);

                entity->AddComponent(component);
            } else {
                BE_WARNLOG(L"'%hs' is not a component class\n", classname);
            }
        } else {
            BE_WARNLOG(L"Unknown component class '%hs'\n", classname);
        }
    }

    return entity;
}

Json::Value Entity::CloneEntityValue(const Json::Value &entityValue, HashTable<Guid, Guid> &oldToNewGuidMap) {
    Json::Value newEntityValue = entityValue;

    Guid oldEntityGuid = Guid::FromString(entityValue["guid"].asCString());
    Guid newEntityGuid = Guid::CreateGuid();

    oldToNewGuidMap.Set(oldEntityGuid, newEntityGuid);

    newEntityValue["guid"] = newEntityGuid.ToString();

    Json::Value &componentsValue = newEntityValue["components"];

    for (int i = 0; i < componentsValue.size(); i++) {
        Guid oldComponentGuid = Guid::FromString(componentsValue[i]["guid"].asCString());
        Guid newComponentGuid = Guid::CreateGuid();

        oldToNewGuidMap.Set(oldComponentGuid, newComponentGuid);

        componentsValue[i]["guid"] = newComponentGuid.ToString();
    }

    return newEntityValue;
}

void Entity::RemapGuids(EntityPtrArray &entities, const HashTable<Guid, Guid> &remapGuidMap) {
    PropertyInfo propInfo;
    Guid toGuid;

    for (int entityIndex = 0; entityIndex < entities.Count(); entityIndex++) {
        Entity *entity = entities[entityIndex];

        for (int propIndex = 0; propIndex < entity->props->Count(); propIndex++) {
            if (entity->props->GetInfo(propIndex, propInfo)) {
                if (propInfo.GetType() == PropertyInfo::ObjectType) {
                    const Guid fromGuid = entity->props->Get(propInfo.GetName()).As<Guid>();

                    if (remapGuidMap.Get(fromGuid, &toGuid)) {
                        entity->props->Set(propInfo.GetName(), toGuid);
                    }
                }
            }
        }

        for (int componentIndex = 0; componentIndex < entity->NumComponents(); componentIndex++) {
            Component *component = entity->GetComponent(componentIndex);

            for (int propIndex = 0; propIndex < component->props->Count(); propIndex++) {
                if (component->props->GetInfo(propIndex, propInfo)) {
                    if (propInfo.GetType() == PropertyInfo::ObjectType) {
                        const Guid fromGuid = component->props->Get(propInfo.GetName()).As<Guid>();

                        if (remapGuidMap.Get(fromGuid, &toGuid)) {
                            component->props->Set(propInfo.GetName(), toGuid);
                        }
                    }
                }
            }
        }
    }
}

BE_NAMESPACE_END
