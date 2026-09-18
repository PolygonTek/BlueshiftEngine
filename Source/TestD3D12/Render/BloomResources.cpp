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
#include "BloomResources.h"
#include "Render/RenderCvars.h"
#include "Render/RenderFrameSettings.h"
#include "Render/Texture/TextureManager.h"
#include "Shaders/ShaderInterop.h"

BloomResources::~BloomResources() {
    Destroy();
}

static uint32_t SelectBloomFFTSize(uint32_t renderWidth, uint32_t renderHeight, uint32_t maxSize) {
    if (renderWidth == 0 || renderHeight == 0) {
        return 0;
    }

    // Convolution bloom 은 절반 해상도로 prefilter 한 화면을 FFT 입력으로 사용하므로, FFT 크기도 절반 해상도의 긴 축을 기준으로 선택한다.
    const uint32_t inputWidth = renderWidth > 1 ? renderWidth >> 1 : 1;
    const uint32_t inputHeight = renderHeight > 1 ? renderHeight >> 1 : 1;
    const uint32_t inputMaxAxis = BE1::Max(inputWidth, inputHeight);

    uint32_t fftSize = 256;
    if (inputMaxAxis >= 1024) {
        fftSize = 1024;
    } else if (inputMaxAxis >= 512) {
        fftSize = 512;
    }

    return BE1::Min(fftSize, maxSize);
}

void BloomResources::Ensure(uint32_t renderWidth, uint32_t renderHeight, const RenderFrameSettings &renderFrameSettings) {
    const RenderBloomMode mode = renderFrameSettings.bloomMode;
    if (renderWidth == 0 || renderHeight == 0 || (mode != RenderBloomMode::Pyramid && mode != RenderBloomMode::Convolution)) {
        Destroy();
        return;
    }

    if (this->renderWidth != renderWidth || this->renderHeight != renderHeight) {
        Destroy();
        this->renderWidth = renderWidth;
        this->renderHeight = renderHeight;
    }

    if (mode == RenderBloomMode::Pyramid) {
        EnsurePyramidResources(renderFrameSettings.pyramidBloomMipMaxLevelCount);
    } else {
        EnsureConvolutionResources(renderFrameSettings.convolutionBloomFFTMaxSize);
    }
}

void BloomResources::EnsurePyramidResources(uint32_t requestedBloomMipMaxLevelCount) {
    DestroyConvolutionBloomResources();
    DestroyConvolutionBloomOutputTexture();

    const bool hasReusablePyramidBloomTextures = bloomDownTexture && bloomUpTexture && bloomMipCount > 0 && bloomMipMaxLevelCount == requestedBloomMipMaxLevelCount;

    if (!hasReusablePyramidBloomTextures) {
        if (!CreatePyramidBloomTextures(renderWidth, renderHeight, requestedBloomMipMaxLevelCount)) {
            DestroyPyramidBloomTextures();
        }
    }
}

