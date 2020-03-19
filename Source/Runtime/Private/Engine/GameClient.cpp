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
#include "Platform/PlatformThread.h"
#include "Core/StrColor.h"
#include "Core/Cmds.h"
#include "Core/CVars.h"
#include "Core/Vec4Color.h"
#include "Render/Render.h"
#include "Physics/Physics.h"
#include "Input/KeyCmd.h"
#include "Input/InputSystem.h"
#include "Sound/SoundSystem.h"
#include "AnimController/AnimController.h"
#include "Engine/Common.h"
#include "Engine/GameClient.h"
#include "Engine/Console.h"
#include "Asset/GuidMapper.h"
#include "Platform/PlatformProcess.h"
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

static const char   CMDLINE_PROMPT_MARK = ']';
static const float  CMDLINE_BLINK_SPEED = 2.0f;

static const int    DRAWTEXT_LINE_SPACING = 4;

static const int    CONSOLE_FONT_HEIGHT = 14;
static const int    CONSOLE_FONT_Y_SPACING = 2;
static const int    CONSOLE_TEXT_BORDER = 8;

static CVAR(cl_updateFps, "500", CVar::Flag::Integer, "FPS update duration in milliseconds");
static CVAR(cl_conSize, "0.6", CVar::Flag::Float | CVar::Flag::Archive, "");
static CVAR(cl_conSpeed, "5.0", CVar::Flag::Float | CVar::Flag::Archive, "");
static CVAR(cl_conNoPrint, "1", CVar::Flag::Bool, "");
static CVAR(cl_conNotifyTime, "3.0", CVar::Flag::Float | CVar::Flag::Archive, "");

GameClient          gameClient;

void GameClient::Init(void *windowHandle, bool useMouseInput) {
    cmdSystem.AddCommand("connect", Cmd_Connect);
    cmdSystem.AddCommand("disconnect", Cmd_Disconnect);
    cmdSystem.AddCommand("toggleConsole", Cmd_ToggleConsole);
    cmdSystem.AddCommand("toggleMenuBar", Cmd_ToggleMenuBar);
    cmdSystem.AddCommand("toggleStatistics", Cmd_ToggleStatistics);

    state = ClientState::Disconnected;

    time = 0;
    oldTime = 0;
    frameCount = 0;

    fps = 0;
    fpsFrametime = 0.0f;
    fpsFrames = 0.0f;
    
    consoleEnabled = false;
    consoleHeight = 0.0f;
    consoleUpScroll = 0;

    editLine = 0;
    historyLine = 0;
    lineOffset = 1;

    keyFocus = KeyFocus::Game;
    compositionMode = false;
    replaceMode = false;

    for (int i = 0; i < CommandLineHistory; i++) {
        cmdLines[i][0] = CMDLINE_PROMPT_MARK;
        cmdLines[i][1] = 0;
    }

    // set default resource GUID mapper
    InitDefaultGuids();

    platform->SetMainWindowHandle(windowHandle);
    platform->EnableMouse(useMouseInput);

    BE_PROFILE_INIT();

    inputSystem.Init();

    renderSystem.Init();

    soundSystem.Init(windowHandle);

    physicsSystem.Init();

    animControllerManager.Init();

    consoleMaterial = nullptr;// materialManager.GetMaterial("Data/EngineMaterials/console.material");    

    currentColor = Color4::white;
    currentTextColor = Color4::white;
    currentTextScale = Vec2(1.0f, 1.0f);
    currentFont = nullptr;
    
    SetFont(fontManager.defaultFont);
}

