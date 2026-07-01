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
#include "Render/Material/Material.h"
#include "Render/ShaderPipeline/VertexInputLayoutFactory.h"

void PassPipelineResolver::Begin(const GraphicsPipelineDesc &pipelineDesc, uint32_t maxEntries) {
    basePipelineDesc = pipelineDesc;
    memset(pipelineDescValid, 0, sizeof(pipelineDescValid));

    maxEntries = BE1::Max(maxEntries, 1u);
    entriesHash.Clear();
    entries.Reserve(maxEntries);
    entries.SetCount(0, false);
}

const GraphicsPipelineDesc &PassPipelineResolver::GetPipelineDesc(VertexInputLayoutId inputLayoutId) {
    const int inputLayoutIndex = static_cast<int>(inputLayoutId);
    assert(inputLayoutIndex >= 0 && inputLayoutIndex < NumInputLayouts);

    if (!pipelineDescValid[inputLayoutIndex]) {
        pipelineDescs[inputLayoutIndex] = basePipelineDesc;
        pipelineDescs[inputLayoutIndex].inputLayout = &VertexInputLayoutFactory::Get(inputLayoutId);
        pipelineDescValid[inputLayoutIndex] = true;
    }

    return pipelineDescs[inputLayoutIndex];
}

PassPipelineResolver::ResolveResult PassPipelineResolver::Resolve(const RenderPass *renderPass, const VisSurf *visSurf, const ShaderFeatureSet &featureSet) {
    assert(renderPass);
    assert(visSurf);
    assert(visSurf->material);
    assert(visSurf->subMesh);

    ResolveResult result;
    result.inputLayoutId = VertexInputLayoutFactory::GetSkinnedId(renderPass->baseInputLayoutId, visSurf->subMesh->MaxVertexWeights());

    // material pointer, featureSet, input layout id 로 hash 키를 만든다.
    const Material *material = visSurf->material;
    uint64_t hash = reinterpret_cast<uintptr_t>(material);
    hash = hash_combine(hash, featureSet.flags);
    hash = hash_combine(hash, static_cast<int>(result.inputLayoutId));

    const int hashKey = entriesHash.GenerateHash(hash);
    for (int index = entriesHash.First(hashKey); index != -1; index = entriesHash.Next(index)) {
        const Entry &entry = entries[index];
        if (entry.material == material && entry.featureFlags == featureSet.flags && entry.inputLayoutId == result.inputLayoutId) {
            result.pso = entry.pso;
            return result;
        }
    }

    // hash 에 존재하지 않는다면 material->GetGraphicsPipeline 호출로 새 ShaderPipeline 을 얻는다.
    result.pso = ShaderPipeline::GetReadyPSO(material->GetGraphicsPipeline(featureSet, GetPipelineDesc(result.inputLayoutId)));

    PassPipelineResolver::Entry &entry = entries.Alloc();
    entry.material = material;
    entry.featureFlags = featureSet.flags;
    entry.inputLayoutId = result.inputLayoutId;
    entry.pso = result.pso;
    entriesHash.Add(hashKey, entries.Count() - 1);

    return result;
}

RenderPass *RenderPassBuilder::AllocRenderPass(RenderFrameData *frameData, VisCamera *visCamera, RenderPassType type, ShaderFeatureSet passFeatureSet, VertexInputLayoutId baseInputLayoutId, int maxDrawSurfs) {
    assert(visCamera->numRenderPasses < VisCamera::MaxRenderPasses);

    RenderPass *renderPass = &visCamera->renderPasses[visCamera->numRenderPasses++];
    renderPass->type = type;
    renderPass->pipelineDesc = {};
    renderPass->passFeatureSet = passFeatureSet;
    renderPass->baseInputLayoutId = baseInputLayoutId;
    renderPass->pipelineDesc.inputLayout = &VertexInputLayoutFactory::Get(baseInputLayoutId);
    renderPass->maxDrawSurfs = maxDrawSurfs;
    renderPass->numDrawSurfs = 0;
    renderPass->drawSurfs = (const PassDrawSurf **)frameData->MemAlloc(maxDrawSurfs * sizeof(PassDrawSurf *));

    return renderPass;
}

void RenderPassBuilder::AddPassDrawSurf(RenderFrameData *frameData, RenderPass *renderPass, const VisSurf *visSurf, uint64_t sortKey, VertexInputLayoutId inputLayoutId, RHI::PipelineState *pso) {
    assert(renderPass->numDrawSurfs < renderPass->maxDrawSurfs);

    PassDrawSurf *passDrawSurf = (PassDrawSurf *)frameData->MemAlloc(sizeof(PassDrawSurf));
    passDrawSurf->visSurf = visSurf;
    passDrawSurf->sortKey = sortKey;
    passDrawSurf->inputLayoutId = inputLayoutId;
    passDrawSurf->pso = pso;

    renderPass->drawSurfs[renderPass->numDrawSurfs++] = passDrawSurf;
}

static uint64_t BuildMainOpaqueSortKey(const VisSurf *visSurf, RHI::PipelineState *pso) {
    //-----------------------------------------------------------
    // 0xFFFFFFFF00000000 (0~4294967295) : PSO sort index
    // 0x00000000FFFF0000 (0~65535)      : subMesh index
    // 0x000000000000FFFF (0~65535)      : shader material index
    //-----------------------------------------------------------
    uint64_t psoIndex = pso ? pso->sortIndex : 0;
    uint64_t subMeshIndex = visSurf->subMesh ? (visSurf->subMesh->GetID() & 0xFFFF) : 0;
    uint64_t shaderMaterialIndex = visSurf->shaderMaterialIndex & 0xFFFF;

    return (psoIndex << 32) | (subMeshIndex << 16) | shaderMaterialIndex;
}

