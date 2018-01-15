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
#include "Script/LuaVM.h"
#include "Components/Component.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _Component = module["Component"];

    _Component.SetClass<Component>(module["Object"]);
    _Component.AddClassMembers<Component>(
        "to_string", &Component::ToString,
        "entity", &Component::GetEntity,
        "gameWorld", &Component::GetGameWorld,
        "can_disable", &Component::CanDisable,
        "is_enabled", &Component::IsEnabled,
        "enable", &Component::SetEnabled,
        "purge", &Component::Purge,
        "is_initialized", &Component::IsInitialized);
        
    _Component["destroy"].SetFunc([](Component *component) {
        Component::DestroyInstance(component);
    });
    
    LuaCpp::Selector _ComponentPtrList = module["ComponentPtrArray"];
    _ComponentPtrList.SetClass<ComponentPtrArray>();
    _ComponentPtrList.AddClassMembers<ComponentPtrArray>(
        "at", static_cast<ComponentPtr&(ComponentPtrArray::*)(int)>(&ComponentPtrArray::operator[]),
        "count", &ComponentPtrArray::Count,
        "append", &ComponentPtrArray::Append<ComponentPtr &>,
        "insert", &ComponentPtrArray::Insert<ComponentPtr &>,
        "find_index", &ComponentPtrArray::FindIndex<ComponentPtr &>,
        "find", &ComponentPtrArray::Find<ComponentPtr &>,
        "remove_index", &ComponentPtrArray::RemoveIndex,
        "remove", &ComponentPtrArray::Remove<ComponentPtr &>,
        "clear", &ComponentPtrArray::Clear);
}

BE_NAMESPACE_END
