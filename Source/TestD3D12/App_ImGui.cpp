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
#include "Platform/PlatformSystem.h"
#include "Render/RenderSystem.h"
#include "Render/RenderCvars.h"
#include "Render/RenderContext.h"
#include "Render/RenderBackend.h"
#include "Render/RenderWorld.h"
#include "Render/RenderCamera.h"
#include "Render/Texture/Texture.h"
#include "GameObject.h"
#include "App.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

extern BE1::CVar       app_showFps;
extern BE1::CVar       app_showStats;
extern BE1::CVar       app_convolutionBloomKernelSource;
extern BE1::CVar       app_convolutionBloomKernelPreset;
extern BE1::CVar       app_convolutionBloomAperturePreset;

#ifdef ENABLE_IMGUI
void App::UpdateImGuiStatsPlotHistory() {
    const float stats[ImGuiStatsPlotSeriesCount] = {
        statFrameAverageMsec,
        statGameAverageMsec,
        statRenderFrontendAverageMsec,
        renderSystem && renderSystem->GetBackend() ? renderSystem->GetBackend()->GetStatRenderAverageMsec() : 0.0f,
        mainRenderContext ? mainRenderContext->GetStatGpuAverageMsec() : 0.0f
    };

    for (int seriesIndex = 0; seriesIndex < ImGuiStatsPlotSeriesCount; ++seriesIndex) {
        imguiStatsPlotSamples[seriesIndex][imguiStatsPlotWriteIndex] = stats[seriesIndex];
    }

    imguiStatsPlotWriteIndex = (imguiStatsPlotWriteIndex + 1) % ImGuiStatsPlotHistoryLength;
    imguiStatsPlotSampleCount = BE1::Min(imguiStatsPlotSampleCount + 1, ImGuiStatsPlotHistoryLength);
}

void App::BuildImGuiStatsPlotWindow() {
    if (!imguiShowStatsPlotWindow) {
        return;
    }

    static const char *statNames[ImGuiStatsPlotSeriesCount] = {
        "Frame",
        "Game",
        "Frontend",
        "Backend",
        "GPU"
    };
    static const ImVec4 statColors[ImGuiStatsPlotSeriesCount] = {
        ImVec4(0.90f, 0.35f, 0.25f, 1.00f),
        ImVec4(0.30f, 0.75f, 0.35f, 1.00f),
        ImVec4(0.25f, 0.55f, 1.00f, 1.00f),
        ImVec4(0.95f, 0.75f, 0.25f, 1.00f),
        ImVec4(0.75f, 0.45f, 1.00f, 1.00f)
    };

    const float fontSize = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2(fontSize * 42.0f, fontSize * 24.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Stats Plot", &imguiShowStatsPlotWindow)) {
        ImGui::End();
        return;
    }

    const int latestSampleIndex = (imguiStatsPlotWriteIndex + ImGuiStatsPlotHistoryLength - 1) % ImGuiStatsPlotHistoryLength;
    for (int seriesIndex = 0; seriesIndex < ImGuiStatsPlotSeriesCount; ++seriesIndex) {
        ImGui::ColorButton(statNames[seriesIndex], statColors[seriesIndex], ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(fontSize, fontSize));
        ImGui::SameLine();
        ImGui::Text("%s %.2f ms", statNames[seriesIndex], imguiStatsPlotSamples[seriesIndex][latestSampleIndex]);
    }

    float maxMsec = 1.0f;
    for (int seriesIndex = 0; seriesIndex < ImGuiStatsPlotSeriesCount; ++seriesIndex) {
        for (int sampleOffset = 0; sampleOffset < imguiStatsPlotSampleCount; ++sampleOffset) {
            const int sampleIndex = (imguiStatsPlotSampleCount == ImGuiStatsPlotHistoryLength)
                ? (imguiStatsPlotWriteIndex + sampleOffset) % ImGuiStatsPlotHistoryLength
                : sampleOffset;
            maxMsec = BE1::Max(maxMsec, imguiStatsPlotSamples[seriesIndex][sampleIndex]);
        }
    }
    maxMsec *= 1.15f;

    ImGui::Text("Scale 0.00 - %.2f ms", maxMsec);

    const ImVec2 plotSize(ImGui::GetContentRegionAvail().x, fontSize * 12.0f);
    ImGui::InvisibleButton("##StatsPlotCanvas", plotSize);

    const ImVec2 plotMin = ImGui::GetItemRectMin();
    const ImVec2 plotMax = ImGui::GetItemRectMax();
    ImDrawList *drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(plotMin, plotMax, IM_COL32(16, 18, 22, 220));
    drawList->AddRect(plotMin, plotMax, IM_COL32(96, 104, 116, 255));

    for (int gridIndex = 1; gridIndex < 4; ++gridIndex) {
        const float y = plotMin.y + (plotMax.y - plotMin.y) * gridIndex / 4.0f;
        drawList->AddLine(ImVec2(plotMin.x, y), ImVec2(plotMax.x, y), IM_COL32(96, 104, 116, 80));
    }

    if (imguiStatsPlotSampleCount > 1) {
        const float plotWidth = plotMax.x - plotMin.x;
        const float plotHeight = plotMax.y - plotMin.y;
        const float invMaxMsec = 1.0f / maxMsec;

        for (int seriesIndex = 0; seriesIndex < ImGuiStatsPlotSeriesCount; ++seriesIndex) {
            const ImU32 color = ImGui::ColorConvertFloat4ToU32(statColors[seriesIndex]);

            for (int sampleOffset = 1; sampleOffset < imguiStatsPlotSampleCount; ++sampleOffset) {
                const int prevSampleIndex = (imguiStatsPlotSampleCount == ImGuiStatsPlotHistoryLength)
                    ? (imguiStatsPlotWriteIndex + sampleOffset - 1) % ImGuiStatsPlotHistoryLength
                    : sampleOffset - 1;
                const int sampleIndex = (imguiStatsPlotSampleCount == ImGuiStatsPlotHistoryLength)
                    ? (imguiStatsPlotWriteIndex + sampleOffset) % ImGuiStatsPlotHistoryLength
                    : sampleOffset;

                const float x0 = plotMin.x + plotWidth * (sampleOffset - 1) / (imguiStatsPlotSampleCount - 1);
                const float x1 = plotMin.x + plotWidth * sampleOffset / (imguiStatsPlotSampleCount - 1);
                const float y0 = plotMax.y - BE1::Min(imguiStatsPlotSamples[seriesIndex][prevSampleIndex] * invMaxMsec, 1.0f) * plotHeight;
                const float y1 = plotMax.y - BE1::Min(imguiStatsPlotSamples[seriesIndex][sampleIndex] * invMaxMsec, 1.0f) * plotHeight;

                drawList->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), color, 1.5f);
            }
        }
    }

    ImGui::End();
}

