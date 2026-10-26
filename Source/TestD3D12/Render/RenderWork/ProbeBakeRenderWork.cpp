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
#include "ProbeBakeRenderWork.h"
#include "Render/ProbeBakeSystem.h"
#include "Render/RenderSystem.h"
#include "Render/Texture/Texture.h"

ProbeBakeRenderWork::ProbeBakeRenderWork(uint64_t bakeSerial, ProbeBakeJob::Desc &&jobDesc, bool generateRadianceMipChain)
    : bakeSerial(bakeSerial), bakeJob(std::move(jobDesc)), generateRadianceMipChain(generateRadianceMipChain) {
}

bool ProbeBakeRenderWork::IsReadyToAttach() const {
    const SharedPtr<Texture> &radianceCubeMap = bakeJob.GetRadianceCubeMap();
    return radianceCubeMap && radianceCubeMap->IsReadyForRendering();
}

bool ProbeBakeRenderWork::RecordCommands(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList) {
    assert(!renderSystem->IsUsingRenderThread() || RenderSystem::IsInRenderThread());

    const SharedPtr<Texture> &radianceCubeMap = bakeJob.GetRadianceCubeMap();
    RHI::Texture *radianceRHITexture = radianceCubeMap ? radianceCubeMap->GetRHITexture() : nullptr;

    if (!radianceRHITexture || radianceRHITexture->GetStatus() != RHI::GPUResourceStatus::Ready) {
        BE_WARNLOG("ProbeBakeRenderWork::RecordCommands: radiance cubemap is not ready\n");
        return false;
    }

    if (generateRadianceMipChain) {
        const uint32_t mipLevelCount = radianceRHITexture->GetMipLevelCount();
        RHI::ViewID *mipSRVs = static_cast<RHI::ViewID *>(_alloca(sizeof(RHI::ViewID) * mipLevelCount));
        RHI::ViewID *mipUAVs = static_cast<RHI::ViewID *>(_alloca(sizeof(RHI::ViewID) * mipLevelCount));

        for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel) {
            mipSRVs[mipLevel] = RHI::renderer->CreateView(radianceRHITexture, RHI::ViewType::SRV, RHI::Format::Auto, 0, radianceRHITexture->GetArraySize(), mipLevel, 1);
            mipUAVs[mipLevel] = RHI::renderer->CreateView(radianceRHITexture, RHI::ViewType::UAV, RHI::Format::Auto, 0, radianceRHITexture->GetArraySize(), mipLevel, 1);

            if (!mipSRVs[mipLevel].IsValid() || !mipUAVs[mipLevel].IsValid()) {
                BE_WARNLOG("ProbeBakeRenderWork::RecordCommands: failed to create radiance cubemap mip views\n");
                return false;
            }
        }

        // 캡처 경로는 mip 0 만 채우므로 probe 필터링 전에 입력 mip chain 을 생성한다.
        RHI::renderer->GenerateMipChain(commandList, radianceRHITexture, mipSRVs, mipUAVs, false);
    }

    return bakeJob.Execute(commandList);
}

void ProbeBakeRenderWork::OnCompleted(bool commandListSubmitted) {
    renderSystem->GetProbeBakeSystem().CompleteBake(bakeSerial, commandListSubmitted, bakeJob.GetDiffuseIrradianceCubeMap(), bakeJob.GetSpecularPrefilteredCubeMap());
}
