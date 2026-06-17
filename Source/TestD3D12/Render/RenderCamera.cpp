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
#include "RenderCamera.h"
#include "RenderCvars.h"
#include "RHI/RHI.h"
#include "Shaders/ShaderInterop.h"

static void SetViewMatrix(const BE1::Mat3 &viewAxis, const BE1::Vec3 &viewOrigin, float *rowMajor4x4ViewMatrix) {
    // left axis
    rowMajor4x4ViewMatrix[0] = -viewAxis[1].x;
    rowMajor4x4ViewMatrix[1] = -viewAxis[1].y;
    rowMajor4x4ViewMatrix[2] = -viewAxis[1].z;
    rowMajor4x4ViewMatrix[3] = -viewAxis[1].Dot(-viewOrigin);

    // up axis
    rowMajor4x4ViewMatrix[4] = viewAxis[2].x;
    rowMajor4x4ViewMatrix[5] = viewAxis[2].y;
    rowMajor4x4ViewMatrix[6] = viewAxis[2].z;
    rowMajor4x4ViewMatrix[7] = viewAxis[2].Dot(-viewOrigin);

    // forward axis
    rowMajor4x4ViewMatrix[8] = -viewAxis[0].x;
    rowMajor4x4ViewMatrix[9] = -viewAxis[0].y;
    rowMajor4x4ViewMatrix[10] = -viewAxis[0].z;
    rowMajor4x4ViewMatrix[11] = -viewAxis[0].Dot(-viewOrigin);

    rowMajor4x4ViewMatrix[12] = 0.0f;
    rowMajor4x4ViewMatrix[13] = 0.0f;
    rowMajor4x4ViewMatrix[14] = 0.0f;
    rowMajor4x4ViewMatrix[15] = 1.0f;
}

void RenderCamera::Update(const RenderCameraDesc &desc) {
    this->desc = desc;

    if (desc.orthogonal) {
        // Set bounding volume for orthogonal view.
        BE1::Vec3 extents((desc.zFar - desc.zNear) * 0.5f, desc.sizeX, desc.sizeY);
        box.SetCenter(desc.origin + desc.axis[0] * (desc.zNear + extents[0]));
        box.SetExtents(extents);
        box.SetAxis(desc.axis);

        // Calculate orthogonal projection matrix.
        projMatrix.SetOrthoRH(-desc.sizeX, desc.sizeX, -desc.sizeY, desc.sizeY, desc.zNear, desc.zFar, false);
    } else {
        // Set bounding frustum for perspective view.
        frustum.SetOrigin(desc.origin);
        frustum.SetAxis(desc.axis);
        frustum.SetSize(desc.zNear, desc.zFar, desc.zFar * BE1::Math::Tan(DEG2RAD(desc.fovX) * 0.5f), desc.zFar * BE1::Math::Tan(DEG2RAD(desc.fovY) * 0.5f));

        // Calculate view frustum planes.
        frustum.ToPlanes(frustumPlanes);

        // Calculate view frustum points.
        frustum.ToPoints(frustumPoints);

        // Calculate perspective projection matrix.
        projMatrix.SetPerspectiveRH(desc.fovY, desc.fovX / desc.fovY, desc.zNear, desc.zFar, false);
    }

    // Calculate view matrix.
    SetViewMatrix(desc.axis, desc.origin, viewMatrix);

    // Calculate view projection matrix.
    viewProjMatrix = projMatrix * viewMatrix;
}

bool RenderCamera::TransformWorldToNDC(const BE1::Vec3 &worldPosition, BE1::Vec3 &normalizedDeviceCoords) const {
    BE1::Vec4 clipCoords = viewProjMatrix * BE1::Vec4(worldPosition, 1.0f);
    if (clipCoords.w > 0) {
        float invW = 1.0f / clipCoords.w;

        normalizedDeviceCoords.x = clipCoords.x * invW; // Clipping range is [-1, 1]
        normalizedDeviceCoords.y = clipCoords.y * invW; // Clipping range is [-1, 1]
        normalizedDeviceCoords.z = clipCoords.z * invW; // Clipping range is [-1, 1] or [0, 1] in D3D
        return true;
    }
    return false;
}