void BloomResources::EnsureConvolutionResources(uint32_t maxFFTSize) {
    DestroyPyramidBloomTextures();

    const uint32_t requestedFFTBloomSize = SelectBloomFFTSize(renderWidth, renderHeight, maxFFTSize);
    const bool hasReusableFFTBloomResources =
        fftBloomScreenSpatialBuffer &&
        fftBloomScreenSpectrumBuffer &&
        fftBloomKernelSpatialBuffer &&
        fftBloomKernelSpectrumBuffer &&
        fftBloomWorkBuffer &&
        fftBloomSize == requestedFFTBloomSize;

    if (requestedFFTBloomSize == 0) {
        DestroyConvolutionBloomResources();
        DestroyConvolutionBloomOutputTexture();
    } else if (!hasReusableFFTBloomResources) {
        if (!CreateConvolutionBloomResources(requestedFFTBloomSize)) {
            DestroyConvolutionBloomResources();
            DestroyConvolutionBloomOutputTexture();
        }
    }

    const bool hasFFTBloomResources =
        fftBloomScreenSpatialBuffer &&
        fftBloomScreenSpectrumBuffer &&
        fftBloomKernelSpatialBuffer &&
        fftBloomKernelSpectrumBuffer &&
        fftBloomWorkBuffer &&
        fftBloomSize == requestedFFTBloomSize;

    if (hasFFTBloomResources) {
        const uint32_t requestedConvolutionBloomWidth = renderWidth > 1 ? renderWidth >> 1 : 1;
        const uint32_t requestedConvolutionBloomHeight = renderHeight > 1 ? renderHeight >> 1 : 1;
        const bool hasReusableConvolutionBloomTexture =
            fftBloomConvolutionOutputTexture &&
            fftBloomConvolutionOutputWidth == requestedConvolutionBloomWidth &&
            fftBloomConvolutionOutputHeight == requestedConvolutionBloomHeight;

        if (!hasReusableConvolutionBloomTexture) {
            if (!CreateConvolutionBloomOutputTexture(requestedConvolutionBloomWidth, requestedConvolutionBloomHeight)) {
                DestroyConvolutionBloomOutputTexture();
            }
        }

    } else {
        DestroyConvolutionBloomOutputTexture();
    }
}

void BloomResources::Destroy() {
    DestroyPyramidBloomTextures();
    DestroyConvolutionBloomResources();
    DestroyConvolutionBloomOutputTexture();
    renderWidth = 0;
    renderHeight = 0;
}

static void WriteConvolutionBloomKernelBoundsDebugImage(const BE1::Image &kernelImage, const BE1::Rect &bounds) {
    BE1::Image debugImage;
    if (!kernelImage.ConvertFormat(BE1::Image::Format::R32G32B32A32_FLOAT, debugImage, BE1::Image::GammaSpace::Linear)) {
        BE_WARNLOG("WriteConvolutionBloomKernelBoundsDebugImage: failed to convert kernel image\n");
        return;
    }

    const int width = debugImage.GetWidth();
    const int height = debugImage.GetHeight();
    float *debugPixels = reinterpret_cast<float *>(debugImage.GetPixels());
    const size_t debugPixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);

    float boundsColorScale = 1.0f;
    for (size_t pixelIndex = 0; pixelIndex < debugPixelCount; pixelIndex++) {
        const float *pixel = debugPixels + pixelIndex * 4;
        for (int componentIndex = 0; componentIndex < 3; componentIndex++) {
            if (std::isfinite(pixel[componentIndex])) {
                boundsColorScale = BE1::Max(boundsColorScale, pixel[componentIndex]);
            }
        }
    }

    auto SetBoundsPixel = [debugPixels, width, boundsColorScale](int x, int y) {
        float *pixel = debugPixels + (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * 4;
        pixel[0] = boundsColorScale;
        pixel[1] = 0.0f;
        pixel[2] = 0.0f;
        pixel[3] = 1.0f;
    };

    const int minX = bounds.x;
    const int minY = bounds.y;
    const int maxX = bounds.x + bounds.w - 1;
    const int maxY = bounds.y + bounds.h - 1;
    const int boundsThickness = BE1::Max(BE1::Min(width, height) / 512, 1);

    for (int thicknessOffset = 0; thicknessOffset < boundsThickness; thicknessOffset++) {
        const int x0 = minX + thicknessOffset;
        const int y0 = minY + thicknessOffset;
        const int x1 = maxX - thicknessOffset;
        const int y1 = maxY - thicknessOffset;
        if (x0 > x1 || y0 > y1) {
            break;
        }

        for (int x = x0; x <= x1; x++) {
            SetBoundsPixel(x, y0);
            SetBoundsPixel(x, y1);
        }
        for (int y = y0 + 1; y < y1; y++) {
            SetBoundsPixel(x0, y);
            SetBoundsPixel(x1, y);
        }
    }

    const BE1::Str documentDir = BE1::fileSystem.GetUserDocumentDir();
    char path[BE1::MaxAbsolutePath];
    BE1::Str::snPrintf(path, sizeof(path), "%s/Screenshots/convolution_bloom_kernel_bounds.hdr", documentDir.c_str());

    if (!debugImage.WriteHDR(path)) {
        BE_WARNLOG("WriteConvolutionBloomKernelBoundsDebugImage: failed to write \"%s\"\n", path);
        return;
    }

    BE_LOG("Convolution bloom kernel bounds debug image saved to \"%s\"\n", path);
}

