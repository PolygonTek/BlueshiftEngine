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

#include "Containers/StaticArray.h"
#include "Entity.h"
#include "Scripting/LuaVM.h"

BE_NAMESPACE_BEGIN

class RenderContext;
class RenderWorld;
class PhysicsWorld;
class Prefab;
class TagLayerSettings;
class PhysicsSettings;
class MapRenderSettings;
class PlayerSettings;
class GameWorld;
class ComCamera;
class ComCanvas;

class GameScene {
public:
    Hierarchy<Entity>           root;
};

class GameWorld : public Object {
public:
    static constexpr int MaxScenes = 16;
    static constexpr int DontDestroyOnLoadSceneNum = MaxScenes - 1;
    static constexpr int EntityNumBits = 16;
    static constexpr int MaxEntities = (1 << EntityNumBits);
    static constexpr int BadEntityNum = (MaxEntities - 1);
    static constexpr int MaxEntityNum = (MaxEntities - 2);

    struct LoadSceneMode {
        enum Enum {
            Single,
            Additive,
            Editor
        };
    };

    OBJECT_PROTOTYPE(GameWorld);

    GameWorld();
    ~GameWorld();

    virtual Str                 ToString() const override { return "Game World"; }

    Random &                    GetRandom() { return random; }

                                /// Returns internal render world interface pointer.
    RenderWorld *               GetRenderWorld() const { return renderWorld; }

                                /// Returns internal physics world interface pointer.
    PhysicsWorld *              GetPhysicsWorld() const { return physicsWorld; }

                                /// Returns Lua VM object.
    LuaVM &                     GetLuaVM() { return luaVM; }

                                /// Enables script debugging when the game started.
    void                        SetDebuggable(bool isDebuggable) { this->isDebuggable = isDebuggable; }

                                /// Returns the elapsed time up to the current frame.
    int                         GetTime() const { return time; }
                                /// Returns the elapsed time up to the previous frame.
    int                         GetPrevTime() const { return prevTime; }
                                /// Returns the delta time between the current frame and the previous frame.
    int                         GetDeltaTime() const;
                                /// Returns the unscaled delta time between the current frame and the previous frame.
    int                         GetUnscaledDeltaTime() const;

                                /// Returns update time scale.
    float                       GetTimeScale() const { return timeScale; }
                                /// Sets update time scale.
    void                        SetTimeScale(float timeScale) { this->timeScale = timeScale; }

                                /// Resets game time and clear debug primitives.
    void                        Reset();

                                /// Destroys all entities in all scenes except those called by DontDestroyOnLoad().
                                /// If clearAll is set, you can force destroy all entities.
    void                        ClearEntities(bool clearAll = true);

                                /// Marks the entity not to be destroyed.
    void                        DontDestroyOnLoad(Entity *entity);

                                /// Simulates physics system and update all registered entities.
    void                        Update(int elapsedMsec);

                                /// Processes mouse & touch input feedback for all responsive entities.
    void                        ProcessPointerInput();

                                /// Ray intersection test for all entities.
    Entity *                    IntersectRay(const Ray &ray, int layerMask) const;
                                /// Ray intersection test for all entities.
    Entity *                    IntersectRay(const Ray &ray, int layerMask, const Array<Entity *> &excludingEntities, float *scale) const;

                                /// Returns an entity intersecting the ray.
    Entity *                    RayCast(const Ray &ray, int layerMask) const;

                                /// Returns an array with all rigid body entities touching or inside the OBB.
    int                         OverlapOBB(const OBB &obb, int layerMask, Array<Entity *> &entities) const;
                                /// Returns an array with all rigid body entities touching or inside the sphere.
    int                         OverlapSphere(const Sphere &sphere, int layerMask, Array<Entity *> &entities) const;
                                /// Returns an array with all rigid body entities touching the triangle.
    int                         OverlapTriangle(const Vec3& a, const Vec3& b, const Vec3& c, int layerMask, Array<Entity *> &entities) const;

                                /// Render camera component from all registered entities.
    void                        RenderCamera();

                                /// Calls function for each entities for each scenes.
    template <typename Func>
    void                        IterateEntities(Func func) const;

                                /// Returns the game scene with the given scene index.
    GameScene &                 GetScene(int sceneIndex) { return scenes[sceneIndex]; }

                                /// Returns the entity with the given entity index.
    Entity *                    GetEntity(int index) const { return entities[index]; }

                                /// Returns the entity that func returns true.
    template <typename Func>
    Entity *                    FindEntity(Func func) const;

