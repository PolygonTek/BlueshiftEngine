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
#include "D3D12ImGuiOverlay.h"
#include "D3D12Renderer.h"
#include "D3D12SwapChain.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include <mutex>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct D3D12ImGuiOverlay::CapturedImGuiDrawData {
    D3D12ImGuiOverlay *         owner = nullptr;
    ImDrawData                  drawData;
    ImVector<ImDrawList *>      ownedDrawLists;
    std::atomic_bool            inUse = false;
};

// Win32 capture changes can synchronously re-enter WndProc while the ImGui handler is running.
static std::recursive_mutex     imguiContextMutex;
static constexpr float          imguiBaseFontSize = 12.0f;

template <typename T>
static void CopyImVector(ImVector<T> &dst, const ImVector<T> &src) {
    dst.resize(src.Size);
    if (src.Size > 0) {
        memcpy(dst.Data, src.Data, (size_t)src.Size * sizeof(T));
    }
}

static void CopyImDrawList(ImDrawList *dstDrawList, const ImDrawList *srcDrawList) {
    CopyImVector(dstDrawList->CmdBuffer, srcDrawList->CmdBuffer);
    CopyImVector(dstDrawList->IdxBuffer, srcDrawList->IdxBuffer);
    CopyImVector(dstDrawList->VtxBuffer, srcDrawList->VtxBuffer);
    CopyImVector(dstDrawList->_CallbacksDataBuf, srcDrawList->_CallbacksDataBuf);
    dstDrawList->Flags = srcDrawList->Flags;

    if (dstDrawList->_CallbacksDataBuf.Size > 0) {
        for (ImDrawCmd &cmd : dstDrawList->CmdBuffer) {
            if (cmd.UserCallback != nullptr && cmd.UserCallbackDataOffset != -1 && cmd.UserCallbackDataSize > 0) {
                cmd.UserCallbackData = dstDrawList->_CallbacksDataBuf.Data + cmd.UserCallbackDataOffset;
            }
        }
    }
}

D3D12ImGuiOverlay::CapturedImGuiDrawData *D3D12ImGuiOverlay::AllocCaptureSlot() {
    if (!captureSlots || captureSlotCount <= 0) {
        return nullptr;
    }

    for (int slotOffset = 0; slotOffset < captureSlotCount; ++slotOffset) {
        const int slotIndex = (nextCaptureSlotIndex + slotOffset) % captureSlotCount;
        CapturedImGuiDrawData &captureSlot = captureSlots[slotIndex];
        bool expected = false;
        if (captureSlot.inUse.compare_exchange_strong(expected, true, std::memory_order_acquire, std::memory_order_relaxed)) {
            nextCaptureSlotIndex = (slotIndex + 1) % captureSlotCount;
            return &captureSlot;
        }
    }

    return nullptr;
}

D3D12ImGuiOverlay::CapturedImGuiDrawData *D3D12ImGuiOverlay::CaptureDrawData(const ImDrawData *srcDrawData) {
    if (!srcDrawData || !srcDrawData->Valid || srcDrawData->CmdListsCount <= 0 || srcDrawData->TotalVtxCount <= 0) {
        return nullptr;
    }

    CapturedImGuiDrawData *capturedDrawData = AllocCaptureSlot();
    if (!capturedDrawData) {
        return nullptr;
    }

    while (capturedDrawData->ownedDrawLists.Size < srcDrawData->CmdListsCount) {
        capturedDrawData->ownedDrawLists.push_back(IM_NEW(ImDrawList(nullptr)));
    }

    capturedDrawData->drawData.Valid = srcDrawData->Valid;
    capturedDrawData->drawData.TotalIdxCount = srcDrawData->TotalIdxCount;
    capturedDrawData->drawData.TotalVtxCount = srcDrawData->TotalVtxCount;
    capturedDrawData->drawData.DisplayPos = srcDrawData->DisplayPos;
    capturedDrawData->drawData.DisplaySize = srcDrawData->DisplaySize;
    capturedDrawData->drawData.FramebufferScale = srcDrawData->FramebufferScale;
    capturedDrawData->drawData.OwnerViewport = srcDrawData->OwnerViewport;
    // Texture updates are completed during capture while the ImGui context is locked.
    capturedDrawData->drawData.Textures = nullptr;
    capturedDrawData->drawData.CmdLists.resize(srcDrawData->CmdListsCount);

    for (int listIndex = 0; listIndex < srcDrawData->CmdListsCount; ++listIndex) {
        ImDrawList *dstDrawList = capturedDrawData->ownedDrawLists[listIndex];
        CopyImDrawList(dstDrawList, srcDrawData->CmdLists[listIndex]);
        capturedDrawData->drawData.CmdLists[listIndex] = dstDrawList;
    }

    capturedDrawData->drawData.CmdListsCount = capturedDrawData->drawData.CmdLists.Size;
    return capturedDrawData;
}

