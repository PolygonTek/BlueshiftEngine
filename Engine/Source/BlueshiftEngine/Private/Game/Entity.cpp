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
    PROPERTY_OBJECT("guid", "GUID", "entity GUID", Guid::zero.ToString(), Entity::metaObject, PropertySpec::ReadWrite),
    PROPERTY_OBJECT("parent", "Parent Entity", "parent entity", Guid::zero.ToString(), Entity::metaObject, PropertySpec::ReadWrite),
    PROPERTY_BOOL("isPrefabParent", "Is prefab Parent", "is prefab parent ?", "false", PropertySpec::ReadWrite),
    PROPERTY_OBJECT("prefabParent", "Prefab Parent", "prefab parent entity", Guid::zero.ToString(), Entity::metaObject, PropertySpec::ReadWrite),
    PROPERTY_STRING("name", "Name", "entity name", "", PropertySpec::ReadWrite),
    PROPERTY_STRING("tag", "Tag", "Tag", "Untagged", PropertySpec::ReadWrite),
    PROPERTY_INT("layer", "Layer", "Layer", "0", PropertySpec::ReadWrite),
    PROPERTY_BOOL("frozen", "Frozen", "is frozen ?", "false", PropertySpec::ReadWrite),
END_PROPERTIES

Entity::Entity() {
    gameWorld = nullptr;
    entityNum = GameWorld::BadEntityNum;
    node.SetOwner(this);
    frozen = false;
    initialized = false;

    Connect(&Properties::SIG_PropertyChanged, this, (SignalCallback)&Entity::PropertyChanged);
}

Entity::~Entity() {
    Purge();
}