static uint64_t BuildMainTransparentSortKey(const VisSurf *visSurf, const VisCamera *visCamera) {
    const BE1::Vec3 localCenter = visSurf->subMesh->GetAABB().Center();
    const BE1::Vec3 worldCenter = visSurf->space->worldMatrix.TransformPos(localCenter);
    const BE1::Vec3 viewCenter = visCamera->viewMatrix.TransformPos(worldCenter);

    uint32_t bits;
    memcpy(&bits, &viewCenter.z, sizeof(bits));
    return (bits & 0x80000000u) ? ~bits : (bits ^ 0x80000000u);
}

void RenderPassBuilder::BuildMain(RenderFrameData *frameData, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildMain");

    if (visCamera->numVisSurfs == 0) {
        return;
    }

    // AllocRenderPass 한 순서대로 백엔드에서 렌더링된다.
    RenderPass *opaquePass = AllocRenderPass(frameData, visCamera, RenderPassType::MainOpaque, {}, VertexInputLayoutId::XyzStColorNormalTangent, visCamera->numVisSurfs);
    RenderContext::activeContext->GetMainRenderDest(opaquePass->pipelineDesc.renderDest);

    RenderPass *skyboxPass = AllocRenderPass(frameData, visCamera, RenderPassType::Skybox, {}, VertexInputLayoutId::XyzStColor, 2);
    RenderContext::activeContext->GetMainRenderDest(skyboxPass->pipelineDesc.renderDest);
    skyboxPass->pipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::ReadOnly;

    RenderPass *blendPass = AllocRenderPass(frameData, visCamera, RenderPassType::MainTransparent, {}, VertexInputLayoutId::XyzStColorNormalTangent, visCamera->numVisSurfs);
    RenderContext::activeContext->GetMainRenderDest(blendPass->pipelineDesc.renderDest);

    skyboxPipelineResolver.Begin(skyboxPass->pipelineDesc, skyboxPass->maxDrawSurfs);
    opaquePipelineResolver.Begin(opaquePass->pipelineDesc, opaquePass->maxDrawSurfs);
    blendPipelineResolver.Begin(blendPass->pipelineDesc, blendPass->maxDrawSurfs);

    for (uint32_t i = 0; i < visCamera->numVisSurfs; ++i) {
        const VisSurf *visSurf = visCamera->visSurfs[i];
        if (BE1::HasFlag(visSurf->flags, VisSurfFlag::Skybox)) {
            ShaderFeatureSet featureSet = visSurf->baseFeatureSet | skyboxPass->passFeatureSet;
            const PassPipelineResolver::ResolveResult resolveResult = skyboxPipelineResolver.Resolve(skyboxPass, visSurf, featureSet);

            AddPassDrawSurf(frameData, skyboxPass, visSurf, BuildMainOpaqueSortKey(visSurf, resolveResult.pso), resolveResult.inputLayoutId, resolveResult.pso);
            continue;
        }

        const bool isBlend = visSurf->material->GetRenderingMode() == Material::RenderingMode::Blend;
        RenderPass *pass = isBlend ? blendPass : opaquePass;
        PassPipelineResolver &pipelineResolver = isBlend ? blendPipelineResolver : opaquePipelineResolver;

        ShaderFeatureSet featureSet = visSurf->baseFeatureSet | pass->passFeatureSet;
        const PassPipelineResolver::ResolveResult resolveResult = pipelineResolver.Resolve(pass, visSurf, featureSet);

        AddPassDrawSurf(frameData, pass, visSurf, isBlend ? BuildMainTransparentSortKey(visSurf, visCamera) : BuildMainOpaqueSortKey(visSurf, resolveResult.pso), resolveResult.inputLayoutId, resolveResult.pso);
    }

    SortPassDrawSurfs(frameData, opaquePass);
    SortPassDrawSurfs(frameData, blendPass);
}

void RenderPassBuilder::BuildGUI(RenderFrameData *frameData, VisCamera *visCamera) {
    PROFILER_CPU_SCOPED_EVENT("RenderPassBuilder::BuildGUI");

    if (visCamera->numVisSurfs == 0) {
        return;
    }

    RenderPass *guiPass = AllocRenderPass(frameData, visCamera, RenderPassType::Gui, {}, VertexInputLayoutId::XyzStColor, visCamera->numVisSurfs);
    RenderContext::activeContext->GetSwapChainRenderDest(guiPass->pipelineDesc.renderDest);
    guiPass->pipelineDesc.depthStencilStateType = RHI::DepthStencilStateType::Never;

    guiPipelineResolver.Begin(guiPass->pipelineDesc, visCamera->numVisSurfs);

    for (uint32_t i = 0; i < visCamera->numVisSurfs; ++i) {
        const VisSurf *visSurf = visCamera->visSurfs[i];
        ShaderFeatureSet featureSet = visSurf->baseFeatureSet | guiPass->passFeatureSet;
        const PassPipelineResolver::ResolveResult resolveResult = guiPipelineResolver.Resolve(guiPass, visSurf, featureSet);

        AddPassDrawSurf(frameData, guiPass, visSurf, i, resolveResult.inputLayoutId, resolveResult.pso);
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
