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
#include "IO/FileSystem.h"
#include "Engine/GameClient.h"
#include "Render/Render.h"
#include "Physics/Collider.h"
#include "Physics/Physics.h"
#include "Input/InputSystem.h"
#include "Sound/SoundSystem.h"
#include "AnimController/AnimController.h"
#include "Asset/GuidMapper.h"
#include "Components/Transform/ComTransform.h"
#include "Components/ComCamera.h"
#include "Components/ComCanvas.h"
#include "Components/ComScript.h"
#include "Components/Physics/ComRigidBody.h"
#include "Game/Entity.h"
#include "Game/MapRenderSettings.h"
#include "Game/GameWorld.h"
#include "Game/GameSettings.h"
#include "Game/CastResult.h"
#include "Scripting/LuaVM.h"
#include "StaticBatching/StaticBatch.h"
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

const EventDef EV_RestartGame("restartGame", false, "s");
const EventDef EV_DontDestroyOnLoad("dontDestroyOnLoad", false, "a");

const SignalDef GameWorld::SIG_EntityRegistered("GameWorld::EntityRegistered", "a");
const SignalDef GameWorld::SIG_EntityUnregistered("GameWorld::EntityUnregistered", "a");

OBJECT_DECLARATION("Game World", GameWorld, Object)
BEGIN_EVENTS(GameWorld)
    EVENT(EV_RestartGame, GameWorld::Event_RestartGame),
    EVENT(EV_DontDestroyOnLoad, GameWorld::Event_DontDestroyOnLoad),
END_EVENTS

void GameWorld::RegisterProperties() {
}

GameWorld::GameWorld() {
    // Create render world.
    renderWorld = renderSystem.AllocRenderWorld();

    // Create physics world.
    physicsWorld = physicsSystem.AllocPhysicsWorld();

    // Create render settings.
    mapRenderSettings = static_cast<MapRenderSettings *>(MapRenderSettings::metaObject.CreateInstance());
    mapRenderSettings->gameWorld = this;

    luaVM.Init();

    Reset();
}

GameWorld::~GameWorld() {
    ClearEntities();

    StaticBatch::ClearAllStaticBatches();

    if (mapRenderSettings) {
        MapRenderSettings::DestroyInstanceImmediate(mapRenderSettings);
    }

    // Free render world.
    renderSystem.FreeRenderWorld(renderWorld);

    // Free physics world.
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

void GameWorld::DontDestroyOnLoad(Entity *entity) {
    if (entity->GetParent()) {
        BE_WARNLOG("DontDestroyOnLoad only works on root entity\n");
        return;
    }

    PostEvent(&EV_DontDestroyOnLoad, entity);
}

void GameWorld::ClearEntities(bool clearAll) {
    // List up all of the entities to remove in depth first order.
    EntityPtrArray entitiesToRemove(1024);

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        if (clearAll || sceneIndex != DontDestroyOnLoadSceneNum) {
            for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNext()) {
                entitiesToRemove.Append(ent);
            }
        }
    }

    // Remove entities in reverse depth first order.
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
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNextSibling()) {
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

    for (Entity *ent = entity->node.GetFirstChild(); ent; ent = ent->node.GetNextSibling()) {
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

Entity *GameWorld::FindEntityByRenderObject(int renderObjectHandle) const {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNext()) {
            if (ent->HasRenderObject(renderObjectHandle)) {
                return ent;
            }
        }
    }
    return nullptr;
}

void GameWorld::OnApplicationResize(int width, int height) {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNext()) {
            ent->OnApplicationResize(width, height);
        }
    }
}

void GameWorld::OnApplicationTerminate() {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNext()) {
            ent->OnApplicationTerminate();
        }
    }
}

