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
#include "Core/Variant.h"
#include "Core/Object.h"

BE_NAMESPACE_BEGIN

Variant Variant::empty;

void Variant::SetType(Type::Enum type) {
    if (this->type == type) {
        return;
    }

    switch (this->type) {
    case Type::Str:
        delete reinterpret_cast<Str *>(value.ptr1);
        break;
    case Type::Mat3:
        delete reinterpret_cast<Mat3 *>(value.ptr1);
        break;
    case Type::Mat3x4:
        delete reinterpret_cast<Mat3x4 *>(value.ptr1);
        break;
    case Type::Mat4:
        delete reinterpret_cast<Mat4 *>(value.ptr1);
        break;
    case Type::MinMaxCurve:
        delete reinterpret_cast<MinMaxCurve *>(value.ptr1);
        break;
    default:
        break;
    }

    this->type = type;

    switch (type) {
    case Type::Str:
        value.ptr1 = new Str();
        break;
    case Type::Mat3:
        value.ptr1 = new Mat3();
        break;
    case Type::Mat3x4:
        value.ptr1 = new Mat3x4();
        break;
    case Type::Mat4:
        value.ptr1 = new Mat4();
        break;
    case Type::MinMaxCurve:
        value.ptr1 = new MinMaxCurve();
        break;
    default:
        break;
    }
}

bool Variant::SetFromString(Type::Enum type, const char *str) {
    switch (type) {
    case Type::Int:
        *this = Variant(atoi(str));
        return true;
    case Type::Int64:
        *this = Variant(Str::ToI64(str));
        return true;
    case Type::Bool:
        *this = Variant(!Str::Cmp(str, "true"));
        return true;
    case Type::Float:
        *this = Variant((float)atof(str));
        return true;
    case Type::Double:
        *this = Variant(atof(str));
        return true;
    case Type::VoidPtr:
        *this = Variant((sizeof(void *) == sizeof(uint64_t) ? Str::ToUI64(str) : Str::ToUI32(str)));
        return true;
    case Type::Vec2:
        *this = Vec2::FromString(str);
        return true;
    case Type::Vec3:
        *this = Vec3::FromString(str);
        return true;
    case Type::Vec4:
        *this = Vec4::FromString(str);
        return true;
    case Type::Color3:
        *this = Color3::FromString(str);
        return true;
    case Type::Color4:
        *this = Color4::FromString(str);
        return true;
    case Type::Mat2:
        *this = Mat2::FromString(str);
        return true;
    case Type::Mat3:
        *this = Mat3::FromString(str);
        return true;
    case Type::Mat4:
        *this = Mat4::FromString(str);
        return true;
    case Type::Mat3x4:
        *this = Mat3x4::FromString(str);
        return true;
    case Type::Angles:
        *this = Angles::FromString(str);
        return true;
    case Type::Quat:
        *this = Quat::FromString(str);
        return true;
    case Type::Point:
        *this = Point::FromString(str);
        return true;
    case Type::PointF:
        *this = PointF::FromString(str);
        return true;
    case Type::Rect:
        *this = Rect::FromString(str);
        return true;
    case Type::RectF:
        *this = RectF::FromString(str);
        return true;
    case Type::Guid:
        *this = Guid::FromString(str);
        return true;
    case Type::Str:
        *this = Str(str);
        return true;
    default:
        // Type::MinMaxCurve can not support here
        assert(0);
        return false;
    }

    return false;
}

Variant &Variant::operator=(const Variant &rhs) {
    SetType(rhs.type);

    switch (type) {
    case Type::Str:
        *(reinterpret_cast<Str *>(value.ptr1)) = *(reinterpret_cast<const Str *>(rhs.value.ptr1));
        break;
    case Type::Mat3:
        *(reinterpret_cast<Mat3 *>(value.ptr1)) = *(reinterpret_cast<const Mat3 *>(rhs.value.ptr1));
        break;
    case Type::Mat3x4:
        *(reinterpret_cast<Mat3x4 *>(value.ptr1)) = *(reinterpret_cast<const Mat3x4 *>(rhs.value.ptr1));
        break;
    case Type::Mat4:
        *(reinterpret_cast<Mat4 *>(value.ptr1)) = *(reinterpret_cast<const Mat4 *>(rhs.value.ptr1));
        break;
    case Type::MinMaxCurve:
        *(reinterpret_cast<MinMaxCurve *>(value.ptr1)) = *(reinterpret_cast<const MinMaxCurve *>(rhs.value.ptr1));
        break;
    default:
        value = rhs.value;
        break;
    }

    return *this;
}

