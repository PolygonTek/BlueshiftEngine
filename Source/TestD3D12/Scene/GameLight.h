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

class GameLight {
public:
    RenderLight::Type           type = RenderLight::Type::Point;
    RenderLight::Flag           flags = RenderLight::Flag::None;
    uint8_t                     layer = 0;
    BE1::Vec3                   origin = BE1::Vec3::origin;
    BE1::Mat3                   axis = BE1::Mat3::identity;
    BE1::Vec3                   size = BE1::Vec3::one;
    float                       zNear = 1.0f;
    BE1::Color3                 linearColor = BE1::Color3::white;
    float                       intensity = 1.0f;
    float                       fallOffExponent = 1.25f;
    float                       shadowOffsetFactor = 3.0f;
    float                       shadowOffsetUnits = 200.0f;
    int                         renderLightHandle = -1;
};
