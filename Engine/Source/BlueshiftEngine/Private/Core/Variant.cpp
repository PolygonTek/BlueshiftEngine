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

void Variant::SetType(Type type) {
    if (this->type == type) {
        return;
    }

    switch (this->type) {
    case StrType:
        delete reinterpret_cast<Str *>(value.ptr1);
        break;
    case Mat3Type:
        delete reinterpret_cast<Mat3 *>(value.ptr1);
        break;
    case Mat3x4Type:
        delete reinterpret_cast<Mat3x4 *>(value.ptr1);
        break;
    case Mat4Type:
        delete reinterpret_cast<Mat4 *>(value.ptr1);
        break;
    case MinMaxCurveType:
        delete reinterpret_cast<MinMaxCurve *>(value.ptr1);
        break;
    default:
        break;
    }

    this->type = type;

    switch (type) {
    case StrType:
        value.ptr1 = new Str();
        break;
    case Mat3Type:
        value.ptr1 = new Mat3();
        break;
    case Mat3x4Type:
        value.ptr1 = new Mat3x4();
        break;
    case Mat4Type:
        value.ptr1 = new Mat4();
        break;
    case MinMaxCurveType:
        value.ptr1 = new MinMaxCurve();
        break;
    default:
        break;
    }
}

bool Variant::SetFromString(Type type, const char *str) {
    switch (type) {
    case IntType:
        *this = Variant(atoi(str));
        return true;
    case Int64Type:
        *this = Variant(Str::ToI64(str));
        return true;
    case BoolType:
        *this = Variant(!Str::Cmp(str, "true"));
        return true;
    case FloatType:
        *this = Variant((float)atof(str));
        return true;
    case DoubleType:
        *this = Variant(atof(str));
        return true;
    case VoidPtrType:
        *this = Variant((sizeof(void *) == sizeof(uint64_t) ? Str::ToUI64(str) : Str::ToUI32(str)));
        return true;
    case Vec2Type:
        *this = Vec2::FromString(str);
        return true;
    case Vec3Type:
        *this = Vec3::FromString(str);
        return true;
    case Vec4Type:
        *this = Vec4::FromString(str);
        return true;
    case Color3Type:
        *this = Color3::FromString(str);
        return true;
    case Color4Type:
        *this = Color4::FromString(str);
        return true;
    case Mat2Type:
        *this = Mat2::FromString(str);
        return true;
    case Mat3Type:
        *this = Mat3::FromString(str);
        return true;
    case Mat4Type:
        *this = Mat4::FromString(str);
        return true;
    case Mat3x4Type:
        *this = Mat3x4::FromString(str);
        return true;
    case AnglesType:
        *this = Angles::FromString(str);
        return true;
    case QuatType:
        *this = Quat::FromString(str);
        return true;
    case PointType:
        *this = Point::FromString(str);
        return true;
    case RectType:
        *this = Rect::FromString(str);
        return true;
    case GuidType:
        *this = Guid::FromString(str);
        return true;
    case StrType:
        *this = Str(str);
        return true;
    default:
        // MinMaxCurveType can not support here
        assert(0);
        return false;
    }

    return false;
}

