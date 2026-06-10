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
#include "RenderPassBuilder.h"
#include "RenderContext.h"
#include "RenderFrameData.h"
#include "RenderInternal.h"
#include "Material.h"
#include "VertexInputLayoutFactory.h"

static RHI::PipelineState *GetReadyPSO(const SharedPtr<ShaderPipeline> &shaderPipeline) {
    if (!shaderPipeline || !shaderPipeline->IsReady()) {
        return nullptr;
    }
    return shaderPipeline->GetPSO();
}

void PassPipelineResolver::Begin(const ShaderPipelineDesc &pipelineDesc, int maxEntries) {
    this->pipelineDesc = &pipelineDesc;

    maxEntries = BE1::Max(maxEntries, 1);
    entriesHash.Clear();
    entries.Reserve(maxEntries);
    entries.SetCount(0, false);
}

RHI::PipelineState *PassPipelineResolver::Resolve(const Material *material, const ShaderFeatureSet &featureSet) {
    assert(pipelineDesc);

    // material pointer 와 featureSet 으로 hash 키를 만든다.
    uint64_t hash = reinterpret_cast<uintptr_t>(material);
    hash = hash_combine(hash, featureSet.flags);

    const int hashKey = entriesHash.GenerateHash(hash);
    for (int index = entriesHash.First(hashKey); index != -1; index = entriesHash.Next(index)) {
        const Entry &entry = entries[index];
        if (entry.material == material && entry.featureFlags == featureSet.flags) {
            return entry.pso;
        }
    }

    // hash 에 존재하지 않는다면 material->GetGraphicsPipeline 호출로 새 ShaderPipeline 을 얻는다.
    RHI::PipelineState *pso = GetReadyPSO(material->GetGraphicsPipeline(featureSet, *pipelineDesc));

    PassPipelineResolver::Entry &entry = entries.Alloc();
    entry.material = material;
    entry.featureFlags = featureSet.flags;
    entry.pso = pso;
    entriesHash.Add(hashKey, entries.Count() - 1);

    return pso;
}

RenderPass *RenderPassBuilder::AllocRenderPass(RenderFrameData *frameData, VisCamera *visCamera, RenderPassType type, ShaderFeatureSet passFeatureSet, int maxDrawSurfs) {
    assert(visCamera->numRenderPasses < VisCamera::MaxRenderPasses);

    RenderPass *renderPass = &visCamera->renderPasses[visCamera->numRenderPasses++];
    renderPass->type = type;
    renderPass->pipelineDesc = {};
    renderPass->passFeatureSet = passFeatureSet;
    renderPass->maxDrawSurfs = maxDrawSurfs;
    renderPass->numDrawSurfs = 0;
    renderPass->drawSurfs = (const PassDrawSurf **)frameData->MemAlloc(maxDrawSurfs * sizeof(PassDrawSurf *));

    return renderPass;
}

void RenderPassBuilder::AddPassDrawSurf(RenderFrameData *frameData, RenderPass *renderPass, const VisSurf *visSurf, uint64_t sortKey, RHI::PipelineState *basePSO, RHI::PipelineState *instancedPSO) {
    assert(renderPass->numDrawSurfs < renderPass->maxDrawSurfs);

    PassDrawSurf *passDrawSurf = (PassDrawSurf *)frameData->MemAlloc(sizeof(PassDrawSurf));
    passDrawSurf->visSurf = visSurf;
    passDrawSurf->sortKey = sortKey;
    passDrawSurf->basePSO = basePSO;
    passDrawSurf->instancedPSO = instancedPSO;

    renderPass->drawSurfs[renderPass->numDrawSurfs++] = passDrawSurf;
}

static uint64_t BuildMainOpaqueSortKey(const VisSurf *visSurf) {
    //---------------------------------------------------
    // 0xFFF0000000000000 (0~4095)  : visLight index
    // 0x000F000000000000 (0~15)    : material sort
    // 0x0000FFFF00000000 (0~65535) : subMesh index
    // 0x00000000FFFF0000 (0~65535) : shader material index
    // 0x000000000000FFFF (0~65535) : visObject index
    //---------------------------------------------------

    uint64_t shaderMaterialIndex = visSurf->shaderMaterialIndex & 0xFFFF;
    uint64_t subMeshIndex = visSurf->subMesh ? (visSurf->subMesh->GetID() & 0xFFFF) : 0;
    uint64_t visObjectIndex = visSurf->space->index;

    return (subMeshIndex << 32) | (shaderMaterialIndex << 16) | visObjectIndex;
}

static uint64_t BuildMainBlendSortKey(const VisSurf *visSurf) {
    // 일단은 기존 order 유지에 가까운 key.
    // 나중에 camera-space depth 기반 back-to-front 로 바꾸면 됨.
    uint64_t shaderMaterialIndex = visSurf->shaderMaterialIndex & 0xFFFF;
    uint64_t visObjectIndex = visSurf->space->index;

    return (shaderMaterialIndex << 16) | visObjectIndex;
}

