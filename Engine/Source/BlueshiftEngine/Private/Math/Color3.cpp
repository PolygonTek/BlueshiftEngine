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

const Color3  Color3::zero      = Color3(0.0f, 0.0f, 0.0f);
const Color3  Color3::black     = Color3(0.0f, 0.0f, 0.0f);
const Color3  Color3::white     = Color3(1.0f, 1.0f, 1.0f);
const Color3  Color3::red       = Color3(1.0f, 0.0f, 0.0f);
const Color3  Color3::green     = Color3(0.0f, 1.0f, 0.0f);
const Color3  Color3::blue      = Color3(0.0f, 0.0f, 1.0f);
const Color3  Color3::yellow    = Color3(1.0f, 1.0f, 0.0f);
const Color3  Color3::cyan      = Color3(0.0f, 1.0f, 1.0f);
const Color3  Color3::magenta   = Color3(1.0f, 0.0f, 1.0f);
const Color3  Color3::orange    = Color3(1.0f, 0.5f, 0.0f);
const Color3  Color3::pink      = Color3(1.0f, 0.0f, 0.5f);
const Color3  Color3::lawn      = Color3(0.5f, 1.0f, 0.0f);
const Color3  Color3::mint      = Color3(0.0f, 1.0f, 0.5f);
const Color3  Color3::violet    = Color3(0.5f, 0.5f, 1.0f);
const Color3  Color3::teal      = Color3(0.3f, 0.5f, 0.6f);
const Color3  Color3::grey      = Color3(0.7f, 0.7f, 0.7f);
const Color3  Color3::darkGrey  = Color3(0.3f, 0.3f, 0.3f);

void Color3::Clip() {
    r = (r > 1.0f) ? 1.0f : ((r < 0.0f) ? 0.0f : r);
    g = (g > 1.0f) ? 1.0f : ((g < 0.0f) ? 0.0f : g);
    b = (b > 1.0f) ? 1.0f : ((b < 0.0f) ? 0.0f : b);
}

void Color3::Invert() {
    r = 1.0f - r;
    g = 1.0f - g;
    b = 1.0f - b;
}

uint32_t Color3::ToUInt32() const {
    uint32_t r = (uint32_t)Clamp(((int)(this->r * 255.0f)), 0, 255);
    uint32_t g = (uint32_t)Clamp(((int)(this->g * 255.0f)), 0, 255);
    uint32_t b = (uint32_t)Clamp(((int)(this->b * 255.0f)), 0, 255);
    return (255 << 24) | (b << 16) | (g << 8) | r;
}

Color3 Color3::FromString(const char *str) {
    Color3 v;
    sscanf(str, "%f %f %f", &v.r, &v.g, &v.b);
    return v;
}

// RGB 를 HSL 로 변환
Color3 Color3::ToHSL() const {
    float h, s, l;
    float v = Max3(r, g, b);
    float m = Min3(r, g, b);

    if ((l = (m + v) / 2.0f) <= 0.0f) {
        return Color3(0.0f, 0.0f, l);
    }

    float vm;
    if ((s = vm = v - m) > 0.0f) {
        s /= ((l <= 0.5f) ? (v + m) : (2.0f - v - m));
    } else {
        return Color3(0.0f, s, l);
    }

    float r2 = (v - r) / vm;
    float g2 = (v - g) / vm;
    float b2 = (v - b) / vm;

    if (r == v) {
        h = (g == m ? 5.0f + b2 : 1.0f - g2);
    } else if (g == v) {
        h = (b == m ? 1.0f + r2 : 3.0f - b2);
    } else {
        h = (r == m ? 3.0f + g2 : 5.0f - r2);
    }

    h /= 6;

    return Color3(h, s, l);
}

// HSL 을 RGB 로 변환
Color3 Color3::FromHSL() const {
    float h = r;
    float s = g;
    float l = b;
    float v = ((l <= 0.5f) ? (l * (1.0f + s)) : (l + s - l * s));
    float r, g, b;

    if (v <= 0) {
        r = g = b = 0.0f;
    } else {
        float m = l + l - v;
        float sv = (v - m) / v;
        int sextant = h * 6.0f;
        float fract = h * 6.0f - sextant;
        float vsf = v * sv * fract;
        float mid1 = m + vsf;
        float mid2 = v - vsf;

        switch (sextant) {
        case 0:
            r = v;
            g = mid1;
            b = m;
            break;
        case 1:
            r = mid2;
            g = v;
            b = m;
            break;
        case 2:
            r = m;
            g = v;
            b = mid1;
            break;
        case 3:
            r = m;
            g = mid2;
            b = v;
            break;
        case 4:
            r = mid1;
            g = m;
            b = v;
            break;
        case 5:
            r = v;
            g = m;
            b = mid2;
            break;
        }
    }

    return Color3(r, g, b);
}

// http://chilliant.blogspot.kr/2012/08/srgb-approximations-for-hlsl.html
Color3 Color3::SRGBtoLinear() const {
    float r2 = r * r;
    float g2 = g * g;
    float b2 = b * b;

    return Color3(
        0.012522878 * r + 0.682171111 * r2 + 0.305306011 * r2 * r,
        0.012522878 * g + 0.682171111 * g2 + 0.305306011 * g2 * g,
        0.012522878 * b + 0.682171111 * b2 + 0.305306011 * b2 * b);
}

Color3 Color3::LinearToSRGB() const {
    float sqrt_r = Math::Sqrt(r);
    float sqrt_g = Math::Sqrt(g);
    float sqrt_b = Math::Sqrt(b);
    float sqrt2_r = Math::Sqrt(sqrt_r);
    float sqrt2_g = Math::Sqrt(sqrt_g);
    float sqrt2_b = Math::Sqrt(sqrt_b);
    float sqrt3_r = Math::Sqrt(sqrt2_r);
    float sqrt3_g = Math::Sqrt(sqrt2_g);
    float sqrt3_b = Math::Sqrt(sqrt2_b);

    return Color3(
        0.585122381 * sqrt_r + 0.783140355 * sqrt2_r - 0.368262736 * sqrt3_r,
        0.585122381 * sqrt_g + 0.783140355 * sqrt2_g - 0.368262736 * sqrt3_g,
        0.585122381 * sqrt_b + 0.783140355 * sqrt2_b - 0.368262736 * sqrt3_b);
}

BE_NAMESPACE_END