void RenderCamera::TransformNDCToPixel(const BE1::Vec3 &normalizedDeviceCoords, BE1::Vec3 &pixelCoords) const {
    float fx = (normalizedDeviceCoords.x + 1.0f) * 0.5f; // Valid range is [0, 1]
    float fy = 1.0f - (normalizedDeviceCoords.y + 1.0f) * 0.5f; // Valid range is [0, 1]
    float fz = (normalizedDeviceCoords.z + 1.0f) * 0.5f; // Valid range is [0, 1]

    pixelCoords.x = desc.outputRect.x + fx * desc.outputRect.w;
    pixelCoords.y = desc.outputRect.y + fy * desc.outputRect.h;
    pixelCoords.z = fz; // depth value
}

bool RenderCamera::TransformWorldToPixel(const BE1::Vec3 &worldPosition, BE1::Vec3 &pixelCoords) const {
    BE1::Vec3 normalizedDeviceCoords;
    if (!TransformWorldToNDC(worldPosition, normalizedDeviceCoords)) {
        return false;
    }

    TransformNDCToPixel(normalizedDeviceCoords, pixelCoords);
    return true;
}

void RenderCamera::UntransformPixelToNDC(const BE1::Vec3 &pixelCoords, BE1::Vec3 &normalizedDeviceCoords) const {
    float fx = (pixelCoords.x - desc.outputRect.x) / desc.outputRect.w;
    float fy = 1.0f - (pixelCoords.y - desc.outputRect.y) / desc.outputRect.h;
    float fz = pixelCoords.z; // depth value

    normalizedDeviceCoords.x = (fx * 2.0f) - 1.0f;
    normalizedDeviceCoords.y = (fy * 2.0f) - 1.0f;
    normalizedDeviceCoords.z = (fz * 2.0f) - 1.0f;
}

bool RenderCamera::UntransformNDCToWorld(const BE1::Vec3 &normalizedDeviceCoords, BE1::Vec3 &worldCoords) const {
    BE1::Vec4 worldPosition = viewProjMatrix.Inverse() * BE1::Vec4(normalizedDeviceCoords, 1.0f);
    if (worldPosition.w > 0) {
        float invW = 1.0f / worldPosition.w;

        worldCoords.x = worldPosition.x * invW;
        worldCoords.y = worldPosition.y * invW;
        worldCoords.z = worldPosition.z * invW;
        return true;
    }
    return false;
}

bool RenderCamera::UntransformPixelToWorld(const BE1::Vec3 &pixelCoords, BE1::Vec3 &worldCoords) const {
    BE1::Vec3 normalizedDeviceCoords;
    UntransformPixelToNDC(pixelCoords, normalizedDeviceCoords);

    if (UntransformNDCToWorld(normalizedDeviceCoords, worldCoords)) {
        return true;
    }
    return false;
}

