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

SceneLight::SceneLight() {
    memset(&parms, 0, sizeof(parms));
    index = 0;
    firstUpdate = true;
    obb.SetZero();
    frustum.SetOrigin(Vec3::origin);
    viewMatrix.SetIdentity();
    projMatrix.SetIdentity();
    viewProjScaleBiasMatrix.SetIdentity();
    viewCount = 0;
    viewLight = nullptr;
    proxy = nullptr;
}

SceneLight::~SceneLight() {
    if (proxy) {
        Mem_Free(proxy);
    }
}

void SceneLight::Update(const SceneLight::Parms *renderLight) {
    this->parms = *renderLight;

    BE1::Clamp(parms.materialParms[SceneEntity::RedParm], 0.0f, 1.0f);
    BE1::Clamp(parms.materialParms[SceneEntity::GreenParm], 0.0f, 1.0f);
    BE1::Clamp(parms.materialParms[SceneEntity::BlueParm], 0.0f, 1.0f);
    BE1::Clamp(parms.materialParms[SceneEntity::AlphaParm], 0.0f, 1.0f);

    // 라이트 타입 별 처리
    if (parms.type == PointLight) {
        obb = OBB(parms.origin, parms.value, parms.axis);

        R_SetOrthogonalProjectionMatrix(parms.value[1], parms.value[2], 0.0f, parms.value[0], projMatrix);
    } else if (parms.type == DirectionalLight) {
        obb = OBB(parms.origin + parms.axis[0] * parms.value[0] * 0.5f, Vec3(parms.value[0] * 0.5f, parms.value[1], parms.value[2]), parms.axis);

        R_SetOrthogonalProjectionMatrix(obb.Extents()[1], obb.Extents()[2], 0.0f, obb.Extents()[0] * 2, projMatrix);
    } else if (parms.type == SpotLight) {
        frustum.SetOrigin(parms.origin);
        frustum.SetAxis(parms.axis);
        frustum.SetSize(BE1::Max(parms.zNear, 3.0f), parms.value[0], parms.value[1], parms.value[2]);

        float xFov = RAD2DEG(Math::ATan(frustum.GetLeft(), frustum.GetFarDistance())) * 2.0f;
        float yFov = RAD2DEG(Math::ATan(frustum.GetUp(), frustum.GetFarDistance())) * 2.0f;

        R_SetPerspectiveProjectionMatrix(xFov, yFov, frustum.GetNearDistance(), frustum.GetFarDistance(), false, projMatrix);
    } else {
        assert(0);
    }

    // NOTE: shader 에서 이미 한번 square 처리가 되므로 여기서 sqrt 해준다
    parms.fallOffExponent = Math::Sqrt(parms.fallOffExponent);

    R_SetViewMatrix(parms.axis, parms.origin, viewMatrix);

    static const Mat4 textureScaleBiasMatrix(Vec4(0.5, 0, 0, 0.5), Vec4(0, 0.5, 0, 0.5), Vec4(0, 0, 0.5, 0.5), Vec4(0.0, 0.0, 0.0, 1));
    viewProjScaleBiasMatrix = textureScaleBiasMatrix * projMatrix * viewMatrix;

    firstUpdate = false;
}

bool SceneLight::IsIntersectAABB(const AABB &aabb) const {
    if (parms.type == DirectionalLight || parms.type == PointLight) {
        if (this->obb.IsIntersectOBB(OBB(aabb))) {
            return true;
        }
        return false;
    }

    if (parms.type == SpotLight) {
        if (!frustum.CullAABB(aabb)) {
            return true;
        }
        return false;
    }

    assert(0);
    return false;
}

bool SceneLight::IsIntersectOBB(const OBB &obb) const {
    if (parms.type == DirectionalLight || parms.type == PointLight) {
        if (this->obb.IsIntersectOBB(obb)) {
            return true;
        }
        return false;
    }

    if (parms.type == SpotLight) {
        if (!frustum.CullOBB(obb)) {
            return true;
        }
        return false;
    }

    assert(0);
    return false;
}

const AABB SceneLight::GetAABB() const {
    if (parms.type == DirectionalLight || parms.type == PointLight) {
        return obb.ToAABB();
    }

    return frustum.ToOBB().ToAABB();
}

