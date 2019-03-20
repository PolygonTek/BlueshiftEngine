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

#pragma once

BE_NAMESPACE_BEGIN

struct KeyFocus {
    enum Enum {
        Game                = 0,
        Console             = 1
    };
};

struct ClientState {
    enum Enum {
        Idle,
        Active,
        Connecting,
        Connected,
        Disconnected,
        Loading,
        Cinematic
    };
};

struct DrawTextFlag {
    enum Enum {
        Right               = BIT(0),
        Center              = BIT(1),
        Bottom              = BIT(2),
        VCenter             = BIT(3),
        MultiLines          = BIT(5),
        WordWrap            = BIT(6),
        DropShadow          = BIT(7),
        Outline             = BIT(8),
        Truncate            = BIT(9)
    };
};

class Font;
class Material;

class GameClient {
public:
    static constexpr int    CommandLineSize = 256;
    static constexpr int    CommandLineHistory = 32;

    void                    Init(void *windowHandle, bool useMouseInput);
    void                    Shutdown();

    void                    EnableConsole(bool flag);
    void                    ClearCommandLine();

    void                    RunFrame();
    void                    EndFrame();

    KeyFocus::Enum          GetKeyFocus() const { return keyFocus; }
    void                    SetKeyFocus(KeyFocus::Enum keyFocus) { this->keyFocus = keyFocus; }

    Color4                  GetColor() const { return currentColor; }
    Color4                  GetTextColor() const { return currentTextColor; }
    Vec2                    GetTextScale() const { return currentTextScale; }
    Font *                  GetFont() const { return currentFont; }

    void                    SetColor(const Color4 &rgba);
    void                    SetTextColor(const Color4 &rgba);
    void                    SetTextScale(float xscale, float yscale);
    void                    SetFont(Font *font);

    void                    DrawPic(float x, float y, float w, float h, const Material *material);
    void                    DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Material *material);
    void                    DrawBar(float x, float y, float w, float h, const Color4 &rgba);
    void                    DrawString(int x, int y, const Str &str, int size = -1, int flags = 0);
    void                    DrawStringInRect(const Rect &rect, int marginX, int marginY, const Str &str, int size = -1, int flags = 0);

    void                    KeyEvent(KeyCode::Enum key, bool down);
    void                    CharEvent(char32_t unicodeChar);
    void                    CompositionEvent(char32_t unicodeChar);
    void                    MouseMoveEvent(int x, int y, int time);
    void                    MouseDeltaEvent(int dx, int dy, int time);
    void                    JoyAxisEvent(int dx, int dy, int time);
    void                    TouchEvent(InputSystem::Touch::Phase phase, uint64_t touchId, int x, int y, int time);
    void                    PacketEvent();

    void                    DrawConsole();

private:
    void                    InitDefaultGuids();
    void                    ConsoleKeyEvent(KeyCode::Enum key);
    void                    ConsoleCharEvent(char32_t unicodeChar);
    void                    ConsoleCompositionEvent(char32_t ch);
    void                    UpdateConsole();

    void                    DrawConsoleScreen();
    void                    DrawConsoleNotify();
    void                    DrawConsoleCmdLine();
    
    ClientState::Enum       state;

    int                     frameCount;
    int                     time;
    int                     oldTime;

    int                     fps;
    int                     fpsFrames;
    int                     fpsFrametime;
    
    Color4                  currentColor;
    Color4                  currentTextColor;
    Vec2                    currentTextScale;
    Font *                  currentFont;
    
    bool                    consoleEnabled;
    float                   consoleHeight;
    int                     consoleUpScroll;
    Material *              consoleMaterial;

    char                    cmdLines[CommandLineHistory][CommandLineSize];
    int                     editLine;       ///< Current edit line index
    int                     historyLine;    ///< Current history line index
    int                     lineOffset;     ///< Current command line position in byte offset

    KeyFocus::Enum          keyFocus;
    bool                    replaceMode;
    bool                    compositionMode;

    static void             Cmd_Connect(const CmdArgs &args);
    static void             Cmd_Disconnect(const CmdArgs &args);
    static void             Cmd_ToggleConsole(const CmdArgs &args);
};

extern GameClient           gameClient;

BE_NAMESPACE_END
