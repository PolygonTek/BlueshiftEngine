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
#include "Profiler/Profiler.h"

BE_NAMESPACE_BEGIN

static const char   CMDLINE_PROMPT_MARK = ']';
static const float  CMDLINE_BLINK_SPEED = 2.0f;

static const int    DRAWTEXT_LINE_SPACING = 4;

static const int    CONSOLE_FONT_HEIGHT = 14;
static const int    CONSOLE_FONT_Y_SPACING = 2;
static const int    CONSOLE_TEXT_BORDER = 8;

static CVAR(cl_updateFps, "500", CVar::Integer, "FPS update duration in milliseconds");
static CVAR(cl_conSize, "0.6", CVar::Float | CVar::Archive, "");
static CVAR(cl_conSpeed, "5.0", CVar::Float | CVar::Archive, "");
static CVAR(cl_conNoPrint, "1", CVar::Bool, "");
static CVAR(cl_conNotifyTime, "3.0", CVar::Float | CVar::Archive, "");
static CVAR(cl_showFps, "0", CVar::Bool, "");
static CVAR(cl_showTimer, "0", CVar::Bool, "");

GameClient          gameClient;

void GameClient::Init(void *windowHandle, bool useMouseInput) {
    cmdSystem.AddCommand("connect", Cmd_Connect);
    cmdSystem.AddCommand("disconnect", Cmd_Disconnect);
    cmdSystem.AddCommand("toggleConsole", Cmd_ToggleConsole);

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
    lineOffset = 1;

    keyFocus = KEYFOCUS_GAME;
    compositionMode = false;
    replaceMode = false;

    for (int i = 0; i < CMDLINE_HISTORY; i++) {
        cmdLines[i][0] = CMDLINE_PROMPT_MARK;
        cmdLines[i][1] = 0;
    }

    RHI::Settings settings;
    settings.colorBits      = cvarSystem.GetCVarInteger("r_colorBits");
    settings.alphaBits      = settings.colorBits == 32 ? 8 : 0;
    settings.depthBits      = cvarSystem.GetCVarInteger("r_depthBits");
    settings.stencilBits    = cvarSystem.GetCVarInteger("r_stencilBits");
    settings.multiSamples   = cvarSystem.GetCVarInteger("r_multiSamples");

    // set default resource GUID mapper
    InitDefaultGuids();

    platform->SetMainWindowHandle(windowHandle);
    platform->EnableMouse(useMouseInput);

    inputSystem.Init();

    renderSystem.Init(windowHandle, &settings);

    soundSystem.Init(windowHandle);

    physicsSystem.Init();

    animControllerManager.Init();

    consoleMaterial = nullptr;// materialManager.GetMaterial("Data/EngineMaterials/console.material");    

    currentColor = Color4::white;
    currentTextColor = Color4::white;
    currentTextScale = Vec2(1.0f, 1.0f);
    currentFont = nullptr;
    
    SetFont(fontManager.defaultFont);

    BE_PROFILE_INIT();
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
    resourceGuidMapper.Set(GuidMapper::unlitShaderGuid, "Shaders/Unlit");
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
    cmdSystem.RemoveCommand("connect");
    cmdSystem.RemoveCommand("disconnect");
    cmdSystem.RemoveCommand("toggleConsole");

    BE_PROFILE_SHUTDOWN();

    //materialManager.ReleaseMaterial(consoleMaterial);

    animControllerManager.Shutdown();

    physicsSystem.Shutdown();

    soundSystem.Shutdown();

    renderSystem.Shutdown();

    inputSystem.Shutdown();
}

void GameClient::RunFrame() {
    BE_PROFILE_SYNC_FRAME();

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
}

