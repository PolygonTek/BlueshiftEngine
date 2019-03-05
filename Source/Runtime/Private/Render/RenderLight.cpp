// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/Heap.h"

BE_NAMESPACE_BEGIN

RenderLight::RenderLight(RenderWorld *renderWorld, int index) {
    this->renderWorld = renderWorld;
    this->index = index;

    worldOBB.SetZero();
    worldFrustum.SetOrigin(Vec3::origin);

    viewMatrix.SetIdentity();
    projMatrix.SetIdentity();
    viewProjScaleBiasMatrix.SetIdentity();

    viewCount = 0;
    visLight = nullptr;
    proxy = nullptr;

    firstUpdate = true;
}

RenderLight::~RenderLight() {
    if (proxy) {
        Mem_Free(proxy);
    }
}

void RenderLight::Update(const RenderLight::State *stateDef) {
    state = *stateDef;

    // Saturate light color RGBA in range [0, 1]
    BE1::Clamp(state.materialParms[RenderObject::RedParm], 0.0f, 1.0f);
    BE1::Clamp(state.materialParms[RenderObject::GreenParm], 0.0f, 1.0f);
    BE1::Clamp(state.materialParms[RenderObject::BlueParm], 0.0f, 1.0f);
    BE1::Clamp(state.materialParms[RenderObject::AlphaParm], 0.0f, 1.0f);

    // NOTE: shader 에서 이미 한번 square 처리가 되므로 여기서 sqrt 해준다
    state.fallOffExponent = Math::Sqrt(state.fallOffExponent);

    // Calculate view matrix with the given origin and axis
    R_SetViewMatrix(state.axis, state.origin, viewMatrix);

    if (state.type == PointLight) {
        // Set bounding volume for point light
        worldOBB = OBB(state.origin, state.size, state.axis);

        // Calculate point light orthogonal projection matrix
        R_SetOrthogonalProjectionMatrix(state.size[1], state.size[2], -state.size[0], state.size[0], projMatrix);

        // Calculate light fall-off matrix
        fallOffMatrix = projMatrix * viewMatrix;
    } else if (state.type == DirectionalLight) {
        // Bounding volume for box light
        worldOBB = OBB(state.origin + state.axis[0] * state.size[0] * 0.5f, Vec3(state.size[0] * 0.5f, state.size[1], state.size[2]), state.axis);

        // Calculate box light orthogonal projection matrix
        R_SetOrthogonalProjectionMatrix(worldOBB.Extents()[1], worldOBB.Extents()[2], 0, 2 * worldOBB.Extents()[0], projMatrix);

        // No fall-off for directional light
        fallOffMatrix = Mat3x4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    } else if (state.type == SpotLight) {
        // Set bounding frustum for spot light
        worldFrustum.SetOrigin(state.origin);
        worldFrustum.SetAxis(state.axis);
        worldFrustum.SetSize(BE1::Max(state.zNear, 0.01f), state.size[0], state.size[1], state.size[2]);

        float xFov = RAD2DEG(Math::ATan(worldFrustum.GetLeft(), worldFrustum.GetFarDistance())) * 2.0f;
        float yFov = RAD2DEG(Math::ATan(worldFrustum.GetUp(), worldFrustum.GetFarDistance())) * 2.0f;

        // Calculate spot light perspective projection matrix
        R_SetPerspectiveProjectionMatrix(xFov, yFov, worldFrustum.GetNearDistance(), worldFrustum.GetFarDistance(), false, projMatrix);

        // Calculate light fall-off matrix
        Mat4 orthoProjMatrix;
        R_SetOrthogonalProjectionMatrix(state.size[1], state.size[2], 0, state.size[0], orthoProjMatrix);
        fallOffMatrix = orthoProjMatrix * viewMatrix;
    } else {
        assert(0);
    }

    static const Mat4 textureScaleBiasMatrix(Vec4(0.5f, 0.0f, 0.0f, 0.5f), Vec4(0.0f, 0.5f, 0.0f, 0.5f), Vec4(0.0f, 0.0f, 0.5f, 0.5f), Vec4(0.0, 0.0, 0.0, 1.0f));
    viewProjScaleBiasMatrix = textureScaleBiasMatrix * projMatrix * viewMatrix;

    maxVisDistSquared = state.maxVisDist * state.maxVisDist;

    firstUpdate = false;
}

bool RenderLight::IsIntersectAABB(const AABB &aabb) const {
    if (state.type == DirectionalLight || state.type == PointLight) {
        if (this->worldOBB.IsIntersectOBB(OBB(aabb))) {
            return true;
        }
        return false;
    }

    if (state.type == SpotLight) {
        if (!worldFrustum.CullAABB(aabb)) {
            return true;
        }
        return false;
    }

    assert(0);
    return false;
}

bool RenderLight::IsIntersectOBB(const OBB &obb) const {
    if (state.type == DirectionalLight || state.type == PointLight) {
        if (this->worldOBB.IsIntersectOBB(obb)) {
            return true;
        }
        return false;
    }

    if (state.type == SpotLight) {
        if (!worldFrustum.CullOBB(obb)) {
            return true;
        }
        return false;
    }

    assert(0);
    return false;
}

const AABB RenderLight::GetWorldAABB() const {
    if (state.type == DirectionalLight || state.type == PointLight) {
        return worldOBB.ToAABB();
    }

    return worldFrustum.ToOBB().ToAABB();
}