void GameClient::InitDefaultGuids() {
    resourceGuidMapper.Set(GuidMapper::defaultTextureGuid, "_defaultTexture");
    resourceGuidMapper.Set(GuidMapper::zeroClampTextureGuid, "_zeroClampTexture");
    resourceGuidMapper.Set(GuidMapper::defaultCubeTextureGuid, "_defaultCubeTexture");
    resourceGuidMapper.Set(GuidMapper::blackCubeTextureGuid, "_blackCubeTexture");
    resourceGuidMapper.Set(GuidMapper::whiteTextureGuid, "_whiteTexture");
    resourceGuidMapper.Set(GuidMapper::blackTextureGuid, "_blackTexture");
    resourceGuidMapper.Set(GuidMapper::greyTextureGuid, "_greyTexture");
    resourceGuidMapper.Set(GuidMapper::linearTextureGuid, "_linearTexture");
    resourceGuidMapper.Set(GuidMapper::exponentTextureGuid, "_exponentTexture");
    resourceGuidMapper.Set(GuidMapper::flatNormalTextureGuid, "_flatNormalTexture");
    resourceGuidMapper.Set(GuidMapper::normalCubeTextureGuid, "_normalCubeTexture");
    resourceGuidMapper.Set(GuidMapper::cubicNormalCubeTextureGuid, "_cubicNormalCubeTexture");
    resourceGuidMapper.Set(GuidMapper::imageShaderGuid, "Shaders/Image");
    resourceGuidMapper.Set(GuidMapper::unlitShaderGuid, "Shaders/Unlit");
    resourceGuidMapper.Set(GuidMapper::standardSpecularShaderGuid, "Shaders/StandardSpec");
    resourceGuidMapper.Set(GuidMapper::standardShaderGuid, "Shaders/Standard");
    resourceGuidMapper.Set(GuidMapper::phongLightingShaderGuid, "Shaders/Phong");
    resourceGuidMapper.Set(GuidMapper::skyboxCubemapShaderGuid, "Shaders/SkyboxCubemap");
    resourceGuidMapper.Set(GuidMapper::skyboxSixSidedShaderGuid, "Shaders/SkyboxSixSided");
    resourceGuidMapper.Set(GuidMapper::envCubemapShaderGuid, "Shaders/EnvCubemap");
    resourceGuidMapper.Set(GuidMapper::defaultMaterialGuid, "_defaultMaterial");
    resourceGuidMapper.Set(GuidMapper::whiteMaterialGuid, "_whiteMaterial");
    resourceGuidMapper.Set(GuidMapper::blendMaterialGuid, "_blendMaterial");
    resourceGuidMapper.Set(GuidMapper::whiteLightMaterialGuid, "_whiteLightMaterial");
    resourceGuidMapper.Set(GuidMapper::zeroClampLightMaterialGuid, "_zeroClampLightMaterial");
    resourceGuidMapper.Set(GuidMapper::defaultSkyboxMaterialGuid, "_defaultSkyboxMaterial");
    resourceGuidMapper.Set(GuidMapper::unlitMaterialGuid, "_unlitMaterial");
    resourceGuidMapper.Set(GuidMapper::defaultFontGuid, FontManager::defaultFontFilename);
    resourceGuidMapper.Set(GuidMapper::defaultSkeletonGuid, "_defaultSkeleton");
    resourceGuidMapper.Set(GuidMapper::defaultMeshGuid, "_defaultMesh");
    resourceGuidMapper.Set(GuidMapper::quadMeshGuid, "_defaultQuadMesh");
    resourceGuidMapper.Set(GuidMapper::planeMeshGuid, "_defaultPlaneMesh");
    resourceGuidMapper.Set(GuidMapper::boxMeshGuid, "_defaultBoxMesh");
    resourceGuidMapper.Set(GuidMapper::sphereMeshGuid, "_defaultSphereMesh");
    resourceGuidMapper.Set(GuidMapper::geoSphereMeshGuid, "_defaultGeoSphereMesh");
    resourceGuidMapper.Set(GuidMapper::cylinderMeshGuid, "_defaultCylinderMesh");
    resourceGuidMapper.Set(GuidMapper::capsuleMeshGuid, "_defaultCapsuleMesh");
    resourceGuidMapper.Set(GuidMapper::defaultParticleSystemGuid, "_defaultParticleSystem");
    resourceGuidMapper.Set(GuidMapper::defaultAnimControllerGuid, "_defaultAnimController");
    resourceGuidMapper.Set(GuidMapper::defaultSoundGuid, "_defaultSound");
}

void GameClient::Shutdown() {
    cmdSystem.RemoveCommand("connect");
    cmdSystem.RemoveCommand("disconnect");
    cmdSystem.RemoveCommand("toggleConsole");
    cmdSystem.RemoveCommand("toggleMenuBar");
    cmdSystem.RemoveCommand("toggleStatistics");

    BE_PROFILE_SHUTDOWN();

    //materialManager.ReleaseMaterial(consoleMaterial);

    animControllerManager.Shutdown();

    physicsSystem.Shutdown();

    soundSystem.Shutdown();

    renderSystem.Shutdown();

    inputSystem.Shutdown();
}

void GameClient::Update() {
    BE_SCOPE_PROFILE_CPU("GameClient::Update");

    if (fpsFrametime >= cl_updateFps.GetInteger()) {
        fps = fpsFrames / MILLI2SEC(fpsFrametime);
        fpsFrames = 0;
        fpsFrametime -= cl_updateFps.GetInteger();
    }

    frameCount++;

    time += common.frameTime;

    fpsFrametime += common.frameTime;

    fpsFrames++;

    UpdateConsole();

    // FIXME: use thread
    soundSystem.Update();

    renderSystem.CheckModifiedCVars();

    physicsSystem.CheckModifiedCVars();

    EventSystem::ServiceEvents();

    SignalSystem::ServiceSignals();
}

void GameClient::EndFrame() {
    inputSystem.EndFrame();
}

void GameClient::Render(const RenderContext *renderContext) {
    BE_SCOPE_PROFILE_CPU("GameClient::Render");

    menuBarHeight = 0.0f;

    // Draw menu bar.
    if (showMenuBar) {
        DrawMenuBar();

        renderSystem.CheckModifiedCVars();
    }

    // Draw statistics.
    if (showStatistics) {
        DrawStatistics(renderContext);
    }

    // Draw IMGUI demo window.
    //ImGui::ShowDemoWindow();

    DrawConsole();
}

static void AddMenuItemCVarBool(const char *cvarName, const char *label) {
    CVar *cvar = cvarSystem.Find(cvarName);
    if (cvar) {
        bool value = cvar->GetBool();
        if (ImGui::Checkbox(label, &value)) {
            cvar->SetBool(value);
        }
    }
};

static void AddMenuItemCVarFloat(const char *cvarName, const char *label) {
    CVar *cvar = cvarSystem.Find(cvarName);
    if (cvar) {
        float value = cvar->GetFloat();
        if (cvar->GetMinValue() < cvar->GetMaxValue()) {
            if (ImGui::SliderFloat(label, &value, cvar->GetMinValue(), cvar->GetMaxValue())) {
                cvar->SetFloat(value);
            }
        } else {
            if (ImGui::InputFloat(label, &value)) {
                cvar->SetFloat(value);
            }
        }
    }
};

static void AddMenuItemCVarInt(const char *cvarName, const char *label) {
    CVar *cvar = cvarSystem.Find(cvarName);
    if (cvar) {
        int value = cvar->GetInteger();
        if (cvar->GetMinValue() < cvar->GetMaxValue()) {
            if (ImGui::SliderInt(label, &value, cvar->GetMinValue(), cvar->GetMaxValue())) {
                cvar->SetInteger(value);
            }
        } else {
            if (ImGui::InputInt(label, &value)) {
                cvar->SetInteger(value);
            }
        }
    }
};

