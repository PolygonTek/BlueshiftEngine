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

#include "RHI.h"

class VisCamera;
class VisObject;

class RenderFrameData {
public:
    void                            Init(int numThreads);
    void                            Shutdown();

                                    // 프레임 별로 임시로 할당하는 메모리 (not thread-safe)
    void *                          MemAlloc(int size);
    void *                          ClearedMemAlloc(int size);

    VisCamera *                     AllocVisCamera();
    void                            FreeVisCamera();

    VisObject *                     AllocVisObjects(int numVisObjects);
    void                            FreeVisObjects();

    RHI::FrameThreadData *          GetThreadData(int threadIndex) { assert(threadIndex >= 0 && threadIndex < COUNT_OF(threadData)); return threadData[threadIndex]; }

    VisCamera *                     GetVisCamera() const { return visCamera; }

    int                             NumVisObjects() const { return numVisObjects; }
    VisObject *                     GetVisObjects() const { return visObjects; }

    void                            ClearMemAllocs();

    void                            BeginFrame();
    void                            EndFrame();

    uint64_t                        GetFenceValue() const { return fenceValue; }
    void                            SetFenceValue(uint64_t fenceValue) { this->fenceValue = fenceValue; }

private:
    struct MemBlock {
        MemBlock *                  next;
        int32_t                     size;
        int32_t                     used;
        byte *                      base;
    };

    void                            InitMemBlocks();
    void                            ClearMemBlocks();
    MemBlock *                      AllocMemBlock();

    MemBlock *                      headBlock;
    MemBlock *                      currentBlock;

    VisCamera *                     visCamera = nullptr;

    int                             numVisObjects = 0;
    VisObject *                     visObjects = nullptr;

#ifdef USE_RENDER_TASK
    RHI::FrameThreadData *          threadData[MaxRenderTaskThreads] = {};
#else
    RHI::FrameThreadData *          threadData[1] = {};
#endif
    int                             numThreads = 0;

    uint64_t                        fenceValue = 0;
};
