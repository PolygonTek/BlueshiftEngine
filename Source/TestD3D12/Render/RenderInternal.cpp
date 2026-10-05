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
#include "RenderInternal.h"

uint8_t FrameCascadedShadow::CullCascades(const BE1::AABB &worldBounds, uint8_t cascadeMask) const {
    for (uint32_t cascadeIndex = 0; cascadeIndex < cascadeCount; ++cascadeIndex) {
        const uint8_t cascadeBit = static_cast<uint8_t>(BIT(cascadeIndex));
        if ((cascadeMask & cascadeBit) == 0) {
            continue;
        }

        const CSMCascade &cascade = cascades[cascadeIndex];
        if (CullShadowCaster(viewPlanes, cascade.shadowMapBounds, worldBounds)) {
            cascadeMask &= ~cascadeBit;
        }
    }
    return cascadeMask;
}

// Spot shadow 의 view-projection 행렬과 frustum 컬링 영역을 계산한다.
void FrameLight::CalcSpotShadowProjection(FrameLocalShadowView &shadowView) const {
    assert(type == RenderLight::Type::Spot);

    const uint32_t resolution = static_cast<uint32_t>(shadowView.tileRect.w);
    const uint32_t borderTexels = shadowView.borderTexels;
    assert(resolution > 2 * borderTexels);
    const uint32_t usableResolution = resolution - 2 * borderTexels;
    const float borderScale = static_cast<float>(resolution) / usableResolution;

    // Outer cone 을 감싸는 정사각 shadow 투영. Near 는 shadow 정밀도만 제어하며 조명 범위와 독립적이다.
    const float zFar = range;
    const float zNear = BE1::Min(BE1::Max(this->zNear, RenderLight::MinLightSize), zFar * 0.5f);
    const float left = zFar * spotTanOuterCone * borderScale;
    const float up = left;

    // 타일 전체에 렌더링하면 원래 조명 영역은 border 안쪽에 들어간다. Caster 컬링에도 확장된 영역을 사용한다.
    shadowView.frustum.SetOrigin(origin);
    shadowView.frustum.SetAxis(axis);
    shadowView.frustum.SetSize(zNear, zFar, left, up);
    shadowView.frustum.ToPlanes(shadowView.frustumPlanes);

    // Local shadow 는 카메라 projection policy 와 무관하게 near = 1, far = 0 의 유한 reverse-Z 를 사용한다.
#if 0
    // 아래 직접 계산과 동등한 참조 구현. SetFrustumRH 는 near 평면의 반폭/반높이를 받는다.
    ALIGN_AS32 BE1::Mat4 viewMatrix;
    SetViewMatrixFromEngineAxis(axis, origin, viewMatrix);

    const float nearScale = zNear / zFar;
    const float nearLeft = left * nearScale;
    const float nearUp = up * nearScale;

    ALIGN_AS32 BE1::Mat4 projMatrix;
    projMatrix.SetFrustumRH(-nearLeft, nearLeft, -nearUp, nearUp, zNear, zFar, false, true, false);
    shadowView.viewProjMatrix = projMatrix * viewMatrix;
#else
    const float xScale = zFar / left;
    const float yScale = zFar / up;
    const float depthScale = -zNear / (zFar - zNear);
    const float depthBias = -zFar * depthScale;
    const float distLeft = axis[EngineAxis::Left].Dot(origin);
    const float distUp = axis[EngineAxis::Up].Dot(origin);
    const float distForward = axis[EngineAxis::Forward].Dot(origin);

    // viewProjMatrix 를 직접 구성하여 별도의 view/projection 행렬 구성과 행렬 곱을 생략한다.
    shadowView.viewProjMatrix[0] = BE1::Vec4(axis[EngineAxis::Left] * -xScale, distLeft * xScale);
    shadowView.viewProjMatrix[1] = BE1::Vec4(axis[EngineAxis::Up] * yScale, -distUp * yScale);
    shadowView.viewProjMatrix[2] = BE1::Vec4(axis[EngineAxis::Forward] * depthScale, depthBias - distForward * depthScale);
    shadowView.viewProjMatrix[3] = BE1::Vec4(axis[EngineAxis::Forward], -distForward);
#endif
}

