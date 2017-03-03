#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComAudioSource.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterAudioSourceComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComAudioSource = module["ComAudioSource"];

    _ComAudioSource.SetClass<ComAudioSource>(module["Component"]);
    _ComAudioSource.AddClassMembers<ComAudioSource>(
        "play", &ComAudioSource::Play,
        "stop", &ComAudioSource::Stop);
    
    _ComAudioSource["meta_object"] = ComAudioSource::metaObject;
}

BE_NAMESPACE_END