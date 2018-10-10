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
#include "Scripting/LuaVM.h"
#include "StaticBatching/StaticBatch.h"
#include "../StaticBatching/MeshCombiner.h"

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

    firstFreeIndex = 0;

    // Create render settings
    mapRenderSettings = static_cast<MapRenderSettings *>(MapRenderSettings::metaObject.CreateInstance());
    mapRenderSettings->gameWorld = this;

    // Create render world
    renderWorld = renderSystem.AllocRenderWorld();

    // Create physics world
    physicsWorld = physicsSystem.AllocPhysicsWorld();

    luaVM.Init();

    Reset();
}

GameWorld::~GameWorld() {
    ClearEntities();

    StaticBatch::ClearAllStaticBatches();

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
    time = 0;
    prevTime = 0;

    random.SetSeed(0);

    renderWorld->ClearDebugPrimitives(0);
    renderWorld->ClearDebugText(0);

    physicsWorld->Reset();
}

int GameWorld::GetDeltaTime() const {
    int dt = time - prevTime;

    return dt;
}

void GameWorld::DontDestroyOnLoad(Entity *entity) {
    if (!entity->node.IsParentedBy(scenes[DontDestroyOnLoadSceneNum].root)) {
        entity->GetRoot()->node.SetParent(scenes[DontDestroyOnLoadSceneNum].root);
    }
}

void GameWorld::ClearEntities(bool clearAll) {
    // List up all of the entities to remove in depth first order
    EntityPtrArray entitiesToRemove;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        if (clearAll || sceneIndex != DontDestroyOnLoadSceneNum) {
            for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
                entitiesToRemove.Append(ent);
            }
        }
    }

    // Remove entities in reverse depth first order
    for (int i = entitiesToRemove.Count() - 1; i >= 0; i--) {
        Entity *ent = entitiesToRemove[i];

        ent->node.RemoveFromHierarchy();

        entityHash.Remove(ent->nameHash, ent->entityNum);
        entityTagHash.Remove(ent->tagHash, ent->entityNum);

        int entityNum = ent->entityNum;

        Entity::DestroyInstanceImmediate(ent);

        entities[entityNum] = nullptr;
    }
}

Entity *GameWorld::FindEntityByName(const char *name) const {
    if (!name || !name[0]) {
        return nullptr;
    }

    int hash = entityHash.GenerateHash(name);
    for (int i = entityHash.First(hash); i != -1; i = entityHash.Next(i)) {
        if (!Str::Cmp(entities[i]->GetName(), name)) {
            return entities[i];
        }
    }

    return nullptr;
}

Entity *GameWorld::FindRootEntityByName(const char *name) const {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNextSibling()) {
            if (ent->name == name) {
                return ent;
            }
        }
    }

    return nullptr;
}

Entity *GameWorld::FindEntity(const char *path) const {
    bool findInRoot = false;
    Str name = path;
    int slashIndex = Str::FindChar(name, '/');
    if (slashIndex == 0) {
        findInRoot = true;
        slashIndex = Str::FindChar(name, '/', 1);
    }

    if (slashIndex >= 0) {
        name[slashIndex] = '\0';
    }

    Entity *entity = findInRoot ? FindRootEntityByName(name) : FindEntityByName(name);
    if (!entity || slashIndex < 0) {
        return entity;
    }

    return FindEntityRelativePath(entity, &name[slashIndex + 1]);
}

Entity *GameWorld::FindEntityRelativePath(const Entity *entity, const char *path) const {
    Str name = path;
    int slashIndex = Str::FindChar(name, '/');
    if (slashIndex >= 0) {
        name[slashIndex] = '\0';
    }

    for (Entity *ent = entity->node.GetChild(); ent; ent = ent->node.GetNextSibling()) {
        if (ent->name == name) {
            if (slashIndex >= 0) {
                return FindEntityRelativePath(ent, &name[slashIndex + 1]);
            } else {
                return ent;
            }
        }
    }

    return nullptr;
}

