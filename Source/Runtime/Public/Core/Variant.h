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

#include "jsoncpp/include/json/json.h"
#include "Containers/Array.h"
#include "Containers/StrArray.h"
#include "Math/Math.h"
#include "Core/Guid.h"
#include "Core/MinMaxCurve.h"
#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class Variant;

using VariantArray = Array<Variant>;

/// Variable that supports a fixed set of types.
class BE_API Variant {
public:
    struct Type {
        enum Enum {
            None = -1,
            Int,
            Int64,
            Bool,
            Float,
            Double,
            VoidPtr,
            Vec2,
            Vec3,
            Vec4,
            Color3,
            Color4,
            Mat2,
            Mat3,
            Mat3x4,
            Mat4,
            Angles,
            Quat,
            Point,
            PointF,
            Size,
            SizeF,
            Rect,
            RectF,
            Guid,
            Str,
            MinMaxCurve,
        };
    };

    struct Value {
        union {
            int i1;
            bool b1;
            float f1;
            double d1;
            void *ptr1;
        };

        union {
            int i2;
            bool b2;
            float f2;
            double d2;
            void *ptr2;
        };

        union {
            int i3;
            bool b3;
            float f3;
            double d3;
            void *ptr3;
        };

        union {
            int i4;
            bool b4;
            float f4;
            double d4;
            void *ptr4;
        };
    };

    Variant() 
        : type(Type::None) {
    }

    Variant(const Variant &value)
        : type(Type::None) {
        *this = value;
    }
    
    Variant(int value)
        : type(Type::None) {
        *this = value;
    }

    Variant(unsigned int value)
        : type(Type::None) {
        *this = (int)value;
    }

    Variant(int64_t value)
        : type(Type::None) {
        *this = value;
    }

    Variant(uint64_t value)
        : type(Type::None) {
        *this = (int64_t)value;
    }

    Variant(bool value)
        : type(Type::None) {
        *this = value;
    }

    Variant(float value)
        : type(Type::None) {
        *this = value;
    }

    Variant(double value)
        : type(Type::None) {
        *this = value;
    }

    Variant(void *value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const char *value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Vec2 &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Vec3 &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Vec4 &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Color3 &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Color4 &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Mat2 &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Mat3 &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Mat3x4 &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Mat4 &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Angles &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Quat &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Point &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const PointF &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Size &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const SizeF &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Rect &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const RectF &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Guid &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const Str &value)
        : type(Type::None) {
        *this = value;
    }

    Variant(const MinMaxCurve &value)
        : type(Type::None) {
        *this = value;
    }

    ~Variant() {
        SetType(Type::None);
    }

    void                    Clear();

    Type::Enum              GetType() const { return type; }
    void                    SetType(Type::Enum type);

    bool                    SetFromString(Type::Enum type, const char *str);

    Variant &               operator=(const Variant &rhs);

    Variant &               operator=(int rhs);
    Variant &               operator=(int64_t rhs);
    Variant &               operator=(bool rhs);
    Variant &               operator=(float rhs);
    Variant &               operator=(double rhs);
    Variant &               operator=(void *rhs);
    Variant &               operator=(const char *rhs);
    Variant &               operator=(const Vec2 &rhs);
    Variant &               operator=(const Vec3 &rhs);
    Variant &               operator=(const Vec4 &rhs);
    Variant &               operator=(const Color3 &rhs);
    Variant &               operator=(const Color4 &rhs);
    Variant &               operator=(const Mat2 &rhs);
    Variant &               operator=(const Mat3 &rhs);
    Variant &               operator=(const Mat4 &rhs);
    Variant &               operator=(const Mat3x4 &rhs);
    Variant &               operator=(const Angles &rhs);
    Variant &               operator=(const Quat &rhs);
    Variant &               operator=(const Point &rhs);
    Variant &               operator=(const PointF &rhs);
    Variant &               operator=(const Size &rhs);
    Variant &               operator=(const SizeF &rhs);
    Variant &               operator=(const Rect &rhs);
    Variant &               operator=(const RectF &rhs);
    Variant &               operator=(const Guid &rhs);
    Variant &               operator=(const Str &rhs);
    Variant &               operator=(const MinMaxCurve &rhs);

    bool                    operator==(const Variant &rhs) const;
    bool                    operator!=(const Variant &rhs) const { return !(*this == rhs); }

    template <typename T>
    const T &               As() const;

    Json::Value             ToJsonValue() const;

    Str                     ToString() const;

    static Variant          FromString(Type::Enum type, const char *str);

    static Variant          empty;

private:
    Type::Enum              type;       ///< Variant type
    Value                   value;      ///< Variant value
};

BE_INLINE void Variant::Clear() {
    SetType(Type::None);
}