struct ImGuiTextureViewerEntry {
    char                    label[64] = {};
    const RHI::Texture *    texture = nullptr;
};

static void AddImGuiTextureViewerEntry(ImGuiTextureViewerEntry *entries, int maxEntries, int &entryCount, const char *label, const RHI::Texture *texture) {
    if (entryCount >= maxEntries || !texture || texture->GetViewCount(RHI::ViewType::SRV) == 0) {
        return;
    }

    ImGuiTextureViewerEntry &entry = entries[entryCount++];
    BE1::Str::Copynz(entry.label, label, sizeof(entry.label));
    entry.texture = texture;
}

static void AppendImGuiTextureFlagName(char *text, size_t textSize, bool &hasText, RHI::TextureFlag flag) {
    if (!text || textSize == 0) {
        return;
    }

    const char *flagName = RHI::Renderer::TextureFlagName(flag);
    const char *separator = hasText ? " | " : "";
    const size_t textLength = strlen(text);
    if (textLength >= textSize - 1) {
        return;
    }

    BE1::Str::snPrintf(text + textLength, static_cast<int>(textSize - textLength), "%s%s", separator, flagName);
    hasText = true;
}

static void TextureFlagsToText(RHI::TextureFlag flags, char *text, size_t textSize) {
    if (!text || textSize == 0) {
        return;
    }

    text[0] = '\0';
    if (flags == RHI::TextureFlag::None) {
        BE1::Str::Copynz(text, RHI::Renderer::TextureFlagName(RHI::TextureFlag::None), static_cast<int>(textSize));
        return;
    }

    bool hasText = false;
    if (BE1::HasFlag(flags, RHI::TextureFlag::RenderTarget)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::RenderTarget);
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::DepthStencil)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::DepthStencil);
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::Typeless)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::Typeless);
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::ShaderResource)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::ShaderResource);
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::UnorderedAccess)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::UnorderedAccess);
    }
    if (BE1::HasFlag(flags, RHI::TextureFlag::SkipDefaultViews)) {
        AppendImGuiTextureFlagName(text, textSize, hasText, RHI::TextureFlag::SkipDefaultViews);
    }

    if (!hasText) {
        BE1::Str::Copynz(text, "Invalid", static_cast<int>(textSize));
    }
}

