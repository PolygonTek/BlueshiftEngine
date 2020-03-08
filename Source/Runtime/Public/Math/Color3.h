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

/// RGB color.
class Color3 {
public:
    /// Specifies the number of elements in this class.
    enum { Size = 3 };

    /// The default constructor does not initialize any members of this class.
    Color3() {}
    /// Constructs a Color3 with the value (r, g, b).
    constexpr Color3(float r, float g, float b);
    /// Constructs a Color3 from a C array, to the value (data[0], data[1], data[2]).
    explicit constexpr Color3(const float data[3]);
    /// Constructs a Color3 from a single value (s, s, s)
    explicit constexpr Color3(float s);

#ifdef QCOLOR_H
    /// Constructs from a QColor.
    Color3(const QColor &qc) { r = qc.red(); g = qc.green(); b = qc.blue(); }
#endif

                        /// Casts this Color3 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&r; }
    float *             Ptr() { return (float *)&r; }
                        /// Casts this Color3 to a C array.
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
    Color3              Add(const Color3 &v) const { return *this + v; }
                        /// Adds a color to this color.
                        /// This function is identical to the member function Add().
    Color3              operator+(const Color3 &rhs) const { return Color3(r + rhs.r, g + rhs.g, b + rhs.b); }
                        /// Adds the color (s, s, s, s) to this color.
    Color3              AddScalar(float s) const { return *this + s; }
                        /// Adds the color (s, s, s, s) to this color.
                        /// This function is identical to the member function AddScalar().
    Color3              operator+(float rhs) const { return Color3(r + rhs, g + rhs, b + rhs); }
                        /// Adds the color v to color (s, s, s, s).
    friend Color3       operator+(float lhs, const Color3 &rhs) { return Color3(lhs + rhs.r, lhs + rhs.g, lhs + rhs.b); }

                        /// Subtracts a color from this color.
    Color3              Sub(const Color3 &v) const { return *this - v; }
                        /// Subtracts the given color from this color.
                        /// This function is identical to the member function Sub()
    Color3              operator-(const Color3 &rhs) const { return Color3(r - rhs.r, g - rhs.g, b - rhs.b); }
                        /// Subtracts the color (s, s, s, s) from this color.
    Color3              SubScalar(float s) const { return *this - s; }
                        /// Subtracts the color (s, s, s, s) from this color.
                        /// This function is identical to the member function SubScalar()
    Color3              operator-(float rhs) const { return Color3(r - rhs, g - rhs, b - rhs); }
                        /// Subtracts the color v from color (s, s, s, s).
    friend Color3       operator-(float lhs, const Color3 &rhs) { return Color3(lhs - rhs.r, lhs - rhs.g, lhs - rhs.b); }

                        /// Multiplies this color by a scalar.
    Color3              Mul(float s) const { return *this * s; }
                        /// Multiplies this color by a scalar.
                        /// This function is identical to the member function Mul().
    Color3              operator*(float rhs) const { return Color3(r * rhs, g * rhs, b * rhs); }
                        /// Multiplies color v by a scalar.
    friend Color3       operator*(float lhs, const Color3 &rhs) { return Color3(lhs * rhs.r, lhs * rhs.g, lhs * rhs.b); }
                        /// Multiplies this color by a color, element-wise.
    Color3              MulComp(const Color3 &v) const { return *this * v; }
                        /// Multiplies this color by a color, element-wise.
                        /// This function is identical to the member function MulComp().
    Color3              operator*(const Color3 &rhs) const { return Color3(r * rhs.r, g * rhs.g, b * rhs.b); }

                        /// Divides this color by a scalar.
    Color3              Div(float s) const { return *this / s; }
                        /// Divides this color by a scalar.
                        /// This function is identical to the member function Div().
    Color3              operator/(float rhs) const { float inv = 1.f / rhs; return Color3(r * inv, g * inv, b * inv); }
                        /// Divides this color by a color, element-wise.
    Color3              DivComp(const Color3 &v) const { return *this / v; }
                        /// This function is identical to the member function DivComp().
                        /// Divides this color by a color, element-wise.
    Color3              operator/(const Color3 &rhs) const { return Color3(r / rhs.r, g / rhs.g, b / rhs.b); }
                        /// Divides color (s, s, s, s) by a color, element-wise.
    friend Color3       operator/(float lhs, const Color3 &rhs) { return Color3(lhs / rhs.r, lhs / rhs.g, lhs / rhs.b); }

