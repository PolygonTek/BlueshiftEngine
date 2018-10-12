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
#include "Render/RenderContext.h"
#include "Render/RenderSystem.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterScreen(LuaCpp::Module &module) {
    LuaCpp::Selector _Screen = module["Screen"];

    _Screen["screen_width"].SetFunc([]() { 
        if (renderSystem.GetMainRenderContext()) {
            return renderSystem.GetMainRenderContext()->GetScreenWidth();
        }
        return 0;
    });
    _Screen["screen_height"].SetFunc([]() { 
        if (renderSystem.GetMainRenderContext()) {
            return renderSystem.GetMainRenderContext()->GetScreenHeight();
        }
        return 0;
    });
    _Screen["device_width"].SetFunc([]() { 
        if (renderSystem.GetMainRenderContext()) {
            return renderSystem.GetMainRenderContext()->GetDeviceWidth();
        }
        return 0;
    });
    _Screen["device_height"].SetFunc([]() { 
        if (renderSystem.GetMainRenderContext()) {
            return renderSystem.GetMainRenderContext()->GetDeviceHeight();
        }
        return 0;
    });
    _Screen["render_width"].SetFunc([]() { 
        if (renderSystem.GetMainRenderContext()) {
            return renderSystem.GetMainRenderContext()->GetRenderingWidth();
        }
        return 0;
    });
    _Screen["render_height"].SetFunc([]() { 
        if (renderSystem.GetMainRenderContext()) {
            return renderSystem.GetMainRenderContext()->GetRenderingHeight();
        }
        return 0;
    });
    _Screen["safe_area_insets"].SetFunc([]() { 
        if (renderSystem.GetMainRenderContext()) {
            return renderSystem.GetMainRenderContext()->GetSafeAreaInsets();
        }
        return Rect::empty;
    });
    _Screen["upscale_factor_x"].SetFunc([]() { 
        if (renderSystem.GetMainRenderContext()) {
            return renderSystem.GetMainRenderContext()->GetUpscaleFactorX();
        }
        return 1.0f;
    });
    _Screen["upscale_factor_y"].SetFunc([]() { 
        if (renderSystem.GetMainRenderContext()) {
            return renderSystem.GetMainRenderContext()->GetUpscaleFactorY();
        }
        return 1.0f;
    });
}

BE_NAMESPACE_END