void D3D12ImGuiOverlay::FreeCaptureSlots() {
    if (!captureSlots) {
        return;
    }

    for (int slotIndex = 0; slotIndex < captureSlotCount; ++slotIndex) {
        CapturedImGuiDrawData &captureSlot = captureSlots[slotIndex];
        captureSlot.drawData.Clear();
        captureSlot.ownedDrawLists.clear_delete();
        captureSlot.inUse.store(false, std::memory_order_relaxed);
    }

    delete [] captureSlots;
    captureSlots = nullptr;
    captureSlotCount = 0;
    nextCaptureSlotIndex = 0;
}

void D3D12ImGuiOverlay::UpdateCaptureState() {
    ImGuiIO &io = ImGui::GetIO();
    cachedWantsCaptureMouse.store(io.WantCaptureMouse, std::memory_order_relaxed);
    cachedWantsCaptureKeyboard.store(io.WantCaptureKeyboard, std::memory_order_relaxed);
}

RHI::ImGuiOverlay *D3D12Renderer::CreateImGuiOverlay(void *windowHandle, RHI::SwapChain *swapChain) {
    D3D12ImGuiOverlay *overlay = new D3D12ImGuiOverlay;
    if (!overlay->Init(windowHandle, swapChain)) {
        delete overlay;
        return nullptr;
    }
    return overlay;
}

void D3D12Renderer::DestroyImGuiOverlay(RHI::ImGuiOverlay *overlay) {
    if (!overlay) {
        return;
    }

    D3D12ImGuiOverlay *_overlay = static_cast<D3D12ImGuiOverlay *>(overlay);
    _overlay->Shutdown();
    delete _overlay;
}

bool D3D12ImGuiOverlay::Init(void *mainWindowHandle, RHI::SwapChain *swapChain) {
    if (initialized) {
        return true;
    }
    if (!mainWindowHandle || !swapChain) {
        BE_WARNLOG("D3D12ImGuiOverlay::Init: invalid window handle or swap chain\n");
        return false;
    }

    D3D12Renderer *renderer = D3D12Renderer::GetRenderer();
    if (!renderer) {
        BE_WARNLOG("D3D12ImGuiOverlay::Init: D3D12 renderer is not ready\n");
        return false;
    }
    rtvFormat = swapChain->GetRTVFormat();

    IMGUI_CHECKVERSION();

    std::lock_guard<std::recursive_mutex> contextLock(imguiContextMutex);

    ImGuiContext *previousContext = ImGui::GetCurrentContext();
    context = ImGui::CreateContext();
    ImGui::SetCurrentContext(context);

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGui::StyleColorsDark();

    float dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd(mainWindowHandle);
    if (dpiScale <= 0.0f) {
        dpiScale = 1.0f;
    }

    ImGuiStyle &style = ImGui::GetStyle();
    style.FontSizeBase = imguiBaseFontSize * dpiScale;
    style.FontScaleMain = 1.0f;
    style.FontScaleDpi = 1.0f;
    style.WindowRounding = imguiBaseFontSize * 0.25f;
    style.ScaleAllSizes(dpiScale);

    ImFontConfig fontConfig;
    fontConfig.SizePixels = imguiBaseFontSize * dpiScale;
    io.FontDefault = io.Fonts->AddFontDefaultVector(&fontConfig);

    if (!ImGui_ImplWin32_Init(mainWindowHandle)) {
        BE_WARNLOG("D3D12ImGuiOverlay::Init: ImGui_ImplWin32_Init failed\n");
        ImGui::DestroyContext(context);
        context = nullptr;
        ImGui::SetCurrentContext(previousContext);
        return false;
    }

    if (!renderer->InitImGuiOverlayRenderer(this, swapChain)) {
        BE_WARNLOG("D3D12ImGuiOverlay::Init: ImGui renderer initialization failed\n");
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(context);
        context = nullptr;
        ImGui::SetCurrentContext(previousContext);
        return false;
    }

    captureSlotCount = D3D12SwapChain::NumSwapChainBuffers;
    captureSlots = new CapturedImGuiDrawData[captureSlotCount];
    for (int slotIndex = 0; slotIndex < captureSlotCount; ++slotIndex) {
        captureSlots[slotIndex].owner = this;
    }

    initialized = true;
    ImGui::SetCurrentContext(previousContext);
    return true;
}

