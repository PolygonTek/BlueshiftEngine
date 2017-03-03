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