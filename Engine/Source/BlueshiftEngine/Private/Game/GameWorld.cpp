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
#include "Platform/PlatformTime.h"
#include "Render/Render.h"
#include "Physics/Collider.h"
#include "Physics/Physics.h"
#include "Input/InputSystem.h"
#include "Sound/SoundSystem.h"
#include "AnimController/AnimController.h"
#include "Components/Component.h"
#include "Components/ComTransform.h"
#include "Components/ComCamera.h"
#include "Components/ComRigidBody.h"
#include "Components/ComSensor.h"
#include "Game/Entity.h"
#include "Game/MapRenderSettings.h"
#include "Game/GameWorld.h"
#include "Game/GameSettings/TagLayerSettings.h"
#include "Game/GameSettings/PhysicsSettings.h"
#include "Containers/StaticArray.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

const EventDef EV_RestartGame("restartGame", false, "s");

const SignalDef GameWorld::SIG_EntityRegistered("entityRegistered", "a");
const SignalDef GameWorld::SIG_EntityUnregistered("entityUnregistered", "a");

OBJECT_DECLARATION("Game World", GameWorld, Object)
BEGIN_EVENTS(GameWorld)
    EVENT(EV_RestartGame, GameWorld::Event_RestartGame),
END_EVENTS
BEGIN_PROPERTIES(GameWorld)
END_PROPERTIES

GameWorld::GameWorld() {
    memset(entities, 0, sizeof(entities));

    tagLayerSettings = nullptr;
    physicsSettings = nullptr;

    // Create render settings
    mapRenderSettings = static_cast<MapRenderSettings *>(MapRenderSettings::metaObject.CreateInstance());
    mapRenderSettings->gameWorld = this;

    // Create render world
    renderWorld = renderSystem.AllocRenderWorld();

    // Create physics world
    physicsWorld = physicsSystem.AllocPhysicsWorld();

    gameStarted = false;

    timeScale = 1.0f;

    Reset();
}

GameWorld::~GameWorld() {
    ClearAllEntities();

    if (tagLayerSettings) {
        TagLayerSettings::DestroyInstanceImmediate(tagLayerSettings);
    }

    if (physicsSettings) {
        PhysicsSettings::DestroyInstanceImmediate(physicsSettings);
    }

    if (mapRenderSettings) {
        MapRenderSettings::DestroyInstanceImmediate(mapRenderSettings);
    }

    // Free render world
    renderSystem.FreeRenderWorld(renderWorld);

    // Free physics world
    physicsSystem.FreePhysicsWorld(physicsWorld);
}

void GameWorld::Reset() {
    ClearAllEntities();

    time = 0;
    prevTime = 0;

    renderWorld->ClearDebugPrimitives(0);
    renderWorld->ClearDebugText(0);
}

int GameWorld::GetDeltaTime() const {
    int dt = time - prevTime;
    return dt;
}

void GameWorld::ClearAllEntities() {
    // list entities in depth first order
    EntityPtrArray entityList;
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        entityList.Append(ent);
    }

    // delete entities in reverse depth first order
    for (int i = entityList.Count() - 1; i >= 0; i--) {
        Entity::DestroyInstanceImmediate(entityList[i]);
    }

    memset(entities, 0, sizeof(entities));

    physicsWorld->ClearScene();

    renderWorld->ClearScene();

    firstFreeIndex = 16; // TEMP
    
    entityHash.Free();
    entityTagHash.Free();

    entityHierarchy.RemoveFromHierarchy();

    memset(spawnIds, -1, sizeof(spawnIds));
    spawnCount = 0;
}

Entity *GameWorld::FindEntity(const char *name) const {
    if (!name || !name[0]) {
        return nullptr;
    }

    int hash = entityHash.GenerateHash(name, false);
    for (int i = entityHash.First(hash); i != -1; i = entityHash.Next(i)) {
        if (!Str::Icmp(entities[i]->GetName(), name)) {
            return entities[i];
        }
    }

    return nullptr;
}

