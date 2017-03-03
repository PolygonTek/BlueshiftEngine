#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComAudioListener.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterAudioListenerComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComAudioListener = module["ComAudioListener"];

    _ComAudioListener.SetClass<ComAudioListener>(module["Component"]);
    
    _ComAudioListener["meta_object"] = ComAudioListener::metaObject;
}

BE_NAMESPACE_END