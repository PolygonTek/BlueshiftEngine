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
#include "RenderBackend.h"

void RenderBackend::DispatchSeparableBlur(const SeparableBlurDesc &desc) {
    RHI::Texture *inputTexture = desc.inputTexture;
    RHI::Texture *tempTexture = desc.tempTexture;
    RHI::Texture *outputTexture = desc.outputTexture;

    assert(desc.blockSizeX > 0 && desc.blockSizeY > 0);
    assert(inputTexture != tempTexture && tempTexture != outputTexture);
    assert(inputTexture->GetWidth() == outputTexture->GetWidth() && inputTexture->GetHeight() == outputTexture->GetHeight());
    assert(tempTexture->GetWidth() == outputTexture->GetWidth() && tempTexture->GetHeight() == outputTexture->GetHeight());

    const uint32_t groupCountX = BE1::DivCeil(outputTexture->GetWidth(), desc.blockSizeX);
    const uint32_t groupCountY = BE1::DivCeil(outputTexture->GetHeight(), desc.blockSizeY);

    // Horizontal blur
    RHI::GPUBarrier horizontalBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(inputTexture, desc.inputState, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(tempTexture, RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(mainCommandList, horizontalBarriers, COUNT_OF(horizontalBarriers));

    RHI::renderer->SetPSO(mainCommandList, desc.horizontalPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, inputTexture);
    if (desc.depthTexture) {
        RHI::renderer->SetTexture(mainCommandList, 1, false, desc.depthTexture);
    }
    RHI::renderer->SetTexture(mainCommandList, 0, true, tempTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, desc.constants, desc.constantsSize, 0);
    if (desc.viewConstantBuffer) {
        RHI::renderer->SetConstantBuffer(mainCommandList, 1, desc.viewConstantBuffer);
    }

    RHI::renderer->Dispatch(mainCommandList, groupCountX, groupCountY, 1);

    // Vertical blur
    RHI::GPUBarrier verticalBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(tempTexture, RHI::GPUResourceState::UnorderedAccess, RHI::GPUResourceState::ShaderResourceCompute),
        RHI::Renderer::MakeTextureBarrier(outputTexture, inputTexture == outputTexture ? RHI::GPUResourceState::ShaderResourceCompute : RHI::GPUResourceState::ShaderResource, RHI::GPUResourceState::UnorderedAccess)
    };
    RHI::renderer->Barrier(mainCommandList, verticalBarriers, COUNT_OF(verticalBarriers));

    RHI::renderer->SetPSO(mainCommandList, desc.verticalPSO);
    RHI::renderer->SetTexture(mainCommandList, 0, false, tempTexture);
    if (desc.depthTexture) {
        RHI::renderer->SetTexture(mainCommandList, 1, false, desc.depthTexture);
    }
    RHI::renderer->SetTexture(mainCommandList, 0, true, outputTexture);
    RHI::renderer->SetConstants(mainCommandList, 0, desc.constants, desc.constantsSize, 0);
    if (desc.viewConstantBuffer) {
        RHI::renderer->SetConstantBuffer(mainCommandList, 1, desc.viewConstantBuffer);
    }

    RHI::renderer->Dispatch(mainCommandList, groupCountX, groupCountY, 1);

    RHI::GPUBarrier endBarriers[] = {
        RHI::Renderer::MakeTextureBarrier(tempTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource),
        RHI::Renderer::MakeTextureBarrier(inputTexture, RHI::GPUResourceState::ShaderResourceCompute, RHI::GPUResourceState::ShaderResource)
    };
    // 입력과 출력을 재사용하면 최종 결과는 UAV 상태로 호출부에 넘긴다.
    RHI::renderer->Barrier(mainCommandList, endBarriers, inputTexture == outputTexture ? 1 : COUNT_OF(endBarriers));
}
