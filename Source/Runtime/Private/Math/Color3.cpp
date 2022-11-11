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

const Color3 Color3::zero                   = Color3(0.000000000f, 0.000000000f, 0.000000000f);
const Color3 Color3::aliceBlue              = Color3(0.941176534f, 0.972549081f, 1.000000000f);
const Color3 Color3::antiqueWhite           = Color3(0.980392218f, 0.921568692f, 0.843137324f);
const Color3 Color3::aqua                   = Color3(0.000000000f, 1.000000000f, 1.000000000f);
const Color3 Color3::aquamarine             = Color3(0.498039246f, 1.000000000f, 0.831372619f);
const Color3 Color3::azure                  = Color3(0.941176534f, 1.000000000f, 1.000000000f);
const Color3 Color3::beige                  = Color3(0.960784376f, 0.960784376f, 0.862745166f);
const Color3 Color3::bisque                 = Color3(1.000000000f, 0.894117713f, 0.768627524f);
const Color3 Color3::black                  = Color3(0.000000000f, 0.000000000f, 0.000000000f);
const Color3 Color3::blanchedAlmond         = Color3(1.000000000f, 0.921568692f, 0.803921640f);
const Color3 Color3::blue                   = Color3(0.000000000f, 0.000000000f, 1.000000000f);
const Color3 Color3::blueViolet             = Color3(0.541176498f, 0.168627456f, 0.886274576f);
const Color3 Color3::brown                  = Color3(0.647058845f, 0.164705887f, 0.164705887f);
const Color3 Color3::burlyWood              = Color3(0.870588303f, 0.721568644f, 0.529411793f);
const Color3 Color3::cadetBlue              = Color3(0.372549027f, 0.619607866f, 0.627451003f);
const Color3 Color3::chartreuse             = Color3(0.498039246f, 1.000000000f, 0.000000000f);
const Color3 Color3::chocolate              = Color3(0.823529482f, 0.411764741f, 0.117647067f);
const Color3 Color3::coral                  = Color3(1.000000000f, 0.498039246f, 0.313725501f);
const Color3 Color3::cornflowerBlue         = Color3(0.392156899f, 0.584313750f, 0.929411829f);
const Color3 Color3::cornsilk               = Color3(1.000000000f, 0.972549081f, 0.862745166f);
const Color3 Color3::crimson                = Color3(0.862745166f, 0.078431375f, 0.235294133f);
const Color3 Color3::cyan                   = Color3(0.000000000f, 1.000000000f, 1.000000000f);
const Color3 Color3::darkBlue               = Color3(0.000000000f, 0.000000000f, 0.545098066f);
const Color3 Color3::darkCyan               = Color3(0.000000000f, 0.545098066f, 0.545098066f);
const Color3 Color3::darkGoldenrod          = Color3(0.721568644f, 0.525490224f, 0.043137256f);
const Color3 Color3::darkGray               = Color3(0.662745118f, 0.662745118f, 0.662745118f);
const Color3 Color3::darkGreen              = Color3(0.000000000f, 0.392156899f, 0.000000000f);
const Color3 Color3::darkKhaki              = Color3(0.741176486f, 0.717647076f, 0.419607878f);
const Color3 Color3::darkMagenta            = Color3(0.545098066f, 0.000000000f, 0.545098066f);
const Color3 Color3::darkOliveGreen         = Color3(0.333333343f, 0.419607878f, 0.184313729f);
const Color3 Color3::darkOrange             = Color3(1.000000000f, 0.549019635f, 0.000000000f);
const Color3 Color3::darkOrchid             = Color3(0.600000024f, 0.196078449f, 0.800000072f);
const Color3 Color3::darkRed                = Color3(0.545098066f, 0.000000000f, 0.000000000f);
const Color3 Color3::darkSalmon             = Color3(0.913725555f, 0.588235319f, 0.478431404f);
const Color3 Color3::darkSeaGreen           = Color3(0.560784340f, 0.737254918f, 0.545098066f);
const Color3 Color3::darkSlateBlue          = Color3(0.282352954f, 0.239215702f, 0.545098066f);
const Color3 Color3::darkSlateGray          = Color3(0.184313729f, 0.309803933f, 0.309803933f);
const Color3 Color3::darkTurquoise          = Color3(0.000000000f, 0.807843208f, 0.819607913f);
const Color3 Color3::darkViolet             = Color3(0.580392182f, 0.000000000f, 0.827451050f);
const Color3 Color3::deepPink               = Color3(1.000000000f, 0.078431375f, 0.576470613f);
const Color3 Color3::deepSkyBlue            = Color3(0.000000000f, 0.749019623f, 1.000000000f);
const Color3 Color3::dimGray                = Color3(0.411764741f, 0.411764741f, 0.411764741f);
const Color3 Color3::dodgerBlue             = Color3(0.117647067f, 0.564705908f, 1.000000000f);
const Color3 Color3::firebrick              = Color3(0.698039234f, 0.133333340f, 0.133333340f);
const Color3 Color3::floralWhite            = Color3(1.000000000f, 0.980392218f, 0.941176534f);
const Color3 Color3::forestGreen            = Color3(0.133333340f, 0.545098066f, 0.133333340f);
const Color3 Color3::fuchsia                = Color3(1.000000000f, 0.000000000f, 1.000000000f);
const Color3 Color3::gainsboro              = Color3(0.862745166f, 0.862745166f, 0.862745166f);
const Color3 Color3::ghostWhite             = Color3(0.972549081f, 0.972549081f, 1.000000000f);
const Color3 Color3::gold                   = Color3(1.000000000f, 0.843137324f, 0.000000000f);
const Color3 Color3::goldenrod              = Color3(0.854902029f, 0.647058845f, 0.125490203f);
const Color3 Color3::gray                   = Color3(0.501960814f, 0.501960814f, 0.501960814f);
const Color3 Color3::green                  = Color3(0.000000000f, 0.501960814f, 0.000000000f);
const Color3 Color3::greenYellow            = Color3(0.678431392f, 1.000000000f, 0.184313729f);
const Color3 Color3::honeydew               = Color3(0.941176534f, 1.000000000f, 0.941176534f);
const Color3 Color3::hotPink                = Color3(1.000000000f, 0.411764741f, 0.705882370f);
const Color3 Color3::indianRed              = Color3(0.803921640f, 0.360784322f, 0.360784322f);
const Color3 Color3::indigo                 = Color3(0.294117659f, 0.000000000f, 0.509803951f);
const Color3 Color3::ivory                  = Color3(1.000000000f, 1.000000000f, 0.941176534f);
const Color3 Color3::khaki                  = Color3(0.941176534f, 0.901960850f, 0.549019635f);
const Color3 Color3::lavender               = Color3(0.901960850f, 0.901960850f, 0.980392218f);
const Color3 Color3::lavenderBlush          = Color3(1.000000000f, 0.941176534f, 0.960784376f);
const Color3 Color3::lawnGreen              = Color3(0.486274540f, 0.988235354f, 0.000000000f);
const Color3 Color3::lemonChiffon           = Color3(1.000000000f, 0.980392218f, 0.803921640f);
const Color3 Color3::lightBlue              = Color3(0.678431392f, 0.847058892f, 0.901960850f);
const Color3 Color3::lightCoral             = Color3(0.941176534f, 0.501960814f, 0.501960814f);
const Color3 Color3::lightCyan              = Color3(0.878431439f, 1.000000000f, 1.000000000f);
const Color3 Color3::lightGoldenrodYellow   = Color3(0.980392218f, 0.980392218f, 0.823529482f);
const Color3 Color3::lightGreen             = Color3(0.564705908f, 0.933333397f, 0.564705908f);
const Color3 Color3::lightGray              = Color3(0.827451050f, 0.827451050f, 0.827451050f);
const Color3 Color3::lightPink              = Color3(1.000000000f, 0.713725507f, 0.756862819f);
const Color3 Color3::lightSalmon            = Color3(1.000000000f, 0.627451003f, 0.478431404f);
const Color3 Color3::lightSeaGreen          = Color3(0.125490203f, 0.698039234f, 0.666666687f);
const Color3 Color3::lightSkyBlue           = Color3(0.529411793f, 0.807843208f, 0.980392218f);
const Color3 Color3::lightSlateGray         = Color3(0.466666698f, 0.533333361f, 0.600000024f);
const Color3 Color3::lightSteelBlue         = Color3(0.690196097f, 0.768627524f, 0.870588303f);
const Color3 Color3::lightYellow            = Color3(1.000000000f, 1.000000000f, 0.878431439f);
const Color3 Color3::lime                   = Color3(0.000000000f, 1.000000000f, 0.000000000f);
const Color3 Color3::limeGreen              = Color3(0.196078449f, 0.803921640f, 0.196078449f);
const Color3 Color3::linen                  = Color3(0.980392218f, 0.941176534f, 0.901960850f);
const Color3 Color3::magenta                = Color3(1.000000000f, 0.000000000f, 1.000000000f);
const Color3 Color3::maroon                 = Color3(0.501960814f, 0.000000000f, 0.000000000f);
const Color3 Color3::mediumAquamarine       = Color3(0.400000036f, 0.803921640f, 0.666666687f);
const Color3 Color3::mediumBlue             = Color3(0.000000000f, 0.000000000f, 0.803921640f);
const Color3 Color3::mediumOrchid           = Color3(0.729411781f, 0.333333343f, 0.827451050f);
const Color3 Color3::mediumPurple           = Color3(0.576470613f, 0.439215720f, 0.858823597f);
const Color3 Color3::mediumSeaGreen         = Color3(0.235294133f, 0.701960802f, 0.443137288f);
const Color3 Color3::mediumSlateBlue        = Color3(0.482352972f, 0.407843173f, 0.933333397f);
const Color3 Color3::mediumSpringGreen      = Color3(0.000000000f, 0.980392218f, 0.603921592f);
const Color3 Color3::mediumTurquoise        = Color3(0.282352954f, 0.819607913f, 0.800000072f);
const Color3 Color3::mediumVioletRed        = Color3(0.780392230f, 0.082352944f, 0.521568656f);
const Color3 Color3::midnightBlue           = Color3(0.098039225f, 0.098039225f, 0.439215720f);
const Color3 Color3::mintCream              = Color3(0.960784376f, 1.000000000f, 0.980392218f);
const Color3 Color3::mistyRose              = Color3(1.000000000f, 0.894117713f, 0.882353008f);
const Color3 Color3::moccasin               = Color3(1.000000000f, 0.894117713f, 0.709803939f);
const Color3 Color3::navajoWhite            = Color3(1.000000000f, 0.870588303f, 0.678431392f);
const Color3 Color3::navy                   = Color3(0.000000000f, 0.000000000f, 0.501960814f);
const Color3 Color3::oldLace                = Color3(0.992156923f, 0.960784376f, 0.901960850f);
const Color3 Color3::olive                  = Color3(0.501960814f, 0.501960814f, 0.000000000f);
const Color3 Color3::oliveDrab              = Color3(0.419607878f, 0.556862772f, 0.137254909f);
const Color3 Color3::orange                 = Color3(1.000000000f, 0.647058845f, 0.000000000f);
const Color3 Color3::orangeRed              = Color3(1.000000000f, 0.270588249f, 0.000000000f);
const Color3 Color3::orchid                 = Color3(0.854902029f, 0.439215720f, 0.839215755f);
const Color3 Color3::paleGoldenrod          = Color3(0.933333397f, 0.909803987f, 0.666666687f);
const Color3 Color3::paleGreen              = Color3(0.596078455f, 0.984313786f, 0.596078455f);
const Color3 Color3::paleTurquoise          = Color3(0.686274529f, 0.933333397f, 0.933333397f);
const Color3 Color3::paleVioletRed          = Color3(0.858823597f, 0.439215720f, 0.576470613f);
const Color3 Color3::papayaWhip             = Color3(1.000000000f, 0.937254965f, 0.835294187f);
const Color3 Color3::peachPuff              = Color3(1.000000000f, 0.854902029f, 0.725490212f);
const Color3 Color3::peru                   = Color3(0.803921640f, 0.521568656f, 0.247058839f);
const Color3 Color3::pink                   = Color3(1.000000000f, 0.752941251f, 0.796078503f);
const Color3 Color3::plum                   = Color3(0.866666734f, 0.627451003f, 0.866666734f);
const Color3 Color3::powderBlue             = Color3(0.690196097f, 0.878431439f, 0.901960850f);
const Color3 Color3::purple                 = Color3(0.501960814f, 0.000000000f, 0.501960814f);
const Color3 Color3::red                    = Color3(1.000000000f, 0.000000000f, 0.000000000f);
const Color3 Color3::rosyBrown              = Color3(0.737254918f, 0.560784340f, 0.560784340f);
const Color3 Color3::royalBlue              = Color3(0.254901975f, 0.411764741f, 0.882353008f);
const Color3 Color3::saddleBrown            = Color3(0.545098066f, 0.270588249f, 0.074509807f);
const Color3 Color3::salmon                 = Color3(0.980392218f, 0.501960814f, 0.447058856f);
const Color3 Color3::sandyBrown             = Color3(0.956862807f, 0.643137276f, 0.376470625f);
const Color3 Color3::seaGreen               = Color3(0.180392161f, 0.545098066f, 0.341176480f);
const Color3 Color3::seaShell               = Color3(1.000000000f, 0.960784376f, 0.933333397f);
const Color3 Color3::sienna                 = Color3(0.627451003f, 0.321568638f, 0.176470593f);
const Color3 Color3::silver                 = Color3(0.752941251f, 0.752941251f, 0.752941251f);
const Color3 Color3::skyBlue                = Color3(0.529411793f, 0.807843208f, 0.921568692f);
const Color3 Color3::slateBlue              = Color3(0.415686309f, 0.352941185f, 0.803921640f);
const Color3 Color3::slateGray              = Color3(0.439215720f, 0.501960814f, 0.564705908f);
const Color3 Color3::snow                   = Color3(1.000000000f, 0.980392218f, 0.980392218f);
const Color3 Color3::springGreen            = Color3(0.000000000f, 1.000000000f, 0.498039246f);
const Color3 Color3::steelBlue              = Color3(0.274509817f, 0.509803951f, 0.705882370f);
const Color3 Color3::tan                    = Color3(0.823529482f, 0.705882370f, 0.549019635f);
const Color3 Color3::teal                   = Color3(0.000000000f, 0.501960814f, 0.501960814f);
const Color3 Color3::thistle                = Color3(0.847058892f, 0.749019623f, 0.847058892f);
const Color3 Color3::tomato                 = Color3(1.000000000f, 0.388235331f, 0.278431386f);
const Color3 Color3::turquoise              = Color3(0.250980407f, 0.878431439f, 0.815686345f);
const Color3 Color3::violet                 = Color3(0.933333397f, 0.509803951f, 0.933333397f);
const Color3 Color3::wheat                  = Color3(0.960784376f, 0.870588303f, 0.701960802f);
const Color3 Color3::white                  = Color3(1.000000000f, 1.000000000f, 1.000000000f);
const Color3 Color3::whiteSmoke             = Color3(0.960784376f, 0.960784376f, 0.960784376f);
const Color3 Color3::yellow                 = Color3(1.000000000f, 1.000000000f, 0.000000000f);
const Color3 Color3::yellowGreen            = Color3(0.603921592f, 0.803921640f, 0.196078449f);

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

