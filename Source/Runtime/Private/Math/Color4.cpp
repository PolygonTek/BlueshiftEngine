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

const Color4 Color4::zero                   = Color4(0.000000000f, 0.000000000f, 0.000000000f, 0.000000000f);
const Color4 Color4::aliceBlue              = Color4(0.941176534f, 0.972549081f, 1.000000000f, 1.000000000f);
const Color4 Color4::antiqueWhite           = Color4(0.980392218f, 0.921568692f, 0.843137324f, 1.000000000f);
const Color4 Color4::aqua                   = Color4(0.000000000f, 1.000000000f, 1.000000000f, 1.000000000f);
const Color4 Color4::aquamarine             = Color4(0.498039246f, 1.000000000f, 0.831372619f, 1.000000000f);
const Color4 Color4::azure                  = Color4(0.941176534f, 1.000000000f, 1.000000000f, 1.000000000f);
const Color4 Color4::beige                  = Color4(0.960784376f, 0.960784376f, 0.862745166f, 1.000000000f);
const Color4 Color4::bisque                 = Color4(1.000000000f, 0.894117713f, 0.768627524f, 1.000000000f);
const Color4 Color4::black                  = Color4(0.000000000f, 0.000000000f, 0.000000000f, 1.000000000f);
const Color4 Color4::blanchedAlmond         = Color4(1.000000000f, 0.921568692f, 0.803921640f, 1.000000000f);
const Color4 Color4::blue                   = Color4(0.000000000f, 0.000000000f, 1.000000000f, 1.000000000f);
const Color4 Color4::blueViolet             = Color4(0.541176498f, 0.168627456f, 0.886274576f, 1.000000000f);
const Color4 Color4::brown                  = Color4(0.647058845f, 0.164705887f, 0.164705887f, 1.000000000f);
const Color4 Color4::burlyWood              = Color4(0.870588303f, 0.721568644f, 0.529411793f, 1.000000000f);
const Color4 Color4::cadetBlue              = Color4(0.372549027f, 0.619607866f, 0.627451003f, 1.000000000f);
const Color4 Color4::chartreuse             = Color4(0.498039246f, 1.000000000f, 0.000000000f, 1.000000000f);
const Color4 Color4::chocolate              = Color4(0.823529482f, 0.411764741f, 0.117647067f, 1.000000000f);
const Color4 Color4::coral                  = Color4(1.000000000f, 0.498039246f, 0.313725501f, 1.000000000f);
const Color4 Color4::cornflowerBlue         = Color4(0.392156899f, 0.584313750f, 0.929411829f, 1.000000000f);
const Color4 Color4::cornsilk               = Color4(1.000000000f, 0.972549081f, 0.862745166f, 1.000000000f);
const Color4 Color4::crimson                = Color4(0.862745166f, 0.078431375f, 0.235294133f, 1.000000000f);
const Color4 Color4::cyan                   = Color4(0.000000000f, 1.000000000f, 1.000000000f, 1.000000000f);
const Color4 Color4::darkBlue               = Color4(0.000000000f, 0.000000000f, 0.545098066f, 1.000000000f);
const Color4 Color4::darkCyan               = Color4(0.000000000f, 0.545098066f, 0.545098066f, 1.000000000f);
const Color4 Color4::darkGoldenrod          = Color4(0.721568644f, 0.525490224f, 0.043137256f, 1.000000000f);
const Color4 Color4::darkGray               = Color4(0.662745118f, 0.662745118f, 0.662745118f, 1.000000000f);
const Color4 Color4::darkGreen              = Color4(0.000000000f, 0.392156899f, 0.000000000f, 1.000000000f);
const Color4 Color4::darkKhaki              = Color4(0.741176486f, 0.717647076f, 0.419607878f, 1.000000000f);
const Color4 Color4::darkMagenta            = Color4(0.545098066f, 0.000000000f, 0.545098066f, 1.000000000f);
const Color4 Color4::darkOliveGreen         = Color4(0.333333343f, 0.419607878f, 0.184313729f, 1.000000000f);
const Color4 Color4::darkOrange             = Color4(1.000000000f, 0.549019635f, 0.000000000f, 1.000000000f);
const Color4 Color4::darkOrchid             = Color4(0.600000024f, 0.196078449f, 0.800000072f, 1.000000000f);
const Color4 Color4::darkRed                = Color4(0.545098066f, 0.000000000f, 0.000000000f, 1.000000000f);
const Color4 Color4::darkSalmon             = Color4(0.913725555f, 0.588235319f, 0.478431404f, 1.000000000f);
const Color4 Color4::darkSeaGreen           = Color4(0.560784340f, 0.737254918f, 0.545098066f, 1.000000000f);
const Color4 Color4::darkSlateBlue          = Color4(0.282352954f, 0.239215702f, 0.545098066f, 1.000000000f);
const Color4 Color4::darkSlateGray          = Color4(0.184313729f, 0.309803933f, 0.309803933f, 1.000000000f);
const Color4 Color4::darkTurquoise          = Color4(0.000000000f, 0.807843208f, 0.819607913f, 1.000000000f);
const Color4 Color4::darkViolet             = Color4(0.580392182f, 0.000000000f, 0.827451050f, 1.000000000f);
const Color4 Color4::deepPink               = Color4(1.000000000f, 0.078431375f, 0.576470613f, 1.000000000f);
const Color4 Color4::deepSkyBlue            = Color4(0.000000000f, 0.749019623f, 1.000000000f, 1.000000000f);
const Color4 Color4::dimGray                = Color4(0.411764741f, 0.411764741f, 0.411764741f, 1.000000000f);
const Color4 Color4::dodgerBlue             = Color4(0.117647067f, 0.564705908f, 1.000000000f, 1.000000000f);
const Color4 Color4::firebrick              = Color4(0.698039234f, 0.133333340f, 0.133333340f, 1.000000000f);
const Color4 Color4::floralWhite            = Color4(1.000000000f, 0.980392218f, 0.941176534f, 1.000000000f);
const Color4 Color4::forestGreen            = Color4(0.133333340f, 0.545098066f, 0.133333340f, 1.000000000f);
const Color4 Color4::fuchsia                = Color4(1.000000000f, 0.000000000f, 1.000000000f, 1.000000000f);
const Color4 Color4::gainsboro              = Color4(0.862745166f, 0.862745166f, 0.862745166f, 1.000000000f);
const Color4 Color4::ghostWhite             = Color4(0.972549081f, 0.972549081f, 1.000000000f, 1.000000000f);
const Color4 Color4::gold                   = Color4(1.000000000f, 0.843137324f, 0.000000000f, 1.000000000f);
const Color4 Color4::goldenrod              = Color4(0.854902029f, 0.647058845f, 0.125490203f, 1.000000000f);
const Color4 Color4::gray                   = Color4(0.501960814f, 0.501960814f, 0.501960814f, 1.000000000f);
const Color4 Color4::green                  = Color4(0.000000000f, 0.501960814f, 0.000000000f, 1.000000000f);
const Color4 Color4::greenYellow            = Color4(0.678431392f, 1.000000000f, 0.184313729f, 1.000000000f);
const Color4 Color4::honeydew               = Color4(0.941176534f, 1.000000000f, 0.941176534f, 1.000000000f);
const Color4 Color4::hotPink                = Color4(1.000000000f, 0.411764741f, 0.705882370f, 1.000000000f);
const Color4 Color4::indianRed              = Color4(0.803921640f, 0.360784322f, 0.360784322f, 1.000000000f);
const Color4 Color4::indigo                 = Color4(0.294117659f, 0.000000000f, 0.509803951f, 1.000000000f);
const Color4 Color4::ivory                  = Color4(1.000000000f, 1.000000000f, 0.941176534f, 1.000000000f);
const Color4 Color4::khaki                  = Color4(0.941176534f, 0.901960850f, 0.549019635f, 1.000000000f);
const Color4 Color4::lavender               = Color4(0.901960850f, 0.901960850f, 0.980392218f, 1.000000000f);
const Color4 Color4::lavenderBlush          = Color4(1.000000000f, 0.941176534f, 0.960784376f, 1.000000000f);
const Color4 Color4::lawnGreen              = Color4(0.486274540f, 0.988235354f, 0.000000000f, 1.000000000f);
const Color4 Color4::lemonChiffon           = Color4(1.000000000f, 0.980392218f, 0.803921640f, 1.000000000f);
const Color4 Color4::lightBlue              = Color4(0.678431392f, 0.847058892f, 0.901960850f, 1.000000000f);
const Color4 Color4::lightCoral             = Color4(0.941176534f, 0.501960814f, 0.501960814f, 1.000000000f);
const Color4 Color4::lightCyan              = Color4(0.878431439f, 1.000000000f, 1.000000000f, 1.000000000f);
const Color4 Color4::lightGoldenrodYellow   = Color4(0.980392218f, 0.980392218f, 0.823529482f, 1.000000000f);
const Color4 Color4::lightGreen             = Color4(0.564705908f, 0.933333397f, 0.564705908f, 1.000000000f);
const Color4 Color4::lightGray              = Color4(0.827451050f, 0.827451050f, 0.827451050f, 1.000000000f);
const Color4 Color4::lightPink              = Color4(1.000000000f, 0.713725507f, 0.756862819f, 1.000000000f);
const Color4 Color4::lightSalmon            = Color4(1.000000000f, 0.627451003f, 0.478431404f, 1.000000000f);
const Color4 Color4::lightSeaGreen          = Color4(0.125490203f, 0.698039234f, 0.666666687f, 1.000000000f);
const Color4 Color4::lightSkyBlue           = Color4(0.529411793f, 0.807843208f, 0.980392218f, 1.000000000f);
const Color4 Color4::lightSlateGray         = Color4(0.466666698f, 0.533333361f, 0.600000024f, 1.000000000f);
const Color4 Color4::lightSteelBlue         = Color4(0.690196097f, 0.768627524f, 0.870588303f, 1.000000000f);
const Color4 Color4::lightYellow            = Color4(1.000000000f, 1.000000000f, 0.878431439f, 1.000000000f);
const Color4 Color4::lime                   = Color4(0.000000000f, 1.000000000f, 0.000000000f, 1.000000000f);
const Color4 Color4::limeGreen              = Color4(0.196078449f, 0.803921640f, 0.196078449f, 1.000000000f);
const Color4 Color4::linen                  = Color4(0.980392218f, 0.941176534f, 0.901960850f, 1.000000000f);
const Color4 Color4::magenta                = Color4(1.000000000f, 0.000000000f, 1.000000000f, 1.000000000f);
const Color4 Color4::maroon                 = Color4(0.501960814f, 0.000000000f, 0.000000000f, 1.000000000f);
const Color4 Color4::mediumAquamarine       = Color4(0.400000036f, 0.803921640f, 0.666666687f, 1.000000000f);
const Color4 Color4::mediumBlue             = Color4(0.000000000f, 0.000000000f, 0.803921640f, 1.000000000f);
const Color4 Color4::mediumOrchid           = Color4(0.729411781f, 0.333333343f, 0.827451050f, 1.000000000f);
const Color4 Color4::mediumPurple           = Color4(0.576470613f, 0.439215720f, 0.858823597f, 1.000000000f);
const Color4 Color4::mediumSeaGreen         = Color4(0.235294133f, 0.701960802f, 0.443137288f, 1.000000000f);
const Color4 Color4::mediumSlateBlue        = Color4(0.482352972f, 0.407843173f, 0.933333397f, 1.000000000f);
const Color4 Color4::mediumSpringGreen      = Color4(0.000000000f, 0.980392218f, 0.603921592f, 1.000000000f);
const Color4 Color4::mediumTurquoise        = Color4(0.282352954f, 0.819607913f, 0.800000072f, 1.000000000f);
const Color4 Color4::mediumVioletRed        = Color4(0.780392230f, 0.082352944f, 0.521568656f, 1.000000000f);
const Color4 Color4::midnightBlue           = Color4(0.098039225f, 0.098039225f, 0.439215720f, 1.000000000f);
const Color4 Color4::mintCream              = Color4(0.960784376f, 1.000000000f, 0.980392218f, 1.000000000f);
const Color4 Color4::mistyRose              = Color4(1.000000000f, 0.894117713f, 0.882353008f, 1.000000000f);
const Color4 Color4::moccasin               = Color4(1.000000000f, 0.894117713f, 0.709803939f, 1.000000000f);
const Color4 Color4::navajoWhite            = Color4(1.000000000f, 0.870588303f, 0.678431392f, 1.000000000f);
const Color4 Color4::navy                   = Color4(0.000000000f, 0.000000000f, 0.501960814f, 1.000000000f);
const Color4 Color4::oldLace                = Color4(0.992156923f, 0.960784376f, 0.901960850f, 1.000000000f);
const Color4 Color4::olive                  = Color4(0.501960814f, 0.501960814f, 0.000000000f, 1.000000000f);
const Color4 Color4::oliveDrab              = Color4(0.419607878f, 0.556862772f, 0.137254909f, 1.000000000f);
const Color4 Color4::orange                 = Color4(1.000000000f, 0.647058845f, 0.000000000f, 1.000000000f);
const Color4 Color4::orangeRed              = Color4(1.000000000f, 0.270588249f, 0.000000000f, 1.000000000f);
const Color4 Color4::orchid                 = Color4(0.854902029f, 0.439215720f, 0.839215755f, 1.000000000f);
const Color4 Color4::paleGoldenrod          = Color4(0.933333397f, 0.909803987f, 0.666666687f, 1.000000000f);
const Color4 Color4::paleGreen              = Color4(0.596078455f, 0.984313786f, 0.596078455f, 1.000000000f);
const Color4 Color4::paleTurquoise          = Color4(0.686274529f, 0.933333397f, 0.933333397f, 1.000000000f);
const Color4 Color4::paleVioletRed          = Color4(0.858823597f, 0.439215720f, 0.576470613f, 1.000000000f);
const Color4 Color4::papayaWhip             = Color4(1.000000000f, 0.937254965f, 0.835294187f, 1.000000000f);
const Color4 Color4::peachPuff              = Color4(1.000000000f, 0.854902029f, 0.725490212f, 1.000000000f);
const Color4 Color4::peru                   = Color4(0.803921640f, 0.521568656f, 0.247058839f, 1.000000000f);
const Color4 Color4::pink                   = Color4(1.000000000f, 0.752941251f, 0.796078503f, 1.000000000f);
const Color4 Color4::plum                   = Color4(0.866666734f, 0.627451003f, 0.866666734f, 1.000000000f);
const Color4 Color4::powderBlue             = Color4(0.690196097f, 0.878431439f, 0.901960850f, 1.000000000f);
const Color4 Color4::purple                 = Color4(0.501960814f, 0.000000000f, 0.501960814f, 1.000000000f);
const Color4 Color4::red                    = Color4(1.000000000f, 0.000000000f, 0.000000000f, 1.000000000f);
const Color4 Color4::rosyBrown              = Color4(0.737254918f, 0.560784340f, 0.560784340f, 1.000000000f);
const Color4 Color4::royalBlue              = Color4(0.254901975f, 0.411764741f, 0.882353008f, 1.000000000f);
const Color4 Color4::saddleBrown            = Color4(0.545098066f, 0.270588249f, 0.074509807f, 1.000000000f);
const Color4 Color4::salmon                 = Color4(0.980392218f, 0.501960814f, 0.447058856f, 1.000000000f);
const Color4 Color4::sandyBrown             = Color4(0.956862807f, 0.643137276f, 0.376470625f, 1.000000000f);
const Color4 Color4::seaGreen               = Color4(0.180392161f, 0.545098066f, 0.341176480f, 1.000000000f);
const Color4 Color4::seaShell               = Color4(1.000000000f, 0.960784376f, 0.933333397f, 1.000000000f);
const Color4 Color4::sienna                 = Color4(0.627451003f, 0.321568638f, 0.176470593f, 1.000000000f);
const Color4 Color4::silver                 = Color4(0.752941251f, 0.752941251f, 0.752941251f, 1.000000000f);
const Color4 Color4::skyBlue                = Color4(0.529411793f, 0.807843208f, 0.921568692f, 1.000000000f);
const Color4 Color4::slateBlue              = Color4(0.415686309f, 0.352941185f, 0.803921640f, 1.000000000f);
const Color4 Color4::slateGray              = Color4(0.439215720f, 0.501960814f, 0.564705908f, 1.000000000f);
const Color4 Color4::snow                   = Color4(1.000000000f, 0.980392218f, 0.980392218f, 1.000000000f);
const Color4 Color4::springGreen            = Color4(0.000000000f, 1.000000000f, 0.498039246f, 1.000000000f);
const Color4 Color4::steelBlue              = Color4(0.274509817f, 0.509803951f, 0.705882370f, 1.000000000f);
const Color4 Color4::tan                    = Color4(0.823529482f, 0.705882370f, 0.549019635f, 1.000000000f);
const Color4 Color4::teal                   = Color4(0.000000000f, 0.501960814f, 0.501960814f, 1.000000000f);
const Color4 Color4::thistle                = Color4(0.847058892f, 0.749019623f, 0.847058892f, 1.000000000f);
const Color4 Color4::tomato                 = Color4(1.000000000f, 0.388235331f, 0.278431386f, 1.000000000f);
const Color4 Color4::turquoise              = Color4(0.250980407f, 0.878431439f, 0.815686345f, 1.000000000f);
const Color4 Color4::violet                 = Color4(0.933333397f, 0.509803951f, 0.933333397f, 1.000000000f);
const Color4 Color4::wheat                  = Color4(0.960784376f, 0.870588303f, 0.701960802f, 1.000000000f);
const Color4 Color4::white                  = Color4(1.000000000f, 1.000000000f, 1.000000000f, 1.000000000f);
const Color4 Color4::whiteSmoke             = Color4(0.960784376f, 0.960784376f, 0.960784376f, 1.000000000f);
const Color4 Color4::yellow                 = Color4(1.000000000f, 1.000000000f, 0.000000000f, 1.000000000f);
const Color4 Color4::yellowGreen            = Color4(0.603921592f, 0.803921640f, 0.196078449f, 1.000000000f);