Entity *GameWorld::FindEntityByGuid(const Guid &guid) const {
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        if (ent->GetGuid() == guid) {
            return ent;
        }
    }

    return nullptr;
}

Entity *GameWorld::FindEntityByTag(const char *tagName) const {
    if (!tagName || !tagName[0]) {
        return nullptr;
    }

    int hash = entityTagHash.GenerateHash(tagName, false);
    for (int i = entityTagHash.First(hash); i != -1; i = entityTagHash.Next(i)) {
        if (!Str::Icmp(entities[i]->GetTag(), tagName)) {
            return entities[i];
        }
    }

    return nullptr;
}

const EntityPtrArray GameWorld::FindEntitiesByTag(const char *tagName) const {
    if (!tagName || !tagName[0]) {
        return EntityPtrArray();
    }

    EntityPtrArray resultEntities;
    int hash = entityTagHash.GenerateHash(tagName, false);
    for (int i = entityTagHash.First(hash); i != -1; i = entityTagHash.Next(i)) {
        if (!Str::Icmp(entities[i]->GetTag(), tagName)) {
            resultEntities.Append(entities[i]);
        }
    }

    return resultEntities;
}

Entity *GameWorld::FindEntityByRenderEntity(int renderEntityHandle) const {
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        if (ent->HasRenderEntity(renderEntityHandle)) {
            return ent; 
        }
    }

    return nullptr;
}

void GameWorld::OnApplicationTerminate() {
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->OnApplicationTerminate();
    }
}

void GameWorld::OnApplicationPause(bool pause) {
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->OnApplicationPause(pause);
    }
}

void GameWorld::OnEntityNameChanged(Entity *ent) {
    entityHash.Remove(ent->nameHash, ent->entityNum);

    int nameHash = entityHash.GenerateHash(ent->GetName(), false);
    ent->nameHash = nameHash;
    entityHash.Add(nameHash, ent->entityNum);
}

void GameWorld::OnEntityTagChanged(Entity *ent) {
    entityTagHash.Remove(ent->tagHash, ent->entityNum);

    int tagHash = entityTagHash.GenerateHash(ent->GetTag(), false);
    ent->tagHash = tagHash;
    entityHash.Add(tagHash, ent->entityNum);
}

int GameWorld::GetEntitySpawnId(const Entity *ent) {
    assert(ent);
    assert(ent->entityNum >= 0 && ent->entityNum < MaxEntities);
    return spawnIds[ent->entityNum];
}

bool GameWorld::IsRegisteredEntity(const Entity *ent) const {
    return ent->entityNum == BadEntityNum ? false : true;
}

void GameWorld::RegisterEntity(Entity *ent, int spawn_entnum) {
    int nameHash = entityHash.GenerateHash(ent->GetName(), false);
    int tagHash = entityTagHash.GenerateHash(ent->GetTag(), false);

    // 인덱스를 미리 정해주지 않은 경우 entities[] 의 빈곳을 찾는다
    if (spawn_entnum < 0) {
        while (entities[firstFreeIndex] && firstFreeIndex < MaxEntityNum) {
            firstFreeIndex++;
        }
    
        if (firstFreeIndex >= MaxEntityNum) {
            BE_FATALERROR(L"no free entities");
        }
    
        spawn_entnum = firstFreeIndex++;
    }

    ent->nameHash = nameHash;
    ent->tagHash = tagHash;

    entityHash.Add(nameHash, spawn_entnum);
    entityTagHash.Add(tagHash, spawn_entnum);

    entities[spawn_entnum] = ent;
    spawnIds[spawn_entnum] = spawnCount++; // spawn ID 는 따로 관리

    ent->entityNum = spawn_entnum;

    Guid parentGuid = ent->props->Get("parent").As<Guid>();
    Entity *parent = FindEntityByGuid(parentGuid);
    if (parent) {
        ent->node.SetParent(parent->node);
    } else {
        ent->node.SetParent(entityHierarchy);
    }
    
    if (gameStarted && !isMapLoading) {
        ent->Awake();
        ent->Start();
    }

    EmitSignal(&SIG_EntityRegistered, ent);
}