static void AddMenuItemCVarIntArray(const char *cvarName, const char *label, int count, const int intArray[]) {
    CVar *cvar = cvarSystem.Find(cvarName);
    if (cvar) {
        int value = cvar->GetInteger();
        int index = 0;
        while (value > intArray[index]) {
            index++;
        }
        if (ImGui::SliderInt(label, &index, 0, count - 1, va("%i", intArray[index]))) {
            cvar->SetInteger(intArray[index]);
        }
    }
};

static void AddMenuItemCVarEnum(const char *cvarName, const char *label, const char *enumNames) {
    CVar *cvar = cvarSystem.Find(cvarName);
    if (cvar) {
        int value = cvar->GetInteger();
        if (ImGui::Combo(label, &value, enumNames)) {
            cvar->SetInteger(value);
        }
    }
};

void GameClient::DrawMenuBar() {
#ifdef ENABLE_IMGUI
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Engine")) {
            bool showStatistics = IsStatisticsVisible();
            if (ImGui::Checkbox("Show Statistics", &showStatistics)) {
                cmdSystem.BufferCommandText(CmdSystem::Execution::Append, "toggleStatistics");
            }
            AddMenuItemCVarBool("developer", "Enable Developer");
            AddMenuItemCVarBool("lua_debug", "Enable Lua Debugging");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Graphics")) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Lighting");
            ImGui::Indent();
            AddMenuItemCVarBool("r_showLights", "Debug Lights");
            AddMenuItemCVarBool("r_specularEnergyCompensation", "Specular Energy Compensation");
            AddMenuItemCVarBool("r_indirectLit", "Indirect Lighting");
            AddMenuItemCVarBool("r_probeBlending", "Blending Probes");
            AddMenuItemCVarBool("r_probeBoxProjection", "Box Projected Probes");
            ImGui::Unindent();

            ImGui::Separator();

            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Shadows");
            ImGui::Indent();
            AddMenuItemCVarBool("r_shadows", "Enabled");
            AddMenuItemCVarBool("r_showShadows", "Debug Shadows");
            const int shadowMapSizeArray[] = { 256, 512, 1024, 2048, 4096 };
            AddMenuItemCVarIntArray("r_shadowMapSize", "Size", COUNT_OF(shadowMapSizeArray), shadowMapSizeArray);
            AddMenuItemCVarEnum("r_shadowMapQuality", "Quality", "PCFx1\0PCFx5\0PCFx9\0PCFx16 (randomly jittered)\0\0");
            AddMenuItemCVarEnum("r_CSM_selectionMethod", "Cascade Selection", "Z-Based\0Map-Based\0\0");
            AddMenuItemCVarBool("r_CSM_blend", "Cascade Blending");
            ImGui::Unindent();

            ImGui::Separator();

            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Post Processing");
            ImGui::Indent();
            AddMenuItemCVarEnum("r_HDR", "HDR Mode", "No HDR\0FP11 or FP16\0FP16\0FP32\0\0");
            AddMenuItemCVarBool("r_HDR_debug", "Debug HDR");
            AddMenuItemCVarBool("r_HDR_toneMapping", "Tone Mapping");
            AddMenuItemCVarEnum("r_HDR_toneMapOp", "Tone Map Operator", "Linear\0Exponential\0Logarithmic\0Drago Logarithmic\0Reinhard\0Reinhard Extended\0Filmic ALU\0Flimic ACES\0Filmic Unreal\0Filmic Uncharted 2\0\0");
            AddMenuItemCVarBool("r_sunShafts", "Sun Shafts");
            AddMenuItemCVarFloat("r_sunShafts_scale", "Sun Shafts Scale");
            ImGui::Unindent();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Physics")) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Settings");
            ImGui::Indent();
            AddMenuItemCVarBool("physics_enableCCD", "Enable CCD");
            AddMenuItemCVarBool("physics_noDeactivation", "No Deactivation");
            ImGui::Unindent();

            ImGui::Separator();

            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Debug Draw");
            ImGui::Indent();
            AddMenuItemCVarBool("physics_showWireframe", "Show Wireframe");
            AddMenuItemCVarBool("physics_showAABB", "Show AABB");
            AddMenuItemCVarBool("physics_showContactPoints", "Show Contact Points");
            AddMenuItemCVarBool("physics_showNormals", "Show Normals");
            ImGui::Unindent();

            ImGui::EndMenu();
        }

        menuBarHeight = ImGui::GetWindowSize().y;

        ImGui::EndMainMenuBar();
    }
#endif
}