void Color4::Clip(bool clipAlpha) {
    Clamp01(r);
    Clamp01(g);
    Clamp01(b);

    if (clipAlpha) {
        Clamp01(a);
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

Color4 &Color4::Grayscale() {
    r = r * 0.299f;
    g = g * 0.587f;
    b = b * 0.114f;

    return *this;
}

Color4 Color4::FromRGBA8888(uint8_t ur, uint8_t ug, uint8_t ub, uint8_t ua) {
    constexpr float invNorm = 1.0f / 255.0f;
    return Color4(ur * invNorm, ug * invNorm, ub * invNorm, ua * invNorm);
}

Color4 Color4::FromUInt32(uint32_t rgba) {
    constexpr float invNorm = 1.0f / 255.0f;
    uint32_t ur = Max<uint32_t>((rgba >> 0) & 0xFF, 0);
    uint32_t ug = Max<uint32_t>((rgba >> 8) & 0xFF, 0);
    uint32_t ub = Max<uint32_t>((rgba >> 16) & 0xFF, 0);
    uint32_t ua = Max<uint32_t>((rgba >> 24) & 0xFF, 0);
    return Color4(ur * invNorm, ug * invNorm, ub * invNorm, ua * invNorm);
}

uint32_t Color4::ToUInt32() const {
    uint32_t ur = (uint32_t)Clamp(((int)(r * 255.0f)), 0, 255);
    uint32_t ug = (uint32_t)Clamp(((int)(g * 255.0f)), 0, 255);
    uint32_t ub = (uint32_t)Clamp(((int)(b * 255.0f)), 0, 255);
    uint32_t ua = (uint32_t)Clamp(((int)(a * 255.0f)), 0, 255);
    return (ua << 24) | (ub << 16) | (ug << 8) | ur;
}

Color4 Color4::FromString(const char *str) {
    Color4 v;
    int count = sscanf(str, "%f %f %f %f", &v.r, &v.g, &v.b, &v.a);
    assert(count == GetDimension());
    return v;
}

BE_NAMESPACE_END
