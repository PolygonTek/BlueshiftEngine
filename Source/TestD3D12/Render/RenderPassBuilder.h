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

#include "Containers/HashIndex.h"
#include "RHI.h"
#include "ShaderFeatureSet.h"

class Material;
class RenderFrameData;
struct ShaderPipelineDesc;
struct RenderPass;
struct VisCamera;
struct VisSurf;
enum class RenderPassType : uint8_t;

class PassPipelineResolver {
public:
    void                        Begin(const ShaderPipelineDesc &pipelineDesc, int maxEntries);
    RHI::PipelineState *        Resolve(const Material *material, const ShaderFeatureSet &featureSet);

private:
    struct Entry {
        const Material *        material = nullptr;
        uint32_t                featureFlags = 0;
        RHI::PipelineState *    pso = nullptr;
    };

    const ShaderPipelineDesc *  pipelineDesc = nullptr;
    BE1::Array<Entry>           entries;
    BE1::HashIndex              entriesHash { 256, 256 };
};

class RenderPassBuilder {
public:
    void                        BuildMain(RenderFrameData *frameData, VisCamera *visCamera);
    void                        BuildGUI(RenderFrameData *frameData, VisCamera *visCamera);

private:
    RenderPass *                AllocRenderPass(RenderFrameData *frameData, VisCamera *visCamera, RenderPassType type, ShaderFeatureSet passFeatureSet, int maxDrawSurfs);
    void                        AddPassDrawSurf(RenderFrameData *frameData, RenderPass *renderPass, const VisSurf *visSurf, uint64_t sortKey, RHI::PipelineState *basePSO, RHI::PipelineState *instancedPSO);
    void                        SortPassDrawSurfs(RenderFrameData *frameData, RenderPass *renderPass);

    PassPipelineResolver        opaquePipelineResolver;
    PassPipelineResolver        blendPipelineResolver;
    PassPipelineResolver        guiPipelineResolver;
};
