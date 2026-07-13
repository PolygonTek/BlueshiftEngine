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

#pragma once

class Material;
class RenderCamera;
class RenderWorld;

class RenderLight {
    friend class RenderWorld;

public:
    enum class Type : uint8_t {
        Point,
        Spot,
        BoxDir                      ///< Finite box-volume light with a constant direction along axis[0]
    };

    enum class Flag : uint8_t {
        None                        = 0,
        Static                      = BIT(0),
        CastShadows                 = BIT(1)
    };

    struct Desc {
        enum class UpdateFlag : uint16_t {
            All                     = 0xFFFF,
            Transform               = BIT(0),   // origin, axis
            Shape                   = BIT(1),   // type, size, zNear
        };
 
        Type                        type = Type::Point;
        Flag                        flags = Flag::None;

        BE1::Vec3                   origin = BE1::Vec3::origin;         ///< Light position in world space
        BE1::Mat3                   axis = BE1::Mat3::identity;         ///< Light orientation
        BE1::Vec3                   size = BE1::Vec3::one;              ///< Light extents for each axis

        float                       zNear = 1.0f;                       ///< Near distance for spot light
        RHI::ProjectionPolicy       projectionPolicy;                   ///< projection convention used for shader-visible matrices

        BE1::Color3                 linearColor = BE1::Color3::white;   ///< Linear RGB light tint in [0, 1]
        float                       intensity = 1.0f;                   ///< Light itensity
        float                       fallOffExponent = 1.25f;
        float                       shadowOffsetFactor = 3.0f;
        float                       shadowOffsetUnits = 200.0f;
    };

    RenderLight() = default;
    ~RenderLight() = default;

                                    /// Returns light index in world.
    int                             GetIndex() const { return index; }

                                    /// Returns light type which is one of the [Point, Spot, Box].
    Type                            GetType() const { return desc.type; }

                                    /// Returns light origin in world space.
    const BE1::Vec3 &               GetOrigin() const { return desc.origin; }

                                    /// Returns extent for each axis.
    const BE1::Vec3 &               GetExtents() const { return desc.size; }

                                    /// Returns radius for each axis. Valid only for point light.
    const BE1::Vec3 &               GetRadius() const { return desc.size; }

                                    /// Returns maximum radius of each axis. Valid only for point light.
    const float                     GetMajorRadius() const { return BE1::Max3(desc.size.x, desc.size.y, desc.size.z); }

                                    /// Is the radius the same for each axis? Valid only for point light.
    bool                            IsRadiusUniform() const { return (desc.size.x == desc.size.y && desc.size.x == desc.size.z) ? true : false; }

                                    /// Returns AABB in world space.
    const BE1::AABB &               GetWorldAABB() const { return worldAABB; }

                                    /// Returns view matrix.
    const BE1::Mat4 &               GetViewMatrix() const { return viewMatrix; }

                                    /// Returns matrix = texture scale-bias * projection * view using the engine-wide D3D convention.
    const BE1::Mat4 &               GetViewProjScaleBiasMatrix() const { return viewProjScaleBiasMatrix; }

                                    /// Returns fall off matrix.
    const BE1::Mat3x4 &             GetFallOffMatrix() const { return fallOffMatrix; }

                                    /// Check intersection of AABB and light bounding volume.
    bool                            IsIntersectAABB(const BE1::AABB &aabb) const;

                                    /// Check intersection of OBB and light bounding volume.
    bool                            IsIntersectOBB(const BE1::OBB &obb) const;

                                    /// Camera culling of light bounding volume.
    bool                            Cull(const RenderCamera &camera) const;

                                    /// Calculates the screen clipping rectangle of light bounding volume.
    bool                            ComputeScreenClipRect(const RenderCamera &camera, BE1::Rect &clipRect) const;

private:
                                    /// Update this render light.
    void                            Update(const Desc &desc, Desc::UpdateFlag flags);

    Desc                            desc;

    BE1::OBB                        worldOBB;                       // bounding volume for point/box-dir light
    BE1::Frustum                    worldFrustum;                   // bounding volume for spot light
    BE1::AABB                       worldAABB = BE1::AABB::empty;

    BE1::Mat4                       viewMatrix = BE1::Mat4::identity;
    BE1::Mat4                       projMatrix = BE1::Mat4::identity;
    BE1::Mat4                       viewProjScaleBiasMatrix = BE1::Mat4::identity;
    BE1::Mat3x4                     fallOffMatrix;

    uint64_t                        visibleFrameIndex = 0;
    int                             index = -1;                     // RenderWorld::renderLights 의 인덱스
    uint64_t                        viewCount = uint64_t(-1);       // 마지막으로 VisLight 에 등록된 DrawCamera or DrawGUICamera 함수의 실행 카운터
};

template<>
struct enable_bitmask_operators<RenderLight::Flag> {
    static constexpr bool enable = true;
};

template<>
struct enable_bitmask_operators<RenderLight::Desc::UpdateFlag> {
    static constexpr bool enable = true;
};