void GameClient::DrawStatistics(const RenderContext *renderContext) {
#ifdef ENABLE_IMGUI
    const int fixedWidth = 480;
    int rightOffset = 10;
    int topOffset = menuBarHeight + 10;

    ImGui::SetNextWindowSize(ImVec2(fixedWidth, 120), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - fixedWidth - rightOffset, topOffset), ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(ImVec2(fixedWidth, -1), ImVec2(fixedWidth, -1));
    ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    const RenderCounter &renderCounter = renderContext->GetPrevFrameRenderCounter();

    Str fpsText = va("FPS: %3i", GetFPS());

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
    ImGui::Text("Render: %ims, FrontEnd: %ims, BackEnd: %ims", renderCounter.frameMsec, renderCounter.frontEndMsec, renderCounter.backEndMsec);
    ImGui::SameLine(ImGui::GetWindowSize().x - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize(fpsText.c_str()).x);
    ImGui::TextUnformatted(fpsText.c_str());
    ImGui::Text("Draw: %i, Verts: %i, Tris: %i", renderCounter.drawCalls, renderCounter.drawVerts, renderCounter.drawIndexes / 3);
    ImGui::Text("Shadow Draw: %i, Verts: %i, Tris: %i", renderCounter.shadowDrawCalls, renderCounter.shadowDrawVerts, renderCounter.shadowDrawIndexes / 3);
    ImGui::PopStyleColor();

    ImGui::Separator();

    if (profiler.IsUnfrozen()) {
        uint64_t tid = PlatformThread::GetCurrentThreadId();

        if (ImGui::CollapsingHeader(BE1::va("CPU (%" PRIu64 ")", tid), ImGuiTreeNodeFlags_DefaultOpen)) {
            int lastStackDepth = 0;

            ImGui::BeginGroup();

            profiler.IterateCpuMarkers(tid, [&lastStackDepth](const char *tagName, const Color3 &tagColor, int stackDepth, bool isLeaf, uint64_t startTime, uint64_t endTime) {
                // Convert nanoseconds to milliseconds.
                const float durationMs = (endTime - startTime) * 0.000001f;

                Color3 textColor = Color3::FromHSL(0.0f, Clamp(durationMs / 3.0f, 0.0f, 1.0f), 0.65f);

                while (stackDepth < lastStackDepth) {
                    ImGui::TreePop();
                    lastStackDepth--;
                }

                lastStackDepth = stackDepth;

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(textColor.r, textColor.g, textColor.b, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(textColor.r, textColor.g, textColor.b, 0.2f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(textColor.r, textColor.g, textColor.b, 0.2f));

                Str id = tagName;
                bool opened = ImGui::TreeNodeEx(id, isLeaf ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0, "%s: %.3fms", tagName, durationMs);

                ImGui::PopStyleColor(3);

                return opened;
            });

            while (lastStackDepth-- > 0) {
                ImGui::TreePop();
            }

            ImGui::EndGroup();
        }

        if (ImGui::CollapsingHeader("GPU", ImGuiTreeNodeFlags_DefaultOpen)) {
            int lastStackDepth = 0;

            ImGui::BeginGroup();

            profiler.IterateGpuMarkers([&lastStackDepth](const char *tagName, const Color3 &tagColor, int stackDepth, bool isLeaf, uint64_t startTime, uint64_t endTime) {
                // Convert nanoseconds to milliseconds.
                const float durationMs = (endTime - startTime) * 0.000001f;

                Color3 textColor = Color3::FromHSL(0.0f, Clamp(durationMs / 3.0f, 0.0f, 1.0f), 0.65f);

                while (stackDepth < lastStackDepth) {
                    ImGui::TreePop();
                    lastStackDepth--;
                }

                lastStackDepth = stackDepth;

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(textColor.r, textColor.g, textColor.b, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(textColor.r, textColor.g, textColor.b, 0.2f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(textColor.r, textColor.g, textColor.b, 0.2f));

                Str id = tagName;
                bool opened = ImGui::TreeNodeEx(id, isLeaf ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen : 0, "%s: %.3fms", tagName, durationMs);

                ImGui::PopStyleColor(3);

                return opened;
            });

            while (lastStackDepth-- > 0) {
                ImGui::TreePop();
            }

            ImGui::EndGroup();
        }
    }

    ImGui::End();
#endif
}

void GameClient::ShowMenuBar(bool show) {
    showMenuBar = show;
}

void GameClient::ShowStatistics(bool show) {
    showStatistics = show;

    if (showStatistics) {
        BE_PROFILE_START();
    } else {
        BE_PROFILE_STOP();
    }
}

void GameClient::UpdateConsole() {
    float targetHeight;

    if (keyFocus == KeyFocus::Console) {
        targetHeight = 720 * cl_conSize.GetFloat();
    } else {
        targetHeight = 0.0f;
    }

    if (targetHeight > consoleHeight) {
        consoleHeight += 720 * cl_conSpeed.GetFloat() * common.frameSec;
        if (targetHeight < consoleHeight) {
            consoleHeight = targetHeight;
        }
    } else {
        consoleHeight = targetHeight;
    }
}

void GameClient::EnableConsole(bool flag) {
    if (flag == false && keyFocus == KeyFocus::Console) {
        ClearCommandLine();
        SetKeyFocus(KeyFocus::Game);
    }

    consoleEnabled = flag;
}

void GameClient::ClearCommandLine() {
    cmdLines[editLine][0] = CMDLINE_PROMPT_MARK;
    cmdLines[editLine][1] = 0;
    lineOffset = 1;
}

void GameClient::SetColor(const Color4 &rgba) {
    currentColor = rgba;
}

void GameClient::SetTextColor(const Color4 &rgba) {
    currentTextColor = rgba;
}

void GameClient::SetFont(Font *font) {
    if (!font) {
        currentFont = fontManager.defaultFont;
        return;
    }

    currentFont = font;
}

void GameClient::SetTextScale(float xscale, float yscale) {
    currentTextScale.x = xscale;
    currentTextScale.y = yscale;
}

void GameClient::DrawPic(float x, float y, float w, float h, const Material *material) {
    renderSystem.GetCurrentRenderContext()->SetColor(currentColor);

    renderSystem.GetCurrentRenderContext()->DrawPic(x, y, w, h, material);
}

void GameClient::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Material *material) {
    renderSystem.GetCurrentRenderContext()->SetColor(currentColor);

    renderSystem.GetCurrentRenderContext()->DrawStretchPic(x, y, w, h, s1, t1, s2, t2, material);
}

void GameClient::DrawBar(float x, float y, float w, float h, const Color4 &rgba) {
    renderSystem.GetCurrentRenderContext()->SetColor(rgba);
    
    renderSystem.GetCurrentRenderContext()->DrawPic(x, y, w, h, materialManager.blendMaterial);
}

void GameClient::DrawString(int x, int y, const Str &str, int size, int flags) {
    int w = renderSystem.GetCurrentRenderContext()->GetRenderingWidth();
    int h = renderSystem.GetCurrentRenderContext()->GetRenderingHeight();
    Rect rect(0, 0, w, h);

    DrawStringInRect(rect, x, y, str, size, flags);
}

void GameClient::DrawStringInRect(const Rect &rect, int marginX, int marginY, const Str &text, int size, int flags) {
    static const int MaxTextLines = 256;
    int lineOffsets[MaxTextLines];
    int lineLen[MaxTextLines];
    int numLines = 0;
    int currentLineWidth = 0;
    int currentLineLength = 0;
    int offset = 0;
    bool truncated = false;
    char32_t unicodeChar;

    if (size == -1) {
        size = text.Length();
    }

    lineOffsets[0] = 0;

    while (offset < size) {
        unicodeChar = text.UTF8CharAdvance(offset);
        if (!unicodeChar) {
            break;
        }

        if (unicodeChar == UC_COLOR_ESCAPE) {
            int prevOffset = offset;
            char32_t nextUnicodeChar = text.UTF8CharAdvance(offset);

            if (nextUnicodeChar != 0 && nextUnicodeChar != UC_COLOR_ESCAPE) {
                currentLineLength += 2;
                continue;
            } else {
                offset = prevOffset;
            }
        }

        if ((flags & DrawTextFlag::MultiLines) && unicodeChar == U'\n') {
            // Save current line length
            lineLen[numLines++] = currentLineLength;

            currentLineLength = 0;
            currentLineWidth = 0;

            // Save next line offset
            lineOffsets[numLines] = offset;
        } else {
            int charWidth = currentFont->GetGlyphAdvanceX(unicodeChar) * currentTextScale.x;

            if (flags & DrawTextFlag::WordWrap) {
                if (currentLineWidth + charWidth > rect.w - marginX) {
                    // Save current line length
                    lineLen[numLines++] = currentLineLength;

                    currentLineLength = 0;
                    currentLineWidth = 0;

                    // Save next line offset
                    lineOffsets[numLines] = offset;
                    continue;
                }
            } else if (currentLineWidth + charWidth > rect.w - marginX) {
                if (flags & DrawTextFlag::Truncate) {
                    if (currentLineWidth > 0) {
                        int dotdotdotWidth = currentFont->GetGlyphAdvanceX(U'.') * currentTextScale.x * 3;
                
                        do {
                            dotdotdotWidth -= currentFont->GetGlyphAdvanceX(unicodeChar) * currentTextScale.x;
                            unicodeChar = text.UTF8CharPrevious(offset);
                        } while (dotdotdotWidth > 0 && unicodeChar);
                    
                        truncated = true;
                    }
                }
                // Abandon remaining text
                break;
            }

            currentLineLength++;
            currentLineWidth += charWidth;
        }
    }

    if (currentLineLength > 0) {
        lineLen[numLines++] = currentLineLength;
    }

    // Calculate the y-coordinate
    int y;
    if (flags & (DrawTextFlag::Bottom | DrawTextFlag::VCenter)) {
        int height = currentFont->GetFontHeight() * currentTextScale.y * numLines + DRAWTEXT_LINE_SPACING * (numLines - 1);

        if (flags & DrawTextFlag::Bottom) {
            y = rect.y + rect.h - height - marginY;
        } else if (flags & DrawTextFlag::VCenter) {
            y = rect.y + (rect.h - height) / 2 + marginY;
        }
    } else {
        y = rect.y + marginY;
    }

    GuiMesh &guiMesh = renderSystem.GetCurrentRenderContext()->GetGuiMesh();

    Color4 textColor = currentTextColor;
    guiMesh.SetColor(textColor);
    guiMesh.SetTextBorderColor(Color4::black);

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        offset = lineOffsets[lineIndex];

        // Calculate the x-coordinate
        int x;
        if (flags & (DrawTextFlag::Right | DrawTextFlag::Center)) {
            int width = currentFont->TextWidth(&text[offset], lineLen[lineIndex], false, true, currentTextScale.x);

            if (flags & DrawTextFlag::Right) {
                x = rect.x + rect.w - width - marginX;
            } else if (flags & DrawTextFlag::Center) {
                x = rect.x + (rect.w - width) / 2 + marginX;
            }
        } else {
            x = rect.x + marginX;
        }

        for (int lineTextIndex = 0; lineTextIndex < lineLen[lineIndex]; lineTextIndex++) {
            char32_t unicodeChar = text.UTF8CharAdvance(offset);

            int colorIndex = -1;

            if (unicodeChar == UC_COLOR_ESCAPE) {
                int prevOffset = offset;
                uint32_t nextUnicodeChar = text.UTF8CharAdvance(offset);

                if (nextUnicodeChar != 0 && nextUnicodeChar != UC_COLOR_ESCAPE) {
                    colorIndex = UC_COLOR_INDEX(nextUnicodeChar);
                } else {
                    offset = prevOffset;
                }
            }

            if (colorIndex >= 0) {
                Clamp(colorIndex, 0, (int)COUNT_OF(Vec4Color::table) - 1);
                textColor.ToColor3() = Vec4Color::table[colorIndex].ToColor3();

                guiMesh.SetColor(textColor);

                lineTextIndex++;
                continue;
            }

            RenderObject::TextDrawMode::Enum drawMode;

            if (flags & DrawTextFlag::Outline) {
                drawMode = RenderObject::TextDrawMode::AddOutlines;
            } else if (flags & DrawTextFlag::DropShadow) {
                drawMode = RenderObject::TextDrawMode::DropShadows;
            } else {
                drawMode = RenderObject::TextDrawMode::Normal;
            }

            x += guiMesh.DrawChar(x, y, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar, drawMode);
        }

        if (truncated && lineIndex == numLines - 1) {
            x += guiMesh.DrawChar(x, y, currentTextScale.x, currentTextScale.y, currentFont, U'.');
            x += guiMesh.DrawChar(x, y, currentTextScale.x, currentTextScale.y, currentFont, U'.');
            x += guiMesh.DrawChar(x, y, currentTextScale.x, currentTextScale.y, currentFont, U'.');
        }

        y += (currentFont->GetFontHeight() + DRAWTEXT_LINE_SPACING) * currentTextScale.y;
    }
}

