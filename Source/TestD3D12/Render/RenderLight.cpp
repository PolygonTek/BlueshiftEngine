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
#include "RenderLight.h"
#include "RenderCamera.h"
#include "RenderUtils.h"

void RenderLight::Update(const RenderLight::Desc &src, Desc::UpdateFlag updateFlags) {
    assert(src.type == desc.type || BE1::HasFlag(updateFlags, Desc::UpdateFlag::Shape));
    assert(src.linearColor.r >= 0.0f && src.linearColor.r <= 1.0f);
    assert(src.linearColor.g >= 0.0f && src.linearColor.g <= 1.0f);
    assert(src.linearColor.b >= 0.0f && src.linearColor.b <= 1.0f);

    desc.flags = src.flags;

    if (desc.projectionPolicy != src.projectionPolicy) {
        updateFlags |= Desc::UpdateFlag::Shape;
    }
    desc.projectionPolicy = src.projectionPolicy;
    desc.linearColor = src.linearColor;
    desc.intensity = src.intensity;
    desc.fallOffExponent = src.fallOffExponent;
    desc.shadowOffsetFactor = src.shadowOffsetFactor;
    desc.shadowOffsetUnits = src.shadowOffsetUnits;

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Transform)) {
        desc.origin = src.origin;
        desc.axis = src.axis;

        SetViewMatrixFromEngineAxis(desc.axis, desc.origin, viewMatrix);
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Shape)) {
        desc.type = src.type;
        desc.size = src.size;
        desc.zNear = src.zNear;
    }

    if (BE1::HasAnyFlag(updateFlags, Desc::UpdateFlag::Transform | Desc::UpdateFlag::Shape)) {
        const bool zNegativeOneToOne = desc.projectionPolicy.IsZNegativeOneToOne();
        const bool reverseZ = desc.projectionPolicy.reverseZ;

        constexpr float MinLightSize = 0.01f;
        const BE1::Vec3 effectiveSize(
            BE1::Max(desc.size[0], MinLightSize),
            BE1::Max(desc.size[1], MinLightSize),
            BE1::Max(desc.size[2], MinLightSize));

        if (desc.type == Type::Point) {
            // Set bounding volume for point light.
            worldOBB = BE1::OBB(desc.origin, effectiveSize, desc.axis);

            // Calculate point light orthogonal projection matrix.
            projMatrix.SetOrthoRH(-effectiveSize[1], effectiveSize[1], -effectiveSize[2], effectiveSize[2], -effectiveSize[0], effectiveSize[0], zNegativeOneToOne, reverseZ);

            // Calculate light fall-off matrix.
            ALIGN_AS32 BE1::Mat4 fallOffProjMatrix;
            fallOffProjMatrix.SetOrthoRH(-effectiveSize[1], effectiveSize[1], -effectiveSize[2], effectiveSize[2], -effectiveSize[0], effectiveSize[0], true, false);
            fallOffMatrix = fallOffProjMatrix * viewMatrix;
        } else if (desc.type == Type::BoxDir) {
            // Set bounding volume for box-directional light.
            worldOBB = BE1::OBB(desc.origin + desc.axis[0] * effectiveSize[0] * 0.5f, BE1::Vec3(effectiveSize[0] * 0.5f, effectiveSize[1], effectiveSize[2]), desc.axis);

            // Calculate box-directional light orthogonal projection matrix.
            const BE1::Vec3 &obbExtents = worldOBB.Extents();
            projMatrix.SetOrthoRH(-obbExtents[1], obbExtents[1], -obbExtents[2], obbExtents[2], 0, 2 * obbExtents[0], zNegativeOneToOne, reverseZ);

            // No fall-off for box-directional light.
            fallOffMatrix = BE1::Mat3x4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        } else if (desc.type == Type::Spot) {
            const float zNear = BE1::Max(desc.zNear, MinLightSize);
            const float zFar = BE1::Max(effectiveSize[0], zNear + MinLightSize);
            const float left = effectiveSize[1];
            const float up = effectiveSize[2];

            // Set bounding frustum for spot light.
            worldFrustum.SetOrigin(desc.origin);
            worldFrustum.SetAxis(desc.axis);
            worldFrustum.SetSize(zNear, zFar, left, up);

            const float yFov = RAD2DEG(BE1::Math::ATan(worldFrustum.GetUp(), worldFrustum.GetFarDistance())) * 2.0f;

            // Calculate spot light perspective projection matrix.
            const float aspect = worldFrustum.GetLeft() / worldFrustum.GetUp();
            projMatrix.SetPerspectiveRH(yFov, aspect, worldFrustum.GetNearDistance(), worldFrustum.GetFarDistance(), zNegativeOneToOne, reverseZ, false);

            // Calculate light fall-off matrix.
            ALIGN_AS32 BE1::Mat4 fallOffProjMatrix;
            fallOffProjMatrix.SetOrthoRH(-left, left, -up, up, 0.0f, zFar, true);
            fallOffMatrix = fallOffProjMatrix * viewMatrix;
        } else {
            assert(0);
        }

        // 모든 render backend 는 D3D 규약으로 통일한다.
        // NDC Y 는 위쪽이 +1 이고 texture Y 는 아래쪽이 +1 이므로 Y 를 뒤집으며, NDC Z 는 이미 [0, 1] 범위이다.
        // Vulkan backend 는 이 변환을 동일하게 유지할 수 있도록 negative viewport height 를 사용해야 한다.
        ALIGN_AS32 static const BE1::Mat4 textureScaleBiasMatrix(
            BE1::Vec4(0.5f, 0.0f, 0.0f, 0.5f),
            BE1::Vec4(0.0f, -0.5f, 0.0f, 0.5f),
            BE1::Vec4(0.0f, 0.0f, 1.0f, 0.0f),
            BE1::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
        viewProjScaleBiasMatrix = textureScaleBiasMatrix * projMatrix * viewMatrix;

        if (desc.type == Type::Point || desc.type == Type::BoxDir) {
            worldAABB = worldOBB.ToAABB();
        } else if (desc.type == Type::Spot) {
            worldAABB = worldFrustum.ToOBB().ToAABB();
        }
    }
}

