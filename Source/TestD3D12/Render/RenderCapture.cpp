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
#include "RenderCapture.h"
#include "RenderCamera.h"
#include "RenderContext.h"
#include "RenderSystem.h"
#include "RenderUtils.h"
#include "RenderWorld.h"
#include "Render/Texture/Texture.h"

static void SetupCaptureCameraDesc(RenderCamera::Desc &cameraDesc, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fovX, float fovY, float zNear, float zFar, uint32_t width, uint32_t height, RHI::Texture *outputTexture, RHI::ViewID outputViewID) {
    cameraDesc.flags = RenderCamera::Flag::SkipDebugDraw | RenderCamera::Flag::SkipPostProcess | RenderCamera::Flag::DisableTemporalHistory | RenderCamera::Flag::DisableHiZCulling | RenderCamera::Flag::ForceSyncPassPSOBuild;
    if (staticOnly) {
        cameraDesc.flags |= RenderCamera::Flag::StaticOnly;
    }
    cameraDesc.layerMask = layerMask;
    cameraDesc.outputRect.Set(0, 0, static_cast<int>(width), static_cast<int>(height));
    cameraDesc.outputTexture = outputTexture;
    cameraDesc.outputViewID = outputViewID;
    cameraDesc.outputBeforeState = RHI::GPUResourceState::ShaderResource;
    cameraDesc.outputAfterState = RHI::GPUResourceState::ShaderResource;
    cameraDesc.renderWidth = width;
    cameraDesc.renderHeight = height;
    cameraDesc.origin = origin;
    cameraDesc.axis = axis;
    cameraDesc.clearMethod = colorClear ? RenderCamera::ClearMethod::Color : RenderCamera::ClearMethod::Skybox;
    cameraDesc.clearColorMode = RenderCamera::ClearColorMode::Explicit;
    cameraDesc.clearColor = clearColor;
    cameraDesc.fovX = fovX;
    cameraDesc.fovY = fovY;
    cameraDesc.zNear = zNear;
    cameraDesc.zFar = zFar;
    cameraDesc.projectionPolicy = RHI::renderer->GetProjectionPolicy();
}

static bool BeginCaptureFrame(RenderContext *renderContext, RenderWorld *renderWorld, const SharedPtr<Texture> &targetTexture, const char *captureName) {
    if (RenderContext::activeContext) {
        BE_WARNLOG("%s: cannot capture while a render frame is active\n", captureName);
        return false;
    }

    RenderContext::FrameOptions frameOptions;
    if (!renderContext->BeginFrame(frameOptions)) {
        BE_WARNLOG("%s: failed to begin an offscreen render frame\n", captureName);
        return false;
    }

    // 렌더링 완료 전까지 출력 텍스처의 수명을 보장한다.
    renderContext->GetCurrentFrontendFrameData()->RetainTexture(targetTexture);

    renderWorld->BeginFrame();
    return true;
}

static bool EndCaptureFrame(RenderContext *renderContext, RenderWorld *renderWorld, bool waitForCompletion, const char *captureName) {
    // EndFrame 이후 완료를 기다릴 캡처 프레임을 보관한다.
    const RenderFrameData *captureFrameData = waitForCompletion ? renderContext->GetCurrentFrontendFrameData() : nullptr;

    renderWorld->EndFrame();
    renderContext->EndFrame();

    if (captureFrameData && !renderSystem->WaitFrameRenderCompleted(captureFrameData)) {
        BE_WARNLOG("%s: failed to wait for capture frame submission\n", captureName);
        return false;
    }
    return true;
}

