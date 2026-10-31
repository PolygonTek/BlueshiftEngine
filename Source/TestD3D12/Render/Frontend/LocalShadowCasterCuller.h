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

#include "Math/Vector3.h"
#include "Math/AABB.h"
#include "Math/Plane.h"

struct FrameLocalShadowView;

/*
---------------------------------------------------------------------------------------------------

    Local Shadow Caster Culler

---------------------------------------------------------------------------------------------------
*/

class LocalShadowCasterCuller {
public:
    LocalShadowCasterCuller(const BE1::AABB &casterBounds, const BE1::Plane viewPlanes[6]) :
        casterBounds(casterBounds), viewPlanes(viewPlanes) {}

                            // shadow caster bounds 를 포인트 라이트로 컬링한다.
    bool                    CullPointLight(const BE1::Vec3 &lightOrigin, float squaredRadius) const;

                            // shadow caster bounds 를 라이트의 shadow frustum 과 view volume 으로 컬링한다.
                            // viewPlaneToOriginDistances 가 nullptr 이면 카메라 기반 컬링과 그 준비 계산을 생략한다.
                            // false 를 리턴하면 lightForward 방향 최소 깊이를 outMinDepth 에 기록한다.
    bool                    Cull(const FrameLocalShadowView &shadowView, const float viewPlaneToOriginDistances[6], float &outMinDepth);

private:
    bool                    PrepareBounds();
    void                    PrepareViewPlaneDistances();

                            // shadow caster bounds 를 라이트의 shadow frustum 으로 컬링한다. 통과 시 라이트의 shadow frustum 방향 최소 거리를 outMinDepth 에 기록한다.
    bool                    CullByShadowFrustum(const FrameLocalShadowView &shadowView, float &outMinDepth) const;

                            // shadow caster bounds 의 shadow volume 을 카메라 view volume 으로 컬링한다.
    bool                    CullShadowVolume(const FrameLocalShadowView &shadowView, const float viewPlaneToOriginDistances[6], float minDepth) const;

    const BE1::AABB &       casterBounds;
    BE1::Vec3               casterBoundsCenter;
    BE1::Vec3               casterBoundsExtents;
    bool                    boundsInitialized = false;

    const BE1::Plane *      viewPlanes;
    float                   viewPlaneMinDistances[6];               // caster bounds 와 카메라 평면과의 거리
    bool                    viewPlaneMinDistancesInitialized = false;
};

BE_INLINE bool LocalShadowCasterCuller::CullPointLight(const BE1::Vec3 &lightOrigin, float squaredRadius) const {
    BE1::Vec3 closestPoint;
    casterBounds.GetClosestPoint(lightOrigin, closestPoint);
    return closestPoint.DistanceSqr(lightOrigin) > squaredRadius;
}
