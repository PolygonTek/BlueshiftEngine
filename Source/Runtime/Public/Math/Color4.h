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

#pragma once

BE_NAMESPACE_BEGIN

/// RGBA color.
class Color4 {
public:
    /// Specifies the number of elements in this class.
    enum { Size = 4 };

    /// The default constructor does not initialize any members of this class.
    Color4() = default;
    /// Constructs a Color4 with the value (r, g, b, a).
    constexpr Color4(float r, float g, float b, float a);
    /// Constructs a Color4 with the value (rgb.r, rgb.g, rgb.b, a).
    constexpr Color4(const Color3 &rgb, float a);
    /// Constructs a Color4 from a C array, to the value (data[0], data[1], data[2], data[3]).
    explicit constexpr Color4(const float data[4]);
    /// Constructs a Color4 from a single value (s, s, s, s)
    explicit constexpr Color4(float s);

#ifdef QCOLOR_H
    /// Constructs from a QColor.
    Color4(const QColor &qc) { r = qc.red(); g = qc.green(); b = qc.blue(); a = qc.alpha(); }
#endif
    
                        /// Casts this Color4 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&r; }
    float *             Ptr() { return (float *)&r; }
                        /// Casts this Color4 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&r; }
                        operator float *() { return (float *)&r; }

                        /// Accesses an element of this color.
    float &             At(int index) { return (*this)[index]; }
                        /// Accesses an element of this color using array notation.
    float               operator[](int index) const;
    float &             operator[](int index);

                        /// Adds a color to this color.
    Color4              operator+(const Color4 &rhs) const & { return Color4(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a); }
    Color4 &&           operator+(const Color4 &rhs) && { *this += rhs; return std::move(*this); }
                        /// Adds the color (s, s, s, s) to this color.
    Color4              operator+(float rhs) const & { return Color4(r + rhs, g + rhs, b + rhs, a + rhs); }
    Color4 &&           operator+(float rhs) && { *this += rhs; return std::move(*this); }
                        /// Adds the color v to color (s, s, s, s).
    friend Color4       operator+(float lhs, const Color4 &rhs) { return Color4(lhs + rhs.r, lhs + rhs.g, lhs + rhs.b, lhs + rhs.a); }
    friend Color4 &&    operator+(float lhs, Color4 &&rhs) { rhs += lhs; return std::move(rhs); }

                        /// Subtracts the given color from this color.
    Color4              operator-(const Color4 &rhs) const & { return Color4(r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a); }
    Color4 &&           operator-(const Color4 &rhs) && { *this -= rhs; return std::move(*this); }
                        /// Subtracts the color (s, s, s, s) from this color.
    Color4              operator-(float rhs) const & { return Color4(r - rhs, g - rhs, b - rhs, a - rhs); }
    Color4 &&           operator-(float rhs) && { *this -= rhs; return std::move(*this); }
                        /// Subtracts the color v from color (s, s, s, s).
    friend Color4       operator-(float lhs, const Color4 &rhs) { return Color4(lhs - rhs.r, lhs - rhs.g, lhs - rhs.b, lhs - rhs.a); }
    friend Color4 &&    operator-(float lhs, Color4 &&rhs) { rhs -= lhs; return std::move(rhs); }

                        /// Multiplies this color by a scalar.
    Color4              operator*(float rhs) const & { return Color4(r * rhs, g * rhs, b * rhs, a * rhs); }
    Color4 &&           operator*(float rhs) && { *this *= rhs; return std::move(*this); }
                        /// Multiplies color v by a scalar.
    friend Color4       operator*(float lhs, const Color4 &rhs) { return Color4(lhs * rhs.r, lhs * rhs.g, lhs * rhs.b, lhs * rhs.a); }
    friend Color4 &&    operator*(float lhs, Color4 &&rhs) { rhs *= lhs; return std::move(rhs); }
                        /// Multiplies this color by a color, element-wise.
    Color4              operator*(const Color4 &rhs) const & { return Color4(r * rhs.r, g * rhs.g, b * rhs.b, a * rhs.a); }
    Color4 &&           operator*(const Color4 &rhs) && { *this *= rhs; return std::move(*this); }

