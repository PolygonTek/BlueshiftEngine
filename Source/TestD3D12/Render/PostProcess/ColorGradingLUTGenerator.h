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

#include "Image/Image.h"
#include "Math/Color3.h"
#include "Shaders/ShaderInterop.h"

/*
---------------------------------------------------------------------------------------------------

    Color Grading LUT Generator

---------------------------------------------------------------------------------------------------
*/

class ColorGradingLUTGenerator {
public:
    struct Settings {
        float                       saturation = 1.0f;
        float                       contrast = 1.0f;
        float                       gamma = 1.0f;
        float                       gain = 1.0f;
        BE1::Color3                 tint = BE1::Color3(1.0f);
    };

    static constexpr int            LUTSize = ColorGradingLUTSize;

    static void                     Generate(const Settings &settings, BE1::Image &outImage);
};