BE_INLINE Variant &Variant::operator=(int rhs) {
    SetType(Type::Int);
    value.i1 = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(int64_t rhs) {
    SetType(Type::Int64);
    *(reinterpret_cast<int64_t *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(bool rhs) {
    SetType(Type::Bool);
    value.b1 = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(float rhs) {
    SetType(Type::Float);
    value.f1 = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(double rhs) {
    SetType(Type::Double);
    value.f1 = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(void *rhs) {
    SetType(Type::VoidPtr);
    value.ptr1 = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const char *rhs) {
    SetType(Type::Str);
    *(reinterpret_cast<Str *>(value.ptr1)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Vec2 &rhs) {
    SetType(Type::Vec2);
    *(reinterpret_cast<Vec2 *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Vec3 &rhs) {
    SetType(Type::Vec3);
    *(reinterpret_cast<Vec3 *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Vec4 &rhs) {
    SetType(Type::Vec4);
    *(reinterpret_cast<Vec4 *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Color3 &rhs) {
    SetType(Type::Color3);
    *(reinterpret_cast<Color3 *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Color4 &rhs) {
    SetType(Type::Color4);
    *(reinterpret_cast<Color4 *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Mat2 &rhs) {
    SetType(Type::Mat2);
    *(reinterpret_cast<Mat2 *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Mat3 &rhs) {
    SetType(Type::Mat3);
    *(reinterpret_cast<Mat3 *>(value.ptr1)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Mat4 &rhs) {
    SetType(Type::Mat4);
    *(reinterpret_cast<Mat4 *>(value.ptr1)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Mat3x4 &rhs) {
    SetType(Type::Mat3x4);
    *(reinterpret_cast<Mat3x4 *>(value.ptr1)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Angles &rhs) {
    SetType(Type::Angles);
    *(reinterpret_cast<Angles *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Quat &rhs) {
    SetType(Type::Quat);
    *(reinterpret_cast<Quat *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Point &rhs) {
    SetType(Type::Point);
    *(reinterpret_cast<Point *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const PointF &rhs) {
    SetType(Type::PointF);
    *(reinterpret_cast<PointF *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Size &rhs) {
    SetType(Type::Size);
    *(reinterpret_cast<Size *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const SizeF &rhs) {
    SetType(Type::SizeF);
    *(reinterpret_cast<SizeF *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Rect &rhs) {
    SetType(Type::Rect);
    *(reinterpret_cast<Rect *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const RectF &rhs) {
    SetType(Type::RectF);
    *(reinterpret_cast<RectF *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Guid &rhs) {
    SetType(Type::Guid);
    *(reinterpret_cast<Guid *>(&value)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const Str &rhs) {
    SetType(Type::Str);
    *(reinterpret_cast<Str *>(value.ptr1)) = rhs;
    return *this;
}

BE_INLINE Variant &Variant::operator=(const MinMaxCurve &rhs) {
    SetType(Type::MinMaxCurve);
    *(reinterpret_cast<MinMaxCurve *>(value.ptr1)) = rhs;
    return *this;
}

template <typename T>
BE_INLINE const T &Variant::As() const {
    return *reinterpret_cast<const T *>(&value);
}

template <>
BE_INLINE const Vec2 &Variant::As() const {
    return type == Type::Vec2 ? *reinterpret_cast<const Vec2 *>(&value) : Vec2::zero;
}

template <>
BE_INLINE const Vec3 &Variant::As() const {
    return type == Type::Vec3 ? *reinterpret_cast<const Vec3 *>(&value) : Vec3::zero;
}

template <>
BE_INLINE const Vec4 &Variant::As() const {
    return type == Type::Vec4 ? *reinterpret_cast<const Vec4 *>(&value) : Vec4::zero;
}

template <>
BE_INLINE const Color3 &Variant::As() const {
    return type == Type::Color3 ? *reinterpret_cast<const Color3 *>(&value) : Color3::zero;
}

template <>
BE_INLINE const Color4 &Variant::As() const {
    return type == Type::Color4 ? *reinterpret_cast<const Color4 *>(&value) : Color4::zero;
}

template <>
BE_INLINE const Mat2 &Variant::As() const {
    return type == Type::Mat2 ? *reinterpret_cast<const Mat2 *>(&value) : Mat2::identity;
}

template <>
BE_INLINE const Mat3 &Variant::As() const {
    return type == Type::Mat3 ? *reinterpret_cast<const Mat3 *>(value.ptr1) : Mat3::identity;
}

template <>
BE_INLINE const Mat4 &Variant::As() const {
    return type == Type::Mat4 ? *reinterpret_cast<const Mat4 *>(value.ptr1) : Mat4::identity;
}

template <>
BE_INLINE const Mat3x4 &Variant::As() const {
    return type == Type::Mat3x4 ? *reinterpret_cast<const Mat3x4 *>(value.ptr1) : Mat3x4::identity;
}

template <>
BE_INLINE const Angles &Variant::As() const {
    return type == Type::Angles ? *reinterpret_cast<const Angles *>(&value) : Angles::zero;
}

template <>
BE_INLINE const Quat &Variant::As() const {
    return type == Type::Quat ? *reinterpret_cast<const Quat *>(&value) : Quat::identity;
}

template <>
BE_INLINE const Point &Variant::As() const {
    return type == Type::Point ? *reinterpret_cast<const Point *>(&value) : Point::zero;
}

template <>
BE_INLINE const PointF &Variant::As() const {
    return type == Type::PointF ? *reinterpret_cast<const PointF *>(&value) : PointF::zero;
}

template <>
BE_INLINE const Size &Variant::As() const {
    return type == Type::Size ? *reinterpret_cast<const Size *>(&value) : Size::zero;
}

template <>
BE_INLINE const SizeF &Variant::As() const {
    return type == Type::SizeF ? *reinterpret_cast<const SizeF *>(&value) : SizeF::zero;
}

template <>
BE_INLINE const Rect &Variant::As() const {
    return type == Type::Rect ? *reinterpret_cast<const Rect *>(&value) : Rect::zero;
}

template <>
BE_INLINE const RectF &Variant::As() const {
    return type == Type::RectF ? *reinterpret_cast<const RectF *>(&value) : RectF::zero;
}

template <>
BE_INLINE const Guid &Variant::As() const {
    return type == Type::Guid ? *reinterpret_cast<const Guid *>(&value) : Guid::zero;
}

template <>
BE_INLINE const Str &Variant::As() const {
    return type == Type::Str ? *reinterpret_cast<const Str *>(value.ptr1) : Str::empty;
}

template <>
BE_INLINE const MinMaxCurve &Variant::As() const {
    return type == Type::MinMaxCurve ? *reinterpret_cast<const MinMaxCurve *>(value.ptr1) : MinMaxCurve::empty;
}

template <typename T, typename = void>
struct VariantType {};

template <typename T>
struct VariantType<T, typename std::enable_if_t<std::is_enum<T>::value>> {
    static Variant::Type::Enum GetType() { return VariantType<std::underlying_type_t<T>>::GetType(); }
};

template <>
struct VariantType<int> {
    static Variant::Type::Enum GetType() { return Variant::Type::Int; }
};

template <>
struct VariantType<unsigned> {
    static Variant::Type::Enum GetType() { return Variant::Type::Int; }
};

template <>
struct VariantType<bool> {
    static Variant::Type::Enum GetType() { return Variant::Type::Bool; }
};

template <>
struct VariantType<float> {
    static Variant::Type::Enum GetType() { return Variant::Type::Float; }
};

template <>
struct VariantType<double> {
    static Variant::Type::Enum GetType() { return Variant::Type::Double; }
};

template <>
struct VariantType<Vec2> {
    static Variant::Type::Enum GetType() { return Variant::Type::Vec2; }
};

template <>
struct VariantType<Vec3> {
    static Variant::Type::Enum GetType() { return Variant::Type::Vec3; }
};

template <>
struct VariantType<Vec4> {
    static Variant::Type::Enum GetType() { return Variant::Type::Vec4; }
};

template <>
struct VariantType<Color3> {
    static Variant::Type::Enum GetType() { return Variant::Type::Color3; }
};

template <>
struct VariantType<Color4> {
    static Variant::Type::Enum GetType() { return Variant::Type::Color4; }
};

template <>
struct VariantType<Mat2> {
    static Variant::Type::Enum GetType() { return Variant::Type::Mat2; }
};

template <>
struct VariantType<Mat3> {
    static Variant::Type::Enum GetType() { return Variant::Type::Mat3; }
};

template <>
struct VariantType<Mat3x4> {
    static Variant::Type::Enum GetType() { return Variant::Type::Mat3x4; }
};

template <>
struct VariantType<Mat4> {
    static Variant::Type::Enum GetType() { return Variant::Type::Mat4; }
};

template <>
struct VariantType<Angles> {
    static Variant::Type::Enum GetType() { return Variant::Type::Angles; }
};

template <>
struct VariantType<Quat> {
    static Variant::Type::Enum GetType() { return Variant::Type::Quat; }
};

template <>
struct VariantType<Point> {
    static Variant::Type::Enum GetType() { return Variant::Type::Point; }
};

template <>
struct VariantType<PointF> {
    static Variant::Type::Enum GetType() { return Variant::Type::PointF; }
};

template <>
struct VariantType<Size> {
    static Variant::Type::Enum GetType() { return Variant::Type::Size; }
};

template <>
struct VariantType<SizeF> {
    static Variant::Type::Enum GetType() { return Variant::Type::SizeF; }
};

template <>
struct VariantType<Rect> {
    static Variant::Type::Enum GetType() { return Variant::Type::Rect; }
};

template <>
struct VariantType<RectF> {
    static Variant::Type::Enum GetType() { return Variant::Type::RectF; }
};

template <>
struct VariantType<Str> {
    static Variant::Type::Enum GetType() { return Variant::Type::Str; }
};

template <>
struct VariantType<Guid> {
    static Variant::Type::Enum GetType() { return Variant::Type::Guid; }
};

BE_NAMESPACE_END
