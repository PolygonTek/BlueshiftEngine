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

class RenderContext;
class RenderBackend;
class RenderFrameData;

/*
---------------------------------------------------------------------------------------------------

    RenderSystem

    렌더러 전역 상태와 렌더 스레드를 관리한다.

    - RenderContext 생성/파괴
    - 렌더 스레드와 프레임 동기화
    - 제출된 프레임 데이터를 제출 순서대로 백엔드에서 실행
    - 백엔드 실행과 지연 해제 리소스 정리

---------------------------------------------------------------------------------------------------
*/

class RenderSystem {
public:
    void                            Init(void *mainWindowHandle);
    void                            Shutdown();

    void                            UpdateFrame();

    const RHI::MemoryUsage &        GetVideoMemoryUsage() const { return videoMemoryUsage; }
    bool                            IsUsingRenderThread() const { return !!renderThread; }

    RenderContext *                 CreateRenderContext(void *windowHandle, bool isMain = false);
    void                            DestroyRenderContext(RenderContext *renderContext);
    RenderContext *                 FindRenderContextByWindowHandle(void *windowHandle) const;

                                    // 프레임 데이터를 렌더 스레드에 제출한다. 제출된 프레임 데이터는 렌더 스레드에서 순서대로 처리된다.
    void                            SubmitFrame(RenderContext *renderContext, RenderFrameData *frameData);

    bool                            WaitRenderCompleted(const RenderFrameData *frameData);

    void                            RequestScreenshot(RenderContext *renderContext, const char *filename);

    RenderBackend *                 GetBackend() const { return backEnd; }

private:
    struct SubmittedFrame {
        RenderContext *             renderContext = nullptr;
        RenderFrameData *           frameData = nullptr;
    };

    void                            InitRenderThread();
    void                            ShutdownRenderThread();

    void                            MarkRenderCompleted(RenderFrameData *frameData);
    bool                            WaitSubmittedFrame(SubmittedFrame &submittedFrame);

    static unsigned int             RenderThreadProc(void *param);

    static void                     Cmd_ScreenShot(const BE1::CmdArgs &args);

    BE1::Array<RenderContext *>     renderContexts;
    RenderContext *                 mainRenderContext = nullptr;
    RenderBackend *                 backEnd = nullptr;
    RHI::MemoryUsage                videoMemoryUsage = {};

    BE1::Array<SubmittedFrame>      submittedFrames;
    int                             submittedFrameReadIndex = 0;

    BE1::PlatformSRWLock *          frameSyncLock = nullptr;
    BE1::PlatformCondition *        renderCompletedCondition = nullptr;
    BE1::PlatformCondition *        updateCompletedCondition = nullptr;
    BE1::PlatformThread *           renderThread = nullptr;
    bool                            isStoppingRenderThread = false;
};

extern RenderSystem *               renderSystem;
