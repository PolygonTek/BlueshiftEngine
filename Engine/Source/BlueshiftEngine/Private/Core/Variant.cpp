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

ObjectRef ObjectRef::empty;
ObjectRefArray ObjectRefArray::empty;

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
    case ObjectRefArrayType:
        delete reinterpret_cast<ObjectRefArray *>(value.ptr1);
        break;
    case MinMaxCurveType:
        delete reinterpret_cast<MinMaxCurve *>(value.ptr1);
        break;
    case VariantArrayType:
        (reinterpret_cast<VariantArray *>(&value))->~VariantArray();
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
    case ObjectRefArrayType:
        value.ptr1 = new ObjectRefArray;
        break;
    case MinMaxCurveType:
        value.ptr1 = new MinMaxCurve();
        break;
    case VariantArrayType:
        new(reinterpret_cast<VariantArray *>(&value))VariantArray();
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
    case VoidPtrType:
        *this = Variant((sizeof(void *) == sizeof(uint64_t) ? Str::ToUI64(str) : Str::ToUI32(str)));
        return true;
    case Vec2Type: {
        Vec2 vec2;
        sscanf(str, "%f %f", &vec2.x, &vec2.y);
        *this = vec2;
        return true;
    }
    case Vec3Type: {
        Vec3 vec3;
        sscanf(str, "%f %f %f", &vec3.x, &vec3.y, &vec3.z);
        *this = vec3;
        return true;
    }
    case Vec4Type: {
        Vec4 vec4;
        sscanf(str, "%f %f %f %f", &vec4.x, &vec4.y, &vec4.z, &vec4.w);
        *this = vec4;
        return true;
    }
    case Color3Type: {
        Color3 color3;
        sscanf(str, "%f %f %f", &color3.r, &color3.g, &color3.b);
        *this = color3;
        return true;
    }
    case Color4Type: {
        Color4 color4;
        sscanf(str, "%f %f %f %f", &color4.r, &color4.g, &color4.b, &color4.a);
        *this = color4;
        return true;
    }
    case Mat2Type: {
        Mat2 mat2;
        sscanf(str, "%f %f %f %f",
            &mat2[0].x, &mat2[0].y,
            &mat2[1].x, &mat2[1].y);
        *this = mat2;
        return true;
    }
    case Mat3Type: {
        Mat3 mat3;
        sscanf(str, "%f %f %f %f %f %f %f %f %f",
            &mat3[0].x, &mat3[0].y, &mat3[0].z,
            &mat3[1].x, &mat3[1].y, &mat3[1].z,
            &mat3[2].x, &mat3[2].y, &mat3[2].z);
        *this = mat3;
        return true;
    }
    case Mat4Type: {
        Mat4 mat4;
        sscanf(str, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
            &mat4[0].x, &mat4[0].y, &mat4[0].z, &mat4[0].w,
            &mat4[1].x, &mat4[1].y, &mat4[1].z, &mat4[1].w,
            &mat4[2].x, &mat4[2].y, &mat4[2].z, &mat4[2].w,
            &mat4[3].x, &mat4[3].y, &mat4[3].z, &mat4[3].w);
        *this = mat4;
        return true;
    }
    case Mat3x4Type: {
        Mat3x4 mat3x4;
        sscanf(str, "%f %f %f %f %f %f %f %f %f %f %f %f",
            &mat3x4[0].x, &mat3x4[0].y, &mat3x4[0].z, &mat3x4[0].w,
            &mat3x4[1].x, &mat3x4[1].y, &mat3x4[1].z, &mat3x4[1].w,
            &mat3x4[2].x, &mat3x4[2].y, &mat3x4[2].z, &mat3x4[2].w);
        *this = mat3x4;
        return true;
    }
    case AnglesType: {
        Angles angles;
        sscanf(str, "%f %f %f", &angles[0], &angles[1], &angles[2]);
        *this = angles;
        return true;
    }
    case QuatType: {
        Quat quat;
        sscanf(str, "%f %f %f %f", &quat[0], &quat[1], &quat[2], &quat[3]);
        *this = quat;
        return true;
    }
    case PointType: {
        Point point;
        sscanf(str, "%i %i", &point.x, &point.y);
        *this = point;
        return true;
    }
    case RectType: {
        Rect rect;
        sscanf(str, "%i %i %i %i", &rect.x, &rect.y, &rect.w, &rect.h);
        *this = rect;
        return true;
    }
    case GuidType: {
        *this = Guid::FromString(str);
        return true;
    }
    case StrType: {
        *this = Str(str);
        return true;
    }
    case ObjectRefType: {
        StrArray values;
        SplitStringIntoList(values, str, ";");
        if (values.Count() == 2) {
            auto *objectRef = reinterpret_cast<ObjectRef *>(&value);
            objectRef->metaObject = Object::FindMetaObject(values[0]);
            objectRef->objectGuid.SetFromString(values[1]);
        }
        return true;
    }
    default:
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
    case ObjectRefArrayType:
        *(reinterpret_cast<ObjectRefArray *>(value.ptr1)) = *(reinterpret_cast<const ObjectRefArray *>(rhs.value.ptr1));
        break;
    case VariantArrayType:
        *(reinterpret_cast<VariantArray *>(value.ptr1)) = *(reinterpret_cast<const VariantArray *>(rhs.value.ptr1));
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
    case ObjectRefType:
        return *(reinterpret_cast<const ObjectRef *>(&value)) == *(reinterpret_cast<const ObjectRef *>(&rhs.value));
    case ObjectRefArrayType:
        return *(reinterpret_cast<const ObjectRefArray *>(value.ptr1)) == *(reinterpret_cast<const ObjectRefArray *>(rhs.value.ptr1));
    case MinMaxCurveType:
        return *(reinterpret_cast<const MinMaxCurve *>(value.ptr1)) == *(reinterpret_cast<const MinMaxCurve *>(rhs.value.ptr1));
    case VariantArrayType:
        return *(reinterpret_cast<const VariantArray *>(value.ptr1)) == *(reinterpret_cast<const VariantArray *>(rhs.value.ptr1));
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
    case VoidPtrType:
        value = (intptr_t)As<void *>();
        break;
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
    case ObjectRefType: {
        auto &objectRef = As<ObjectRef>();
        value[0] = objectRef.metaObject->ClassName();
        value[1] = objectRef.objectGuid.ToString();
        break; 
    }
    case ObjectRefArrayType: {
        auto &objectRefArray = As<ObjectRefArray>();

        value.resize(objectRefArray.objectGuids.Count());
        for (int i = 0; i < objectRefArray.objectGuids.Count(); i++) {
            value[i] = objectRefArray.objectGuids[i].ToString();
        }
        break; 
    }
    default:
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
    case VoidPtrType:
        return Str((intptr_t)value.ptr1);
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