bool Variant::operator==(const Variant &rhs) const {
    if (type != rhs.type) {
        return false;
    }
    
    switch (type) {
    case Type::Int:
        return value.i1 == rhs.value.i1;
    case Type::Int64:
        return *(reinterpret_cast<const int64_t *>(&value)) == *(reinterpret_cast<const int64_t *>(&rhs.value));
    case Type::Bool:
        return value.b1 == rhs.value.b1;
    case Type::Float:
        return value.f1 == rhs.value.f1;
    case Type::Double:
        return value.d1 == rhs.value.d1;
    case Type::VoidPtr:
        return value.ptr1 == rhs.value.ptr2;
    case Type::Vec2:
        return *(reinterpret_cast<const Vec2 *>(&value)) == *(reinterpret_cast<const Vec2 *>(&rhs.value));
    case Type::Vec3:
        return *(reinterpret_cast<const Vec3 *>(&value)) == *(reinterpret_cast<const Vec3 *>(&rhs.value));
    case Type::Vec4:
        return *(reinterpret_cast<const Vec4 *>(&value)) == *(reinterpret_cast<const Vec4 *>(&rhs.value));
    case Type::Color3:
        return *(reinterpret_cast<const Color3 *>(&value)) == *(reinterpret_cast<const Color3 *>(&rhs.value));
    case Type::Color4:
        return *(reinterpret_cast<const Color4 *>(&value)) == *(reinterpret_cast<const Color4 *>(&rhs.value));
    case Type::Mat2:
        return *(reinterpret_cast<const Mat2 *>(&value)) == *(reinterpret_cast<const Mat2 *>(&rhs.value));
    case Type::Mat3:
        return *(reinterpret_cast<const Mat3 *>(value.ptr1)) == *(reinterpret_cast<const Mat3 *>(rhs.value.ptr1));
    case Type::Mat4:
        return *(reinterpret_cast<const Mat4 *>(value.ptr1)) == *(reinterpret_cast<const Mat4 *>(rhs.value.ptr1));
    case Type::Mat3x4:
        return *(reinterpret_cast<const Mat3x4 *>(value.ptr1)) == *(reinterpret_cast<const Mat3x4 *>(rhs.value.ptr1));
    case Type::Angles:
        return *(reinterpret_cast<const Angles *>(&value)) == *(reinterpret_cast<const Angles *>(&rhs.value));
    case Type::Quat:
        return *(reinterpret_cast<const Quat *>(&value)) == *(reinterpret_cast<const Quat *>(&rhs.value));
    case Type::Point:
        return *(reinterpret_cast<const Point *>(&value)) == *(reinterpret_cast<const Point *>(&rhs.value));
    case Type::PointF:
        return *(reinterpret_cast<const PointF *>(&value)) == *(reinterpret_cast<const PointF *>(&rhs.value));
    case Type::Rect:
        return *(reinterpret_cast<const Rect *>(&value)) == *(reinterpret_cast<const Rect *>(&rhs.value));
    case Type::RectF:
        return *(reinterpret_cast<const RectF *>(&value)) == *(reinterpret_cast<const RectF *>(&rhs.value));
    case Type::Guid:
        return *(reinterpret_cast<const Guid *>(&value)) == *(reinterpret_cast<const Guid *>(&rhs.value));
    case Type::Str:
        return *(reinterpret_cast<const Str *>(value.ptr1)) == *(reinterpret_cast<const Str *>(rhs.value.ptr1));
    case Type::MinMaxCurve:
        return *(reinterpret_cast<const MinMaxCurve *>(value.ptr1)) == *(reinterpret_cast<const MinMaxCurve *>(rhs.value.ptr1));
    default:
        assert(0);
        return false;
    }

    return false;
}

