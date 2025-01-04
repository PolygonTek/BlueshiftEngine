// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "D3D12Common.h"

class D3D12Renderer;

class D3D12SwapChain : public RHI::SwapChain {
    friend class D3D12Renderer;

public:
    virtual ~D3D12SwapChain() { Release(); }

    void                            Release();

    void                            CreateRTVs();

    virtual uint32_t                GetWidth() const { return viewportRect.w; }
    virtual uint32_t                GetHeight() const { return viewportRect.h; }

    virtual void                    Resize(uint32_t width, uint32_t height) override;
    virtual void                    SwapBuffers(bool vsync) override;

    ID3D12Resource *                GetCurrentBackBuffer() const { return renderTargetBuffers[currentBackBufferIndex]; }
    D3D12_CPU_DESCRIPTOR_HANDLE     GetCurrentBackBufferDescriptorHandle() const { return rtvDescriptorHandles[currentBackBufferIndex]; }

    bool                            IsSwapChainSupportsHDR() const;

    static constexpr int            NumSwapChainBuffers = 3;

private:
    IDXGISwapChain3 *               dxgiSwapChain = nullptr;
    ID3D12Resource *                renderTargetBuffers[NumSwapChainBuffers] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE     rtvDescriptorHandles[NumSwapChainBuffers] = {};
    uint32_t                        currentBackBufferIndex = 0;
    BE1::Rect                       viewportRect;
    BE1::Rect                       scissorRect;
};