void GameWorld::UnregisterEntity(Entity *ent) {
    assert(ent);

    if (!IsRegisteredEntity(ent)) {
        assert(0);
        return;
    }

    if (ent->entityNum < firstFreeIndex) {
        firstFreeIndex = ent->entityNum;
    }

    ent->node.RemoveFromHierarchy();
    
    entityHash.Remove(ent->nameHash, ent->entityNum);
    entityTagHash.Remove(ent->tagHash, ent->entityNum);

    int index = ent->entityNum;
    ent->entityNum = BadEntityNum;
    entities[index] = nullptr;
    spawnIds[index] = -1;

    EmitSignal(&SIG_EntityUnregistered, ent);
}

Entity *GameWorld::CloneEntity(const Entity *originalEntity) {
    EntityPtrArray originalEntities;

    // Get the original entity and all of his children
    originalEntities.Append(const_cast<Entity *>(originalEntity));
    originalEntity->GetChildren(originalEntities);

    HashTable<Guid, Guid> guidMap;
    EntityPtrArray clonedEntities;

    int numEntities = originalEntities.Count();

    for (int i = 0; i < numEntities; i++) {
        Json::Value originalEntityValue;
        originalEntities[i]->Serialize(originalEntityValue);
        
        Json::Value clonedEntityValue = Entity::CloneEntityValue(originalEntityValue, guidMap);

        Entity *clonedEntity = Entity::CreateEntity(clonedEntityValue);
        clonedEntities.Append(clonedEntity);
    }

    // Remap GUIDs for the cloned entities
    Entity::RemapGuids(clonedEntities, guidMap);

    // Initialize & register cloned entities
    for (int i = 0; i < numEntities; i++) {
        const Entity *originalEntity = originalEntities[i];
        Entity *clonedEntity = clonedEntities[i];

        clonedEntity->gameWorld = this;

        // if the originalEntity is a prefab parent
        if (originalEntity->IsPrefabParent()) {
            clonedEntity->props->Set("prefabParent", originalEntity->GetGuid());
            clonedEntity->props->Set("isPrefabParent", false);
        }

        clonedEntity->InitHierarchy();
        clonedEntity->Init();
    }

    return clonedEntities[0];
}

Entity *GameWorld::InstantiateEntity(const Entity *originalEntity) {
    Entity *clonedEntity = CloneEntity(originalEntity);

    RegisterEntity(clonedEntity);

    EntityPtrArray children;
    clonedEntity->GetChildren(children);

    for (int i = 0; i < children.Count(); i++) {
        RegisterEntity(children[i]);
    }

    return clonedEntity;
}

Entity *GameWorld::InstantiateEntityWithTransform(const Entity *originalEntity, const Vec3 &origin, const Angles &angles) {
    Entity *clonedEntity = CloneEntity(originalEntity);
    
    ComTransform *transform = clonedEntity->GetTransform();
    transform->SetLocalOrigin(origin);
    transform->SetLocalAngles(angles);

    RegisterEntity(clonedEntity);

    EntityPtrArray children;
    clonedEntity->GetChildren(children);

    for (int i = 0; i < children.Count(); i++) {
        RegisterEntity(children[i]);
    }

    return clonedEntity;
}