bool RenderCapture::CaptureSceneToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, int width, int height, const SharedPtr<Texture> &targetTexture, bool waitForCompletion) {
    assert(BE1::Engine::IsInMainThread());

    if (!targetTexture || !targetTexture->IsReady() || width <= 0 || height <= 0) {
        BE_WARNLOG("RenderCapture::CaptureSceneToTexture: invalid capture parameters\n");
        return false;
    }

    RHI::Texture *targetRHITexture = targetTexture->GetRHITexture();
    RHI::TextureViewInfo targetViewInfo;
    if (!targetRHITexture || targetRHITexture->type != RHI::TextureType::Texture2D ||
        !targetRHITexture->GetViewInfo(RHI::ViewType::RTV, RHI::ViewID::PrimaryID, targetViewInfo)) {
        BE_WARNLOG("RenderCapture::CaptureSceneToTexture: target texture has no valid 2D render target view\n");
        return false;
    }

    if (static_cast<uint32_t>(width) > targetViewInfo.width || static_cast<uint32_t>(height) > targetViewInfo.height) {
        BE_WARNLOG("RenderCapture::CaptureSceneToTexture: capture dimensions exceed the target texture view\n");
        return false;
    }

    RenderCamera renderCamera;
    RenderCamera::Desc cameraDesc;
    float fovX;
    float fovY;
    RenderCamera::CalculateFov(fov, 1.25f, static_cast<float>(width) / height, &fovX, &fovY);

    SetupCaptureCameraDesc(cameraDesc, layerMask, false, colorClear, clearColor, origin, axis, fovX, fovY, BE1::MeterToUnit(0.05f), BE1::MeterToUnit(2000.0f), width, height, targetRHITexture, RHI::ViewID::PrimaryID);
    renderCamera.Update(cameraDesc);

    if (!BeginCaptureFrame(renderContext, renderWorld, targetTexture, "RenderCapture::CaptureSceneToTexture")) {
        return false;
    }

    renderWorld->RenderScene(&renderCamera);

    return EndCaptureFrame(renderContext, renderWorld, waitForCompletion, "RenderCapture::CaptureSceneToTexture");
}

bool RenderCapture::CaptureSceneToImage(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, bool useHDR, int width, int height, BE1::Image &outImage) {
    assert(BE1::Engine::IsInMainThread());

    if (width <= 0 || height <= 0) {
        BE_WARNLOG("RenderCapture::CaptureSceneToImage: invalid capture parameters\n");
        return false;
    }

    const BE1::Image::Format captureFormat = useHDR ? BE1::Image::Format::R11G11B10_FLOAT : BE1::Image::Format::R8G8B8A8;
    const BE1::Image::GammaSpace captureGammaSpace = useHDR ? BE1::Image::GammaSpace::Linear : BE1::Image::GammaSpace::sRGB;

    SharedPtr<BE1::Image> textureImage = MakeShared<BE1::Image>();
    textureImage->InitFromMemory(width, height, 1, 1, 1, captureFormat, captureGammaSpace, nullptr, BE1::Image::Flag::None);

    SharedPtr<Texture> targetTexture = MakeShared<Texture>();
    const Texture::Flag textureFlags = Texture::Flag::RenderTarget | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo;
    if (!targetTexture->Create(RHI::TextureType::Texture2D, textureImage, textureFlags)) {
        BE_WARNLOG("RenderCapture::CaptureSceneToImage: failed to create capture texture\n");
        return false;
    }

    if (!CaptureSceneToTexture(renderContext, renderWorld, layerMask, colorClear, clearColor, origin, axis, fov, width, height, targetTexture, true)) {
        return false;
    }

    const BE1::Image::Format readbackFormat = targetTexture->GetFormat();
    const BE1::Image::GammaSpace readbackGammaSpace = BE1::Image::NeedFloatConversion(readbackFormat) ? BE1::Image::GammaSpace::Linear : BE1::Image::GammaSpace::sRGB;

    BE1::Image capturedImage;
    capturedImage.Create2D(targetTexture->GetWidth(), targetTexture->GetHeight(), 1, readbackFormat, readbackGammaSpace, nullptr, BE1::Image::Flag::None);
    if (!targetTexture->GetTexels2D(0, readbackFormat, capturedImage.GetPixels())) {
        BE_WARNLOG("RenderCapture::CaptureSceneToImage: failed to read capture texture\n");
        return false;
    }

    outImage = std::move(capturedImage);
    return true;
}

static bool CaptureEnvCubeFacesToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, float zNear, float zFar, const SharedPtr<Texture> &targetCubeTexture, const RHI::CubemapFace *faces, uint32_t faceCount, bool waitForCompletion, const char *captureName) {
    assert(BE1::Engine::IsInMainThread());

    if (!targetCubeTexture || !targetCubeTexture->IsReady() || !faces || faceCount == 0 || faceCount > 6 || zNear <= 0.0f || zFar <= zNear) {
        BE_WARNLOG("%s: invalid capture parameters\n", captureName);
        return false;
    }

    RHI::Texture *targetRHITexture = targetCubeTexture->GetRHITexture();
    if (!targetRHITexture || targetRHITexture->type != RHI::TextureType::TextureCube || targetRHITexture->GetArraySize() != 6 || targetRHITexture->GetWidth() == 0 || targetRHITexture->GetWidth() != targetRHITexture->GetHeight()) {
        BE_WARNLOG("%s: target texture is not a valid cubemap render target\n", captureName);
        return false;
    }

    // 캡처할 cubemap face 별 RTV 를 준비한다.
    RHI::ViewID faceViewIDs[6];
    for (uint32_t index = 0; index < faceCount; ++index) {
        const uint32_t faceIndex = static_cast<uint32_t>(faces[index]);
        if (faceIndex >= 6) {
            BE_WARNLOG("%s: invalid cubemap face\n", captureName);
            return false;
        }

        faceViewIDs[index] = RHI::ViewID::InvalidID;

        const uint32_t viewCount = targetRHITexture->GetViewCount(RHI::ViewType::RTV);
        for (uint32_t viewIndex = 0; viewIndex < viewCount; ++viewIndex) {
            const RHI::ViewID viewID = targetRHITexture->GetViewID(RHI::ViewType::RTV, viewIndex);
            RHI::TextureViewInfo viewInfo;
            if (targetRHITexture->GetViewInfo(RHI::ViewType::RTV, viewID, viewInfo) &&
                viewInfo.firstSlice == faceIndex && viewInfo.sliceCount == 1 && viewInfo.firstMipLevel == 0 && viewInfo.mipCount == 1) {
                faceViewIDs[index] = viewID;
                break;
            }
        }

        if (!faceViewIDs[index].IsValid()) {
            faceViewIDs[index] = RHI::renderer->CreateView(targetRHITexture, RHI::ViewType::RTV, RHI::Format::Auto, faceIndex, 1, 0, 1);
        }

        if (!faceViewIDs[index].IsValid()) {
            BE_WARNLOG("%s: failed to create a cubemap face render target view\n", captureName);
            return false;
        }
    }

    if (!BeginCaptureFrame(renderContext, renderWorld, targetCubeTexture, captureName)) {
        return false;
    }

    const uint32_t faceSize = targetRHITexture->GetWidth();
    RenderCamera renderCamera;

    for (uint32_t faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
        BE1::Mat3 axis;
        CubeMapFaceToViewAxis(faces[faceIndex], axis);

        RenderCamera::Desc cameraDesc;
        SetupCaptureCameraDesc(cameraDesc, layerMask, staticOnly, colorClear, clearColor, origin, axis, 90.0f, 90.0f, zNear, zFar, faceSize, faceSize, targetRHITexture, faceViewIDs[faceIndex]);

        // Cubemap 렌더링과 샘플링 모두 EngineAxis (오른손 좌표계) 를 사용한다.
        // 다만 카메라 출력과 cubemap face 의 세로축 방향이 반대이므로, cubemap face 에 저장할 때 최종 blit 의 Y를 반전시킨다.
        cameraDesc.flags |= RenderCamera::Flag::FlipOutputTextureY;

        renderCamera.Update(cameraDesc);

        renderWorld->RenderScene(&renderCamera);
    }

    return EndCaptureFrame(renderContext, renderWorld, waitForCompletion, captureName);
}

bool RenderCapture::CaptureEnvCubeFaceToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, float zNear, float zFar, const SharedPtr<Texture> &targetCubeTexture, RHI::CubemapFace face, bool waitForCompletion) {
    return CaptureEnvCubeFacesToTexture(renderContext, renderWorld, layerMask, staticOnly, colorClear, clearColor, origin, zNear, zFar, targetCubeTexture, &face, 1, waitForCompletion, "RenderCapture::CaptureEnvCubeFaceToTexture");
}

bool RenderCapture::CaptureEnvCubeToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, float zNear, float zFar, const SharedPtr<Texture> &targetCubeTexture, bool waitForCompletion) {
    static constexpr RHI::CubemapFace Faces[6] = {
        RHI::CubemapFace::PositiveX,
        RHI::CubemapFace::NegativeX,
        RHI::CubemapFace::PositiveY,
        RHI::CubemapFace::NegativeY,
        RHI::CubemapFace::PositiveZ,
        RHI::CubemapFace::NegativeZ
    };

    return CaptureEnvCubeFacesToTexture(renderContext, renderWorld, layerMask, staticOnly, colorClear, clearColor, origin, zNear, zFar, targetCubeTexture, Faces, 6, waitForCompletion, "RenderCapture::CaptureEnvCubeToTexture");
}

