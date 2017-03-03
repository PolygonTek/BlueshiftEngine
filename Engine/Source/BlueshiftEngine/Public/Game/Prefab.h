#pragma once

#include "Entity.h"

BE_NAMESPACE_BEGIN

class Component;

class Prefab : public Object {
    friend class PrefabManager;

public:
    OBJECT_PROTOTYPE(Prefab);

    Prefab() {}
    virtual ~Prefab();

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
    EntityPtrArray               entities;
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
    void                        Init();
    void                        Shutdown();

    Prefab *                    AllocPrefab(const char *hashName, const Guid guid = Guid::zero);
    Prefab *                    FindPrefab(const char *hashName) const;
    Prefab *                    GetPrefab(const char *filename);

    static Json::Value          CreatePrefabValue(const Entity *entity);

private:
    StrIHashMap<Prefab *>       prefabHashMap;
};

extern PrefabManager            prefabManager;

BE_NAMESPACE_END