bool GameWorld::SpawnEntityFromJson(Json::Value &entityValue, Entity **ent) {
    const char *classname = entityValue["classname"].asCString();
    if (Str::Cmp(classname, Entity::metaObject.ClassName()) != 0) {
        BE_WARNLOG(L"bad classname '%hs' for entity\n", classname);
        return false;
    }

    int spawn_entnum = entityValue.get("spawn_entnum", -1).asInt();

    Entity *entity = Entity::CreateEntity(entityValue);
    entity->gameWorld = this;

    entity->InitHierarchy();
    entity->Init();

    RegisterEntity(entity, spawn_entnum);

    if (ent) {
        *ent = entity;
    }

    return true;
}

void GameWorld::SpawnEntitiesFromJson(Json::Value &entitiesValue) {
    for (int i = 0; i < entitiesValue.size(); i++) {
        Json::Value entityValue = entitiesValue[i];

        const char *classname = entityValue["classname"].asCString();

        if (!Str::Cmp(classname, Entity::metaObject.ClassName())) {
            SpawnEntityFromJson(entityValue);
        } else {
            BE_WARNLOG(L"Unknown classname '%hs'\n", classname);
        }
    }
}

void GameWorld::BeginMapLoading() {
    isMapLoading = true;

    Reset();
}

void GameWorld::FinishMapLoading() {
    isMapLoading = false;

    renderWorld->FinishMapLoading();

    animControllerManager.DestroyUnusedAnimControllers();
    textureManager.DestroyUnusedTextures();
    shaderManager.DestroyUnusedShaders();
    materialManager.DestroyUnusedMaterials();
    meshManager.DestroyUnusedMeshes();
    particleSystemManager.DestroyUnusedParticleSystems();
    skeletonManager.DestroyUnusedSkeletons();
    animManager.DestroyUnusedAnims();
    skinManager.DestroyUnusedSkins();
    colliderManager.DestroyUnusedColliders();
    soundSystem.DestroyUnusedSounds();
    //meshManager.EndLevelLoad();
}

void GameWorld::StartGame() {
    gameStarted = true;

    timeScale = 1.0f;

    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->Awake();
    }

    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->Start();
    }
}

void GameWorld::StopGame() {
    gameStarted = false;

    soundSystem.StopAllSounds();
}

void GameWorld::RestartGame(const char *mapName) {
    PostEvent(&EV_RestartGame, mapName);
}

void GameWorld::Event_RestartGame(const char *mapName) {
    StopGame();
    
    LoadMap(mapName);
    
    StartGame();
}

void GameWorld::LoadSettings() {
    if (tagLayerSettings) {
        TagLayerSettings::DestroyInstanceImmediate(tagLayerSettings);
        tagLayerSettings = nullptr;
    }

    if (physicsSettings) {
        PhysicsSettings::DestroyInstanceImmediate(physicsSettings);
        physicsSettings = nullptr;
    }

    // Load all project settings
    LoadTagLayerSettings("ProjectSettings/tagLayer.settings");
    //LoadInputSettings("ProjectSettings/input.settings");
    LoadPhysicsSettings("ProjectSettings/physics.settings");
    //LoadRendererSettings("ProjectSettings/renderer.settings");
}

void GameWorld::LoadTagLayerSettings(const char *filename) {
    Json::Value jsonNode;
    Json::Reader jsonReader;
    bool failedToParse = false;

    char *text = nullptr;
    fileSystem.LoadFile(filename, true, (void **)&text);
    if (text) {
        if (!jsonReader.parse(text, jsonNode)) {
            BE_WARNLOG(L"Failed to parse JSON text '%hs'\n", filename);
            failedToParse = true;
        }

        fileSystem.FreeFile(text);
    } else {
        failedToParse = true;
    }

    if (failedToParse) {
        jsonNode["classname"] = TagLayerSettings::metaObject.ClassName();

        // default tags
        jsonNode["tag"][0] = "Untagged";
        jsonNode["tag"][1] = "MainCamera";
        jsonNode["tag"][2] = "Player";

        // default layers
        jsonNode["layer"][0] = "Default";
        jsonNode["layer"][1] = "UI";
        jsonNode["layer"][2] = "Editor";
    }

    const char *classname = jsonNode["classname"].asCString();

    if (!Str::Cmp(classname, TagLayerSettings::metaObject.ClassName())) {
        tagLayerSettings = static_cast<TagLayerSettings *>(TagLayerSettings::metaObject.CreateInstance());
        tagLayerSettings->props->Init(jsonNode);
        tagLayerSettings->SetGameWorld(this);
        tagLayerSettings->Init();
    } else {
        BE_WARNLOG(L"Unknown classname '%hs'\n", classname);
    }
}