                        /// Divides this color by a scalar.
    Color4              operator/(float rhs) const & { float inv = 1.f / rhs; return Color4(r * inv, g * inv, b * inv, a * inv); }
    Color4 &&           operator/(float rhs) && { *this /= rhs; return std::move(*this); }
                        /// Divides this color by a color, element-wise.
    Color4              operator/(const Color4 &rhs) const & { return Color4(r / rhs.r, g / rhs.g, b / rhs.b, a / rhs.a); }
    Color4 &&           operator/(const Color4 &rhs) && { *this /= rhs; return std::move(*this); }
                        /// Divides color (s, s, s, s) by a color, element-wise.
    friend Color4       operator/(float lhs, const Color4 &rhs) { return Color4(lhs / rhs.r, lhs / rhs.g, lhs / rhs.b, lhs / rhs.a); }
    friend Color4 &&    operator/(float lhs, Color4 &&rhs) { rhs /= lhs; return std::move(rhs); }

                        /// Assign from another color.
    Color4 &            operator=(const Color4 &rhs);

                        /// Adds a color to this color, in-place.
    Color4 &            operator+=(const Color4 &rhs);
    Color4 &            operator+=(float rhs);

                        /// Subtracts a color from this color, in-place.
    Color4 &            operator-=(const Color4 &rhs);
    Color4 &            operator-=(float rhs);

                        /// Multiplies this color by a scalar, in-place.
    Color4 &            operator*=(float rhs);

                        /// Multiplies this color by a color, element-wise, in-place.
    Color4 &            operator*=(const Color4 &rhs);

                        /// Divides this color by a scalar, in-place.
    Color4 &            operator/=(float rhs);

                        /// Divides this color by a color, element-wise, in-place.
    Color4 &            operator/=(const Color4 &rhs);

                        /// Exact compare, no epsilon.
    bool                Equals(const Color4 &c) const;
                        /// Compare with epsilon.
    bool                Equals(const Color4 &c, const float epsilon) const;
                        /// Exact compare, no epsilon.
    bool                operator==(const Color4 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const Color4 &rhs) const { return !Equals(rhs); }

                        /// Sets all element of this color.
    void                Set(float r, float g, float b, float a);

                        /// Clip to [0, 1] range.
    void                Clip(bool clipAlpha = false);
                        /// Inverts the RGB channels and optionally the alpha channel as well.
    void                Invert(bool invertAlpha = false);
                        /// Grayscale this color, in-place.
    Color4 &            Grayscale();

                        /// Returns "r g b a".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "r g b a" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static Color4       FromString(const char *str);

                        /// Casts this Color4 to a Color3.
    const Color3 &      ToColor3() const;
    Color3 &            ToColor3();

                        /// Casts this Color4 to a Vec3.
    const Vec3 &        ToVec3() const;
    Vec3 &              ToVec3();

                        /// Casts this Color4 to a Vec4.
    const Vec4 &        ToVec4() const;
    Vec4 &              ToVec4();

                        /// Creates from RGBA8888.
    static Color4       FromRGBA8888(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

                        /// Creates from packed to a 32-bit integer, with R component in the lowest 8 bits.
    static Color4       FromUInt32(uint32_t rgba);

                        /// Convert color packed to a 32-bit integer, with R component in the lowest 8 bits. Components are clamped to [0, 1] range.
    uint32_t            ToUInt32() const;

#ifdef QCOLOR_H
                        /// Convert Color4 to QColor.
    QColor              ToQColor() const { return QColor::fromRgbF(r, g, b, a); }
#endif

                        /// Convert RGB to HSL (Hue, Saturation, Lightness).
    Color4              ToHSL() const { return Color4(ToColor3().ToHSL(), a); }
                        /// Convert HSL (Hue, Saturation, Lightness) to RGB.
    static Color4       FromHSL(float h, float s, float l, float a) { return Color4(Color3::FromHSL(h, s, l), a); }

                        /// Convert sRGB to Linear.
    Color4              SRGBToLinear() const { return Color4(ToColor3().SRGBToLinear(), a); }
                        /// Convert Linear to sRGB.
    Color4              LinearToSRGB() const { return Color4(ToColor3().LinearToSRGB(), a); }

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return Size; }

    static const Color4 zero;

