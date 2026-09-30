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
#include "LocalShadowCasterCuller.h"
#include "RenderInternal.h"

bool LocalShadowCasterCuller::PrepareBounds() {
    if (boundsInitialized) {
        return true;
    }

    // 비어 있는 bounds 면 false 를 리턴한다.
    if (casterBounds.IsCleared()) {
        return false;
    }

    casterBoundsCenter = casterBounds.Center();
    casterBoundsExtents = casterBounds.Extents();
    boundsInitialized = true;
    return true;
}

void LocalShadowCasterCuller::PrepareViewPlaneDistances() {
    if (viewPlaneMinDistancesInitialized) {
        return;
    }

    // caster bounds 와 카메라 평면과의 거리를 미리 계산한다.
    for (int planeIndex = 0; planeIndex < 6; ++planeIndex) {
        const BE1::Plane &viewPlane = viewPlanes[planeIndex];
        viewPlaneMinDistances[planeIndex] = viewPlane.Distance(casterBoundsCenter) - casterBoundsExtents.AbsDot(viewPlane.normal);
    }
    viewPlaneMinDistancesInitialized = true;
}

bool LocalShadowCasterCuller::CullByShadowFrustum(const FrameLocalShadowView &shadowView, float &outMinDepth) const {
    const BE1::Frustum &shadowFrustum = shadowView.frustum;

    // casterBounds 가 shadowFrustum 의 side/far 평면 밖에 있는지 검사
    for (int planeIndex = 1; planeIndex < 6; ++planeIndex) {
        const BE1::Plane &frustumPlane = shadowView.frustumPlanes[planeIndex];
        if (frustumPlane.Distance(casterBoundsCenter) > casterBoundsExtents.AbsDot(frustumPlane.normal)) {
            return true;
        }
    }

    // casterBounds 가 shadowFrustum 의 near 평면 바깥에 있는지 검사
    const BE1::Vec3 &lightOrigin = shadowFrustum.GetOrigin();
    const BE1::Vec3 &lightForward = shadowFrustum.GetAxis()[0];
    const float centerDepth = lightForward.Dot(casterBoundsCenter - lightOrigin);
    const float depthRadius = casterBoundsExtents.AbsDot(lightForward);
    if (centerDepth + depthRadius < shadowFrustum.GetNearDistance()) {
        return true;
    }

    // 광원 원점에서 casterBounds 까지의 lightForward 방향 최소 깊이
    outMinDepth = centerDepth - depthRadius;
    return false;
}

bool LocalShadowCasterCuller::CullShadowVolume(const FrameLocalShadowView &shadowView, const float viewPlaneToOriginDistances[6], float minDepth) const {
    // 라이트에서 최소 거리의 caster 점이 frustum far 까지 도달하도록 projection scale 을 계산한다.
    const float projectionScale = BE1::Max(1.0f, shadowView.frustum.GetFarDistance() / minDepth);

    for (int planeIndex = 0; planeIndex < 6; ++planeIndex) {
        // 카메라 평면에서 caster bounds 까지의 최소 거리
        const float casterMinDist = viewPlaneMinDistances[planeIndex];
        if (casterMinDist <= 0.0f) {
            // 카메라 평면 안쪽에 caster bounds 가 있는 경우, 이 평면으로는 컬링할 수 없다.
            continue;
        }

        // 카메라 평면에서 광원까지의 거리
        const float originDistance = viewPlaneToOriginDistances[planeIndex];

        // 카메라 평면에서 투영된 caster bounds 까지의 최소 거리
        const float projectedMin = originDistance + (casterMinDist - originDistance) * projectionScale;
        if (projectedMin > 0.0f) {
            // caster bounds 와 투영된 caster bounds 가 카메라 평면 안쪽에 있지 않다면 컬링한다.
            return true;
        }
    }

    return false;
}

bool LocalShadowCasterCuller::Cull(const FrameLocalShadowView &shadowView, const float viewPlaneToOriginDistances[6], float &outMinDepth) {
    if (!PrepareBounds()) {
        return true;
    }

    // shadow caster 를 shadow frustum 으로 컬링한다.
    if (CullByShadowFrustum(shadowView, outMinDepth)) {
        return true;
    }

    if (outMinDepth <= 0.0f) {
        // 최소 깊이가 0 이하이면 bounds 전체를 far 까지 확대할 수 없으므로 shadow volume 컬링을 생략한다.
        return false;
    }

    PrepareViewPlaneDistances();

    // shadow caster 를 shadow volume 과 view planes 로 컬링한다.
    return CullShadowVolume(shadowView, viewPlaneToOriginDistances, outMinDepth);
}
