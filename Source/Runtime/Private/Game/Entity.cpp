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
#include "Components/ComRenderable.h"
#include "Components/ComScript.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

const SignalDef Entity::SIG_ActiveChanged("Entity::ActiveChanged", "ai");
const SignalDef Entity::SIG_ActiveInHierarchyChanged("Entity::ActiveInHierachyChanged", "ai");
const SignalDef Entity::SIG_NameChanged("Entity::NameChanged", "as");
const SignalDef Entity::SIG_LayerChanged("Entity::LayerChanged", "a");
const SignalDef Entity::SIG_FrozenChanged("Entity::FrozenChanged", "ai");
const SignalDef Entity::SIG_ParentChanged("Entity::ParentChanged", "aa");
const SignalDef Entity::SIG_ComponentInserted("Entity::ComponentInserted", "ai");
const SignalDef Entity::SIG_ComponentRemoved("Entity::ComponentRemoved", "a");
const SignalDef Entity::SIG_ComponentSwapped("Entity::ComponentSwapped", "ii");

OBJECT_DECLARATION("Entity", Entity, Object)
BEGIN_EVENTS(Entity)
END_EVENTS

void Entity::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("parent", "Parent", Guid, GetParentGuid, SetParentGuid, Guid::zero,
        "Parent Entity", PropertyInfo::EditorFlag).SetMetaObject(&Entity::metaObject);
    REGISTER_PROPERTY("prefab", "Prefab", bool, prefab, false,
        "Is prefab ?", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("prefabSource", "Prefab Source", Guid, GetPrefabSourceGuid, SetPrefabSourceGuid, Guid::zero,
        "", PropertyInfo::EditorFlag).SetMetaObject(&Entity::metaObject);
    REGISTER_MIXED_ACCESSOR_PROPERTY("name", "Name", Str, GetName, SetName, "Entity",
        "", PropertyInfo::EditorFlag);
    REGISTER_MIXED_ACCESSOR_PROPERTY("tag", "Tag", Str, GetTag, SetTag, "Untagged",
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("layer", "Layer", int, GetLayer, SetLayer, 0,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("active", "Active", bool, IsActiveSelf, SetActive, true,
        "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("activeInHierarchy", "Active In Hierarchy", bool, activeInHierarchy, true,
        "", PropertyInfo::EditorFlag);
    REGISTER_ACCESSOR_PROPERTY("frozen", "Frozen", bool, IsFrozen, SetFrozen, false,
        "", PropertyInfo::EditorFlag);
    REGISTER_PROPERTY("static", "Static", bool, isStatic, false,
        "", PropertyInfo::EditorFlag);
}

Entity::Entity() {
    gameWorld = nullptr;
    entityNum = GameWorld::BadEntityNum;
    node.SetOwner(this);
    layer = 0;
    frozen = false;
    isStatic = false;
    prefab = false;
    prefabSourceGuid = Guid::zero;
    activeSelf = true;
    activeInHierarchy = true;
    awaked = false;
    started = false;
    initialized = false;
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

    awaked = false;
    started = false;
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
        renderable->SetProperty("skipSelection", frozen);
    }
}

void Entity::Awake() {
    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component) {
            component->Awake();
        }
    }

    awaked = true;
}

void Entity::Start() {
    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        ComScript *scriptComponent = components[componentIndex]->Cast<ComScript>();

        if (scriptComponent && scriptComponent->IsActiveInHierarchy()) {
            scriptComponent->Start();
        }
    }

    started = true;
}

void Entity::FixedUpdate(float timeStep) {
    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        ComScript *scriptComponent = components[componentIndex]->Cast<ComScript>();

        if (scriptComponent && scriptComponent->IsActiveInHierarchy()) {
            scriptComponent->FixedUpdate(timeStep);
        }
    }
}

void Entity::FixedLateUpdate(float timeStep) {
    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        ComScript *scriptComponent = components[componentIndex]->Cast<ComScript>();

        if (scriptComponent && scriptComponent->IsActiveInHierarchy()) {
            scriptComponent->FixedLateUpdate(timeStep);
        }
    }
}

