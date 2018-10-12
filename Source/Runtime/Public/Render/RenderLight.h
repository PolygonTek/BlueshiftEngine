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

    RenderLight

-------------------------------------------------------------------------------
*/

#include "Render/RenderObject.h"

BE_NAMESPACE_BEGIN

struct DbvtProxy;

class VisibleLight;
class Material;
class RenderView;

class RenderLight {
    friend class RenderView;
    friend class RenderWorld;

public:
    enum Type {
        PointLight,
        SpotLight,
        DirectionalLight
    };

    enum Flag {
        StaticFlag          = BIT(0),
        CastShadowsFlag     = BIT(1),
        PrimaryLightFlag    = BIT(2)
    };

    struct State {
        int                 flags;
        int                 layer;
        Type                type;
        float               maxVisDist;

        Vec3                origin;
        Vec3                size;       // extent for each axis
        Mat3                axis;

        float               zNear;      // near distance for SpotLight

        float               intensity;
        float               fallOffExponent;
        float               shadowOffsetFactor;
        float               shadowOffsetUnits;

        Material *          material;
        float               materialParms[RenderObject::MaxMaterialParms];
    };

    RenderLight();
    ~RenderLight();

    void                    Update(const State *state);

                            /// Returns light type (Point, Spot, Directional).
    Type                    GetType() const { return state.type; }

                            /// Returns light material.
    const Material *        GetMaterial() const { return state.material; }

                            /// Returns light origin in world space.
    const Vec3 &            GetOrigin() const { return state.origin; }

                            /// Returns extent for each axis.
    const Vec3 &            GetExtents() const { return state.size; }

                            // 라이트 타원체의 각 axis 당 반지름 - Point 라이트인 경우에만
    const Vec3 &            GetRadius() const { return state.size; }

                            // axis 별 가장 큰 반지름 - Point 라이트인 경우에만
    const float             GetMajorRadius() const { return BE1::Max3(state.size.x, state.size.y, state.size.z); }

                            // axis 별 반지름의 크기가 동일한가 - Point 라이트인 경우에만
    bool                    IsRadiusUniform() const { return (state.size.x == state.size.y && state.size.x == state.size.z) ? true : false; }

                            // AABB - 개략적인 bounding volume
    const AABB              GetWorldAABB() const;

                            // frustum - Projected 라이트인 경우에만
    const Frustum &         GetWorldFrustum() const { return worldFrustum; }

                            // OBB - Directional/Point 라이트인 경우에만
    const OBB &             GetWorldOBB() const { return worldOBB; }

                            /// Returns view matrix.
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
    bool                    ComputeScreenClipRect(const RenderView *viewDef, Rect &clipRect) const;

    int                     index;
    bool                    firstUpdate;

    State                   state;

    OBB                     worldOBB;           // used for PointLight / DirectionalLight
    Frustum                 worldFrustum;       // used for SpotLight
    Mat4                    viewMatrix;
    Mat4                    projMatrix;
    Mat4                    viewProjScaleBiasMatrix;
    Mat3x4                  fallOffMatrix;

    VisibleLight *          visLight;
    int                     viewCount;

    DbvtProxy *             proxy;
};

BE_NAMESPACE_END
