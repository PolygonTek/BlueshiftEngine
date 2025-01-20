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
#include "RHI.h"

class RenderContext;
class VisCamera;

class RenderBackEnd {
public:
    void                            Init();
    void                            Shutdown();

    void                            Execute(const void *data);

private:
    struct DrawObjectTaskDesc {
        const VisCamera *           visCamera = nullptr;
        int                         threadIndex = -1;
        uint32_t                    visObjectStartIndex = 0;
        uint32_t                    visObjectEndIndex = -1;
        RHI::CommandList *          activeCommandList = nullptr;
    };

    const void *                    ExecuteBeginContext(const void *data);
    const void *                    ExecuteDrawCamera(const void *data);
    const void *                    ExecuteScreenshot(const void *data);
    const void *                    ExecuteSwapBuffers(const void *data);

    void                            DrawVisObjects(RHI::CommandList *commandList, const VisCamera *visCamera, uint32_t startIndex, uint32_t endIndex);
    void                            DrawVisObjectsWithoutTask(const VisCamera *visCamera);
#ifdef USE_TASK_MANAGER
    void                            DrawVisObjectsWithTask(const VisCamera *visCamera, uint32_t numTasks);
    void                            DrawVisObjectsByTask(RenderBackEnd::DrawObjectTaskDesc *taskDesc);
    static void                     DrawVisObjectsByTaskFunction(void *data);
#endif

    RenderContext *                 currentContext = nullptr;
    RHI::CommandList *              mainCommandList = nullptr;
    uint32_t                        frameCount = 0;

    BE1::Array<DrawObjectTaskDesc>  objectDrawingTaskDescs;
    uint32_t                        drawGroupId = -1;
};