Entity *GameWorld::FindEntityByTag(const char *tagName) const {
    if (!tagName || !tagName[0]) {
        return nullptr;
    }

    int hash = entityTagHash.GenerateHash(tagName);
    for (int i = entityTagHash.First(hash); i != -1; i = entityTagHash.Next(i)) {
        if (!Str::Cmp(entities[i]->GetTag(), tagName)) {
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
    int hash = entityTagHash.GenerateHash(tagName);
    for (int i = entityTagHash.First(hash); i != -1; i = entityTagHash.Next(i)) {
        if (!Str::Cmp(entities[i]->GetTag(), tagName)) {
            resultEntities.Append(entities[i]);
        }
    }

    return resultEntities;
}

Entity *GameWorld::FindEntityByRenderEntity(int renderEntityHandle) const {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            if (ent->HasRenderEntity(renderEntityHandle)) {
                return ent;
            }
        }
    }

    return nullptr;
}

void GameWorld::OnApplicationResize(int width, int height) {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            ent->OnApplicationResize(width, height);
        }
    }
}

void GameWorld::OnApplicationTerminate() {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            ent->OnApplicationTerminate();
        }
    }
}

void GameWorld::OnApplicationPause(bool pause) {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            ent->OnApplicationPause(pause);
        }
    }
}

void GameWorld::OnEntityNameChanged(Entity *ent) {
    entityHash.Remove(ent->nameHash, ent->entityNum);

    int nameHash = entityHash.GenerateHash(ent->GetName());
    ent->nameHash = nameHash;
    entityHash.Add(nameHash, ent->entityNum);
}

void GameWorld::OnEntityTagChanged(Entity *ent) {
    entityTagHash.Remove(ent->tagHash, ent->entityNum);

    int tagHash = entityTagHash.GenerateHash(ent->GetTag());
    ent->tagHash = tagHash;
    entityTagHash.Add(tagHash, ent->entityNum);
}

bool GameWorld::IsRegisteredEntity(const Entity *ent) const {
    return ent->entityNum == BadEntityNum ? false : true;
}

void GameWorld::RegisterEntity(Entity *ent, int entityIndex) {
    int nameHash = entityHash.GenerateHash(ent->GetName());
    int tagHash = entityTagHash.GenerateHash(ent->GetTag());

    // If entityIndex is not given, find a blank space in entities[]
    if (entityIndex < 0) {
        while (entities[firstFreeIndex] && firstFreeIndex < MaxEntityNum) {
            firstFreeIndex++;
        }
    
        if (firstFreeIndex >= MaxEntityNum) {
            BE_FATALERROR(L"no free entities");
        }
    
        entityIndex = firstFreeIndex++;
    }

    entities[entityIndex] = ent;

    ent->entityNum = entityIndex;
    ent->nameHash = nameHash;
    ent->tagHash = tagHash;

    entityHash.Add(nameHash, entityIndex);
    entityTagHash.Add(tagHash, entityIndex);

    if (!isMapLoading) {
        if (gameAwaking) {
            ent->Awake();
        } else if (gameStarted) {
            ent->Awake();
            ent->Start();
        }
    }

    EmitSignal(&SIG_EntityRegistered, ent);
}