// 원본 컨볼루션 커널 이미지에서 normalized extents 와 평균 luminance 의 역수를 계산한다.
void BloomResources::CalcConvolutionBloomKernelParams(const BE1::Image &kernelImage, BE1::Vec2 &outNormalizedKernelExtents, float &outInverseAverageLuminance) {
    outInverseAverageLuminance = 1.0f;
    outNormalizedKernelExtents = BE1::Vec2(1.0f, 1.0f);

    const int width = kernelImage.GetWidth();
    const int height = kernelImage.GetHeight();

    if (kernelImage.IsEmpty() || width <= 0 || height <= 0) {
        return;
    }

    // 원본 kernel 이미지의 픽셀 개수
    const float pixelCount = static_cast<float>(width * height);

    // 전체 luminance 에너지의 99.98% 이상을 보존하는 대칭 바운딩 박스를 계산한다.
    static constexpr float retainedKernelEnergyFraction = 0.9998f;
    float luminanceSum;
    BE1::Rect bounds;
    if (!kernelImage.ComputeLuminanceEnergyBounds(0, 0, retainedKernelEnergyFraction, true, bounds, luminanceSum)) {
        return;
    }

    if (!std::isfinite(luminanceSum) || luminanceSum <= 0.000001f) {
        return;
    }

    const float averageLuminance = luminanceSum / pixelCount;

    if (r_convolutionBloomDebug.GetBool()) {
        // 계산된 kernel bounds 를 원본 HDR 범위를 유지한 채 확인할 수 있도록 디버그 이미지에 표시한다.
        WriteConvolutionBloomKernelBoundsDebugImage(kernelImage, bounds);
    }

    const float centerX = static_cast<float>(width) * 0.5f;
    const float centerY = static_cast<float>(height) * 0.5f;
    const float minX = (static_cast<float>(bounds.x) + 0.5f - centerX) / centerX;
    const float minY = (static_cast<float>(bounds.y) + 0.5f - centerY) / centerY;
    const float maxX = (static_cast<float>(bounds.x + bounds.w - 1) + 0.5f - centerX) / centerX;
    const float maxY = (static_cast<float>(bounds.y + bounds.h - 1) + 0.5f - centerY) / centerY;

    // 한 texel 여유를 준다.
    const float texelSizeX = 1.0f / static_cast<float>(width);
    const float texelSizeY = 1.0f / static_cast<float>(height);

    const float kernelExtentX = BE1::AbsMax(minX, maxX) + texelSizeX;
    const float kernelExtentY = BE1::AbsMax(minY, maxY) + texelSizeY;

    // normalized extents 를 계산한다.
    outNormalizedKernelExtents = BE1::Vec2(BE1::Min(kernelExtentX, 1.0f), BE1::Min(kernelExtentY, 1.0f));

    // 평균 luminance 의 역수
    outInverseAverageLuminance = 1.0f / averageLuminance;
}

bool BloomResources::SetConvolutionBloomKernelFromImage(const char *textureName, const BE1::Image &kernelImage) {
    if (kernelImage.IsEmpty()) {
        return false;
    }
    if (textureManager.FindTexture(textureName)) {
        BE_WARNLOG("BloomResources::SetConvolutionBloomKernelFromImage: texture '%s' already exists\n", textureName);
        return false;
    }

    BE1::Vec2 normalizedKernelExtents;
    float inverseAverageLuminance;
    CalcConvolutionBloomKernelParams(kernelImage, normalizedKernelExtents, inverseAverageLuminance);

    SharedPtr<BE1::Image> textureImage = MakeShared<BE1::Image>(kernelImage);
    SharedPtr<Texture> texture = textureManager.AllocTexture(textureName);
    if (!texture->Create(RHI::TextureType::Texture2D, textureImage, Texture::Flag::HighQuality)) {
        return false;
    }

    SetConvolutionBloomKernelTexture(texture, normalizedKernelExtents, inverseAverageLuminance);
    return true;
}

