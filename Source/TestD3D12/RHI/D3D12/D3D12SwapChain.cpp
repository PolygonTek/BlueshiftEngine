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
#include "D3D12Renderer.h"
#include "D3D12DescriptorPool.h"
#include "D3D12SwapChain.h"

static CVAR(d3d12_swapChainAllowTearing, "1", BE1::CVar::Flag::Bool, "allow tearing when presenting without vsync");

RHI::SwapChain *D3D12Renderer::CreateSwapChain(HWND hwnd, uint32_t width, uint32_t height, RHI::Format format) {
    DXGI_FORMAT dxgiFormat = RHIFormatToDXGIFormat(format);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = (UINT)width;
    swapChainDesc.Height = (UINT)height;
    swapChainDesc.Format = dxgiFormat;
    //swapChainDesc.BufferDesc.RefreshRate.Numerator = refreshRate;
    //swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = D3D12SwapChain::NumSwapChainBuffers;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    if (supportsTearing) {
        swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = {};
    swapChainFullscreenDesc.Windowed = TRUE;

    IDXGISwapChain1 *dxgiSwapChain1 = nullptr;
    HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(commandQueues[to_int(RHI::CommandQueueType::Graphics)], hwnd, &swapChainDesc, &swapChainFullscreenDesc, nullptr, &dxgiSwapChain1);
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

    hr = dxgiSwapChain3->SetMaximumFrameLatency(1);
    if (FAILED(hr)) {
        BE_FATALERROR("SetMaximumFrameLatency failed, ERROR: 0x%x", hr);
    }

    //IDXGIOutput *dxgiOutput = nullptr;
    //hr = dxgiSwapChain3->GetContainingOutput(&dxgiOutput);

    D3D12SwapChain *swapChain = new D3D12SwapChain;
    swapChain->hwnd = hwnd;
    swapChain->swapChainFormat = dxgiFormat;
    swapChain->width = width;
    swapChain->height = height;
    swapChain->dxgiSwapChain = dxgiSwapChain3;
    swapChain->CreateRTVs();

    return swapChain;
}

void D3D12Renderer::DestroySwapChain(RHI::SwapChain *swapChain) {
    SAFE_DELETE(swapChain);
}

void D3D12SwapChain::Release() {
    ReleaseRTVs();

    for (int bufferIndex = 0; bufferIndex < COUNT_OF(backBuffers); ++bufferIndex) {
        SAFE_RELEASE(backBuffers[bufferIndex]);
    }
    SAFE_RELEASE(dxgiSwapChain);
    hwnd = nullptr;
}

void D3D12SwapChain::ReleaseRTVs() {
    D3D12DescriptorPool *rtvDescriptorPool = D3D12Renderer::GetRenderer()->rtvDescriptorPool;

    for (int bufferIndex = 0; bufferIndex < COUNT_OF(backBufferRTVs); ++bufferIndex) {
        if (backBufferRTVs[bufferIndex].ptr != 0) {
            rtvDescriptorPool->Free(backBufferRTVs[bufferIndex]);
            backBufferRTVs[bufferIndex] = {};
        }
    }
}

void D3D12SwapChain::CreateRTVs() {
    // 각 백버퍼의 RTV 를 생성한다.
    if (D3D12Renderer::IsHDRSwapChainFormat(swapChainFormat)) {
        rtvFormat = swapChainFormat;
    } else {
        // sRGB 포맷으로 변환
        rtvFormat = D3D12Renderer::ToSRGBFormat(swapChainFormat);
    }

    ReleaseRTVs();

    for (int bufferIndex = 0; bufferIndex < COUNT_OF(backBuffers); ++bufferIndex) {
        D3D12Renderer::GetRenderer()->rtvDescriptorPool->Alloc(&backBufferRTVs[bufferIndex], nullptr);

        // 스왑 체인의 백버퍼 리소스를 가져온다.
        dxgiSwapChain->GetBuffer(bufferIndex, IID_PPV_ARGS(&backBuffers[bufferIndex]));

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;

        D3D12Renderer::GetRenderer()->GetDevice()->CreateRenderTargetView(backBuffers[bufferIndex], &rtvDesc, backBufferRTVs[bufferIndex]);
    }

    currentBackBufferIndex = dxgiSwapChain->GetCurrentBackBufferIndex();
}