bool RenderCamera::CalcClipRectFromSphere(const BE1::Sphere &sphere, BE1::Rect &clipRect) const {
    BE1::Vec3 planeNormal1, planeNormal2;
    int xmin, xmax, ymax, ymin;
    float x, y;

    float r2 = sphere.radius * sphere.radius;

    // in case camera in in sphere.
    if (desc.origin.DistanceSqr(sphere.center) < r2) {
        clipRect = desc.outputRect;
        return true;
    }

    // sphere 의 중심좌표(L) 를 카메라 로컬좌표계(X, Y, Z = FORWARD, LEFT, UP) 로 변환.
    BE1::Vec3 localOrigin = desc.axis.TransposedMulVec(sphere.center - desc.origin);

    float x2 = localOrigin.x * localOrigin.x;
    float y2 = localOrigin.y * localOrigin.y;
    float z2 = localOrigin.z * localOrigin.z;

    // 세로 접평면 T 는 z 축에 평행하고 카메라(C) 를 지나므로,
    // T = (Nx, Ny, 0, 0) 이면
    //
    // 1) T dot L = r 
    // 2) Nx^2 + Ny^2 = 1

    // 식1 을 식2 에 대입해서 이차방정식으로 풀수 있다.
    // 접평면의 Nx 에 대한 판별식
    float d = r2 * y2 - (y2 + x2) * (r2 - x2);

    if (d <= 0.001f) {
        xmin = desc.outputRect.x;
        xmax = desc.outputRect.x + desc.outputRect.w;
    } else {
        d = BE1::Math::Sqrt(d);

        // If the sphere is behind the camera..
        if (localOrigin.x < 0) {
            d = -d;
        }

        // planeNormal1 = left plane
        planeNormal1.x = (sphere.radius * -localOrigin.y + d) / (y2 + x2);
        planeNormal1.y = (sphere.radius - planeNormal1.x * -localOrigin.y) / -localOrigin.x;
        planeNormal1.z = 0.0f;

        // planeNormal2 = right plane
        planeNormal2.x = (sphere.radius * -localOrigin.y - d) / (y2 + x2);
        planeNormal2.y = (sphere.radius - planeNormal2.x * -localOrigin.y) / -localOrigin.x;
        planeNormal2.z = 0.0f;

        float pz1 = -localOrigin.x - sphere.radius * planeNormal1.y;
        float pz2 = -localOrigin.x - sphere.radius * planeNormal2.y;

        if (pz1 >= 0 && pz2 >= 0) {
            return false;
        }

        float e = BE1::Math::Tan(DEG2RAD(desc.fovX * 0.5f));

        if (pz1 < 0) {
            x = planeNormal1.y / (planeNormal1.x * e);
            BE1::Clamp(x, -1.0f, 1.0f);
            int vx = desc.outputRect.x + (x + 1.0f) * desc.outputRect.w * 0.5f;

            xmin = BE1::Max(desc.outputRect.x, vx);
        } else {
            xmin = desc.outputRect.x;
        }

        if (pz2 < 0) {
            x = planeNormal2.y / (planeNormal2.x * e);
            BE1::Clamp(x, -1.0f, 1.0f);
            int vx = desc.outputRect.x + (x + 1.0f) * desc.outputRect.w * 0.5f;

            xmax = BE1::Min(desc.outputRect.x + desc.outputRect.w, vx);
        } else {
            xmax = desc.outputRect.x + desc.outputRect.w;
        }

        if (xmax - xmin <= 0) {
            return false;
        }
    }

    // 가로 접평면 T 는 y 축에 평행하고 카메라(C) 를 지나므로,
    // T = (Nx, 0, Nz, 0) 이면
    //
    // 1) T dot L = r 
    // 2) Nx^2 + Nz^2 = 1

    // 접평면의 Nz 에 대한 판별식
    d = r2 * z2 - (z2 + x2) * (r2 - x2);

    if (d <= 0.001f) {
        ymin = desc.outputRect.y;
        ymax = desc.outputRect.y + desc.outputRect.h;
    } else {
        d = BE1::Math::Sqrt(d);

        // If the sphere is behind the camera..
        if (localOrigin.x < 0) {
            d = -d;
        }

        // planeNormal1 = upper plane
        planeNormal1.z = (sphere.radius * localOrigin.z - d) / (z2 + x2);
        planeNormal1.x = (sphere.radius - planeNormal1.z * localOrigin.z) / -localOrigin.x;
        planeNormal1.y = 0.0f;

        // planeNormal2 = bottom plane
        planeNormal2.z = (sphere.radius * localOrigin.z + d) / (z2 + x2);
        planeNormal2.x = (sphere.radius - planeNormal2.z * localOrigin.z) / -localOrigin.x;
        planeNormal2.y = 0.0f;

        float pz1 = -localOrigin.x - sphere.radius * planeNormal1.x;
        float pz2 = -localOrigin.x - sphere.radius * planeNormal2.x;

        if (pz1 >= 0 && pz2 >= 0) {
            return false;
        }

        float e = BE1::Math::Tan(DEG2RAD(desc.fovY * 0.5f));

        if (pz1 < 0) {
            y = planeNormal1.x / (planeNormal1.z * e);
            BE1::Clamp(y, -1.0f, 1.0f);
            int vy = desc.outputRect.y + (1.0f - y) * desc.outputRect.h * 0.5f;

            ymin = BE1::Max(desc.outputRect.y, vy);
        } else {
            ymin = desc.outputRect.y;
        }

        if (pz2 < 0) {
            y = planeNormal2.x / (planeNormal2.z * e);
            BE1::Clamp(y, -1.0f, 1.0f);
            int vy = desc.outputRect.y + (1.0f - y) * desc.outputRect.h * 0.5f;

            ymax = BE1::Min(desc.outputRect.y + desc.outputRect.h, vy);
        } else {
            ymax = desc.outputRect.y + desc.outputRect.h;
        }

        if (ymax - ymin <= 0) {
            return false;
        }
    }

    clipRect.x = xmin;
    clipRect.y = ymin;
    clipRect.w = xmax - xmin;
    clipRect.h = ymax - ymin;

    return true;
}

