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
#include "Render/Frontend/RenderCamera.h"
#include "RenderContext.h"
#include "RenderSystem.h"
#include "RenderUtils.h"
#include "Render/Frontend/RenderWorld.h"
#include "Render/Texture/Texture.h"

static void SetupCameraDesc(RenderCamera::Desc &cameraDesc, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
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

static bool BeginRenderFrame(RenderContext *renderContext, RenderWorld *renderWorld, const SharedPtr<Texture> &targetTexture, const char *captureName) {
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

static void EndRenderFrame(RenderContext *renderContext, RenderWorld *renderWorld, bool waitForCompletion) {
    // EndFrame 이후 완료를 기다릴 캡처 프레임을 보관한다.
    const RenderFrameData *captureFrameData = waitForCompletion ? renderContext->GetCurrentFrontendFrameData() : nullptr;

    renderWorld->EndFrame();
    renderContext->EndFrame();

    if (captureFrameData) {
        renderSystem->WaitFrameRenderCompleted(captureFrameData);
    }
}

static bool CreateIBLCubemapTexture(int size, bool useHDR, bool renderTarget, bool allocateMipmaps, SharedPtr<Texture> &outTexture) {
    const BE1::Image::Format format = useHDR ? BE1::Image::Format::R11G11B10_FLOAT : BE1::Image::Format::R8G8B8A8;

    // IBL 적분과 mip 생성은 linear radiance 를 대상으로 하므로 LDR 텍스처도 sRGB view 를 사용하지 않는다.
    SharedPtr<BE1::Image> textureImage = MakeShared<BE1::Image>();
    textureImage->InitFromMemory(size, size, 1, 6, 1, format, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::CubeMap);

    Texture::Flag textureFlags = Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo;
    if (renderTarget) {
        textureFlags |= Texture::Flag::RenderTarget;
    }
    if (allocateMipmaps) {
        textureFlags |= Texture::Flag::AllocateEmptyMipmaps;
    } else {
        textureFlags |= Texture::Flag::NoMipmaps;
    }
    if (!renderTarget || allocateMipmaps) {
        textureFlags |= Texture::Flag::UnorderedAccess;
    }

    SharedPtr<Texture> texture = MakeShared<Texture>();
    if (!texture->Create(RHI::TextureType::TextureCube, textureImage, textureFlags)) {
        return false;
    }

    outTexture = std::move(texture);
    return true;
}

static bool CreateTextureMipViews(RHI::Texture *texture, RHI::ViewType viewType, RHI::ViewID *outViewIDs) {
    const uint32_t mipLevelCount = texture->GetMipLevelCount();

    for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel) {
        outViewIDs[mipLevel] = RHI::renderer->CreateView(texture, viewType, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
        if (!outViewIDs[mipLevel].IsValid()) {
            return false;
        }
    }
    return true;
}

bool RenderCapture::RenderSceneToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, int width, int height, const SharedPtr<Texture> &targetTexture, bool waitForCompletion) {
    assert(BE1::Engine::IsInMainThread());

    if (!targetTexture || !targetTexture->IsReady() || width <= 0 || height <= 0) {
        BE_WARNLOG("RenderCapture::RenderSceneToTexture: invalid capture parameters\n");
        return false;
    }

    RHI::Texture *targetRHITexture = targetTexture->GetRHITexture();
    RHI::TextureViewInfo targetViewInfo;
    if (!targetRHITexture || targetRHITexture->type != RHI::TextureType::Texture2D ||
        !targetRHITexture->GetViewInfo(RHI::ViewType::RTV, RHI::ViewID::PrimaryID, targetViewInfo)) {
        BE_WARNLOG("RenderCapture::RenderSceneToTexture: target texture has no valid 2D render target view\n");
        return false;
    }

    if (static_cast<uint32_t>(width) > targetViewInfo.width || static_cast<uint32_t>(height) > targetViewInfo.height) {
        BE_WARNLOG("RenderCapture::RenderSceneToTexture: capture dimensions exceed the target texture view\n");
        return false;
    }

    RenderCamera renderCamera;
    RenderCamera::Desc cameraDesc;
    float fovX;
    float fovY;
    RenderCamera::CalculateFov(fov, 1.25f, static_cast<float>(width) / height, &fovX, &fovY);

    SetupCameraDesc(cameraDesc, layerMask, false, colorClear, clearColor, origin, axis, fovX, fovY, BE1::MeterToUnit(0.05f), BE1::MeterToUnit(2000.0f), width, height, targetRHITexture, RHI::ViewID::PrimaryID);
    renderCamera.Update(cameraDesc);

    if (!BeginRenderFrame(renderContext, renderWorld, targetTexture, "RenderCapture::RenderSceneToTexture")) {
        return false;
    }

    renderWorld->RenderScene(&renderCamera);

    EndRenderFrame(renderContext, renderWorld, waitForCompletion);
    return true;
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

    if (!RenderSceneToTexture(renderContext, renderWorld, layerMask, colorClear, clearColor, origin, axis, fov, width, height, targetTexture, true)) {
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

bool RenderCapture::CaptureSceneToFile(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, const BE1::Mat3 &axis, float fov, bool useHDR, int width, int height, const char *filename) {
    if (!filename || !filename[0]) {
        BE_WARNLOG("RenderCapture::CaptureSceneToFile: invalid filename\n");
        return false;
    }

    BE1::Image sceneImage;
    if (!CaptureSceneToImage(renderContext, renderWorld, layerMask, colorClear, clearColor, origin, axis, fov, useHDR, width, height, sceneImage)) {
        return false;
    }

    BE1::Str outputPath = filename;
    outputPath.DefaultFileExtension(".png");
    if (!sceneImage.Write(outputPath)) {
        BE_WARNLOG("RenderCapture::CaptureSceneToFile: failed to write \"%s\"\n", outputPath.c_str());
        return false;
    }

    BE_LOG("Scene capture saved to \"%s\"\n", outputPath.c_str());
    return true;
}

static bool RenderEnvCubeFacesToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
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

    if (!BeginRenderFrame(renderContext, renderWorld, targetCubeTexture, captureName)) {
        return false;
    }

    const uint32_t faceSize = targetRHITexture->GetWidth();
    RenderCamera renderCamera;

    for (uint32_t faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
        BE1::Mat3 axis;
        CubeMapFaceToViewAxis(faces[faceIndex], axis);

        RenderCamera::Desc cameraDesc;
        SetupCameraDesc(cameraDesc, layerMask, staticOnly, colorClear, clearColor, origin, axis, 90.0f, 90.0f, zNear, zFar, faceSize, faceSize, targetRHITexture, faceViewIDs[faceIndex]);

        // Cubemap 렌더링과 샘플링 모두 EngineAxis (오른손 좌표계) 를 사용한다.
        // 다만 카메라 출력과 cubemap face 의 세로축 방향이 반대이므로, cubemap face 에 저장할 때 최종 blit 의 Y를 반전시킨다.
        cameraDesc.flags |= RenderCamera::Flag::FlipOutputTextureY;

        renderCamera.Update(cameraDesc);

        renderWorld->RenderScene(&renderCamera);
    }

    EndRenderFrame(renderContext, renderWorld, waitForCompletion);
    return true;
}

bool RenderCapture::RenderEnvCubeFaceToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, float zNear, float zFar, const SharedPtr<Texture> &targetCubeTexture, RHI::CubemapFace face, bool waitForCompletion) {
    return RenderEnvCubeFacesToTexture(renderContext, renderWorld, layerMask, staticOnly, colorClear, clearColor, origin, zNear, zFar, targetCubeTexture, &face, 1, waitForCompletion, "RenderCapture::RenderEnvCubeFaceToTexture");
}

bool RenderCapture::RenderEnvCubeToTexture(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, float zNear, float zFar, const SharedPtr<Texture> &targetCubeTexture, bool waitForCompletion) {
    static constexpr RHI::CubemapFace Faces[6] = {
        RHI::CubemapFace::PositiveX,
        RHI::CubemapFace::NegativeX,
        RHI::CubemapFace::PositiveY,
        RHI::CubemapFace::NegativeY,
        RHI::CubemapFace::PositiveZ,
        RHI::CubemapFace::NegativeZ
    };

    return RenderEnvCubeFacesToTexture(renderContext, renderWorld, layerMask, staticOnly, colorClear, clearColor, origin, zNear, zFar, targetCubeTexture, Faces, 6, waitForCompletion, "RenderCapture::RenderEnvCubeToTexture");
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

    if (!RenderEnvCubeToTexture(renderContext, renderWorld, layerMask, staticOnly, colorClear, clearColor, origin, zNear, zFar, targetCubeTexture, true)) {
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

bool RenderCapture::CaptureEnvCubeToFile(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly, bool colorClear, const BE1::Color4 &clearColor,
    const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, const char *filename) {
    if (!filename || !filename[0]) {
        BE_WARNLOG("RenderCapture::CaptureEnvCubeToFile: invalid filename\n");
        return false;
    }

    BE1::Image envCubeImage;
    if (!CaptureEnvCubeToImage(renderContext, renderWorld, layerMask, staticOnly, colorClear, clearColor, origin, zNear, zFar, useHDR, size, envCubeImage)) {
        return false;
    }

    BE1::Str outputPath = filename;
    outputPath.DefaultFileExtension(".dds");
    if (!envCubeImage.Write(outputPath)) {
        BE_WARNLOG("RenderCapture::CaptureEnvCubeToFile: failed to write \"%s\"\n", outputPath.c_str());
        return false;
    }

    BE_LOG("Environment cubemap capture saved to \"%s\"\n", outputPath.c_str());
    return true;
}

bool RenderCapture::CaptureDiffuseIrradianceEnvCubeToImage(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly,
    const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, int envSize, BE1::Image &outCubeImage) {
    assert(BE1::Engine::IsInMainThread());

    if (size <= 0 || envSize <= 0 || zNear <= 0.0f || zFar <= zNear) {
        BE_WARNLOG("RenderCapture::CaptureDiffuseIrradianceEnvCubeToImage: invalid capture parameters\n");
        return false;
    }

    SharedPtr<Texture> radianceCubeTexture;
    if (!CreateIBLCubemapTexture(envSize, useHDR, true, false, radianceCubeTexture)) {
        BE_WARNLOG("RenderCapture::CaptureDiffuseIrradianceEnvCubeToImage: failed to create radiance cubemap\n");
        return false;
    }

    if (!RenderEnvCubeToTexture(renderContext, renderWorld, layerMask, staticOnly, false, BE1::Color4::black, origin, zNear, zFar, radianceCubeTexture, true)) {
        return false;
    }

    SharedPtr<Texture> diffuseIrradianceCubeTexture;
    if (!CreateIBLCubemapTexture(size, useHDR, false, false, diffuseIrradianceCubeTexture)) {
        BE_WARNLOG("RenderCapture::CaptureDiffuseIrradianceEnvCubeToImage: failed to create output cubemap\n");
        return false;
    }

    RHI::Texture *outputRHITexture = diffuseIrradianceCubeTexture->GetRHITexture();

    RHI::FrameThreadData *frameThreadData = renderContext->GetCurrentFrontendFrameData()->GetThreadData(0);
    RHI::CommandList *commandList = frameThreadData->BeginCommandList(RHI::CommandQueueType::Graphics);
#if 0
    bool generated = renderSystem->GetIBLBaker().GenerateDiffuseIrradiance(commandList, radianceCubeTexture->GetRHITexture(), outputRHITexture);
#else
    bool generated = renderSystem->GetIBLBaker().GenerateDiffuseIrradianceSH(commandList, radianceCubeTexture->GetRHITexture(), outputRHITexture);
#endif

    commandList->CloseAndExecute();
    if (!generated) {
        BE_WARNLOG("RenderCapture::CaptureDiffuseIrradianceEnvCubeToImage: failed to generate diffuse irradiance cubemap\n");
        return false;
    }

    const BE1::Image::Format outputFormat = useHDR ? BE1::Image::Format::R11G11B10_FLOAT : BE1::Image::Format::R8G8B8A8;

    BE1::Image diffuseIrradianceCubeImage;
    diffuseIrradianceCubeImage.CreateCube(diffuseIrradianceCubeTexture->GetWidth(), 1, outputFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex) {
        if (!diffuseIrradianceCubeTexture->GetTexelsCubeFace(static_cast<RHI::CubemapFace>(faceIndex), 0, outputFormat, diffuseIrradianceCubeImage.GetPixels(0, faceIndex))) {
            BE_WARNLOG("RenderCapture::CaptureDiffuseIrradianceEnvCubeToImage: failed to read cubemap face %u mip 0\n", faceIndex);
            return false;
        }
    }

    outCubeImage = std::move(diffuseIrradianceCubeImage);
    return true;
}

bool RenderCapture::CaptureDiffuseIrradianceEnvCubeToFile(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly,
    const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, int envSize, const char *filename) {
    if (!filename || !filename[0]) {
        BE_WARNLOG("RenderCapture::CaptureDiffuseIrradianceEnvCubeToFile: invalid filename\n");
        return false;
    }

    BE1::Image diffuseIrradianceCubeImage;
    if (!CaptureDiffuseIrradianceEnvCubeToImage(renderContext, renderWorld, layerMask, staticOnly, origin, zNear, zFar, useHDR, size, envSize, diffuseIrradianceCubeImage)) {
        return false;
    }

    BE1::Str outputPath = filename;
    outputPath.DefaultFileExtension(".dds");
    if (!diffuseIrradianceCubeImage.Write(outputPath)) {
        BE_WARNLOG("RenderCapture::CaptureDiffuseIrradianceEnvCubeToFile: failed to write \"%s\"\n", outputPath.c_str());
        return false;
    }

    BE_LOG("Diffuse irradiance environment cubemap capture saved to \"%s\"\n", outputPath.c_str());
    return true;
}

bool RenderCapture::CaptureSpecularPrefilteredEnvCubeToImage(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly,
    const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, int envSize, BE1::Image &outCubeImage) {
    assert(BE1::Engine::IsInMainThread());

    if (size <= 0 || envSize <= 0 || zNear <= 0.0f || zFar <= zNear) {
        BE_WARNLOG("RenderCapture::CaptureSpecularPrefilteredEnvCubeToImage: invalid capture parameters\n");
        return false;
    }

    SharedPtr<Texture> radianceCubeTexture;
    if (!CreateIBLCubemapTexture(envSize, useHDR, true, true, radianceCubeTexture)) {
        BE_WARNLOG("RenderCapture::CaptureSpecularPrefilteredEnvCubeToImage: failed to create radiance cubemap\n");
        return false;
    }

    if (!RenderEnvCubeToTexture(renderContext, renderWorld, layerMask, staticOnly, false, BE1::Color4::black, origin, zNear, zFar, radianceCubeTexture, true)) {
        return false;
    }

    SharedPtr<Texture> specularPrefilteredCubeTexture;
    if (!CreateIBLCubemapTexture(size, useHDR, false, true, specularPrefilteredCubeTexture)) {
        BE_WARNLOG("RenderCapture::CaptureSpecularPrefilteredEnvCubeToImage: failed to create output cubemap\n");
        return false;
    }

    RHI::Texture *radianceRHITexture = radianceCubeTexture->GetRHITexture();
    RHI::Texture *outputRHITexture = specularPrefilteredCubeTexture->GetRHITexture();

    const uint32_t radianceMipLevelCount = radianceRHITexture->GetMipLevelCount();
    const uint32_t outputMipLevelCount = outputRHITexture->GetMipLevelCount();

    RHI::ViewID *radianceMipSRVs = static_cast<RHI::ViewID *>(_alloca(sizeof(RHI::ViewID) * radianceMipLevelCount));
    RHI::ViewID *radianceMipUAVs = static_cast<RHI::ViewID *>(_alloca(sizeof(RHI::ViewID) * radianceMipLevelCount));
    RHI::ViewID *outputMipUAVs = static_cast<RHI::ViewID *>(_alloca(sizeof(RHI::ViewID) * outputMipLevelCount));

    if (!CreateTextureMipViews(radianceRHITexture, RHI::ViewType::SRV, radianceMipSRVs) ||
        !CreateTextureMipViews(radianceRHITexture, RHI::ViewType::UAV, radianceMipUAVs) ||
        !CreateTextureMipViews(outputRHITexture, RHI::ViewType::UAV, outputMipUAVs)) {
        BE_WARNLOG("RenderCapture::CaptureSpecularPrefilteredEnvCubeToImage: failed to create mip views\n");
        return false;
    }

    RHI::FrameThreadData *frameThreadData = renderContext->GetCurrentFrontendFrameData()->GetThreadData(0);
    RHI::CommandList *commandList = frameThreadData->BeginCommandList(RHI::CommandQueueType::Graphics);

    // GGX importance sampling 이 source LOD 를 선택할 수 있도록 캡처된 radiance cubemap 의 mip chain 을 먼저 만든다.
    RHI::renderer->GenerateMipChain(commandList, radianceRHITexture, radianceMipSRVs, radianceMipUAVs, false);
#if 1
    bool generated = renderSystem->GetIBLBaker().GenerateLDSumGGX(commandList, radianceRHITexture, outputRHITexture, outputMipUAVs);
#else
    bool generated = renderSystem->GetIBLBaker().GenerateLDSumPhongSpecular(commandList, radianceRHITexture, 2048, outputRHITexture, outputMipUAVs);
#endif

    commandList->CloseAndExecute();
    if (!generated) {
        BE_WARNLOG("RenderCapture::CaptureSpecularPrefilteredEnvCubeToImage: failed to generate specular prefiltered cubemap\n");
        return false;
    }

    const BE1::Image::Format outputFormat = useHDR ? BE1::Image::Format::R11G11B10_FLOAT : BE1::Image::Format::R8G8B8A8;

    BE1::Image specularPrefilteredCubeImage;
    specularPrefilteredCubeImage.CreateCube(specularPrefilteredCubeTexture->GetWidth(), outputMipLevelCount, outputFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex) {
        for (uint32_t mipLevel = 0; mipLevel < outputMipLevelCount; ++mipLevel) {
            if (!specularPrefilteredCubeTexture->GetTexelsCubeFace(static_cast<RHI::CubemapFace>(faceIndex), mipLevel, outputFormat, specularPrefilteredCubeImage.GetPixels(mipLevel, faceIndex))) {
                BE_WARNLOG("RenderCapture::CaptureSpecularPrefilteredEnvCubeToImage: failed to read cubemap face %u mip %u\n", faceIndex, mipLevel);
                return false;
            }
        }
    }

    outCubeImage = std::move(specularPrefilteredCubeImage);
    return true;
}

bool RenderCapture::CaptureSpecularPrefilteredEnvCubeToFile(RenderContext *renderContext, RenderWorld *renderWorld, uint32_t layerMask, bool staticOnly,
    const BE1::Vec3 &origin, float zNear, float zFar, bool useHDR, int size, int envSize, const char *filename) {
    if (!filename || !filename[0]) {
        BE_WARNLOG("RenderCapture::CaptureSpecularPrefilteredEnvCubeToFile: invalid filename\n");
        return false;
    }

    BE1::Image specularPrefilteredCubeImage;
    if (!CaptureSpecularPrefilteredEnvCubeToImage(renderContext, renderWorld, layerMask, staticOnly, origin, zNear, zFar, useHDR, size, envSize, specularPrefilteredCubeImage)) {
        return false;
    }

    BE1::Str outputPath = filename;
    outputPath.DefaultFileExtension(".dds");
    if (!specularPrefilteredCubeImage.Write(outputPath)) {
        BE_WARNLOG("RenderCapture::CaptureSpecularPrefilteredEnvCubeToFile: failed to write \"%s\"\n", outputPath.c_str());
        return false;
    }

    BE_LOG("Specular prefiltered environment cubemap capture saved to \"%s\"\n", outputPath.c_str());
    return true;
}
