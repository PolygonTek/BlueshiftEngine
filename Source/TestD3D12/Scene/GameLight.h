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

#include "Render/Frontend/RenderLight.h"

class RenderWorld;

class GameLight {
public:
    bool                        SetEnabled(RenderWorld *renderWorld, bool enabled);

    RenderLight::Type           type = RenderLight::Type::Point;
    RenderLight::Flag           flags = RenderLight::Flag::None;
    uint8_t                     layer = 0;
    BE1::Vec3                   origin = BE1::Vec3::origin;
    BE1::Mat3                   axis = BE1::Mat3::identity;
    float                       range = 1.0f;                       // Point/Spot 거리 감쇠 반경 (m).
    float                       innerConeAngle = 30.0f;             // Spot 중심축에서 잰 반각 (도).
    float                       outerConeAngle = 45.0f;             // Spot 중심축에서 잰 반각 (도).
    float                       zNear = 0.01f;
    BE1::Color3                 linearColor = BE1::Color3::white;
    float                       intensity = 1.0f;
    float                       shadowOffsetUnits = 1500.0f;
    float                       shadowOffsetFactor = 3.0f;
    int                         renderLightHandle = -1;
};
