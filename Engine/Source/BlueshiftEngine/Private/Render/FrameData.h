#pragma once

BE_NAMESPACE_BEGIN

/// All of the information needed by the back end must be contained in.
class FrameData {
public:
    void                    Init();
    void                    Shutdown();
    void                    ToggleFrame();

    void *                  Alloc(int bytes);
    void *                  ClearedAlloc(int bytes);

    RenderCommandBuffer *   GetCommands() { return &commands; }

private:
    struct MemBlock {
        MemBlock *          next;
        int32_t             size;
        int32_t             used;
        byte *              base;
    };

    MemBlock *              mem;
    MemBlock *              alloc;
    RenderCommandBuffer     commands;
};

extern FrameData            frameData;

BE_NAMESPACE_END