void GameClient::EnableConsole(bool flag) {
    if (flag == false && keyFocus == KEYFOCUS_CONSOLE) {
        ClearCommandLine();
        SetKeyFocus(KEYFOCUS_GAME);
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

        if ((flags & DTF_MULTILINE) && unicodeChar == U'\n') {
            // Save current line length
            lineLen[numLines++] = currentLineLength;

            currentLineLength = 0;
            currentLineWidth = 0;

            // Save next line offset
            lineOffsets[numLines] = offset;
        } else {
            int charWidth = currentFont->GetGlyphAdvance(unicodeChar) * currentTextScale.x;

            if (flags & DTF_WORDWRAP) {
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
                if (flags & DTF_TRUNCATE) {
                    if (currentLineWidth > 0) {
                        int dotdotdotWidth = currentFont->GetGlyphAdvance(U'.') * currentTextScale.x * 3;
                
                        do {
                            dotdotdotWidth -= currentFont->GetGlyphAdvance(unicodeChar) * currentTextScale.x;
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
        offset = lineOffsets[lineIndex];

        // Calculate the x-coordinate
        int x;
        if (flags & (DTF_RIGHT | DTF_CENTER)) {
            int width = currentFont->StringWidth(&text[offset], lineLen[lineIndex], false, true, currentTextScale.x);

            if (flags & DTF_RIGHT) {
                x = rect.x + rect.w - width - marginX;
            } else if (flags & DTF_CENTER) {
                x = rect.x + (rect.w - width) / 2 + marginX;
            }
        } else {
            x = rect.x + marginX;
        }

        for (int lineTextIndex = 0; lineTextIndex < lineLen[lineIndex]; lineTextIndex++) {
            char32_t unicodeChar = text.UTF8CharAdvance(offset);

            if (flags & DTF_OUTLINE) {
                // Draw outline of text with black color
                guiMesh.SetColor(Color4(0, 0, 0, textColor[3]));

                guiMesh.DrawChar(x - 1, y + 0, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar);
                guiMesh.DrawChar(x - 1, y - 1, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar);
                guiMesh.DrawChar(x + 0, y - 1, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar);
                guiMesh.DrawChar(x + 1, y - 1, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar);
                guiMesh.DrawChar(x + 1, y + 0, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar);
                guiMesh.DrawChar(x + 1, y + 1, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar);
                guiMesh.DrawChar(x + 0, y + 1, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar);
                guiMesh.DrawChar(x - 1, y + 1, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar);

                guiMesh.SetColor(textColor);
            } else if (flags & DTF_DROPSHADOW) {
                // Draw text shadow with black color
                guiMesh.SetColor(Color4(0, 0, 0, textColor[3]));

                guiMesh.DrawChar(x + 1, y + 1, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar);

                guiMesh.SetColor(textColor);
            }

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

            x += guiMesh.DrawChar(x, y, currentTextScale.x, currentTextScale.y, currentFont, unicodeChar);
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
    SetTextColor(Color4::grey);

    if (cl_showFps.GetBool()) {
        SetTextColor(Color4::white);
        DrawString(0, 0, va("%ifps", fps), -1, DTF_RIGHT | DTF_DROPSHADOW);
    }

    if (cl_showTimer.GetBool()) {
        SetTextColor(Color4::white);

        int ts = (int)(time * 0.001f);
        int hours = (ts / 3600) % 24;
        int minutes = (ts / 60) % 60;
        int seconds = ts % 60;
        
        DrawString(0, CONSOLE_FONT_HEIGHT, va("%02i:%02i:%02i", hours, minutes, seconds), -1, DTF_RIGHT | DTF_DROPSHADOW);
    }

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
    DrawString(CONSOLE_TEXT_BORDER, consoleHeight - (CONSOLE_FONT_HEIGHT * 1.5f), versionString, -1, DTF_RIGHT | DTF_DROPSHADOW);

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
    if (keyFocus != KEYFOCUS_CONSOLE) {
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

        int charWidth = currentFont->GetGlyphAdvance(unicodeChar);
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
        caretW = currentFont->GetGlyphAdvance(UTF8::Char(text, offset));
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
        DrawString(0, y, text, console.sizeOfLine, DTF_DROPSHADOW);

        y += CONSOLE_FONT_HEIGHT + CONSOLE_FONT_Y_SPACING;
    }*/
}

void GameClient::ConsoleKeyEvent(KeyCode::Enum key) {
    char buffer[CMDLINE_SIZE];
    const char *cmd;

    char *lineText = cmdLines[editLine];

    switch (key) {
    case KeyCode::Enter: // Enter
        cmdSystem.BufferCommandText(CmdSystem::Append, lineText + 1); // skip the CMDLINE_PROMPT_MARK
        cmdSystem.BufferCommandText(CmdSystem::Append, "\n");

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
    int charSize = tempPtr - temp;
    temp[charSize] = '\0';

    char *lineText = cmdLines[editLine];
    char buffer[CMDLINE_SIZE];

    if (!replaceMode && strlen(lineText) + charSize >= CMDLINE_SIZE) {
        return;
    }

    if (replaceMode) {
        // Replace a character.
        int nextCharOffset = lineOffset;
        UTF8::Advance(lineText, nextCharOffset);

        int appendedBytes = charSize - (nextCharOffset - lineOffset);
        if (strlen(lineText) + appendedBytes >= CMDLINE_SIZE) {
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
    int charSize = tempPtr - temp;
    temp[charSize] = '\0';

    char *lineText = cmdLines[editLine];
    char buffer[CMDLINE_SIZE];

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

    if (!replaceMode && strlen(lineText) + charSize >= CMDLINE_SIZE) {
        return;
    }

    if (replaceMode) {
        // Replace a character.
        int nextCharOffset = lineOffset;
        UTF8::Advance(lineText, nextCharOffset);

        int appendedBytes = charSize - (nextCharOffset - lineOffset);
        if (strlen(lineText) + appendedBytes >= CMDLINE_SIZE) {
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
            cursorLocked = inputSystem.LockCursor(keyFocus == KEYFOCUS_CONSOLE ? cursorLocked : false);

            cmdSystem.BufferCommandText(CmdSystem::Append, "toggleConsole\n");
            return;
        }
    } 

    if (keyFocus == KEYFOCUS_CONSOLE) {
        if (down) {
            if (key == KeyCode::Escape) {
                inputSystem.LockCursor(cursorLocked);

                cmdSystem.BufferCommandText(CmdSystem::Append, "toggleConsole\n");
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
    if (keyFocus == KEYFOCUS_CONSOLE) {
        ConsoleCharEvent(unicodeChar);
        return;
    }
}

void GameClient::CompositionEvent(char32_t unicodeChar) {
    if (keyFocus == KEYFOCUS_CONSOLE) {
        ConsoleCompositionEvent(unicodeChar);
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

    keyCmdSystem.ClearStates();

    gameClient.SetKeyFocus(gameClient.GetKeyFocus() == KEYFOCUS_CONSOLE ? KEYFOCUS_GAME : KEYFOCUS_CONSOLE);
}

BE_NAMESPACE_END
