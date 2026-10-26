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

#include "RHI/RHI.h"

class Texture;
struct RenderFrameSettings;

static constexpr int RenderBloomDownMaxLevelCount = 8;
static constexpr int RenderBloomUpMaxLevelCount = RenderBloomDownMaxLevelCount - 1;
static constexpr int RenderConvolutionBloomMaxMipLevelCount = 16;

class BloomResources {
public:
    BloomResources() = default;
    ~BloomResources();
    BloomResources(const BloomResources &) = delete;
    BloomResources &operator=(const BloomResources &) = delete;

    void                        Ensure(uint32_t renderWidth, uint32_t renderHeight, const RenderFrameSettings &renderFrameSettings);
    void                        Destroy();

                                /// Calculates the effective normalized extents and inverse average luminance of a convolution bloom kernel image.
    static void                 CalcConvolutionBloomKernelParams(const BE1::Image &kernelImage, BE1::Vec2 &outNormalizedKernelExtents, float &inverseAverageLuminance);

    bool                        SetConvolutionBloomKernelFromImage(const char *textureName, const BE1::Image &kernelImage);
    bool                        SetConvolutionBloomKernelFromFile(const char *filename);
    void                        SetConvolutionBloomKernelTexture(SharedPtr<Texture> texture, const BE1::Vec2 &normalizedKernelExtents, float inverseAverageLuminance);
    void                        SetBloomDirtMaskTexture(SharedPtr<Texture> texture) { bloomDirtMaskTexture = std::move(texture); }
    void                        SetConvolutionBloomKernelRotation(float rotation);
    void                        SetConvolutionBloomKernelScale(float scale);
    void                        SetConvolutionBloomPaddingTrim(float paddingTrim);

    RHI::Texture *              GetBloomDownTexture() const { return bloomDownTexture; }
    RHI::Texture *              GetBloomUpTexture() const { return bloomUpTexture; }
    const RHI::ViewID *         GetBloomDownMipSRVs() const { return bloomDownMipSRVs; }
    const RHI::ViewID *         GetBloomDownMipUAVs() const { return bloomDownMipUAVs; }
    const RHI::ViewID *         GetBloomUpMipSRVs() const { return bloomUpMipSRVs; }
    const RHI::ViewID *         GetBloomUpMipUAVs() const { return bloomUpMipUAVs; }
    const uint32_t *            GetBloomMipWidths() const { return bloomMipWidths; }
    const uint32_t *            GetBloomMipHeights() const { return bloomMipHeights; }
    uint32_t                    GetBloomMipCount() const { return bloomMipCount; }
    const SharedPtr<Texture> &  GetBloomDirtMaskTexture() const { return bloomDirtMaskTexture; }

    uint32_t                    GetFFTBloomSize() const { return fftBloomSize; }
    RHI::Buffer *               GetFFTBloomScreenSpatialBuffer() const { return fftBloomScreenSpatialBuffer; }
    RHI::Buffer *               GetFFTBloomScreenSpectrumBuffer() const { return fftBloomScreenSpectrumBuffer; }
    const SharedPtr<Texture> &  GetFFTBloomKernelTexture() const { return fftBloomKernelSourceTexture; }
    float                       GetFFTBloomKernelInverseAverageLuminance() const { return fftBloomKernelInverseAverageLuminance; }
    const BE1::Vec2 &           GetFFTBloomKernelNormalizedExtents() const { return fftBloomKernelNormalizedExtents; }
    RHI::Buffer *               GetFFTBloomKernelSpatialBuffer() const { return fftBloomKernelSpatialBuffer; }
    RHI::Buffer *               GetFFTBloomKernelSpectrumBuffer() const { return fftBloomKernelSpectrumBuffer; }
    RHI::Buffer *               GetFFTBloomWorkBuffer() const { return fftBloomWorkBuffer; }
    RHI::Texture *              GetFFTBloomConvolutionOutputTexture() const { return fftBloomConvolutionOutputTexture; }
    uint32_t                    GetFFTBloomConvolutionOutputWidth() const { return fftBloomConvolutionOutputWidth; }
    uint32_t                    GetFFTBloomConvolutionOutputHeight() const { return fftBloomConvolutionOutputHeight; }
    const RHI::ViewID *         GetFFTBloomConvolutionOutputMipSRVs() const { return fftBloomConvolutionOutputMipSRVs; }
    const RHI::ViewID *         GetFFTBloomConvolutionOutputMipUAVs() const { return fftBloomConvolutionOutputMipUAVs; }
    uint32_t                    GetFFTBloomConvolutionOutputMipCount() const { return fftBloomConvolutionOutputMipCount; }
    bool                        IsFFTBloomKernelDirty() const { return fftBloomKernelDirty; }
    float                       GetFFTBloomKernelRotation() const { return fftBloomKernelRotation; }
    float                       GetFFTBloomKernelScale() const { return fftBloomKernelScale; }
    float                       GetFFTBloomPaddingTrim() const { return fftBloomPaddingTrim; }
    const BE1::RectF &          GetFFTBloomMappedContentRect() const { return fftBloomMappedContentRect; }
    const BE1::Vec2 &           GetFFTBloomMappedKernelScale() const { return fftBloomMappedKernelScale; }