static bool DrawImGuiTexturePreview(const RHI::Texture *texture, RHI::ViewID viewID, float maxPreviewSize, bool center) {
    if (!texture || maxPreviewSize <= 0.0f) {
        return false;
    }

    RHI::TextureViewInfo viewInfo;
    const bool hasViewInfo = texture->GetViewInfo(RHI::ViewType::SRV, viewID, viewInfo);
    const uint32_t textureID = texture->GetBindlessIndex(RHI::ViewType::SRV, viewID);
    const uint32_t previewWidth = hasViewInfo ? viewInfo.width : texture->GetWidth();
    const uint32_t previewHeight = hasViewInfo ? viewInfo.height : texture->GetHeight();
    if (textureID == 0 || previewWidth == 0 || previewHeight == 0) {
        return false;
    }

    const float previewAspectRatio = static_cast<float>(previewWidth) / previewHeight;
    ImVec2 previewSize;
    if (previewAspectRatio > 1.0f) {
        previewSize.x = maxPreviewSize;
        previewSize.y = maxPreviewSize / previewAspectRatio;
    } else {
        previewSize.x = maxPreviewSize * previewAspectRatio;
        previewSize.y = maxPreviewSize;
    }

    const ImVec2 previewFrameSize(maxPreviewSize, maxPreviewSize);
    if (center) {
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        if (availableWidth > previewFrameSize.x) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availableWidth - previewFrameSize.x) * 0.5f);
        }
    }

    const ImVec2 framePos = ImGui::GetCursorScreenPos();
    const ImVec2 imagePos(
        framePos.x + (previewFrameSize.x - previewSize.x) * 0.5f,
        framePos.y + (previewFrameSize.y - previewSize.y) * 0.5f);

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(
        framePos,
        ImVec2(framePos.x + previewFrameSize.x, framePos.y + previewFrameSize.y),
        IM_COL32(0, 0, 0, 255));
    drawList->AddImage(
        (ImTextureID)(UINT64)textureID,
        imagePos,
        ImVec2(imagePos.x + previewSize.x, imagePos.y + previewSize.y));
    ImGui::Dummy(previewFrameSize);

    return true;
}