                                /// Returns the entity that have given path.
    Entity *                    FindEntity(const char *path) const;
                                /// Returns the entity that have given name.
    Entity *                    FindEntityByName(const char *name) const;
                                /// Returns the entity that have given tag.
    Entity *                    FindEntityByTag(const char *tag) const;
                                /// Returns all of the entities that match given tag.
    const EntityPtrArray        FindEntitiesByTag(const char *tag) const;
                                /// Returns an entity by render object handle.
    Entity *                    FindEntityByRenderObject(int renderObjectHandle) const;

                                /// Called when entity name changed.
    void                        OnEntityNameChanged(Entity *ent);

                                /// Called when entity tag changed.
    void                        OnEntityTagChanged(Entity *ent);

                                /// Returns if given entity is registered in this world.
    bool                        IsRegisteredEntity(const Entity *ent) const;
                                /// Registers given entity to this world.
    void                        RegisterEntity(Entity *ent, int entityIndex = -1);
                                /// Unregisters given entity from this world.
    void                        UnregisterEntity(Entity *ent);

                                /// Creates an entity that has no components but transform component.
    Entity *                    CreateEmptyEntity(const char *name);

    Entity *                    InstantiateEntity(const Entity *originalEntity);
    Entity *                    InstantiateEntityWithTransform(const Entity *originalEntity, const Vec3 &origin, const Quat &rotation);

    Entity *                    SpawnEntityFromJson(const Json::Value &entityValue, int sceneIndex = 0);
    void                        SpawnEntitiesFromJson(const Json::Value &entitiesValue, int sceneIndex = 0);

    void                        SaveSnapshot();
    void                        RestoreSnapshot();

    const char *                MapFilename() const { return mapFilename.c_str(); }

    void                        NewMap();
    bool                        LoadMap(const char *filename, LoadSceneMode::Enum mode);
    void                        SaveMap(const char *filename);
    
    bool                        IsGameStarted() const { return gameStarted; }

    void                        StartGame();
    void                        StopGame(bool stopAllSounds = true);
    void                        RestartGame(const char *mapFilename);

    bool                        HasScriptError() const;

    void                        StopAllSounds();

    void                        UpdateCanvas();

    void                        OnApplicationResize(int width, int height);
    void                        OnApplicationPause(bool pause);
    void                        OnApplicationTerminate();

    static const SignalDef      SIG_EntityRegistered;
    static const SignalDef      SIG_EntityUnregistered;
    
private:
    void                        Event_RestartGame(const char *mapFilename);
    void                        Event_DontDestroyOnLoad(Entity *entity);

    Entity *                    FindRootEntityByName(const char *name) const;
    Entity *                    FindEntityRelativePath(const Entity *entity, const char *path) const;
    void                        BeginMapLoading();
    void                        FinishMapLoading();
    Entity *                    CloneEntity(const Entity *originalEntity);
    void                        FixedUpdateEntities(float timeStep);
    void                        FixedLateUpdateEntities(float timeStep);
    void                        UpdateEntities();
    void                        LateUpdateEntities();
    void                        UpdateLuaVM();

    void                        ListUpActiveCameraComponents(StaticArray<ComCamera *, 16> &cameraComponents) const;
    void                        ListUpActiveCanvasComponents(StaticArray<ComCanvas *, 16> &canvasComponents) const;

    Entity *                    entities[MaxEntities] = { nullptr, };
    HashIndex                   entityHash;
    HashIndex                   entityTagHash;
    int                         firstFreeIndex = 0;

    GameScene                   scenes[MaxScenes];

    Json::Value                 snapshotValues;

    Random                      random;

    LuaVM                       luaVM;

    Str                         mapFilename;

    MapRenderSettings *         mapRenderSettings;

    RenderWorld *               renderWorld;
    PhysicsWorld *              physicsWorld;

    int                         time;
    int                         prevTime;
    float                       timeScale = 1.0f;
    int                         deltaTime = 0;
    int                         unscaledDeltaTime = 0;

    bool                        gameAwaking = false;
    bool                        gameStarted = false;
    bool                        isDebuggable = false;
    bool                        isMapLoading = false;
};

template <typename Func>
BE_INLINE void GameWorld::IterateEntities(Func func) const {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetNext(); ent; ent = ent->GetNode().GetNext()) {
            if (!func(ent)) {
                // Abandon iteration if func returns false.
                return;
            }
        }
    }
}

template <typename Func>
BE_INLINE Entity *GameWorld::FindEntity(Func func) const {
    for (int sceneIndex = 0; sceneIndex < COUNT_OF(scenes); sceneIndex++) {
        for (Entity *ent = scenes[sceneIndex].root.GetNext(); ent; ent = ent->GetNode().GetNext()) {
            if (func(ent)) {
                return ent;
            }
        }
    }
    return nullptr;
}

BE_NAMESPACE_END