void GameWorld::OnApplicationPause(bool pause) {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNext()) {
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

    // If entityIndex is not given, find a blank space in entities[].
    if (entityIndex < 0) {
        while (entities[firstFreeIndex] && firstFreeIndex < MaxEntityNum) {
            firstFreeIndex++;
        }
    
        if (firstFreeIndex >= MaxEntityNum) {
            BE_FATALERROR("no free entities");
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
        BE_WARNLOG("GameWorld::UnregisterEntity: Entity '%s' is already unregistered\n", ent->GetName().c_str());
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

Entity *GameWorld::CloneEntity(const Entity *sourceEntity) {
    // Serialize source entity and it's children.
    Json::Value sourceEntitiesValue;
    Entity::SerializeHierarchy(sourceEntity, sourceEntitiesValue);

    // Clone entities value which is replaced by new GUIDs.
    HashTable<Guid, Guid> guidMap;
    Json::Value clonedEntitiesValue = Entity::CloneEntitiesValue(sourceEntitiesValue, guidMap);

    EntityPtrArray clonedEntities;

    for (int i = 0; i < clonedEntitiesValue.size(); i++) {
        // Create cloned entity.
        Entity *clonedEntity = Entity::CreateEntity(clonedEntitiesValue[i], this, sourceEntity->sceneNum);
        clonedEntities.Append(clonedEntity);

        // Remap all GUID references to newly created.
        clonedEntity->RemapGuids(guidMap);

        // If source entity is prefab source, mark cloned entity originated from prefab entity.
        if (sourceEntitiesValue[i]["prefab"].asBool()) {
            clonedEntity->SetProperty("prefabSource", Guid::FromString(sourceEntitiesValue[i]["guid"].asCString()));
            clonedEntity->SetProperty("prefab", false);
        }

        clonedEntity->Init();
        clonedEntity->InitComponents();
    }

    for (int i = 0; i < clonedEntities.Count(); i++) {
        clonedEntities[i]->LateInitComponents();
    }

    return clonedEntities[0];
}

Entity *GameWorld::CreateEmptyEntityWithPositionAndRotation(const char *name, const Vec3 &position, const Quat &rotation) {
    Json::Value value;
    value["name"] = name;

    value["components"][0]["classname"] = ComTransform::metaObject.ClassName();
    value["components"][0]["origin"] = position.ToString();
    value["components"][0]["rotation"] = rotation.ToString() ;

    Entity *entity = Entity::CreateEntity(value, this);

    entity->Init();
    entity->InitComponents();
    entity->LateInitComponents();

    RegisterEntity(entity);

    return entity;
}

Entity *GameWorld::InstantiateEntity(const Entity *originalEntity) {
    Entity *clonedEntity = CloneEntity(originalEntity);

    RegisterEntity(clonedEntity);

    EntityPtrArray children;
    clonedEntity->GetChildrenRecursive(children);

    for (int i = 0; i < children.Count(); i++) {
        RegisterEntity(children[i]);
    }

    return clonedEntity;
}

Entity *GameWorld::InstantiateEntityWithTransform(const Entity *originalEntity, const Vec3 &origin, const Quat &rotation) {
    Entity *clonedEntity = CloneEntity(originalEntity);
    
    ComTransform *transform = clonedEntity->GetTransform();
    transform->SetOriginRotation(origin, rotation);

    RegisterEntity(clonedEntity);

    EntityPtrArray children;
    clonedEntity->GetChildrenRecursive(children);

    for (int i = 0; i < children.Count(); i++) {
        RegisterEntity(children[i]);
    }

    return clonedEntity;
}

Entity *GameWorld::SpawnEntityFromJson(const Json::Value &entityValue, int sceneIndex) {
    const char *classname = entityValue["classname"].asCString();
    if (Str::Cmp(classname, Entity::metaObject.ClassName()) != 0) {
        BE_WARNLOG("GameWorld::SpawnEntityFromJson: Bad classname '%s' for entity\n", classname);
        return nullptr;
    }

    Entity *entity = Entity::CreateEntity(entityValue, this, sceneIndex);

    entity->Init();
    entity->InitComponents();
    entity->LateInitComponents();

    int spawn_entnum = entityValue.get("spawn_entnum", -1).asInt();

    RegisterEntity(entity, spawn_entnum);

    return entity;
}

void GameWorld::SpawnEntitiesFromJson(const Json::Value &entitiesValue, int sceneIndex) {
    EntityPtrArray entities;

    for (int i = 0; i < entitiesValue.size(); i++) {
        Json::Value entityValue = entitiesValue[i];

        const char *classname = entityValue["classname"].asCString();
        if (Str::Cmp(classname, Entity::metaObject.ClassName()) != 0) {
            BE_WARNLOG("GameWorld::SpawnEntitiesFromJson: Bad classname '%s' for entity\n", classname);
            continue;
        }

        Entity *entity = Entity::CreateEntity(entityValue, this, sceneIndex);

        entity->Init();
        entity->InitComponents();

        entities.Append(entity);
    }

    for (int i = 0; i < entities.Count(); i++) {
        Entity *entity = entities[i];
        entity->LateInitComponents();

        Json::Value entityValue = entitiesValue[i];
        int spawn_entnum = entityValue.get("spawn_entnum", -1).asInt();

        RegisterEntity(entity, spawn_entnum);
    }
}

void GameWorld::BeginMapLoading() {
    isMapLoading = true;

    fontManager.ClearAtlasTextures();
}

void GameWorld::FinishMapLoading() {
    isMapLoading = false;

    renderWorld->FinishMapLoading();

    animControllerManager.DestroyUnusedAnimControllers();
    particleSystemManager.DestroyUnusedParticleSystems();
    skeletonManager.DestroyUnusedSkeletons();
    animManager.DestroyUnusedAnims();
    skinManager.DestroyUnusedSkins();
    meshManager.DestroyUnusedMeshes();
    fontManager.DestroyUnusedFonts();
    materialManager.DestroyUnusedMaterials();
    shaderManager.DestroyUnusedShaders();
    textureManager.DestroyUnusedTextures();
    colliderManager.DestroyUnusedColliders();
    soundSystem.DestroyUnusedSounds();

    //meshManager.EndLevelLoad();

    StaticBatch::ClearAllStaticBatches();
}

bool GameWorld::HasScriptError() const {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNext()) {
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

    if (gameClient.IsStatisticsVisible()) {
        BE_PROFILE_START();
    }

    timeScale = 1.0f;

    if (isDebuggable) {
        luaVM.StartDebuggee();
    }

    luaVM.ClearWaitingThreads();

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        StaticBatch::CombineAll(scenes[sceneIndex].root);
    }

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetNextLeaf(); ent; ent = ent->node.GetNextLeaf()) {
            ent->UpdateUpdatableInHierarchyFlags();
        }
    }

    gameAwaking = true;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNext()) {
            if (!ent->IsAwaked()) {
                ent->Awake();
            }
        }
    }

    gameAwaking = false;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNext()) {
            if (!ent->IsStarted()) {
                ent->Start();
            }
        }
    }

    luaVM.State().ForceGC();

    physicsWorld->Connect(&PhysicsWorld::SIG_PreStep, this, (SignalCallback)&GameWorld::FixedUpdateEntities);
    //physicsWorld->Connect(&PhysicsWorld::SIG_PostStep, this, (SignalCallback)&GameWorld::FixedLateUpdateEntities);
}

