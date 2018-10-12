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

BE_NAMESPACE_BEGIN

static const wchar_t        CMDLINE_PROMPT_MARK = L']';
static const float          CMDLINE_BLINK_SPEED = 2.0f;

static const int            DRAWTEXT_LINE_SPACING = 4;

static const int            CONSOLE_FONT_HEIGHT = 14;
static const int            CONSOLE_FONT_Y_SPACING = 2;
static const int            CONSOLE_TEXT_BORDER = 8;

static CVAR(cl_updateFps, L"500", CVar::Integer, L"FPS update duration in milliseconds");
static CVAR(cl_conSize, L"0.6", CVar::Float | CVar::Archive, L"");
static CVAR(cl_conSpeed, L"5.0", CVar::Float | CVar::Archive, L"");
static CVAR(cl_conNoPrint, L"1", CVar::Bool, L"");
static CVAR(cl_conNotifyTime, L"3.0", CVar::Float | CVar::Archive, L"");
static CVAR(cl_showFps, L"0", CVar::Bool, L"");
static CVAR(cl_showTimer, L"0", CVar::Bool, L"");

GameClient      gameClient;

void GameClient::Init(void *windowHandle, bool useMouseInput) {
    cmdSystem.AddCommand(L"connect", Cmd_Connect);
    cmdSystem.AddCommand(L"disconnect", Cmd_Disconnect);
    cmdSystem.AddCommand(L"toggleConsole", Cmd_ToggleConsole);

    state = CS_DISCONNECTED;

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
    linePos = 1;

    keyFocus = KEYFOCUS_GAME;
    composition = false;
    insertMode = false;

    for (int i = 0; i < CMDLINE_HISTORY; i++) {
        commandLines[i][0] = CMDLINE_PROMPT_MARK;
        commandLines[i][1] = 0;
    }

    RHI::Settings settings;
    settings.colorBits      = cvarSystem.GetCVarInteger(L"r_colorBits");
    settings.alphaBits      = settings.colorBits == 32 ? 8 : 0;
    settings.depthBits      = cvarSystem.GetCVarInteger(L"r_depthBits");
    settings.stencilBits    = cvarSystem.GetCVarInteger(L"r_stencilBits");
    settings.multiSamples   = cvarSystem.GetCVarInteger(L"r_multiSamples");

    // set default resource GUID mapper
    InitDefaultGuids();

    platform->SetMainWindowHandle(windowHandle);
    platform->EnableMouse(useMouseInput);

    inputSystem.Init();

    renderSystem.Init(windowHandle, &settings);

    soundSystem.Init(windowHandle);

    physicsSystem.Init();

    animControllerManager.Init();
    
    console.ClearNotify();

    console.CheckResize(1280); // FIXME: 해상도에 따른 콘솔버퍼 최대 라인길이 조절 안해야 됨

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
    resourceGuidMapper.Set(GuidMapper::simpleShaderGuid, "Shaders/Simple");
    resourceGuidMapper.Set(GuidMapper::standardSpecularShaderGuid, "Shaders/StandardSpec");
    resourceGuidMapper.Set(GuidMapper::standardShaderGuid, "Shaders/Standard");
    resourceGuidMapper.Set(GuidMapper::phongLightingShaderGuid, "Shaders/Phong");
    resourceGuidMapper.Set(GuidMapper::skyboxCubemapShaderGuid, "Shaders/skyboxCubemap");
    resourceGuidMapper.Set(GuidMapper::skyboxSixSidedShaderGuid, "Shaders/skyboxSixSided");
    resourceGuidMapper.Set(GuidMapper::defaultMaterialGuid, "_defaultMaterial");
    resourceGuidMapper.Set(GuidMapper::whiteMaterialGuid, "_whiteMaterial");
    resourceGuidMapper.Set(GuidMapper::blendMaterialGuid, "_blendMaterial");
    resourceGuidMapper.Set(GuidMapper::whiteLightMaterialGuid, "_whiteLightMaterial");
    resourceGuidMapper.Set(GuidMapper::zeroClampLightMaterialGuid, "_zeroClampLightMaterial");
    resourceGuidMapper.Set(GuidMapper::defaultSkyboxMaterialGuid, "_defaultSkyboxMaterial");
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
    cmdSystem.RemoveCommand(L"connect");
    cmdSystem.RemoveCommand(L"disconnect");
    cmdSystem.RemoveCommand(L"toggleConsole");

    //materialManager.ReleaseMaterial(consoleMaterial);

    animControllerManager.Shutdown();

    physicsSystem.Shutdown();

    soundSystem.Shutdown();

    renderSystem.Shutdown();

    inputSystem.Shutdown();
}