bool RenderCapture::CaptureEnvCubeToImage(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, BE1::Image &outCubeImage) {
    assert(BE1::Engine::IsInMainThread());

    if (size <= 0 || zNear <= 0.0f || zFar <= zNear) {
        BE_WARNLOG("RenderCapture::CaptureEnvCubeToImage: invalid capture parameters\n");
        return false;
    }

    const BE1::Image::Format captureFormat = useHDR ? BE1::Image::Format::R11G11B10_FLOAT : BE1::Image::Format::R8G8B8A8;
    const BE1::Image::GammaSpace captureGammaSpace = useHDR ? BE1::Image::GammaSpace::Linear : BE1::Image::GammaSpace::sRGB;

    SharedPtr<BE1::Image> textureImage = MakeShared<BE1::Image>();
    textureImage->InitFromMemory(size, size, 1, 6, 1, captureFormat, captureGammaSpace, nullptr, BE1::Image::Flag::CubeMap);

    SharedPtr<Texture> targetCubeTexture = MakeShared<Texture>();
    const Texture::Flag textureFlags = Texture::Flag::RenderTarget | Texture::Flag::NoMipmaps | Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo;
    if (!targetCubeTexture->Create(RHI::TextureType::TextureCube, textureImage, textureFlags)) {
        BE_WARNLOG("RenderCapture::CaptureEnvCubeToImage: failed to create capture texture\n");
        return false;
    }

    if (!CaptureEnvCubeToTexture(renderContext, renderWorld, layerMask, staticOnly, colorClear, clearColor, origin, zNear, zFar, targetCubeTexture, true)) {
        return false;
    }

    const BE1::Image::Format readbackFormat = targetCubeTexture->GetFormat();
    const BE1::Image::GammaSpace readbackGammaSpace = BE1::Image::NeedFloatConversion(readbackFormat) ? BE1::Image::GammaSpace::Linear : BE1::Image::GammaSpace::sRGB;

    BE1::Image capturedCubeImage;
    capturedCubeImage.CreateCube(targetCubeTexture->GetWidth(), 1, readbackFormat, readbackGammaSpace, nullptr, BE1::Image::Flag::None);

    for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex) {
        if (!targetCubeTexture->GetTexelsCubeFace(static_cast<RHI::CubemapFace>(faceIndex), 0, readbackFormat, capturedCubeImage.GetPixels(0, faceIndex))) {
            BE_WARNLOG("RenderCapture::CaptureEnvCubeToImage: failed to read cubemap face %u\n", faceIndex);
            return false;
        }
    }

    outCubeImage = std::move(capturedCubeImage);
    return true;
}

bool RenderCapture::TakeScreenShot(const char *filename, RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask,
    const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, bool useHDR, int width, int height) {
    if (!filename || !filename[0]) {
        BE_WARNLOG("RenderCapture::TakeScreenShot: invalid filename\n");
        return false;
    }

    BE1::Image screenImage;
    if (!CaptureSceneToImage(renderContext, renderWorld, layerMask, false, BE1::Color4::black, origin, axis, fov, useHDR, width, height, screenImage)) {
        return false;
    }

    BE1::Str outputPath = filename;
    outputPath.DefaultFileExtension(".png");
    if (!screenImage.Write(outputPath)) {
        BE_WARNLOG("RenderCapture::TakeScreenShot: failed to write \"%s\"\n", outputPath.c_str());
        return false;
    }

    BE_LOG("Screenshot saved to \"%s\"\n", outputPath.c_str());
    return true;
}

bool RenderCapture::TakeEnvShot(const char *filename, RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly,
    const BE1::Vec3 &origin, bool useHDR, int size) {
    if (!filename || !filename[0]) {
        BE_WARNLOG("RenderCapture::TakeEnvShot: invalid filename\n");
        return false;
    }

    BE1::Image envCubeImage;
    if (!CaptureEnvCubeToImage(renderContext, renderWorld, layerMask, staticOnly, false, BE1::Color4::black, origin,
        BE1::CmToUnit(5.0f), BE1::MeterToUnit(100.0f), useHDR, size, envCubeImage)) {
        return false;
    }

    BE1::Str outputPath = filename;
    outputPath.DefaultFileExtension(".dds");
    if (!envCubeImage.Write(outputPath)) {
        BE_WARNLOG("RenderCapture::TakeEnvShot: failed to write \"%s\"\n", outputPath.c_str());
        return false;
    }

    BE_LOG("Environment cubemap snapshot saved to \"%s\"\n", outputPath.c_str());
    return true;
}