                        /// Assign from another color.
    Color3 &            operator=(const Color3 &rhs);

                        /// Adds a color to this color, in-place.
    Color3 &            AddSelf(const Color3 &v) { *this += v; return *this; }
                        /// Adds a color to this color, in-place.
                        /// This function is identical to the member function AddSelf().
    Color3 &            operator+=(const Color3 &rhs);

                        /// Subtracts a color from this color, in-place.
    Color3 &            SubSelf(const Color3 &v) { *this -= v; return *this; }
                        /// Subtracts a color from this color, in-place.
                        /// This function is identical to the member function SubSelf().
    Color3 &            operator-=(const Color3 &rhs);

                        /// Multiplies this color by a scalar, in-place.
    Color3 &            MulSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this color by a scalar, in-place.
                        /// This function is identical to the member function MulSelf().
    Color3 &            operator*=(float rhs);

                        /// Multiplies this color by a color, element-wise, in-place.
    Color3 &            MulCompSelf(const Color3 &v) { *this *= v; return *this; }
                        /// Multiplies this color by a color, element-wise, in-place.
                        /// This function is identical to the member function MulCompSelf().
    Color3 &            operator*=(const Color3 &rhs);

                        /// Divides this color by a scalar, in-place.
    Color3 &            DivSelf(float s) { *this /= s; return *this; }
                        /// Divides this color by a scalar, in-place.
                        /// This function is identical to the member function DivSelf().
    Color3 &            operator/=(float rhs);

                        /// Divides this color by a color, element-wise, in-place.
    Color3 &            DivCompSelf(const Color3 &v) { *this /= v; return *this; }
                        /// Divides this color by a color, element-wise, in-place.
                        /// This function is identical to the member function DivCompSelf().
    Color3 &            operator/=(const Color3 &rhs);

                        /// Exact compare, no epsilon.
    bool                Equals(const Color3 &c) const;
                        /// Compare with epsilon.
    bool                Equals(const Color3 &c, const float epsilon) const;
                        /// Exact compare, no epsilon.
    bool                operator==(const Color3 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const Color3 &rhs) const { return !Equals(rhs); }

                        /// Sets all element of this color.
    void                Set(float r, float g, float b);

                        /// Performs a RGB swizzled access to this color.
    Color3              rrr() const { return Color3(r, r, r); }
    Color3              rrg() const { return Color3(r, r, g); }
    Color3              rrb() const { return Color3(r, r, b); }
    Color3              rgr() const { return Color3(r, g, r); }
    Color3              rgg() const { return Color3(r, g, g); }
    Color3              rgb() const { return Color3(r, g, b); }
    Color3              rbr() const { return Color3(r, b, r); }
    Color3              rbg() const { return Color3(r, b, g); }
    Color3              rbb() const { return Color3(r, b, b); }

    Color3              grr() const { return Color3(g, r, r); }
    Color3              grg() const { return Color3(g, r, g); }
    Color3              grb() const { return Color3(g, r, b); }
    Color3              ggr() const { return Color3(g, g, r); }
    Color3              ggg() const { return Color3(g, g, g); }
    Color3              ggb() const { return Color3(g, g, b); }
    Color3              gbr() const { return Color3(g, b, r); }
    Color3              gbg() const { return Color3(g, b, g); }
    Color3              gbb() const { return Color3(g, b, b); }

    Color3              brr() const { return Color3(b, r, r); }
    Color3              brg() const { return Color3(b, r, g); }
    Color3              brb() const { return Color3(b, r, b); }
    Color3              bgr() const { return Color3(b, g, r); }
    Color3              bgg() const { return Color3(b, g, g); }
    Color3              bgb() const { return Color3(b, g, b); }
    Color3              bbr() const { return Color3(b, b, r); }
    Color3              bbg() const { return Color3(b, b, g); }
    Color3              bbb() const { return Color3(b, b, b); }

                        /// Clip to [0, 1] range.
    void                Clip();
                        /// Inverts the RGB channels.
    void                Invert();
                        /// Grayscale this color, in-place.
    Color3 &            Grayscale();

                        /// Returns "r g b".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "r g b" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static Color3       FromString(const char *str);

                        /// Casts this Color3 to a Vec3.
    const Vec3 &        ToVec3() const;
    Vec3 &              ToVec3();

