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
#include "Components/ComRenderable.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterRenderableComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComRenderable = module["ComRenderable"];

    _ComRenderable.SetClass<ComRenderable>(module["Component"]);
    _ComRenderable.AddClassMembers<ComRenderable>(
        "max_vis_dist", &ComRenderable::GetMaxVisDist,
        "set_max_vis_dist", &ComRenderable::SetMaxVisDist,
        "color", &ComRenderable::GetColor,
        "set_color", &ComRenderable::SetColor,
        "alpha", &ComRenderable::GetAlpha,
        "set_alpha", &ComRenderable::SetAlpha,
        "aabb", &ComRenderable::GetAABB);

    _ComRenderable["meta_object"] = ComRenderable::metaObject;
}

BE_NAMESPACE_END