void GameWorld::LoadPhysicsSettings(const char *filename) {
    Json::Value jsonNode;
    Json::Reader jsonReader;
    bool failedToParse = false;

    char *text = nullptr;
    fileSystem.LoadFile(filename, true, (void **)&text);
    if (text) {
        if (!jsonReader.parse(text, jsonNode)) {
            BE_WARNLOG(L"Failed to parse JSON text '%hs'\n", filename);
            failedToParse = true;
        }

        fileSystem.FreeFile(text);
    } else {
        failedToParse = true;
    }

    if (failedToParse) {
        jsonNode["classname"] = PhysicsSettings::metaObject.ClassName();
    }

    const char *classname = jsonNode["classname"].asCString();

    if (!Str::Cmp(classname, PhysicsSettings::metaObject.ClassName())) {
        physicsSettings = static_cast<PhysicsSettings *>(PhysicsSettings::metaObject.CreateInstance());
        physicsSettings->props->Init(jsonNode);
        physicsSettings->SetGameWorld(this);
        physicsSettings->Init();
    } else {
        BE_WARNLOG(L"Unknown classname '%hs'\n", classname);
    }
}

void GameWorld::SaveSettings() {
    SaveObject("ProjectSettings/tagLayer.settings", tagLayerSettings);
    SaveObject("ProjectSettings/physics.settings", physicsSettings);
}

void GameWorld::SaveObject(const char *filename, const Object *object) const {
    Json::Value jsonNode;
    object->props->Serialize(jsonNode);

    Json::StyledWriter jsonWriter;
    Str jsonText = jsonWriter.write(jsonNode).c_str();

    fileSystem.WriteFile(filename, jsonText.c_str(), jsonText.Length());
}

void GameWorld::NewMap() {
    Json::Value defaultMapRenderSettingsValue;
    defaultMapRenderSettingsValue["classname"] = MapRenderSettings::metaObject.ClassName();

    mapRenderSettings->props->Init(defaultMapRenderSettingsValue);

    Reset();
}

bool GameWorld::LoadMap(const char *filename) {
    BE_LOG(L"Loading map '%hs'...\n", filename);

    BeginMapLoading();

    char *text = nullptr;
    fileSystem.LoadFile(filename, true, (void **)&text);
    if (!text) {
        BE_WARNLOG(L"Couldn't load '%hs'\n", filename);
        FinishMapLoading();
        return false;
    }

    mapName = filename;

    Json::Value map;
    Json::Reader jsonReader;
    if (!jsonReader.parse(text, map)) {
        BE_WARNLOG(L"Failed to parse JSON text\n");
        return false;
    }

    // Read map version
    int mapVersion = map["version"].asInt();

    // Read map render settings
    mapRenderSettings->props->Init(map["renderSettings"]);
    mapRenderSettings->Init();

    // Read entities
    SpawnEntitiesFromJson(map["entities"]);
    
    fileSystem.FreeFile(text);

    FinishMapLoading();

    return true;
}

void GameWorld::SerializeEntityHierarchy(const Hierarchy<Entity> &entityHierarchy, Json::Value &entitiesValue) {
    Json::Value entityValue;

    Entity *ent = entityHierarchy.Owner();
    ent->Serialize(entityValue);

    entitiesValue.append(entityValue);

    for (Entity *child = entityHierarchy.GetChild(); child; child = child->GetNode().GetNextSibling()) {
        GameWorld::SerializeEntityHierarchy(child->GetNode(), entitiesValue);
    }
}