Entity *Entity::CreateEntity(Json::Value &entityValue) {
    Guid entityGuid = Guid::ParseString(entityValue.get("guid", Guid::zero.ToString()).asCString());
    if (entityGuid.IsZero()) {
        entityValue["guid"] = Guid::CreateGuid().ToString();
    }

    entityGuid = Guid::ParseString(entityValue["guid"].asCString());

    Entity *entity = static_cast<Entity *>(Entity::metaObject.CreateInstance(entityGuid));
    entity->props->Init(entityValue);

    entity->name = entity->props->Get("name").As<Str>();
    entity->tag = entity->props->Get("tag").As<Str>();
    
    Json::Value &componentsValue = entityValue["components"];

    for (int i = 0; i < componentsValue.size(); i++) {
        Json::Value &componentValue = componentsValue[i];

        const char *classname = componentValue["classname"].asCString();
        MetaObject *metaComponent = Object::GetMetaObject(classname);

        if (metaComponent) {
            if (metaComponent->IsTypeOf(Component::metaObject)) {
                Guid componentGuid = Guid::ParseString(componentValue.get("guid", Guid::zero.ToString()).asCString());
                if (componentGuid.IsZero()) {
                    componentValue["guid"] = Guid::CreateGuid().ToString();
                }

                componentGuid = Guid::ParseString(componentValue["guid"].asCString());

                Component *component = static_cast<Component *>(metaComponent->CreateInstance(componentGuid));

                if (metaComponent->IsTypeOf(ComScript::metaObject)) {
                    ComScript *scriptComponent = component->Cast<ComScript>();
                    scriptComponent->InitPropertySpec(componentValue);
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

    Guid oldEntityGuid = Guid::ParseString(entityValue["guid"].asCString());
    Guid newEntityGuid = Guid::CreateGuid();

    //BE_LOG(L"NewGUID %hs: %hs -> %hs\n", entityValue["name"].asCString(), oldEntityGuid.ToString(), newEntityGuid.ToString());

    oldToNewGuidMap.Set(oldEntityGuid, newEntityGuid);

    newEntityValue["guid"] = newEntityGuid.ToString();

    Json::Value &componentsValue = newEntityValue["components"];

    for (int i = 0; i < componentsValue.size(); i++) {
        Guid oldComponentGuid = Guid::ParseString(componentsValue[i]["guid"].asCString());
        Guid newComponentGuid = Guid::CreateGuid();

        oldToNewGuidMap.Set(oldComponentGuid, newComponentGuid);

        componentsValue[i]["guid"] = newComponentGuid.ToString();
    }

    return newEntityValue;
}

void Entity::RemapGuids(EntityPtrArray &entities, const HashTable<Guid, Guid> &guidMap) {
    for (int i = 0; i < entities.Count(); i++) {
        Entity *ent = entities[i];

        for (int propIndex = 0; propIndex < ent->props->Count(); propIndex++) {
            const PropertySpec *spec = ent->props->GetSpec(propIndex);

            if (spec->GetType() == PropertySpec::ObjectType) {
                const Guid objectGuid = ent->props->Get(spec->GetName()).As<Guid>();
                Guid mappedGuid;

                if (guidMap.Get(objectGuid, &mappedGuid)) {
                    //BE_LOG(L"Remap %hs %hs: %hs -> %hs\n", ent->GetName(), spec->GetName(), objectGuid.ToString(), mappedGuidPtr->ToString());

                    ent->props->Set(spec->GetName(), mappedGuid);
                }
            }
        }

        for (int componentIndex = 0; componentIndex < ent->NumComponents(); componentIndex++) {
            Component *component = ent->GetComponent(componentIndex);

            for (int propIndex = 0; propIndex < component->props->Count(); propIndex++) {
                const PropertySpec *spec = component->props->GetSpec(propIndex);

                if (spec->GetType() == PropertySpec::ObjectType) {
                    const Guid objectGuid = component->props->Get(spec->GetName()).As<Guid>();
                    Guid mappedGuid;

                    if (guidMap.Get(objectGuid, &mappedGuid)) {
                        component->props->Set(spec->GetName(), mappedGuid);
                    }
                }
            }
        }
    }
}

int Entity::GetSpawnId() const {
    return gameWorld->GetEntitySpawnId(this); 
}

bool Entity::IsPrefabParent() const {
    bool isPrefabParent = props->Get("isPrefabParent").As<bool>();
    return isPrefabParent;
}

bool Entity::IsPrefabInstance() const {
    Guid prefabParentGuid = props->Get("prefabParent").As<Guid>();
    return !prefabParentGuid.IsZero();
}

Entity *Entity::GetPrefabParent() const {
    Guid prefabParentGuid = props->Get("prefabParent").As<Guid>();
    Object *prefabParentObj = Entity::FindInstance(prefabParentGuid);
    Entity *prefabParent = prefabParentObj ? prefabParentObj->Cast<Entity>() : nullptr;
    return prefabParent;
}

bool Entity::HasComponent(const MetaObject &type) const {
    if (GetComponent(type)) {
        return true;
    }
    return false;
}

Component *Entity::GetConflictingComponent(const MetaObject &type) const {
    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component->IsConflictComponent(type)) {
            return component;
        }
    }

    return nullptr;
}

int Entity::GetComponentIndex(const Component *component) const {
    return components.FindIndex(const_cast<Component *>(component));
}

Component *Entity::GetComponent(const MetaObject &type) const {
    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component->GetMetaObject()->IsTypeOf(type)) {
            return component;
        }
    }

    return nullptr;
}

ComponentPtrArray Entity::GetComponents(const MetaObject &type) const {
    ComponentPtrArray subComponents;

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];

        if (component->GetMetaObject()->IsTypeOf(type)) {
            subComponents.Append(component);
        }
    }

    return subComponents;
}

ComTransform *Entity::GetTransform() const {
    ComTransform *transform = static_cast<ComTransform *>(GetComponent(0));
    assert(transform);
    return transform;
}

void Entity::AddComponent(Component *component) {
    InsertComponent(component, components.Count());
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
    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component) {
            if (component->HasRenderEntity(renderEntityHandle)) {
                return true;
            }
        }
    }
    return false;
}

void Entity::SetParent(Entity *parentEntity) {
    props->Set("parent", parentEntity->GetGuid());
}

void Entity::PurgeJointComponents() {
    for (int i = components.Count() - 1; i >= 0; i--) {
        Component *component = components[i];
        if (component && component->IsTypeOf<ComJoint>()) {
            component->Purge();
        }
    }
}

