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

const Color3 Color3::zero       = Color3::FromRGB888(  0,   0,   0);

const Color3 Color3::black      = Color3::FromRGB888(  0,   0,   0);
const Color3 Color3::white      = Color3::FromRGB888(255, 255, 255);
const Color3 Color3::grey       = Color3::FromRGB888(128, 128, 128);
const Color3 Color3::red        = Color3::FromRGB888(255,   0,   0);
const Color3 Color3::maroon     = Color3::FromRGB888(128,   0,   0);
const Color3 Color3::lime       = Color3::FromRGB888(  0, 255,   0);
const Color3 Color3::green      = Color3::FromRGB888(  0, 128,   0);
const Color3 Color3::blue       = Color3::FromRGB888(  0,   0, 255);
const Color3 Color3::navy       = Color3::FromRGB888(  0,   0, 128);
const Color3 Color3::yellow     = Color3::FromRGB888(255, 255,   0);
const Color3 Color3::olive      = Color3::FromRGB888(128, 128,   0);
const Color3 Color3::cyan       = Color3::FromRGB888(  0, 255, 255);
const Color3 Color3::teal       = Color3::FromRGB888(  0, 128, 128);
const Color3 Color3::magenta    = Color3::FromRGB888(255,   0, 255);
const Color3 Color3::purple     = Color3::FromRGB888(128,   0, 128);

const Color3 Color3::indianRed  = Color3::FromRGB888(205,  92,  92);
const Color3 Color3::lightCoral = Color3::FromRGB888(240, 128, 128);
const Color3 Color3::salmon     = Color3::FromRGB888(250, 128, 114);
const Color3 Color3::orange     = Color3::FromRGB888(255, 127,   0);
const Color3 Color3::pink       = Color3::FromRGB888(255,   0, 127);
const Color3 Color3::lawn       = Color3::FromRGB888(127, 255,   0);
const Color3 Color3::mint       = Color3::FromRGB888(  0, 255, 127);
const Color3 Color3::violet     = Color3::FromRGB888(127, 127, 255);
const Color3 Color3::aquamarine = Color3::FromRGB888(112, 219, 147);
const Color3 Color3::lightGrey  = Color3::FromRGB888(211, 211, 211);
const Color3 Color3::darkGrey   = Color3::FromRGB888( 84,  84,  84);
const Color3 Color3::darkBrown  = Color3::FromRGB888( 92,  64,  51);

void Color3::Clip() {
    Clamp01(r);
    Clamp01(g);
    Clamp01(b);
}

void Color3::Invert() {
    r = 1.0f - r;
    g = 1.0f - g;
    b = 1.0f - b;
}

Color3 &Color3::Grayscale() {
    r = r * 0.299f;
    g = g * 0.587f;
    b = b * 0.114f;

    return *this;
}

Color3 Color3::FromRGB888(uint8_t r, uint8_t g, uint8_t b) {
    float invNorm = 1.0f / 255.0f;
    return Color3(r * invNorm, g * invNorm, b * invNorm);
}

Color3 Color3::FromUInt32(uint32_t rgbx) {
    const uint32_t r = Max<uint32_t>((rgbx >> 0) & 0xFF, 0);
    const uint32_t g = Max<uint32_t>((rgbx >> 8) & 0xFF, 0);
    const uint32_t b = Max<uint32_t>((rgbx >> 16) & 0xFF, 0);

    return Color3((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f);
}

uint32_t Color3::ToUInt32() const {
    uint32_t ur = (uint32_t)Clamp(((int)(r * 255.0f)), 0, 255);
    uint32_t ug = (uint32_t)Clamp(((int)(g * 255.0f)), 0, 255);
    uint32_t ub = (uint32_t)Clamp(((int)(b * 255.0f)), 0, 255);
    return (255 << 24) | (ub << 16) | (ug << 8) | ur;
}

Color3 Color3::FromString(const char *str) {
    Color3 v;
    sscanf(str, "%f %f %f", &v.r, &v.g, &v.b);
    return v;
}

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
Color3 Color3::SRGBToLinear() const {
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

Color3 Color3::FromColorTemperature(float temp) {
    Clamp(temp, 1000.0f, 15000.0f);

    float temp2 = temp * temp;

    // Approximate Planckian locus in CIE 1960 UCS
    float u = (0.860117757f + 1.54118254e-4f * temp + 1.28641212e-7f * temp2) / (1.0f + 8.42420235e-4f * temp + 7.08145163e-7f * temp2);
    float v = (0.317398726f + 4.22806245e-5f * temp + 4.20481691e-8f * temp2) / (1.0f - 2.89741816e-5f * temp + 1.61456053e-7f * temp2);

    float x = 3.0f * u / (2.0f * u - 8.0f * v + 4.0f);
    float y = 2.0f * v / (2.0f * u - 8.0f * v + 4.0f);
    float z = 1.0f - x - y;

    float Y = 1.0f;
    float X = Y / y * x;
    float Z = Y / y * z;

    // XYZ to RGB with BT.709 primaries
    float r = 3.2404542f * X + -1.5371385f * Y + -0.4985314f * Z;
    float g = -0.9692660f * X + 1.8760108f * Y + 0.0415560f * Z;
    float b = 0.0556434f * X + -0.2040259f * Y + 1.0572252f * Z;

    return Color3(r, g, b);
}

BE_NAMESPACE_END