void GameClient::DrawConsole() {
    Font *oldFont = gameClient.GetFont();

    SetFont(nullptr);
    SetTextColor(Color4::gray);

    if (consoleHeight > 0.0f) {
        DrawConsoleScreen();
    } else {
        //DrawConsoleNotify();
    }

    SetFont(oldFont);
}

void GameClient::DrawConsoleScreen() {
    char versionString[64];
    
    if (consoleHeight <= 0.0f) {
        return;
    }

    int screenWidth = renderSystem.GetCurrentRenderContext()->GetRenderingWidth();

    // Draw console background.
    SetColor(Color4(0.0f, 0.0f, 0.0f, 0.8f));
    DrawPic(0, 0, screenWidth, consoleHeight, materialManager.blendMaterial);
    DrawBar(0, consoleHeight, screenWidth, 3, Color4::black);

    // Draw version string.
    Str::snPrintf(versionString, COUNT_OF(versionString), "%s-%s %s", BE_NAME, PlatformProcess::PlatformName(), BE_VERSION);
    SetTextColor(Color4(1.0f, 0.5f, 0.0f, 1.0f));
    DrawString(CONSOLE_TEXT_BORDER, consoleHeight - (CONSOLE_FONT_HEIGHT * 1.5f), versionString, -1, DrawTextFlag::Right | DrawTextFlag::DropShadow);

    int y = consoleHeight - (CONSOLE_FONT_HEIGHT + CONSOLE_FONT_Y_SPACING) * 3;

    // Draw back scroll marks.
    if (consoleUpScroll > 0) {
        for (int x = CONSOLE_TEXT_BORDER; x < screenWidth; x += 100) {
            DrawString(x, y, "~");
        }
        y -= (CONSOLE_FONT_HEIGHT + CONSOLE_FONT_Y_SPACING);
    }

    SetTextColor(Color4::white);

    // Calculate number of lines to show.
    int numDrawLines = consoleHeight / (CONSOLE_FONT_HEIGHT + CONSOLE_FONT_Y_SPACING) - 3;

    // Draw content of console text.
    for (int i = console.currentLineIndex; i >= console.currentLineIndex - numDrawLines; i--) {
        int index = (i - consoleUpScroll) % console.textLines.Count();
        if (index < 0) {
            index += console.textLines.Count();
        }

        const Str &string = console.textLines[index];

        DrawString(CONSOLE_TEXT_BORDER, y, string);

        y -= CONSOLE_FONT_HEIGHT + CONSOLE_FONT_Y_SPACING;
    }

    DrawConsoleCmdLine();
}