Variant &Variant::operator=(const Variant &rhs) {
    SetType(rhs.type);

    switch (type) {
    case StrType:
        *(reinterpret_cast<Str *>(value.ptr1)) = *(reinterpret_cast<const Str *>(rhs.value.ptr1));
        break;
    case Mat3Type:
        *(reinterpret_cast<Mat3 *>(value.ptr1)) = *(reinterpret_cast<const Mat3 *>(rhs.value.ptr1));
        break;
    case Mat3x4Type:
        *(reinterpret_cast<Mat3x4 *>(value.ptr1)) = *(reinterpret_cast<const Mat3x4 *>(rhs.value.ptr1));
        break;
    case Mat4Type:
        *(reinterpret_cast<Mat4 *>(value.ptr1)) = *(reinterpret_cast<const Mat4 *>(rhs.value.ptr1));
        break;
    case MinMaxCurveType:
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
    case IntType:
        return value.i1 == rhs.value.i1;
    case Int64Type:
        return *(reinterpret_cast<const int64_t *>(&value)) == *(reinterpret_cast<const int64_t *>(&rhs.value));
    case BoolType:
        return value.b1 == rhs.value.b1;
    case FloatType:
        return value.f1 == rhs.value.f1;
    case DoubleType:
        return value.d1 == rhs.value.d1;
    case VoidPtrType:
        return value.ptr1 == rhs.value.ptr2;
    case Vec2Type:
        return *(reinterpret_cast<const Vec2 *>(&value)) == *(reinterpret_cast<const Vec2 *>(&rhs.value));
    case Vec3Type:
        return *(reinterpret_cast<const Vec3 *>(&value)) == *(reinterpret_cast<const Vec3 *>(&rhs.value));
    case Vec4Type:
        return *(reinterpret_cast<const Vec4 *>(&value)) == *(reinterpret_cast<const Vec4 *>(&rhs.value));
    case Color3Type:
        return *(reinterpret_cast<const Color3 *>(&value)) == *(reinterpret_cast<const Color3 *>(&rhs.value));
    case Color4Type:
        return *(reinterpret_cast<const Color4 *>(&value)) == *(reinterpret_cast<const Color4 *>(&rhs.value));
    case Mat2Type:
        return *(reinterpret_cast<const Mat2 *>(&value)) == *(reinterpret_cast<const Mat2 *>(&rhs.value));
    case Mat3Type:
        return *(reinterpret_cast<const Mat3 *>(value.ptr1)) == *(reinterpret_cast<const Mat3 *>(rhs.value.ptr1));
    case Mat4Type:
        return *(reinterpret_cast<const Mat4 *>(value.ptr1)) == *(reinterpret_cast<const Mat4 *>(rhs.value.ptr1));
    case Mat3x4Type:
        return *(reinterpret_cast<const Mat3x4 *>(value.ptr1)) == *(reinterpret_cast<const Mat3x4 *>(rhs.value.ptr1));
    case AnglesType:
        return *(reinterpret_cast<const Angles *>(&value)) == *(reinterpret_cast<const Angles *>(&rhs.value));
    case QuatType:
        return *(reinterpret_cast<const Quat *>(&value)) == *(reinterpret_cast<const Quat *>(&rhs.value));
    case PointType:
        return *(reinterpret_cast<const Point *>(&value)) == *(reinterpret_cast<const Point *>(&rhs.value));
    case RectType:
        return *(reinterpret_cast<const Rect *>(&value)) == *(reinterpret_cast<const Rect *>(&rhs.value));
    case GuidType:
        return *(reinterpret_cast<const Guid *>(&value)) == *(reinterpret_cast<const Guid *>(&rhs.value));
    case StrType:
        return *(reinterpret_cast<const Str *>(value.ptr1)) == *(reinterpret_cast<const Str *>(rhs.value.ptr1));
    case MinMaxCurveType:
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
    case IntType:
        value = As<int>();
        break;
    case Int64Type:
        value = As<int64_t>();
        break;
    case BoolType:
        value = As<bool>();
        break;
    case FloatType:
        value = As<float>();
        break;
    case DoubleType:
        value = As<double>();
        break;
    case VoidPtrType: {
        char addr[32];
        snprintf(addr, sizeof(addr), "%p", As<void *>());
        value = addr;
        break;
    }
    case Vec2Type:
        value = As<Vec2>().ToString();
        break;
    case Vec3Type:
        value = As<Vec3>().ToString();
        break;
    case Vec4Type:
        value = As<Vec4>().ToString();
        break;
    case Color3Type:
        value = As<Color3>().ToString();
        break;
    case Color4Type:
        value = As<Color4>().ToString();
        break;
    case Mat2Type:
        value = As<Mat2>().ToString();
        break;
    case Mat3Type:
        value = As<Mat3>().ToString();
        break;
    case Mat4Type:
        value = As<Mat4>().ToString();
        break;
    case Mat3x4Type:
        value = As<Mat3x4>().ToString();
        break;
    case AnglesType:
        value = As<Angles>().ToString();
        break;
    case QuatType:
        value = As<Quat>().ToString();
        break;
    case PointType:
        value = As<Point>().ToString();
        break;
    case RectType:
        value = As<Rect>().ToString();
        break;
    case GuidType:
        value = As<Guid>().ToString();
        break;
    case StrType:
        value = As<Str>().c_str();
        break;
    default:
        // MinMaxCurveType can not support here
        assert(0);
        break;
    }

    return value;
}

Variant Variant::FromString(Type type, const char *str) {
    Variant var;
    if (!var.SetFromString(type, str)) {
        return Variant();
    }
    return var;
}

Str Variant::ToString() const {
    switch (type) {
    case IntType:
        return Str(value.i1);
    case Int64Type:
        return Str(*(reinterpret_cast<const int64_t *>(&value)));
    case BoolType:
        return Str(value.b1);
    case FloatType:
        return Str(value.f1);
    case DoubleType:
        return Str(value.d1);
    case VoidPtrType: {
        char str[32];
        sscanf(str, "%p", (void **)&value.ptr1);
        return Str(str);
    }
    case Vec2Type:
        return (reinterpret_cast<const Vec2 *>(&value))->ToString();
    case Vec3Type:
        return (reinterpret_cast<const Vec3 *>(&value))->ToString();
    case Vec4Type:
        return (reinterpret_cast<const Vec4 *>(&value))->ToString();
    case Color3Type:
        return (reinterpret_cast<const Color3 *>(&value))->ToString();
    case Color4Type:
        return (reinterpret_cast<const Color4 *>(&value))->ToString();
    case Mat2Type:
        return (reinterpret_cast<const Mat2 *>(&value))->ToString();
    case Mat3Type:
        return (reinterpret_cast<const Mat3 *>(value.ptr1))->ToString();
    case Mat4Type:
        return (reinterpret_cast<const Mat4 *>(value.ptr1))->ToString();
    case Mat3x4Type:
        return (reinterpret_cast<const Mat3x4 *>(value.ptr1))->ToString();
    case AnglesType:
        return (reinterpret_cast<const Angles *>(&value))->ToString();
    case QuatType:
        return (reinterpret_cast<const Quat *>(&value))->ToString();
    case PointType:
        return (reinterpret_cast<const Point *>(&value))->ToString();
    case RectType:
        return (reinterpret_cast<const Rect *>(&value))->ToString();
    case GuidType:
        return (reinterpret_cast<const Guid *>(&value))->ToString();
    case StrType:
        return *(reinterpret_cast<const Str *>(value.ptr1));
    default:
        // MinMaxCurveType can not support here
        return Str();
    }
}

BE_NAMESPACE_END
