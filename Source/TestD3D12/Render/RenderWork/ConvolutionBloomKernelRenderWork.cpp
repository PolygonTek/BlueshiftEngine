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
#include "ConvolutionBloomKernelRenderWork.h"
#include "Render/PostProcess/ConvolutionBloomKernelGenerator.h"
#include "Render/RenderSystem.h"
#include "Render/Texture/Texture.h"

ConvolutionBloomKernelRenderWork::ConvolutionBloomKernelRenderWork(uint64_t requestSerial, const SharedPtr<Texture> &apertureTexture, float apertureScale, float dispersionAmount) {
    this->requestSerial = requestSerial;
    this->apertureTexture = apertureTexture;
    this->apertureScale = apertureScale;
    this->dispersionAmount = dispersionAmount;
}

bool ConvolutionBloomKernelRenderWork::IsReadyToAttach() const {
    return apertureTexture && apertureTexture->IsReadyForRendering();
}

bool ConvolutionBloomKernelRenderWork::RecordCommands(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList) {
    assert(!renderSystem->IsUsingRenderThread() || RenderSystem::IsInRenderThread());

    if (!apertureTexture) {
        return false;
    }

    ConvolutionBloomKernelGenerator &kernelGenerator = renderSystem->GetConvolutionBloomKernelGenerator();
    kernelGenerator.SetApertureTexture(apertureTexture);
    kernelGenerator.SetApertureScale(apertureScale);
    kernelGenerator.SetDispersionAmount(dispersionAmount);
    return kernelGenerator.GenerateKernel(commandList, frameThreadData);
}

void ConvolutionBloomKernelRenderWork::OnCompleted(bool commandListSubmitted) {
    renderSystem->CompleteConvolutionBloomKernelGeneration(requestSerial, commandListSubmitted);
}