                        /// Creates from RGB888.
    static Color3       FromRGB888(uint8_t r, uint8_t g, uint8_t b);

                        /// Creates from packed to a 32-bit integer, with R component in the lowest 8 bits.
    static Color3       FromUInt32(uint32_t rgbx);

                        /// Converts color to a 32-bit integer, with R component in the lowest 8 bits. Components are clamped to [0, 1] range.
    uint32_t            ToUInt32() const;

#ifdef QCOLOR_H
                        /// Convert Color3 to QColor.
    QColor              ToQColor() const { return QColor::fromRgbF(r, g, b); }
#endif

                        /// Converts RGB to HSL.
    Color3              ToHSL() const;
                        /// Converts HSL to RGB.
    Color3              FromHSL() const;

                        /// Converts sRGB to Linear.
    Color3              SRGBToLinear() const;
                        /// Converts Linear to sRGB.
    Color3              LinearToSRGB() const;

                        /// Converts temperature in Kelvins [1000, 15000] of a black body radiator to RGB chromaticity (linear).
    static Color3       FromColorTemperature(float temp);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return Size; }

    static const Color3 zero;

    static const Color3 aliceBlue;
    static const Color3 antiqueWhite;
    static const Color3 aqua;
    static const Color3 aquamarine;
    static const Color3 azure;
    static const Color3 beige;
    static const Color3 bisque;
    static const Color3 black;
    static const Color3 blanchedAlmond;
    static const Color3 blue;
    static const Color3 blueViolet;
    static const Color3 brown;
    static const Color3 burlyWood;
    static const Color3 cadetBlue;
    static const Color3 chartreuse;
    static const Color3 chocolate;
    static const Color3 coral;
    static const Color3 cornflowerBlue;
    static const Color3 cornsilk;
    static const Color3 crimson;
    static const Color3 cyan;
    static const Color3 darkBlue;
    static const Color3 darkCyan;
    static const Color3 darkGoldenrod;
    static const Color3 darkGray;
    static const Color3 darkGreen;
    static const Color3 darkKhaki;
    static const Color3 darkMagenta;
    static const Color3 darkOliveGreen;
    static const Color3 darkOrange;
    static const Color3 darkOrchid;
    static const Color3 darkRed;
    static const Color3 darkSalmon;
    static const Color3 darkSeaGreen;
    static const Color3 darkSlateBlue;
    static const Color3 darkSlateGray;
    static const Color3 darkTurquoise;
    static const Color3 darkViolet;
    static const Color3 deepPink;
    static const Color3 deepSkyBlue;
    static const Color3 dimGray;
    static const Color3 dodgerBlue;
    static const Color3 firebrick;
    static const Color3 floralWhite;
    static const Color3 forestGreen;
    static const Color3 fuchsia;
    static const Color3 gainsboro;
    static const Color3 ghostWhite;
    static const Color3 gold;
    static const Color3 goldenrod;
    static const Color3 gray;
    static const Color3 green;
    static const Color3 greenYellow;
    static const Color3 honeydew;
    static const Color3 hotPink;
    static const Color3 indianRed;
    static const Color3 indigo;
    static const Color3 ivory;
    static const Color3 khaki;
    static const Color3 lavender;
    static const Color3 lavenderBlush;
    static const Color3 lawnGreen;
    static const Color3 lemonChiffon;
    static const Color3 lightBlue;
    static const Color3 lightCoral;
    static const Color3 lightCyan;
    static const Color3 lightGoldenrodYellow;
    static const Color3 lightGreen;
    static const Color3 lightGray;
    static const Color3 lightPink;
    static const Color3 lightSalmon;
    static const Color3 lightSeaGreen;
    static const Color3 lightSkyBlue;
    static const Color3 lightSlateGray;
    static const Color3 lightSteelBlue;
    static const Color3 lightYellow;
    static const Color3 lime;
    static const Color3 limeGreen;
    static const Color3 linen;
    static const Color3 magenta;
    static const Color3 maroon;
    static const Color3 mediumAquamarine;
    static const Color3 mediumBlue;
    static const Color3 mediumOrchid;
    static const Color3 mediumPurple;
    static const Color3 mediumSeaGreen;
    static const Color3 mediumSlateBlue;
    static const Color3 mediumSpringGreen;
    static const Color3 mediumTurquoise;
    static const Color3 mediumVioletRed;
    static const Color3 midnightBlue;
    static const Color3 mintCream;
    static const Color3 mistyRose;
    static const Color3 moccasin;
    static const Color3 navajoWhite;
    static const Color3 navy;
    static const Color3 oldLace;
    static const Color3 olive;
    static const Color3 oliveDrab;
    static const Color3 orange;
    static const Color3 orangeRed;
    static const Color3 orchid;
    static const Color3 paleGoldenrod;
    static const Color3 paleGreen;
    static const Color3 paleTurquoise;
    static const Color3 paleVioletRed;
    static const Color3 papayaWhip;
    static const Color3 peachPuff;
    static const Color3 peru;
    static const Color3 pink;
    static const Color3 plum;
    static const Color3 powderBlue;
    static const Color3 purple;
    static const Color3 red;
    static const Color3 rosyBrown;
    static const Color3 royalBlue;
    static const Color3 saddleBrown;
    static const Color3 salmon;
    static const Color3 sandyBrown;
    static const Color3 seaGreen;
    static const Color3 seaShell;
    static const Color3 sienna;
    static const Color3 silver;
    static const Color3 skyBlue;
    static const Color3 slateBlue;
    static const Color3 slateGray;
    static const Color3 snow;
    static const Color3 springGreen;
    static const Color3 steelBlue;
    static const Color3 tan;
    static const Color3 teal;
    static const Color3 thistle;
    static const Color3 tomato;
    static const Color3 turquoise;
    static const Color3 violet;
    static const Color3 wheat;
    static const Color3 white;
    static const Color3 whiteSmoke;
    static const Color3 yellow;
    static const Color3 yellowGreen;

    float               r;          ///< The red component.
    float               g;          ///< The green component.
    float               b;          ///< The blue component.
};

