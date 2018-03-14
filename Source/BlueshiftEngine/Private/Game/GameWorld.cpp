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
#include "File/FileSystem.h"
#include "Render/Render.h"
#include "Physics/Collider.h"
#include "Physics/Physics.h"
#include "Input/InputSystem.h"
#include "Sound/SoundSystem.h"
#include "AnimController/AnimController.h"
#include "Components/ComTransform.h"
#include "Components/ComCamera.h"
#include "Components/ComScript.h"
#include "Game/Entity.h"
#include "Game/MapRenderSettings.h"
#include "Game/GameWorld.h"
#include "Game/GameSettings.h"
#include "Script/LuaVM.h"

BE_NAMESPACE_BEGIN

const EventDef EV_RestartGame("restartGame", false, "s");

const SignalDef GameWorld::SIG_EntityRegistered("GameWorld::EntityRegistered", "a");
const SignalDef GameWorld::SIG_EntityUnregistered("GameWorld::EntityUnregistered", "a");

OBJECT_DECLARATION("Game World", GameWorld, Object)
BEGIN_EVENTS(GameWorld)
    EVENT(EV_RestartGame, GameWorld::Event_RestartGame),
END_EVENTS

void GameWorld::RegisterProperties() {
}

GameWorld::GameWorld() {
    memset(entities, 0, sizeof(entities));

    // Create render settings
    mapRenderSettings = static_cast<MapRenderSettings *>(MapRenderSettings::metaObject.CreateInstance());
    mapRenderSettings->gameWorld = this;

    // Create render world
    renderWorld = renderSystem.AllocRenderWorld();

    // Create physics world
    physicsWorld = physicsSystem.AllocPhysicsWorld();

    isDebuggable = false;
    isMapLoading = false;
    gameStarted = false;

    timeScale = 1.0f;

    luaVM.Init();

    Reset();
}

GameWorld::~GameWorld() {
    ClearAllEntities();

    if (mapRenderSettings) {
        MapRenderSettings::DestroyInstanceImmediate(mapRenderSettings);
    }

    // Free render world
    renderSystem.FreeRenderWorld(renderWorld);

    // Free physics world
    physicsSystem.FreePhysicsWorld(physicsWorld);

    luaVM.Shutdown();
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
    // List up all of the destructable entities in depth first order
    EntityPtrArray entityList;
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        entityList.Append(ent);
    }

    // Delete entities in reverse depth first order
    for (int i = entityList.Count() - 1; i >= 0; i--) {
        Entity *ent = entityList[i];

        int entityNum = ent->entityNum;

        ent->node.RemoveFromHierarchy();

        Entity::DestroyInstanceImmediate(ent);

        entities[entityNum] = nullptr;
    }

    firstFreeIndex = 16; // TEMP

    entityHash.Free();
    entityTagHash.Free();

    physicsWorld->ClearScene();

    renderWorld->ClearScene();
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

    ent->entityNum = spawn_entnum;
    
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

    EmitSignal(&SIG_EntityUnregistered, ent);
}

Entity *GameWorld::CloneEntity(const Entity *originalEntity) {
    // Serialize source entity and it's children
    Json::Value originalEntitiesValue;
    BE1::Entity::SerializeHierarchy(originalEntity, originalEntitiesValue);

    // Clone entities value which is replaced by new GUIDs
    HashTable<Guid, Guid> guidMap;
    Json::Value clonedEntitiesValue = Entity::CloneEntitiesValue(originalEntitiesValue, guidMap);

    EntityPtrArray clonedEntities;

    for (int i = 0; i < clonedEntitiesValue.size(); i++) {
        // Create cloned entity
        Entity *clonedEntity = Entity::CreateEntity(clonedEntitiesValue[i], this);
        clonedEntities.Append(clonedEntity);

        // Remap all GUID references to newly created
        Entity::RemapGuids(clonedEntity, guidMap);

        // If source entity is prefab source, mark cloned entity originated from prefab entity
        if (originalEntitiesValue[i]["prefab"].asBool()) {
            clonedEntity->SetProperty("prefabSource", Guid::FromString(originalEntitiesValue[i]["guid"].asCString()));
            clonedEntity->SetProperty("prefab", false);
        }

        clonedEntity->Init();
        clonedEntity->InitComponents();
    }

    return clonedEntities[0];
}

Entity *GameWorld::CreateEmptyEntity(const char *name) {
    Json::Value value;
    value["name"] = name;

    value["components"][0]["classname"] = ComTransform::metaObject.ClassName();
    value["components"][0]["origin"] = Vec3::zero.ToString();
    value["components"][0]["angles"] = Angles::zero.ToString();

    Entity *entity = Entity::CreateEntity(value, this);

    entity->Init();
    entity->InitComponents();

    RegisterEntity(entity);

    return entity;
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
    transform->SetLocalTransform(origin, angles.ToMat3(), Vec3::one);

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

    Entity *entity = Entity::CreateEntity(entityValue, this);
    entity->Init();
    entity->InitComponents();

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

bool GameWorld::CheckScriptError() const {
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ComponentPtrArray scriptComponents = ent->GetComponents(&ComScript::metaObject);

        for (int i = 0; i < scriptComponents.Count(); i++) {
            ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();
            if (scriptComponent->HasError()) {
                return true;
            }
        }
    }

    return false;
}

