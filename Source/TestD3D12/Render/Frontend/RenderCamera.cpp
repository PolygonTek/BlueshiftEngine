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
#include "Render/RenderCvars.h"
#include "Render/RenderUtils.h"
#include "Math/Hammersley.h"
#include "RHI/RHI.h"
#include "Shaders/ShaderInterop.h"
#include "Render/Texture/TextureManager.h"

RenderCamera::~RenderCamera() {
    DestroyPostProcessResources();
    DestroyHiZResources();
    DestroyRenderTextures();
}

void RenderCamera::Update(const Desc &desc) {
    this->desc = desc;

    const bool zNegativeOneToOne = desc.projectionPolicy.IsZNegativeOneToOne();
    const bool reverseZ = desc.projectionPolicy.reverseZ;

    SetViewMatrixFromEngineAxis(desc.axis, desc.origin, viewMatrix);

    if (desc.orthogonal) {
        // Set bounding volume for orthogonal view.
        BE1::Vec3 extents((desc.zFar - desc.zNear) * 0.5f, desc.sizeX, desc.sizeY);
        box.SetCenter(desc.origin + desc.axis[0] * (desc.zNear + extents[0]));
        box.SetExtents(extents);
        box.SetAxis(desc.axis);

        // 직교 투영 행렬을 계산한다. viewMatrix 가 오른손 좌표계이므로, 오른손 좌표계용 투영 행렬을 계산한다.
        projMatrix.SetOrthoRH(-desc.sizeX, desc.sizeX, -desc.sizeY, desc.sizeY, desc.zNear, desc.zFar, zNegativeOneToOne, reverseZ);
    } else {
        const float tanHalfFovX = BE1::Math::Tan(DEG2RAD(desc.fovX * 0.5f));
        const float tanHalfFovY = BE1::Math::Tan(DEG2RAD(desc.fovY * 0.5f));
        const float aspectRatio = tanHalfFovX / tanHalfFovY;

        // Set bounding frustum for perspective view.
        frustum.SetOrigin(desc.origin);
        frustum.SetAxis(desc.axis);
        frustum.SetSize(desc.zNear, desc.zFar, desc.zFar * tanHalfFovX, desc.zFar * tanHalfFovY);

        // Calculate view frustum planes.
        frustum.ToPlanes(frustumPlanes);

        // Calculate view frustum points.
        frustum.ToPoints(frustumPoints);

        // 원근 투영 행렬을 계산한다. viewMatrix 가 오른손 좌표계이므로, 오른손 좌표계용 투영 행렬을 계산한다.
        projMatrix.SetPerspectiveRH(desc.fovY, aspectRatio, desc.zNear, desc.zFar, zNegativeOneToOne, reverseZ, desc.infiniteZFar);
    }

    viewProjMatrix = projMatrix * viewMatrix;
    invViewProjMatrix = viewProjMatrix.Inverse();
    jitteredViewProjMatrix = viewProjMatrix;
    jitteredInvViewProjMatrix = invViewProjMatrix;
}

bool RenderCamera::Cull(const BE1::AABB &bounds) const {
    if (desc.orthogonal) {
        return !box.IsIntersectAABB(bounds);
    }

    return frustum.CullAABB(bounds);
}

bool RenderCamera::Cull(const BE1::OBB &bounds) const {
    if (desc.orthogonal) {
        return !box.IsIntersectOBB(bounds);
    }

    return frustum.CullOBB(bounds);
}

bool RenderCamera::Cull(const BE1::Sphere &bounds) const {
    if (desc.orthogonal) {
        return !box.IsIntersectSphere(bounds);
    }

    return frustum.CullSphere(bounds);
}

