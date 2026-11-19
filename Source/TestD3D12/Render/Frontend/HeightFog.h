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

class HeightFog {
    friend class RenderWorld;

public:
    struct Desc {
        bool                        enabled = true;
        float                       baseHeight = 0.0f;                  ///< 기준 월드 높이 (m).
        float                       heightFalloff = 0.2f;               ///< exp2 감쇠율 (1/m). 0 이면 균일 매질.
        float                       maxDistance = 6000.0f;              ///< 하늘과 표면에 적용할 최대 광선 거리 (m).
        MediumParams                medium = { 0.04f, float3(0.9f, 0.9f, 0.9f), 0.0f, {} }; ///< 기준 높이에서의 광학적 매질 속성.
        BE1::Color3                 ambientColor = BE1::Color3::white;  ///< Height fog 에 모든 방향에서 일정하게 들어온다고 가정하는 주변광.
        float                       ambientIntensity = 0.0f;

        bool                        IsValid() const;

                                    // 활성화된 매질에 소멸이나 자체 발광이 있는지 확인한다.
        bool                        IsActive() const;
    };

    const Desc &                    GetDesc() const { return desc; }

private:
    Desc                            desc;
};