void GameClient::RunFrame() {
    frameCount++;

    time += common.frameTime;

    fpsFrametime += common.frameTime;
    
    if (fpsFrametime >= cl_updateFps.GetInteger()) {
        fps = Math::Rint(fpsFrames / MS2SEC(fpsFrametime));
        fpsFrames = 0;
        fpsFrametime -= cl_updateFps.GetInteger();
    }

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

void GameClient::UpdateConsole() {
    float targetHeight;

    if (keyFocus == KEYFOCUS_CONSOLE) {
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

    console.CheckResize(1280); // FIXME: 해상도에 따른 콘솔버퍼 최대 라인길이 조절안해야 됨
}

void GameClient::EnableConsole(bool flag) {
    if (flag == false && keyFocus == KEYFOCUS_CONSOLE) {
        ClearCommandLine();
        console.ClearNotify();
        SetKeyFocus(KEYFOCUS_GAME);
    }

    consoleEnabled = flag;
}

void GameClient::ClearCommandLine() {
    commandLines[editLine][0] = CMDLINE_PROMPT_MARK;
    commandLines[editLine][1] = 0;
    linePos = 1;
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
    
    renderSystem.GetCurrentRenderContext()->DrawPic(x, y, w, h, rgba.a < 1.0f ? materialManager.blendMaterial : materialManager.whiteMaterial);	
}

void GameClient::DrawString(int x, int y, const wchar_t *str, int len, int flags) {
    int w = renderSystem.GetCurrentRenderContext()->GetRenderingWidth();
    int h = renderSystem.GetCurrentRenderContext()->GetRenderingHeight();
    Rect rect(0, 0, w, h);

    DrawText(rect, x, y, str, len, flags);
}

void GameClient::DrawText(const Rect &rect, int marginX, int marginY, const wchar_t *text, int len, int flags) {
    static const int MaxTextLines = 256;
    const wchar_t *textLines[MaxTextLines];

    textLines[0] = (const wchar_t *)text;

    if (len == -1) {
        len = (int)wcslen(text);
    }
    int lineLen[MaxTextLines];
    int maxLen = len;
    int numLines = 0;
    int lineWidth = 0;

    bool truncated = false;

    const wchar_t *ptr = (const wchar_t *)text;

    while (*ptr && maxLen) {
        if (WS_IS_COLOR(ptr)) {
            ptr += 2;
            maxLen -= 2;
            continue;
        }

        if (flags & DTF_MULTILINE && *ptr == L'\n') {
            lineLen[numLines] = (int)(ptr - textLines[numLines]);
            textLines[++numLines] = ptr + 1;

            if (flags & DTF_WORDWRAP) {
                lineWidth = 0;
            }
        } else {
            int charWidth = currentFont->GetGlyphAdvance(*ptr) * currentTextScale.x;

            if (flags & DTF_WORDWRAP) {
                if (lineWidth + charWidth > rect.w - marginX) {
                    lineLen[numLines] = (int)(ptr - textLines[numLines]);
                    textLines[++numLines] = ptr;
                    lineWidth = 0;
                    continue;
                }
            } else if (flags & DTF_TRUNCATE) {
                if (lineWidth + charWidth > rect.w - marginX) {
                    if (ptr > text) {
                        int dotdotdotWidth = currentFont->GetGlyphAdvance(L'.') * currentTextScale.x * 3;
                
                        do {
                            dotdotdotWidth -= currentFont->GetGlyphAdvance(*ptr) * currentTextScale.x;
                            ptr--;
                        } while (dotdotdotWidth > 0 && ptr > text);
                    
                        truncated = true;
                    }
                    break;
                }
            }

            lineWidth += charWidth;
        }

        ptr++;
        maxLen--;
    }

    if (ptr - textLines[numLines] > 0) {
        lineLen[numLines] = (int)(ptr - textLines[numLines]);
        numLines++;
    }

    // Calculate the coordinate y
    int y;
    if (flags & (DTF_BOTTOM | DTF_VCENTER)) {
        int height = currentFont->GetFontHeight() * currentTextScale.y * numLines + DRAWTEXT_LINE_SPACING * (numLines - 1);

        if (flags & DTF_BOTTOM) {
            y = rect.y + rect.h - height - marginY;
        } else if (flags & DTF_VCENTER) {
            y = rect.y + (rect.h - height) / 2 + marginY;
        }
    } else {
        y = rect.y + marginY;
    }

    GuiMesh &guiMesh = renderSystem.GetCurrentRenderContext()->GetGuiMesh();

    Color4 textColor = currentTextColor;
    guiMesh.SetColor(textColor);

    for (int lineIndex = 0; lineIndex < numLines; lineIndex++) {
        ptr = textLines[lineIndex];

        // Calculate the coordinate x
        int x;
        if (flags & (DTF_RIGHT | DTF_CENTER)) {
            int width = currentFont->StringWidth(ptr, lineLen[lineIndex], false, true, currentTextScale.x);

            if (flags & DTF_RIGHT) {
                x = rect.x + rect.w - width - marginX;
            } else if (flags & DTF_CENTER) {
                x = rect.x + (rect.w - width) / 2 + marginX;
            }
        } else {
            x = rect.x + marginX;
        }

        for (int lineTextIndex = 0; lineTextIndex < lineLen[lineIndex]; lineTextIndex++) {
            if (flags & DTF_OUTLINE) { 
                // Draw outline of text with black color
                guiMesh.SetColor(Color4(0, 0, 0, textColor[3]));

                if (!WS_IS_COLOR(ptr)) {
                    guiMesh.DrawChar(x - 1, y + 0, currentTextScale.x, currentTextScale.y, currentFont, *ptr);
                    guiMesh.DrawChar(x - 1, y - 1, currentTextScale.x, currentTextScale.y, currentFont, *ptr);
                    guiMesh.DrawChar(x + 0, y - 1, currentTextScale.x, currentTextScale.y, currentFont, *ptr);
                    guiMesh.DrawChar(x + 1, y - 1, currentTextScale.x, currentTextScale.y, currentFont, *ptr);
                    guiMesh.DrawChar(x + 1, y + 0, currentTextScale.x, currentTextScale.y, currentFont, *ptr);
                    guiMesh.DrawChar(x + 1, y + 1, currentTextScale.x, currentTextScale.y, currentFont, *ptr);
                    guiMesh.DrawChar(x + 0, y + 1, currentTextScale.x, currentTextScale.y, currentFont, *ptr);
                    guiMesh.DrawChar(x - 1, y + 1, currentTextScale.x, currentTextScale.y, currentFont, *ptr);

                    guiMesh.SetColor(textColor);
                }
            } else if (flags & DTF_DROPSHADOW) { 
                // Draw text shadow with black color
                guiMesh.SetColor(Color4(0, 0, 0, textColor[3]));

                if (!WS_IS_COLOR(ptr)) {
                    guiMesh.DrawChar(x + 1, y + 1, currentTextScale.x, currentTextScale.y, currentFont, *ptr);

                    guiMesh.SetColor(textColor);
                }
            }

            if (WS_IS_COLOR(ptr)) {
                int colorIndex = WC_COLOR_INDEX(*(ptr + 1));
                Clamp(colorIndex, 0, (int)COUNT_OF(Vec4Color::table) - 1);
                textColor.ToColor3() = Vec4Color::table[colorIndex].ToColor3();

                guiMesh.SetColor(textColor);
                ptr += 2;
                lineTextIndex++;
                continue;
            }

            x += guiMesh.DrawChar(x, y, currentTextScale.x, currentTextScale.y, currentFont, *ptr);
            ptr++;
        }

        if (truncated && lineIndex == numLines - 1) {
            x += guiMesh.DrawChar(x, y, currentTextScale.x, currentTextScale.y, currentFont, L'.');
            x += guiMesh.DrawChar(x, y, currentTextScale.x, currentTextScale.y, currentFont, L'.');
            x += guiMesh.DrawChar(x, y, currentTextScale.x, currentTextScale.y, currentFont, L'.');
        }

        y += (currentFont->GetFontHeight() + DRAWTEXT_LINE_SPACING) * currentTextScale.y;
    }
}

void GameClient::DrawConsole() {
    Font *oldFont = gameClient.GetFont();

    SetFont(nullptr);
    SetTextColor(Color4::grey);

    if (cl_showFps.GetBool()) {
        SetTextColor(Color4::white);
        DrawString(0, 0, wva(L"%ifps", fps), -1, DTF_RIGHT | DTF_DROPSHADOW);
    }

    if (cl_showTimer.GetBool()) {
        SetTextColor(Color4::white);

        int ts = (int)(time * 0.001f);
        int hours = (ts / 3600) % 24;
        int minutes = (ts / 60) % 60;
        int seconds = ts % 60;
        
        DrawString(0, CONSOLE_FONT_HEIGHT, wva(L"%02i:%02i:%02i", hours, minutes, seconds), -1, DTF_RIGHT | DTF_DROPSHADOW);
    }

    if (consoleHeight > 0.0f) {
        DrawConsoleScreen();
    } else {
        DrawConsoleNotify();
    }

    SetFont(oldFont);
}

void GameClient::DrawConsoleScreen() {
    wchar_t	version[64];
    
    if (consoleHeight <= 0.0f) {
        return;
    }

    // 콘솔창 뒷배경 그리기
    SetColor(Color4(1.0f, 1.0f, 1.0f, 0.8f));
    DrawPic(0, 0, renderSystem.GetCurrentRenderContext()->GetRenderingWidth(), consoleHeight, consoleMaterial);
    DrawBar(0, consoleHeight, renderSystem.GetCurrentRenderContext()->GetRenderingWidth(), 3, Color4::black);

    // version 표시
    WStr::snPrintf(version, COUNT_OF(version), L"%hs-%hs %i.%i.%i", B_ENGINE_NAME, PlatformProcess::PlatformName(), B_ENGINE_VERSION_MAJOR, B_ENGINE_VERSION_MINOR, B_ENGINE_VERSION_PATCH);
    SetTextColor(Color4(1.0f, 0.5f, 0.0f, 1.0f));
    DrawString(CONSOLE_TEXT_BORDER, consoleHeight - (CONSOLE_FONT_HEIGHT * 1.5f), version, -1, DTF_RIGHT | DTF_DROPSHADOW);

    int commandLines = consoleHeight / (CONSOLE_FONT_HEIGHT + CONSOLE_FONT_Y_SPACING) - 2;	// 표시할 줄수
    int y = consoleHeight - (CONSOLE_FONT_HEIGHT) * 3 - CONSOLE_FONT_Y_SPACING;	// y 좌표

    // 백스크롤 화살표 그리기
    if (consoleUpScroll > 0) {
        for (int i = CONSOLE_TEXT_BORDER; i < renderSystem.GetCurrentRenderContext()->GetRenderingWidth(); i += 100) {
            DrawString(i, y, L"~");
        }
        y -= CONSOLE_FONT_HEIGHT;
    }

    SetTextColor(Color4::white);

    // con_text 내용 그리기
    for (int i = console.currentLine; i >= console.currentLine - commandLines; i--, y -= CONSOLE_FONT_HEIGHT + CONSOLE_FONT_Y_SPACING) {
        int j = i - consoleUpScroll;
        if (j < 0) {
            j = 0;
        }

        wchar_t *text = console.text + (j % console.totalLines) * console.sizeOfLine;
        DrawString(CONSOLE_TEXT_BORDER, y, text, console.sizeOfLine, 0);
    }

    DrawConsoleCmdLine();
}

void GameClient::DrawConsoleCmdLine() {	
    if (keyFocus != KEYFOCUS_CONSOLE) {
        return;
    }

    wchar_t *text = commandLines[editLine];
    int len = WStr::Length(text);

    // 커서 위치 구하기
    int cursorpos = linePos;
    for (int i = 0; i < linePos; i++) {
        if (text[i] == WC_COLOR_ESCAPE && text[i + 1] != WC_COLOR_ESCAPE && text[i + 1] != 0) {
            cursorpos -= 2;
        }
    }

    // 입력이 가로길이를 넘었다면 가로 스크롤
    wchar_t *textPtr = text;
    if (cursorpos >= console.sizeOfLine) {
        textPtr = text + 1 + cursorpos - console.sizeOfLine;
        cursorpos -= console.sizeOfLine;
    }

    // 텍스트 그리기
    len = WStr::Length(textPtr);
    DrawString(CONSOLE_TEXT_BORDER, consoleHeight - CONSOLE_FONT_HEIGHT * 2, textPtr, Min(len, console.sizeOfLine), 0);		
    
    // 캐럿 모양 정하기
    int widthToCursor = currentFont->StringWidth(textPtr, (int)(&text[linePos] - textPtr), false, true, 1.0f);
    int caret_x = CONSOLE_TEXT_BORDER + widthToCursor;
    int caret_y = consoleHeight - CONSOLE_FONT_HEIGHT * 2 + 2;
    int caret_w = 0;
    int caret_h = CONSOLE_FONT_HEIGHT + 2;
    Color4 caret_color;

    if (insertMode || composition) {
        caret_w = currentFont->GetGlyphAdvance(text[linePos]);
        caret_color = Color4(1, 1, 1, 0.5f);
    } else {
        caret_w = 1;
        caret_color = Color4(1, 1, 1, 1);
    }

    // 캐럿 그리기
    if ((int)(time * 0.001f * CMDLINE_BLINK_SPEED) & 1) {
        DrawBar(caret_x, caret_y, caret_w, caret_h, caret_color);
    }
}

void GameClient::DrawConsoleNotify() {
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

        wchar_t *text = console.text + (i % console.totalLines) * console.sizeOfLine;
        DrawString(0, y, text, console.sizeOfLine, DTF_DROPSHADOW);

        y += CONSOLE_FONT_HEIGHT + CONSOLE_FONT_Y_SPACING;
    }
}

void GameClient::ConsoleKeyEvent(KeyCode::Enum key) {
    wchar_t         buffer[CMDLINE_SIZE];
    const wchar_t * cmd;

    wchar_t *line = commandLines[editLine];

    switch (key) {
    case KeyCode::Enter: // enter
        cmdSystem.BufferCommandText(CmdSystem::Append, line + 1); // skip the CMDLINE_PROMPT_MARK
        cmdSystem.BufferCommandText(CmdSystem::Append, L"\n");

        BE_LOG(L"%ls\n", line);
        editLine = (editLine + 1) & 31;
        historyLine = editLine;

        line = commandLines[editLine];
        line[0] = CMDLINE_PROMPT_MARK;
        line[1] = 0;
        linePos = 1;

        consoleUpScroll = 0;
        return;
    case KeyCode::Backspace: // backspace
        if (linePos > 1) {
            wcscpy(buffer, &line[linePos]);
            linePos--;
            wcscpy(&line[linePos], buffer);
        }
        return;
    case KeyCode::Tab:	// command 자동 완성
        cmd = cmdSystem.CompleteCommand(line + 1);
        if (!cmd) {
            cmd = cvarSystem.CompleteVariable(line + 1);
        }

        if (cmd) {
            wcscpy(line + 1, cmd);
            linePos = WStr::Length(line + 1) + 1;
            line[linePos] = 0;
        }
        return;	
    case KeyCode::Delete:
        if (line[linePos]) {
            wcscpy(buffer, &line[linePos+1]);
            wcscpy(&line[linePos], buffer);
        }
        return;
    case KeyCode::PageUp: // 스크롤 업
    case KeyCode::MouseWheelUp:
        consoleUpScroll += 2;
        if (consoleUpScroll > console.totalLines - consoleHeight) {
            consoleUpScroll = console.totalLines - consoleHeight;
        }
        return;
    case KeyCode::PageDown: // 스크롤 다운
    case KeyCode::MouseWheelDown:
        consoleUpScroll -= 2;
        if (consoleUpScroll < 0) {
            consoleUpScroll = 0;
        }
        return;
    case KeyCode::Home: // 커서 처음으로 이동
        linePos = 1;
        return;
    case KeyCode::End:	// 커서 끝으로 이동
        linePos = WStr::Length(line);
        return;
    case KeyCode::LeftArrow:
        if (linePos > 1) {
            linePos--;
        }
        return;
    case KeyCode::RightArrow:
        if (line[linePos] && !composition) {
            linePos++;
        }
        return;
    case KeyCode::UpArrow: // 히스토리 업
        do {
            historyLine = (historyLine - 1) & 31;
        } while (historyLine != editLine && !commandLines[historyLine][1]);

        if (historyLine == editLine) {
            historyLine = (editLine + 1) & 31;
        }

        wcscpy(line, commandLines[historyLine]);
        linePos = WStr::Length(line);
        return;
    case KeyCode::DownArrow: // 히스토리 다운
        if (historyLine == editLine) {
            return;
        }

        do {
            historyLine = (historyLine + 1) & 31;
        } while (historyLine != editLine && !commandLines[historyLine][1]);

        if (historyLine == editLine) {
            line[0] = CMDLINE_PROMPT_MARK;
            line[1] = 0;
            linePos = 1;
        } else {
            wcscpy(line, commandLines[historyLine]);
            linePos = WStr::Length(line);
        }
        return;
    default:
        break;
    }
}

void GameClient::ConsoleCharEvent(wchar_t key) {
    wchar_t buffer[CMDLINE_SIZE];

    wchar_t *line = commandLines[editLine];	

    if (key < 32) {
        return;
    }

    if (linePos >= CMDLINE_SIZE - 1) {
        return;
    }

    if (insertMode) {
        line[linePos] = key;
        linePos++;
    } else if (WStr::Length(line) < CMDLINE_SIZE - 1) {
        if (line[linePos]) {
            if (!composition) {
                wcscpy(buffer, &line[linePos]);
                wcscpy(&line[linePos + 1], buffer);
            }
            line[linePos] = key;
            linePos++;
        } else {
            line[linePos] = key;
            linePos++;
            line[linePos] = 0;
        }			
    }

    composition = false;
}

void GameClient::ConsoleCompositionEvent(wchar_t key) {
    wchar_t buffer[CMDLINE_SIZE];
    wchar_t *line = commandLines[editLine];

    if ((KeyCode::Enum)key == KeyCode::Backspace) {
        if (linePos > 0) {
            wcscpy(buffer, &line[linePos + 1]);
            wcscpy(&line[linePos], buffer);
            composition = false;
        }
        return;
    }

    if (key < 32) {
        return;
    }

    if (linePos >= CMDLINE_SIZE - 1) {
        return;
    }

    if (insertMode) {
        line[linePos] = key;
    } else if (WStr::Length(line) < CMDLINE_SIZE - 1) {
        if (line[linePos]) {
            if (!composition) {
                wcscpy(buffer, &line[linePos]);
                wcscpy(&line[linePos + 1], buffer);
            }
            line[linePos] = key;
        } else {
            line[linePos] = key;
            line[linePos + 1] = 0;
        }
    }

    composition = true;
}

void GameClient::KeyEvent(KeyCode::Enum key, bool down) {
    static bool cursorLocked = false;

    // Toggle console without key binding
    if (key == KeyCode::Grave) {
        if (down) {
            cursorLocked = inputSystem.LockCursor(keyFocus == KEYFOCUS_CONSOLE ? cursorLocked : false);

            cmdSystem.BufferCommandText(CmdSystem::Append, L"toggleConsole\n");
            return;
        }
    } 

    if (keyFocus == KEYFOCUS_CONSOLE) {
        if (down) {
            if (key == KeyCode::Escape) {
                inputSystem.LockCursor(cursorLocked);

                cmdSystem.BufferCommandText(CmdSystem::Append, L"toggleConsole\n");
                return;
            }

            if (key == KeyCode::Insert) {
                insertMode = ! insertMode;
                return;
            }
            
            ConsoleKeyEvent(key);
            return;
        }

        return;
    }

    inputSystem.KeyEvent(key, down);
}

void GameClient::CharEvent(wchar_t key) {
    if (keyFocus == KEYFOCUS_CONSOLE) {
        ConsoleCharEvent(key);
        return;
    }
}

void GameClient::CompositionEvent(int key) {
    if (keyFocus == KEYFOCUS_CONSOLE) {
        ConsoleCompositionEvent(key);
        return;
    }
}

void GameClient::MouseMoveEvent(int x, int y, int time) {
    inputSystem.MouseMoveEvent(x, y, time);
}

void GameClient::MouseDeltaEvent(int dx, int dy, int time) {
    if (keyFocus == KEYFOCUS_CONSOLE) {
        return;
    }
 
    inputSystem.MouseDeltaEvent(dx, dy, time);
}

void GameClient::JoyAxisEvent(int dx, int dy, int time) {
    if (keyFocus == KEYFOCUS_CONSOLE) {
        return;
    }

    inputSystem.JoyAxisEvent(dx, dy, time);
}

void GameClient::TouchEvent(InputSystem::Touch::Phase phase, uint64_t touchId, int x, int y, int time) {
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

    console.ClearNotify();

    keyCmdSystem.ClearStates();

    gameClient.SetKeyFocus(gameClient.GetKeyFocus() == KEYFOCUS_CONSOLE ? KEYFOCUS_GAME : KEYFOCUS_CONSOLE);
}

BE_NAMESPACE_END