void GameClient::DrawConsoleCmdLine() {
    if (keyFocus != KeyFocus::Console) {
        return;
    }

    int screenWidth = renderSystem.GetCurrentRenderContext()->GetRenderingWidth();

    const char *text = cmdLines[editLine];
    const char *textPtr = text;

    int offset = 0;
    int textWidth = 0;

    while (offset < lineOffset) {
        if (text[offset] == C_COLOR_ESCAPE && text[offset + 1] != C_COLOR_ESCAPE && text[offset + 1] != 0) {
            // Exclude color escape characters when drawing.
            offset += 2;
            continue;
        }

        int prevOffset = offset;

        char32_t unicodeChar = UTF8::CharAdvance(text, offset);

        int charWidth = currentFont->GetGlyphAdvanceX(unicodeChar);
        if (textWidth + charWidth < screenWidth) {
            textWidth += charWidth;
        } else {
            textWidth = charWidth;
            textPtr += prevOffset;
        }
    }

    // Draw console command line text.
    DrawString(CONSOLE_TEXT_BORDER, consoleHeight - CONSOLE_FONT_HEIGHT * 2, textPtr, -1, 0);
    
    // Determine caret shape.
    int caretX = CONSOLE_TEXT_BORDER + textWidth;
    int caretY = consoleHeight - CONSOLE_FONT_HEIGHT * 2 + 1;
    int caretW = 0;
    int caretH = CONSOLE_FONT_HEIGHT;
    Color4 caretColor = Color4(1, 1, 1, 0.5f);

    if (replaceMode || compositionMode) {
        caretW = currentFont->GetGlyphAdvanceX(UTF8::Char(text, offset));
    } else {
        caretW = 2;
    }

    // Draw caret.
    if ((int)(time * 0.001f * CMDLINE_BLINK_SPEED) & 1) {
        DrawBar(caretX, caretY, caretW, caretH, caretColor);
    }
}