bool BloomResources::SetConvolutionBloomKernelFromFile(const char *filename) {
    if (!filename || !filename[0]) {
        return false;
    }

    BE1::Image kernelImage = BE1::Image::ImageFromFile(filename);
    if (kernelImage.IsEmpty()) {
        return false;
    }

    BE1::Vec2 normalizedKernelExtents;
    float inverseAverageLuminance;
    CalcConvolutionBloomKernelParams(kernelImage, normalizedKernelExtents, inverseAverageLuminance);

    SharedPtr<Texture> texture = textureManager.GetTexture(filename, Texture::Flag::HighQuality);
    if (!texture) {
        return false;
    }

    SetConvolutionBloomKernelTexture(texture, normalizedKernelExtents, inverseAverageLuminance);
    return true;
}

void BloomResources::SetConvolutionBloomKernelTexture(const SharedPtr<Texture> &texture, const BE1::Vec2 &normalizedKernelExtents, float inverseAverageLuminance) {
    if (fftBloomKernelSourceTexture != texture) {
        fftBloomKernelSourceTexture = texture;
        fftBloomKernelDirty = true;
    }
    if (fftBloomKernelNormalizedExtents != normalizedKernelExtents) {
        fftBloomKernelNormalizedExtents = normalizedKernelExtents;
        fftBloomKernelDirty = true;
    }
    fftBloomKernelInverseAverageLuminance = inverseAverageLuminance;
}

void BloomResources::SetConvolutionBloomKernelRotation(float rotation) {
    if (fftBloomKernelRotation == rotation) {
        return;
    }

    fftBloomKernelRotation = rotation;
    fftBloomKernelDirty = true;
}

void BloomResources::SetConvolutionBloomKernelScale(float scale) {
    scale = BE1::Max(scale, 0.01f);
    if (fftBloomKernelScale == scale) {
        return;
    }

    fftBloomKernelScale = scale;
    fftBloomKernelDirty = true;
}

void BloomResources::SetConvolutionBloomPaddingTrim(float paddingTrim) {
    paddingTrim = BE1::Min(BE1::Max(paddingTrim, 0.0f), 0.5f);
    if (fftBloomPaddingTrim == paddingTrim) {
        return;
    }

    fftBloomPaddingTrim = paddingTrim;
    fftBloomKernelDirty = true;
}

BE1::Vec2 BloomResources::CalcNormalizedKernelExtents() const {
    // 커널의 회전 각도에 따른 sin, cos 계산
    float rotationSin;
    float rotationCos;
    BE1::Math::SinCos(DEG2RAD(fftBloomKernelRotation), rotationSin, rotationCos);

    const float absCos = BE1::Math::Abs(rotationCos);
    const float absSin = BE1::Math::Abs(rotationSin);

    // 외부에서 임의로 입력한 커널의 회전과 스케일이 고려된 normalized kernel extent 계산하기
    BE1::Vec2 normalizedKernelExtents = fftBloomKernelNormalizedExtents;
    const float scaledKernelExtentX = normalizedKernelExtents.x * fftBloomKernelScale;
    const float scaledKernelExtentY = normalizedKernelExtents.y * fftBloomKernelScale;
    normalizedKernelExtents.x = scaledKernelExtentX * absCos + scaledKernelExtentY * absSin;
    normalizedKernelExtents.y = scaledKernelExtentX * absSin + scaledKernelExtentY * absCos;

    return normalizedKernelExtents;
}

