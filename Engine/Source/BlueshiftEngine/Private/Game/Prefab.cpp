#include "Precompiled.h"
#include "Game/Entity.h"
#include "Game/Prefab.h"
#include "Game/GameWorld.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Prefab", Prefab, Object)
BEGIN_EVENTS(Prefab)
END_EVENTS
BEGIN_PROPERTIES(Prefab)
END_PROPERTIES

void Prefab::Clear() {
    entityHierarchy.RemoveFromHierarchy();

    for (int i = 0; i < entities.Count(); i++) {
        Entity *ent = entities[i];
        Entity::DestroyInstanceImmediate(ent);
        entities[i] = nullptr;
    }

    entities.Clear();
}

bool Prefab::Create(const Json::Value &entitiesValue) {
    for (int i = 0; i < entitiesValue.size(); i++) {
        Json::Value entityValue = entitiesValue[i];
        const char *classname = entityValue["classname"].asCString();

        if (!Str::Cmp(classname, Entity::metaObject.ClassName())) {
            Entity *entity = Entity::CreateEntity(entityValue);

            // all of the entities in the prefab have this property
            assert(entity->props->Get("isPrefabParent").As<bool>());
            
            const Guid parentGuid = Guid::ParseString(entityValue["parent"].asCString());
            if (parentGuid.IsZero()) {
                // guid 0 means a root entity
                entity->node.SetParent(entityHierarchy);
            }

            entity->InitHierarchy();

            entities.Append(entity);
        } else {
            BE_WARNLOG(L"Unknown classname '%hs'\n", classname);
        }
    }

    return true;
}

bool Prefab::Load(const char *filename) {
    char *text = nullptr;

    fileSystem.LoadFile(filename, true, (void **)&text);
    if (!text) {
        BE_WARNLOG(L"Couldn't load '%hs'\n", filename);
        return false;
    }

    Json::Value entitiesValue;
    Json::Reader jsonReader;

    if (!jsonReader.parse(text, entitiesValue)) {
        BE_WARNLOG(L"Failed to parse JSON text\n");
        fileSystem.FreeFile(text);
        return false;
    }

    Create(entitiesValue);

    fileSystem.FreeFile(text);

    return true;
}

void Prefab::Write(const char *filename) {
    Json::Value entitiesValue;
    GameWorld::SerializeEntityHierarchy(entityHierarchy.GetChild()->GetNode(), entitiesValue);

    Json::StyledWriter jsonWriter;
    Str jsonText = jsonWriter.write(entitiesValue).c_str();

    fileSystem.WriteFile(filename, jsonText.c_str(), jsonText.Length());
}

bool Prefab::Reload() {
    Str _hashName = this->hashName;
    bool ret = Load(_hashName);

    return ret;
}

BE_NAMESPACE_END