void GameClient::DrawConsoleNotify() {
    /*
    if (cl_conNoPrint.GetBool()) {
        return;
    }

    int y = 0;

    for (int i = console.currentLine - CONSOLE_NOTIFY_TIMES + 1; i <= console.currentLine; i++) {
        if (i < 0) {
            continue;
        }

        int t = console.notifyTimes[i % CONSOLE_NOTIFY_TIMES];
        if (!t) {
            continue;
        }

        t = time - t;
        if (t > cl_conNotifyTime.GetFloat() * 1000) {
            continue;
        }

        wchar_t *text = console.text + (i % console.textLines.Count()) * console.sizeOfLine;
        DrawString(0, y, text, console.sizeOfLine, DrawTextFlag::DropShadow);

        y += CONSOLE_FONT_HEIGHT + CONSOLE_FONT_Y_SPACING;
    }*/
}

void GameClient::ConsoleKeyEvent(KeyCode::Enum key) {
    char buffer[CommandLineSize];
    const char *cmd;

    char *lineText = cmdLines[editLine];

    switch (key) {
    case KeyCode::Enter: // Enter
        cmdSystem.BufferCommandText(CmdSystem::Execution::Append, lineText + 1); // skip the CMDLINE_PROMPT_MARK
        cmdSystem.BufferCommandText(CmdSystem::Execution::Append, "\n");

        BE_LOG("%s\n", lineText);

        editLine = (editLine + 1) & 31;
        historyLine = editLine;

        lineText = cmdLines[editLine];
        lineText[0] = CMDLINE_PROMPT_MARK;
        lineText[1] = 0;
        lineOffset = 1;

        consoleUpScroll = 0;
        return;
    case KeyCode::Tab: // Auto completion
        cmd = cmdSystem.CompleteCommand(lineText + 1);
        if (!cmd) {
            cmd = cvarSystem.CompleteVariable(lineText + 1);
        }

        if (cmd) {
            strcpy(lineText + 1, cmd);
            lineOffset = Str::Length(lineText + 1) + 1;
            lineText[lineOffset] = 0;
        }
        return;
    case KeyCode::Backspace: // Backspace
        if (lineOffset > 1) {
            strcpy(buffer, &lineText[lineOffset]);

            int previousCharOffset = lineOffset;
            UTF8::Previous(lineText, previousCharOffset);
            lineOffset = previousCharOffset;

            strcpy(&lineText[lineOffset], buffer);
        }
        return;
    case KeyCode::Delete:
        if (lineText[lineOffset]) {
            int nextCharOffset = lineOffset;
            UTF8::Advance(lineText, nextCharOffset);

            strcpy(buffer, &lineText[nextCharOffset]);
            strcpy(&lineText[lineOffset], buffer);
        }
        return;
    case KeyCode::PageUp: // Scroll up
    case KeyCode::MouseWheelUp:
        {
            int numDrawLines = consoleHeight / (CONSOLE_FONT_HEIGHT + CONSOLE_FONT_Y_SPACING) - 3;
            int maxLines = console.NumLines();

            consoleUpScroll += 2;
            if (consoleUpScroll > maxLines - numDrawLines) {
                consoleUpScroll = maxLines - numDrawLines;
            }
        }
        return;
    case KeyCode::PageDown: // Scroll down
    case KeyCode::MouseWheelDown:
        consoleUpScroll -= 2;
        if (consoleUpScroll < 0) {
            consoleUpScroll = 0;
        }
        return;
    case KeyCode::Home: // Move to the begining of a lineText
        lineOffset = 1;
        return;
    case KeyCode::End: // Move to the end of a lineText
        lineOffset = Str::Length(lineText);
        return;
    case KeyCode::LeftArrow:
        if (lineOffset > 1) {
            UTF8::Previous(lineText, lineOffset);
        }
        return;
    case KeyCode::RightArrow:
        if (lineText[lineOffset] && !compositionMode) {
            UTF8::Advance(lineText, lineOffset);
        }
        return;
    case KeyCode::UpArrow: // History up
        do {
            historyLine = (historyLine - 1) & 31;
        } while (historyLine != editLine && !cmdLines[historyLine][1]);

        if (historyLine == editLine) {
            historyLine = (editLine + 1) & 31;
        }

        strcpy(lineText, cmdLines[historyLine]);
        lineOffset = Str::Length(lineText);
        return;
    case KeyCode::DownArrow: // History down
        if (historyLine == editLine) {
            return;
        }

        do {
            historyLine = (historyLine + 1) & 31;
        } while (historyLine != editLine && !cmdLines[historyLine][1]);

        if (historyLine == editLine) {
            lineText[0] = CMDLINE_PROMPT_MARK;
            lineText[1] = 0;
            lineOffset = 1;
        } else {
            strcpy(lineText, cmdLines[historyLine]);
            lineOffset = Str::Length(lineText);
        }
        return;
    default:
        break;
    }
}