void GameWorld::StopGame(bool stopAllSounds) {
    gameStarted = false;

    BE_PROFILE_STOP();
    
    if (isDebuggable) {
        luaVM.StopDebuggee();
    }

    luaVM.ClearWaitingThreads();

    if (stopAllSounds) {
        soundSystem.StopAllSounds();
    }

    physicsWorld->Disconnect(&PhysicsWorld::SIG_PreStep, this);
    physicsWorld->Disconnect(&PhysicsWorld::SIG_PostStep, this);
}

void GameWorld::RestartGame(const char *mapFilename) {
    PostEvent(&EV_RestartGame, mapFilename);
}

void GameWorld::StopAllSounds() {
    soundSystem.StopAllSounds();
}

void GameWorld::Event_RestartGame(const char *mapFilename) {
    StopGame(false);
    
    LoadMap(mapFilename, LoadSceneMode::Single);
    
    StartGame();
}

void GameWorld::Event_DontDestroyOnLoad(Entity *entity) {
    if (entity->node.IsParentedBy(scenes[DontDestroyOnLoadSceneNum].root)) {
        return;
    }
    EntityPtrArray children;
    entity->GetChildrenRecursive(children);

    for (int i = 0; i < children.Count(); i++) {
        children[i]->sceneNum = DontDestroyOnLoadSceneNum;
    }
    // Change parent of root entity to reserved scene root.
    entity->GetRoot()->node.SetParent(scenes[DontDestroyOnLoadSceneNum].root);
}

void GameWorld::NewMap() {
    ClearEntities();

    Reset();

    BeginMapLoading();

    Json::Value defaultMapRenderSettingsValue;
    defaultMapRenderSettingsValue["classname"] = MapRenderSettings::metaObject.ClassName();

    mapRenderSettings->Deserialize(defaultMapRenderSettingsValue);

    FinishMapLoading();
}

