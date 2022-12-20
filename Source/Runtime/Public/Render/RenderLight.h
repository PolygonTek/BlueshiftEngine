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

class Material;
class VisLight;
class RenderCamera;

class RenderLight {
    friend class RenderCamera;
    friend class RenderWorld;

public:
    struct Type {
        enum Enum {
            Point,
            Spot,
            Directional
        };
    };

    struct Flag {
        enum Enum {
            Static          = BIT(0),
            CastShadows     = BIT(1),
            PrimaryLight    = BIT(2)
        };
    };

    struct State {
        int                 flags = 0;
        int                 layer = 0;
        int                 staticMask = 0;
        Type::Enum          type = Type::Point;
        float               maxVisDist = MeterToUnit(10.0f);

        Vec3                origin = Vec3::origin;          ///< Light position in world space
        Vec3                size = Vec3::one;               ///< Light extents for each axis
        Mat3                axis = Mat3::identity;          ///< Light orientation

        float               zNear = 1.0f;                   ///< Near distance for Light::Spot

        float               intensity = 1.0f;               ///< Light intensity
        float               fallOffExponent = 1.25f;
        float               shadowOffsetFactor = 3.0f;
        float               shadowOffsetUnits = 200.f;

        Material *          material = nullptr;
        float               materialParms[RenderObject::MaterialParm::Count] = { 1, 1, 1, 1, 0, 1 };
    };

    RenderLight(RenderWorld *renderWorld, int index);
    ~RenderLight();

                            /// Returns state.
    const State &           GetState() const { return state; }

                            /// Returns light type which is one of the [Point, Spot, Directional].
    Type::Enum              GetType() const { return state.type; }

                            /// Returns light material.
    const Material *        GetMaterial() const { return state.material; }

                            /// Returns light origin in world space.
    const Vec3 &            GetOrigin() const { return state.origin; }

                            /// Returns extent for each axis.
    const Vec3 &            GetExtents() const { return state.size; }

                            /// Returns radius for each axis. Valid only for point light.
    const Vec3 &            GetRadius() const { return state.size; }

                            /// Returns maximum radius of each axis. Valid only for point light.
    const float             GetMajorRadius() const { return BE1::Max3(state.size.x, state.size.y, state.size.z); }

                            /// Is the radius the same for each axis? Valid only for point light.
    bool                    IsRadiusUniform() const { return (state.size.x == state.size.y && state.size.x == state.size.z) ? true : false; }

                            /// Returns world AABB.
    const AABB &            GetWorldAABB() const { return worldAABB; }

                            /// Returns world bounding frustum. Valid only for projected light.
    const Frustum &         GetWorldFrustum() const { return worldFrustum; }

                            /// Returns world object-oriendted bounding box. Valid only for directional/point light.
    const OBB &             GetWorldOBB() const { return worldOBB; }

                            /// Returns view matrix.
    const Mat4 &            GetViewMatrix() const { return viewMatrix; }

                            /// Returns matrix = bias * scale * proj * view (multiplication in OpenGL way)
    const Mat4 &            GetViewProjScaleBiasMatrix() const { return viewProjScaleBiasMatrix; }

                            /// Returns fall off matrix.
    const Mat3x4 &          GetFallOffMatrix() const { return fallOffMatrix; }

                            /// Check intersection of AABB and light bounding volume.
    bool                    IsIntersectAABB(const AABB &aabb) const;

                            /// Check intersection of OBB and light bounding volume.
    bool                    IsIntersectOBB(const OBB &obb) const;

                            /// Camera culling of lighting bounding volume.
    bool                    Cull(const RenderCamera &camera) const;

                            /// Frustum culling of shadow caster OBB.
    bool                    CullShadowCaster(const OBB &casterOBB, const Frustum &viewFrustum, const AABB &visAABB) const;

                            //
    bool                    ComputeScreenClipRect(const RenderCamera *viewDef, Rect &clipRect) const;

private:
                            /// Updates this render light with the given state.
    void                    Update(const State *state);

                            /// Frustum culling of light bounding volume.
    bool                    Cull(const Frustum &viewFrustum) const;

                            /// Box culling of light bounding volume.
    bool                    Cull(const OBB &viewBox) const;

    bool                    DirLight_ShadowBVFromCaster(const OBB &casterOBB, OBB &shadowOBB) const;
    bool                    PointLight_ShadowBVFromCaster(const OBB &casterOBB, Frustum &shadowFrustum) const;
    bool                    SpotLight_ShadowBVFromCaster(const OBB &casterOBB, Frustum &shadowFrustum) const;

    State                   state;

    bool                    firstUpdate;

    AABB                    worldAABB;
    OBB                     worldOBB;           // used for Light::Point, Light::Directional
    Frustum                 worldFrustum;       // used for Light::Spot
    Mat4                    viewMatrix;
    Mat4                    projMatrix;
    Mat4                    viewProjScaleBiasMatrix;
    Mat3x4                  fallOffMatrix;
    float                   maxVisDistSquared;

    VisLight *              visLight;
    int                     viewCount;

    RenderWorld *           renderWorld;
    int                     index;              // index of light list in RenderWorld
    DbvtProxy *             proxy;
};

BE_NAMESPACE_END