void Entity::Update() {
    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component && component->IsActiveInHierarchy()) {
            component->Update();
        }
    }
}

void Entity::LateUpdate() {
    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        ComScript *scriptComponent = components[componentIndex]->Cast<ComScript>();

        if (scriptComponent && scriptComponent->IsActiveInHierarchy()) {
            scriptComponent->LateUpdate();
        }
    }
}

ComTransform *Entity::GetTransform() const {
    ComTransform *transform = static_cast<ComTransform *>(GetComponent(0));
    assert(transform);
    return transform;
}

Component *Entity::AddNewComponent(const MetaObject *type) {
    if (!type->IsTypeOf(Component::metaObject)) {
        BE_ERRLOG(L"Entity::AddNewComponent: %hs is not component type\n", type->ClassName());
        return nullptr;
    }
    Component *component = (Component *)type->CreateInstance();
    component->SetEntity(this);
    component->Deserialize(Json::Value());

    AddComponent(component);

    return component;
}

void Entity::InsertComponent(Component *component, int index) {
    component->SetEntity(this);

    components.Insert(component, index);

    if (initialized) {
        component->Init();
    }

    if (awaked) {
        component->Awake();
    }

    EmitSignal(&SIG_ComponentInserted, component, index);
}

bool Entity::RemoveComponent(Component *component) {
    return components.Remove(component);
}

bool Entity::SwapComponent(int fromIndex, int toIndex) {
    Clamp(fromIndex, 1, components.Count());
    Clamp(toIndex, 1, components.Count());

    if (fromIndex != toIndex) {
        Swap(components[fromIndex], components[toIndex]);

        EmitSignal(&SIG_ComponentSwapped, fromIndex, toIndex);
        return true;
    }
    return false;
}

bool Entity::HasChildren() const {
    return node.GetChild() ? true : false;
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

        Entity *foundEntity = child->FindChild(name);
        if (foundEntity) {
            return foundEntity;
        }
    }
    return nullptr;
}

Component *Entity::GetComponent(const MetaObject *type) const {
    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];
        if (component->GetMetaObject()->IsTypeOf(*type)) {
            return component;
        }
    }

    return nullptr;
}

ComponentPtrArray Entity::GetComponents(const MetaObject *type) const {
    ComponentPtrArray subComponents;

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];

        if (component->GetMetaObject()->IsTypeOf(*type)) {
            subComponents.Append(component);
        }
    }

    return subComponents;
}

ComponentPtrArray Entity::GetComponentsInChildren(const MetaObject *type) const {
    ComponentPtrArray subComponents;

    for (int i = 0; i < components.Count(); i++) {
        Component *component = components[i];

        if (component->GetMetaObject()->IsTypeOf(*type)) {
            subComponents.Append(component);
        }
    }

    for (Entity *child = node.GetChild(); child; child = child->node.GetNextSibling()) {
        subComponents.AppendArray(child->GetComponentsInChildren(type));
    }

    return subComponents;
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

void Entity::OnApplicationResize(int width, int height) {
    ComponentPtrArray scriptComponents = GetComponents(&ComScript::metaObject);
    for (int i = 0; i < scriptComponents.Count(); i++) {
        ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();

        scriptComponent->OnApplicationResize(width, height);
    }
}


void Entity::OnApplicationTerminate() {
    ComponentPtrArray scriptComponents = GetComponents(&ComScript::metaObject);
    for (int i = 0; i < scriptComponents.Count(); i++) {
        ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();
        
        scriptComponent->OnApplicationTerminate();
    }
}

void Entity::OnApplicationPause(bool pause) {
    ComponentPtrArray scriptComponents = GetComponents(&ComScript::metaObject);
    for (int i = 0; i < scriptComponents.Count(); i++) {
        ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();
        
        scriptComponent->OnApplicationPause(pause);
    }
}

void Entity::Serialize(Json::Value &value) const {
    Json::Value componentsValue;

    Serializable::Serialize(value);

    for (int componentIndex = 0; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component) {
            Json::Value componentValue;
            component->Serialize(componentValue);

            componentsValue.append(componentValue);
        }
    }

    value["components"] = componentsValue;
}