bool RenderCamera::CalcClipRectFromAABB(const BE1::AABB &aabb, BE1::Rect &clipRect) const {
    return CalcClipRectFromOBB(BE1::OBB(aabb, BE1::Vec3::origin, BE1::Mat3::identity), clipRect);
}

bool RenderCamera::CalcClipRectFromOBB(const BE1::OBB &obb, BE1::Rect &clipRect) const {
    BE1::AABB bounds;

    if (desc.orthogonal) {
        if (!box.ProjectionBounds(obb, bounds)) {
            return false;
        }
    } else {
        if (!frustum.ProjectionBounds(obb, bounds)) {
            return false;
        }
    }

    if (bounds[1][1] - bounds[0][1] <= 0) {
        return false;
    }

    if (bounds[1][2] - bounds[0][2] <= 0) {
        return false;
    }

    clipRect.x = desc.outputRect.x + (-bounds[1][1] + 1) * desc.outputRect.w * 0.5f;
    clipRect.y = desc.outputRect.y + (-bounds[1][2] + 1) * desc.outputRect.h * 0.5f;
    clipRect.w = desc.outputRect.w * 0.5f * (bounds[1][1] - bounds[0][1]);
    clipRect.h = desc.outputRect.h * 0.5f * (bounds[1][2] - bounds[0][2]);

    return true;
}

bool RenderCamera::CalcClipRectFromFrustum(const BE1::Frustum &frustum, BE1::Rect &clipRect) const {
    BE1::AABB bounds;

    if (desc.orthogonal) {
        if (!this->box.ProjectionBounds(frustum, bounds)) {
            return false;
        }
    } else {
        // CHECK: is valid AABB ?
        if (!this->frustum.ProjectionBounds(frustum, bounds)) {
            return false;
        }
    }

    if (bounds[1][1] - bounds[0][1] <= 0) {
        return false;
    }

    if (bounds[1][2] - bounds[0][2] <= 0) {
        return false;
    }

    clipRect.x = desc.outputRect.x + (-bounds[1][1] + 1) * desc.outputRect.w * 0.5f;
    clipRect.y = desc.outputRect.y + (-bounds[1][2] + 1) * desc.outputRect.h * 0.5f;
    clipRect.w = desc.outputRect.w * 0.5f * (bounds[1][1] - bounds[0][1]);
    clipRect.h = desc.outputRect.h * 0.5f * (bounds[1][2] - bounds[0][2]);

    return true;
}

bool RenderCamera::CalcDepthBoundsFromPoints(int numPoints, const BE1::Vec3 *points, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const {
    float localMin = BE1::Math::Infinity;
    float localMax = -BE1::Math::Infinity;

    for (int i = 0; i < numPoints; ++i) {
        const BE1::Vec3 &v = points[i];

        float cz = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2] + mvp[2][3];
        float cw = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2] + mvp[3][3];

        if (cw > BE1::Math::FloatSmallestNonDenormal) {
            cz = cz / cw;
        } else {
            cz = -1.0f;
        }

        localMin = BE1::Min(localMin, cz);
        localMax = BE1::Max(localMax, cz);
    }

    if (localMin <= localMax && (localMin >= -1.0 || localMax <= 1.0)) {
        *depthMin = localMin * 0.5f + 0.5f;
        *depthMax = localMax * 0.5f + 0.5f;
        return true;
    }

    return false;
}

