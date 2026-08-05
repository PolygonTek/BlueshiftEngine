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

#include "Precompiled.h"
#include "RenderWork.h"

class Texture;

class ConvolutionBloomKernelRenderWork : public RenderWork {
public:
    ConvolutionBloomKernelRenderWork(uint64_t requestSerial, const SharedPtr<Texture> &apertureTexture, float apertureScale, float dispersionAmount);

    virtual const char *            GetDebugName() const override { return "ConvolutionBloomKernelRenderWork"; }
    virtual bool                    IsReadyToAttach() const override;
    virtual bool                    RecordCommands(RHI::FrameThreadData *frameThreadData, RHI::CommandList *commandList) override;
    virtual void                    OnCompleted(bool commandListSubmitted) override;

private:
    uint64_t                        requestSerial = 0;
    SharedPtr<Texture>              apertureTexture;
    float                           apertureScale = 0.0f;
    float                           dispersionAmount = 0.0f;
};