    static const Color4 aliceBlue;
    static const Color4 antiqueWhite;
    static const Color4 aqua;
    static const Color4 aquamarine;
    static const Color4 azure;
    static const Color4 beige;
    static const Color4 bisque;
    static const Color4 black;
    static const Color4 blanchedAlmond;
    static const Color4 blue;
    static const Color4 blueViolet;
    static const Color4 brown;
    static const Color4 burlyWood;
    static const Color4 cadetBlue;
    static const Color4 chartreuse;
    static const Color4 chocolate;
    static const Color4 coral;
    static const Color4 cornflowerBlue;
    static const Color4 cornsilk;
    static const Color4 crimson;
    static const Color4 cyan;
    static const Color4 darkBlue;
    static const Color4 darkCyan;
    static const Color4 darkGoldenrod;
    static const Color4 darkGray;
    static const Color4 darkGreen;
    static const Color4 darkKhaki;
    static const Color4 darkMagenta;
    static const Color4 darkOliveGreen;
    static const Color4 darkOrange;
    static const Color4 darkOrchid;
    static const Color4 darkRed;
    static const Color4 darkSalmon;
    static const Color4 darkSeaGreen;
    static const Color4 darkSlateBlue;
    static const Color4 darkSlateGray;
    static const Color4 darkTurquoise;
    static const Color4 darkViolet;
    static const Color4 deepPink;
    static const Color4 deepSkyBlue;
    static const Color4 dimGray;
    static const Color4 dodgerBlue;
    static const Color4 firebrick;
    static const Color4 floralWhite;
    static const Color4 forestGreen;
    static const Color4 fuchsia;
    static const Color4 gainsboro;
    static const Color4 ghostWhite;
    static const Color4 gold;
    static const Color4 goldenrod;
    static const Color4 gray;
    static const Color4 green;
    static const Color4 greenYellow;
    static const Color4 honeydew;
    static const Color4 hotPink;
    static const Color4 indianRed;
    static const Color4 indigo;
    static const Color4 ivory;
    static const Color4 khaki;
    static const Color4 lavender;
    static const Color4 lavenderBlush;
    static const Color4 lawnGreen;
    static const Color4 lemonChiffon;
    static const Color4 lightBlue;
    static const Color4 lightCoral;
    static const Color4 lightCyan;
    static const Color4 lightGoldenrodYellow;
    static const Color4 lightGreen;
    static const Color4 lightGray;
    static const Color4 lightPink;
    static const Color4 lightSalmon;
    static const Color4 lightSeaGreen;
    static const Color4 lightSkyBlue;
    static const Color4 lightSlateGray;
    static const Color4 lightSteelBlue;
    static const Color4 lightYellow;
    static const Color4 lime;
    static const Color4 limeGreen;
    static const Color4 linen;
    static const Color4 magenta;
    static const Color4 maroon;
    static const Color4 mediumAquamarine;
    static const Color4 mediumBlue;
    static const Color4 mediumOrchid;
    static const Color4 mediumPurple;
    static const Color4 mediumSeaGreen;
    static const Color4 mediumSlateBlue;
    static const Color4 mediumSpringGreen;
    static const Color4 mediumTurquoise;
    static const Color4 mediumVioletRed;
    static const Color4 midnightBlue;
    static const Color4 mintCream;
    static const Color4 mistyRose;
    static const Color4 moccasin;
    static const Color4 navajoWhite;
    static const Color4 navy;
    static const Color4 oldLace;
    static const Color4 olive;
    static const Color4 oliveDrab;
    static const Color4 orange;
    static const Color4 orangeRed;
    static const Color4 orchid;
    static const Color4 paleGoldenrod;
    static const Color4 paleGreen;
    static const Color4 paleTurquoise;
    static const Color4 paleVioletRed;
    static const Color4 papayaWhip;
    static const Color4 peachPuff;
    static const Color4 peru;
    static const Color4 pink;
    static const Color4 plum;
    static const Color4 powderBlue;
    static const Color4 purple;
    static const Color4 red;
    static const Color4 rosyBrown;
    static const Color4 royalBlue;
    static const Color4 saddleBrown;
    static const Color4 salmon;
    static const Color4 sandyBrown;
    static const Color4 seaGreen;
    static const Color4 seaShell;
    static const Color4 sienna;
    static const Color4 silver;
    static const Color4 skyBlue;
    static const Color4 slateBlue;
    static const Color4 slateGray;
    static const Color4 snow;
    static const Color4 springGreen;
    static const Color4 steelBlue;
    static const Color4 tan;
    static const Color4 teal;
    static const Color4 thistle;
    static const Color4 tomato;
    static const Color4 turquoise;
    static const Color4 violet;
    static const Color4 wheat;
    static const Color4 white;
    static const Color4 whiteSmoke;
    static const Color4 yellow;
    static const Color4 yellowGreen;

