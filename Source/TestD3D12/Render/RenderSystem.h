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
#include "RHI/RHI.h"

class RenderContext;
class RenderBackend;
class RenderFrameData;
class ShaderPipeline;

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
    friend class RenderFrameData;
    friend class RenderBackend;

public:
    void                            Init(void *mainWindowHandle);
    void                            Shutdown();

    void                            UpdateFrame();

    const RHI::MemoryUsage &        GetVideoMemoryUsage() const { return videoMemoryUsage; }
    bool                            IsUsingRenderThread() const { return !!renderThread; }

    [[nodiscard]] RenderContext *   CreateRenderContext(void *windowHandle, bool isMain = false);
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
    void                            InitFFTPSOs();
    void                            ReleaseFFTPSOs();
    void                            InitPostProcessPSOs();
    void                            ReleasePostProcessPSOs();

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

    SharedPtr<ShaderPipeline>       toneMappingPSO;
    SharedPtr<ShaderPipeline>       luminancePass1PSO;
    SharedPtr<ShaderPipeline>       luminancePass2PSO;

    SharedPtr<ShaderPipeline>       bloomPrefilterPSO;
    SharedPtr<ShaderPipeline>       bloomDownsamplePSO;
    SharedPtr<ShaderPipeline>       bloomUpsamplePSO;

    SharedPtr<ShaderPipeline>       convolutionBloomPrefilterPSO;
    SharedPtr<ShaderPipeline>       convolutionBloomKernelPreparePSO;
    SharedPtr<ShaderPipeline>       convolutionBloomResolvePSO;

#ifdef USE_FUSED_FFT
    SharedPtr<ShaderPipeline>       fftComplex256PSO;
    SharedPtr<ShaderPipeline>       fftComplex512PSO;
    SharedPtr<ShaderPipeline>       fftComplex1024PSO;
    SharedPtr<ShaderPipeline>       fftReal512PSO;
    SharedPtr<ShaderPipeline>       fftReal1024PSO;
    SharedPtr<ShaderPipeline>       ifftReal512PSO;
    SharedPtr<ShaderPipeline>       ifftReal1024PSO;
    SharedPtr<ShaderPipeline>       fftDualReal256PSO;
    SharedPtr<ShaderPipeline>       fftDualReal512PSO;
    SharedPtr<ShaderPipeline>       fftDualReal1024PSO;
    SharedPtr<ShaderPipeline>       ifftDualReal256PSO;
    SharedPtr<ShaderPipeline>       ifftDualReal512PSO;
    SharedPtr<ShaderPipeline>       ifftDualReal1024PSO;
#else
    SharedPtr<ShaderPipeline>       fftComplex256StagePSO;
    SharedPtr<ShaderPipeline>       fftComplex512StagePSO;
    SharedPtr<ShaderPipeline>       fftComplex1024StagePSO;
    SharedPtr<ShaderPipeline>       fftComplex1024LastStagePSO;
    SharedPtr<ShaderPipeline>       fftReal512FirstStagePSO;
    SharedPtr<ShaderPipeline>       fftReal512LastStagePSO;
    SharedPtr<ShaderPipeline>       fftReal1024FirstStagePSO;
    SharedPtr<ShaderPipeline>       fftReal1024LastStagePSO;
    SharedPtr<ShaderPipeline>       ifftReal512FirstStagePSO;
    SharedPtr<ShaderPipeline>       ifftReal512LastStagePSO;
    SharedPtr<ShaderPipeline>       ifftReal1024FirstStagePSO;
    SharedPtr<ShaderPipeline>       ifftReal1024LastStagePSO;
    SharedPtr<ShaderPipeline>       fftDualReal256FirstStagePSO;
    SharedPtr<ShaderPipeline>       fftDualReal256LastStagePSO;
    SharedPtr<ShaderPipeline>       fftDualReal512FirstStagePSO;
    SharedPtr<ShaderPipeline>       fftDualReal512LastStagePSO;
    SharedPtr<ShaderPipeline>       fftDualReal1024FirstStagePSO;
    SharedPtr<ShaderPipeline>       fftDualReal1024LastStagePSO;
    SharedPtr<ShaderPipeline>       ifftDualReal256FirstStagePSO;
    SharedPtr<ShaderPipeline>       ifftDualReal256LastStagePSO;
    SharedPtr<ShaderPipeline>       ifftDualReal512FirstStagePSO;
    SharedPtr<ShaderPipeline>       ifftDualReal512LastStagePSO;
    SharedPtr<ShaderPipeline>       ifftDualReal1024FirstStagePSO;
    SharedPtr<ShaderPipeline>       ifftDualReal1024LastStagePSO;
#endif

    SharedPtr<ShaderPipeline>       multiplyComplexSpectrum2DPSO;
    SharedPtr<ShaderPipeline>       multiplyPackedRealSpectrum2DPSO;
    SharedPtr<ShaderPipeline>       multiplyPackedDualRealSpectrum2DPSO;

    SharedPtr<ShaderPipeline>       transposeComplex256PSO;
    SharedPtr<ShaderPipeline>       transposeComplex256x512PSO;
    SharedPtr<ShaderPipeline>       transposeComplex512x256PSO;
    SharedPtr<ShaderPipeline>       transposeComplex512x1024PSO;
    SharedPtr<ShaderPipeline>       transposeComplex1024x512PSO;
    SharedPtr<ShaderPipeline>       transposeComplex512PSO;
    SharedPtr<ShaderPipeline>       transposeComplex1024PSO;
};

extern RenderSystem *               renderSystem;
