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
#include "Components/ComRectTransform.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterRectTransformComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComRectTransform = module["ComRectTransform"];

    _ComRectTransform.SetClass<ComRectTransform>(module["ComRenderable"]);
    _ComRectTransform.AddClassMembers<ComRectTransform>(
        "anchor_min", &ComRectTransform::GetAnchorMin,
        "anchor_max", &ComRectTransform::GetAnchorMax,
        "anchored_position", &ComRectTransform::GetAnchoredPosition,
        "size_delta", &ComRectTransform::GetSizeDelta,
        "pivot", &ComRectTransform::GetPivot,
        "local_rect", &ComRectTransform::GetLocalRect,
        "set_anchor_min", &ComRectTransform::SetAnchorMin,
        "set_anchor_man", &ComRectTransform::SetAnchorMax,
        "set_anchored_position", &ComRectTransform::SetAnchoredPosition,
        "set_size_delta", &ComRectTransform::SetSizeDelta,
        "set_pivot", &ComRectTransform::SetPivot);

    _ComRectTransform["meta_object"] = ComRectTransform::metaObject;
}

BE_NAMESPACE_END
