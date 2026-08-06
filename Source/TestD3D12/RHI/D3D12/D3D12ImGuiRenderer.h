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
#include "D3D12Common.h"
#include <unordered_map>

class D3D12ImGuiOverlay;
template <typename T> struct ImVector;
struct ImTextureData;

class D3D12ImGuiRenderer {
public:
    bool                            Init();
    void                            Shutdown();

    bool                            IsInitialized() const { return initialized; }

    bool                            InitOverlay(D3D12ImGuiOverlay *overlay, RHI::Format renderTargetFormat);
    void                            ShutdownOverlay(D3D12ImGuiOverlay *overlay);
    bool                            UpdateTextures(D3D12ImGuiOverlay *overlay, ImVector<ImTextureData *> *textures);

    void                            RenderDrawData(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList, RHI::ImGuiOverlay *overlay, void *userData);
    void                            FreeDrawData(RHI::ImGuiOverlay *overlay, void *userData);

private:
    bool                            CreatePipelineObjects(DXGI_FORMAT rtvFormat, bool linearOutput);
    void                            FreePipelineObjects();
    bool                            CreateRootSignature();
    bool                            CreatePipelineState(DXGI_FORMAT rtvFormat, bool linearOutput);
    ID3D12PipelineState *           GetPipelineState(DXGI_FORMAT rtvFormat, bool linearOutput) const;
    bool                            UpdateTexture(D3D12ImGuiOverlay *overlay, ImTextureData *textureData);

    ID3D12RootSignature *           rootSignature = nullptr;
    std::unordered_map<uint32_t, ID3D12PipelineState *> pipelineStateMap;
    bool                            initialized = false;
};
