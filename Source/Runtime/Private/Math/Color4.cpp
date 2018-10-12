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

#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

const Color4  Color4::zero      = Color4(0.0f, 0.0f, 0.0f, 0.0f);
const Color4  Color4::black     = Color4(0.0f, 0.0f, 0.0f, 1.0f);
const Color4  Color4::white     = Color4(1.0f, 1.0f, 1.0f, 1.0f);
const Color4  Color4::red       = Color4(1.0f, 0.0f, 0.0f, 1.0f);
const Color4  Color4::green     = Color4(0.0f, 1.0f, 0.0f, 1.0f);
const Color4  Color4::blue      = Color4(0.0f, 0.0f, 1.0f, 1.0f);
const Color4  Color4::yellow    = Color4(1.0f, 1.0f, 0.0f, 1.0f);
const Color4  Color4::cyan      = Color4(0.0f, 1.0f, 1.0f, 1.0f);
const Color4  Color4::magenta   = Color4(1.0f, 0.0f, 1.0f, 1.0f);
const Color4  Color4::orange    = Color4(1.0f, 0.5f, 0.0f, 1.0f);
const Color4  Color4::pink      = Color4(1.0f, 0.0f, 0.5f, 1.0f);
const Color4  Color4::lawn      = Color4(0.5f, 1.0f, 0.0f, 1.0f);
const Color4  Color4::mint      = Color4(0.0f, 1.0f, 0.5f, 1.0f);
const Color4  Color4::violet    = Color4(0.5f, 0.5f, 1.0f, 1.0f);
const Color4  Color4::teal      = Color4(0.3f, 0.5f, 0.6f, 1.0f);
const Color4  Color4::grey      = Color4(0.7f, 0.7f, 0.7f, 1.0f);
const Color4  Color4::darkGrey  = Color4(0.3f, 0.3f, 0.3f, 1.0f);

void Color4::Clip(bool clipAlpha) {
    r = (r > 1.0f) ? 1.0f : ((r < 0.0f) ? 0.0f : r);
    g = (g > 1.0f) ? 1.0f : ((g < 0.0f) ? 0.0f : g);
    b = (b > 1.0f) ? 1.0f : ((b < 0.0f) ? 0.0f : b);

    if (clipAlpha) {
        a = (a > 1.0f) ? 1.0f : ((a < 0.0f) ? 0.0f : a);
    }
}

void Color4::Invert(bool invertAlpha) {
    r = 1.0f - r;
    g = 1.0f - g;
    b = 1.0f - b;

    if (invertAlpha) {
        a = 1.0f - a;
    }
}

uint32_t Color4::ToUInt32() const {
    uint32_t r = (uint32_t)Clamp(((int)(this->r * 255.0f)), 0, 255);
    uint32_t g = (uint32_t)Clamp(((int)(this->g * 255.0f)), 0, 255);
    uint32_t b = (uint32_t)Clamp(((int)(this->b * 255.0f)), 0, 255);
    uint32_t a = (uint32_t)Clamp(((int)(this->a * 255.0f)), 0, 255);
    return (a << 24) | (b << 16) | (g << 8) | r;
}

Color4 Color4::FromString(const char *str) {
    Color4 v;
    sscanf(str, "%f %f %f %f", &v.r, &v.g, &v.b, &v.a);
    return v;
}

BE_NAMESPACE_END