void Entity::Purge() {
    for (int i = components.Count() - 1; i >= 0; i--) {
        Component *component = components[i];
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

    for (int i = 0; i < components.Count(); i++) {
        components[i]->entity = nullptr;
        Component::DestroyInstanceImmediate(components[i]);
    }

    components.Clear();
    
    Object::Event_ImmediateDestroy();
}

void Entity::InitHierarchy() {
    Entity *parent = nullptr;

    const Guid parentGuid = props->Get("parent").As<Guid>();
    if (!parentGuid.IsZero()) {
        Object *parentObj = Entity::FindInstance(parentGuid);
        parent = parentObj ? parentObj->Cast<Entity>() : nullptr;
        if (!parent) {
            BE_WARNLOG(L"Couldn't find parent entity %hs of %hs\n", parentGuid.ToString(), name.c_str());
        }
    }

    if (parent) {
        node.SetParent(parent->node);
    } 
}

void Entity::Init() {
    if (!gameWorld) {
        return;
    }

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component && !component->IsInitalized()) {
            component->Init();
        }
    }

    frozen = props->Get("frozen").As<bool>();

    ComRenderable *renderable = GetComponent<ComRenderable>();
    if (renderable) {
        renderable->props->Set("skipSelection", frozen);
    }

    initialized = true;
}

void Entity::Awake() {
    if (!gameWorld) {
        return;
    }

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component) {
            component->Awake();
        }
    }
}

void Entity::Start() {
    if (!gameWorld) {
        return;
    }

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component) {
            component->Start();
        }
    }
}

void Entity::Update() {
    if (!gameWorld) {
        return;
    }

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component && component->IsEnabled()) {
            component->Update();
        }
    }
}

void Entity::LateUpdate() {
    if (!gameWorld) {
        return;
    }

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component && component->IsEnabled()) {
            component->LateUpdate();
        }
    }
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

void Entity::Serialize(Json::Value &jsonEntity) const {
    Json::Value jsonComponents;

    props->Serialize(jsonEntity);

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component) {
            Json::Value jsonComponent;
            component->props->Serialize(jsonComponent);

            jsonComponents.append(jsonComponent);
        }
    }

    jsonEntity["components"] = jsonComponents;
}

bool Entity::IsActiveSelf() const {
    for (int i = 1; i < components.Count(); i++) {
        Component *component = components[i];
        if (component) {
            if (component->IsEnabled()) {
                return true;
            }
        }
    }

    return false;
}

void Entity::SetActive(bool active) {
    for (int i = 1; i < components.Count(); i++) {
        Component *component = components[i];
        if (component) {
            component->Enable(active);
        }
    }

    for (Entity *childEnt = node.GetChild(); childEnt; childEnt = childEnt->node.GetNextSibling()) {
        childEnt->SetActive(active);
    }
}

const AABB Entity::GetAABB() const {
    AABB aabb;
    aabb.Clear();

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
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
    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component && component->IsEnabled()) {
            component->DrawGizmos(sceneView, selected);
        }
    }
}

bool Entity::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    float s = lastScale;

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
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
        name = props->Get("name").As<Str>();
        GetGameWorld()->OnEntityNameChanged(this);
        return;
    }

    if (!Str::Cmp(propName, "tag")) {
        tag = props->Get("tag").As<Str>();
        GetGameWorld()->OnEntityTagChanged(this);
        return;
    }

    if (!Str::Cmp(propName, "layer")) {
        EmitSignal(&SIG_LayerChanged, this);
        return;
    }

    if (!Str::Cmp(propName, "parent")) {
        Guid parentGuid = props->Get("parent").As<Guid>();
        Object *parentObj = Entity::FindInstance(parentGuid);
        Entity *parent = parentObj ? parentObj->Cast<Entity>() : nullptr;
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
        return;
    }

    if (!Str::Cmp(propName, "frozen")) {
        frozen = props->Get("frozen").As<bool>();

        ComRenderable *renderable = GetComponent<ComRenderable>();
        if (renderable) {
            renderable->props->Set("skipSelection", frozen);
        }
        return;
    }
}

BE_NAMESPACE_END