void GameClient::ConsoleCharEvent(char32_t unicodeChar) {
    if (unicodeChar < 32) {
        return;
    }

    char temp[7];
    char *tempPtr = temp;
    UTF8::Encode(tempPtr, unicodeChar);
    ptrdiff_t charSize = tempPtr - temp;
    temp[charSize] = '\0';

    char *lineText = cmdLines[editLine];
    char buffer[CommandLineSize];

    if (!replaceMode && strlen(lineText) + charSize >= CommandLineSize) {
        return;
    }

    if (replaceMode) {
        // Replace a character.
        int nextCharOffset = lineOffset;
        UTF8::Advance(lineText, nextCharOffset);

        int appendedBytes = charSize - (nextCharOffset - lineOffset);
        if (strlen(lineText) + appendedBytes >= CommandLineSize) {
            return;
        }

        strcpy(buffer, &lineText[nextCharOffset]);

        strcpy(lineText, temp);
        strcpy(&lineText[charSize], buffer);
    } else if (lineText[lineOffset]) {
        // Insert a character.
        if (!compositionMode) {
            strcpy(buffer, &lineText[lineOffset]);
            strcpy(&lineText[lineOffset + charSize], buffer);
        }
        memcpy(&lineText[lineOffset], temp, charSize);
    } else {
        // Append a character.
        strcpy(&lineText[lineOffset], temp);
    }

    lineOffset += charSize;

    compositionMode = false;
}

void GameClient::ConsoleCompositionEvent(char32_t unicodeChar) {
    char temp[7];
    char *tempPtr = temp;
    UTF8::Encode(tempPtr, unicodeChar);
    ptrdiff_t charSize = tempPtr - temp;
    temp[charSize] = '\0';

    char *lineText = cmdLines[editLine];
    char buffer[CommandLineSize];

    if (unicodeChar == U'\b' && lineOffset > 1) {
        int nextCharOffset = lineOffset;
        UTF8::Advance(lineText, nextCharOffset);

        strcpy(buffer, &lineText[nextCharOffset]);
        strcpy(&lineText[lineOffset], buffer);

        compositionMode = false;
        return;
    }

    if (unicodeChar < 32) {
        return;
    }

    if (!replaceMode && strlen(lineText) + charSize >= CommandLineSize) {
        return;
    }

    if (replaceMode) {
        // Replace a character.
        int nextCharOffset = lineOffset;
        UTF8::Advance(lineText, nextCharOffset);

        int appendedBytes = charSize - (nextCharOffset - lineOffset);
        if (strlen(lineText) + appendedBytes >= CommandLineSize) {
            return;
        }

        strcpy(buffer, &lineText[nextCharOffset]);

        strcpy(lineText, temp);
        strcpy(&lineText[charSize], buffer);
    } else if (lineText[lineOffset]) {
        // Insert a character.
        if (!compositionMode) {
            strcpy(buffer, &lineText[lineOffset]);
            strcpy(&lineText[lineOffset + charSize], buffer);
        }
        memcpy(&lineText[lineOffset], temp, charSize);
    } else {
        // Append a character.
        strcpy(&lineText[lineOffset], temp);
    }

    compositionMode = true;
}

void GameClient::KeyEvent(KeyCode::Enum key, bool down) {
    static bool cursorLocked = false;

    // Toggle console without key binding
    if (key == KeyCode::Grave) {
        if (down) {
            cursorLocked = inputSystem.LockCursor(keyFocus == KeyFocus::Console ? cursorLocked : false);

            cmdSystem.BufferCommandText(CmdSystem::Execution::Append, "toggleConsole\n");
            return;
        }
    } 

    if (keyFocus == KeyFocus::Console) {
        if (down) {
            if (key == KeyCode::Escape) {
                inputSystem.LockCursor(cursorLocked);

                cmdSystem.BufferCommandText(CmdSystem::Execution::Append, "toggleConsole\n");
                return;
            }

            if (key == KeyCode::Insert) {
                replaceMode = !replaceMode;
                return;
            }
            
            ConsoleKeyEvent(key);
            return;
        }

        return;
    }

    inputSystem.KeyEvent(key, down);
}

void GameClient::CharEvent(char32_t unicodeChar) {
    if (keyFocus == KeyFocus::Console) {
        ConsoleCharEvent(unicodeChar);
        return;
    }
}

void GameClient::CompositionEvent(char32_t unicodeChar) {
    if (keyFocus == KeyFocus::Console) {
        ConsoleCompositionEvent(unicodeChar);
        return;
    }
}

void GameClient::MouseMoveEvent(int x, int y, int time) {
    inputSystem.MouseMoveEvent(x, y, time);
}

void GameClient::MouseDeltaEvent(int dx, int dy, int time) {
    if (keyFocus == KeyFocus::Console) {
        return;
    }
 
    inputSystem.MouseDeltaEvent(dx, dy, time);
}

void GameClient::JoyAxisEvent(int dx, int dy, int time) {
    if (keyFocus == KeyFocus::Console) {
        return;
    }

    inputSystem.JoyAxisEvent(dx, dy, time);
}

void GameClient::TouchEvent(InputSystem::Touch::Phase::Enum phase, uint64_t touchId, int x, int y, int time) {
    inputSystem.TouchEvent(phase, touchId, x, y);
}

void GameClient::PacketEvent(/*netadr_t *adr, sizebuf_t *buf*/) {
}

//--------------------------------------------------------------------------------------------------

void GameClient::Cmd_Connect(const CmdArgs &args) {
}

void GameClient::Cmd_Disconnect(const CmdArgs &args) {
}

void GameClient::Cmd_ToggleConsole(const CmdArgs &args) {
    gameClient.ClearCommandLine();

    keyCmdSystem.ClearStates();

    gameClient.SetKeyFocus(gameClient.GetKeyFocus() == KeyFocus::Console ? KeyFocus::Game : KeyFocus::Console);
}

void GameClient::Cmd_ToggleMenuBar(const CmdArgs &args) {
    gameClient.ShowMenuBar(!gameClient.IsMenuBarVisible());
}

void GameClient::Cmd_ToggleStatistics(const CmdArgs &args) {
    gameClient.ShowStatistics(!gameClient.IsStatisticsVisible());
}

BE_NAMESPACE_END