BE_INLINE constexpr Color3::Color3(float inR, float inG, float inB) :
    r(inR), g(inG), b(inB) {
}

BE_INLINE constexpr Color3::Color3(const float data[3]) :
    r(data[0]), g(data[1]), b(data[2]) {
}

BE_INLINE constexpr Color3::Color3(float c) :
    r(c), g(c), b(c) {
}

BE_INLINE void Color3::Set(float r, float g, float b) {
    this->r = r;
    this->g = g;
    this->b = b;
}

BE_INLINE float Color3::operator[](int index) const {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE float &Color3::operator[](int index) {
    assert(index >= 0 && index < Size);
    return ((float *)this)[index];
}

BE_INLINE Color3 &Color3::operator=(const Color3 &rhs) {
    r = rhs.r;
    g = rhs.g;
    b = rhs.b;
    return *this;
}

BE_INLINE Color3 &Color3::operator+=(const Color3 &rhs) {
    r += rhs.r;
    g += rhs.g;
    b += rhs.b;
    return *this;
}

BE_INLINE Color3 &Color3::operator-=(const Color3 &rhs) {
    r -= rhs.r;
    g -= rhs.g;
    b -= rhs.b;
    return *this;
}

BE_INLINE Color3 &Color3::operator*=(float rhs) {
    r *= rhs;
    g *= rhs;
    b *= rhs;
    return *this;
}

BE_INLINE Color3 &Color3::operator*=(const Color3 &rhs) {
    r *= rhs.r;
    g *= rhs.g;
    b *= rhs.b;
    return *this;
};

BE_INLINE Color3 &Color3::operator/=(float rhs) {
    float inv = 1.0f / rhs;
    r *= inv;
    g *= inv;
    b *= inv;
    return *this;
}

BE_INLINE Color3 &Color3::operator/=(const Color3 &rhs) {
    r /= rhs.r;
    g /= rhs.g;
    b /= rhs.b;
    return *this;
};

BE_INLINE bool Color3::Equals(const Color3 &rhs) const {
    return ((r == rhs.r) && (g == rhs.g) && (b == rhs.b));
}

BE_INLINE bool Color3::Equals(const Color3 &rhs, const float epsilon) const {
    if (Math::Fabs(r - rhs.r) > epsilon) {
        return false;
    }
    if (Math::Fabs(g - rhs.g) > epsilon) {
        return false;
    }
    if (Math::Fabs(b - rhs.b) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE const char *Color3::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_INLINE const Vec3 &Color3::ToVec3() const {
    return *reinterpret_cast<const Vec3 *>(this);
}

BE_INLINE Vec3 &Color3::ToVec3() {
    return *reinterpret_cast<Vec3 *>(this);
}

BE_NAMESPACE_END
