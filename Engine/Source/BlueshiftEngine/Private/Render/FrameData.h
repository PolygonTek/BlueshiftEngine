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

#include "RenderCmd.h"

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
