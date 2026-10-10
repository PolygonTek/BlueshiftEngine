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
    static constexpr float          MinLightSize = 0.01f;   // 광원 거리 및 shadow near 보정용 최소 길이.
    static constexpr float          MinSpotOuterConeAngle = 1.0f;
    static constexpr float          MaxSpotOuterConeAngle = 89.0f;
    static constexpr float          MinSpotConeAngleGap = 0.1f; // 코사인 차의 역수가 발산하지 않도록 유지할 최소 각도 차이 (도).

    enum class Type : uint8_t {
        Point,                      ///< Omnidirectional light within a sphere centered at origin
        Spot,                       ///< Circular cone light directed along axis[0], bounded by range
        Directional                 ///< Infinite-distance light whose rays travel along axis[0]
    };

    enum class Flag : uint8_t {
        None                        = 0,
        Static                      = BIT(0),
        CastShadows                 = BIT(1),
        StaticShadows               = BIT(2)  ///< Point/Spot: cache shadows from static casters, independently of camera visibility
    };

    friend constexpr std::true_type EnableBitmaskEnum(Flag) noexcept { return {}; }

    struct Desc {
        enum class UpdateFlag : uint16_t {
            All                     = 0xFFFF,
            Flags                   = BIT(0),   // flags
            Layer                   = BIT(1),   // layer
            Transform               = BIT(2),   // origin, axis
            Shape                   = BIT(3),   // type, range, zNear, innerConeAngle, outerConeAngle
            Lighting                = BIT(4),   // linearColor, intensity
            Shadow                  = BIT(5)    // shadowOffsetUnits, shadowOffsetFactor
        };

        friend constexpr std::true_type EnableBitmaskEnum(UpdateFlag) noexcept { return {}; }
 
        Type                        type = Type::Point;
        Flag                        flags = Flag::None;
        uint8_t                     layer = 0;                          ///< Render layer bit index in [0, 31]

        BE1::Vec3                   origin = BE1::Vec3::origin;         ///< Light position in world space
        BE1::Mat3                   axis = BE1::Mat3::identity;         ///< Light orientation
        float                       range = 1.0f;                       ///< Point/Spot radial attenuation range in meters
        float                       innerConeAngle = 30.0f;             ///< Spot half-angle from the forward axis, in degrees
        float                       outerConeAngle = 45.0f;             ///< Spot half-angle from the forward axis, in degrees

        float                       zNear = 0.1f;                       ///< Shadow near distance, capped at half the range; does not clip illumination
        float                       shadowOffsetUnits = 500.0f;
        float                       shadowOffsetFactor = 5.0f;

        BE1::Color3                 linearColor = BE1::Color3::white;   ///< Linear RGB chromaticity in [0, 1], normalized by luminance; black emits no light
        float                       intensity = 1.0f;                   ///< Finite, nonnegative: Directional normal illuminance in lux; Point/Spot intensity in cd (inside the Spot inner cone)
    };

    RenderLight() = default;
    ~RenderLight() = default;

                                    /// Returns light index in world.
    int                             GetIndex() const { return index; }

                                    /// Returns light type which is one of the [Point, Spot, Directional].
    Type                            GetType() const { return desc.type; }

                                    /// Returns light origin in world space.
    const BE1::Vec3 &               GetOrigin() const { return desc.origin; }

                                    /// Returns radial attenuation range for Point/Spot lights.
    float                           GetRange() const { return desc.range; }

                                    /// Returns AABB in world space.
    const BE1::AABB &               GetWorldAABB() const { return worldAABB; }

                                    /// Returns view matrix.
    const BE1::Mat4 &               GetViewMatrix() const { return viewMatrix; }

                                    /// Returns matrix = texture scale-bias * projection * view using the engine-wide D3D convention.
    const BE1::Mat4 &               GetViewProjScaleBiasMatrix() const { return viewProjScaleBiasMatrix; }

                                    /// Check intersection of AABB and light bounding volume.
    bool                            IsIntersectAABB(const BE1::AABB &aabb) const;

                                    /// Check intersection of OBB and light bounding volume.
    bool                            IsIntersectOBB(const BE1::OBB &obb) const;

                                    /// Calculates minimum / maximum values by projecting light bounding volume onto the given axis.
    void                            ProjectOnAxis(const BE1::Vec3 &axis, float &min, float &max) const;

                                    /// Camera culling of light bounding volume.
    bool                            Cull(const RenderCamera &camera) const;

                                    /// Calculates the screen clipping rectangle of light bounding volume.
    bool                            ComputeScreenClipRect(const RenderCamera &camera, BE1::Rect &clipRect) const;

private:
                                    /// Update this render light.
    void                            Update(const Desc &desc, Desc::UpdateFlag flags);
    void                            UpdateFlags(Flag flags);
    void                            UpdateLayer(uint8_t layer);
    void                            UpdateTransform(const BE1::Vec3 &origin, const BE1::Mat3 &axis);
    void                            UpdateShape(Type type, float range, float zNear, float innerConeAngle, float outerConeAngle);
    void                            UpdateLighting(const BE1::Color3 &linearColor, float intensity);
    void                            UpdateShadow(float shadowOffsetUnits, float shadowOffsetFactor);
    void                            UpdateViewMatrix();
    void                            UpdateProjectionAndBounds();

    Desc                            desc;
    BE1::Color3                     intensityRGB = BE1::Color3::white;  ///< Luminance-normalized RGB times intensity, cached when lighting changes

    BE1::Sphere                     worldSphere;                        ///< Radial range bound for Point/Spot lights
    BE1::Frustum                    worldFrustum;                       ///< Conservative square frustum enclosing the circular Spot cone, starting at its apex
    BE1::AABB                       worldAABB = BE1::AABB::empty;       ///< World-space AABB enclosing the light volume; empty for directional lights

    BE1::Vec2                       spotCosCone = BE1::Vec2::zero;      ///< X = cos(inner), Y = cos(outer); updated only when shape changes
    float                           spotInvCosConeDifference = 0.0f;    ///< 1 / (cos(inner) - cos(outer))
    float                           spotTanOuterCone = 0.0f;            ///< tan(outer)

    BE1::Mat4                       viewMatrix = BE1::Mat4::identity;
    BE1::Mat4                       projMatrix = BE1::Mat4::identity;
    BE1::Mat4                       viewProjScaleBiasMatrix = BE1::Mat4::identity;

    uint64_t                        visibleFrameIndex = 0;
    uint64_t                        lightId = 0;                        ///< light 생성 시 부여되는 고유 ID.
    uint64_t                        shadowRevision = 0;                 ///< shadow 에 영향을 주는 설정 변경 시 증가. 카메라별 atlas 캐시 무효화에 사용한다.
    int                             index = -1;                         ///< RenderWorld::renderLights 의 인덱스
    uint64_t                        viewCount = uint64_t(-1);           ///< 마지막으로 FrameLight 로 등록된 뷰의 카운터 값. 같은 뷰에서 중복 등록 방지.
};