void Entity::Deserialize(const Json::Value &entityValue) {
    Serializable::Deserialize(entityValue);

    const Json::Value &componentsValue = entityValue["components"];

    for (int i = 0; i < componentsValue.size(); i++) {
        const Json::Value &componentValue = componentsValue[i];

        const char *classname = componentValue["classname"].asCString();
        MetaObject *metaComponent = Object::FindMetaObject(classname);

        if (metaComponent) {
            if (metaComponent->IsTypeOf(Component::metaObject)) {
                Guid componentGuid = Guid::FromString(componentValue.get("guid", Guid::zero.ToString()).asCString());
                if (componentGuid.IsZero()) {
                    componentGuid = Guid::CreateGuid();
                }

                Component *component = static_cast<Component *>(metaComponent->CreateInstance(componentGuid));
                component->SetEntity(this);
                component->Deserialize(componentValue);

                AddComponent(component);
            } else {
                BE_WARNLOG(L"'%hs' is not a component class\n", classname);
            }
        } else {
            BE_WARNLOG(L"Unknown component class '%hs'\n", classname);
        }
    }
}

void Entity::SerializeHierarchy(const Entity *entity, Json::Value &entitiesValue) {
    Json::Value entityValue;

    entity->Serialize(entityValue);

    entitiesValue.append(entityValue);

    for (Entity *child = entity->GetNode().GetChild(); child; child = child->GetNode().GetNextSibling()) {
        Entity::SerializeHierarchy(child, entitiesValue);
    }
}

void Entity::SetActive(bool active) {
    if (active == activeSelf) {
        return;
    }

    activeSelf = active;

    EmitSignal(&SIG_ActiveChanged, this, active);

    const Entity *parentEntity = node.GetParent();

    if (!parentEntity || parentEntity->activeInHierarchy) {
        SetActiveInHierarchy(active);
    }
}

void Entity::SetActiveInHierarchy(bool active) {
    if (activeInHierarchy == active) {
        return;
    }

    activeInHierarchy = active;

    EmitSignal(&SIG_ActiveInHierarchyChanged, this, active ? 1 : 0);

    for (int componentIndex = 1; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component->IsEnabled()) {
            if (activeInHierarchy) {
                component->OnActive();
            } else {
                component->OnInactive();
            }
        }
    }

    for (Entity *childEnt = node.GetChild(); childEnt; childEnt = childEnt->node.GetNextSibling()) {
        childEnt->SetActiveInHierarchy(active && childEnt->activeSelf);
    }
}

const AABB Entity::GetLocalAABB(bool includingChildren) const {
    AABB outAabb;
    outAabb.SetZero();

    for (int componentIndex = 1; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component) {
            outAabb.AddAABB(component->GetAABB());
        }
    }

    if (includingChildren) {
        Mat3x4 rootMatrixInverse = GetTransform()->GetMatrixNoScale().Inverse();

        Array<Entity *> children;
        GetChildren(children);

        for (int childIndex = 0; childIndex < children.Count(); childIndex++) {
            const Entity *child = children[childIndex];

            Mat3x4 localMatrix = rootMatrixInverse * child->GetTransform()->GetMatrixNoScale();
            Vec3 translation, scale;
            Mat3 rotation;
            localMatrix.GetTRS(translation, rotation, scale);

            AABB childLocalAabb;
            childLocalAabb = child->GetLocalAABB();
            if (childLocalAabb != AABB::zero) {
                childLocalAabb.SetFromTransformedAABB(childLocalAabb, translation, rotation);

                outAabb += childLocalAabb;
            }
        }
    }
    return outAabb;
}

const AABB Entity::GetWorldAABB(bool includingChildren) const {
    const ComTransform *transform = GetTransform();

    AABB worldAabb;
    worldAabb.SetFromTransformedAABB(GetLocalAABB(includingChildren), transform->GetOrigin(), transform->GetAxis());
    return worldAabb;
}