void D3D12SwapChain::Resize(uint32_t width, uint32_t height) {
    // 기존 스왑 체인 백버퍼 해제
    for (int bufferIndex = 0; bufferIndex < COUNT_OF(backBuffers); ++bufferIndex) {
        SAFE_RELEASE(backBuffers[bufferIndex]);
    }

    UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    if (D3D12Renderer::GetRenderer()->supportsTearing) {
        swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    this->width = width;
    this->height = height;

    // 스왑 체인 백버퍼의 사이즈를 조정한다.
    dxgiSwapChain->ResizeBuffers(D3D12SwapChain::NumSwapChainBuffers, width, height, swapChainFormat, swapChainFlags);

    // 백버퍼 리소스를 다시 가져오고 RTV 를 다시 생성한다.
    CreateRTVs();
}

void D3D12SwapChain::SwapBuffers(bool vsync) {
    const UINT syncInterval = vsync ? 1 : 0;
    const UINT presentFlags = (!vsync && D3D12Renderer::GetRenderer()->supportsTearing && d3d12_swapChainAllowTearing.GetBool()) ? DXGI_PRESENT_ALLOW_TEARING : 0;

    if (dxgiSwapChain->Present(syncInterval, presentFlags) == DXGI_ERROR_DEVICE_REMOVED) {
        BE_FATALERROR("DXGI Device Removed");
    }

    // 다음 프레임에 사용할 백버퍼 인덱스 얻어오기
    currentBackBufferIndex = dxgiSwapChain->GetCurrentBackBufferIndex();
}

RHI::Format D3D12SwapChain::GetFormat() const {
    return D3D12Renderer::DXGIFormatToRHIFormat(swapChainFormat);
}

RHI::Format D3D12SwapChain::GetRTVFormat() const {
    return D3D12Renderer::DXGIFormatToRHIFormat(rtvFormat);
}

bool D3D12SwapChain::GetDisplayInfo(RHI::DisplayInfo &displayInfo) const {
    return D3D12Renderer::GetRenderer()->GetDisplayInfo(hwnd, displayInfo);
}

bool D3D12Renderer::RefreshDXGIFactory() {
    // DXGI Factory가 보유한 Adapter/Output 열거 정보가 아직 최신인지 확인한다.
    if (dxgiFactory->IsCurrent()) {
        return true;
    }

    IDXGIFactory4 *newFactory = nullptr;
    const HRESULT hr = CreateDXGIFactory2(validationLayerEnabled ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&newFactory));
    if (FAILED(hr)) {
        BE_WARNLOG("D3D12Renderer::RefreshDXGIFactory: CreateDXGIFactory2 failed, ERROR: 0x%x\n", hr);
        return false;
    }

    SAFE_RELEASE(dxgiFactory);
    dxgiFactory = newFactory;
    return true;
}

bool D3D12Renderer::GetDisplayInfo(HWND hwnd, RHI::DisplayInfo &displayInfo) {
    displayInfo = {};

    if (!hwnd || !RefreshDXGIFactory()) {
        return false;
    }

    // Advanced Color display query: https://learn.microsoft.com/windows/win32/direct3darticles/high-dynamic-range
    // 갱신된 DXGI Factory 에서 윈도우가 가장 많이 걸쳐 있는 모니터의 출력 정보를 다시 찾는다.
    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor) {
        return false;
    }

    for (UINT adapterIndex = 0; ; ++adapterIndex) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        const HRESULT adapterResult = dxgiFactory->EnumAdapters1(adapterIndex, &adapter);
        if (adapterResult == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(adapterResult)) {
            return false;
        }

        for (UINT outputIndex = 0; ; ++outputIndex) {
            Microsoft::WRL::ComPtr<IDXGIOutput> output;
            const HRESULT outputResult = adapter->EnumOutputs(outputIndex, &output);
            if (outputResult == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            if (FAILED(outputResult)) {
                return false;
            }

            DXGI_OUTPUT_DESC outputDesc = {};
            if (FAILED(output->GetDesc(&outputDesc)) || outputDesc.Monitor != monitor) {
                continue;
            }

            Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
            if (FAILED(output.As(&output6))) {
                return false;
            }

            DXGI_OUTPUT_DESC1 outputDesc1 = {};
            if (FAILED(output6->GetDesc1(&outputDesc1))) {
                return false;
            }

            displayInfo.hdrActive = outputDesc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
            displayInfo.minLuminanceNits = outputDesc1.MinLuminance;
            displayInfo.maxLuminanceNits = outputDesc1.MaxLuminance;
            displayInfo.maxFullFrameLuminanceNits = outputDesc1.MaxFullFrameLuminance;
            return true;
        }
    }

    return false;
}

bool D3D12SwapChain::IsHDROutputAvailable() const {
    RHI::DisplayInfo displayInfo;
    return GetDisplayInfo(displayInfo) && displayInfo.hdrActive;
}

static DXGI_COLOR_SPACE_TYPE ToDXGIColorSpace(RHI::SwapChain::ColorSpace colorSpace) {
    switch (colorSpace) {
    case RHI::SwapChain::ColorSpace::BT2020PQ:
        return DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
    case RHI::SwapChain::ColorSpace::SRGB:
    default:
        return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    }
}

bool D3D12SwapChain::IsColorSpaceSupported(RHI::SwapChain::ColorSpace colorSpace) const {
    if (colorSpace == RHI::SwapChain::ColorSpace::DisplayP3LinearEDR) {
        return false;
    }
    if (colorSpace == RHI::SwapChain::ColorSpace::BT2020PQ && swapChainFormat != DXGI_FORMAT_R10G10B10A2_UNORM) {
        return false;
    }

    UINT colorSpaceSupport = 0;
    HRESULT hr = dxgiSwapChain->CheckColorSpaceSupport(ToDXGIColorSpace(colorSpace), &colorSpaceSupport);
    return SUCCEEDED(hr) && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) != 0;
}

bool D3D12SwapChain::SetColorSpace(RHI::SwapChain::ColorSpace colorSpace) {
    if (!IsColorSpaceSupported(colorSpace)) {
        return false;
    }

    HRESULT hr = dxgiSwapChain->SetColorSpace1(ToDXGIColorSpace(colorSpace));
    if (FAILED(hr)) {
        BE_WARNLOG("D3D12SwapChain::SetColorSpace: SetColorSpace1 failed, ERROR: 0x%x\n", hr);
        return false;
    }

    this->colorSpace = colorSpace;
    return true;
}