void App::BuildImGuiTextureViewer() {
    static int selectedTextureIndex = 0;
    static int selectedViewIndex = 0;

    constexpr int MaxTextureViewerEntries = 9;
    ImGuiTextureViewerEntry entries[MaxTextureViewerEntries];
    int entryCount = 0;

    if (renderCamera) {
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Color", renderCamera->GetColorTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Depth", renderCamera->GetDepthTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Normal", renderCamera->GetNormalTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Post Process Color", renderCamera->GetPostProcessColorTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Bloom Down", renderCamera->GetBloomDownTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Bloom Up", renderCamera->GetBloomUpTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Bloom Convolution Output", renderCamera->GetFFTBloomConvolutionOutputTexture());
    }

    if (renderSystem) {
        const ConvolutionBloomKernelGenerator &kernelGenerator = renderSystem->GetConvolutionBloomKernelGenerator();
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Bloom Diffraction PSF", kernelGenerator.GetDiffractionPSFTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Generated Bloom Kernel", kernelGenerator.GetKernelTexture());
    }

    if (entryCount == 0) {
        ImGui::TextUnformatted("No texture is available.");
        return;
    }

    selectedTextureIndex = BE1::Min(BE1::Max(selectedTextureIndex, 0), entryCount - 1);

    const float fontSize = ImGui::GetFontSize();
    ImGui::SetNextItemWidth(fontSize * 18.0f);
    if (ImGui::BeginCombo("Texture", entries[selectedTextureIndex].label)) {
        for (int entryIndex = 0; entryIndex < entryCount; ++entryIndex) {
            const bool selected = selectedTextureIndex == entryIndex;
            if (ImGui::Selectable(entries[entryIndex].label, selected)) {
                selectedTextureIndex = entryIndex;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    const ImGuiTextureViewerEntry &entry = entries[selectedTextureIndex];
    const RHI::Texture *texture = entry.texture;
    const uint32_t viewCount = texture->GetViewCount(RHI::ViewType::SRV);
    selectedViewIndex = BE1::Min(BE1::Max(selectedViewIndex, 0), static_cast<int>(viewCount) - 1);

    if (viewCount > 1) {
        ImGui::SetNextItemWidth(fontSize * 12.0f);
        char viewSliderLabel[32];
        BE1::Str::snPrintf(viewSliderLabel, static_cast<int>(sizeof(viewSliderLabel)), "View (0..%u)", viewCount - 1);
        ImGui::SliderInt(viewSliderLabel, &selectedViewIndex, 0, static_cast<int>(viewCount) - 1);
    }

    const RHI::ViewID viewID = texture->GetViewID(RHI::ViewType::SRV, selectedViewIndex);
    RHI::TextureViewInfo viewInfo;
    const bool hasViewInfo = texture->GetViewInfo(RHI::ViewType::SRV, viewID, viewInfo);

    ImGui::Separator();
    ImGui::Text("Type: %s, Format: %s", RHI::Renderer::TextureTypeName(texture->type), RHI::Renderer::FormatName(texture->GetFormat()));
    ImGui::Text("Size: %u x %u x %u", texture->GetWidth(), texture->GetHeight(), texture->GetDepth());
    ImGui::Text("Array: %u, Mips: %u, Samples: %u", texture->GetArraySize(), texture->GetMipLevelCount(), texture->GetSampleCount());

    char flagsText[128];
    TextureFlagsToText(texture->flags, flagsText, sizeof(flagsText));
    ImGui::TextUnformatted("Flags:");
    ImGui::SameLine();
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(flagsText);
    ImGui::PopTextWrapPos();

    if (hasViewInfo) {
        ImGui::Text("View ID: %u", viewInfo.viewID.value);
        if (viewInfo.format != texture->GetFormat()) {
            ImGui::Text("View Format: %s", RHI::Renderer::FormatName(viewInfo.format));
        }
        if (viewInfo.sliceCount > 1) {
            ImGui::Text("View Slice Range: %u..%u (%u)", viewInfo.firstSlice, viewInfo.firstSlice + viewInfo.sliceCount - 1, viewInfo.sliceCount);
        } else {
            ImGui::Text("View Slice: %u", viewInfo.firstSlice);
        }
        if (viewInfo.mipCount > 1) {
            ImGui::Text("View Mip Range: %u..%u (%u)", viewInfo.firstMipLevel, viewInfo.firstMipLevel + viewInfo.mipCount - 1, viewInfo.mipCount);
        } else {
            ImGui::Text("View Mip: %u", viewInfo.firstMipLevel);
        }
        ImGui::Text("View Size: %u x %u x %u", viewInfo.width, viewInfo.height, viewInfo.depth);
    } else {
        ImGui::Text("View ID: %u", viewID.value);
    }

    if (!DrawImGuiTexturePreview(texture, viewID, fontSize * 24.0f, true)) {
        ImGui::TextUnformatted("Preview is unavailable.");
    }
}

void App::BuildImGuiOverlay() {
    UpdateImGuiStatsPlotHistory();

    const float fontSize = ImGui::GetFontSize();

    //ImGui::StyleColorsClassic();
    //ImGui::StyleColorsLight();
    ImGui::StyleColorsDark();

    const auto roundedCollapsingHeader = [fontSize](const char *label) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, fontSize * 0.3f);
        const bool isOpen = ImGui::CollapsingHeader(label);
        ImGui::PopStyleVar();
        return isOpen;
    };

    ImGui::SetNextWindowSizeConstraints(ImVec2(fontSize * 29.0f, fontSize * 8.0f), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("TestD3D12", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::BeginTabBar("TestD3D12Tabs")) {
        if (ImGui::BeginTabItem("System")) {
            ImGui::Text("CPU Cores: %i physical / %i logical",
                BE1::PlatformSystem::NumCPUCores(),
                BE1::PlatformSystem::NumCPUCoresIncludingHyperthreads());
            ImGui::Text("Render Thread: %s",
                renderSystem && renderSystem->IsUsingRenderThread() ? "Enabled" : "Disabled");
            ImGui::Text("Task Threads: %u",
                BE1::Engine::taskManager ? BE1::Engine::taskManager->NumThreads() : 0);
            ImGui::Separator();

            if (RHI::renderer) {
                const RHI::RendererInfo &rendererInfo = RHI::renderer->GetRendererInfo();

                ImGui::Text("GPU Adapter: %s", rendererInfo.name);
                ImGui::Text("Vendor ID: 0x%04X", rendererInfo.vendorId);
                ImGui::Text("Device ID: 0x%04X", rendererInfo.deviceId);
                ImGui::Separator();
                ImGui::Text("Dedicated VRAM: %s", BE1::Str::FormatBytes(rendererInfo.dedicatedVideoMemoryBytes).c_str());
                ImGui::Text("Dedicated System Memory: %s", BE1::Str::FormatBytes(rendererInfo.dedicatedSystemMemoryBytes).c_str());
                ImGui::Text("Shared System Memory: %s", BE1::Str::FormatBytes(rendererInfo.sharedSystemMemoryBytes).c_str());
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Render")) {
            bool showFps = app_showFps.GetBool();
            if (ImGui::Checkbox("Show FPS", &showFps)) {
                app_showFps.SetBool(showFps);
            }

            bool showRenderStats = app_showStats.GetBool();
            if (ImGui::Checkbox("Show statistics", &showRenderStats)) {
                app_showStats.SetBool(showRenderStats);
            }

            if (ImGui::Button(imguiShowStatsPlotWindow ? "Hide Stats Plot" : "Show Stats Plot")) {
                imguiShowStatsPlotWindow = !imguiShowStatsPlotWindow;
            }

            bool depthPrepass = r_depthPrepass.GetBool();
            if (ImGui::Checkbox("Depth prepass", &depthPrepass)) {
                r_depthPrepass.SetBool(depthPrepass);
            }

            if (roundedCollapsingHeader("Skip options##RenderSkipOptionsHeader")) {
                bool skipCulling = r_skipFrustumCulling.GetBool();
                if (ImGui::Checkbox("Skip frustum culling", &skipCulling)) {
                    r_skipFrustumCulling.SetBool(skipCulling);
                }

                bool skipDrawCamera2D = r_skipDrawCamera2D.GetBool();
                if (ImGui::Checkbox("Skip draw camera 2D", &skipDrawCamera2D)) {
                    r_skipDrawCamera2D.SetBool(skipDrawCamera2D);
                }

                bool skipDrawCamera3D = r_skipDrawCamera3D.GetBool();
                if (ImGui::Checkbox("Skip draw camera 3D", &skipDrawCamera3D)) {
                    r_skipDrawCamera3D.SetBool(skipDrawCamera3D);
                }

                bool skipPostProcess = r_skipPostProcess.GetBool();
                if (ImGui::Checkbox("Skip post process", &skipPostProcess)) {
                    r_skipPostProcess.SetBool(skipPostProcess);
                }
            }

            if (roundedCollapsingHeader("Bloom##BloomHeader")) {
                int bloomMode = BE1::Clamp(r_bloom.GetInteger(), static_cast<int>(RenderBloomMode::Off), static_cast<int>(RenderBloomMode::Convolution));
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::Combo("Bloom", &bloomMode,
                    "Off\0"
                    "Pyramid\0"
                    "Convolution\0")) {
                    r_bloom.SetInteger(bloomMode);
                }
                const bool useBloom = bloomMode != static_cast<int>(RenderBloomMode::Off);
                const bool usePyramidBloom = bloomMode == static_cast<int>(RenderBloomMode::Pyramid);
                const bool useConvolutionBloom = bloomMode == static_cast<int>(RenderBloomMode::Convolution);

                if (useBloom) {
                    float bloomThreshold = r_bloomThreshold.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom threshold", &bloomThreshold, 0.0f, 10.0f, "%.2f")) {
                        r_bloomThreshold.SetFloat(bloomThreshold);
                    }

                    float bloomSoftThreshold = r_bloomSoftThreshold.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom soft threshold", &bloomSoftThreshold, 0.0f, 5.0f, "%.2f")) {
                        r_bloomSoftThreshold.SetFloat(bloomSoftThreshold);
                    }

                    float bloomIntensity = r_bloomIntensity.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom intensity", &bloomIntensity, 0.0f, 2.0f, "%.3f")) {
                        r_bloomIntensity.SetFloat(bloomIntensity);
                    }
                }

                if (usePyramidBloom) {
                    int pyramidBloomPrefilterQuality = BE1::Clamp(r_pyramidBloomPrefilterQuality.GetInteger(), 0, 1);
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::Combo("Bloom prefilter quality", &pyramidBloomPrefilterQuality,
                        "Rotated Grid 5 Tap\0"
                        "Jimenez 13 Tap\0")) {
                        r_pyramidBloomPrefilterQuality.SetInteger(pyramidBloomPrefilterQuality);
                    }

                    float pyramidBloomScatter = r_pyramidBloomScatter.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom scatter", &pyramidBloomScatter, 0.0f, 1.0f, "%.3f")) {
                        r_pyramidBloomScatter.SetFloat(pyramidBloomScatter);
                    }

                    float pyramidBloomPrefilterRadius = r_pyramidBloomPrefilterRadius.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom prefilter radius", &pyramidBloomPrefilterRadius, 0.0f, 5.0f, "%.2f")) {
                        r_pyramidBloomPrefilterRadius.SetFloat(pyramidBloomPrefilterRadius);
                    }

                    float pyramidBloomDownsampleRadius = r_pyramidBloomDownsampleRadius.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom downsample radius", &pyramidBloomDownsampleRadius, 0.0f, 5.0f, "%.2f")) {
                        r_pyramidBloomDownsampleRadius.SetFloat(pyramidBloomDownsampleRadius);
                    }

                    float pyramidBloomUpsampleRadius = r_pyramidBloomUpsampleRadius.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom upsample radius", &pyramidBloomUpsampleRadius, 0.0f, 5.0f, "%.2f")) {
                        r_pyramidBloomUpsampleRadius.SetFloat(pyramidBloomUpsampleRadius);
                    }
                }

                if (useConvolutionBloom) {
                    static constexpr int convolutionBloomFFTMaxSizeOptions[] = { 256, 512, 1024 };
                    int convolutionBloomFFTMaxSize = r_convolutionBloomFFTMaxSize.GetInteger();
                    int convolutionBloomFFTMaxSizeIndex = convolutionBloomFFTMaxSize <= 256 ? 0 : (convolutionBloomFFTMaxSize <= 512 ? 1 : 2);
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::Combo("Bloom FFT max size", &convolutionBloomFFTMaxSizeIndex,
                        "256\0"
                        "512\0"
                        "1024\0")) {
                        r_convolutionBloomFFTMaxSize.SetInteger(convolutionBloomFFTMaxSizeOptions[convolutionBloomFFTMaxSizeIndex]);
                    }

                    int convolutionBloomKernelSource = app_convolutionBloomKernelSource.GetInteger();
                    const char *currentKernelSourceName = convolutionBloomKernelSource == static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture) ? "From Aperture" : "Preset";
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::BeginCombo("Bloom kernel source", currentKernelSourceName)) {
                        const bool presetSelected = convolutionBloomKernelSource == static_cast<int>(App::ConvolutionBloomKernelSource::Preset);
                        if (ImGui::Selectable("Preset", presetSelected)) {
                            if (!presetSelected) {
                                app_convolutionBloomKernelSource.SetInteger(static_cast<int>(App::ConvolutionBloomKernelSource::Preset));
                                convolutionBloomKernelSource = static_cast<int>(App::ConvolutionBloomKernelSource::Preset);
                            }
                        }
                        if (presetSelected) {
                            ImGui::SetItemDefaultFocus();
                        }

                        if (ImGui::Selectable("From Aperture", convolutionBloomKernelSource == static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture))) {
                            if (convolutionBloomKernelSource != static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture)) {
                                app_convolutionBloomKernelSource.SetInteger(static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture));
                                convolutionBloomKernelSource = static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture);
                            }
                        }
                        if (convolutionBloomKernelSource == static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture)) {
                            ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    if (convolutionBloomKernelSource == static_cast<int>(App::ConvolutionBloomKernelSource::Preset)) {
                        char currentKernelName[32];
                        GetConvolutionBloomKernelPresetName(app_convolutionBloomKernelPreset.GetInteger(), currentKernelName, sizeof(currentKernelName));
                        ImGui::SetNextItemWidth(fontSize * 12.0f);
                        if (ImGui::BeginCombo("Bloom kernel preset", currentKernelName)) {
                            for (int kernelPreset = 0; kernelPreset < App::ConvolutionBloomKernelPresetCount; ++kernelPreset) {
                                char kernelName[32];
                                GetConvolutionBloomKernelPresetName(kernelPreset, kernelName, sizeof(kernelName));

                                const bool isSelected = kernelPreset == app_convolutionBloomKernelPreset.GetInteger();
                                if (ImGui::Selectable(kernelName, isSelected)) {
                                    if (kernelPreset != app_convolutionBloomKernelPreset.GetInteger()) {
                                        app_convolutionBloomKernelPreset.SetInteger(kernelPreset);
                                    }
                                }
                                if (isSelected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }
                    } else if (convolutionBloomKernelSource == static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture)) {
                        int convolutionBloomAperturePreset = app_convolutionBloomAperturePreset.GetInteger();
                        ImGui::SetNextItemWidth(fontSize * 12.0f);
                        if (ImGui::Combo("Bloom aperture preset", &convolutionBloomAperturePreset,
                            "Triangle\0"
                            "Square\0"
                            "Horz Rectangle\0"
                            "Vert Rectangle\0"
                            "Pentagon\0"
                            "Hexagon\0"
                            "Heptagon\0"
                            "Octagon\0"
                            "Circle\0")) {
                            app_convolutionBloomAperturePreset.SetInteger(convolutionBloomAperturePreset);
                        }

                        if (convolutionBloomApertureTexture && convolutionBloomApertureTexture->IsReadyForRendering()) {
                            const RHI::Texture *apertureTexture = convolutionBloomApertureTexture->GetRHITexture();
                            if (apertureTexture) {
                                ImGui::Spacing();
                                DrawImGuiTexturePreview(apertureTexture, RHI::ViewID::PrimaryID, fontSize * 8.0f, false);
                            }
                        }
                    }

                    float convolutionBloomKernelRotation = r_convolutionBloomKernelRotation.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom kernel rotation", &convolutionBloomKernelRotation, -180.0f, 180.0f, "%.1f deg")) {
                        r_convolutionBloomKernelRotation.SetFloat(convolutionBloomKernelRotation);
                    }

                    float convolutionBloomKernelSizeScale = r_convolutionBloomKernelSizeScale.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom kernel size scale", &convolutionBloomKernelSizeScale, 0.01f, 10.0f, "%.3f")) {
                        r_convolutionBloomKernelSizeScale.SetFloat(BE1::Max(convolutionBloomKernelSizeScale, 0.01f));
                    }

                    float convolutionBloomKernelScale = r_convolutionBloomKernelScale.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom kernel scale", &convolutionBloomKernelScale, 0.0f, 10.0f, "%.4f")) {
                        r_convolutionBloomKernelScale.SetFloat(BE1::Max(convolutionBloomKernelScale, 0.0f));
                    }

                    float convolutionBloomPaddingTrim = r_convolutionBloomPaddingTrim.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom padding trim", &convolutionBloomPaddingTrim, 0.0f, 0.25f, "%.3f")) {
                        r_convolutionBloomPaddingTrim.SetFloat(BE1::Min(BE1::Max(convolutionBloomPaddingTrim, 0.0f), 0.25f));
                    }

                    if (convolutionBloomKernelSource == static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture)) {
                        float convolutionBloomDiffractionPSFScale = r_convolutionBloomDiffractionPSFScale.GetFloat();
                        ImGui::SetNextItemWidth(fontSize * 12.0f);
                        if (ImGui::SliderFloat("Bloom diffraction PSF scale", &convolutionBloomDiffractionPSFScale, 0.0f, 100.0f, "%.3f")) {
                            r_convolutionBloomDiffractionPSFScale.SetFloat(BE1::Max(convolutionBloomDiffractionPSFScale, 0.0f));
                        }

                        float convolutionBloomDispersionAmount = r_convolutionBloomDispersionAmount.GetFloat();
                        ImGui::SetNextItemWidth(fontSize * 12.0f);
                        if (ImGui::SliderFloat("Bloom dispersion amount", &convolutionBloomDispersionAmount, 0.0f, 10.0f, "%.3f")) {
                            r_convolutionBloomDispersionAmount.SetFloat(BE1::Max(convolutionBloomDispersionAmount, 0.0f));
                        }

                        if (ImGui::Button("Generate Bloom kernel")) {
                            RequestConvolutionBloomKernelGeneration();
                        }
                    }
                }
            }

            if (roundedCollapsingHeader("Tone mapping##ToneMappingHeader")) {
                bool toneMapping = r_toneMapping.GetBool();
                if (ImGui::Checkbox("Tone mapping", &toneMapping)) {
                    r_toneMapping.SetBool(toneMapping);
                }

                int toneMapOp = BE1::Clamp(r_toneMapOp.GetInteger(), static_cast<int>(ToneMappingOperatorLinear), static_cast<int>(ToneMappingOperatorFilmicGT));
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::Combo("Tone map operator", &toneMapOp,
                    "Linear\0"
                    "Exponential\0"
                    "Logarithmic\0"
                    "Drago Logarithmic\0"
                    "Reinhard\0"
                    "Reinhard Extended\0"
                    "Filmic ALU\0"
                    "Filmic ACES\0"
                    "Filmic Unreal3\0"
                    "Filmic Uncharted 2\0"
                    "Filmic GT\0\0")) {
                    r_toneMapOp.SetInteger(toneMapOp);
                }

                float luminanceLogMin = r_toneMapLuminanceLogMin.GetFloat();
                float luminanceLogMax = r_toneMapLuminanceLogMax.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::DragFloatRange2("Luminance log2 range", &luminanceLogMin, &luminanceLogMax, 0.05f, -16.0f, 16.0f, "Min: %.2f", "Max: %.2f")) {
                    r_toneMapLuminanceLogMin.SetFloat(luminanceLogMin);
                    r_toneMapLuminanceLogMax.SetFloat(BE1::Max(luminanceLogMax, luminanceLogMin + 0.0001f));
                }

                float luminanceAdaptationRate = r_toneMapLuminanceAdaptationRate.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("Luminance adaptation rate", &luminanceAdaptationRate, 0.0f, 10.0f, "%.2f")) {
                    r_toneMapLuminanceAdaptationRate.SetFloat(luminanceAdaptationRate);
                }

                float middleGray = r_toneMapMiddleGray.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("Middle gray", &middleGray, 0.0f, 2.0f, "%.3f")) {
                    r_toneMapMiddleGray.SetFloat(middleGray);
                }
            }

            if (roundedCollapsingHeader("Texture Streaming##TextureStreamingHeader")) {
                int streamInDelay = r_textureStreamingStreamInDelay.GetInteger();
                ImGui::SetNextItemWidth(fontSize * 6.0f);
                if (ImGui::InputInt("Texture stream-in delay", &streamInDelay)) {
                    r_textureStreamingStreamInDelay.SetInteger(BE1::Max(0, streamInDelay));
                }

                int maxLevelReduction = r_textureStreamingMaxLevelReduction.GetInteger();
                ImGui::SetNextItemWidth(fontSize * 6.0f);
                if (ImGui::SliderInt("Texture max level reduction", &maxLevelReduction,
                        static_cast<int>(r_textureStreamingMaxLevelReduction.GetMinValue()),
                        static_cast<int>(r_textureStreamingMaxLevelReduction.GetMaxValue()))) {
                    r_textureStreamingMaxLevelReduction.SetInteger(maxLevelReduction);
                }
            }

            if (roundedCollapsingHeader("Texture Viewer##TextureViewerHeader")) {
                BuildImGuiTextureViewer();
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Scene")) {
            ImGui::SetNextItemWidth(fontSize * 10.0f);
            ImGui::SliderInt("Count X", &imguiGoCountX, 1, 250);
            const bool applyCountX = ImGui::IsItemDeactivatedAfterEdit();

            ImGui::SetNextItemWidth(fontSize * 10.0f);
            ImGui::SliderInt("Count Y", &imguiGoCountY, 1, 250);
            const bool applyCountY = ImGui::IsItemDeactivatedAfterEdit();

            if ((applyCountX || applyCountY) && (static_cast<uint32_t>(imguiGoCountX) != goCountX || static_cast<uint32_t>(imguiGoCountY) != goCountY || goCountZ != 1)) {
                ChangeGameObjectCount(static_cast<uint32_t>(imguiGoCountX), static_cast<uint32_t>(imguiGoCountY), 1);
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Debug")) {
            ImGui::Text("Mouse capture: %s", ImGui::GetIO().WantCaptureMouse ? "true" : "false");
            ImGui::Text("Keyboard capture: %s", ImGui::GetIO().WantCaptureKeyboard ? "true" : "false");
            ImGui::Separator();

            if (ImGui::Button(imguiShowDemoWindow ? "Hide ImGui Demo" : "Show ImGui Demo")) {
                imguiShowDemoWindow = !imguiShowDemoWindow;
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    BuildImGuiStatsPlotWindow();

    if (imguiShowDemoWindow) {
        ImGui::ShowDemoWindow(&imguiShowDemoWindow);
    }
}
#endif