const Vec3 Entity::GetWorldPosition(WorldPosTrait posTrait, bool includingChildren) const {
    Vec3 vec;

    if (posTrait == Pivot) {
        vec = GetTransform()->GetOrigin();
    } else {
        AABB aabb = GetWorldAABB(includingChildren);

        if (posTrait == Minimum) {
            vec = aabb.b[0];
        } else if (posTrait == Maximum) {
            vec = aabb.b[1];
        } else if (posTrait == Center) {
            vec = aabb.Center();
        } else {
            assert(0);
        }
    }

    return vec;
}

void Entity::DrawGizmos(const RenderView::State &viewState, bool selected) {
    for (int componentIndex = 1; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component && component->IsActiveInHierarchy()) {
            component->DrawGizmos(viewState, selected);
        }
    }
}

bool Entity::RayIntersection(const Vec3 &start, const Vec3 &dir, bool backFaceCull, float &lastScale) const {
    float s = lastScale;

    for (int componentIndex = 1; componentIndex < components.Count(); componentIndex++) {
        Component *component = components[componentIndex];

        if (component && component->IsActiveInHierarchy()) {
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

void Entity::SetName(const Str &name) {
    this->name = name;

    if (initialized) {
        GetGameWorld()->OnEntityNameChanged(this);

        EmitSignal(&SIG_NameChanged, this, name);
    }
}

void Entity::SetTag(const Str &tag) {
    this->tag = tag;

    if (initialized) {
        GetGameWorld()->OnEntityTagChanged(this);
    }
}

void Entity::SetLayer(int layer) {
    this->layer = layer;

    if (initialized) {
        EmitSignal(&SIG_LayerChanged, this);
    }
}

void Entity::SetParent(Entity *parentEntity) {
    SetParentGuid(parentEntity->GetGuid());
}

Guid Entity::GetParentGuid() const {
    Entity *parentEntity = node.GetParent();
    if (parentEntity) {
        return parentEntity->GetGuid();
    }
    return Guid();
}

void Entity::SetParentGuid(const Guid &parentGuid) {
    Object *parentObject = Entity::FindInstance(parentGuid);
    Entity *parentEntity = parentObject ? parentObject->Cast<Entity>() : nullptr;
    
    if (parentEntity) {
        node.SetParent(parentEntity->node);
    } else {
        if (gameWorld) {
            assert(sceneIndex >= 0);
            node.SetParent(gameWorld->GetScenes()[sceneIndex].root);
        }
    }

    if (initialized) {
        EmitSignal(&SIG_ParentChanged, this, parentEntity);
    }
}

Guid Entity::GetPrefabSourceGuid() const {
    return prefabSourceGuid;
}

void Entity::SetPrefabSourceGuid(const Guid &prefabSourceGuid) {
    this->prefabSourceGuid = prefabSourceGuid;
}

void Entity::SetStatic(bool isStatic) {
    this->isStatic = isStatic;
}

void Entity::SetFrozen(bool frozen) {
    this->frozen = frozen;

    if (initialized) {
        ComRenderable *renderable = GetComponent<ComRenderable>();
        if (renderable) {
            renderable->SetProperty("skipSelection", frozen);
        }

        EmitSignal(&SIG_FrozenChanged, this, frozen ? 1 : 0);
    }
}

Entity *Entity::CreateEntity(Json::Value &entityValue, GameWorld *gameWorld, int sceneIndex) {
    Guid entityGuid = Guid::FromString(entityValue.get("guid", Guid::zero.ToString()).asCString());
    if (entityGuid.IsZero()) {
        entityGuid = Guid::CreateGuid();
    }

    Entity *entity = static_cast<Entity *>(Entity::metaObject.CreateInstance(entityGuid));

    entity->gameWorld = gameWorld;
    entity->sceneIndex = sceneIndex;
    entity->Deserialize(entityValue);

    return entity;
}

Json::Value Entity::CloneEntityValue(const Json::Value &entityValue, HashTable<Guid, Guid> &oldToNewGuidMap) {
    // Copy entity JSON value
    Json::Value newEntityValue = entityValue;

    Guid oldEntityGuid = Guid::FromString(entityValue["guid"].asCString());
    Guid newEntityGuid = Guid::CreateGuid();

    // Replace entity GUID to the new one
    newEntityValue["guid"] = newEntityGuid.ToString();

    // Mark in the GUID hash table
    oldToNewGuidMap.Set(oldEntityGuid, newEntityGuid);

    Json::Value &componentsValue = newEntityValue["components"];

    for (int i = 0; i < componentsValue.size(); i++) {
        Guid oldComponentGuid = Guid::FromString(componentsValue[i]["guid"].asCString());
        Guid newComponentGuid = Guid::CreateGuid();

        // Replace component GUID to the new one
        componentsValue[i]["guid"] = newComponentGuid.ToString();

        // Mark in the GUID hash table
        oldToNewGuidMap.Set(oldComponentGuid, newComponentGuid);
    }

    return newEntityValue;
}

Json::Value Entity::CloneEntitiesValue(const Json::Value &entitiesValue, HashTable<Guid, Guid> &oldToNewGuidMap) {
    Json::Value clonedEntitiesValue;

    for (int i = 0; i < entitiesValue.size(); i++) {
        clonedEntitiesValue.append(CloneEntityValue(entitiesValue[i], oldToNewGuidMap));
    }
    return clonedEntitiesValue;
}

void Entity::RemapGuids(Entity *entity, const HashTable<Guid, Guid> &remapGuidMap) {
    PropertyInfo propInfo;
    Guid toGuid;

    Array<PropertyInfo> propertyInfoList;
    entity->GetPropertyInfoList(propertyInfoList);

    for (int propIndex = 0; propIndex < propertyInfoList.Count(); propIndex++) {
        const auto &propInfo = propertyInfoList[propIndex];
            
        if (propInfo.GetType() == Variant::GuidType) {
            if (propInfo.GetFlags() & PropertyInfo::ArrayFlag) {
                for (int arrayIndex = 0; arrayIndex < entity->GetPropertyArrayCount(propInfo.GetName()); arrayIndex++) {
                    const Guid fromGuid = entity->GetArrayProperty(propIndex, arrayIndex).As<Guid>();

                    if (remapGuidMap.Get(fromGuid, &toGuid)) {
                        entity->SetArrayProperty(propIndex, arrayIndex, toGuid);
                    }
                }
            } else {
                const Guid fromGuid = entity->GetProperty(propIndex).As<Guid>();

                if (remapGuidMap.Get(fromGuid, &toGuid)) {
                    entity->SetProperty(propIndex, toGuid);
                }
            }
        }
    }

    for (int componentIndex = 0; componentIndex < entity->NumComponents(); componentIndex++) {
        Component *component = entity->GetComponent(componentIndex);

        Array<PropertyInfo> propertyInfoList;
        component->GetPropertyInfoList(propertyInfoList);

        for (int propIndex = 0; propIndex < propertyInfoList.Count(); propIndex++) {
            const auto &propInfo = propertyInfoList[propIndex];

            if (propInfo.GetType() == Variant::GuidType) {
                if (propInfo.GetFlags() & PropertyInfo::ArrayFlag) {
                    for (int arrayIndex = 0; arrayIndex < component->GetPropertyArrayCount(propInfo.GetName()); arrayIndex++) {
                        const Guid fromGuid = component->GetArrayProperty(propIndex, arrayIndex).As<Guid>();

                        if (remapGuidMap.Get(fromGuid, &toGuid)) {
                            component->SetArrayProperty(propIndex, arrayIndex, toGuid);
                        }
                    }
                } else {
                    const Guid fromGuid = component->GetProperty(propIndex).As<Guid>();

                    if (remapGuidMap.Get(fromGuid, &toGuid)) {
                        component->SetProperty(propIndex, toGuid);
                    }
                }
            }
        }
    }
}

BE_NAMESPACE_END