    float               r;          ///< The red component.
    float               g;          ///< The green component.
    float               b;          ///< The blue component.
    float               a;          ///< The alpha component.
};

BE_INLINE constexpr Color4::Color4(float inR, float inG, float inB, float inA) :
    r(inR), g(inG), b(inB), a(inA) {
}

BE_INLINE constexpr Color4::Color4(const Color3 &inRgb, float inA) :
    r(inRgb.r), g(inRgb.g), b(inRgb.b), a(inA) {
}

BE_INLINE constexpr Color4::Color4(const float data[4]) :
    r(data[0]), g(data[1]), b(data[2]), a(data[3]) {
}

BE_INLINE constexpr Color4::Color4(float c) :
    r(c), g(c), b(c), a(c) {
}

BE_INLINE void Color4::Set(float r, float g, float b, float a) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

BE_INLINE float Color4::operator[](int index) const {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE float &Color4::operator[](int index) {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE Color4 &Color4::operator=(const Color4 &rhs) {
    r = rhs.r;
    g = rhs.g;
    b = rhs.b;
    a = rhs.a;
    return *this;
}

BE_INLINE Color4 &Color4::operator+=(const Color4 &rhs) {
    r += rhs.r;
    g += rhs.g;
    b += rhs.b;
    a += rhs.a;
    return *this;
}

BE_INLINE Color4 &Color4::operator+=(float rhs) {
    r += rhs;
    g += rhs;
    b += rhs;
    a += rhs;
    return *this;
}

BE_INLINE Color4 &Color4::operator-=(const Color4 &rhs) {
    r -= rhs.r;
    g -= rhs.g;
    b -= rhs.b;
    a -= rhs.a;
    return *this;
}

BE_INLINE Color4 &Color4::operator-=(float rhs) {
    r -= rhs;
    g -= rhs;
    b -= rhs;
    a -= rhs;
    return *this;
}

BE_INLINE Color4 &Color4::operator*=(float rhs) {
    r *= rhs;
    g *= rhs;
    b *= rhs;
    a *= rhs;
    return *this;
}

BE_INLINE Color4 &Color4::operator*=(const Color4 &rhs) {
    r *= rhs.r;
    g *= rhs.g;
    b *= rhs.b;
    a *= rhs.a;
    return *this;
};

BE_INLINE Color4 &Color4::operator/=(float rhs) {
    float inv = 1.0f / rhs;
    r *= inv;
    g *= inv;
    b *= inv;
    a *= inv;
    return *this;
}

BE_INLINE Color4 &Color4::operator/=(const Color4 &rhs) {
    r /= rhs.r;
    g /= rhs.g;
    b /= rhs.b;
    a /= rhs.a;
    return *this;
};

BE_INLINE bool Color4::Equals(const Color4 &c) const {
    return ((r == c.r) && (g == c.g) && (b == c.b) && (a == c.a));
}

BE_INLINE bool Color4::Equals(const Color4 &c, const float epsilon) const {
    if (Math::Fabs(r - c.r) > epsilon) {
        return false;
    }
    if (Math::Fabs(g - c.g) > epsilon) {
        return false;
    }
    if (Math::Fabs(b - c.b) > epsilon) {
        return false;
    }
    if (Math::Fabs(a - c.a) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE const char *Color4::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_INLINE const Color3 &Color4::ToColor3() const {
    return *reinterpret_cast<const Color3 *>(this);
}

BE_INLINE Color3 &Color4::ToColor3() {
    return *reinterpret_cast<Color3 *>(this);
}

BE_INLINE const Vec3 &Color4::ToVec3() const {
    return *reinterpret_cast<const Vec3 *>(this);
}

BE_INLINE Vec3 &Color4::ToVec3() {
    return *reinterpret_cast<Vec3 *>(this);
}

BE_INLINE const Vec4 &Color4::ToVec4() const {
    return *reinterpret_cast<const Vec4 *>(this);
}

BE_INLINE Vec4 &Color4::ToVec4() {
    return *reinterpret_cast<Vec4 *>(this);
}

BE_NAMESPACE_END