bool RenderLight::IsIntersectAABB(const BE1::AABB &aabb) const {
    if (desc.type == Type::BoxDir || desc.type == Type::Point) {
        if (this->worldOBB.IsIntersectAABB(aabb)) {
            return true;
        }
        return false;
    }

    if (desc.type == Type::Spot) {
        if (!worldFrustum.CullAABB(aabb)) {
            return true;
        }
        return false;
    }

    assert(0);
    return false;
}

bool RenderLight::IsIntersectOBB(const BE1::OBB &obb) const {
    if (desc.type == Type::BoxDir || desc.type == Type::Point) {
        if (this->worldOBB.IsIntersectOBB(obb)) {
            return true;
        }
        return false;
    }

    if (desc.type == Type::Spot) {
        if (!worldFrustum.CullOBB(obb)) {
            return true;
        }
        return false;
    }

    assert(0);
    return false;
}

void RenderLight::ProjectOnAxis(const BE1::Vec3 &axis, float &min, float &max) const {
    if (desc.type == Type::BoxDir) {
        worldOBB.ProjectOnAxis(axis, min, max);
        return;
    }

    if (desc.type == Type::Spot) {
        worldFrustum.ProjectOnAxis(axis, min, max);
        return;
    }

    if (desc.type == Type::Point) {
        const BE1::Vec3 &radius = worldOBB.Extents();
        if (radius.x == radius.y && radius.x == radius.z) {
            BE1::Sphere(desc.origin, radius.x).ProjectOnAxis(axis, min, max);
            return;
        }

        worldOBB.ProjectOnAxis(axis, min, max);
        return;
    }

    assert(0);
    min = max = 0.0f;
}

bool RenderLight::Cull(const RenderCamera &camera) const {
    if (desc.type == Type::BoxDir) {
        return camera.Cull(worldOBB);
    }

    if (desc.type == Type::Spot) {
        return camera.Cull(worldFrustum);
    }

    if (desc.type == Type::Point) {
        const BE1::Vec3 &radius = worldOBB.Extents();
        if (radius.x == radius.y && radius.x == radius.z) {
            return camera.Cull(BE1::Sphere(desc.origin, radius.x));
        }

        return camera.Cull(worldOBB);
    }

    assert(0);
    return false;
}

bool RenderLight::ComputeScreenClipRect(const RenderCamera &camera, BE1::Rect &clipRect) const {
    if (desc.type == Type::BoxDir) {
        return camera.CalcClipRectFromOBB(worldOBB, clipRect);
    }

    if (desc.type == Type::Spot) {
        return camera.CalcClipRectFromFrustum(worldFrustum, clipRect);
    }

    if (desc.type == Type::Point) {
        const BE1::Vec3 &radius = worldOBB.Extents();
        if (radius.x == radius.y && radius.x == radius.z) {
            return camera.CalcClipRectFromSphere(BE1::Sphere(desc.origin, radius.x), clipRect);
        }

        return camera.CalcClipRectFromOBB(worldOBB, clipRect);
    }

    assert(0);
    return false;
}

