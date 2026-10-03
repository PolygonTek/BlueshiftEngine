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

    if (BE1::HasFlag(updateFlags, Desc::UpdateFlag::Lighting)) {
        UpdateLighting(src.linearColor, src.intensity);
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
        // Transform 은 위에서 반영했으므로 경계 계산은 한 번만 수행한다.
        UpdateShape(src.type, src.range, src.zNear, src.innerConeAngle, src.outerConeAngle);
    } else if (updateProjectionAndBounds) {
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

void RenderLight::UpdateShape(Type type, float range, float zNear, float innerConeAngle, float outerConeAngle) {
    desc.type = type;
    desc.range = BE1::Max(range, MinLightSize);
    desc.zNear = zNear;
    desc.outerConeAngle = BE1::Clamp(outerConeAngle, MinSpotOuterConeAngle, MaxSpotOuterConeAngle);
    desc.innerConeAngle = BE1::Clamp(innerConeAngle, 0.0f, desc.outerConeAngle - MinSpotConeAngleGap);

    if (type == Type::Spot) {
        // 각도 감쇠와 shadow 투영 계수는 모양이 바뀔 때만 계산한다.
        float sinOuter;
        BE1::Math::SinCos(DEG2RAD(desc.outerConeAngle), sinOuter, spotCosCone.y);
        spotCosCone.x = BE1::Math::Cos(DEG2RAD(desc.innerConeAngle));
        spotInvCosConeDifference = 1.0f / (spotCosCone.x - spotCosCone.y);
        spotTanOuterCone = sinOuter / spotCosCone.y;
    }

    UpdateProjectionAndBounds();
}

void RenderLight::UpdateLighting(const BE1::Color3 &linearColor, float intensity) {
    assert(linearColor.r >= 0.0f && linearColor.r <= 1.0f);
    assert(linearColor.g >= 0.0f && linearColor.g <= 1.0f);
    assert(linearColor.b >= 0.0f && linearColor.b <= 1.0f);
    assert(BE1::Math::IsFinite(intensity) && intensity >= 0.0f);

    desc.linearColor = linearColor;
    desc.intensity = BE1::Math::IsFinite(intensity) ? BE1::Max(intensity, 0.0f) : 0.0f;

    // Shaders/Colors.hlsli 의 GetLuma 와 같은 계수로 색상과 무관하게 입력 lux/cd 를 유지한다.
    // 매우 어두운 색상도 정규화 중 overflow/underflow 가 나지 않도록 중간값은 double 로 계산한다.
    const double luminance = 0.212655 * linearColor.r + 0.715158 * linearColor.g + 0.072187 * linearColor.b;
    if (luminance > 0.0 && desc.intensity > 0.0f) {
        const double scale = static_cast<double>(desc.intensity) / luminance;
        intensityRGB = BE1::Color3(static_cast<float>(linearColor.r * scale), static_cast<float>(linearColor.g * scale), static_cast<float>(linearColor.b * scale));
    } else {
        intensityRGB = BE1::Color3::black;
    }
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

    worldSphere = BE1::Sphere(desc.origin, desc.range);

    // 광원 투영은 카메라와 독립적으로 [0, 1] 정방향 depth 를 사용한다.
    if (desc.type == Type::Point) {
        // Point lights always use a spherical bounding volume and radial fall-off.
        // Project a planar texture through the spherical light volume along axis[0].
        const float radius = desc.range;
        projMatrix.SetOrthoRH(-radius, radius, -radius, radius, -radius, radius, false, false);
    } else if (desc.type == Type::Spot) {
        const float zNear = BE1::Min(BE1::Max(desc.zNear, MinLightSize), desc.range * 0.5f);
        const float halfSize = desc.range * spotTanOuterCone;

        // 원형 콘을 감싸는 사각 frustum 으로 보수적으로 컬링한다.
        // 조명은 원점부터 시작한다. Shadow near plane 으로 가까운 조명을 잘라내지 않는다.
        worldFrustum.SetOrigin(desc.origin);
        worldFrustum.SetAxis(desc.axis);
        worldFrustum.SetSize(0.0f, desc.range, halfSize, halfSize);

        // 정사각 투영의 FOV 는 outer 반각의 두 배다. 조명 감쇠는 이 투영과 독립적이다.
        projMatrix.SetPerspectiveRH(desc.outerConeAngle * 2.0f, 1.0f, zNear, desc.range, false, false, false);
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
    } else if (desc.type == Type::Spot) {
        worldAABB = worldFrustum.ToOBB().ToAABB();
        worldAABB.IntersectSelf(worldSphere.ToAABB());
    }
}

bool RenderLight::IsIntersectAABB(const BE1::AABB &aabb) const {
    if (desc.type == Type::Point) {
        return worldSphere.IsIntersectAABB(aabb);
    }

    if (desc.type == Type::Spot) {
        return worldSphere.IsIntersectAABB(aabb) && !worldFrustum.CullAABB(aabb);
    }

    assert(0);
    return false;
}

bool RenderLight::IsIntersectOBB(const BE1::OBB &obb) const {
    if (desc.type == Type::Point) {
        return obb.IsIntersectSphere(worldSphere);
    }

    if (desc.type == Type::Spot) {
        return obb.IsIntersectSphere(worldSphere) && !worldFrustum.CullOBB(obb);
    }

    assert(0);
    return false;
}

void RenderLight::ProjectOnAxis(const BE1::Vec3 &axis, float &min, float &max) const {
    if (desc.type == Type::Spot) {
        worldFrustum.ProjectOnAxis(axis, min, max);
        float radialMin;
        float radialMax;
        worldSphere.ProjectOnAxis(axis, radialMin, radialMax);
        min = BE1::Max(min, radialMin);
        max = BE1::Min(max, radialMax);
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
    if (desc.type == Type::Spot) {
        return camera.Cull(worldSphere) || camera.Cull(worldFrustum);
    }

    if (desc.type == Type::Point) {
        return camera.Cull(worldSphere);
    }

    assert(0);
    return false;
}

bool RenderLight::ComputeScreenClipRect(const RenderCamera &camera, BE1::Rect &clipRect) const {
    if (desc.type == Type::Spot) {
        // 넓은 콘에서도 range 밖으로 light-culling 타일과 shadow 요청 해상도가 커지지 않게 제한한다.
        BE1::Rect coneClipRect;
        if (!camera.CalcClipRectFromSphere(worldSphere, clipRect) || !camera.CalcClipRectFromFrustum(worldFrustum, coneClipRect)) {
            return false;
        }
        clipRect.IntersectSelf(coneClipRect);
        return clipRect.w > 0 && clipRect.h > 0;
    }

    if (desc.type == Type::Point) {
        return camera.CalcClipRectFromSphere(worldSphere, clipRect);
    }

    assert(0);
    return false;
}

