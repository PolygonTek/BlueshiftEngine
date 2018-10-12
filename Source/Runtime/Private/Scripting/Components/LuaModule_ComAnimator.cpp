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
#include "Components/ComAnimator.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterAnimatorComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComAnimator = module["ComAnimator"];

    _ComAnimator.SetClass<ComAnimator>(module["Component"]);
    _ComAnimator.AddClassMembers<ComAnimator>(
        "translation", &ComAnimator::GetTranslation,
        "translation_delta", &ComAnimator::GetTranslationDelta,
        "rotation_delta", &ComAnimator::GetRotationDelta,
        "set_anim_parameter", &ComAnimator::SetAnimParameter,
        "reset_anim_state", &ComAnimator::ResetAnimState,
        "current_anim_state", &ComAnimator::GetCurrentAnimState,
        "transit_anim_state", &ComAnimator::TransitAnimState,
        "update_anim", &ComAnimator::UpdateAnim);

    _ComAnimator["meta_object"] = ComAnimator::metaObject;
}

BE_NAMESPACE_END
