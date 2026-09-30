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
    static constexpr float          MinLightSize = 0.01f;   // 광원 크기, near 거리, near-far 간격 보정용 최소 길이.

    enum class Type : uint8_t {
        Point,                      ///< Omnidirectional light within a sphere centered at origin
        Spot,                       ///< Perspective-frustum light directed along axis[0]
        BoxDir,                     ///< Finite box-volume light with a constant direction along axis[0]
        Directional                 ///< Infinite-distance light whose rays travel along axis[0]
    };

    enum class Flag : uint8_t {
        None                        = 0,
        Static                      = BIT(0),
        CastShadows                 = BIT(1)
    };

    friend constexpr std::true_type EnableBitmaskEnum(Flag) noexcept { return {}; }

    struct Desc {
        enum class UpdateFlag : uint16_t {
            All                     = 0xFFFF,
            Flags                   = BIT(0),   // flags
            Layer                   = BIT(1),   // layer
            Transform               = BIT(2),   // origin, axis
            Shape                   = BIT(3),   // type, size, zNear
            ProjectionPolicy        = BIT(4),   // projectionPolicy
            Lighting                = BIT(5),   // linearColor, intensity, fallOffExponent
            Shadow                  = BIT(6)    // shadowOffsetUnits, shadowOffsetFactor
        };

        friend constexpr std::true_type EnableBitmaskEnum(UpdateFlag) noexcept { return {}; }
 
        Type                        type = Type::Point;
        Flag                        flags = Flag::None;
        uint8_t                     layer = 0;                          ///< Render layer bit index in [0, 31]

        BE1::Vec3                   origin = BE1::Vec3::origin;         ///< Light position in world space
        BE1::Mat3                   axis = BE1::Mat3::identity;         ///< Light orientation
        BE1::Vec3                   size = BE1::Vec3::one;              ///< Point: size[0] is radius. Spot/BoxDir: shape extents

        float                       zNear = 1.0f;                       ///< Spot near distance; Point shadow near distance (capped at half the radius)
        RHI::ProjectionPolicy       projectionPolicy;                   ///< projection convention used for shader-visible matrices

        BE1::Color3                 linearColor = BE1::Color3::white;   ///< Linear RGB light tint in [0, 1]
        float                       intensity = 1.0f;                   ///< Light itensity
        float                       fallOffExponent = 1.25f;
        float                       shadowOffsetUnits = 1500.0f;
        float                       shadowOffsetFactor = 3.0f;
    };

    RenderLight() = default;
    ~RenderLight() = default;

                                    /// Returns light index in world.
    int                             GetIndex() const { return index; }

                                    /// 그림자 입력의 변경 번호. 카메라별 캐시가 저장한 번호와 비교한다.
    uint64_t                        GetShadowRevision() const { return shadowRevision; }

                                    /// Returns light type which is one of the [Point, Spot, BoxDir, Directional].
    Type                            GetType() const { return desc.type; }

                                    /// Returns light origin in world space.
    const BE1::Vec3 &               GetOrigin() const { return desc.origin; }

                                    /// Returns extent for each axis.
    const BE1::Vec3 &               GetExtents() const { return desc.size; }

                                    /// Returns radius. Valid only for point light.
    float                           GetRadius() const { return desc.size[0]; }

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
    void                            UpdateShape(Type type, const BE1::Vec3 &size, float zNear);
    void                            UpdateProjectionPolicy(const RHI::ProjectionPolicy &projectionPolicy);
    void                            UpdateLighting(const BE1::Color3 &linearColor, float intensity, float fallOffExponent);
    void                            UpdateShadow(float shadowOffsetUnits, float shadowOffsetFactor);
    void                            UpdateViewMatrix();
    void                            UpdateProjectionAndBounds();

    Desc                            desc;

    BE1::Sphere                     worldSphere;                        // bounding volume for point light
    BE1::OBB                        worldOBB;                           // bounding volume for box-dir light
    BE1::Frustum                    worldFrustum;                       // bounding volume for spot light
    BE1::AABB                       worldAABB = BE1::AABB::empty;       // World-space AABB enclosing the light volume; empty for directional lights

    BE1::Mat4                       viewMatrix = BE1::Mat4::identity;
    BE1::Mat4                       projMatrix = BE1::Mat4::identity;
    BE1::Mat4                       viewProjScaleBiasMatrix = BE1::Mat4::identity;
    BE1::Mat3x4                     fallOffMatrix;

    uint64_t                        visibleFrameIndex = 0;
    uint64_t                        lightId = 0;                        ///< 광원 생성 시 부여되는 고유 ID. Index 재사용이나 world 교체에도 이전 광원과 구분한다.
    uint64_t                        shadowRevision = 1;                 ///< 0 은 아직 렌더링하지 않은 캐시의 초기값으로 남긴다.
    int                             index = -1;                         // RenderWorld::renderLights 의 인덱스
    uint64_t                        viewCount = uint64_t(-1);           // 마지막으로 FrameLight 로 등록된 뷰의 카운터 값. 같은 뷰에서 중복 등록 방지.
};
