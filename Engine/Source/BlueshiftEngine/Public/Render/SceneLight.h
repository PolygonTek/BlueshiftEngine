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

#pragma once

/*
-------------------------------------------------------------------------------

    SceneLight

-------------------------------------------------------------------------------
*/

#include "Render/SceneEntity.h"

BE_NAMESPACE_BEGIN

struct viewLight_t;
struct DbvtProxy;

class Material;
class SceneView;

class SceneLight {
    friend class SceneView;
    friend class RenderWorld;

public:
    enum Type {
        PointLight,
        SpotLight,
        DirectionalLight
    };

    struct Parms {
        int                 layer;
        Type                type;
        Material *          material;
        float               materialParms[SceneEntity::MaxMaterialParms];
        float               intensity;

        Vec3                origin;
        Mat3                axis;

                            // Point Light 일 경우 radius 값, 또는
                            // Directional/Spot Light 일 경우 각 xyz axis 별 extent 값
        Vec3                value;

                            // for SpotLight
        float               zNear;

        float               fallOffExponent;
        float               maxVisDist;
        float               shadowOffsetFactor;
        float               shadowOffsetUnits;

        bool                turnOn;
        bool                castShadows;
        bool                isPrimaryLight;
        bool                isStaticLight;
    };

    SceneLight();
    ~SceneLight();

    void                    Update(const Parms *parms);

                            /// Returns light type (Point, Spot, Directional)
    Type                    GetType() const { return parms.type; }

                            /// Returns light material
    const Material *        GetMaterial() const { return parms.material; }

                            /// Returns light origin
    const Vec3 &            GetOrigin() const { return parms.origin; }

                            // directional OBB 라이트의 extents
    const Vec3 &            GetExtents() const { return parms.value; }

                            // 라이트 타원체의 각 axis 당 반지름 - Point 라이트인 경우에만
    const Vec3 &            GetRadius() const { return parms.value; }

                            // axis 별 가장 큰 반지름 - Point 라이트인 경우에만
    const float             GetMajorRadius() const { return BE1::Max3(parms.value.x, parms.value.y, parms.value.z); }

                            // axis 별 반지름의 크기가 동일한가 - Point 라이트인 경우에만
    bool                    IsRadiusUniform() const { return (parms.value.x == parms.value.y && parms.value.x == parms.value.z) ? true : false; }	

                            // frustum - Projected 라이트인 경우에만
    const Frustum &         GetFrustum() const { return frustum; }

                            // OBB - Directional/Point 라이트인 경우에만
    const OBB &             GetOBB() const { return obb; }

                            // AABB - 개략적인 bounding volume
    const AABB              GetAABB() const;

                            // 라이트 view matrix
    const Mat4 &            GetViewMatrix() const { return viewMatrix; }

                            // 라이트 bias * scale * proj * view matrix (곱셈은 OpenGL 순서)
    const Mat4 &            GetViewProjScaleBiasMatrix() const { return viewProjScaleBiasMatrix; }

                            // aabb 가 light bounding volume 과 교차하는지 테스트
    bool                    IsIntersectAABB(const AABB &aabb) const;

                            // obb 가 light bounding volume 과 교차하는지 테스트
    bool                    IsIntersectOBB(const OBB &obb) const;

                            // light type 별로 bounding volume culling
    bool                    Cull(const Frustum &viewFrustum) const;

                            // light type 별로 bounding volume culling
    bool                    Cull(const OBB &viewBox) const;

                            // shadow caster OBB culling
    bool                    CullShadowCasterOBB(const OBB &casterOBB, const Frustum &viewFrustum, const AABB &visAABB) const;

                            //
    bool                    ComputeScreenClipRect(const SceneView *viewDef, Rect &clipRect) const;

    int                     index;
    Parms                   parms;
    bool                    firstUpdate;
    OBB                     obb;            // used for PointLight / DirectionalLight
    Frustum                 frustum;        // used for SpotLight
    Mat4                    viewMatrix;
    Mat4                    projMatrix;
    Mat4                    viewProjScaleBiasMatrix;
    int                     viewCount;
    viewLight_t *           viewLight;
    DbvtProxy *             proxy;
};

BE_NAMESPACE_END
