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
#include "Scripting/LuaVM.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterGameWorld(LuaCpp::Module &module) {
    LuaCpp::Selector _GameWorld = module["GameWorld"];

    _GameWorld.SetClass<GameWorld>(module["Object"]);
    _GameWorld.AddClassMembers<GameWorld>(
        "time", &GameWorld::GetTime,
        "prev_time", &GameWorld::GetPrevTime,
        "unscaled_delta_time", &GameWorld::GetUnscaledDeltaTime,
        "delta_time", &GameWorld::GetDeltaTime,
        "time_scale", &GameWorld::GetTimeScale,
        "set_time_scale", &GameWorld::SetTimeScale,
        "create_empty_entity", &GameWorld::CreateEmptyEntity,
        "create_empty_entity_with_position", &GameWorld::CreateEmptyEntityWithPosition,
        "create_empty_entity_with_position_and_rotation", &GameWorld::CreateEmptyEntityWithPositionAndRotation,
        "find_entity", static_cast<Entity*(GameWorld::*)(const char *)const>(&GameWorld::FindEntity),
        "find_entity_by_name", &GameWorld::FindEntityByName,
        "find_entity_by_tag", &GameWorld::FindEntityByTag,
        "find_entities_by_tag", &GameWorld::FindEntitiesByTag,
        "intersect_ray", static_cast<Entity*(GameWorld::*)(const Ray &, int)const>(&GameWorld::IntersectRay),
        "ray_cast", &GameWorld::RayCast,
        "overlap_box", &GameWorld::OverlapBox,
        "overlap_sphere", &GameWorld::OverlapSphere,
        "overlap_triangle", &GameWorld::OverlapTriangle,
        "instantiate_entity", &GameWorld::InstantiateEntity,
        "instantiate_entity_with_transform", &GameWorld::InstantiateEntityWithTransform,
        "dont_destroy_on_load", &GameWorld::DontDestroyOnLoad,
        "map_filename", &GameWorld::MapFilename,
        "restart_game", &GameWorld::RestartGame,
        "stop_all_sounds", &GameWorld::StopAllSounds);

    _GameWorld["meta_object"] = GameWorld::metaObject;
}

BE_NAMESPACE_END