// convolution 을 위한 zero 패딩을 포함한 커널과 컨텐츠가 fftBloomSize x fftBloomSize 로 고정된 상황에서
// 그에 맞춰서 역으로 content 와 kernel 을 얼마나 줄여야 하는지를 계산한다.
void BloomResources::UpdateConvolutionBloomMapping() {
    if (renderWidth == 0 || renderHeight == 0 || fftBloomSize == 0) {
        fftBloomMappedContentRect = BE1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
        fftBloomMappedKernelScale = BE1::Vec2(1.0f, 1.0f);
        return;
    }

    // normalized kernel extent 계산
    const BE1::Vec2 normalizedKernelExtents = CalcNormalizedKernelExtents();

    // content 의 aspect ratio 를 유지하면서 fftBloomSize x fftBloomSize 안에 fit 된 크기 계산
    float aspectFitContentW;
    float aspectFitContentH;
    if (renderWidth >= renderHeight) {
        aspectFitContentW = static_cast<float>(fftBloomSize);
        aspectFitContentH = static_cast<float>(fftBloomSize * renderHeight) / static_cast<float>(renderWidth);
    } else {
        aspectFitContentW = static_cast<float>(fftBloomSize * renderWidth) / static_cast<float>(renderHeight);
        aspectFitContentH = static_cast<float>(fftBloomSize);
    }

    // normalized content extent 계산
    const float normalizedContentExtentX = aspectFitContentW / static_cast<float>(fftBloomSize);
    const float normalizedContentExtentY = aspectFitContentH / static_cast<float>(fftBloomSize);

    // support extent (= content extent + kernel extent) 계산
    const float supportExtentX = normalizedContentExtentX + normalizedKernelExtents.x;
    const float supportExtentY = normalizedContentExtentY + normalizedKernelExtents.y;

    // supportExtents 가 normalized [0, 1] 영역 내부로 들어오도록 scale 을 계산한다.
    // paddingTrim 만큼 normalized 영역 밖으로 support 가 나가는 것을 허용한다.
    const float allowedSupportExtent = 1.0f + fftBloomPaddingTrim;
    const float convolutionExtentX = BE1::Max(supportExtentX, allowedSupportExtent);
    const float convolutionExtentY = BE1::Max(supportExtentY, allowedSupportExtent);
    const float scaleX = allowedSupportExtent / convolutionExtentX;
    const float scaleY = allowedSupportExtent / convolutionExtentY;

    // 컨벌루션에 필요한 패딩을 고려한 실제 content 크기와 오프셋을 계산한다.
    const float contentW = BE1::Max(aspectFitContentW * scaleX, 1.0f);
    const float contentH = BE1::Max(aspectFitContentH * scaleY, 1.0f);
    const float contentX = (static_cast<float>(fftBloomSize) - contentW) * 0.5f;
    const float contentY = (static_cast<float>(fftBloomSize) - contentH) * 0.5f;
    fftBloomMappedContentRect = BE1::RectF(contentX, contentY, contentW, contentH);

    // 컨벌루션에 필요한 패딩을 고려한 커널의 축소 스케일을 계산한다.
    fftBloomMappedKernelScale.x = (contentW / aspectFitContentW) * fftBloomKernelScale;
    fftBloomMappedKernelScale.y = (contentH / aspectFitContentH) * fftBloomKernelScale;
}