Color3 Color3::FromRGB888(uint8_t ur, uint8_t ug, uint8_t ub) {
    constexpr float invNorm = 1.0f / 255.0f;
    return Color3(ur * invNorm, ug * invNorm, ub * invNorm);
}

Color3 Color3::FromUInt32(uint32_t rgbx) {
    constexpr float invNorm = 1.0f / 255.0f;
    uint32_t ur = Max<uint32_t>((rgbx >> 0) & 0xFF, 0);
    uint32_t ug = Max<uint32_t>((rgbx >> 8) & 0xFF, 0);
    uint32_t ub = Max<uint32_t>((rgbx >> 16) & 0xFF, 0);
    return Color3(ur * invNorm, ug * invNorm, ub * invNorm);
}

uint32_t Color3::ToUInt32() const {
    uint32_t ur = (uint32_t)Clamp(((int)(r * 255.0f)), 0, 255);
    uint32_t ug = (uint32_t)Clamp(((int)(g * 255.0f)), 0, 255);
    uint32_t ub = (uint32_t)Clamp(((int)(b * 255.0f)), 0, 255);
    return (255 << 24) | (ub << 16) | (ug << 8) | ur;
}

Color3 Color3::FromString(const char *str) {
    Color3 v;
    int count = sscanf(str, "%f %f %f", &v.r, &v.g, &v.b);
    assert(count == v.GetDimension());
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

Color3 Color3::FromHSL(float h, float s, float l) {
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
