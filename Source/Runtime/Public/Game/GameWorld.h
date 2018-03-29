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
#include "Script/LuaVM.h"

BE_NAMESPACE_BEGIN

class RenderContext;
class RenderWorld;
class PhysicsWorld;
class Prefab;
class TagLayerSettings;
class PhysicsSettings;
class MapRenderSettings;
class PlayerSettings;

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

    virtual Str                 ToString() const override { return "Game World"; }

    RenderWorld *               GetRenderWorld() const { return renderWorld; }
    PhysicsWorld *              GetPhysicsWorld() const { return physicsWorld; }

    LuaVM &                     GetLuaVM() { return luaVM; }

    void                        SetDebuggable(bool isDebuggable) { this->isDebuggable = isDebuggable; }

    int                         GetTime() const { return time; }
    int                         GetPrevTime() const { return prevTime; }
    int                         GetDeltaTime() const;

    float                       GetTimeScale() const { return timeScale; }
    void                        SetTimeScale(float timeScale) { this->timeScale = timeScale; }
    
                                /// Reset all entities in this game world
    void                        Reset();

                                /// Simulates physics system and update all registered entities 
    void                        Update(int elapsedTime);

                                /// Process mouse (touch) input feedback for all responsive entities
    void                        ProcessPointerInput();

                                /// Ray intersection test for all entities
    Entity *                    RayIntersection(const Vec3 &start, const Vec3 &dir, const Array<Entity *> &excludingList, float *scale) const;

                                /// Render camera component from all registered entities
    void                        RenderCamera();
    
                                /// Returns entity hierarchy.
    Hierarchy<Entity> &         GetEntityHierarchy() { return entityHierarchy; }

                                /// Returns the entity with the given entity index.
    Entity *                    GetEntity(int index) const { return entities[index]; }

                                /// Returns the entity that have given name.
    Entity *                    FindEntity(const char *name) const;
                                /// Returns the entity that have given GUID.
    Entity *                    FindEntityByGuid(const Guid &guid) const;
                                /// Returns the entity that have given tag.
    Entity *                    FindEntityByTag(const char *tag) const;
                                /// Returns all of the entities that match given tag.
    const EntityPtrArray        FindEntitiesByTag(const char *tag) const;
                                /// Returns an entity by render entity handle.
    Entity *                    FindEntityByRenderEntity(int renderEntityHandle) const;

                                /// Called when entity name changed.
    void                        OnEntityNameChanged(Entity *ent);

                                /// Called when entity tag changed.
    void                        OnEntityTagChanged(Entity *ent);

    bool                        IsRegisteredEntity(const Entity *ent) const;
    void                        RegisterEntity(Entity *ent, int spawn_entnum = -1);
    void                        UnregisterEntity(Entity *ent);

                                /// Creates an entity that has no components but transform component
    Entity *                    CreateEmptyEntity(const char *name);

    Entity *                    InstantiateEntity(const Entity *originalEntity);
    Entity *                    InstantiateEntityWithTransform(const Entity *originalEntity, const Vec3 &origin, const Angles &angles);

    bool                        SpawnEntityFromJson(Json::Value &entityValue, Entity **ent = nullptr);
    void                        SpawnEntitiesFromJson(Json::Value &entitiesValue);

    void                        SaveSnapshot();
    void                        RestoreSnapshot();
    
    void                        OnApplicationPause(bool pause);
    void                        OnApplicationTerminate();

    bool                        CheckScriptError() const;

    bool                        IsGameStarted() const { return gameStarted; }
    void                        StartGame();
    void                        StopGame(bool stopAllSounds = true);
    void                        RestartGame(const char *mapName);

    void                        StopAllSounds();
        
    const char *                MapName() const { return mapName.c_str(); }

    void                        NewMap();
    bool                        LoadMap(const char *filename);
    void                        SaveMap(const char *filename);

    static const SignalDef      SIG_EntityRegistered;
    static const SignalDef      SIG_EntityUnregistered;
    
private:
    void                        Event_RestartGame(const char *mapName);

    void                        BeginMapLoading();
    void                        FinishMapLoading();
    Entity *                    CloneEntity(const Entity *originalEntity);
    void                        SaveObject(const char *filename, const Object *object) const;
    void                        ClearAllEntities();
    void                        FixedUpdateEntities(float timeStep);
    void                        FixedLateUpdateEntities(float timeStep);
    void                        UpdateEntities();
    void                        LateUpdateEntities();

    Entity *                    entities[MaxEntities];
    HashIndex                   entityHash;
    HashIndex                   entityTagHash;
    int                         firstFreeIndex;
    Hierarchy<Entity>           entityHierarchy;

    Json::Value                 snapshotValues;

    LuaVM                       luaVM;

    Str                         mapName;

    MapRenderSettings *         mapRenderSettings;

    RenderWorld *               renderWorld;
    PhysicsWorld *              physicsWorld;

    int                         time;
    int                         prevTime;
    float                       timeScale;

    bool                        gameStarted;
    bool                        isDebuggable;
    bool                        isMapLoading;
};

BE_NAMESPACE_END
