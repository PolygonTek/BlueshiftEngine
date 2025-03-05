// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "RHI.h"
#include "GuiMesh.h"

class RenderContext;
class VisCamera;
class VisObject;
class Texture;
class SubMesh;
class DrawSurf;

class RenderBackEnd {
public:
    void                            Init();
    void                            Shutdown();

    void                            Execute(const void *data);

private:
    struct DrawObjectTaskDesc {
        int                         threadIndex = -1;
        const DrawSurf **           drawSurfs = nullptr;
        uint32_t                    numDrawSurfs = 0;
        RHI::CommandList *          activeCommandList = nullptr;
    };

    const void *                    ExecuteBeginContext(const void *data);
    const void *                    ExecuteDrawCamera(const void *data);
    const void *                    ExecuteScreenshot(const void *data);
    const void *                    ExecuteSwapBuffers(const void *data);

    void                            DrawCamera3D();
    void                            DrawCamera2D();

    void                            DrawAllSurfaces(const DrawSurf **drawSurfs, uint32_t numDrawSurfs);
    void                            DrawSurfaces(const DrawSurf **drawSurfs, uint32_t numDrawSurfs);
    void                            DrawInstancedSurface(const DrawSurf **instanceSurfs, uint32_t instanceCount);

    void                            DrawSurfacesWithoutTask(const DrawSurf **drawSurfs, uint32_t numDrawSurfs);
#ifdef USE_RENDER_TASK
    void                            DrawSurfacesWithTask(const DrawSurf **drawSurfs, uint32_t numDrawSurfs, uint32_t numTasks);
    void                            DrawSurfacesByTask(RenderBackEnd::DrawObjectTaskDesc *taskDesc);
    static void                     DrawSurfacesByTaskFunction(void *data);
#endif

    void                            DrawSurface(RHI::CommandList *commandList, const DrawSurf *drawSurf);
    void                            DrawInstancedSurface(RHI::CommandList *commandList, const DrawSurf **instanceSurfs, int instanceCount);
    void                            DrawGuiSurface(RHI::CommandList *commandList, const DrawSurf *drawSurf);

    RenderContext *                 currentContext = nullptr;
    RHI::CommandList *              mainCommandList = nullptr;
    uint32_t                        frameCount = 0;

    const VisCamera *               currentVisCamera = nullptr;

    BE1::Array<DrawObjectTaskDesc>  objectDrawingTaskDescs;
    uint32_t                        drawGroupId = -1;
};
