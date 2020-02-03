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
#include "Render/Render.h"
#include "Components/ComCanvas.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterCanvasComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComCanvas = module["ComCanvas"];

    _ComCanvas.SetClass<ComCanvas>(module["Component"]);
    _ComCanvas.AddClassMembers<ComCanvas>(
        "world_to_screen_point", &ComCanvas::WorldToScreenPoint,
        "world_to_canvas_point", &ComCanvas::WorldToCanvasPoint,
        "screen_to_canvas_point", &ComCanvas::ScreenToCanvasPoint,
        "canvas_to_screen_point", &ComCanvas::CanvasToScreenPoint,
        "screen_point_to_ray", &ComCanvas::ScreenPointToRay, 
        "canvas_point_to_ray", &ComCanvas::CanvasPointToRay,
        "is_screen_point_over_child_rect", &ComCanvas::IsScreenPointOverChildRect);

    _ComCanvas["meta_object"] = ComCanvas::metaObject;
}

BE_NAMESPACE_END