bool BloomResources::CreatePyramidBloomTextures(uint32_t width, uint32_t height, uint32_t requestedBloomMipMaxLevelCount) {
    DestroyPyramidBloomTextures();

    if (width == 0 || height == 0) {
        return false;
    }

    const BE1::Image::Format bloomImageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R11G11B10_FLOAT);
    const RHI::TextureFlag bloomTextureFlags = RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess | RHI::TextureFlag::SkipDefaultViews;

    // bloom texture 의 mip 0 는 카메라 렌더 타겟의 절반 크기로 시작
    uint32_t bloomWidth = width > 1 ? width >> 1 : 1;
    uint32_t bloomHeight = height > 1 ? height >> 1 : 1;
    uint32_t levelCount = 0;

    // 1x1 이 될 때까지 몇 개의 bloom down mip 이 필요한지 계산한다. (최대 RenderBloomDownMaxLevelCount)
    while (levelCount < requestedBloomMipMaxLevelCount) {
        bloomMipWidths[levelCount] = bloomWidth;
        bloomMipHeights[levelCount] = bloomHeight;
        levelCount++;

        if (bloomWidth == 1 && bloomHeight == 1) {
            break;
        }

        bloomWidth = bloomWidth > 1 ? bloomWidth >> 1 : 1;
        bloomHeight = bloomHeight > 1 ? bloomHeight >> 1 : 1;
    }

    if (levelCount < 2) {
        return false;
    }

    const uint32_t bloomDownMipCount = levelCount;
    const uint32_t bloomUpMipCount = levelCount - 1;

    SharedPtr<BE1::Image> bloomDownImage = MakeShared<BE1::Image>();
    bloomDownImage->InitFromMemory(bloomMipWidths[0], bloomMipHeights[0], 1, 1, bloomDownMipCount, bloomImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    bloomDownTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, bloomTextureFlags, bloomDownImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!bloomDownTexture) {
        BE_WARNLOG("BloomResources::CreatePyramidBloomTextures: failed to create bloom down texture\n");
        return false;
    }
    RHI::renderer->SetName(bloomDownTexture, "BloomResources::bloomDownTexture");

    // bloomUpTexture 는 upsample pass 의 출력 mip 만 필요하므로 가장 작은 source-only mip 은 만들지 않는다.
    SharedPtr<BE1::Image> bloomUpImage = MakeShared<BE1::Image>();
    bloomUpImage->InitFromMemory(bloomMipWidths[0], bloomMipHeights[0], 1, 1, bloomUpMipCount, bloomImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    bloomUpTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, bloomTextureFlags, bloomUpImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!bloomUpTexture) {
        BE_WARNLOG("BloomResources::CreatePyramidBloomTextures: failed to create bloom up texture\n");
        DestroyPyramidBloomTextures();
        return false;
    }
    RHI::renderer->SetName(bloomUpTexture, "BloomResources::bloomUpTexture");

    auto createBloomMipViews = [](RHI::Texture *texture, RHI::ViewID *srvViewIDs, RHI::ViewID *uavViewIDs, uint32_t levelCount, const char *debugName) -> bool {
        for (uint32_t mipLevel = 0; mipLevel < levelCount; ++mipLevel) {
            srvViewIDs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::SRV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
            if (!srvViewIDs[mipLevel].IsValid()) {
                BE_WARNLOG("BloomResources::CreatePyramidBloomTextures: failed to create %s mip %i SRV\n", debugName, mipLevel);
                return false;
            }

            uavViewIDs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::UAV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
            if (!uavViewIDs[mipLevel].IsValid()) {
                BE_WARNLOG("BloomResources::CreatePyramidBloomTextures: failed to create %s mip %i UAV\n", debugName, mipLevel);
                return false;
            }
        }

        return true;
    };

    if (!createBloomMipViews(bloomDownTexture, bloomDownMipSRVs, bloomDownMipUAVs, bloomDownMipCount, "bloomDownTexture") ||
        !createBloomMipViews(bloomUpTexture, bloomUpMipSRVs, bloomUpMipUAVs, bloomUpMipCount, "bloomUpTexture")) {
        DestroyPyramidBloomTextures();
        return false;
    }

    bloomMipCount = levelCount;
    bloomMipMaxLevelCount = requestedBloomMipMaxLevelCount;

    return true;
}

