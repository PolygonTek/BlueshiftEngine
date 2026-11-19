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

#include "Shaders/Fog/FogInterop.h"

class RenderWorld;
class Texture;

class LocalFogVolume {
    friend class RenderWorld;

public:
    enum class Shape : uint8_t {
        Sphere,
        Box
    };

    enum class MaskAddressMode : uint8_t {
        Repeat,
        Clamp
    };

    struct Desc {
        bool                        enabled = true;
        Shape                       shape = Shape::Sphere;
        BE1::Vec3                   origin = BE1::Vec3::origin;
        BE1::Mat3                   axis = BE1::Mat3::identity;
        float                       radius = 1.0f;                      ///< 구의 반지름 (m).
        BE1::Vec3                   halfExtents = BE1::Vec3(1.0f);      ///< 박스의 로컬 축별 반길이 (m).
        float                       boundaryFade = 0.1f;                ///< 경계 감쇠 폭의 비율 [0, 1]. 구의 반지름 또는 박스의 최소 반길이 기준.
        MediumParams                medium = { 0.02f, float3(0.9f, 0.9f, 0.9f), 0.0f, {} }; ///< 경계, 높이 감쇠, 마스크를 적용하기 전의 광학적 매질 속성.
        SharedPtr<Texture>          maskTexture;                        ///< 선형 3D 마스크 [0, 1]: RGB 는 산란 albedo 배율, A 는 밀도 배율.
        MaskAddressMode             maskAddressMode = MaskAddressMode::Repeat; ///< U/V/W 공통 addressing. Clamp 는 텍스처 경계 texel 을 유지한다.
        BE1::Vec3                   maskTiling = BE1::Vec3(1.0f);       ///< 볼륨 전체 길이 (구는 지름) 의 로컬 축별 UV 범위. Repeat 에서는 반복 횟수이며, 음수는 축을 반전한다.
        BE1::Vec3                   maskScrollSpeed = BE1::Vec3::origin;///< Repeat 전용 로컬 축별 스크롤 속도 (UVW/s). Clamp 에서는 무시한다.
        float                       heightFogFalloff = 0.0f;            ///< 기준면 위쪽에 적용하는 월드 Z 방향의 exp2 높이 감쇠율 (1/m). 0 이면 높이에 따른 변화가 없다.
        float                       heightFogOffset = 0.0f;             ///< 볼륨 중심에서 높이 감쇠가 시작되는 기준면까지의 월드 Z 오프셋 (m).

                                    // 현재 형상에서 사용하는 설정값을 검사한다. 월드 bounds 의 유한성은 등록/갱신 시 추가 검사한다.
        bool                        IsValid() const;

                                    // 소멸이 없는 자체 발광 매질도 활성 볼륨으로 취급한다.
        bool                        IsActive() const;
    };

    const Desc &                    GetDesc() const { return desc; }

                                    /// Returns local fog volume index in world.
    int                             GetIndex() const { return index; }

                                    /// Returns AABB in world space.
    const BE1::AABB &               GetWorldAABB() const { return worldAABB; }

private:
                                    // 검증된 설정을 반영한다. bounds 를 표현할 수 없으면 기존 상태를 유지한다.
    bool                            Update(const Desc &desc);

                                    // frame 당 한 번 스크롤을 진행한다.
    void                            AdvanceMaskScroll(float frameTime, uint64_t frameIndex);

    Desc                            desc;
    BE1::AABB                       worldAABB = BE1::AABB::empty;
    float                           maskScrollOffset[3] = {};           // Repeat 에서는 [0, 1) 로 정규화한 누적 UVW 이동량. Clamp 에서는 0 을 유지한다.
    uint64_t                        maskScrollFrameIndex = 0;
    int                             index = -1;
};