static bool DirLight_ShadowBVFromCaster(const SceneLight *light, const OBB &casterOBB, OBB &shadowOBB) {	
    assert(light->parms.type == SceneLight::DirectionalLight);

    AABB b1, b2;
    casterOBB.AxisProjection(light->parms.axis, b1); // light 축에서의 caster bounds
    light->obb.AxisProjection(light->parms.axis, b2); // light 축에서의 light bounds

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

    shadowOBB = OBB(b1, Vec3::origin, light->parms.axis);
    return true;
}

static bool PointLight_ShadowBVFromCaster(const SceneLight *light, const OBB &casterOBB, Frustum &shadowFrustum) {
    assert(light->parms.type == SceneLight::PointLight);

    Vec3 dir = casterOBB.Center() - light->parms.origin;
    dir.Normalize();

    float dFar = light->obb.Extents().Length();	
    if (!shadowFrustum.FromProjection(casterOBB, light->parms.origin, dFar)) {
        return false;
    }

    //renderSystem.GetRenderWorld()->DebugFrustum(shadowFrustum, false, 1.0f, true, true);	
    return true;
}

static bool SpotLight_ShadowBVFromCaster(const SceneLight *light, const OBB &casterOBB, Frustum &shadowFrustum) {
    assert(light->parms.type == SceneLight::SpotLight);

    Vec3 dir = light->frustum.GetAxis()[0];
    dir.Normalize();

    float dmin, dmax;
    casterOBB.AxisProjection(dir, dmin, dmax);

    float dNear = dmin - light->parms.origin.Dot(dir);
    if (dNear < light->frustum.GetNearDistance()) {
        dNear = light->frustum.GetNearDistance();
    }

    shadowFrustum = light->frustum;
    shadowFrustum.MoveNearDistance(dNear);

    return true;
}

bool SceneLight::Cull(const Frustum &viewFrustum) const {
    switch (parms.type) {
    case DirectionalLight:
        if (viewFrustum.CullOBB(obb)) {
            return true;
        }
        break;
    case SpotLight:
        if (viewFrustum.CullFrustum(frustum)) {
            return true;
        }
        break;
    case PointLight:
        if (IsRadiusUniform()) {
            if (viewFrustum.CullSphere(Sphere(parms.origin, parms.value[0]))) {
                return true;
            }
        } else {
            if (viewFrustum.CullOBB(obb)) {
                return true;
            }
        }
        break;
    default:
        break;
    }

    return false;
}

bool SceneLight::Cull(const OBB &viewBox) const {
    switch (parms.type) {
    case DirectionalLight:
        if (!viewBox.IsIntersectOBB(obb)) {
            return true;
        }
        break;
    case SpotLight:
        if (frustum.CullOBB(viewBox)) {
            return true;
        }
        break;
    case PointLight:
        if (IsRadiusUniform()) {
            if (!viewBox.IsIntersectSphere(Sphere(parms.origin, parms.value[0]))) {
                return true;
            }
        } else {
            if (!viewBox.IsIntersectOBB(obb)) {
                return true;
            }
        }
        break;
    default:
        break;
    }

    return false;
}

bool SceneLight::CullShadowCasterOBB(const OBB &casterOBB, const Frustum &viewFrustum, const AABB &visAABB) const {
    if (viewFrustum.CullAABB(visAABB)) {
        return true;
    }

    if (parms.type == DirectionalLight) {
        OBB shadowOBB;
        if (!DirLight_ShadowBVFromCaster(this, casterOBB, shadowOBB)) {
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

    if (parms.type == PointLight) {
        Frustum shadowFrustum;
        if (PointLight_ShadowBVFromCaster(this, casterOBB, shadowFrustum)) {
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

    if (parms.type == SpotLight) {
        Frustum shadowFrustum;
        SpotLight_ShadowBVFromCaster(this, casterOBB, shadowFrustum);
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

bool SceneLight::ComputeScreenClipRect(const SceneView *viewDef, Rect &clipRect) const {
    switch (parms.type) {
    case DirectionalLight:
        if (!viewDef->GetClipRectFromOBB(obb, clipRect)) {
            return false;
        }
        break;
    case SpotLight:
        if (!viewDef->GetClipRectFromFrustum(frustum, clipRect)) {
            return false;
        }
        break;
    case PointLight:
        if (IsRadiusUniform()) {
            if (!viewDef->GetClipRectFromSphere(Sphere(parms.origin, GetRadius()[0]), clipRect)) {
                return false;
            }
        } else {
            if (!viewDef->GetClipRectFromOBB(obb, clipRect)) {
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
