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

    Game World

-------------------------------------------------------------------------------
*/

#include "Entity.h"

BE_NAMESPACE_BEGIN

class RenderContext;
class RenderWorld;
class PhysicsWorld;
class Prefab;
class TagLayerSettings;
class PhysicsSettings;

class GameWorld : public Object {
    friend class GameEdit;

public:
    enum { 
        EntityNumBits           = 13,
        MaxEntities             = (1 << EntityNumBits),
        BadEntityNum            = (MaxEntities - 1),
        MaxEntityNum            = (MaxEntities - 2)
    };

    OBJECT_PROTOTYPE(GameWorld);

    GameWorld();
    ~GameWorld();

    virtual const Str           ToString() const override { return "Game World"; }

    RenderWorld *               GetRenderWorld() const { return renderWorld; }
    PhysicsWorld *              GetPhysicsWorld() const { return physicsWorld; }

    int                         GetTime() const { return time; }
    int                         GetPrevTime() const { return prevTime; }
    int                         GetDeltaTime() const;

    float                       GetTimeScale() const { return timeScale; }
    void                        SetTimeScale(float timeScale) { this->timeScale = timeScale; }
    
                                // Reset all entities in this game world
    void                        Reset();

                                // Simulate physics system and update all registered entities 
    void                        Update(int elapsedTime);

                                // Process mouse (touch) input feedback for all responsive entities
    void                        ProcessPointerInput();

                                // Ray intersection test for all entities
    Entity *                    RayIntersection(const BE1::Vec3 &start, const BE1::Vec3 &dir, const BE1::Array<BE1::Entity *> &excludingList, float *scale) const;

                                // Render camera component from all registered entities
    void                        RenderCamera();
    
    Hierarchy<Entity> &         GetEntityHierarchy() { return entityHierarchy; }
    Entity *                    GetEntity(int index) const { return entities[index]; }
    int                         GetEntitySpawnId(const Entity *ent);

    Entity *                    FindEntity(const char *name) const;
    Entity *                    FindEntityByGuid(const Guid &guid) const;
    Entity *                    FindEntityByTag(const char *tag) const;
    const EntityPtrArray        FindEntitiesByTag(const char *tag) const;
    Entity *                    FindEntityByRenderEntity(int renderEntityHandle) const;

    void                        OnEntityNameChanged(Entity *ent);
    void                        OnEntityTagChanged(Entity *ent);

    bool                        IsRegisteredEntity(const Entity *ent) const;
    void                        RegisterEntity(Entity *ent, int spawn_entnum = -1);
    void                        UnregisterEntity(Entity *ent);

    Entity *                    InstantiateEntity(const Entity *originalEntity);
    Entity *                    InstantiateEntityWithTransform(const Entity *originalEntity, const Vec3 &origin, const Angles &angles);

    bool                        SpawnEntityFromJson(Json::Value &entityValue, Entity **ent = nullptr);
    void                        SpawnEntitiesFromJson(Json::Value &entitiesValue);
    void                        SpawnEntitiesFromString(const char *entityString);

    static void                 SerializeEntityHierarchy(const Hierarchy<Entity> &entityHierarchy, Json::Value &entitiesValue);

    void                        SaveSnapshot();
    void                        RestoreSnapshot();

    void                        BeginMapLoading();
    void                        FinishMapLoading();
    
    void                        OnApplicationPause(bool pause);
    void                        OnApplicationTerminate();

    bool                        IsGameStarted() const { return gameStarted; }
    void                        StartGame();
    void                        StopGame();
    void                        RestartGame(const char *mapName);

    TagLayerSettings *          GetTagLayerSettings() { return tagLayerSettings; }

    void                        LoadSettings();
    void                        SaveSettings();

    void                        LoadTagLayerSettings(const char *filename);
    void                        LoadInputSettings(const char *filename);
    void                        LoadPhysicsSettings(const char *filename);

    const char *                MapName() const { return mapName.c_str(); }

    bool                        LoadMap(const char *filename);
    void                        SaveMap(const char *filename);

    static const SignalDef      SIG_EntityRegistered;
    static const SignalDef      SIG_EntityUnregistered;
    
private:
    void                        Event_RestartGame(const char *mapName);

    Entity *                    CloneEntity(const Entity *originalEntity);
    void                        SaveObject(const char *filename, const Object *object) const;
    void                        ClearAllEntities();
    void                        UpdateEntities();   

    Entity *                    entities[MaxEntities];
    HashIndex                   entityHash;
    HashIndex                   entityTagHash;
    int                         firstFreeIndex;
    int                         spawnIds[MaxEntities];
    int                         spawnCount;
    Hierarchy<Entity>           entityHierarchy;

    Json::Value                 snapshotValues;

    Str                         mapName;

    TagLayerSettings *          tagLayerSettings; 
    PhysicsSettings *           physicsSettings;
        
    RenderWorld *               renderWorld;
    PhysicsWorld *              physicsWorld;

    int                         time;
    int                         prevTime;
    float                       timeScale;

    bool                        gameStarted;
    bool                        isMapLoading;
};

BE_NAMESPACE_END
