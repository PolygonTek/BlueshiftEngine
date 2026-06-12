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

#pragma once

#include "../RHI.h"
#include <atomic>

struct ImGuiContext;
struct ImDrawData;

class D3D12ImGuiOverlay : public RHI::ImGuiOverlay {
public:
    bool                            Init(void *mainWindowHandle, RHI::SwapChain *swapChain);
    void                            Shutdown();

    bool                            IsInitialized() const { return initialized; }

    RHI::Format                     GetRTVFormat() const { return rtvFormat; }

    virtual bool                    BeginFrameCapture() override;
    virtual void *                  EndFrameCapture() override;

    ImDrawData *                    GetCapturedDrawData(void *userData);
    void                            FreeCapturedDrawData(void *userData);

    virtual bool                    HandleWndProc(void *hwnd, uint32_t message, uintptr_t wParam, intptr_t lParam) override;
    virtual bool                    WantsCaptureMouse() const override;
    virtual bool                    WantsCaptureKeyboard() const override;

private:
    struct CapturedImGuiDrawData;

    CapturedImGuiDrawData *         CaptureDrawData(const ImDrawData *srcDrawData);
    CapturedImGuiDrawData *         AllocCaptureSlot();
    void                            FreeCaptureSlots();
    void                            UpdateCaptureState();

    ImGuiContext *                  context = nullptr;
    CapturedImGuiDrawData *         captureSlots = nullptr;
    int                             captureSlotCount = 0;
    int                             nextCaptureSlotIndex = 0;
    RHI::Format                     rtvFormat = RHI::Format::Unknown;
    std::atomic_bool                cachedWantsCaptureMouse = false;
    std::atomic_bool                cachedWantsCaptureKeyboard = false;
    bool                            initialized = false;
    bool                            frameStarted = false;
};