void BloomResources::DestroyPyramidBloomTextures() {
    if (bloomDownTexture) {
        RHI::renderer->DestroyTexture(bloomDownTexture);
        bloomDownTexture = nullptr;
    }
    if (bloomUpTexture) {
        RHI::renderer->DestroyTexture(bloomUpTexture);
        bloomUpTexture = nullptr;
    }

    for (uint32_t mipLevel = 0; mipLevel < RenderBloomDownMaxLevelCount; ++mipLevel) {
        bloomDownMipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        bloomDownMipUAVs[mipLevel] = RHI::ViewID::InvalidID;
        bloomMipWidths[mipLevel] = 0;
        bloomMipHeights[mipLevel] = 0;
    }
    for (uint32_t mipLevel = 0; mipLevel < RenderBloomUpMaxLevelCount; ++mipLevel) {
        bloomUpMipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        bloomUpMipUAVs[mipLevel] = RHI::ViewID::InvalidID;
    }

    bloomMipCount = 0;
    bloomMipMaxLevelCount = 0;
}

static RHI::Buffer *CreateFFTBloomStructuredBuffer(const char *debugName, uint64_t elementCount) {
    const uint64_t bufferSize = elementCount * sizeof(float2);
    RHI::Buffer *buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, bufferSize, nullptr);
    if (!buffer) {
        BE_WARNLOG("BloomResources::CreateConvolutionBloomResources: failed to create %s\n", debugName);
        return nullptr;
    }

    RHI::renderer->SetName(buffer, debugName);

    RHI::ViewID srvViewID = RHI::renderer->CreateView(buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(float2), 0, bufferSize);
    if (!srvViewID.IsValid()) {
        BE_WARNLOG("BloomResources::CreateConvolutionBloomResources: failed to create %s SRV\n", debugName);
        RHI::renderer->DestroyBuffer(buffer);
        return nullptr;
    }

    RHI::ViewID uavViewID = RHI::renderer->CreateView(buffer, RHI::ViewType::UAV, RHI::Format::Unknown, sizeof(float2), 0, bufferSize);
    if (!uavViewID.IsValid()) {
        BE_WARNLOG("BloomResources::CreateConvolutionBloomResources: failed to create %s UAV\n", debugName);
        RHI::renderer->DestroyBuffer(buffer);
        return nullptr;
    }

    return buffer;
}

bool BloomResources::CreateConvolutionBloomResources(uint32_t size) {
    DestroyConvolutionBloomResources();

    if (size == 0) {
        return false;
    }

    // RG 배열 이후에 B0 배열을 담는다.
    constexpr uint64_t ColorPairCount = 2; // RG, B0
    const uint64_t colorPairElementCount = uint64_t(size) * uint64_t(size) * ColorPairCount;

    fftBloomScreenSpatialBuffer = CreateFFTBloomStructuredBuffer("BloomResources::fftBloomScreenSpatialBuffer", colorPairElementCount);
    if (!fftBloomScreenSpatialBuffer) {
        return false;
    }

    fftBloomScreenSpectrumBuffer = CreateFFTBloomStructuredBuffer("BloomResources::fftBloomScreenSpectrumBuffer", colorPairElementCount);
    if (!fftBloomScreenSpectrumBuffer) {
        DestroyConvolutionBloomResources();
        return false;
    }

    fftBloomKernelSpatialBuffer = CreateFFTBloomStructuredBuffer("BloomResources::fftBloomKernelSpatialBuffer", colorPairElementCount);
    if (!fftBloomKernelSpatialBuffer) {
        DestroyConvolutionBloomResources();
        return false;
    }

    fftBloomKernelSpectrumBuffer = CreateFFTBloomStructuredBuffer("BloomResources::fftBloomKernelSpectrumBuffer", colorPairElementCount);
    if (!fftBloomKernelSpectrumBuffer) {
        DestroyConvolutionBloomResources();
        return false;
    }

    fftBloomWorkBuffer = CreateFFTBloomStructuredBuffer("BloomResources::fftBloomWorkBuffer", colorPairElementCount);
    if (!fftBloomWorkBuffer) {
        DestroyConvolutionBloomResources();
        return false;
    }

    fftBloomSize = size;
    fftBloomKernelDirty = true;

    return true;
}

