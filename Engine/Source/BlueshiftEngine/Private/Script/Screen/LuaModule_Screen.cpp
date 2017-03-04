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
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterScreen(LuaCpp::Module &module) {
    LuaCpp::Selector _Screen = module["Screen"];

    _Screen["screenWidth"].SetFunc([]() { return renderSystem.GetMainRenderContext()->GetScreenWidth(); });
    _Screen["screenHeight"].SetFunc([]() { return renderSystem.GetMainRenderContext()->GetScreenHeight(); });
    _Screen["deviceWidth"].SetFunc([]() { return renderSystem.GetMainRenderContext()->GetDeviceWidth(); });
    _Screen["deviceHeight"].SetFunc([]() { return renderSystem.GetMainRenderContext()->GetDeviceHeight(); });
    _Screen["renderWidth"].SetFunc([]() { return renderSystem.GetMainRenderContext()->GetRenderWidth(); });
    _Screen["renderHeight"].SetFunc([]() { return renderSystem.GetMainRenderContext()->GetRenderHeight(); });
    _Screen["upscaleFactorX"].SetFunc([]() { return renderSystem.GetMainRenderContext()->GetUpscaleFactorX(); });
    _Screen["upscaleFactorY"].SetFunc([]() { return renderSystem.GetMainRenderContext()->GetUpscaleFactorY(); });
}

BE_NAMESPACE_END