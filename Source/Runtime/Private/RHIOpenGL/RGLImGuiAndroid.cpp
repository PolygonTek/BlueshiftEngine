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
#include "Platform/PlatformTime.h"
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

void OpenGLRHI::ImGuiCreateContext(GLContext *ctx) {
    // Setup Dear ImGui context.
    ctx->imGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx->imGuiContext);

    ImGui::GetStyle().TouchExtraPadding = ImVec2(4.0F, 4.0F);

    // Setup Dear ImGui style.
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    
    ctx->imGuiLastTime = PlatformTime::Seconds();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Setup back-end capabilities flags.
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    //io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
    io.BackendPlatformName = "OpenGLRHI-Android";

    ImGui_ImplOpenGL_Init("#version 300 es");
}

void OpenGLRHI::ImGuiDestroyContext(GLContext *ctx) {
    ImGui_ImplOpenGL_Shutdown();

    ImGui::DestroyContext(ctx->imGuiContext);
}

void OpenGLRHI::ImGuiBeginFrame(Handle ctxHandle) {
    BE_SCOPE_PROFILE_CPU("OpenGLRHI::ImGuiBeginFrame");

    ImGui_ImplOpenGL_ValidateFrame();

    GLContext *ctx = ctxHandle == NullContext ? mainContext : contextList[ctxHandle];

    DisplayMetrics dm;
    GetDisplayMetrics(ctxHandle, &dm);

    // Setup display size
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(dm.screenWidth, dm.screenHeight);
    io.DisplayFramebufferScale = ImVec2((float)dm.backingWidth / dm.screenWidth, (float)dm.backingHeight / dm.screenHeight);

    // Setup time step
    double currentTime = PlatformTime::Seconds();
    io.DeltaTime = currentTime - ctx->imGuiLastTime;
    ctx->imGuiLastTime = currentTime;
    
    ImGui::NewFrame();
}

void OpenGLRHI::ImGuiRender() {
    BE_SCOPE_PROFILE_CPU("OpenGLRHI::ImGuiRender");
    BE_SCOPE_PROFILE_GPU("OpenGLRHI::ImGuiRender");

    ImGui::Render();

    bool sRGBWriteEnabled = OpenGL::SupportsFrameBufferSRGB() && IsSRGBWriteEnabled();
    if (sRGBWriteEnabled) {
        SetSRGBWrite(false);
    }

    ImGui_ImplOpenGL_RenderDrawData(ImGui::GetDrawData());

    if (sRGBWriteEnabled) {
        SetSRGBWrite(true);
    }
}

void OpenGLRHI::ImGuiEndFrame() {
    BE_SCOPE_PROFILE_CPU("OpenGLRHI::ImGuiEndFrame");

    ImGui::EndFrame();

    // HACK: invalidate touch position after one frame.
    ImGuiIO &io = ImGui::GetIO();
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
}

BE_NAMESPACE_END