bool GameWorld::LoadMap(const char *filename, LoadSceneMode::Enum mode) {
    BE_LOG("Loading map '%s'...\n", filename);

    if (mode != LoadSceneMode::Additive) {
        ClearEntities(mode == LoadSceneMode::Editor);
    }

    Reset();

    BeginMapLoading();

    char *text = nullptr;
    fileSystem.LoadFile(filename, true, (void **)&text);
    if (!text) {
        BE_WARNLOG("Couldn't load '%s'\n", filename);
        FinishMapLoading();
        return false;
    }

    mapFilename = filename;

    Json::Value map;
    Json::Reader jsonReader;
    if (!jsonReader.parse(text, map)) {
        BE_WARNLOG("Failed to parse JSON text\n");
        return false;
    }

    fileSystem.FreeFile(text);

    // Read map version.
    int mapVersion = map["version"].asInt();

    // Read map render settings.
    mapRenderSettings->Deserialize(map["renderSettings"]);
    mapRenderSettings->Init();

    // Find empty scene index.
    int sceneIndex = 0;
    for (; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        if (!scenes[sceneIndex].root.GetFirstChild()) {
            break;
        }
    }

    assert(sceneIndex < COUNT_OF(scenes));

    // Read and spawn all entities.
    SpawnEntitiesFromJson(map["entities"], sceneIndex);

    FinishMapLoading();

    return true;
}

void GameWorld::SaveMap(const char *filename) {
    BE_LOG("Saving map '%s'...\n", filename);

    Json::Value map;

    // Serialize map version.
    map["version"] = 1;

    // Serialize map render settings.
    mapRenderSettings->Serialize(map["renderSettings"]);

    // Serialize all entities in the main scene.
    for (Entity *ent = scenes[0].root.GetFirstChild(); ent; ent = ent->node.GetNextSibling()) {
        Entity::SerializeHierarchy(ent, map["entities"]);
    }

    Json::StyledWriter jsonWriter;
    Str jsonText = jsonWriter.write(map).c_str();

    fileSystem.WriteFile(filename, jsonText.c_str(), jsonText.Length());
}

int GameWorld::GetDeltaTime() const {
    return deltaTime;
}

int GameWorld::GetUnscaledDeltaTime() const {
    return unscaledDeltaTime;
}

void GameWorld::Update(int elapsedMsec) {
    BE_PROFILE_CPU_SCOPE_STATIC("GameWorld::Update");

    if (isDebuggable) {
        luaVM.PollDebuggee();
    }

    prevTime = time;

    unscaledDeltaTime = elapsedMsec;

    deltaTime = unscaledDeltaTime * timeScale;

    time += deltaTime;

    if (gameStarted) {
        // FixedUpdate() is called in StepSimulation() internally.
        physicsWorld->StepSimulation(MILLI2SEC(deltaTime));

        UpdateEntities();

        LateUpdateEntities();

        UpdateLuaVM();
    }
}

void GameWorld::FixedUpdateEntities(float timeStep) {
    BE_PROFILE_CPU_SCOPE_STATIC("GameWorld::FixedUpdateEntities");

    float scaledTimeStep = timeStep * timeScale;

    Entity *next;

    // Call fixed update function for each entities in depth-first order.
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = next) {
            if (!(ent->flags & Entity::Flag::FixedUpdatableInHierarchy)) {
                next = ent->node.GetNextSibling();
                if (next) {
                    continue;
                }
            }
            ent->FixedUpdate(scaledTimeStep);
            next = ent->node.GetNext();
        }
    }
}

void GameWorld::UpdateEntities() {
    BE_PROFILE_CPU_SCOPE_STATIC("GameWorld::UpdateEntities");

    Entity *next;

    // Call update function for each entities in depth-first order.
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = next) {
            if (!(ent->flags & Entity::Flag::UpdatableInHierarchy)) {
                next = ent->node.GetNextSibling();
                if (next) {
                    continue;
                }
            }
            ent->Update();
            next = ent->node.GetNext();
        }
    }
}

void GameWorld::LateUpdateEntities() {
    BE_PROFILE_CPU_SCOPE_STATIC("GameWorld::LateUpdateEntities");

    Entity *next;

    // Call post-update function for each entities in depth-first order.
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = next) {
            if (!(ent->flags & Entity::Flag::LateUpdatableInHierarchy)) {
                next = ent->node.GetNextSibling();
                if (next) {
                    continue;
                }
            }
            ent->LateUpdate();
            next = ent->node.GetNext();
        }
    }
}

