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
#include "Render/Backend/RenderBackend.h"
#include "Render/Frontend/RenderCamera.h"
#include "Render/Texture/Texture.h"
#include "Scene.h"
#include "SceneRegistry.h"
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
static const ImVec4    ImGuiWarningTextColor(1.0f, 1.0f, 0.0f, 1.0f);

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

    const float contentWidth = ImGui::GetContentRegionAvail().x;
    const float timingInfoWidth = fontSize * 11.0f;
    const float plotSpacing = ImGui::GetStyle().ItemSpacing.x;
    const bool showTimingInfoRight = contentWidth >= fontSize * 16.0f;
    const float plotWidth = showTimingInfoRight ? BE1::Max(contentWidth - timingInfoWidth - plotSpacing, fontSize * 6.0f) : contentWidth;
    const ImVec2 plotSize(plotWidth, fontSize * 12.0f);
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

    if (showTimingInfoRight) {
        ImGui::SameLine();
    }
    ImGui::BeginGroup();
    ImGui::Text("Scale");
    ImGui::Text("0.00 - %.2f ms", maxMsec);
    ImGui::Spacing();
    for (int seriesIndex = 0; seriesIndex < ImGuiStatsPlotSeriesCount; ++seriesIndex) {
        ImGui::ColorButton(statNames[seriesIndex], statColors[seriesIndex], ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(fontSize, fontSize));
        ImGui::SameLine();
        ImGui::Text("%s %.2f ms", statNames[seriesIndex], imguiStatsPlotSamples[seriesIndex][latestSampleIndex]);
    }
    ImGui::EndGroup();

    if (mainRenderContext) {
        ImGui::Separator();
        ImGui::Text("GPU Timing Breakdown");

        struct GpuTimingTagRow {
            int                 tagIndex;
            GpuTiming::Tag      tag;
            float               msec;
        };

        GpuTimingTagRow rows[static_cast<int>(GpuTiming::Tag::Count)];
        int rowCount = 0;
        for (int tagIndex = 0; tagIndex < static_cast<int>(GpuTiming::Tag::Count); ++tagIndex) {
            const GpuTiming::Tag tag = static_cast<GpuTiming::Tag>(tagIndex);
            const float tagMsec = mainRenderContext->GetStatGpuTagAverageMsec(tag);
            if (tagMsec <= 0.001f) {
                continue;
            }

            GpuTimingTagRow &row = rows[rowCount++];
            row.tagIndex = tagIndex;
            row.tag = tag;
            row.msec = tagMsec;
        }

        if (rowCount > 0) {
            const ImGuiTableFlags tableFlags =
                ImGuiTableFlags_Sortable |
                ImGuiTableFlags_SortTristate |
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingStretchProp;

            if (ImGui::BeginTable("GpuTimingBreakdownTable", 3, tableFlags)) {
                ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortAscending, fontSize * 4.0f);
                ImGui::TableSetupColumn("Tag", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortAscending);
                ImGui::TableSetupColumn("Avg ms", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, fontSize * 7.0f);
                ImGui::TableHeadersRow();

                ImGuiTableSortSpecs *sortSpecs = ImGui::TableGetSortSpecs();
                if (sortSpecs && sortSpecs->SpecsCount > 0) {
                    const ImGuiTableColumnSortSpecs &sortSpec = sortSpecs->Specs[0];

                    auto compareRows = [](const GpuTimingTagRow &lhs, const GpuTimingTagRow &rhs, const ImGuiTableColumnSortSpecs &sortSpec) {
                        int result = 0;
                        switch (sortSpec.ColumnIndex) {
                        case 0:
                            result = lhs.tagIndex - rhs.tagIndex;
                            break;
                        case 1:
                            result = strcmp(GpuTiming::TagName(lhs.tag), GpuTiming::TagName(rhs.tag));
                            break;
                        case 2:
                            result = lhs.msec < rhs.msec ? -1 : (lhs.msec > rhs.msec ? 1 : 0);
                            break;
                        default:
                            break;
                        }
                        if (result == 0) {
                            result = lhs.tagIndex - rhs.tagIndex;
                        }
                        return sortSpec.SortDirection == ImGuiSortDirection_Descending ? result > 0 : result < 0;
                    };

                    for (int rowIndex = 1; rowIndex < rowCount; ++rowIndex) {
                        const GpuTimingTagRow row = rows[rowIndex];
                        int insertIndex = rowIndex;
                        while (insertIndex > 0 && compareRows(row, rows[insertIndex - 1], sortSpec)) {
                            rows[insertIndex] = rows[insertIndex - 1];
                            --insertIndex;
                        }
                        rows[insertIndex] = row;
                    }

                    sortSpecs->SpecsDirty = false;
                }

                for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
                    const GpuTimingTagRow &row = rows[rowIndex];
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", row.tagIndex);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(GpuTiming::TagName(row.tag));
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.3f", row.msec);
                }

                ImGui::EndTable();
            }
        } else {
            ImGui::Text("No GPU tag samples yet");
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
        IM_COL32(0, 0, 0, 0));
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

    constexpr int MaxTextureViewerEntries = 16;
    ImGuiTextureViewerEntry entries[MaxTextureViewerEntries];
    int entryCount = 0;

    if (renderCamera) {
        const RHI::Texture *depthTexture = renderCamera->GetDepthTexture();

        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Color", renderCamera->GetRTColorTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Depth", depthTexture);
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Velocity", renderCamera->GetRTVelocityTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Normal", renderCamera->GetRTNormalTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Post Process Color", renderCamera->GetPostProcessColorTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Bloom Down", renderCamera->GetBloomDownTexture());
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Bloom Up", renderCamera->GetBloomUpTexture());

        const SharedPtr<Texture> &bloomDirtMaskTexture = renderCamera->GetBloomDirtMaskTexture();
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Bloom Dirt Mask", bloomDirtMaskTexture ? bloomDirtMaskTexture->GetReadyRHITexture() : nullptr);

        const SharedPtr<Texture> &fftBloomKernelTexture = renderCamera->GetFFTBloomKernelTexture();
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "FFT Bloom Kernel", fftBloomKernelTexture ? fftBloomKernelTexture->GetReadyRHITexture() : nullptr);
        AddImGuiTextureViewerEntry(entries, MaxTextureViewerEntries, entryCount, "Bloom Convolution Output", renderCamera->GetFFTBloomConvolutionOutputTexture());
    }

    if (entryCount == 0) {
        ImGui::TextColored(ImGuiWarningTextColor, "No texture is available.");
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
        ImGui::TextColored(ImGuiWarningTextColor, "Preview is unavailable.");
    }
}