void D3D12ImGuiOverlay::Shutdown() {
    if (!initialized && !context) {
        return;
    }

    std::lock_guard<std::recursive_mutex> contextLock(imguiContextMutex);
    ImGuiContext *previousContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(context);

    FreeCaptureSlots();

    if (initialized) {
        D3D12Renderer *renderer = D3D12Renderer::GetRenderer();
        if (renderer) {
            renderer->ShutdownImGuiOverlayRenderer(this);
        }
        ImGui_ImplWin32_Shutdown();
    }

    if (context) {
        ImGui::DestroyContext(context);
        context = nullptr;
    }
    ImGui::SetCurrentContext(previousContext);

    initialized = false;
    frameStarted = false;
    rtvFormat = RHI::Format::Unknown;
    cachedWantsCaptureMouse.store(false, std::memory_order_relaxed);
    cachedWantsCaptureKeyboard.store(false, std::memory_order_relaxed);
}

bool D3D12ImGuiOverlay::BeginFrameCapture() {
    if (!initialized || !context || frameStarted) {
        return false;
    }

    imguiContextMutex.lock();
    ImGui::SetCurrentContext(context);
    frameStarted = true;

    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    UpdateCaptureState();

    return true;
}

void *D3D12ImGuiOverlay::EndFrameCapture() {
    if (!frameStarted) {
        return nullptr;
    }

    ImGui::SetCurrentContext(context);
    ImGui::Render();
    UpdateCaptureState();

    ImDrawData *drawData = ImGui::GetDrawData();
    D3D12Renderer *renderer = D3D12Renderer::GetRenderer();
    if (renderer && !renderer->UpdateImGuiOverlayTextures(this, drawData ? drawData->Textures : nullptr)) {
        BE_WARNLOG("D3D12ImGuiOverlay::EndFrameCapture: failed to update ImGui textures\n");
    }

    void *capturedDrawData = CaptureDrawData(drawData);

    frameStarted = false;
    imguiContextMutex.unlock();

    return capturedDrawData;
}

ImDrawData *D3D12ImGuiOverlay::GetCapturedDrawData(void *userData) {
    CapturedImGuiDrawData *capturedDrawData = static_cast<CapturedImGuiDrawData *>(userData);
    if (!initialized || !context || !capturedDrawData || capturedDrawData->owner != this || !capturedDrawData->inUse.load(std::memory_order_acquire) || !capturedDrawData->drawData.Valid) {
        return nullptr;
    }

    return &capturedDrawData->drawData;
}

void D3D12ImGuiOverlay::FreeCapturedDrawData(void *userData) {
    CapturedImGuiDrawData *capturedDrawData = static_cast<CapturedImGuiDrawData *>(userData);
    if (!capturedDrawData || capturedDrawData->owner != this) {
        return;
    }

    capturedDrawData->drawData.Clear();
    capturedDrawData->inUse.store(false, std::memory_order_release);
}

bool D3D12ImGuiOverlay::HandleWndProc(void *hwnd, uint32_t message, uintptr_t wParam, intptr_t lParam) {
    if (!initialized || !context) {
        return false;
    }

    std::lock_guard<std::recursive_mutex> contextLock(imguiContextMutex);
    ImGuiContext *previousContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(context);
    const bool handled = ImGui_ImplWin32_WndProcHandler(reinterpret_cast<HWND>(hwnd), static_cast<UINT>(message), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam)) != 0;
    UpdateCaptureState();
    ImGui::SetCurrentContext(previousContext);
    return handled;
}

bool D3D12ImGuiOverlay::WantsCaptureMouse() const {
    return cachedWantsCaptureMouse.load(std::memory_order_relaxed);
}

bool D3D12ImGuiOverlay::WantsCaptureKeyboard() const {
    return cachedWantsCaptureKeyboard.load(std::memory_order_relaxed);
}