BE1::Vec2 FrameLight::GetPointShadowDepthRange() const {
    assert(type == RenderLight::Type::Point);
    const float zFar = range;
    const float zNear = BE1::Min(BE1::Max(this->zNear, RenderLight::MinLightSize), zFar * 0.5f);
    return BE1::Vec2(zNear, zFar);
}

// Point shadow 6 면의 view-projection 행렬과 frustum 컬링 영역을 계산한다.
BE1::Vec2 FrameLight::CalcPointShadowProjections(FrameLocalShadowView shadowViews[6]) const {
    assert(type == RenderLight::Type::Point);

    const uint32_t resolution = static_cast<uint32_t>(shadowViews[0].tileRect.w);
    const uint32_t borderTexels = shadowViews[0].borderTexels;
    assert(resolution > 2 * borderTexels);
    const uint32_t usableResolution = resolution - 2 * borderTexels;
    const float xyScale = static_cast<float>(usableResolution) / resolution;

    const BE1::Vec2 depthRange = GetPointShadowDepthRange();
    const float zNear = depthRange.x;
    const float zFar = depthRange.y;
    const float halfSize = zFar / xyScale;
    const float depthScale = -zNear / (zFar - zNear);
    const float depthBias = -zFar * depthScale;

    for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex) {
        FrameLocalShadowView &shadowView = shadowViews[faceIndex];
        assert(shadowView.faceIndex == faceIndex);
        assert(static_cast<uint32_t>(shadowView.tileRect.w) == resolution && shadowView.borderTexels == borderTexels);

        // 광원 회전과 무관하게 +X, -X, +Y, -Y, +Z, -Z 순서의 월드 축 6면을 사용한다.
        BE1::Mat3 faceAxis;
        CubeMapFaceToViewAxis(static_cast<RHI::CubemapFace>(faceIndex), faceAxis);

        // 90도 영역이 border 안쪽에 들어가도록 렌더링/컬링 frustum 을 함께 확장한다.
        shadowView.frustum.SetOrigin(origin);
        shadowView.frustum.SetAxis(faceAxis);
        shadowView.frustum.SetSize(zNear, zFar, halfSize, halfSize);
        shadowView.frustum.ToPlanes(shadowView.frustumPlanes);

#if 0
        ALIGN_AS32 BE1::Mat4 viewMatrix;
        SetViewMatrixFromEngineAxis(faceAxis, origin, viewMatrix);

        const float nearScale = zNear / zFar;
        const float nearHalfSize = halfSize * nearScale;

        ALIGN_AS32 BE1::Mat4 projMatrix;
        projMatrix.SetFrustumRH(-nearHalfSize, nearHalfSize, -nearHalfSize, nearHalfSize, zNear, zFar, false, true, false);
        shadowView.viewProjMatrix = projMatrix * viewMatrix;
#else
        const float distLeft = faceAxis[EngineAxis::Left].Dot(origin);
        const float distUp = faceAxis[EngineAxis::Up].Dot(origin);
        const float distForward = faceAxis[EngineAxis::Forward].Dot(origin);

        // 6면이 공유하는 투영 계수로 직접 구성한다. Z/W 는 near = 1, far = 0 이며 W 는 면의 전방 거리이다.
        shadowView.viewProjMatrix[0] = BE1::Vec4(faceAxis[EngineAxis::Left] * -xyScale, distLeft * xyScale);
        shadowView.viewProjMatrix[1] = BE1::Vec4(faceAxis[EngineAxis::Up] * xyScale, -distUp * xyScale);
        shadowView.viewProjMatrix[2] = BE1::Vec4(faceAxis[EngineAxis::Forward] * depthScale, depthBias - distForward * depthScale);
        shadowView.viewProjMatrix[3] = BE1::Vec4(faceAxis[EngineAxis::Forward], -distForward);
#endif
    }
    return BE1::Vec2(depthScale, depthBias);
}