void RenderPassBuilder::BuildMain(RenderFrameData *frameData, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildMain");

    if (visCamera->numVisSurfs == 0) {
        return;
    }

    RenderPass *opaquePass = AllocRenderPass(frameData, visCamera, RenderPassType::MainOpaque, {}, visCamera->numVisSurfs);
    RenderContext::activeContext->GetMainRenderDest(opaquePass->pipelineDesc.renderDest);
    opaquePass->pipelineDesc.inputLayout = &VertexInputLayoutFactory::GetGenericLit();

    RenderPass *blendPass = AllocRenderPass(frameData, visCamera, RenderPassType::MainBlend, {}, visCamera->numVisSurfs);
    RenderContext::activeContext->GetMainRenderDest(blendPass->pipelineDesc.renderDest);
    blendPass->pipelineDesc.inputLayout = &VertexInputLayoutFactory::GetGenericLit();

    opaquePipelineResolver.Begin(opaquePass->pipelineDesc, visCamera->numVisSurfs * 2);
    blendPipelineResolver.Begin(blendPass->pipelineDesc, visCamera->numVisSurfs * 2);

    for (uint32_t i = 0; i < visCamera->numVisSurfs; ++i) {
        const VisSurf *visSurf = visCamera->visSurfs[i];
        const bool isBlend = visSurf->material->GetRenderingMode() == Material::RenderingMode::Blend;
        RenderPass *pass = isBlend ? blendPass : opaquePass;
        PassPipelineResolver &pipelineResolver = isBlend ? blendPipelineResolver : opaquePipelineResolver;

        ShaderFeatureSet baseFeatureSet = visSurf->baseFeatureSet | pass->passFeatureSet;
        RHI::PipelineState *basePSO = pipelineResolver.Resolve(visSurf->material, baseFeatureSet);

        RHI::PipelineState *instancedPSO = nullptr;
        if (BE1::HasFlag(visSurf->flags, VisSurfFlag::UseInstancing)) {
            ShaderFeatureSet instancedFeatureSet = baseFeatureSet;
            instancedFeatureSet.EnableFeature(ShaderFeature::GpuInstancing);
            instancedPSO = pipelineResolver.Resolve(visSurf->material, instancedFeatureSet);
        }

        AddPassDrawSurf(frameData, pass, visSurf, isBlend ? BuildMainBlendSortKey(visSurf) : BuildMainOpaqueSortKey(visSurf), basePSO, instancedPSO);
    }

    SortPassDrawSurfs(frameData, opaquePass);
    SortPassDrawSurfs(frameData, blendPass);
}

void RenderPassBuilder::BuildGUI(RenderFrameData *frameData, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildGUI");

    if (visCamera->numVisSurfs == 0) {
        return;
    }

    RenderPass *guiPass = AllocRenderPass(frameData, visCamera, RenderPassType::Gui, {}, visCamera->numVisSurfs);
    RenderContext::activeContext->GetMainRenderDest(guiPass->pipelineDesc.renderDest);
    guiPass->pipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Never;
    guiPass->pipelineDesc.inputLayout = &VertexInputLayoutFactory::GetGeneric();

    guiPipelineResolver.Begin(guiPass->pipelineDesc, visCamera->numVisSurfs);

    for (uint32_t i = 0; i < visCamera->numVisSurfs; ++i) {
        const VisSurf *visSurf = visCamera->visSurfs[i];
        ShaderFeatureSet featureSet = visSurf->baseFeatureSet | guiPass->passFeatureSet;
        RHI::PipelineState *basePSO = guiPipelineResolver.Resolve(visSurf->material, featureSet);

        AddPassDrawSurf(frameData, guiPass, visSurf, i, basePSO, nullptr);
    }
}

static void RadixSortDrawSurfs(const PassDrawSurf **drawSurfs, uint32_t numDrawSurfs, const PassDrawSurf **tempDrawSurfs) {
    constexpr uint32_t NumBuckets = 256;
    constexpr uint32_t NumPasses = 8;

    const PassDrawSurf **src = drawSurfs;
    const PassDrawSurf **dst = tempDrawSurfs;

    for (uint32_t pass = 0; pass < NumPasses; pass++) {
        const uint32_t shift = pass * 8;

        uint32_t counts[NumBuckets] = {};

        for (uint32_t i = 0; i < numDrawSurfs; i++) {
            const uint32_t bucket = (src[i]->sortKey >> shift) & 0xFF;
            counts[bucket]++;
        }

        uint32_t offsets[NumBuckets];
        uint32_t sum = 0;

        for (uint32_t i = 0; i < NumBuckets; i++) {
            offsets[i] = sum;
            sum += counts[i];
        }

        for (uint32_t i = 0; i < numDrawSurfs; i++) {
            const PassDrawSurf *drawSurf = src[i];
            const uint32_t bucket = (drawSurf->sortKey >> shift) & 0xFF;
            dst[offsets[bucket]++] = drawSurf;
        }

        const PassDrawSurf **swap = src;
        src = dst;
        dst = swap;
    }

    // 8 passes 라 짝수 번 swap 되므로 보통 src == drawSurfs 이지만,
    // pass 수가 바뀌어도 안전하게 유지.
    if (src != drawSurfs) {
        memcpy(drawSurfs, src, numDrawSurfs * sizeof(drawSurfs[0]));
    }
}

void RenderPassBuilder::SortPassDrawSurfs(RenderFrameData *frameData, RenderPass *renderPass) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::SortPassDrawSurfs");

    if (renderPass->numDrawSurfs < 128) {
        std::sort(renderPass->drawSurfs, renderPass->drawSurfs + renderPass->numDrawSurfs, [](const PassDrawSurf *lhs, const PassDrawSurf *rhs) {
            return lhs->sortKey < rhs->sortKey;
        });
        return;
    }

    const PassDrawSurf **tempDrawSurfs = (const PassDrawSurf **)frameData->MemAlloc(renderPass->numDrawSurfs * sizeof(tempDrawSurfs[0]));
    RadixSortDrawSurfs(renderPass->drawSurfs, renderPass->numDrawSurfs, tempDrawSurfs);
}
