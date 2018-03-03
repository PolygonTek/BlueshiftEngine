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

#define CMDLINE_SIZE        256
#define CMDLINE_HISTORY     32

enum KeyFocus {
    KEYFOCUS_GAME           = 0,
    KEYFOCUS_CONSOLE        = 1
};

enum ClientState {
    CS_IDLE,
    CS_ACTIVE,
    CS_CONNECTING,
    CS_CONNECTED,
    CS_DISCONNECTED,
    CS_LOADING,
    CS_CINEMATIC,
};

enum DrawTextFlag {
    DTF_RIGHT               = BIT(0),
    DTF_CENTER              = BIT(1),
    DTF_BOTTOM              = BIT(2),
    DTF_VCENTER             = BIT(3),
    DTF_MULTILINE           = BIT(5),
    DTF_WORDWRAP            = BIT(6),
    DTF_DROPSHADOW          = BIT(7),
    DTF_OUTLINE             = BIT(8),
    DTF_TRUNCATE            = BIT(9)
};

class Font;
class Material;

class GameClient {
public:
    void                Init(void *windowHandle, bool useMouseInput);
    void                Shutdown();

    void                EnableConsole(bool flag);
    void                ClearCommandLine();

    void                RunFrame();
    void                EndFrame();

    int	                GetKeyFocus() const { return keyFocus; }
    void                SetKeyFocus(int focus) { keyFocus = focus; }

    Color4              GetColor() const { return currentColor; }
    Color4              GetTextColor() const { return currentTextColor; }
    Vec2                GetTextScale() const { return currentTextScale; }
    Font *              GetFont() const { return currentFont; }	

    void                SetColor(const Color4 &rgba);
    void                SetTextColor(const Color4 &rgba);
    void                SetTextScale(float xscale, float yscale);
    void                SetFont(Font *font);

    void                DrawPic(float x, float y, float w, float h, const Material *material);
    void                DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const Material *material);
    void                DrawBar(float x, float y, float w, float h, const Color4 &rgba);
    void                DrawString(int x, int y, const wchar_t *str, int len = -1, int flags = 0);
    void                DrawText(const Rect &rect, int xmargin, int ymargin, const wchar_t *str, int len = -1, int flags = 0);

    void                KeyEvent(KeyCode::Enum key, bool down);
    void                CharEvent(wchar_t key);
    void                CompositionEvent(int key);
    void                MouseMoveEvent(int x, int y, int time);
    void                MouseDeltaEvent(int dx, int dy, int time);
    void                JoyAxisEvent(int dx, int dy, int time);
    void                TouchEvent(InputSystem::Touch::Phase phase, uint64_t touchId, int x, int y, int time);
    void                PacketEvent();

    void                DrawConsole();

private:
    void                InitDefaultGuids();
    void                ConsoleKeyEvent(KeyCode::Enum key);
    void                ConsoleCharEvent(wchar_t key);
    void                ConsoleCompositionEvent(wchar_t key);
    void                UpdateConsole();
    void                UpdateScreen();

    void                DrawConsoleScreen();
    void                DrawConsoleNotify();
    void                DrawConsoleCmdLine();
    
    ClientState         state;

    int                 frameCount;
    int                 time;
    int                 oldTime;

    int                 fps;
    int                 fpsFrames;
    int                 fpsFrametime;
    
    Color4              currentColor;
    Color4              currentTextColor;
    Vec2                currentTextScale;
    Font *              currentFont;
    
    bool                consoleEnabled;
    float               consoleHeight;
    int                 consoleUpScroll;
    Material *          consoleMaterial;

    wchar_t             commandLines[CMDLINE_HISTORY][CMDLINE_SIZE];
    int                 editLine;
    int                 historyLine;
    int                 linePos;

    int                 keyFocus;
    bool                composition;
    bool                insertMode;

    static void         Cmd_Connect(const CmdArgs &args);
    static void         Cmd_Disconnect(const CmdArgs &args);
    static void         Cmd_ToggleConsole(const CmdArgs &args);
};

extern GameClient       gameClient;

BE_NAMESPACE_END