    void                        UpdateConvolutionBloomMapping();
    void                        MarkConvolutionBloomKernelClean() { fftBloomKernelDirty = false; }

private:
    void                        EnsurePyramidResources(uint32_t requestedBloomMipMaxLevelCount);
    void                        EnsureConvolutionResources(uint32_t maxFFTSize);
    bool                        CreatePyramidBloomTextures(uint32_t width, uint32_t height, uint32_t requestedBloomMipMaxLevelCount);
    void                        DestroyPyramidBloomTextures();
    bool                        CreateConvolutionBloomResources(uint32_t size);
    void                        DestroyConvolutionBloomResources();
    bool                        CreateConvolutionBloomOutputTexture(uint32_t width, uint32_t height);
    void                        DestroyConvolutionBloomOutputTexture();
    BE1::Vec2                   CalcNormalizedKernelExtents() const;

    uint32_t                    renderWidth = 0;
    uint32_t                    renderHeight = 0;

    RHI::Texture *              bloomDownTexture = nullptr;
    RHI::Texture *              bloomUpTexture = nullptr;
    uint32_t                    bloomMipWidths[RenderBloomDownMaxLevelCount] = {};
    uint32_t                    bloomMipHeights[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomDownMipSRVs[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomDownMipUAVs[RenderBloomDownMaxLevelCount] = {};
    RHI::ViewID                 bloomUpMipSRVs[RenderBloomUpMaxLevelCount] = {};
    RHI::ViewID                 bloomUpMipUAVs[RenderBloomUpMaxLevelCount] = {};
    uint32_t                    bloomMipCount = 0;
    uint32_t                    bloomMipMaxLevelCount = 0;
    SharedPtr<Texture>          bloomDirtMaskTexture;

    uint32_t                    fftBloomSize = 0;
    SharedPtr<Texture>          fftBloomKernelSourceTexture;
    float                       fftBloomKernelInverseAverageLuminance = 1.0f;
    BE1::Vec2                   fftBloomKernelNormalizedExtents = BE1::Vec2(1.0f, 1.0f);
    RHI::Buffer *               fftBloomKernelSpatialBuffer = nullptr;
    RHI::Buffer *               fftBloomKernelSpectrumBuffer = nullptr;
    BE1::RectF                  fftBloomMappedContentRect = BE1::RectF(0.0f, 0.0f, 0.0f, 0.0f); // fftBloomSize x fftBloomSize 공간에서 convolution padding 을 확보한 뒤 content 가 배치되는 픽셀 단위 영역
    BE1::Vec2                   fftBloomMappedKernelScale = BE1::Vec2(1.0f, 1.0f); // content padding 에 따른 축소율과 fftBloomKernelScale 을 반영해 FFT 공간의 kernel 에 적용할 최종 X/Y scale
    float                       fftBloomKernelRotation = 0.0f;
    float                       fftBloomKernelScale = 1.0f;
    float                       fftBloomPaddingTrim = 0.0f;
    bool                        fftBloomKernelDirty = true;
    RHI::Buffer *               fftBloomScreenSpatialBuffer = nullptr;
    RHI::Buffer *               fftBloomScreenSpectrumBuffer = nullptr;
    RHI::Buffer *               fftBloomWorkBuffer = nullptr;
    RHI::Texture *              fftBloomConvolutionOutputTexture = nullptr;
    uint32_t                    fftBloomConvolutionOutputWidth = 0;
    uint32_t                    fftBloomConvolutionOutputHeight = 0;
    RHI::ViewID                 fftBloomConvolutionOutputMipSRVs[RenderConvolutionBloomMaxMipLevelCount] = {};
    RHI::ViewID                 fftBloomConvolutionOutputMipUAVs[RenderConvolutionBloomMaxMipLevelCount] = {};
    uint32_t                    fftBloomConvolutionOutputMipCount = 0;
};
