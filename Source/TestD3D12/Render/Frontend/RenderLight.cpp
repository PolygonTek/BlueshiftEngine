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
#include "Render/RenderUtils.h"

void RenderLight::Update(const RenderLight::Desc &src, Desc::UpdateFlag updateFlags) {
    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Flags)) {
        desc.flags = src.flags;
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Layer)) {
        desc.layer = src.layer;
    }

    bool updateProjectionAndBounds = false;

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::ProjectionPolicy)) {
        if (desc.projectionPolicy != src.projectionPolicy) {
            desc.projectionPolicy = src.projectionPolicy;
            updateProjectionAndBounds = true;
        }
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Lighting)) {
        assert(src.linearColor.r >= 0.0f && src.linearColor.r <= 1.0f);
        assert(src.linearColor.g >= 0.0f && src.linearColor.g <= 1.0f);
        assert(src.linearColor.b >= 0.0f && src.linearColor.b <= 1.0f);

        desc.linearColor = src.linearColor;
        desc.intensity = src.intensity;
        desc.fallOffExponent = src.fallOffExponent;
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Shadow)) {
        desc.shadowOffsetUnits = src.shadowOffsetUnits;
        desc.shadowOffsetFactor = src.shadowOffsetFactor;
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Transform)) {
        desc.origin = src.origin;
        desc.axis = src.axis;

        UpdateViewMatrix();
        updateProjectionAndBounds = true;
    }

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Shape)) {
        desc.type = src.type;
        desc.size = src.size;
        if (desc.type == Type::Point) {
            const float radius = BE1::Max(desc.size[0], MinLightSize);
            desc.size.Set(radius, radius, radius);
        }
        desc.zNear = src.zNear;
        updateProjectionAndBounds = true;
    }

    if (updateProjectionAndBounds) {
        UpdateProjectionAndBounds();
    }
}

void RenderLight::UpdateFlags(Flag flags) {
    desc.flags = flags;
}

void RenderLight::UpdateLayer(uint8_t layer) {
    desc.layer = layer;
}

void RenderLight::UpdateTransform(const BE1::Vec3 &origin, const BE1::Mat3 &axis) {
    desc.origin = origin;
    desc.axis = axis;

    UpdateViewMatrix();
    UpdateProjectionAndBounds();
}

void RenderLight::UpdateShape(Type type, const BE1::Vec3 &size, float zNear) {
    desc.type = type;
    desc.size = size;
    if (desc.type == Type::Point) {
        const float radius = BE1::Max(desc.size[0], MinLightSize);
        desc.size.Set(radius, radius, radius);
    }
    desc.zNear = zNear;

    UpdateProjectionAndBounds();
}

void RenderLight::UpdateProjectionPolicy(const RHI::ProjectionPolicy &projectionPolicy) {
    if (desc.projectionPolicy != projectionPolicy) {
        desc.projectionPolicy = projectionPolicy;
        UpdateProjectionAndBounds();
    }
}

void RenderLight::UpdateLighting(const BE1::Color3 &linearColor, float intensity, float fallOffExponent) {
    assert(linearColor.r >= 0.0f && linearColor.r <= 1.0f);
    assert(linearColor.g >= 0.0f && linearColor.g <= 1.0f);
    assert(linearColor.b >= 0.0f && linearColor.b <= 1.0f);

    desc.linearColor = linearColor;
    desc.intensity = intensity;
    desc.fallOffExponent = fallOffExponent;
}

void RenderLight::UpdateShadow(float shadowOffsetUnits, float shadowOffsetFactor) {
    desc.shadowOffsetUnits = shadowOffsetUnits;
    desc.shadowOffsetFactor = shadowOffsetFactor;
}

void RenderLight::UpdateViewMatrix() {
    SetViewMatrixFromEngineAxis(desc.axis, desc.origin, viewMatrix);
}

void RenderLight::UpdateProjectionAndBounds() {
    // Directional lights have no finite volume, projection, or distance fall-off.
    if (desc.type == Type::Directional) {
        worldAABB = BE1::AABB::empty;
        return;
    }

    const bool zNegativeOneToOne = desc.projectionPolicy.IsZNegativeOneToOne();
    const bool reverseZ = desc.projectionPolicy.reverseZ;

    const BE1::Vec3 effectiveSize(
        BE1::Max(desc.size[0], MinLightSize),
        BE1::Max(desc.size[1], MinLightSize),
        BE1::Max(desc.size[2], MinLightSize));

    if (desc.type == Type::Point) {
        // Point lights always use a spherical bounding volume and radial fall-off.
        worldSphere = BE1::Sphere(desc.origin, effectiveSize[0]);

        // Project a planar texture through the spherical light volume along axis[0].
        const float radius = effectiveSize[0];
        projMatrix.SetOrthoRH(-radius, radius, -radius, radius, -radius, radius, zNegativeOneToOne, reverseZ);
    } else if (desc.type == Type::BoxDir) {
        // Set bounding volume for box-directional light.
        worldOBB = BE1::OBB(desc.origin + desc.axis[0] * effectiveSize[0] * 0.5f, BE1::Vec3(effectiveSize[0] * 0.5f, effectiveSize[1], effectiveSize[2]), desc.axis);

        // Calculate box-directional light orthogonal projection matrix.
        const BE1::Vec3 &obbExtents = worldOBB.Extents();
        projMatrix.SetOrthoRH(-obbExtents[1], obbExtents[1], -obbExtents[2], obbExtents[2], 0, 2 * obbExtents[0], zNegativeOneToOne, reverseZ);
    } else if (desc.type == Type::Spot) {
        const float zNear = BE1::Max(desc.zNear, MinLightSize);
        const float zFar = BE1::Max(effectiveSize[0], zNear + MinLightSize);
        const float left = effectiveSize[1];
        const float up = effectiveSize[2];

        // Set bounding frustum for spot light.
        worldFrustum.SetOrigin(desc.origin);
        worldFrustum.SetAxis(desc.axis);
        worldFrustum.SetSize(zNear, zFar, left, up);

        const float yFov = RAD2DEG(BE1::Math::ATan2(worldFrustum.GetUp(), worldFrustum.GetFarDistance())) * 2.0f;

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

    if (desc.type == Type::Point) {
        worldAABB = worldSphere.ToAABB();
    } else if (desc.type == Type::BoxDir) {
        worldAABB = worldOBB.ToAABB();
    } else if (desc.type == Type::Spot) {
        worldAABB = worldFrustum.ToOBB().ToAABB();
    }
}

bool RenderLight::IsIntersectAABB(const BE1::AABB &aabb) const {
    if (desc.type == Type::Point) {
        return worldSphere.IsIntersectAABB(aabb);
    }

    if (desc.type == Type::BoxDir) {
        return worldOBB.IsIntersectAABB(aabb);
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
    if (desc.type == Type::Point) {
        return obb.IsIntersectSphere(worldSphere);
    }

    if (desc.type == Type::BoxDir) {
        return worldOBB.IsIntersectOBB(obb);
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
        worldSphere.ProjectOnAxis(axis, min, max);
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
        return camera.Cull(worldSphere);
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
        return camera.CalcClipRectFromSphere(worldSphere, clipRect);
    }

    assert(0);
    return false;
}