Json::Value Variant::ToJsonValue() const {
    Json::Value value;

    switch (type) {
    case Type::Int:
        value = As<int>();
        break;
    case Type::Int64:
        value = As<int64_t>();
        break;
    case Type::Bool:
        value = As<bool>();
        break;
    case Type::Float:
        value = As<float>();
        break;
    case Type::Double:
        value = As<double>();
        break;
    case Type::VoidPtr: {
        char addr[32];
        snprintf(addr, sizeof(addr), "%p", As<void *>());
        value = addr;
        break;
    }
    case Type::Vec2:
        value = As<Vec2>().ToString(7);
        break;
    case Type::Vec3:
        value = As<Vec3>().ToString(7);
        break;
    case Type::Vec4:
        value = As<Vec4>().ToString(7);
        break;
    case Type::Color3:
        value = As<Color3>().ToString(7);
        break;
    case Type::Color4:
        value = As<Color4>().ToString(7);
        break;
    case Type::Mat2:
        value = As<Mat2>().ToString(7);
        break;
    case Type::Mat3:
        value = As<Mat3>().ToString(7);
        break;
    case Type::Mat4:
        value = As<Mat4>().ToString(7);
        break;
    case Type::Mat3x4:
        value = As<Mat3x4>().ToString(7);
        break;
    case Type::Angles:
        value = As<Angles>().ToString(7);
        break;
    case Type::Quat:
        value = As<Quat>().ToString(7);
        break;
    case Type::Point:
        value = As<Point>().ToString();
        break;
    case Type::PointF:
        value = As<PointF>().ToString();
        break;
    case Type::Rect:
        value = As<Rect>().ToString();
        break;
    case Type::RectF:
        value = As<RectF>().ToString();
        break;
    case Type::Guid:
        value = As<Guid>().ToString();
        break;
    case Type::Str:
        value = As<Str>().c_str();
        break;
    default:
        // Type::MinMaxCurve can not support here
        assert(0);
        break;
    }

    return value;
}

Variant Variant::FromString(Type::Enum type, const char *str) {
    Variant var;
    if (!var.SetFromString(type, str)) {
        return Variant();
    }
    return var;
}

Str Variant::ToString() const {
    switch (type) {
    case Type::Int:
        return Str(value.i1);
    case Type::Int64:
        return Str(*(reinterpret_cast<const int64_t *>(&value)));
    case Type::Bool:
        return Str(value.b1);
    case Type::Float:
        return Str(value.f1);
    case Type::Double:
        return Str(value.d1);
    case Type::VoidPtr: {
        char str[32];
        sscanf(str, "%p", (void **)&value.ptr1);
        return Str(str);
    }
    case Type::Vec2:
        return (reinterpret_cast<const Vec2 *>(&value))->ToString(7);
    case Type::Vec3:
        return (reinterpret_cast<const Vec3 *>(&value))->ToString(7);
    case Type::Vec4:
        return (reinterpret_cast<const Vec4 *>(&value))->ToString(7);
    case Type::Color3:
        return (reinterpret_cast<const Color3 *>(&value))->ToString(7);
    case Type::Color4:
        return (reinterpret_cast<const Color4 *>(&value))->ToString(7);
    case Type::Mat2:
        return (reinterpret_cast<const Mat2 *>(&value))->ToString(7);
    case Type::Mat3:
        return (reinterpret_cast<const Mat3 *>(value.ptr1))->ToString(7);
    case Type::Mat4:
        return (reinterpret_cast<const Mat4 *>(value.ptr1))->ToString(7);
    case Type::Mat3x4:
        return (reinterpret_cast<const Mat3x4 *>(value.ptr1))->ToString(7);
    case Type::Angles:
        return (reinterpret_cast<const Angles *>(&value))->ToString(7);
    case Type::Quat:
        return (reinterpret_cast<const Quat *>(&value))->ToString(7);
    case Type::Point:
        return (reinterpret_cast<const Point *>(&value))->ToString();
    case Type::PointF:
        return (reinterpret_cast<const PointF *>(&value))->ToString();
    case Type::Rect:
        return (reinterpret_cast<const Rect *>(&value))->ToString();
    case Type::RectF:
        return (reinterpret_cast<const RectF *>(&value))->ToString();
    case Type::Guid:
        return (reinterpret_cast<const Guid *>(&value))->ToString();
    case Type::Str:
        return *(reinterpret_cast<const Str *>(value.ptr1));
    default:
        // Type::MinMaxCurve can not support here
        return Str();
    }
}

BE_NAMESPACE_END
