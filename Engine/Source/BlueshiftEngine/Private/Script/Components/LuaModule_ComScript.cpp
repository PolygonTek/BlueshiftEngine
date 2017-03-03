#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComScript.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterScriptComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComScript = module["ComScript"];

    _ComScript.SetClass<ComScript>(module["Component"]);
    _ComScript.AddClassMembers<ComScript>(
        "sandbox_name", &ComScript::GetSandboxName);

    _ComScript["meta_object"] = ComScript::metaObject;
}

BE_NAMESPACE_END