void BloomResources::DestroyConvolutionBloomResources() {
    if (fftBloomScreenSpatialBuffer) {
        RHI::renderer->DestroyBuffer(fftBloomScreenSpatialBuffer);
        fftBloomScreenSpatialBuffer = nullptr;
    }
    if (fftBloomScreenSpectrumBuffer) {
        RHI::renderer->DestroyBuffer(fftBloomScreenSpectrumBuffer);
        fftBloomScreenSpectrumBuffer = nullptr;
    }
    if (fftBloomKernelSpatialBuffer) {
        RHI::renderer->DestroyBuffer(fftBloomKernelSpatialBuffer);
        fftBloomKernelSpatialBuffer = nullptr;
    }
    if (fftBloomKernelSpectrumBuffer) {
        RHI::renderer->DestroyBuffer(fftBloomKernelSpectrumBuffer);
        fftBloomKernelSpectrumBuffer = nullptr;
    }
    if (fftBloomWorkBuffer) {
        RHI::renderer->DestroyBuffer(fftBloomWorkBuffer);
        fftBloomWorkBuffer = nullptr;
    }
    fftBloomSize = 0;
    fftBloomMappedContentRect = BE1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
    fftBloomMappedKernelScale = BE1::Vec2(1.0f, 1.0f);
    fftBloomKernelDirty = true;
}

bool BloomResources::CreateConvolutionBloomOutputTexture(uint32_t width, uint32_t height) {
    DestroyConvolutionBloomOutputTexture();

    if (width == 0 || height == 0) {
        return false;
    }

    const BE1::Image::Format imageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R11G11B10_FLOAT);
    const uint32_t mipLevelCount = static_cast<uint32_t>(BE1::Min(BE1::Image::MaxMipLevels(width, height, 1), RenderConvolutionBloomMaxMipLevelCount));

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->InitFromMemory(width, height, 1, 1, mipLevelCount, imageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    fftBloomConvolutionOutputTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, image, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!fftBloomConvolutionOutputTexture) {
        BE_WARNLOG("BloomResources::CreateConvolutionBloomOutputTexture: failed to create convolution texture\n");
        return false;
    }

    RHI::renderer->SetName(fftBloomConvolutionOutputTexture, "BloomResources::fftBloomConvolutionOutputTexture");

    for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel) {
        fftBloomConvolutionOutputMipSRVs[mipLevel] = RHI::renderer->CreateView(fftBloomConvolutionOutputTexture, RHI::ViewType::SRV, RHI::Format::Auto, 0, 1, mipLevel, 1);
        fftBloomConvolutionOutputMipUAVs[mipLevel] = RHI::renderer->CreateView(fftBloomConvolutionOutputTexture, RHI::ViewType::UAV, RHI::Format::Auto, 0, 1, mipLevel, 1);
        if (!fftBloomConvolutionOutputMipSRVs[mipLevel].IsValid() || !fftBloomConvolutionOutputMipUAVs[mipLevel].IsValid()) {
            BE_WARNLOG("BloomResources::CreateConvolutionBloomOutputTexture: failed to create mip %u views\n", mipLevel);
            DestroyConvolutionBloomOutputTexture();
            return false;
        }
    }

    fftBloomConvolutionOutputWidth = width;
    fftBloomConvolutionOutputHeight = height;
    fftBloomConvolutionOutputMipCount = mipLevelCount;

    return true;
}

void BloomResources::DestroyConvolutionBloomOutputTexture() {
    if (fftBloomConvolutionOutputTexture) {
        RHI::renderer->DestroyTexture(fftBloomConvolutionOutputTexture);
        fftBloomConvolutionOutputTexture = nullptr;
    }

    fftBloomConvolutionOutputWidth = 0;
    fftBloomConvolutionOutputHeight = 0;
    fftBloomConvolutionOutputMipCount = 0;
    for (uint32_t mipLevel = 0; mipLevel < RenderConvolutionBloomMaxMipLevelCount; ++mipLevel) {
        fftBloomConvolutionOutputMipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        fftBloomConvolutionOutputMipUAVs[mipLevel] = RHI::ViewID::InvalidID;
    }
}