void GameWorld::StartGame() {
    if (isDebuggable) {
        luaVM.StartDebuggee();
    }

    gameStarted = true;

    timeScale = 1.0f;

    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->Awake();
    }

    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->Start();
    }

    physicsWorld->Connect(&PhysicsWorld::SIG_PreStep, this, (SignalCallback)&GameWorld::FixedUpdateEntities);
    physicsWorld->Connect(&PhysicsWorld::SIG_PostStep, this, (SignalCallback)&GameWorld::FixedLateUpdateEntities);
}

void GameWorld::StopGame(bool stopAllSounds) {
    if (isDebuggable) {
        luaVM.StopDebuggee();
    }

    gameStarted = false;

    if (stopAllSounds) {
        soundSystem.StopAllSounds();
    }

    physicsWorld->Disconnect(&PhysicsWorld::SIG_PreStep, this);
    physicsWorld->Disconnect(&PhysicsWorld::SIG_PostStep, this);
}

void GameWorld::RestartGame(const char *mapName) {
    PostEvent(&EV_RestartGame, mapName);
}

void GameWorld::StopAllSounds() {
    soundSystem.StopAllSounds();
}

void GameWorld::Event_RestartGame(const char *mapName) {
    StopGame();
    
    LoadMap(mapName);
    
    StartGame();
}

void GameWorld::NewMap() {
    Json::Value defaultMapRenderSettingsValue;
    defaultMapRenderSettingsValue["classname"] = MapRenderSettings::metaObject.ClassName();

    mapRenderSettings->Deserialize(defaultMapRenderSettingsValue);

    Reset();
}

bool GameWorld::LoadMap(const char *filename) {
    BE_LOG(L"Loading map '%hs'...\n", filename);

    Reset();

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
    mapRenderSettings->Deserialize(map["renderSettings"]);
    mapRenderSettings->Init();

    // Read entities
    SpawnEntitiesFromJson(map["entities"]);
    
    fileSystem.FreeFile(text);

    FinishMapLoading();

    return true;
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
        Entity::SerializeHierarchy(child, map["entities"]);
    }

    Json::StyledWriter jsonWriter;
    Str jsonText = jsonWriter.write(map).c_str();

    fileSystem.WriteFile(filename, jsonText.c_str(), jsonText.Length());
}

void GameWorld::Update(int elapsedTime) {
    if (isDebuggable) {
        luaVM.PollDebuggee();
    }

    prevTime = time;

    int scaledElapsedTime = elapsedTime * timeScale;

    time += scaledElapsedTime;

    if (gameStarted) {
        physicsWorld->StepSimulation(scaledElapsedTime);

        UpdateEntities();

        LateUpdateEntities();

        luaVM.State().ForceGC();
    }
}

void GameWorld::FixedUpdateEntities(float timeStep) {
    // Call fixed update function for each entities in depth-first order
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->FixedUpdate(timeStep * timeScale);
    }
}

void GameWorld::FixedLateUpdateEntities(float timeStep) {
    // Call fixed post-update function for each entities in depth-first order
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->FixedLateUpdate(timeStep * timeScale);
    }
}

void GameWorld::UpdateEntities() {
    // Call update function for each entities in depth-first order
    for (Entity *ent = entityHierarchy.GetChild(); ent; ent = ent->node.GetNext()) {
        ent->Update();
    }
}

void GameWorld::LateUpdateEntities() {
    // Call post-update function for each entities in depth-first order
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
        if (!camera || !camera->IsActiveInHierarchy()) {
            continue;
        }

        camera->ProcessPointerInput(inputSystem.GetMousePos());
    }
}

Entity *GameWorld::RayIntersection(const Vec3 &start, const Vec3 &dir, const Array<Entity *> &excludingArray, float *scale) const {
    Entity *minEntity = nullptr;

    float minScale = FLT_MAX;
    if (scale) {
        *scale = minScale;
    }

    for (Entity *ent = entityHierarchy.GetNext(); ent; ent = ent->GetNode().GetNext()) {
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
        if (!camera || !camera->IsActiveInHierarchy()) {
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
        Entity::SerializeHierarchy(child, snapshotValues["entities"]);
    }

    //BE_LOG(L"%i entities snapshot saved\n", snapshotValues["entities"].size());
}

void GameWorld::RestoreSnapshot() {
    Reset();

    BeginMapLoading();

    mapRenderSettings->Deserialize(snapshotValues["renderSettings"]);
    mapRenderSettings->Init();

    SpawnEntitiesFromJson(snapshotValues["entities"]);

    //BE_LOG(L"%i entities snapshot restored\n", snapshotValues["entities"].size());

    FinishMapLoading();
}

BE_NAMESPACE_END
