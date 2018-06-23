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
#include "Components/ComAnimation.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterAnimationComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComAnimation = module["ComAnimation"];

    _ComAnimation.SetClass<ComAnimation>(module["Component"]);
    _ComAnimation.AddClassMembers<ComAnimation>(
        "time_offset", &ComAnimation::GetTimeOffset,
        "set_time_offset", &ComAnimation::SetTimeOffset,
        "time_scale", &ComAnimation::GetTimeScale,
        "set_time_scale", &ComAnimation::SetTimeScale,
        "current_anim_seconds", &ComAnimation::GetCurrentAnimSeconds);

    _ComAnimation["meta_object"] = ComAnimation::metaObject;
}

BE_NAMESPACE_END
