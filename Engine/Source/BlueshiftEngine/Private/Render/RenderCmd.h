#pragma once

/*
-------------------------------------------------------------------------------

    Render command

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

const int RenderCommandBufferSize = 0x80000;

enum RenderCommand {
    EndOfCommand,
    BeginContextCommand,
    DrawViewCommand,
    ScreenShotCommand,
    SwapBuffersCommand
};

struct RenderCommandBuffer {
    byte                data[RenderCommandBufferSize];
    int                 used;
};

struct BeginContextRenderCommand {
    int                 commandId;
    RenderContext *     renderContext;
};

struct DrawViewRenderCommand {
    int                 commandId;
    view_t              view;
};

struct ScreenShotRenderCommand {
    int                 commandId;
    int                 x;
    int                 y;
    int                 width;
    int                 height;
    char                filename[MaxAbsolutePath];
};

struct SwapBuffersRenderCommand {
    int                 commandId;
};

BE_NAMESPACE_END
