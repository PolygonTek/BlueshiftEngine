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

BE_NAMESPACE_BEGIN

void Variant::SetType(Type type) {
    if (this->type == type) {
        return;
    }

    switch (this->type) {
    case VariantArrayType:
        (reinterpret_cast<VariantArray *>(&value))->~VariantArray();
        break;
    case ObjectRefType:
        delete reinterpret_cast<ObjectRef *>(value.ptr1);
        break;
    case ObjectRefArrayType:
        delete reinterpret_cast<ObjectRefArray *>(value.ptr1);
        break;
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
    }

    this->type = type;

    switch (type) {
    case VariantArrayType:
        new(reinterpret_cast<VariantArray *>(&value))VariantArray();
        break;
    case ObjectRefType:
        value.ptr1 = new ObjectRef;
        break;
    case ObjectRefArrayType:
        value.ptr1 = new ObjectRefArray;
        break;
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
    }
}

Variant &Variant::operator=(const Variant &rhs) {
    SetType(rhs.type);

    switch (type) {
    case VariantArrayType:
        *(reinterpret_cast<VariantArray *>(value.ptr1)) = *(reinterpret_cast<const VariantArray *>(rhs.value.ptr1));
        break;
    case ObjectRefType:
        *(reinterpret_cast<ObjectRef *>(value.ptr1)) = *(reinterpret_cast<const ObjectRef *>(rhs.value.ptr1));
        break;
    case ObjectRefArrayType:
        *(reinterpret_cast<ObjectRefArray *>(value.ptr1)) = *(reinterpret_cast<const ObjectRefArray *>(rhs.value.ptr1));
        break;
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
    case ObjectRefType:
        return *(reinterpret_cast<const ObjectRef *>(value.ptr1)) == *(reinterpret_cast<const ObjectRef *>(rhs.value.ptr1));
    case ObjectRefArrayType:
        return *(reinterpret_cast<const ObjectRefArray *>(value.ptr1)) == *(reinterpret_cast<const ObjectRefArray *>(rhs.value.ptr1));
    case MinMaxCurveType:
        return *(reinterpret_cast<const MinMaxCurve *>(value.ptr1)) == *(reinterpret_cast<const MinMaxCurve *>(rhs.value.ptr1));
    case VariantArrayType:
        return *(reinterpret_cast<const VariantArray *>(value.ptr1)) == *(reinterpret_cast<const VariantArray *>(rhs.value.ptr1));
    }

    return false;
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
        return Str();
    }
}

BE_NAMESPACE_END