bool RenderLight::DirLight_ShadowBVFromCaster(const OBB &casterOBB, OBB &shadowOBB) const {
    assert(state.type == RenderLight::DirectionalLight);

    AABB b1, b2;
    // Compute caster bounds for light axis
    casterOBB.AxisProjection(state.axis, b1);
    // Compute light bounds for light axis
    worldOBB.AxisProjection(state.axis, b2); 

    b1.IntersectSelf(b2);
    if (b1.IsCleared()) {
        return false;
    }

    b1[0].x = BE1::Max(b1[0].x, b2[0].x);
    b1[0].y = BE1::Max(b1[0].y, b2[0].y);
    b1[0].z = BE1::Max(b1[0].z, b2[0].z);
    b1[1].x = b2[1].x; // light maximum x
    b1[1].y = BE1::Min(b1[1].y, b2[1].y);
    b1[1].z = BE1::Min(b1[1].z, b2[1].z);

    shadowOBB = OBB(b1, Vec3::origin, state.axis);
    return true;
}

bool RenderLight::PointLight_ShadowBVFromCaster(const OBB &casterOBB, Frustum &shadowFrustum) const {
    assert(state.type == RenderLight::PointLight);

    Vec3 dir = casterOBB.Center() - state.origin;
    dir.Normalize();

    float dFar = worldOBB.Extents().Length();
    if (!shadowFrustum.FromProjection(casterOBB, state.origin, dFar)) {
        return false;
    }

    //renderSystem.GetRenderWorld()->DebugFrustum(shadowFrustum, false, 1.0f, true, true);
    return true;
}

bool RenderLight::SpotLight_ShadowBVFromCaster(const OBB &casterOBB, Frustum &shadowFrustum) const {
    assert(state.type == RenderLight::SpotLight);

    Vec3 dir = worldFrustum.GetAxis()[0];
    dir.Normalize();

    float dmin, dmax;
    casterOBB.AxisProjection(dir, dmin, dmax);

    float dNear = dmin - state.origin.Dot(dir);
    if (dNear < worldFrustum.GetNearDistance()) {
        dNear = worldFrustum.GetNearDistance();
    }

    shadowFrustum = worldFrustum;
    shadowFrustum.MoveNearDistance(dNear);

    return true;
}

bool RenderLight::Cull(const RenderCamera &camera) const {
    if (camera.GetState().orthogonal) {
        return Cull(camera.GetBox());
    } else {
        return Cull(camera.GetFrustum());
    }
}

bool RenderLight::Cull(const Frustum &viewFrustum) const {
    switch (state.type) {
    case DirectionalLight:
        if (viewFrustum.CullOBB(worldOBB)) {
            return true;
        }
        break;
    case SpotLight:
        if (viewFrustum.CullFrustum(worldFrustum)) {
            return true;
        }
        break;
    case PointLight:
        if (IsRadiusUniform()) {
            if (viewFrustum.CullSphere(Sphere(state.origin, state.size[0]))) {
                return true;
            }
        } else {
            if (viewFrustum.CullOBB(worldOBB)) {
                return true;
            }
        }
        break;
    default:
        break;
    }

    return false;
}

bool RenderLight::Cull(const OBB &viewBox) const {
    switch (state.type) {
    case DirectionalLight:
        if (!viewBox.IsIntersectOBB(worldOBB)) {
            return true;
        }
        break;
    case SpotLight:
        if (worldFrustum.CullOBB(viewBox)) {
            return true;
        }
        break;
    case PointLight:
        if (IsRadiusUniform()) {
            if (!viewBox.IsIntersectSphere(Sphere(state.origin, state.size[0]))) {
                return true;
            }
        } else {
            if (!viewBox.IsIntersectOBB(worldOBB)) {
                return true;
            }
        }
        break;
    default:
        break;
    }

    return false;
}

bool RenderLight::CullShadowCaster(const OBB &casterOBB, const Frustum &viewFrustum, const AABB &visAABB) const {
    if (viewFrustum.CullAABB(visAABB)) {
        return true;
    }

    if (state.type == DirectionalLight) {
        OBB shadowOBB;
        if (!DirLight_ShadowBVFromCaster(casterOBB, shadowOBB)) {
            return true;
        }

        if (viewFrustum.CullOBB(shadowOBB)) {
            return true;
        }

        if (!OBB(visAABB).IsIntersectOBB(shadowOBB)) {
            return true;
        }

        return false;
    }

    if (state.type == PointLight) {
        Frustum shadowFrustum;
        if (PointLight_ShadowBVFromCaster(casterOBB, shadowFrustum)) {
            if (shadowFrustum.GetFarDistance() <= r_shadowCubeMapZNear.GetFloat()) {
                return true;
            }

            if (viewFrustum.CullFrustum(shadowFrustum)) {
                return true;
            }

            if (shadowFrustum.CullAABB(visAABB)) {
                return true;
            }
        }

        return false;
    }

    if (state.type == SpotLight) {
        Frustum shadowFrustum;
        SpotLight_ShadowBVFromCaster(casterOBB, shadowFrustum);
        if (viewFrustum.CullFrustum(shadowFrustum)) {
            return true;
        }

        if (shadowFrustum.CullAABB(visAABB)) {
            return true;
        }

        return false;
    }

    assert(0);
    return false;
}

bool RenderLight::ComputeScreenClipRect(const RenderCamera *viewDef, Rect &clipRect) const {
    switch (state.type) {
    case DirectionalLight:
        if (!viewDef->CalcClipRectFromOBB(worldOBB, clipRect)) {
            return false;
        }
        break;
    case SpotLight:
        if (!viewDef->CalcClipRectFromFrustum(worldFrustum, clipRect)) {
            return false;
        }
        break;
    case PointLight:
        if (IsRadiusUniform()) {
            if (!viewDef->CalcClipRectFromSphere(Sphere(state.origin, GetRadius()[0]), clipRect)) {
                return false;
            }
        } else {
            if (!viewDef->CalcClipRectFromOBB(worldOBB, clipRect)) {
                return false;
            }
        }
        break;
    default:
        break;
    }

    return true;
}

BE_NAMESPACE_END