void GameWorld::UnregisterEntity(Entity *ent) {
    if (!IsRegisteredEntity(ent)) {
        BE_WARNLOG(L"GameWorld::UnregisterEntity: Entity '%hs' is already unregistered\n", ent->GetName().c_str());
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
    Entity::SerializeHierarchy(originalEntity, originalEntitiesValue);

    // Clone entities value which is replaced by new GUIDs
    HashTable<Guid, Guid> guidMap;
    Json::Value clonedEntitiesValue = Entity::CloneEntitiesValue(originalEntitiesValue, guidMap);

    EntityPtrArray clonedEntities;

    for (int i = 0; i < clonedEntitiesValue.size(); i++) {
        // Create cloned entity
        Entity *clonedEntity = Entity::CreateEntity(clonedEntitiesValue[i], this, originalEntity->sceneIndex);
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

Entity *GameWorld::InstantiateEntityWithTransform(const Entity *originalEntity, const Vec3 &origin, const Quat &rotation) {
    Entity *clonedEntity = CloneEntity(originalEntity);
    
    ComTransform *transform = clonedEntity->GetTransform();
    transform->SetLocalOriginRotation(origin, rotation);

    RegisterEntity(clonedEntity);

    EntityPtrArray children;
    clonedEntity->GetChildren(children);

    for (int i = 0; i < children.Count(); i++) {
        RegisterEntity(children[i]);
    }

    return clonedEntity;
}

Entity *GameWorld::SpawnEntityFromJson(Json::Value &entityValue, int sceneIndex) {
    const char *classname = entityValue["classname"].asCString();
    if (Str::Cmp(classname, Entity::metaObject.ClassName()) != 0) {
        BE_WARNLOG(L"GameWorld::SpawnEntityFromJson: Bad classname '%hs' for entity\n", classname);
        return nullptr;
    }

    int spawn_entnum = entityValue.get("spawn_entnum", -1).asInt();

    Entity *entity = Entity::CreateEntity(entityValue, this, sceneIndex);

    entity->Init();
    entity->InitComponents();

    RegisterEntity(entity, spawn_entnum);

    return entity;
}

void GameWorld::SpawnEntitiesFromJson(Json::Value &entitiesValue, int sceneIndex) {
    for (int i = 0; i < entitiesValue.size(); i++) {
        Json::Value entityValue = entitiesValue[i];

        SpawnEntityFromJson(entityValue, sceneIndex);
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

    StaticBatch::ClearAllStaticBatches();
}

bool GameWorld::CheckScriptError() const {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            ComponentPtrArray scriptComponents = ent->GetComponents(&ComScript::metaObject);

            for (int i = 0; i < scriptComponents.Count(); i++) {
                ComScript *scriptComponent = scriptComponents[i]->Cast<ComScript>();
                if (scriptComponent->HasError()) {
                    return true;
                }
            }
        }
    }

    return false;
}

void GameWorld::StartGame() {
    gameStarted = true;

    timeScale = 1.0f;

    if (isDebuggable) {
        luaVM.StartDebuggee();
    }

    luaVM.ClearTweeners();

    luaVM.ClearWatingThreads();

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        StaticBatch::CombineAll(scenes[sceneIndex].root);
    }

    gameAwaking = true;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            if (!ent->awaked) {
                ent->Awake();
            }
        }
    }

    gameAwaking = false;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            if (!ent->started) {
                ent->Start();
            }
        }
    }

    luaVM.State().ForceGC();

    physicsWorld->Connect(&PhysicsWorld::SIG_PreStep, this, (SignalCallback)&GameWorld::FixedUpdateEntities);
    physicsWorld->Connect(&PhysicsWorld::SIG_PostStep, this, (SignalCallback)&GameWorld::FixedLateUpdateEntities);
}

void GameWorld::StopGame(bool stopAllSounds) {
    gameStarted = false;

    if (isDebuggable) {
        luaVM.StopDebuggee();
    }

    luaVM.ClearTweeners();

    luaVM.ClearWatingThreads();

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
    StopGame(false);
    
    LoadMap(mapName, LoadSceneMode::Single);
    
    StartGame();
}

void GameWorld::NewMap() {
    Json::Value defaultMapRenderSettingsValue;
    defaultMapRenderSettingsValue["classname"] = MapRenderSettings::metaObject.ClassName();

    mapRenderSettings->Deserialize(defaultMapRenderSettingsValue);

    ClearEntities();

    Reset();
}

bool GameWorld::LoadMap(const char *filename, LoadSceneMode mode) {
    BE_LOG(L"Loading map '%hs'...\n", filename);

    if (mode != LoadSceneMode::Additive) {
        ClearEntities(mode == LoadSceneMode::Editor);
    }

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

    fileSystem.FreeFile(text);

    // Read map version
    int mapVersion = map["version"].asInt();

    // Read map render settings
    mapRenderSettings->Deserialize(map["renderSettings"]);
    mapRenderSettings->Init();

    int sceneIndex = 0;
    for (; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        if (!scenes[sceneIndex].root.GetChild()) {
            break;
        }
    }

    assert(sceneIndex < COUNT_OF(scenes));

    // Read and spawn entities
    SpawnEntitiesFromJson(map["entities"], sceneIndex);

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
    for (Entity *ent = scenes[0].root.GetChild(); ent; ent = ent->node.GetNextSibling()) {
        Entity::SerializeHierarchy(ent, map["entities"]);
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
        // FixedUpdate() is called in StepSimulation() internally
        physicsWorld->StepSimulation(scaledElapsedTime);

        UpdateEntities();

        LateUpdateEntities();

        // Wake up waiting coroutine in Lua scripts
        luaVM.WakeUpWatingThreads(MS2SEC(time));

        // Update tweeners in Lua scripts
        luaVM.UpdateTweeners(MS2SEC(elapsedTime), timeScale);

        luaVM.State().ForceGC();
    }
}

