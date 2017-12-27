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

#include "Containers/HashMap.h"
#include "Entity.h"

BE_NAMESPACE_BEGIN

class Component;

class Prefab : public Object {
    friend class PrefabManager;

public:
    OBJECT_PROTOTYPE(Prefab);

    Prefab() {}
    virtual ~Prefab();

    const char *                GetName() const { return name; }
    Str                         GetFilename() const { return hashName; }

    Hierarchy<Entity> &         GetEntityHierarchy() { return entityHierarchy; }
    Entity *                    GetRootEntity() { return entityHierarchy.GetChild(); }

    void                        Clear();
    bool                        Create(const Json::Value &entitiesValue);

    bool                        Load(const char *filename);
    void                        Write(const char *filename);

    bool                        Reload();

private:
    Str                         hashName;
    Str                         name;
    EntityPtrArray              entities;
    Hierarchy<Entity>           entityHierarchy;
};

BE_INLINE Prefab::~Prefab() {
    Clear();
}
/*
class PrefabInstance {
public:
    struct Mod {
        Component *             component;
        Str                     name;           // modified property name
        Variant                 value;          // modified property value
    };

    PrefabInstance();
    virtual ~PrefabInstance();

    int                         NumMods() const { return modifications.Count(); }
    Mod *                       GetMod(int index) const { assert(index >= 0 && index < modifications.Count()); return modifications[index]; }
    Mod *                       GetMod(const char *name);
    bool                        AddMod(const char *name, const Variant &value);
    bool                        SetMod(const char *name, const Variant &value);
    bool                        RemoveMod(const char *name);

private:
    Array<Mod *>                modifications;
    Array<Guid>                 removedComponents;
};*/

class PrefabManager {
public:
    PrefabManager() : initialized(false) {}

    void                        Init();
    void                        Shutdown();

    Prefab *                    AllocPrefab(const char *hashName, const Guid &guid = Guid::zero);
    Prefab *                    FindPrefab(const char *hashName) const;
    Prefab *                    GetPrefab(const char *filename);

    void                        RenamePrefab(Prefab *prefab, const Str &newName);

    static Json::Value          CreatePrefabValue(const Entity *entity);

    GameWorld *                 GetPrefabWorld() { return prefabWorld; }

private:
    StrIHashMap<Prefab *>       prefabHashMap;
    GameWorld *                 prefabWorld;

    bool                        initialized;
};

extern PrefabManager            prefabManager;

BE_NAMESPACE_END