void App::BuildImGuiOverlay() {
    UpdateImGuiStatsPlotHistory();

    const float fontSize = ImGui::GetFontSize();

    //ImGui::StyleColorsClassic();
    //ImGui::StyleColorsLight();
    ImGui::StyleColorsDark();

    const auto roundedCollapsingHeader = [fontSize](const char *label) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, fontSize * 0.25f);
        const bool isOpen = ImGui::CollapsingHeader(label);
        ImGui::PopStyleVar();
        return isOpen;
    };
    bool hiZDebugPreviewOpen = false;

    ImGui::SetNextWindowSizeConstraints(ImVec2(fontSize * 29.0f, fontSize * 8.0f), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::Begin("Blueshift Renderer Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::BeginTabBar("Tabs")) {
        if (ImGui::BeginTabItem("Scene")) {
            const float itemWidth = fontSize * 17.0f;
            int selectedSceneIndex = pendingSceneIndex != -1 ? pendingSceneIndex : sceneIndex;
            ImGui::SetNextItemWidth(itemWidth);
            if (ImGui::BeginCombo("Scene", SceneRegistry::GetName(selectedSceneIndex))) {
                for (int index = 0; index < SceneRegistry::GetCount(); ++index) {
                    const bool selected = index == selectedSceneIndex;
                    if (ImGui::Selectable(SceneRegistry::GetName(index), selected)) {
                        RequestSceneChange(index);
                        selectedSceneIndex = index;
                    }

                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            ImGui::TextUnformatted("Camera Mode");
            if (ImGui::RadioButton("Orbit", cameraControlMode == CameraControlMode::Orbit)) {
                SetCameraControlMode(CameraControlMode::Orbit);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Fly", cameraControlMode == CameraControlMode::Fly)) {
                SetCameraControlMode(CameraControlMode::Fly);
            }

            if (cameraControlMode == CameraControlMode::Fly) {
                ImGui::SetNextItemWidth(itemWidth);
                if (ImGui::DragFloat("Fly Speed", &cameraFlySpeed, 1.0f, 1.0f, 1000.0f, "%.1f")) {
                    cameraFlySpeed = BE1::Max(cameraFlySpeed, 1.0f);
                }
            }

            ImGui::Separator();

            if (pendingSceneIndex == -1) {
                scene->BuildImGui();
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

            bool showLights = r_showLights2.GetBool();
            if (ImGui::Checkbox("Show lights", &showLights)) {
                r_showLights2.SetBool(showLights);
            }

            if (ImGui::Button(imguiShowStatsPlotWindow ? "Hide Stats Plot" : "Show Stats Plot")) {
                imguiShowStatsPlotWindow = !imguiShowStatsPlotWindow;
            }

            bool vsync = r_vsync.GetBool();
            if (ImGui::Checkbox("VSync", &vsync)) {
                r_vsync.SetBool(vsync);
            }

            bool frameComposition = r_frameComposition.GetBool();
            if (ImGui::Checkbox("Frame composition", &frameComposition)) {
                r_frameComposition.SetBool(frameComposition);
            }

            bool depthPrepass = r_depthPrepass.GetBool();
            if (ImGui::Checkbox("Depth prepass", &depthPrepass)) {
                r_depthPrepass.SetBool(depthPrepass);
            }

            bool specularEnergyCompensation = r_specularEnergyCompensation2.GetBool();
            if (ImGui::Checkbox("Specular energy compensation", &specularEnergyCompensation)) {
                r_specularEnergyCompensation2.SetBool(specularEnergyCompensation);
            }

            bool specularAA = r_specularAA.GetBool();
            if (ImGui::Checkbox("Specular AA", &specularAA)) {
                r_specularAA.SetBool(specularAA);
            }

            static constexpr int msaaSampleCounts[] = { 1, 2, 4, 8 };
            int msaaSampleCount = r_msaa.GetInteger();
            int msaaSampleCountIndex = msaaSampleCount >= 8 ? 3 : (msaaSampleCount >= 4 ? 2 : (msaaSampleCount >= 2 ? 1 : 0));
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::Combo("MSAA", &msaaSampleCountIndex,
                "Off\0"
                "2x\0"
                "4x\0"
                "8x\0")) {
                r_msaa.SetInteger(msaaSampleCounts[msaaSampleCountIndex]);
            }

            bool lightCullingZBin = r_lightCullingZBin.GetBool();
            if (ImGui::Checkbox("Light culling Z-bin", &lightCullingZBin)) {
                r_lightCullingZBin.SetBool(lightCullingZBin);
            }

            int lightCullingDebugMode = BE1::Clamp(r_lightCullingDebug.GetInteger(),
                static_cast<int>(r_lightCullingDebug.GetMinValue()), static_cast<int>(r_lightCullingDebug.GetMaxValue()));
            ImGui::SetNextItemWidth(fontSize * 12.0f);
            if (ImGui::Combo("Light culling debug", &lightCullingDebugMode,
                "Off\0"
                "Tile mask\0"
                "Tile mask + Z-bin\0")) {
                r_lightCullingDebug.SetInteger(lightCullingDebugMode);
            }

            if (roundedCollapsingHeader("Skip options##RenderSkipOptionsHeader")) {
                bool skipCulling = r_skipViewCulling.GetBool();
                if (ImGui::Checkbox("Skip view culling", &skipCulling)) {
                    r_skipViewCulling.SetBool(skipCulling);
                }

                bool skipDrawCamera2D = r_skipDrawCamera2D.GetBool();
                if (ImGui::Checkbox("Skip draw camera 2D", &skipDrawCamera2D)) {
                    r_skipDrawCamera2D.SetBool(skipDrawCamera2D);
                }

                bool skipDrawCamera3D = r_skipDrawCamera3D.GetBool();
                if (ImGui::Checkbox("Skip draw camera 3D", &skipDrawCamera3D)) {
                    r_skipDrawCamera3D.SetBool(skipDrawCamera3D);
                }
            }

            const bool hiZHeaderOpen = roundedCollapsingHeader("Hi-Z##HiZHeader");
            hiZDebugPreviewOpen = hiZHeaderOpen;
            if (hiZHeaderOpen) {

                bool hiZOcclusionCulling = r_hiZOcclusionCulling.GetBool();
                if (ImGui::Checkbox("Hi-Z occlusion culling", &hiZOcclusionCulling)) {
                    r_hiZOcclusionCulling.SetBool(hiZOcclusionCulling);
                }

                bool hiZOcclusionCullingFreeze = r_hiZOcclusionCullingFreeze.GetBool();
                if (ImGui::Checkbox("Freeze Hi-Z occlusion culling", &hiZOcclusionCullingFreeze)) {
                    r_hiZOcclusionCullingFreeze.SetBool(hiZOcclusionCullingFreeze);
                }

                const RHI::Texture *hiZTexture = renderCamera ? renderCamera->GetHiZTexture() : nullptr;
                const uint32_t hiZMipCount = renderCamera ? renderCamera->GetHiZMipCount() : 0;
                if (hiZTexture && hiZMipCount > 0) {
                    const int maxHiZMipLevel = static_cast<int>(hiZMipCount) - 1;
                    int selectedHiZMipLevel = BE1::Clamp(r_hiZDebugPreviewMipLevel.GetInteger(), 0, maxHiZMipLevel);

                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderInt("Mip Level", &selectedHiZMipLevel, 0, maxHiZMipLevel)) {
                        r_hiZDebugPreviewMipLevel.SetInteger(selectedHiZMipLevel);
                    } else if (selectedHiZMipLevel != r_hiZDebugPreviewMipLevel.GetInteger()) {
                        r_hiZDebugPreviewMipLevel.SetInteger(selectedHiZMipLevel);
                    }

                    ImGui::Spacing();
                    const RHI::Texture *hiZDebugPreviewTexture = renderCamera ? renderCamera->GetHiZDebugPreviewTexture() : nullptr;
                    if (!DrawImGuiTexturePreview(hiZDebugPreviewTexture, RHI::ViewID::PrimaryID, fontSize * 24.0f, true)) {
                        ImGui::TextColored(ImGuiWarningTextColor, "Hi-Z preview is unavailable.");
                    }
                } else {
                    ImGui::TextColored(ImGuiWarningTextColor, "Hi-Z texture is unavailable.");
                }
            }

            if (roundedCollapsingHeader("Temporal AA##TemporalAAHeader")) {
                bool taa = r_taa.GetBool();
                if (ImGui::Checkbox("TAA", &taa)) {
                    r_taa.SetBool(taa);
                }

                bool taaJitter = r_taaJitter.GetBool();
                if (ImGui::Checkbox("TAA jitter", &taaJitter)) {
                    r_taaJitter.SetBool(taaJitter);
                }

                int taaJitterSampleCount = BE1::Clamp(r_taaJitterSampleCount.GetInteger(), static_cast<int>(r_taaJitterSampleCount.GetMinValue()), static_cast<int>(r_taaJitterSampleCount.GetMaxValue()));
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderInt("TAA jitter samples", &taaJitterSampleCount, static_cast<int>(r_taaJitterSampleCount.GetMinValue()), static_cast<int>(r_taaJitterSampleCount.GetMaxValue()))) {
                    r_taaJitterSampleCount.SetInteger(taaJitterSampleCount);
                }

                float taaHistoryWeight = BE1::Clamp(r_taaHistoryWeight.GetFloat(), r_taaHistoryWeight.GetMinValue(), r_taaHistoryWeight.GetMaxValue());
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("TAA history weight", &taaHistoryWeight, r_taaHistoryWeight.GetMinValue(), r_taaHistoryWeight.GetMaxValue(), "%.3f")) {
                    r_taaHistoryWeight.SetFloat(taaHistoryWeight);
                }

                float taaColorBoxSigma = BE1::Clamp(r_taaColorBoxSigma.GetFloat(), r_taaColorBoxSigma.GetMinValue(), r_taaColorBoxSigma.GetMaxValue());
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("TAA color box sigma", &taaColorBoxSigma, r_taaColorBoxSigma.GetMinValue(), r_taaColorBoxSigma.GetMaxValue(), "%.3f")) {
                    r_taaColorBoxSigma.SetFloat(taaColorBoxSigma);
                }

                float taaSubpixelWeight = BE1::Clamp(r_taaSubpixelWeight.GetFloat(), r_taaSubpixelWeight.GetMinValue(), r_taaSubpixelWeight.GetMaxValue());
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("TAA subpixel weight", &taaSubpixelWeight, r_taaSubpixelWeight.GetMinValue(), r_taaSubpixelWeight.GetMaxValue(), "%.3f")) {
                    r_taaSubpixelWeight.SetFloat(taaSubpixelWeight);
                }

                float taaLuminanceWeight = BE1::Clamp(r_taaLuminanceWeight.GetFloat(), r_taaLuminanceWeight.GetMinValue(), r_taaLuminanceWeight.GetMaxValue());
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("TAA luminance weight", &taaLuminanceWeight, r_taaLuminanceWeight.GetMinValue(), r_taaLuminanceWeight.GetMaxValue(), "%.3f")) {
                    r_taaLuminanceWeight.SetFloat(taaLuminanceWeight);
                }

                float taaDepthFallbackThreshold = BE1::Clamp(r_taaDepthFallbackThreshold.GetFloat(), r_taaDepthFallbackThreshold.GetMinValue(), r_taaDepthFallbackThreshold.GetMaxValue());
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::DragFloat("TAA depth fallback threshold", &taaDepthFallbackThreshold, 0.05f, r_taaDepthFallbackThreshold.GetMinValue(), r_taaDepthFallbackThreshold.GetMaxValue(), "%.3f")) {
                    r_taaDepthFallbackThreshold.SetFloat(taaDepthFallbackThreshold);
                }
            }

            if (roundedCollapsingHeader("Texture Sampling##TextureSamplingHeader")) {
                int textureFilterIndex = r_textureFilter.GetInteger();
                const RHI::TextureFilter textureFilter = static_cast<RHI::TextureFilter>(textureFilterIndex);

                ImGui::SetNextItemWidth(fontSize * 14.0f);
                if (ImGui::BeginCombo("Texture filter", RHI::Renderer::TextureFilterName(textureFilter))) {
                    for (int index = 0; index < static_cast<int>(RHI::TextureFilter::Count); index++) {
                        const bool selected = textureFilterIndex == index;
                        if (ImGui::Selectable(RHI::Renderer::TextureFilterName(static_cast<RHI::TextureFilter>(index)), selected)) {
                            r_textureFilter.SetInteger(index);
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                int maxAnisotropy = r_textureMaxAnisotropy.GetInteger();
                ImGui::SetNextItemWidth(fontSize * 14.0f);
                if (ImGui::SliderInt("Texture max anisotropy", &maxAnisotropy,
                        static_cast<int>(r_textureMaxAnisotropy.GetMinValue()),
                        static_cast<int>(r_textureMaxAnisotropy.GetMaxValue()))) {
                    r_textureMaxAnisotropy.SetInteger(maxAnisotropy);
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

        if (ImGui::BeginTabItem("Post Process")) {
            bool postProcessing = !r_skipPostProcess.GetBool();
            if (ImGui::Checkbox("Post processing", &postProcessing)) {
                r_skipPostProcess.SetBool(!postProcessing);
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
                    if (ImGui::SliderFloat("Bloom threshold", &bloomThreshold, r_bloomThreshold.GetMinValue(), r_bloomThreshold.GetMaxValue(), "%.2f")) {
                        r_bloomThreshold.SetFloat(bloomThreshold);
                    }

                    float bloomSoftThreshold = r_bloomSoftThreshold.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom soft threshold", &bloomSoftThreshold, r_bloomSoftThreshold.GetMinValue(), r_bloomSoftThreshold.GetMaxValue(), "%.2f")) {
                        r_bloomSoftThreshold.SetFloat(bloomSoftThreshold);
                    }

                    float bloomIntensity = r_bloomIntensity.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom intensity", &bloomIntensity, r_bloomIntensity.GetMinValue(), r_bloomIntensity.GetMaxValue(), "%.3f")) {
                        r_bloomIntensity.SetFloat(bloomIntensity);
                    }

                    float bloomDirtIntensity = r_bloomDirtIntensity.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom dirt intensity", &bloomDirtIntensity,
                        r_bloomDirtIntensity.GetMinValue(), r_bloomDirtIntensity.GetMaxValue(), "%.3f")) {
                        r_bloomDirtIntensity.SetFloat(bloomDirtIntensity);
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
                    if (ImGui::SliderFloat("Bloom scatter", &pyramidBloomScatter, r_pyramidBloomScatter.GetMinValue(), r_pyramidBloomScatter.GetMaxValue(), "%.3f")) {
                        r_pyramidBloomScatter.SetFloat(pyramidBloomScatter);
                    }

                    float pyramidBloomPrefilterRadius = r_pyramidBloomPrefilterRadius.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom prefilter radius", &pyramidBloomPrefilterRadius, r_pyramidBloomPrefilterRadius.GetMinValue(), r_pyramidBloomPrefilterRadius.GetMaxValue(), "%.2f")) {
                        r_pyramidBloomPrefilterRadius.SetFloat(pyramidBloomPrefilterRadius);
                    }

                    float pyramidBloomDownsampleRadius = r_pyramidBloomDownsampleRadius.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom downsample radius", &pyramidBloomDownsampleRadius, r_pyramidBloomDownsampleRadius.GetMinValue(), r_pyramidBloomDownsampleRadius.GetMaxValue(), "%.2f")) {
                        r_pyramidBloomDownsampleRadius.SetFloat(pyramidBloomDownsampleRadius);
                    }

                    float pyramidBloomUpsampleRadius = r_pyramidBloomUpsampleRadius.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom upsample radius", &pyramidBloomUpsampleRadius, r_pyramidBloomUpsampleRadius.GetMinValue(), r_pyramidBloomUpsampleRadius.GetMaxValue(), "%.2f")) {
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

                    float convolutionBloomPrefilterRadius = r_convolutionBloomPrefilterRadius.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom prefilter radius", &convolutionBloomPrefilterRadius, r_convolutionBloomPrefilterRadius.GetMinValue(), r_convolutionBloomPrefilterRadius.GetMaxValue(), "%.2f")) {
                        r_convolutionBloomPrefilterRadius.SetFloat(convolutionBloomPrefilterRadius);
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
                    if (ImGui::SliderFloat("Bloom kernel size scale", &convolutionBloomKernelSizeScale, r_convolutionBloomKernelSizeScale.GetMinValue(), r_convolutionBloomKernelSizeScale.GetMaxValue(), "%.3f")) {
                        r_convolutionBloomKernelSizeScale.SetFloat(convolutionBloomKernelSizeScale);
                    }

                    float convolutionBloomKernelGain = r_convolutionBloomKernelGain.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom kernel gain", &convolutionBloomKernelGain, r_convolutionBloomKernelGain.GetMinValue(), r_convolutionBloomKernelGain.GetMaxValue(), "%.4f")) {
                        r_convolutionBloomKernelGain.SetFloat(convolutionBloomKernelGain);
                    }

                    float convolutionBloomPaddingTrim = r_convolutionBloomPaddingTrim.GetFloat();
                    ImGui::SetNextItemWidth(fontSize * 12.0f);
                    if (ImGui::SliderFloat("Bloom padding trim", &convolutionBloomPaddingTrim, r_convolutionBloomPaddingTrim.GetMinValue(), r_convolutionBloomPaddingTrim.GetMaxValue(), "%.3f")) {
                        r_convolutionBloomPaddingTrim.SetFloat(convolutionBloomPaddingTrim);
                    }

                    if (convolutionBloomKernelSource == static_cast<int>(App::ConvolutionBloomKernelSource::GenerateFromAperture)) {
                        bool regenerateConvolutionBloomKernel = false;

                        float convolutionBloomApertureScale = r_convolutionBloomApertureScale.GetFloat();
                        ImGui::SetNextItemWidth(fontSize * 12.0f);
                        if (ImGui::SliderFloat("Bloom aperture scale", &convolutionBloomApertureScale, r_convolutionBloomApertureScale.GetMinValue(), r_convolutionBloomApertureScale.GetMaxValue(), "%.3f")) {
                            r_convolutionBloomApertureScale.SetFloat(convolutionBloomApertureScale);
                        }
                        regenerateConvolutionBloomKernel |= ImGui::IsItemDeactivatedAfterEdit();

                        float convolutionBloomDispersionAmount = r_convolutionBloomDispersionAmount.GetFloat();
                        ImGui::SetNextItemWidth(fontSize * 12.0f);
                        if (ImGui::SliderFloat("Bloom dispersion amount", &convolutionBloomDispersionAmount, r_convolutionBloomDispersionAmount.GetMinValue(), r_convolutionBloomDispersionAmount.GetMaxValue(), "%.3f")) {
                            r_convolutionBloomDispersionAmount.SetFloat(convolutionBloomDispersionAmount);
                        }
                        regenerateConvolutionBloomKernel |= ImGui::IsItemDeactivatedAfterEdit();

                        if (regenerateConvolutionBloomKernel) {
                            RegenerateConvolutionBloomKernel();
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
                if (ImGui::SliderFloat("Luminance adaptation rate", &luminanceAdaptationRate, r_toneMapLuminanceAdaptationRate.GetMinValue(), r_toneMapLuminanceAdaptationRate.GetMaxValue(), "%.2f")) {
                    r_toneMapLuminanceAdaptationRate.SetFloat(luminanceAdaptationRate);
                }

                float middleGray = r_toneMapMiddleGray.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("Middle gray", &middleGray, r_toneMapMiddleGray.GetMinValue(), r_toneMapMiddleGray.GetMaxValue(), "%.3f")) {
                    r_toneMapMiddleGray.SetFloat(middleGray);
                }

                float whiteBalanceTemperature = r_whiteBalanceTemperature.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("White balance temperature", &whiteBalanceTemperature, r_whiteBalanceTemperature.GetMinValue(), r_whiteBalanceTemperature.GetMaxValue(), "%.0f K")) {
                    r_whiteBalanceTemperature.SetFloat(whiteBalanceTemperature);
                }
            }

            if (roundedCollapsingHeader("Color grading##ColorGradingHeader")) {
                bool colorGrading = r_colorGrading.GetBool();
                if (ImGui::Checkbox("Color grading", &colorGrading)) {
                    r_colorGrading.SetBool(colorGrading);
                }

                bool colorGradingLUTChanged = false;

                float colorGradingSaturation = r_colorGradingSaturation.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("Color grading saturation", &colorGradingSaturation,
                    r_colorGradingSaturation.GetMinValue(), r_colorGradingSaturation.GetMaxValue(), "%.3f")) {
                    r_colorGradingSaturation.SetFloat(colorGradingSaturation);
                    colorGradingLUTChanged = true;
                }

                float colorGradingContrast = r_colorGradingContrast.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("Color grading contrast", &colorGradingContrast,
                    r_colorGradingContrast.GetMinValue(), r_colorGradingContrast.GetMaxValue(), "%.3f")) {
                    r_colorGradingContrast.SetFloat(colorGradingContrast);
                    colorGradingLUTChanged = true;
                }

                float colorGradingGamma = r_colorGradingGamma.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("Color grading gamma", &colorGradingGamma,
                    r_colorGradingGamma.GetMinValue(), r_colorGradingGamma.GetMaxValue(), "%.3f")) {
                    r_colorGradingGamma.SetFloat(colorGradingGamma);
                    colorGradingLUTChanged = true;
                }

                float colorGradingGain = r_colorGradingGain.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("Color grading gain", &colorGradingGain,
                    r_colorGradingGain.GetMinValue(), r_colorGradingGain.GetMaxValue(), "%.3f")) {
                    r_colorGradingGain.SetFloat(colorGradingGain);
                    colorGradingLUTChanged = true;
                }

                float colorGradingTint[3] = {
                    r_colorGradingTintR.GetFloat(),
                    r_colorGradingTintG.GetFloat(),
                    r_colorGradingTintB.GetFloat()
                };
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::ColorEdit3("Color tint", colorGradingTint, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR)) {
                    BE1::Clamp(colorGradingTint[0], r_colorGradingTintR.GetMinValue(), r_colorGradingTintR.GetMaxValue());
                    BE1::Clamp(colorGradingTint[1], r_colorGradingTintG.GetMinValue(), r_colorGradingTintG.GetMaxValue());
                    BE1::Clamp(colorGradingTint[2], r_colorGradingTintB.GetMinValue(), r_colorGradingTintB.GetMaxValue());
                    r_colorGradingTintR.SetFloat(colorGradingTint[0]);
                    r_colorGradingTintG.SetFloat(colorGradingTint[1]);
                    r_colorGradingTintB.SetFloat(colorGradingTint[2]);
                    colorGradingLUTChanged = true;
                }

                if (colorGradingLUTChanged) {
                    renderSystem->RequestColorGradingLUTUpdate();
                }
            }

            if (roundedCollapsingHeader("Image effects##PostProcessImageEffectsHeader")) {
                float lensDistortion = r_lensDistortion.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("Lens distortion", &lensDistortion, r_lensDistortion.GetMinValue(), r_lensDistortion.GetMaxValue(), "%.3f")) {
                    r_lensDistortion.SetFloat(lensDistortion);
                }

                float chromaticAberration = r_chromaticAberration.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("Chromatic aberration", &chromaticAberration, r_chromaticAberration.GetMinValue(), r_chromaticAberration.GetMaxValue(), "%.2f px")) {
                    r_chromaticAberration.SetFloat(chromaticAberration);
                }

                float vignette = r_vignette.GetFloat();
                ImGui::SetNextItemWidth(fontSize * 12.0f);
                if (ImGui::SliderFloat("Vignette", &vignette, r_vignette.GetMinValue(), r_vignette.GetMaxValue(), "%.3f")) {
                    r_vignette.SetFloat(vignette);
                }

                bool dithering = r_dithering.GetBool();
                if (ImGui::Checkbox("Dithering", &dithering)) {
                    r_dithering.SetBool(dithering);
                }
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("System")) {
            ImGui::Text("CPU Cores: %i physical / %i logical",
                BE1::PlatformSystem::NumCPUCores(),
                BE1::PlatformSystem::NumCPUCoresIncludingHyperthreads());
            ImGui::Text("Render Thread: %s",
                renderSystem && renderSystem->IsUsingRenderThread() ? "Enabled" : "Disabled");
            ImGui::Text("Task Threads: %u",
                BE1::Engine::taskManager ? BE1::Engine::taskManager->NumThreads() : 0);
            ImGui::Text("Resolution: %u x %u",
                mainRenderContext->GetWidth(), mainRenderContext->GetHeight());
            ImGui::Separator();

            if (RHI::renderer) {
                const RHI::RendererInfo &rendererInfo = RHI::renderer->GetRendererInfo();

                ImGui::Text("RHI Backend: %s", rendererInfo.backendName);
                ImGui::Text("GPU Adapter: %s", rendererInfo.deviceName);
                ImGui::Text("Vendor ID: 0x%04X", rendererInfo.vendorId);
                ImGui::Text("Device ID: 0x%04X", rendererInfo.deviceId);
                ImGui::Separator();
                ImGui::Text("Dedicated VRAM: %s", BE1::Str::FormatBytes(rendererInfo.dedicatedVideoMemoryBytes).c_str());
                ImGui::Text("Dedicated System Memory: %s", BE1::Str::FormatBytes(rendererInfo.dedicatedSystemMemoryBytes).c_str());
                ImGui::Text("Shared System Memory: %s", BE1::Str::FormatBytes(rendererInfo.sharedSystemMemoryBytes).c_str());
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Debug")) {
            ImGui::Text("Mouse capture: %s", ImGui::GetIO().WantCaptureMouse ? "true" : "false");
            ImGui::Text("Keyboard capture: %s", ImGui::GetIO().WantCaptureKeyboard ? "true" : "false");
            ImGui::Separator();

            if (ImGui::Button("Capture scene")) {
                pendingScreenshotCapture = true;
            }

            if (ImGui::Button("Capture environment cubemap")) {
                pendingEnvShotCapture = true;
            }

            if (ImGui::Button("Capture diffuse irradiance cubemap")) {
                pendingDiffuseIrradianceEnvShotCapture = true;
            }

            if (ImGui::Button("Capture specular prefiltered cubemap")) {
                pendingSpecularPrefilteredEnvShotCapture = true;
            }

            if (ImGui::Button("Generate GGX DFG LUT")) {
                pendingDFGSumGGXGeneration = true;
            }

            if (ImGui::Button(imguiShowDemoWindow ? "Hide ImGui Demo" : "Show ImGui Demo")) {
                imguiShowDemoWindow = !imguiShowDemoWindow;
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    if (r_hiZDebugPreview.GetBool() != hiZDebugPreviewOpen) {
        r_hiZDebugPreview.SetBool(hiZDebugPreviewOpen);
    }

    BuildImGuiStatsPlotWindow();

    if (imguiShowDemoWindow) {
        ImGui::ShowDemoWindow(&imguiShowDemoWindow);
    }
}
#endif