bool RenderCamera::CalcDepthBoundsFromSphere(const BE1::Sphere &sphere, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const {
    BE1::Vec3 points[2];
    points[0] = sphere.center + desc.axis[0] * sphere.radius;
    points[1] = sphere.center - desc.axis[0] * sphere.radius;
    return CalcDepthBoundsFromPoints(2, points, mvp, depthMin, depthMax);
}

bool RenderCamera::CalcDepthBoundsFromAABB(const BE1::AABB &aabb, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const {
    BE1::Vec3 points[8];
    aabb.ToPoints(points);
    return CalcDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

bool RenderCamera::CalcDepthBoundsFromOBB(const BE1::OBB &obb, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const {
    BE1::Vec3 points[8];
    obb.ToPoints(points);
    return CalcDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

bool RenderCamera::CalcDepthBoundsFromFrustum(const BE1::Frustum &frustum, const BE1::Mat4 &mvp, float *depthMin, float *depthMax) const {
    BE1::Vec3 points[8];
    frustum.ToPoints(points);
    return CalcDepthBoundsFromPoints(8, points, mvp, depthMin, depthMax);
}

void RenderCamera::CalculateFov(float fromFovX, float fromAspectRatio, float toAspectRatio, float *toFovX, float *toFovY) {
    float tanFovX = BE1::Math::Tan(DEG2RAD(fromFovX * 0.5f));
    float tanFovY = tanFovX / fromAspectRatio;

    *toFovX = RAD2DEG(BE1::Math::ATan(tanFovY * toAspectRatio) * 2.0f);
    *toFovY = RAD2DEG(BE1::Math::ATan(tanFovY) * 2.0f);
}

void RenderCamera::EnsureRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, const BE1::Color4 &optimizedClearColor) {
    sampleCount = BE1::Max(sampleCount, 1u);

    if (colorTexture &&
        depthTexture &&
        renderWidth == width &&
        renderHeight == height &&
        renderColorFormat == colorFormat &&
        renderDepthFormat == depthFormat &&
        renderSampleCount == sampleCount) {
        return;
    }

    DestroyPostProcessColorTexture();
    DestroyPyramidBloomTextures();

    if (!CreateRenderTextures(width, height, colorFormat, depthFormat, sampleCount, optimizedClearColor)) {
        DestroyRenderTextures();
        return;
    }
}

void RenderCamera::EnsurePostProcessResources() {
    if (BE1::HasFlag(desc.flags, RenderCameraFlag::SkipPostProcess) || r_skipPostProcess.GetBool() || !r_toneMapping.GetBool() || renderWidth == 0 || renderHeight == 0) {
        DestroyPostProcessResources();
        return;
    }

    if (!postProcessColorTexture && !CreatePostProcessColorTexture(renderWidth, renderHeight)) {
        DestroyPostProcessResources();
        return;
    }

    const RenderBloomMode bloomMode = static_cast<RenderBloomMode>(BE1::Clamp(r_bloom.GetInteger(), static_cast<int>(RenderBloomMode::Off), static_cast<int>(RenderBloomMode::FFTConvolution)));
    if (bloomMode == RenderBloomMode::Pyramid) {
        const uint32_t requestedBloomMipMaxLevelCount = static_cast<uint32_t>(BE1::Clamp(r_bloomMipCount.GetInteger(), 1, RenderBloomDownMaxLevelCount));
        const bool hasReusablePyramidBloomTextures = bloomDownTexture &&
            bloomUpTexture &&
            bloomMipCount > 0 &&
            bloomMipMaxLevelCount == requestedBloomMipMaxLevelCount;

        if (!hasReusablePyramidBloomTextures) {
            if (!CreatePyramidBloomTextures(renderWidth, renderHeight, requestedBloomMipMaxLevelCount)) {
                DestroyPyramidBloomTextures();
            }
        }
    } else {
        DestroyPyramidBloomTextures();
    }

    if (!luminanceBuffer) {
        // Tonemapping 에 사용할 Luminance 버퍼 생성
        CreateLuminanceBuffer();
    }
}

void RenderCamera::DestroyPostProcessResources() {
    DestroyLuminanceBuffer();
    DestroyPostProcessColorTexture();
    DestroyPyramidBloomTextures();
}

bool RenderCamera::CreateRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, const BE1::Color4 &optimizedClearColor) {
    DestroyRenderTextures();

    if (width == 0 || height == 0) {
        return false;
    }

    sampleCount = BE1::Max(sampleCount, 1u);

    BE1::Image::Format colorImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(colorFormat, &colorImageFormat, nullptr);

    const RHI::ClearValue colorOptimizedClearValue = RHI::ClearValue::Color(optimizedClearColor[0], optimizedClearColor[1], optimizedClearColor[2], optimizedClearColor[3]);

    SharedPtr<BE1::Image> colorImage = MakeShared<BE1::Image>();
    colorImage->InitFromMemory(width, height, 1, 1, 1, colorImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    colorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, colorImage, false, colorOptimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
    if (!colorTexture) {
        BE_WARNLOG("RenderCamera::CreateRenderTextures: failed to create color texture\n");
        return false;
    }
    RHI::renderer->SetName(colorTexture, "RenderCamera::colorTexture");

    if (sampleCount > 1) {
        colorMSAATexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource, colorImage, false, colorOptimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
        if (!colorMSAATexture) {
            BE_WARNLOG("RenderCamera::CreateRenderTextures: failed to create MSAA color texture\n");
            DestroyRenderTextures();
            return false;
        }
        RHI::renderer->SetName(colorMSAATexture, "RenderCamera::colorMSAATexture");
    }

    BE1::Image::Format depthImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(depthFormat, &depthImageFormat, nullptr);

    const RHI::ClearValue depthStencilOptimizedClearValue = RHI::ClearValue::DepthStencil(1.0f, 0);

    SharedPtr<BE1::Image> depthStencilImage = MakeShared<BE1::Image>();
    depthStencilImage->InitFromMemory(width, height, 1, 1, 1, depthImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    depthTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless, depthStencilImage, false, depthStencilOptimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
    if (!depthTexture) {
        BE_WARNLOG("RenderCamera::CreateRenderTextures: failed to create depth texture\n");
        DestroyRenderTextures();
        return false;
    }
    RHI::renderer->SetName(depthTexture, "RenderCamera::depthTexture");

    if (!RHI::renderer->CreateView(depthTexture, RHI::ViewType::SRV).IsValid() ||
        !RHI::renderer->CreateView(depthTexture, RHI::ViewType::DSV).IsValid()) {
        BE_WARNLOG("RenderCamera::CreateRenderTextures: failed to create depth texture views\n");
        DestroyRenderTextures();
        return false;
    }

    renderWidth = width;
    renderHeight = height;
    renderColorFormat = colorFormat;
    renderDepthFormat = depthFormat;
    renderSampleCount = sampleCount;

    return true;
}

void RenderCamera::DestroyRenderTextures() {
    if (colorTexture) {
        RHI::renderer->DestroyTexture(colorTexture);
        colorTexture = nullptr;
    }
    if (colorMSAATexture) {
        RHI::renderer->DestroyTexture(colorMSAATexture);
        colorMSAATexture = nullptr;
    }
    if (depthTexture) {
        RHI::renderer->DestroyTexture(depthTexture);
        depthTexture = nullptr;
    }

    renderWidth = 0;
    renderHeight = 0;
    renderColorFormat = RHI::Format::Unknown;
    renderDepthFormat = RHI::Format::Unknown;
    renderSampleCount = 1;
}

bool RenderCamera::CreatePostProcessColorTexture(uint32_t width, uint32_t height) {
    DestroyPostProcessColorTexture();

    if (width == 0 || height == 0) {
        return false;
    }

    BE1::Image::Format postProcessColorImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(RHI::Format::R8G8B8A8_UNORM, &postProcessColorImageFormat, nullptr);

    SharedPtr<BE1::Image> postProcessColorImage = MakeShared<BE1::Image>();
    postProcessColorImage->InitFromMemory(width, height, 1, 1, 1, postProcessColorImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    postProcessColorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, postProcessColorImage, false, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!postProcessColorTexture) {
        BE_WARNLOG("RenderCamera::CreatePostProcessColorTexture: failed to create post process color texture\n");
        return false;
    }

    RHI::renderer->SetName(postProcessColorTexture, "RenderCamera::postProcessColorTexture");
    return true;
}

void RenderCamera::DestroyPostProcessColorTexture() {
    if (postProcessColorTexture) {
        RHI::renderer->DestroyTexture(postProcessColorTexture);
        postProcessColorTexture = nullptr;
    }
}

bool RenderCamera::CreatePyramidBloomTextures(uint32_t width, uint32_t height, uint32_t requestedBloomMipMaxLevelCount) {
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

    bloomDownTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, bloomTextureFlags, bloomDownImage, false, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!bloomDownTexture) {
        BE_WARNLOG("RenderCamera::CreatePyramidBloomTextures: failed to create bloom down texture\n");
        return false;
    }
    RHI::renderer->SetName(bloomDownTexture, "RenderCamera::bloomDownTexture");

    // bloomUpTexture 는 upsample pass 의 출력 mip 만 필요하므로 가장 작은 source-only mip 은 만들지 않는다.
    SharedPtr<BE1::Image> bloomUpImage = MakeShared<BE1::Image>();
    bloomUpImage->InitFromMemory(bloomMipWidths[0], bloomMipHeights[0], 1, 1, bloomUpMipCount, bloomImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    bloomUpTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, bloomTextureFlags, bloomUpImage, false, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!bloomUpTexture) {
        BE_WARNLOG("RenderCamera::CreatePyramidBloomTextures: failed to create bloom up texture\n");
        DestroyPyramidBloomTextures();
        return false;
    }
    RHI::renderer->SetName(bloomUpTexture, "RenderCamera::bloomUpTexture");

    auto createBloomMipViews = [](RHI::Texture *texture, RHI::ViewID *srvViewIDs, RHI::ViewID *uavViewIDs, uint32_t levelCount, const char *debugName) -> bool {
        for (uint32_t mipLevel = 0; mipLevel < levelCount; ++mipLevel) {
            srvViewIDs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::SRV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
            if (!srvViewIDs[mipLevel].IsValid()) {
                BE_WARNLOG("RenderCamera::CreatePyramidBloomTextures: failed to create %s mip %i SRV\n", debugName, mipLevel);
                return false;
            }

            uavViewIDs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::UAV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
            if (!uavViewIDs[mipLevel].IsValid()) {
                BE_WARNLOG("RenderCamera::CreatePyramidBloomTextures: failed to create %s mip %i UAV\n", debugName, mipLevel);
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

void RenderCamera::DestroyPyramidBloomTextures() {
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

void RenderCamera::CreateLuminanceBuffer() {
    DestroyLuminanceBuffer();

    // Luminance buffer : exposure (float) + luminance (float) + histogram (uint32_t * 1024)
    static constexpr uint32_t LuminanceBufferSize = LuminanceBufferOffsetHistogram + LuminanceHistogramBinCount * sizeof(uint32_t);
    static constexpr uint32_t LuminanceBufferDwordCount = LuminanceBufferSize / sizeof(uint32_t);
    const uint32_t initialLuminanceBufferData[LuminanceBufferDwordCount] = {};

    luminanceBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, LuminanceBufferSize, initialLuminanceBufferData);
    if (!luminanceBuffer) {
        BE_WARNLOG("RenderCamera::CreateLuminanceBuffer: failed to create luminance buffer\n");
        return;
    }

    RHI::renderer->SetName(luminanceBuffer, "RenderCamera::luminanceBuffer");

    RHI::ViewID srvViewID = RHI::renderer->CreateView(luminanceBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, 0, 0, LuminanceBufferSize);
    if (!srvViewID.IsValid()) {
        BE_WARNLOG("RenderCamera::CreateLuminanceBuffer: failed to create luminance SRV\n");
        RHI::renderer->DestroyBuffer(luminanceBuffer);
        luminanceBuffer = nullptr;
        return;
    }

    RHI::ViewID uavViewID = RHI::renderer->CreateView(luminanceBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, 0, 0, LuminanceBufferSize);
    if (!uavViewID.IsValid()) {
        BE_WARNLOG("RenderCamera::CreateLuminanceBuffer: failed to create luminance UAV\n");
        RHI::renderer->DestroyBuffer(luminanceBuffer);
        luminanceBuffer = nullptr;
        return;
    }
}

void RenderCamera::DestroyLuminanceBuffer() {
    if (luminanceBuffer) {
        RHI::renderer->DestroyBuffer(luminanceBuffer);
        luminanceBuffer = nullptr;
    }
}
