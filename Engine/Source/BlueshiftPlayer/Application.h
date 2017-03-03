#pragma once

enum gameState_t {
    GAMESTATE_UNINITIALIZED,    // prior to Init being called
    GAMESTATE_NOMAP,            // no map loaded
    GAMESTATE_STARTUP,          // 
    GAMESTATE_ACTIVE,           // normal gameplay
    GAMESTATE_SHUTDOWN          // 
};

class Application {
public:
    void                    Init();
    void                    Shutdown();

    void                    Update();

    void                    Draw();

    void                    LoadMap(const char *mapName);
    
    void                    OnApplicationPause(bool pause);
    void                    OnApplicationTerminate();

    BE1::RenderContext *    mainRenderContext;

    gameState_t             gameState;

    BE1::Str                mapName;
    BE1::GameWorld *        gameWorld;
    BE1::Font *             gameFont;
    
    static void             Cmd_Map_f(const BE1::CmdArgs &args);
};

extern Application          app;
