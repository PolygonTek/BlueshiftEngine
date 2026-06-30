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
#include "RHI/RHI.h"

class RenderFrameData;
class Texture;

/*
---------------------------------------------------------------------------------------------------

    Convolution Bloom Kernel Generator

---------------------------------------------------------------------------------------------------
*/

class ConvolutionBloomKernelGenerator {
public:
    static constexpr uint32_t       KernelSize = 2048;

    void                            Init();
    void                            Shutdown();

    void                            SetApertureTexture(const SharedPtr<Texture> &texture);
    const SharedPtr<Texture> &      GetApertureTexture() const { return apertureSourceTexture; }

    void                            SetDiffractionPSFScale(float scale);
    float                           GetDiffractionPSFScale() const { return diffractionPSFScale; }

    void                            SetDispersionAmount(float amount);
    float                           GetDispersionAmount() const { return dispersionAmount; }

    bool                            GenerateKernel(RHI::CommandList *commandList, RenderFrameData *frameData);
    bool                            ReadKernelImage(BE1::Image &outImage) const;

    bool                            EnsureResources();
    void                            DestroyResources();
    bool                            HasResources() const;

    bool                            IsKernelDirty() const { return kernelDirty; }
    void                            MarkKernelClean() { kernelDirty = false; }

    uint32_t                        GetKernelSize() const { return KernelSize; }
    RHI::Buffer *                   GetApertureSpatialBuffer() const { return apertureSpatialBuffer; }
    RHI::Buffer *                   GetApertureSpectrumBuffer() const { return apertureSpectrumBuffer; }
    RHI::Buffer *                   GetWorkBuffer() const { return workBuffer; }
    RHI::Buffer *                   GetApertureEnergyBuffer0() const { return apertureEnergyBuffer0; }
    RHI::Buffer *                   GetApertureEnergyBuffer1() const { return apertureEnergyBuffer1; }
    RHI::Texture *                  GetDiffractionPSFTexture() const { return diffractionPSFTexture; }
    RHI::Texture *                  GetKernelTexture() const { return kernelTexture; }

private:
    bool                            DispatchAperturePrepare(RHI::CommandList *commandList, RenderFrameData *frameData);
    bool                            DispatchReduceApertureEnergy(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer **outEnergyBuffer);
    bool                            DispatchReduceApertureEnergyPass(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *inputBuffer, RHI::Buffer *outputBuffer, uint32_t inputElementCount, bool inputIsPartialEnergy, RHI::GPUResourceState inputState);
    bool                            DispatchResolveDiffractionPSF(RHI::CommandList *commandList, RenderFrameData *frameData, RHI::Buffer *apertureEnergyBuffer);
    bool                            DispatchApplyDispersion(RHI::CommandList *commandList, RenderFrameData *frameData);

    SharedPtr<Texture>              apertureSourceTexture;
    bool                            kernelDirty = true;
    float                           diffractionPSFScale = 1.0f;
    float                           dispersionAmount = 1.0f;

    RHI::Buffer *                   apertureSpatialBuffer = nullptr;
    RHI::Buffer *                   apertureSpectrumBuffer = nullptr;
    RHI::Buffer *                   workBuffer = nullptr;
    RHI::Buffer *                   apertureEnergyBuffer0 = nullptr;
    RHI::Buffer *                   apertureEnergyBuffer1 = nullptr;
    RHI::Texture *                  diffractionPSFTexture = nullptr;
    RHI::Texture *                  kernelTexture = nullptr;
};
