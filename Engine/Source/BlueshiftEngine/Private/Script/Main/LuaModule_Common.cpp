#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Main/Common.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterCommon(LuaCpp::Module &module) {
    LuaCpp::Selector _Common = module["Common"];

    _Common.SetObj(common);
    _Common.AddObjMembers(common,
        "real_time", &Common::realTime,
        "frame_time", &Common::frameTime,
        "frame_sec", &Common::frameSec);
}

BE_NAMESPACE_END