bool RenderCamera::Cull(const BE1::Frustum &bounds) const {
    if (desc.orthogonal) {
        return bounds.CullOBB(box);
    }

    return frustum.CullFrustum(bounds);
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
void RenderCamera::CalcConvolutionBloomKernelParams(const BE1::Image &kernelImage, BE1::Vec2 &outNormalizedKernelExtents, float &outInverseAverageLuminance) {
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

    const float kernelExtentX = BE1::Max(BE1::Math::Fabs(minX), BE1::Math::Fabs(maxX)) + texelSizeX;
    const float kernelExtentY = BE1::Max(BE1::Math::Fabs(minY), BE1::Math::Fabs(maxY)) + texelSizeY;

    // normalized extents 를 계산한다.
    outNormalizedKernelExtents = BE1::Vec2(BE1::Min(kernelExtentX, 1.0f), BE1::Min(kernelExtentY, 1.0f));

    // 평균 luminance 의 역수
    outInverseAverageLuminance = 1.0f / averageLuminance;
}

bool RenderCamera::SetConvolutionBloomKernelFromImage(const char *textureName, const BE1::Image &kernelImage) {
    if (kernelImage.IsEmpty()) {
        return false;
    }
    if (textureManager.FindTexture(textureName)) {
        BE_WARNLOG("RenderCamera::SetConvolutionBloomKernelFromImage: texture '%s' already exists\n", textureName);
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

bool RenderCamera::SetConvolutionBloomKernelFromFile(const char *filename) {
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

void RenderCamera::SetConvolutionBloomKernelTexture(const SharedPtr<Texture> &texture, const BE1::Vec2 &normalizedKernelExtents, float inverseAverageLuminance) {
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

void RenderCamera::SetConvolutionBloomKernelRotation(float rotation) {
    if (fftBloomKernelRotation == rotation) {
        return;
    }

    fftBloomKernelRotation = rotation;
    fftBloomKernelDirty = true;
}

void RenderCamera::SetConvolutionBloomKernelScale(float scale) {
    scale = BE1::Max(scale, 0.01f);
    if (fftBloomKernelScale == scale) {
        return;
    }

    fftBloomKernelScale = scale;
    fftBloomKernelDirty = true;
}

void RenderCamera::SetConvolutionBloomPaddingTrim(float paddingTrim) {
    paddingTrim = BE1::Min(BE1::Max(paddingTrim, 0.0f), 0.5f);
    if (fftBloomPaddingTrim == paddingTrim) {
        return;
    }

    fftBloomPaddingTrim = paddingTrim;
    fftBloomKernelDirty = true;
}

BE1::Vec2 RenderCamera::CalcNormalizedKernelExtents() const {
    // 커널의 회전 각도에 따른 sin, cos 계산
    float rotationSin;
    float rotationCos;
    BE1::Math::SinCos(DEG2RAD(fftBloomKernelRotation), rotationSin, rotationCos);

    const float absCos = BE1::Math::Fabs(rotationCos);
    const float absSin = BE1::Math::Fabs(rotationSin);

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
void RenderCamera::UpdateConvolutionBloomMapping() {
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

static float NDCZToDeviceDepth(float ndcZ, const RHI::ProjectionPolicy &projectionPolicy) {
    return projectionPolicy.IsZNegativeOneToOne() ? ndcZ * 0.5f + 0.5f : ndcZ;
}

static float DeviceDepthToNDCZ(float depth, const RHI::ProjectionPolicy &projectionPolicy) {
    return projectionPolicy.IsZNegativeOneToOne() ? depth * 2.0f - 1.0f : depth;
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
    float fz = NDCZToDeviceDepth(normalizedDeviceCoords.z, desc.projectionPolicy); // Valid range is [0, 1]

    pixelCoords.x = desc.outputRect.x + fx * desc.outputRect.w;
    pixelCoords.y = desc.outputRect.y + fy * desc.outputRect.h;
    pixelCoords.z = fz; // device depth value
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
    float fz = pixelCoords.z; // device depth value

    normalizedDeviceCoords.x = (fx * 2.0f) - 1.0f;
    normalizedDeviceCoords.y = (fy * 2.0f) - 1.0f;
    normalizedDeviceCoords.z = DeviceDepthToNDCZ(fz, desc.projectionPolicy);
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
            x = BE1::Clamp(x, -1.0f, 1.0f);
            int vx = desc.outputRect.x + (x + 1.0f) * desc.outputRect.w * 0.5f;

            xmin = BE1::Max(desc.outputRect.x, vx);
        } else {
            xmin = desc.outputRect.x;
        }

        if (pz2 < 0) {
            x = planeNormal2.y / (planeNormal2.x * e);
            x = BE1::Clamp(x, -1.0f, 1.0f);
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
            y = BE1::Clamp(y, -1.0f, 1.0f);
            int vy = desc.outputRect.y + (1.0f - y) * desc.outputRect.h * 0.5f;

            ymin = BE1::Max(desc.outputRect.y, vy);
        } else {
            ymin = desc.outputRect.y;
        }

        if (pz2 < 0) {
            y = planeNormal2.x / (planeNormal2.z * e);
            y = BE1::Clamp(y, -1.0f, 1.0f);
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

    bounds[0][1] = BE1::Clamp(bounds[0][1], -1.0f, 1.0f);
    bounds[1][1] = BE1::Clamp(bounds[1][1], -1.0f, 1.0f);
    bounds[0][2] = BE1::Clamp(bounds[0][2], -1.0f, 1.0f);
    bounds[1][2] = BE1::Clamp(bounds[1][2], -1.0f, 1.0f);

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

    bounds[0][1] = BE1::Clamp(bounds[0][1], -1.0f, 1.0f);
    bounds[1][1] = BE1::Clamp(bounds[1][1], -1.0f, 1.0f);
    bounds[0][2] = BE1::Clamp(bounds[0][2], -1.0f, 1.0f);
    bounds[1][2] = BE1::Clamp(bounds[1][2], -1.0f, 1.0f);

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
    const float ndcMin = desc.projectionPolicy.IsZNegativeOneToOne() ? -1.0f : 0.0f;
    const float ndcMax = 1.0f;
    const float clippedNearZ = desc.projectionPolicy.reverseZ ? ndcMax : ndcMin;

    float localMin = BE1::Math::BigNumber;
    float localMax = -BE1::Math::BigNumber;

    for (int i = 0; i < numPoints; ++i) {
        const BE1::Vec3 &v = points[i];

        float cz = v[0] * mvp[2][0] + v[1] * mvp[2][1] + v[2] * mvp[2][2] + mvp[2][3];
        float cw = v[0] * mvp[3][0] + v[1] * mvp[3][1] + v[2] * mvp[3][2] + mvp[3][3];

        if (cw > std::numeric_limits<float>::min()) {
            cz = cz / cw;
        } else {
            cz = clippedNearZ;
        }

        localMin = BE1::Min(localMin, cz);
        localMax = BE1::Max(localMax, cz);
    }

    if (localMax < ndcMin || localMin > ndcMax) {
        return false;
    }

    localMin = BE1::Max(localMin, ndcMin);
    localMax = BE1::Min(localMax, ndcMax);

    *depthMin = NDCZToDeviceDepth(localMin, desc.projectionPolicy);
    *depthMax = NDCZToDeviceDepth(localMax, desc.projectionPolicy);
    return true;
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

// needDepthHistory 가 꺼진 경우
// - depthTexture : MSAA sample count 를 따르는 DSV / SRV
// - depthHistoryTextures : 없음
//
// needDepthHistory 가 켜지고, MSAA 가 꺼진 경우
// - depthTexture : 없음
// - depthHistoryTextures : DSV / SRV 두 개를 ping-pong depth attachment 로 사용
//
// needDepthHistory 가 켜지고, MSAA 가 켜진 경우
// - depthTexture : MSAA DSV / SRV
// - depthHistoryTextures : SRV / UAV 두 개를 ping-pong 하며 MSAA depthTexture 의 resolve 결과를 저장
bool RenderCamera::EnsureRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, bool needDepthHistory, const BE1::Color4 &optimizedClearColor) {
    sampleCount = BE1::Max(sampleCount, 1u);

    const bool useDepthHistoryRenderTarget = needDepthHistory && sampleCount == 1;
    const bool hasExpectedDepthTextures = useDepthHistoryRenderTarget ? (depthHistoryAsRenderTarget && depthHistoryTextures[0] && depthHistoryTextures[1] && !depthTexture) : (!depthHistoryAsRenderTarget && depthTexture);
    const bool hasExpectedMSAASurfaceTextures = sampleCount > 1 ? (rtNormalMSAATexture && rtVelocityMSAATexture) : (!rtNormalMSAATexture && !rtVelocityMSAATexture);

    if (rtColorTexture &&
        hasExpectedDepthTextures &&
        rtNormalTexture &&
        rtVelocityTexture &&
        hasExpectedMSAASurfaceTextures &&
        renderWidth == width &&
        renderHeight == height &&
        renderColorFormat == colorFormat &&
        renderDepthFormat == depthFormat &&
        renderSampleCount == sampleCount) {
        return true;
    }

    DestroyPostProcessColorTexture();
    DestroyColorHistoryTextures();
    DestroyDepthHistoryTextures();
    DestroyPyramidBloomTextures();
    DestroyConvolutionBloomResources();
    DestroyConvolutionBloomOutputTexture();

    if (!CreateRenderTextures(width, height, colorFormat, depthFormat, sampleCount, needDepthHistory, optimizedClearColor)) {
        DestroyRenderTextures();
        return false;
    }

    return true;
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

void RenderCamera::EnsurePostProcessResources(bool skipPostProcess, RHI::Format colorFormat, bool needColorHistory, bool needDepthHistory, RenderBloomMode bloomMode) {
    if (skipPostProcess) {
        DestroyPostProcessResources();
        return;
    }

    if (renderWidth == 0 || renderHeight == 0) {
        DestroyPostProcessResources();
        return;
    }

    if ((!postProcessColorTexture || postProcessColorTexture->GetFormat() != colorFormat) && !CreatePostProcessColorTexture(renderWidth, renderHeight, colorFormat)) {
        DestroyPostProcessResources();
        return;
    }

    if (needColorHistory) {
        if (!EnsureColorHistoryTextures(renderWidth, renderHeight, renderColorFormat)) {
            DestroyColorHistoryTextures();
        }
    } else {
        DestroyColorHistoryTextures();
    }

    if (needDepthHistory) {
        if (!depthHistoryAsRenderTarget && !EnsureDepthHistoryTextures(renderWidth, renderHeight)) {
            DestroyDepthHistoryTextures();
        }
    } else {
        if (!depthHistoryAsRenderTarget) {
            DestroyDepthHistoryTextures();
        }
    }

    const bool useConvolutionBloom = bloomMode == RenderBloomMode::Convolution;
    if (bloomMode == RenderBloomMode::Pyramid) {
        DestroyConvolutionBloomResources();
        DestroyConvolutionBloomOutputTexture();

        const uint32_t requestedBloomMipMaxLevelCount = static_cast<uint32_t>(BE1::Clamp(r_pyramidBloomMipCount.GetInteger(), 1, RenderBloomDownMaxLevelCount));
        const bool hasReusablePyramidBloomTextures = bloomDownTexture && bloomUpTexture && bloomMipCount > 0 && bloomMipMaxLevelCount == requestedBloomMipMaxLevelCount;

        if (!hasReusablePyramidBloomTextures) {
            if (!CreatePyramidBloomTextures(renderWidth, renderHeight, requestedBloomMipMaxLevelCount)) {
                DestroyPyramidBloomTextures();
            }
        }
    } else if (useConvolutionBloom) {
        DestroyPyramidBloomTextures();

        const uint32_t convolutionBloomFFTMaxSize = static_cast<uint32_t>(BE1::Clamp(r_convolutionBloomFFTMaxSize.GetInteger(), 256, 1024));
        const uint32_t requestedFFTBloomSize = SelectBloomFFTSize(renderWidth, renderHeight, convolutionBloomFFTMaxSize);
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
    } else {
        DestroyPyramidBloomTextures();
        DestroyConvolutionBloomResources();
        DestroyConvolutionBloomOutputTexture();
    }

    if (!luminanceBuffer) {
        // Tonemapping 에 사용할 Luminance 버퍼 생성
        CreateLuminanceBuffer();
    }
}

void RenderCamera::InvalidateTemporalHistory() {
    temporalHistoryInvalidated = true;
    temporalFrameIndex = 0;
    temporalJitterSampleCount = 0;
    temporalJitterActive = false;
    historyValid = false;

    currentJitter = BE1::Vec2::zero;
    previousJitter = BE1::Vec2::zero;
    previousJitteredViewProjMatrix = viewProjMatrix;
    previousJitteredInvViewProjMatrix = invViewProjMatrix;
    previousHiZValid = false;
}

void RenderCamera::UpdateTemporalJitter(bool enableJitter, uint32_t sampleCount) {
    static constexpr uint32_t MaxTemporalJitterSampleCount = 64;

    const bool requestedJitter = enableJitter && sampleCount > 1;
    const uint32_t requestedSampleCount = requestedJitter ? BE1::Min(sampleCount, MaxTemporalJitterSampleCount) : 0;
    const bool jitterSettingChanged = temporalJitterActive != requestedJitter || temporalJitterSampleCount != requestedSampleCount;

    if (jitterSettingChanged) {
        temporalHistoryInvalidated = true;
        temporalFrameIndex = 0;
        previousJitter = BE1::Vec2::zero;
        previousJitteredViewProjMatrix = viewProjMatrix;
        previousJitteredInvViewProjMatrix = invViewProjMatrix;
    }

    temporalJitterActive = requestedJitter;
    temporalJitterSampleCount = requestedSampleCount;

    if (!requestedJitter) {
        currentJitter = BE1::Vec2::zero;
        previousJitter = BE1::Vec2::zero;
        temporalFrameIndex = 0;

        jitteredViewProjMatrix = viewProjMatrix;
        jitteredInvViewProjMatrix = invViewProjMatrix;
        return;
    }

    // 이전 프레임의 jitter offset 을 저장한다.
    previousJitter = jitterSettingChanged ? BE1::Vec2::zero : currentJitter;

    // deterministic offset 이 Vec2::zero 이므로 0 번 샘플은 (0, 0) 이다.
    // jitter 로 변환하면 (-0.5, -0.5) 이 되어 최대 offset 이 되므로 1 번 샘플부터 사용한다.
    static const BE1::Hammersley<MaxTemporalJitterSampleCount + 1> jitterSamples(2, 3, BE1::Vec2::zero);
    const uint32_t sampleIndex = (temporalFrameIndex % temporalJitterSampleCount) + 1;
    const BE1::Vec2 &sample = jitterSamples.GetSample(sampleIndex);

    currentJitter = BE1::Vec2(sample.x - 0.5f, sample.y - 0.5f);

    assert(renderWidth > 0 && renderHeight > 0);
    const float ndcJitterX = currentJitter.x * (2.0f / static_cast<float>(renderWidth));
    const float ndcJitterY = currentJitter.y * (-2.0f / static_cast<float>(renderHeight));

    jitteredViewProjMatrix = viewProjMatrix;

    // newClip.x = (row0 + row3 * ndcJitterX) dot position
    // newClip.y = (row1 + row3 * ndcJitterY) dot position
    // newMatrixRow0 = row0 + row3 * ndcJitterX
    // newMatrixRow1 = row1 + row3 * ndcJitterY
    for (int i = 0; i < 4; ++i) {
        jitteredViewProjMatrix[0][i] += viewProjMatrix[3][i] * ndcJitterX;
        jitteredViewProjMatrix[1][i] += viewProjMatrix[3][i] * ndcJitterY;
    }

    jitteredInvViewProjMatrix = jitteredViewProjMatrix.Inverse();

    temporalFrameIndex++;
}

void RenderCamera::DestroyPostProcessResources() {
    DestroyLuminanceBuffer();
    DestroyPostProcessColorTexture();
    DestroyColorHistoryTextures();
    if (!depthHistoryAsRenderTarget) {
        // depthHistoryAsRenderTarget == false 일 때만 depthHistoryTextures 가 post process textures 이므로, 그때만 파괴한다.
        // depthHistoryAsRenderTarget == true 라면, render textures 이므로 DestroyRenderTextures 에서 파괴된다.
        DestroyDepthHistoryTextures();
    }
    DestroyPyramidBloomTextures();
    DestroyConvolutionBloomResources();
    DestroyConvolutionBloomOutputTexture();
}

bool RenderCamera::CreateRenderTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, RHI::Format depthFormat, uint32_t sampleCount, bool needDepthHistory, const BE1::Color4 &optimizedClearColor) {
    DestroyRenderTextures();

    if (width == 0 || height == 0) {
        return false;
    }

    sampleCount = BE1::Max(sampleCount, 1u);

    if (!CreateRTColorTextures(width, height, colorFormat, sampleCount, optimizedClearColor)) {
        return false;
    }

    if (needDepthHistory && sampleCount == 1) {
        if (!CreateDepthHistoryRenderTargets(width, height, depthFormat)) {
            return false;
        }
    } else {
        if (!CreateDepthTexture(width, height, depthFormat, sampleCount)) {
            return false;
        }
    }

    if (!CreateRTNormalTextures(width, height, sampleCount)) {
        return false;
    }

    if (!CreateRTVelocityTextures(width, height, sampleCount)) {
        return false;
    }

    renderWidth = width;
    renderHeight = height;
    renderColorFormat = colorFormat;
    renderDepthFormat = depthFormat;
    renderSampleCount = sampleCount;

    return true;
}

bool RenderCamera::CreateRTColorTextures(uint32_t width, uint32_t height, RHI::Format colorFormat, uint32_t sampleCount, const BE1::Color4 &optimizedClearColor) {
    BE1::Image::Format colorImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(colorFormat, &colorImageFormat, nullptr);

    const RHI::ClearValue colorOptimizedClearValue = RHI::ClearValue::Color(optimizedClearColor[0], optimizedClearColor[1], optimizedClearColor[2], optimizedClearColor[3]);

    SharedPtr<BE1::Image> colorImage = MakeShared<BE1::Image>();
    colorImage->InitFromMemory(width, height, 1, 1, 1, colorImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    RHI::TextureFlag colorTextureFlags = RHI::TextureFlag::ShaderResource;
    if (sampleCount == 1) {
        // MSAA 가 아닌 경우에만 RenderTarget 플래그를 설정한다. MSAA 인 경우에는 직접 렌더 대상이 아니고, Resolve 만 수행되므로 RenderTarget 플래그를 설정하지 않는다.
        colorTextureFlags |= RHI::TextureFlag::RenderTarget;
    }
    rtColorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, colorTextureFlags, colorImage, RHI::TextureMipInitMode::FromImage, colorOptimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
    if (!rtColorTexture) {
        DestroyRenderTextures();
        BE_WARNLOG("RenderCamera::CreateRTColorTextures: failed to create color texture\n");
        return false;
    }
    RHI::renderer->SetName(rtColorTexture, "RenderCamera::rtColorTexture");

    if (sampleCount > 1) {
        rtColorMSAATexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget, colorImage, RHI::TextureMipInitMode::FromImage, colorOptimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
        if (!rtColorMSAATexture) {
            BE_WARNLOG("RenderCamera::CreateRTColorTextures: failed to create MSAA color texture\n");
            DestroyRenderTextures();
            return false;
        }
        RHI::renderer->SetName(rtColorMSAATexture, "RenderCamera::rtColorMSAATexture");
    }

    return true;
}

bool RenderCamera::CreateDepthTexture(uint32_t width, uint32_t height, RHI::Format depthFormat, uint32_t sampleCount) {
    BE1::Image::Format depthImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(depthFormat, &depthImageFormat, nullptr);

    const RHI::ClearValue depthStencilOptimizedClearValue = RHI::ClearValue::DepthStencil(desc.projectionPolicy.GetDepthClearValue(), 0);

    SharedPtr<BE1::Image> depthStencilImage = MakeShared<BE1::Image>();
    depthStencilImage->InitFromMemory(width, height, 1, 1, 1, depthImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    depthTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless, depthStencilImage, RHI::TextureMipInitMode::FromImage, depthStencilOptimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
    if (!depthTexture) {
        BE_WARNLOG("RenderCamera::CreateDepthTexture: failed to create depth texture\n");
        DestroyRenderTextures();
        return false;
    }
    RHI::renderer->SetName(depthTexture, "RenderCamera::depthTexture");

    if (!RHI::renderer->CreateView(depthTexture, RHI::ViewType::SRV).IsValid() ||
        !RHI::renderer->CreateView(depthTexture, RHI::ViewType::DSV).IsValid()) {
        BE_WARNLOG("RenderCamera::CreateDepthTexture: failed to create depth texture views\n");
        DestroyRenderTextures();
        return false;
    }

    return true;
}

void RenderCamera::EnsureHiZResources(uint32_t width, uint32_t height, bool needHiZTexture, bool needHiZCullResources, bool needHiZDebugPreview) {
    assert(!needHiZCullResources || needHiZTexture);
    assert(!needHiZDebugPreview || needHiZTexture);

    if (!needHiZTexture || width == 0 || height == 0) {
        DestroyHiZResources();
        return;
    }

    if (!hiZTexture || hiZTexture->GetWidth() != width || hiZTexture->GetHeight() != height) {
        DestroyHiZDebugPreviewTexture();
        DestroyHiZTexture();
        if (!CreateHiZTexture(width, height)) {
            DestroyHiZCullResources();
            return;
        }
    }

    if (needHiZCullResources) {
        if ((!hiZCullVisibleInstanceBuffer || !hiZCullIndirectArgsBuffer) && !CreateHiZCullResources()) {
            DestroyHiZCullResources();
        }
    } else {
        DestroyHiZCullResources();
    }

    if (needHiZDebugPreview) {
        EnsureHiZDebugPreviewTexture(width, height);
    } else {
        DestroyHiZDebugPreviewTexture();
    }
}

bool RenderCamera::CreateHiZTexture(uint32_t width, uint32_t height) {
    uint32_t hiZLevelCount = 0;
    uint32_t w = width;
    uint32_t h = height;

    while (hiZLevelCount < RenderHiZMaxLevelCount) {
        hiZMipWidths[hiZLevelCount] = w;
        hiZMipHeights[hiZLevelCount] = h;
        hiZLevelCount++;

        if (w == 1 && h == 1) {
            break;
        }

        w = w > 1 ? w >> 1 : 1;
        h = h > 1 ? h >> 1 : 1;
    }

    const BE1::Image::Format hiZImageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R32_FLOAT);
    SharedPtr<BE1::Image> hiZImage = MakeShared<BE1::Image>();
    hiZImage->InitFromMemory(width, height, 1, 1, hiZLevelCount, hiZImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    auto createHiZMipViews = [](RHI::Texture *texture, RHI::ViewID *srvViewIDs, RHI::ViewID *uavViewIDs, uint32_t levelCount) -> bool {
        for (uint32_t mipLevel = 0; mipLevel < levelCount; ++mipLevel) {
            srvViewIDs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::SRV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
            if (!srvViewIDs[mipLevel].IsValid()) {
                BE_WARNLOG("RenderCamera::CreateHiZTexture: failed to create Hi-Z texture mip %i SRV\n", mipLevel);
                return false;
            }

            uavViewIDs[mipLevel] = RHI::renderer->CreateView(texture, RHI::ViewType::UAV, RHI::Format::Auto, 0, texture->GetArraySize(), mipLevel, 1);
            if (!uavViewIDs[mipLevel].IsValid()) {
                BE_WARNLOG("RenderCamera::CreateHiZTexture: failed to create Hi-Z texture mip %i UAV\n", mipLevel);
                return false;
            }
        }

        return true;
    };

    hiZTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, hiZImage, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!hiZTexture) {
        BE_WARNLOG("RenderCamera::CreateHiZTexture: failed to create Hi-Z texture\n");
        DestroyHiZTexture();
        return false;
    }

    RHI::renderer->SetName(hiZTexture, "RenderCamera::hiZTexture");

    if (!createHiZMipViews(hiZTexture, hiZMipSRVs, hiZMipUAVs, hiZLevelCount)) {
        DestroyHiZTexture();
        return false;
    }
    hiZMipCount = hiZLevelCount;

    return true;
}

void RenderCamera::DestroyHiZTexture() {
    if (hiZTexture) {
        RHI::renderer->DestroyTexture(hiZTexture);
        hiZTexture = nullptr;
    }

    for (uint32_t mipLevel = 0; mipLevel < RenderHiZMaxLevelCount; ++mipLevel) {
        hiZMipSRVs[mipLevel] = RHI::ViewID::InvalidID;
        hiZMipUAVs[mipLevel] = RHI::ViewID::InvalidID;
        hiZMipWidths[mipLevel] = 0;
        hiZMipHeights[mipLevel] = 0;
    }
    hiZMipCount = 0;
    previousHiZValid = false;
}

void RenderCamera::DestroyHiZResources() {
    DestroyHiZDebugPreviewTexture();
    DestroyHiZCullResources();
    DestroyHiZTexture();
}

bool RenderCamera::EnsureHiZDebugPreviewTexture(uint32_t width, uint32_t height) {
    if (hiZDebugPreviewTexture && hiZDebugPreviewTexture->GetWidth() == width && hiZDebugPreviewTexture->GetHeight() == height) {
        return true;
    }

    DestroyHiZDebugPreviewTexture();
    return CreateHiZDebugPreviewTexture(width, height);
}

bool RenderCamera::CreateHiZDebugPreviewTexture(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) {
        return false;
    }

    BE1::Image::Format debugPreviewImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(RHI::Format::R8G8B8A8_UNORM, &debugPreviewImageFormat, nullptr);

    SharedPtr<BE1::Image> debugPreviewImage = MakeShared<BE1::Image>();
    debugPreviewImage->InitFromMemory(width, height, 1, 1, 1, debugPreviewImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    hiZDebugPreviewTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, debugPreviewImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!hiZDebugPreviewTexture) {
        BE_WARNLOG("RenderCamera::CreateHiZDebugPreviewTexture: failed to create Hi-Z debug preview texture\n");
        return false;
    }

    RHI::renderer->SetName(hiZDebugPreviewTexture, "RenderCamera::hiZDebugPreviewTexture");
    return true;
}

void RenderCamera::DestroyHiZDebugPreviewTexture() {
    if (hiZDebugPreviewTexture) {
        RHI::renderer->DestroyTexture(hiZDebugPreviewTexture);
        hiZDebugPreviewTexture = nullptr;
    }
}

bool RenderCamera::CreateHiZCullResources() {
    DestroyHiZCullResources();

    const uint64_t visibleInstanceBufferSize = uint64_t(RenderHiZCullMaxVisibleInstanceCount) * sizeof(uint32_t);
    hiZCullVisibleInstanceBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, visibleInstanceBufferSize, nullptr);
    if (!hiZCullVisibleInstanceBuffer) {
        BE_WARNLOG("RenderCamera::CreateHiZCullResources: failed to create visible instance buffer\n");
        DestroyHiZCullResources();
        return false;
    }

    RHI::renderer->SetName(hiZCullVisibleInstanceBuffer, "RenderCamera::hiZCullVisibleInstanceBuffer");

    RHI::ViewID visibleInstanceSRV = RHI::renderer->CreateView(hiZCullVisibleInstanceBuffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(uint32_t), 0, visibleInstanceBufferSize);
    if (!visibleInstanceSRV.IsValid()) {
        BE_WARNLOG("RenderCamera::CreateHiZCullResources: failed to create visible instance SRV\n");
        DestroyHiZCullResources();
        return false;
    }

    RHI::ViewID visibleInstanceUAV = RHI::renderer->CreateView(hiZCullVisibleInstanceBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, sizeof(uint32_t), 0, visibleInstanceBufferSize);
    if (!visibleInstanceUAV.IsValid()) {
        BE_WARNLOG("RenderCamera::CreateHiZCullResources: failed to create visible instance UAV\n");
        DestroyHiZCullResources();
        return false;
    }

    const uint64_t indirectArgsBufferSize = uint64_t(RenderHiZCullMaxBatchCount) * HiZCullIndirectArgSize;
    hiZCullIndirectArgsBuffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::UnorderedAccess | RHI::BufferFlag::IndirectBuffer, indirectArgsBufferSize, nullptr);
    if (!hiZCullIndirectArgsBuffer) {
        BE_WARNLOG("RenderCamera::CreateHiZCullResources: failed to create indirect args buffer\n");
        DestroyHiZCullResources();
        return false;
    }

    RHI::renderer->SetName(hiZCullIndirectArgsBuffer, "RenderCamera::hiZCullIndirectArgsBuffer");

    RHI::ViewID indirectArgsUAV = RHI::renderer->CreateView(hiZCullIndirectArgsBuffer, RHI::ViewType::UAV, RHI::Format::Unknown, 0, 0, indirectArgsBufferSize);
    if (!indirectArgsUAV.IsValid()) {
        BE_WARNLOG("RenderCamera::CreateHiZCullResources: failed to create indirect args UAV\n");
        DestroyHiZCullResources();
        return false;
    }

    return true;
}

void RenderCamera::DestroyHiZCullResources() {
    if (hiZCullVisibleInstanceBuffer) {
        RHI::renderer->DestroyBuffer(hiZCullVisibleInstanceBuffer);
        hiZCullVisibleInstanceBuffer = nullptr;
    }
    if (hiZCullIndirectArgsBuffer) {
        RHI::renderer->DestroyBuffer(hiZCullIndirectArgsBuffer);
        hiZCullIndirectArgsBuffer = nullptr;
    }
}

bool RenderCamera::CreateRTNormalTextures(uint32_t width, uint32_t height, uint32_t sampleCount) {
    BE1::Image::Format normalImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(RHI::Format::R16G16_FLOAT, &normalImageFormat, nullptr);

    const RHI::ClearValue normalOptimizedClearValue = RHI::ClearValue::Color(0.0f, 0.0f, 0.0f, 0.0f);

    SharedPtr<BE1::Image> normalImage = MakeShared<BE1::Image>();
    normalImage->InitFromMemory(width, height, 1, 1, 1, normalImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    RHI::TextureFlag normalTextureFlags = RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess;
    if (sampleCount == 1) {
        normalTextureFlags |= RHI::TextureFlag::RenderTarget;
    }
    rtNormalTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, normalTextureFlags, normalImage, RHI::TextureMipInitMode::FromImage, normalOptimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
    if (!rtNormalTexture) {
        BE_WARNLOG("RenderCamera::CreateRTNormalTextures: failed to create normal texture\n");
        DestroyRenderTextures();
        return false;
    }
    RHI::renderer->SetName(rtNormalTexture, "RenderCamera::rtNormalTexture");

    if (sampleCount > 1) {
        rtNormalMSAATexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::RenderTarget | RHI::TextureFlag::ShaderResource, normalImage, RHI::TextureMipInitMode::FromImage, normalOptimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
        if (!rtNormalMSAATexture) {
            BE_WARNLOG("RenderCamera::CreateRTNormalTextures: failed to create MSAA normal texture\n");
            DestroyRenderTextures();
            return false;
        }
        RHI::renderer->SetName(rtNormalMSAATexture, "RenderCamera::rtNormalMSAATexture");
    }

    return true;
}

bool RenderCamera::CreateRTVelocityTextures(uint32_t width, uint32_t height, uint32_t sampleCount) {
    BE1::Image::Format velocityImageFormat = BE1::Image::Format::Unknown;
    RHI::Renderer::RHIFormatToImageFormat(RHI::Format::R16G16_FLOAT, &velocityImageFormat, nullptr);

    const RHI::ClearValue velocityOptimizedClearValue = RHI::ClearValue::Color(0.0f, 0.0f, 0.0f, 0.0f);

    SharedPtr<BE1::Image> velocityImage = MakeShared<BE1::Image>();
    velocityImage->InitFromMemory(width, height, 1, 1, 1, velocityImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    RHI::TextureFlag velocityTextureFlags = RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess;
    if (sampleCount == 1) {
        velocityTextureFlags |= RHI::TextureFlag::RenderTarget;
    }
    rtVelocityTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, velocityTextureFlags, velocityImage, RHI::TextureMipInitMode::FromImage, velocityOptimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
    if (!rtVelocityTexture) {
        BE_WARNLOG("RenderCamera::CreateRTVelocityTextures: failed to create velocity texture\n");
        DestroyRenderTextures();
        return false;
    }
    RHI::renderer->SetName(rtVelocityTexture, "RenderCamera::rtVelocityTexture");

    if (sampleCount > 1) {
        rtVelocityMSAATexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::RenderTarget, velocityImage, RHI::TextureMipInitMode::FromImage, velocityOptimizedClearValue, sampleCount, RHI::GPUResourceState::ShaderResource);
        if (!rtVelocityMSAATexture) {
            BE_WARNLOG("RenderCamera::CreateRTVelocityTextures: failed to create MSAA velocity texture\n");
            DestroyRenderTextures();
            return false;
        }
        RHI::renderer->SetName(rtVelocityMSAATexture, "RenderCamera::rtVelocityMSAATexture");
    }

    return true;
}

void RenderCamera::DestroyRenderTextures() {
    if (rtColorTexture) {
        RHI::renderer->DestroyTexture(rtColorTexture);
        rtColorTexture = nullptr;
    }
    if (rtColorMSAATexture) {
        RHI::renderer->DestroyTexture(rtColorMSAATexture);
        rtColorMSAATexture = nullptr;
    }
    if (depthTexture) {
        RHI::renderer->DestroyTexture(depthTexture);
        depthTexture = nullptr;
    }
    if (rtNormalTexture) {
        RHI::renderer->DestroyTexture(rtNormalTexture);
        rtNormalTexture = nullptr;
    }
    if (rtNormalMSAATexture) {
        RHI::renderer->DestroyTexture(rtNormalMSAATexture);
        rtNormalMSAATexture = nullptr;
    }
    if (rtVelocityTexture) {
        RHI::renderer->DestroyTexture(rtVelocityTexture);
        rtVelocityTexture = nullptr;
    }
    if (rtVelocityMSAATexture) {
        RHI::renderer->DestroyTexture(rtVelocityMSAATexture);
        rtVelocityMSAATexture = nullptr;
    }
    if (depthHistoryAsRenderTarget) {
        DestroyDepthHistoryTextures();
    }

    renderWidth = 0;
    renderHeight = 0;
    renderColorFormat = RHI::Format::Unknown;
    renderDepthFormat = RHI::Format::Unknown;
    renderSampleCount = 1;
}

bool RenderCamera::CreatePostProcessColorTexture(uint32_t width, uint32_t height, RHI::Format colorFormat) {
    DestroyPostProcessColorTexture();

    if (width == 0 || height == 0 || colorFormat == RHI::Format::Unknown) {
        return false;
    }

    BE1::Image::Format postProcessColorImageFormat = BE1::Image::Format::Unknown;
    if (!RHI::Renderer::RHIFormatToImageFormat(colorFormat, &postProcessColorImageFormat, nullptr)) {
        return false;
    }

    SharedPtr<BE1::Image> postProcessColorImage = MakeShared<BE1::Image>();
    postProcessColorImage->InitFromMemory(width, height, 1, 1, 1, postProcessColorImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    postProcessColorTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, postProcessColorImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
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

bool RenderCamera::EnsureColorHistoryTextures(uint32_t width, uint32_t height, RHI::Format colorFormat) {
    if (colorHistoryTextures[0] &&
        colorHistoryTextures[1] &&
        colorHistoryTextures[0]->GetWidth() == width &&
        colorHistoryTextures[0]->GetHeight() == height &&
        colorHistoryTextures[0]->GetFormat() == colorFormat &&
        colorHistoryTextures[1]->GetWidth() == width &&
        colorHistoryTextures[1]->GetHeight() == height &&
        colorHistoryTextures[1]->GetFormat() == colorFormat) {
        return true;
    }

    return CreateColorHistoryTextures(width, height, colorFormat);
}

bool RenderCamera::CreateColorHistoryTextures(uint32_t width, uint32_t height, RHI::Format colorFormat) {
    DestroyColorHistoryTextures();

    if (width == 0 || height == 0) {
        return false;
    }

    BE1::Image::Format colorHistoryImageFormat = BE1::Image::Format::Unknown;
    if (!RHI::Renderer::RHIFormatToImageFormat(colorFormat, &colorHistoryImageFormat, nullptr)) {
        BE_WARNLOG("RenderCamera::CreateColorHistoryTextures: failed to convert color history texture format\n");
        return false;
    }

    SharedPtr<BE1::Image> colorHistoryImage = MakeShared<BE1::Image>();
    colorHistoryImage->InitFromMemory(width, height, 1, 1, 1, colorHistoryImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    for (uint32_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
        colorHistoryTextures[textureIndex] = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, colorHistoryImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!colorHistoryTextures[textureIndex]) {
            BE_WARNLOG("RenderCamera::CreateColorHistoryTextures: failed to create color history texture %i\n", textureIndex);
            DestroyColorHistoryTextures();
            return false;
        }

        RHI::renderer->SetName(colorHistoryTextures[textureIndex], BE1::va("RenderCamera::colorHistoryTextures[%i]", textureIndex));
    }

    currentHistoryTextureIndex = 0;
    historyValid = false;

    return true;
}

void RenderCamera::DestroyColorHistoryTextures() {
    for (uint32_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
        if (colorHistoryTextures[textureIndex]) {
            RHI::renderer->DestroyTexture(colorHistoryTextures[textureIndex]);
            colorHistoryTextures[textureIndex] = nullptr;
        }
    }

    currentHistoryTextureIndex = 0;
    historyValid = false;
}

bool RenderCamera::EnsureDepthHistoryTextures(uint32_t width, uint32_t height) {
    if (depthHistoryTextures[0] &&
        depthHistoryTextures[1] &&
        depthHistoryTextures[0]->GetWidth() == width &&
        depthHistoryTextures[0]->GetHeight() == height &&
        depthHistoryTextures[1]->GetWidth() == width &&
        depthHistoryTextures[1]->GetHeight() == height) {
        return true;
    }

    return CreateDepthHistoryTextures(width, height);
}

// depthHistoryTextures 를 포스트 프로세싱 용 (TAA) 으로 생성한다.
bool RenderCamera::CreateDepthHistoryTextures(uint32_t width, uint32_t height) {
    DestroyDepthHistoryTextures();

    if (width == 0 || height == 0) {
        return false;
    }

    const BE1::Image::Format depthHistoryImageFormat = RHI::renderer->ToUAVImageFormat(BE1::Image::Format::R32_FLOAT);
    SharedPtr<BE1::Image> depthHistoryImage = MakeShared<BE1::Image>();
    depthHistoryImage->InitFromMemory(width, height, 1, 1, 1, depthHistoryImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    for (uint32_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
        depthHistoryTextures[textureIndex] = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, RHI::TextureFlag::ShaderResource | RHI::TextureFlag::UnorderedAccess, depthHistoryImage, RHI::TextureMipInitMode::AllocateEmpty, {}, 1, RHI::GPUResourceState::ShaderResource);
        if (!depthHistoryTextures[textureIndex]) {
            BE_WARNLOG("RenderCamera::CreateDepthHistoryTextures: failed to create depth history texture %i\n", textureIndex);
            DestroyDepthHistoryTextures();
            return false;
        }

        RHI::renderer->SetName(depthHistoryTextures[textureIndex], BE1::va("RenderCamera::depthHistoryTextures[%i]", textureIndex));
    }

    currentHistoryTextureIndex = 0;
    historyValid = false;

    return true;
}

// depthHistoryTextures 를 depth attachment 용으로 생성한다.
bool RenderCamera::CreateDepthHistoryRenderTargets(uint32_t width, uint32_t height, RHI::Format depthFormat) {
    DestroyDepthHistoryTextures();

    if (width == 0 || height == 0) {
        return false;
    }

    BE1::Image::Format depthImageFormat = BE1::Image::Format::Unknown;
    if (!RHI::Renderer::RHIFormatToImageFormat(depthFormat, &depthImageFormat, nullptr)) {
        BE_WARNLOG("RenderCamera::CreateDepthHistoryRenderTargets: failed to convert depth texture format\n");
        return false;
    }

    const RHI::ClearValue optimizedClearValue = RHI::ClearValue::DepthStencil(desc.projectionPolicy.GetDepthClearValue(), 0);
    SharedPtr<BE1::Image> depthImage = MakeShared<BE1::Image>();
    depthImage->InitFromMemory(width, height, 1, 1, 1, depthImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    const RHI::TextureFlag textureFlags = RHI::TextureFlag::DepthStencil | RHI::TextureFlag::ShaderResource | RHI::TextureFlag::Typeless;
    for (uint32_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
        depthHistoryTextures[textureIndex] = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, textureFlags, depthImage, RHI::TextureMipInitMode::FromImage, optimizedClearValue, 1, RHI::GPUResourceState::ShaderResource);
        if (!depthHistoryTextures[textureIndex] ||
            !RHI::renderer->CreateView(depthHistoryTextures[textureIndex], RHI::ViewType::SRV).IsValid() ||
            !RHI::renderer->CreateView(depthHistoryTextures[textureIndex], RHI::ViewType::DSV).IsValid()) {
            BE_WARNLOG("RenderCamera::CreateDepthHistoryRenderTargets: failed to create depth history texture %i\n", textureIndex);
            DestroyDepthHistoryTextures();
            return false;
        }

        RHI::renderer->SetName(depthHistoryTextures[textureIndex], BE1::va("RenderCamera::depthHistoryTextures[%i]", textureIndex));
    }

    depthHistoryAsRenderTarget = true;
    currentHistoryTextureIndex = 0;
    historyValid = false;

    return true;
}

void RenderCamera::DestroyDepthHistoryTextures() {
    for (uint32_t textureIndex = 0; textureIndex < 2; ++textureIndex) {
        if (depthHistoryTextures[textureIndex]) {
            RHI::renderer->DestroyTexture(depthHistoryTextures[textureIndex]);
            depthHistoryTextures[textureIndex] = nullptr;
        }
    }

    depthHistoryAsRenderTarget = false;
    currentHistoryTextureIndex = 0;
    historyValid = false;
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

    bloomDownTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, bloomTextureFlags, bloomDownImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
    if (!bloomDownTexture) {
        BE_WARNLOG("RenderCamera::CreatePyramidBloomTextures: failed to create bloom down texture\n");
        return false;
    }
    RHI::renderer->SetName(bloomDownTexture, "RenderCamera::bloomDownTexture");

    // bloomUpTexture 는 upsample pass 의 출력 mip 만 필요하므로 가장 작은 source-only mip 은 만들지 않는다.
    SharedPtr<BE1::Image> bloomUpImage = MakeShared<BE1::Image>();
    bloomUpImage->InitFromMemory(bloomMipWidths[0], bloomMipHeights[0], 1, 1, bloomUpMipCount, bloomImageFormat, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);

    bloomUpTexture = RHI::renderer->CreateTexture(RHI::TextureType::Texture2D, bloomTextureFlags, bloomUpImage, RHI::TextureMipInitMode::FromImage, {}, 1, RHI::GPUResourceState::ShaderResource);
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

static RHI::Buffer *CreateFFTBloomStructuredBuffer(const char *debugName, uint64_t elementCount) {
    const uint64_t bufferSize = elementCount * sizeof(float2);
    RHI::Buffer *buffer = RHI::renderer->CreateBuffer(RHI::BufferUsage::Default, RHI::BufferFlag::ShaderResource | RHI::BufferFlag::UnorderedAccess, bufferSize, nullptr);
    if (!buffer) {
        BE_WARNLOG("RenderCamera::CreateConvolutionBloomResources: failed to create %s\n", debugName);
        return nullptr;
    }

    RHI::renderer->SetName(buffer, debugName);

    RHI::ViewID srvViewID = RHI::renderer->CreateView(buffer, RHI::ViewType::SRV, RHI::Format::Unknown, sizeof(float2), 0, bufferSize);
    if (!srvViewID.IsValid()) {
        BE_WARNLOG("RenderCamera::CreateConvolutionBloomResources: failed to create %s SRV\n", debugName);
        RHI::renderer->DestroyBuffer(buffer);
        return nullptr;
    }

    RHI::ViewID uavViewID = RHI::renderer->CreateView(buffer, RHI::ViewType::UAV, RHI::Format::Unknown, sizeof(float2), 0, bufferSize);
    if (!uavViewID.IsValid()) {
        BE_WARNLOG("RenderCamera::CreateConvolutionBloomResources: failed to create %s UAV\n", debugName);
        RHI::renderer->DestroyBuffer(buffer);
        return nullptr;
    }

    return buffer;
}

bool RenderCamera::CreateConvolutionBloomResources(uint32_t size) {
    DestroyConvolutionBloomResources();

    if (size == 0) {
        return false;
    }

    // RG 배열 이후에 B0 배열을 담는다.
    constexpr uint64_t ColorPairCount = 2; // RG, B0
    const uint64_t colorPairElementCount = uint64_t(size) * uint64_t(size) * ColorPairCount;

    fftBloomScreenSpatialBuffer = CreateFFTBloomStructuredBuffer("RenderCamera::fftBloomScreenSpatialBuffer", colorPairElementCount);
    if (!fftBloomScreenSpatialBuffer) {
        return false;
    }

    fftBloomScreenSpectrumBuffer = CreateFFTBloomStructuredBuffer("RenderCamera::fftBloomScreenSpectrumBuffer", colorPairElementCount);
    if (!fftBloomScreenSpectrumBuffer) {
        DestroyConvolutionBloomResources();
        return false;
    }

    fftBloomKernelSpatialBuffer = CreateFFTBloomStructuredBuffer("RenderCamera::fftBloomKernelSpatialBuffer", colorPairElementCount);
    if (!fftBloomKernelSpatialBuffer) {
        DestroyConvolutionBloomResources();
        return false;
    }

    fftBloomKernelSpectrumBuffer = CreateFFTBloomStructuredBuffer("RenderCamera::fftBloomKernelSpectrumBuffer", colorPairElementCount);
    if (!fftBloomKernelSpectrumBuffer) {
        DestroyConvolutionBloomResources();
        return false;
    }

    fftBloomWorkBuffer = CreateFFTBloomStructuredBuffer("RenderCamera::fftBloomWorkBuffer", colorPairElementCount);
    if (!fftBloomWorkBuffer) {
        DestroyConvolutionBloomResources();
        return false;
    }

    fftBloomSize = size;
    fftBloomKernelDirty = true;

    return true;
}

void RenderCamera::DestroyConvolutionBloomResources() {
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

bool RenderCamera::CreateConvolutionBloomOutputTexture(uint32_t width, uint32_t height) {
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
        BE_WARNLOG("RenderCamera::CreateConvolutionBloomOutputTexture: failed to create convolution texture\n");
        return false;
    }

    RHI::renderer->SetName(fftBloomConvolutionOutputTexture, "RenderCamera::fftBloomConvolutionOutputTexture");

    for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel) {
        fftBloomConvolutionOutputMipSRVs[mipLevel] = RHI::renderer->CreateView(fftBloomConvolutionOutputTexture, RHI::ViewType::SRV, RHI::Format::Auto, 0, 1, mipLevel, 1);
        fftBloomConvolutionOutputMipUAVs[mipLevel] = RHI::renderer->CreateView(fftBloomConvolutionOutputTexture, RHI::ViewType::UAV, RHI::Format::Auto, 0, 1, mipLevel, 1);
        if (!fftBloomConvolutionOutputMipSRVs[mipLevel].IsValid() || !fftBloomConvolutionOutputMipUAVs[mipLevel].IsValid()) {
            BE_WARNLOG("RenderCamera::CreateConvolutionBloomOutputTexture: failed to create mip %u views\n", mipLevel);
            DestroyConvolutionBloomOutputTexture();
            return false;
        }
    }

    fftBloomConvolutionOutputWidth = width;
    fftBloomConvolutionOutputHeight = height;
    fftBloomConvolutionOutputMipCount = mipLevelCount;

    return true;
}

void RenderCamera::DestroyConvolutionBloomOutputTexture() {
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