void GameWorld::FixedUpdateEntities(float timeStep) {
    // Call fixed update function for each entities in depth-first order
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            ent->FixedUpdate(timeStep * timeScale);
        }
    }
}

void GameWorld::FixedLateUpdateEntities(float timeStep) {
    // Call fixed post-update function for each entities in depth-first order
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            ent->FixedLateUpdate(timeStep * timeScale);
        }
    }
}

void GameWorld::UpdateEntities() {
    // Call update function for each entities in depth-first order
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            ent->Update();
        }
    }
}

void GameWorld::LateUpdateEntities() {
    // Call post-update function for each entities in depth-first order
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            ent->LateUpdate();
        }
    }
}

void GameWorld::ProcessPointerInput() {
    if (!gameStarted) {
        return;
    }

    if (!inputSystem.IsUpdated()) {
        return;
    }

    StaticArray<ComCamera *, 16> cameraComponents;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            ComCamera *camera = ent->GetComponent<ComCamera>();
            if (!camera || !camera->IsActiveInHierarchy()) {
                continue;
            }

            if (cameraComponents.Append(camera) == -1) {
                break;
            }
        }
    }

    // Process pointer input in reverse order
    auto compareFunc = [](const ComCamera *arg1, const ComCamera *arg2) -> bool {
        return arg1->GetOrder() > arg2->GetOrder() ? true : false;
    };
    cameraComponents.Sort(compareFunc);

    for (int i = 0; i < cameraComponents.Count(); i++) {
        ComCamera *cameraComponent = cameraComponents[i];

        if (cameraComponent->ProcessMousePointerInput(inputSystem.GetMousePos()) ||
            cameraComponent->ProcessTouchPointerInput()) {
            break;
        }
    }
}

Entity *GameWorld::RayIntersection(const Vec3 &start, const Vec3 &dir, int layerMask) const {
    Entity *minEntity = nullptr;

    float minScale = FLT_MAX;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            if (!(BIT(ent->GetLayer()) & layerMask)) {
                continue;
            }

            if (ent->RayIntersection(start, dir, true, minScale)) {
                minEntity = ent;
            }
        }
    }

    return minEntity;
}

Entity *GameWorld::RayIntersection(const Vec3 &start, const Vec3 &dir, int layerMask, const Array<Entity *> &excludingArray, float *scale) const {
    Entity *minEntity = nullptr;

    float minScale = FLT_MAX;
    if (scale) {
        *scale = minScale;
    }

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            if (!(BIT(ent->GetLayer()) & layerMask)) {
                continue;
            }

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
    }

    return minEntity;
}

void GameWorld::Render() {
    StaticArray<ComCamera *, 16> cameraComponents;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNext()) {
            ComCamera *camera = ent->GetComponent<ComCamera>();
            if (!camera || !camera->IsActiveInHierarchy()) {
                continue;
            }

            if (cameraComponents.Append(camera) == -1) {
                break;
            }
        }
    }

    auto compareFunc = [](const ComCamera *arg1, const ComCamera *arg2) -> bool {
        return arg1->GetOrder() < arg2->GetOrder() ? true : false;
    };
    cameraComponents.Sort(compareFunc);

    for (int i = 0; i < cameraComponents.Count(); i++) {
        cameraComponents[i]->RenderScene();
    }
}

void GameWorld::SaveSnapshot() {
    snapshotValues.clear();

    mapRenderSettings->Serialize(snapshotValues["renderSettings"]);

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetChild(); ent; ent = ent->node.GetNextSibling()) {
            Entity::SerializeHierarchy(ent, snapshotValues["entities"][sceneIndex]);
        }
    }
}

void GameWorld::RestoreSnapshot() {
    ClearEntities();

    Reset();

    BeginMapLoading();

    mapRenderSettings->Deserialize(snapshotValues["renderSettings"]);
    mapRenderSettings->Init();

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        SpawnEntitiesFromJson(snapshotValues["entities"][sceneIndex], sceneIndex);
    }

    FinishMapLoading();
}

BE_NAMESPACE_END