void GameWorld::SaveMap(const char *filename) {
    BE_LOG(L"Saving map '%hs'...\n", filename);

    Json::Value map;

    // Write map version
    map["version"] = 1;

    // Write map render settings
    mapRenderSettings->Serialize(map["renderSettings"]);

    // Write entities
    for (Entity *child = entityHierarchy.GetChild(); child; child = child->GetNode().GetNextSibling()) {
        GameWorld::SerializeEntityHierarchy(child->GetNode(), map["entities"]);
    }

    Json::StyledWriter jsonWriter;
    Str jsonText = jsonWriter.write(map).c_str();

    fileSystem.WriteFile(filename, jsonText.c_str(), jsonText.Length());
}

void GameWorld::Update(int elapsedTime) {
    prevTime = time;

    int scaledElapsedTime = elapsedTime * timeScale;

    time += scaledElapsedTime;

    if (gameStarted) {
        physicsWorld->StepSimulation(scaledElapsedTime);

        UpdateEntities();
    }
}

void GameWorld::UpdateEntities() {
    // depth-first order  
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->Update();
    }

    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->LateUpdate();
    }
}

void GameWorld::ProcessPointerInput() {
    if (!gameStarted) {
        return;
    }

    if (!inputSystem.IsUpdated()) {
        return;
    }

    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ComCamera *camera = ent->GetComponent<ComCamera>();
        if (!camera) {
            continue;
        }

        camera->ProcessPointerInput(inputSystem.GetMousePos());
    }
}

BE1::Entity *GameWorld::RayIntersection(const BE1::Vec3 &start, const BE1::Vec3 &dir, const BE1::Array<BE1::Entity *> &excludingArray, float *scale) const {
    BE1::Entity *minEntity = nullptr;

    float minScale = FLT_MAX;
    if (scale) {
        *scale = minScale;
    }

    for (BE1::Entity *ent = entityHierarchy.GetNext(); ent; ent = ent->GetNode().GetNext()) {
        if (excludingArray.Find(ent)) {
            continue;
        }

        if (ent->RayIntersection(start, dir, true, minScale)) {
            minEntity = ent;
            if (scale) {
                *scale = minScale;
            }
        }
    }

    return minEntity;
}

void GameWorld::RenderCamera() {
    StaticArray<ComCamera *, 16> cameraArray;

    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ComCamera *camera = ent->GetComponent<ComCamera>();
        if (!camera) {
            continue;
        }

        if (cameraArray.Append(camera) == -1) {
            break;
        }
    }

    auto compareFunc = [](const ComCamera *arg1, const ComCamera *arg2) -> bool {
        return arg1->GetOrder() < arg2->GetOrder() ? true : false;
    };
    cameraArray.Sort(compareFunc);

    for (int i = 0; i < cameraArray.Count(); i++) {
        cameraArray[i]->RenderScene();
    }
}

void GameWorld::SaveSnapshot() {
    snapshotValues.clear();

    mapRenderSettings->Serialize(snapshotValues["renderSettings"]);

    for (Entity *child = entityHierarchy.GetChild(); child; child = child->GetNode().GetNextSibling()) {
        GameWorld::SerializeEntityHierarchy(child->GetNode(), snapshotValues["entities"]);
    }

    //BE_LOG(L"%i entities snapshot saved\n", snapshotValues["entities"].size());
}

void GameWorld::RestoreSnapshot() {
    BeginMapLoading();

    mapRenderSettings->props->Init(snapshotValues["renderSettings"]);
    mapRenderSettings->Init();

    SpawnEntitiesFromJson(snapshotValues["entities"]);

    //BE_LOG(L"%i entities snapshot restored\n", snapshotValues["entities"].size());

    FinishMapLoading();
}

BE_NAMESPACE_END
