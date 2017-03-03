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
        "enable", &Component::Enable,
        "purge", &Component::Purge,
        "is_initialized", &Component::IsInitalized);
        
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