void GameWorld::UpdateLuaVM() {
    BE_PROFILE_CPU_SCOPE_STATIC("GameWorld::UpdateLuaVM");

    // Wake up waiting coroutine in Lua scripts.
    luaVM.WakeUpWaitingThreads(MILLI2SEC(time));

    luaVM.State().ForceGC();
}

void GameWorld::UpdateCanvas() {
    Entity *next;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = next) {
            // FIXME
            if (ent->IsLockedInHierarchy()) {
                next = ent->node.GetNextSibling();
                if (next) {
                    continue;
                }
            }
            next = ent->node.GetNext();

            ComCanvas *canvas = ent->GetComponent<ComCanvas>();
            if (!canvas) {
                continue;
            }
            canvas->UpdateRenderingOrderForCanvasElements();
        }
    }
}

void GameWorld::ListUpActiveCameraComponents(StaticArray<ComCamera *, MaxActiveCameraComponents> &cameraComponents) const {
    Entity *next;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = next) {
            // FIXME
            if (ent->IsLockedInHierarchy()) {
                next = ent->node.GetNextSibling();
                if (next) {
                    continue;
                }
            }
            next = ent->node.GetNext();

            ComCamera *camera = ent->GetComponent<ComCamera>();
            if (!camera || !camera->IsActiveInHierarchy()) {
                continue;
            }

            // NOTE: Support just only (MaxActiveCameraComponents == 16) camera components. Need to fix this ?
            if (cameraComponents.Append(camera) == -1) {
                break;
            }
        }
    }

    auto compareFunc = [](const ComCamera *arg1, const ComCamera *arg2) -> bool {
        return arg1->GetOrder() < arg2->GetOrder() ? true : false;
    };
    cameraComponents.Sort(compareFunc);
}

void GameWorld::ListUpActiveCanvasComponents(StaticArray<ComCanvas *, MaxActiveCanvasComponents> &canvasComponents) const {
    Entity *next;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = next) {
            // FIXME
            if (ent->IsLockedInHierarchy()) {
                next = ent->node.GetNextSibling();
                if (next) {
                    continue;
                }
            }
            next = ent->node.GetNext();

            ComCanvas *canvas = ent->GetComponent<ComCanvas>();
            if (!canvas || !canvas->IsActiveInHierarchy()) {
                continue;
            }

            if (canvasComponents.Append(canvas) == -1) {
                break;
            }
        }
    }
}

void GameWorld::RenderCamera() {
    BE_PROFILE_CPU_SCOPE_STATIC("GameWorld::RenderCamera");

    StaticArray<ComCamera *, MaxActiveCameraComponents> cameraComponents;
    ListUpActiveCameraComponents(cameraComponents);

    // Render camera in order.
    for (int i = 0; i < cameraComponents.Count(); i++) {
        cameraComponents[i]->Render();
    }

    StaticArray<ComCanvas *, MaxActiveCanvasComponents> canvasComponents;
    ListUpActiveCanvasComponents(canvasComponents);

    // Render canvas in order.
    for (int i = 0; i < canvasComponents.Count(); i++) {
        canvasComponents[i]->Render();
    }
}

void GameWorld::ProcessPointerInput() {
    if (!gameStarted) {
        return;
    }

    if (!inputSystem.IsUpdated()) {
        return;
    }

    StaticArray<ComCamera *, MaxActiveCameraComponents> cameraComponents;
    ListUpActiveCameraComponents(cameraComponents);

    // Process pointer input in reverse order.
    for (int i = cameraComponents.Count() - 1; i >= 0; i--) {
        ComCamera *cameraComponent = cameraComponents[i];

        if (cameraComponent->ProcessMousePointerInput() || cameraComponent->ProcessTouchPointerInput()) {
            break;
        }
    }

    StaticArray<ComCanvas *, MaxActiveCanvasComponents> canvasComponents;
    ListUpActiveCanvasComponents(canvasComponents);

    // Process pointer input in reverse order.
    for (int i = canvasComponents.Count() - 1; i >= 0; i--) {
        ComCanvas *canvasComponent = canvasComponents[i];

        if (canvasComponent->ProcessMousePointerInput() || canvasComponent->ProcessTouchPointerInput()) {
            break;
        }
    }
}

