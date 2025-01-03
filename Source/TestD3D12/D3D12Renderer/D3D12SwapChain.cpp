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

#include "Precompiled.h"
#include "D3D12Renderer.h"
#include "D3D12DescriptorPool.h"
#include "D3D12SwapChain.h"

D3D12SwapChain *D3D12Renderer::CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height) {
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = (UINT)width;
    swapChainDesc.Height = (UINT)height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
    //swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = D3D12SwapChain::NumSwapChainBuffers;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    if (supportsTearing) {
        swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = {};
    swapChainFullscreenDesc.Windowed = TRUE;

    IDXGISwapChain1 *dxgiSwapChain1 = nullptr;
    HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(commandQueues[to_int(CommandQueueType::Graphics)], hwnd, &swapChainDesc, &swapChainFullscreenDesc, nullptr, &dxgiSwapChain1);
    if (FAILED(hr)) {
        BE_FATALERROR("CreateSwapChainForHwnd failed, ERROR: 0x%x", hr);
    }
    hr = dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(hr)) {
        BE_FATALERROR("MakeWindowAssociation failed, ERROR: 0x%x", hr);
    }

    IDXGISwapChain3 *dxgiSwapChain3 = nullptr;
    hr = dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain3));
    dxgiSwapChain1->Release();
    if (FAILED(hr)) {
        BE_FATALERROR("Failed to create swapchain, ERROR: 0x%x", hr);
    }

    //IDXGIOutput *dxgiOutput = nullptr;
    //hr = dxgiSwapChain3->GetContainingOutput(&dxgiOutput);

    D3D12SwapChain *swapChain = new D3D12SwapChain;
    swapChain->dxgiSwapChain = dxgiSwapChain3;
    swapChain->CreateRTVs();

    // Viewport 설정을 백버퍼 크기에 맞게 설정
    swapChain->viewportRect.x = 0.0f;
    swapChain->viewportRect.y = 0.0f;
    swapChain->viewportRect.w = (float)width;
    swapChain->viewportRect.h = (float)height;

    // ScissorRect 설정을 백버퍼 크기에 맞게 설정
    swapChain->scissorRect.x = 0;
    swapChain->scissorRect.y = 0;
    swapChain->scissorRect.w = width;
    swapChain->scissorRect.h = height;

    return swapChain;
}

void D3D12SwapChain::Release() {
    SAFE_RELEASE_ARRAY(renderTargetBuffers);
    SAFE_RELEASE(dxgiSwapChain);
}

void D3D12SwapChain::CreateRTVs() {
    for (UINT bufferIndex = 0; bufferIndex < D3D12SwapChain::NumSwapChainBuffers; ++bufferIndex) {
        if (rtvDescriptorHandles[bufferIndex].ptr != 0) {
            renderer->rtvCpuDescriptorPool->Free(rtvDescriptorHandles[bufferIndex]);
        }
        renderer->rtvCpuDescriptorPool->Alloc(&rtvDescriptorHandles[bufferIndex], nullptr);

        // 스왑체인의 백버퍼 리소스를 가져온다.
        dxgiSwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&renderTargetBuffers[bufferIndex]));

        // 백버퍼로 RTV 를 생성한다.
        renderer->device->CreateRenderTargetView(renderTargetBuffers[bufferIndex], nullptr, rtvDescriptorHandles[bufferIndex]);
    }

    currentBackBufferIndex = dxgiSwapChain->GetCurrentBackBufferIndex();
}

void D3D12SwapChain::Resize(uint32_t width, uint32_t height) {
    // 기존 스왑 체인 백버퍼 해제
    SAFE_RELEASE_ARRAY(renderTargetBuffers);

    // 스왑 체인 버퍼의 사이즈를 조정한다.
    dxgiSwapChain->ResizeBuffers(D3D12SwapChain::NumSwapChainBuffers, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);

    CreateRTVs();

    viewportRect.w = static_cast<float>(width);
    viewportRect.h = static_cast<float>(height);

    scissorRect.w = width;
    scissorRect.h = height;
}

void D3D12SwapChain::SwapBuffers(bool vsync) {
    if (dxgiSwapChain->Present(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING) == DXGI_ERROR_DEVICE_REMOVED) {
        BE_FATALERROR("DXGI Device Removed");
    }

    // 다음 프레임에 사용할 백버퍼 인덱스 얻어오기
    currentBackBufferIndex = dxgiSwapChain->GetCurrentBackBufferIndex();
}

bool D3D12SwapChain::IsSwapChainSupportsHDR() const {
    // HDR display query: https://docs.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
    Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
    if (SUCCEEDED(dxgiSwapChain->GetContainingOutput(&dxgiOutput))) {
        Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
        if (SUCCEEDED(dxgiOutput.As(&output6))) {
            DXGI_OUTPUT_DESC1 desc1;
            if (SUCCEEDED(output6->GetDesc1(&desc1))) {
                if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
                    return true;
                }
            }
        }
    }
    return false;
}
