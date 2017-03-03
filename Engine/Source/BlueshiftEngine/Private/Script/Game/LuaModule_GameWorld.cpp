#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterGameWorld(LuaCpp::Module &module) {
    LuaCpp::Selector _GameWorld = module["GameWorld"];

    _GameWorld.SetClass<GameWorld>(module["Object"]);
    _GameWorld.AddClassMembers<GameWorld>(
        "time", &GameWorld::GetTime,
        "prev_time", &GameWorld::GetPrevTime,
        "delta_time", &GameWorld::GetDeltaTime,
        "time_scale", &GameWorld::GetTimeScale,
        "set_time_scale", &GameWorld::SetTimeScale,
        "find_entity", &GameWorld::FindEntity,
        "find_entity_by_guid", &GameWorld::FindEntityByGuid,
        "find_entity_by_tag", &GameWorld::FindEntityByTag,
        "find_entities_by_tag", &GameWorld::FindEntitiesByTag,
        "clone_entity", &GameWorld::CloneEntity,
        "loded_map_name", &GameWorld::MapName,
        "restart_game", &GameWorld::RestartGame);

    _GameWorld["meta_object"] = GameWorld::metaObject;
}

BE_NAMESPACE_END