Entity *GameWorld::IntersectRay(const Ray &ray, int layerMask) const {
    Entity *minEntity = nullptr;
    float minDist = FLT_MAX;
    Vec3 minDistNormal = Vec3::unitX;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNext()) {
            if (!(BIT(ent->GetLayer()) & layerMask)) {
                continue;
            }

            if (ent->IntersectRay(ray, true, minDist, minDistNormal)) {
                minEntity = ent;
            }
        }
    }

    return minEntity;
}

Entity *GameWorld::IntersectRay(const Ray &ray, int layerMask, const Array<Entity *> &excludingEntities, float *hitDist, Vec3 *hitNormal) const {
    float minDist = FLT_MAX;
    Vec3 minDistNormal = Vec3::unitX;

    if (hitDist) {
        *hitDist = minDist;
    }

    Entity *minEntity = nullptr;

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNext()) {
            if (!(BIT(ent->GetLayer()) & layerMask)) {
                continue;
            }

            if (excludingEntities.Find(ent)) {
                continue;
            }

            if (ent->IntersectRay(ray, true, minDist, minDistNormal)) {
                minEntity = ent;
                if (hitDist) {
                    *hitDist = minDist;
                }
                if (hitNormal) {
                    *hitNormal = minDistNormal;
                }
            }
        }
    }

    return minEntity;
}

Entity *GameWorld::RayCast(const Ray &ray, int layerMask) const {
    CastResult castResult;

    if (!GetPhysicsWorld()->RayCast(ray.origin, ray.GetPoint(MeterToUnit(10000.0f)), layerMask, castResult)) {
        return nullptr;
    }

    ComRigidBody *hitTestRigidBody = GetRigidBodyFromCastResult(castResult);
    if (hitTestRigidBody) {
        return hitTestRigidBody->GetEntity();
    }

    return nullptr;
}

int GameWorld::OverlapBox(const Vec3 &boxCenter, const Vec3 &boxExtents, int layerMask, Array<Entity *> &entities) const {
    Array<PhysCollidable *> colliders;

    GetPhysicsWorld()->OverlapBox(boxCenter, boxExtents, layerMask, colliders);

    for (int i = 0; i < colliders.Count(); i++) {
        const PhysCollidable *collidable = colliders[i];

        if (!collidable->GetUserPointer()) {
            continue;
        }
        ComRigidBody *rigidBodyComponent = (reinterpret_cast<Component *>(collidable->GetUserPointer()))->Cast<ComRigidBody>();
        if (!rigidBodyComponent) {
            continue;
        }
        entities.Append(rigidBodyComponent->GetEntity());
    }
    return entities.Count();
}

int GameWorld::OverlapSphere(const Vec3 &sphereCenter, float sphereRadius, int layerMask, Array<Entity *> &entities) const {
    Array<PhysCollidable *> colliders;

    GetPhysicsWorld()->OverlapSphere(sphereCenter, sphereRadius, layerMask, colliders);

    for (int i = 0; i < colliders.Count(); i++) {
        const PhysCollidable *collidable = colliders[i];

        if (!collidable->GetUserPointer()) {
            continue;
        }
        ComRigidBody *rigidBodyComponent = (reinterpret_cast<Component *>(collidable->GetUserPointer()))->Cast<ComRigidBody>();
        if (!rigidBodyComponent) {
            continue;
        }
        entities.Append(rigidBodyComponent->GetEntity());
    }    
    return entities.Count();
}

int GameWorld::OverlapTriangle(const Vec3 &a, const Vec3 &b, const Vec3 &c, int layerMask, Array<Entity *> &entities) const {
    Array<PhysCollidable *> colliders;

    GetPhysicsWorld()->OverlapTriangle(a, b, c, layerMask, colliders);

    for (int i = 0; i < colliders.Count(); i++) {
        const PhysCollidable *collidable = colliders[i];

        if (!collidable->GetUserPointer()) {
            continue;
        }
        ComRigidBody *rigidBodyComponent = (reinterpret_cast<Component *>(collidable->GetUserPointer()))->Cast<ComRigidBody>();
        if (!rigidBodyComponent) {
            continue;
        }
        entities.Append(rigidBodyComponent->GetEntity());
    }
    return entities.Count();
}

void GameWorld::SaveSnapshot() {
    snapshotValues.clear();

    mapRenderSettings->Serialize(snapshotValues["renderSettings"]);

    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetFirstChild(); ent; ent = ent->node.GetNextSibling()) {
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
